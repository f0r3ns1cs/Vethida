#include "TweakEngine.h"

#include <QtCore/QPair>

// Services. A service's startup type is the registry DWORD Services\<name>\Start
// (2=Auto, 3=Manual, 4=Disabled), so disabling is a registry write and reverting
// restores the service's default Start.
//
// Reverts use each service's typical Windows 10/11 default (Auto vs Manual).
// This is "good enough" - the service ends up in a working state - but is not a
// byte-exact restore of a non-standard prior value; a future snapshot-based
// revert (using the app's SnapshotStore) would capture the exact original.

namespace vth::tweaks::detail {

    namespace {

        RegistryOp svcStart(const QString& name, quint32 start, quint32 revertDefault) {
            return dwordOp(
                QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\%1").arg(name),
                QStringLiteral("Start"), start, revertDefault);
        }

        // Disable (=4) a set of services, reverting each to its (default, typical) start.
        QVector<RegistryOp> disableAll(const QVector<QPair<QString, quint32>>& svcDefaults) {
            QVector<RegistryOp> ops;
            for (const auto& sd : svcDefaults)
                ops.push_back(svcStart(sd.first, 4, sd.second));
            return ops;
        }

    }


    void appendServices(QVector<TweakSpec>& c) {
        {
            TweakSpec t;
            t.id = QStringLiteral("svc.telemetry-off");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Telemetry");
            t.name = QStringLiteral("Disable telemetry and diagnostics services");
            t.description = QStringLiteral("Disables the telemetry and diagnostics services: Connected User Experiences & Telemetry (DiagTrack), the diagnostics-hub collector, dmwappushservice, Windows Error Reporting and the Program Compatibility Assistant, plus the Diagnostic Service/System Host (WDI). Cuts telemetry uploads, but also disables the built-in network troubleshooters and automatic crash reporting.");
            t.iconName = QStringLiteral("eye-off");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.registryOps = disableAll({
                { QStringLiteral("DiagTrack"), 2 },
                { QStringLiteral("diagnosticshub.standardcollector.service"), 3 },
                { QStringLiteral("dmwappushservice"), 3 },
                { QStringLiteral("WdiServiceHost"), 3 },
                { QStringLiteral("WdiSystemHost"), 3 },
                { QStringLiteral("WerSvc"), 3 },
                { QStringLiteral("PcaSvc"), 2 },
            });
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("svc.xbox-off");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Xbox & Game Bar");
            t.name = QStringLiteral("Disable Xbox services");
            t.description = QStringLiteral(
                "Disables Xbox Live auth, game save, networking and accessory services.");
            t.iconName = QStringLiteral("gamepad-2");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = { QStringLiteral("Breaks Xbox app sign-in, game save sync and the Xbox controller app.") };
            t.registryOps = disableAll({
                { QStringLiteral("XblAuthManager"), 3 },
                { QStringLiteral("XblGameSave"), 3 },
                { QStringLiteral("XboxNetApiSvc"), 3 },
                { QStringLiteral("XboxGipSvc"), 3 },
            });
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("svc.print-off");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Printing");
            t.name = QStringLiteral("Disable printing services");
            t.description = QStringLiteral("Disables the Print Spooler, printer notifications and Fax.");
            t.iconName = QStringLiteral("printer");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = { QStringLiteral("Breaks all printing, including \"print to PDF\" in some apps.") };
            t.registryOps = disableAll({
                { QStringLiteral("Spooler"), 2 },
                { QStringLiteral("PrintNotify"), 3 },
                { QStringLiteral("Fax"), 3 },
            });
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("svc.sysmain-off");
            t.category = QStringLiteral("general");
            t.subCategory = QStringLiteral("SysMain / SuperFetch");
            t.name = QStringLiteral("Disable SysMain (SuperFetch)");
            t.description = QStringLiteral(
                "Stops the Windows service that keeps your most-used programs loaded "
                "in spare memory. Frees that memory and the disk activity behind it.");
            t.iconName = QStringLiteral("zap");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "SSD: safe and removes background I/O. HDD: hurts app launch times - leave on.");
            t.registryOps = disableAll({ { QStringLiteral("SysMain"), 2 } });
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("svc.search-off");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Search & Cortana");
            t.name = QStringLiteral("Disable Windows Search");
            t.description = QStringLiteral("Disables the Windows Search indexer service.");
            t.iconName = QStringLiteral("search");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = { QStringLiteral("Breaks Start-menu search, Outlook search and file-content search.") };
            t.concern = QStringLiteral(
                "operator preference: indexing costs background I/O, but losing search is a "
                "real usability hit on most desktops.");
            t.registryOps = disableAll({ { QStringLiteral("WSearch"), 2 } });
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("svc.edge-update-off");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Edge");
            t.name = QStringLiteral("Disable Edge update services");
            t.description = QStringLiteral("Disables the Microsoft Edge auto-update and elevation services (edgeupdate, edgeupdatem, MicrosoftEdgeElevationService). Edge and WebView2 stop updating themselves; only apply if you do not use Edge as a browser.");
            t.iconName = QStringLiteral("globe");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = { QStringLiteral("Edge will no longer update itself automatically.") };
            t.registryOps = disableAll({
                { QStringLiteral("edgeupdate"), 3 },
                { QStringLiteral("edgeupdatem"), 3 },
                { QStringLiteral("MicrosoftEdgeElevationService"), 3 },
            });
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("svc.hyperv-off");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Virtualization");
            t.name = QStringLiteral("Disable Hyper-V services");
            t.description = QStringLiteral("Disables the Hyper-V host and integration services.");
            t.iconName = QStringLiteral("box");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "only if you don't use Hyper-V, WSL2, Windows Sandbox, or VBS-based security.");
            t.registryOps = disableAll({
                { QStringLiteral("HvHost"), 3 },
                { QStringLiteral("vmickvpexchange"), 3 },
                { QStringLiteral("vmicguestinterface"), 3 },
                { QStringLiteral("vmicshutdown"), 3 },
                { QStringLiteral("vmicheartbeat"), 3 },
                { QStringLiteral("vmicvmsession"), 3 },
                { QStringLiteral("vmicrdv"), 3 },
                { QStringLiteral("vmictimesync"), 3 },
            });
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("svc.sensors-off");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Sensors");
            t.name = QStringLiteral("Disable sensor services");
            t.description = QStringLiteral("Disables the sensor data/monitoring/service trio.");
            t.iconName = QStringLiteral("compass");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.registryOps = disableAll({
                { QStringLiteral("SensorDataService"), 3 },
                { QStringLiteral("SensrSvc"), 3 },
                { QStringLiteral("SensorService"), 3 },
            });
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("svc.geolocation-off");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Location & privacy");
            t.name = QStringLiteral("Disable geolocation and maps");
            t.description = QStringLiteral("Disables the geolocation service and downloaded-maps manager.");
            t.iconName = QStringLiteral("map-pin");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = { QStringLiteral("Apps that use location (Weather, Maps) stop working.") };
            t.registryOps = disableAll({
                { QStringLiteral("lfsvc"), 3 },
                { QStringLiteral("MapsBroker"), 3 },
            });
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("svc.bluetooth-off");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Bluetooth");
            t.name = QStringLiteral("Disable Bluetooth services");
            t.description = QStringLiteral("Disables the Bluetooth support and audio (AVCTP) services.");
            t.iconName = QStringLiteral("bluetooth");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = { QStringLiteral("All Bluetooth devices stop working until re-enabled.") };
            t.concern = QStringLiteral("only if you use no Bluetooth peripherals.");
            t.registryOps = disableAll({
                { QStringLiteral("bthserv"), 3 },
                { QStringLiteral("BthAvctpSvc"), 3 },
            });
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("svc.disable-legacy-drivers");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Legacy drivers & protocols");
            t.name = QStringLiteral("Disable legacy protocol drivers");
            t.description = QStringLiteral("Disables a set of legacy/rarely-used drivers and services: the IPv6 stack (Tcpip6) and IP Helper (iphlpsvc), the PC-speaker beep, virtual NDIS bus, NetBIOS, parallel/serial/IrDA ports, the Remote Desktop listener (TermDD), the Bluetooth port driver, Fax and the smart-card reader. Situational: skip it if you rely on IPv6, a VPN, incoming Remote Desktop, Bluetooth or smart cards.");
            t.iconName = QStringLiteral("ban");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "situational: disables Fax, smart-card and RDP listener support along with "
                "unused legacy ports; skip if you use any of those.");
            t.registryOps = disableAll({
                { QStringLiteral("Tcpip6"), 3 },
                { QStringLiteral("Beep"), 3 },
                { QStringLiteral("NdisVirtualBus"), 3 },
                { QStringLiteral("NetBIOS"), 3 },
                { QStringLiteral("Parport"), 3 },
                { QStringLiteral("Serial"), 3 },
                { QStringLiteral("IrDA"), 3 },
                { QStringLiteral("TermDD"), 3 },
                { QStringLiteral("BTHPORT"), 3 },
                { QStringLiteral("Fax"), 3 },
                { QStringLiteral("iphlpsvc"), 3 },
                { QStringLiteral("SCardSvr"), 3 },
            });
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("svc.xbox-presence-writer-off");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Xbox & Game Bar");
            t.name = QStringLiteral("Disable Xbox presence writer activation");
            t.description = QStringLiteral(
                "Blocks activation of the Game Bar presence-writer WinRT class that reports "
                "in-game presence to Xbox Live.");
            t.iconName = QStringLiteral("gamepad-2");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\WindowsRuntime\\ActivatableClassId\\"
                                       "Windows.Gaming.GameBar.PresenceServer.Internal.PresenceWriter"),
                        QStringLiteral("ActivationType"), 0, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("svc.svchost-split-share");
            t.category = QStringLiteral("general");
            t.name = QStringLiteral("Share svchost processes");
            t.description = QStringLiteral(
                "Raises the svchost split threshold so services share host processes, "
                "cutting process count and context switches.");
            t.iconName = QStringLiteral("layers");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "trades process isolation for fewer processes; the exact threshold matters "
                "less than enabling sharing.");
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control"),
                        QStringLiteral("SvcHostSplitThresholdInKB"), 67108864u, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("svc.disable-broad-services");
            t.category = QStringLiteral("debloating");
            t.name = QStringLiteral("Disable broad background services");
            t.description = QStringLiteral(
                "Turns off background services a desktop PC usually has no use for: "
                "printing, Bluetooth, sensors, Xbox sign-in, fingerprint and PIN sign-in, "
                "virtual machines, and older networking protocols. Turn this off again if "
                "you add a printer or a Bluetooth device.");
            t.iconName = QStringLiteral("ban");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "very broad: revert sets services to demand-start, not their captured original "
                "start type. Includes services other tweaks already cover individually "
                "(SysMain, bthserv, MapsBroker, lfsvc, SensorDataService/SensrSvc/SensorService) "
                "- writing the same value twice is harmless. Review the full list before "
                "applying on a machine that uses Hyper-V, WSL2, smart cards, biometrics, "
                "Bluetooth or printing.");
            t.registryOps = disableAll({
                { QStringLiteral("luafv"), 3 }, { QStringLiteral("mpsdrv"), 3 },
                { QStringLiteral("MsLldp"), 3 }, { QStringLiteral("NdisVirtualBus"), 3 },
                { QStringLiteral("NdisWan"), 3 }, { QStringLiteral("Ndu"), 3 },
                { QStringLiteral("PEAUTH"), 3 }, { QStringLiteral("PptpMiniport"), 3 },
                { QStringLiteral("RasAgileVpn"), 3 }, { QStringLiteral("Rasl2tp"), 3 },
                { QStringLiteral("RasPppoe"), 3 }, { QStringLiteral("RasSstp"), 3 },
                { QStringLiteral("rspndr"), 3 }, { QStringLiteral("tcpipreg"), 3 },
                { QStringLiteral("tunnel"), 3 }, { QStringLiteral("lltdio"), 3 },
                { QStringLiteral("acpipagr"), 3 }, { QStringLiteral("afunix"), 3 },
                { QStringLiteral("AsyncMac"), 3 }, { QStringLiteral("bowser"), 3 },
                { QStringLiteral("NetBIOS"), 3 }, { QStringLiteral("NetBT"), 3 },
                { QStringLiteral("SgrmAgent"), 3 }, { QStringLiteral("wanarp"), 3 },
                { QStringLiteral("Wanarpv6"), 3 }, { QStringLiteral("wcifs"), 3 },
                { QStringLiteral("wuauserv"), 3 }, { QStringLiteral("bits"), 3 },
                { QStringLiteral("dosvc"), 3 }, { QStringLiteral("WaaSMedicSvc"), 3 },
                { QStringLiteral("UsoSvc"), 3 }, { QStringLiteral("WindowsTrustedRTProxy"), 3 },
                { QStringLiteral("WmiAcpi"), 3 }, { QStringLiteral("Wof"), 3 },
                { QStringLiteral("fhsvc"), 3 }, { QStringLiteral("MicrosoftEdgeElevationService"), 3 },
                { QStringLiteral("edgeupdate"), 3 }, { QStringLiteral("edgeupdatem"), 3 },
                { QStringLiteral("FDResPub"), 3 }, { QStringLiteral("TapiSrv"), 3 },
                { QStringLiteral("Themes"), 3 }, { QStringLiteral("StorSvc"), 3 },
                { QStringLiteral("TieringEngineService"), 3 }, { QStringLiteral("ALG"), 3 },
                { QStringLiteral("AssignedAccessManagerSvc"), 3 }, { QStringLiteral("LicenseManager"), 3 },
                { QStringLiteral("AppXSVC"), 3 }, { QStringLiteral("ClipSVC"), 3 },
                { QStringLiteral("autotimesvc"), 3 }, { QStringLiteral("BthAvctpSvc"), 3 },
                { QStringLiteral("BITS"), 3 }, { QStringLiteral("BDESVC"), 3 },
                { QStringLiteral("wbengine"), 3 }, { QStringLiteral("RemoteRegistry"), 3 },
                { QStringLiteral("BTAGService"), 3 }, { QStringLiteral("bthserv"), 3 },
                { QStringLiteral("BluetoothUserService"), 3 }, { QStringLiteral("PeerDistSvc"), 3 },
                { QStringLiteral("CertPropSvc"), 3 }, { QStringLiteral("DmEnrollmentSvc"), 3 },
                { QStringLiteral("diagsvc"), 3 }, { QStringLiteral("DPS"), 3 },
                { QStringLiteral("WdiServiceHost"), 3 }, { QStringLiteral("WdiSystemHost"), 3 },
                { QStringLiteral("DisplayEnhancementService"), 3 }, { QStringLiteral("SENS"), 3 },
                { QStringLiteral("MapsBroker"), 3 }, { QStringLiteral("EntAppSvc"), 3 },
                { QStringLiteral("Fax"), 3 }, { QStringLiteral("BcastDVRUserService"), 3 },
                { QStringLiteral("lfsvc"), 3 }, { QStringLiteral("GraphicsPerfSvc"), 3 },
                { QStringLiteral("SharedAccess"), 3 }, { QStringLiteral("wlpasvc"), 3 },
                { QStringLiteral("wlidsvc"), 3 }, { QStringLiteral("diagnosticshub.standardcollector.service"), 3 },
                { QStringLiteral("MsKeyboardFilter"), 3 }, { QStringLiteral("InstallService"), 3 },
                { QStringLiteral("NaturalAuthentication"), 3 }, { QStringLiteral("TabletInputService"), 3 },
                { QStringLiteral("NetTcpPortSharing"), 3 }, { QStringLiteral("Netlogon"), 3 },
                { QStringLiteral("NcdAutoSetup"), 3 }, { QStringLiteral("NcbService"), 3 },
                { QStringLiteral("NcaSvc"), 3 }, { QStringLiteral("CscService"), 3 },
                { QStringLiteral("ssh-agent"), 3 }, { QStringLiteral("defragsvc"), 3 },
                { QStringLiteral("WpcMonSvc"), 3 }, { QStringLiteral("SEMgrSvc"), 3 },
                { QStringLiteral("PhoneSvc"), 3 }, { QStringLiteral("WPDBusEnum"), 3 },
                { QStringLiteral("Spooler"), 3 }, { QStringLiteral("PrintNotify"), 3 },
                { QStringLiteral("PrintWorkflowUserSvc"), 3 }, { QStringLiteral("SessionEnv"), 3 },
                { QStringLiteral("TermService"), 3 }, { QStringLiteral("RetailDemo"), 3 },
                { QStringLiteral("RemoteAccess"), 3 }, { QStringLiteral("seclogon"), 3 },
                { QStringLiteral("SstpSvc"), 3 }, { QStringLiteral("LanmanServer"), 3 },
                { QStringLiteral("shpamsvc"), 3 }, { QStringLiteral("hidserv"), 3 },
                { QStringLiteral("ShellHWDetection"), 3 }, { QStringLiteral("SCardSvr"), 3 },
                { QStringLiteral("ScDeviceEnum"), 3 }, { QStringLiteral("SCPolicySvc"), 3 },
                { QStringLiteral("SNMPTRAP"), 3 }, { QStringLiteral("SysMain"), 3 },
                { QStringLiteral("WMPNetworkSvc"), 3 }, { QStringLiteral("icssvc"), 3 },
                { QStringLiteral("spectrum"), 3 }, { QStringLiteral("W32Time"), 3 },
                { QStringLiteral("RmSvc"), 3 }, { QStringLiteral("PNRPsvc"), 3 },
                { QStringLiteral("AxInstSV"), 3 }, { QStringLiteral("AJRouter"), 3 },
                { QStringLiteral("WEPHOSTSVC"), 3 }, { QStringLiteral("p2psvc"), 3 },
                { QStringLiteral("p2pimsvc"), 3 }, { QStringLiteral("SensorDataService"), 3 },
                { QStringLiteral("SensrSvc"), 3 }, { QStringLiteral("SensorService"), 3 },
                { QStringLiteral("perceptionsimulation"), 3 }, { QStringLiteral("OneSyncSvc"), 3 },
                { QStringLiteral("UnistoreSvc"), 3 }, { QStringLiteral("WpnUserService"), 3 },
                { QStringLiteral("DevicesFlowUserSvc"), 3 }, { QStringLiteral("ConsentUxUserSvc"), 3 },
                { QStringLiteral("PimIndexMaintenanceSvc"), 3 }, { QStringLiteral("DevicePickerUserSvc"), 3 },
                { QStringLiteral("CaptureService"), 3 }, { QStringLiteral("clipsvc"), 3 },
                { QStringLiteral("licensemanager"), 3 }, { QStringLiteral("appxsvc"), 3 },
                { QStringLiteral("MessagingService"), 3 }, { QStringLiteral("WwanSvc"), 3 },
                { QStringLiteral("PenService"), 3 }, { QStringLiteral("lmhosts"), 3 },
                { QStringLiteral("UserDataSvc"), 3 }, { QStringLiteral("cbdhsvc"), 3 },
                { QStringLiteral("CDPSvc"), 3 }, { QStringLiteral("CDPUserSvc"), 3 },
                { QStringLiteral("HvHost"), 3 }, { QStringLiteral("vmickvpexchange"), 3 },
                { QStringLiteral("vmicguestinterface"), 3 }, { QStringLiteral("vmicshutdown"), 3 },
                { QStringLiteral("vmicheartbeat"), 3 }, { QStringLiteral("vmcompute"), 3 },
                { QStringLiteral("vmicvmsession"), 3 }, { QStringLiteral("vmicrdv"), 3 },
                { QStringLiteral("vmictimesync"), 3 }, { QStringLiteral("vmms"), 3 },
                { QStringLiteral("vmicvss"), 3 }, { QStringLiteral("CloudBackupRestoreSvc"), 3 },
                { QStringLiteral("QWAVE"), 3 }, { QStringLiteral("IpxlatCfgSvc"), 3 },
                { QStringLiteral("pla"), 3 }, { QStringLiteral("SSDPSRV"), 3 },
                { QStringLiteral("WiaRpc"), 3 }, { QStringLiteral("upnphost"), 3 },
                { QStringLiteral("VSS"), 3 }, { QStringLiteral("VacSvc"), 3 },
                { QStringLiteral("WalletService"), 3 }, { QStringLiteral("WarpJITSvc"), 3 },
                { QStringLiteral("TokenBroker"), 3 }, { QStringLiteral("WFDSConMgrSvc"), 3 },
                { QStringLiteral("MSiSCSI"), 3 }, { QStringLiteral("SDRSVC"), 3 },
                { QStringLiteral("tzautoupdate"), 3 }, { QStringLiteral("wisvc"), 3 },
                { QStringLiteral("SharedRealitySvc"), 3 }, { QStringLiteral("TroubleshootingSvc"), 3 },
                { QStringLiteral("RasAuto"), 3 }, { QStringLiteral("Wcncsvc"), 3 },
                { QStringLiteral("FrameServer"), 3 },
            });
            c.push_back(t);
        }
    }

}
