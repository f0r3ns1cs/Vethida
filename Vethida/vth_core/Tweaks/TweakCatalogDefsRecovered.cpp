#include "TweakEngine.h"

namespace vth::tweaks::detail {
void appendDefsRecovered(QVector<TweakSpec>& c) {
    const auto task = [&c](const QString& id, const QString& name, const QString& desc,
                           const QString& taskPath) {
        TweakSpec t;
        t.id = id;
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Scheduled Tasks");
        t.name = name;
        t.description = desc;
        t.iconName = QStringLiteral("clock");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        CommandOp op;
        op.program = QStringLiteral("schtasks");
        op.applyArgs  = { QStringLiteral("/Change"), QStringLiteral("/TN"), taskPath, QStringLiteral("/Disable") };
        op.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), taskPath, QStringLiteral("/Enable") };
        t.commandOps.push_back(op);
        c.push_back(t);
    };

    task(QStringLiteral("tasks.disable-ceip-consolidator"),
         QStringLiteral("Disable CEIP Consolidator task"),
         QStringLiteral("Disables the Customer Experience Improvement Program Consolidator scheduled task, which collects and uploads CEIP usage data."),
         QStringLiteral("\\Microsoft\\Windows\\Customer Experience Improvement Program\\Consolidator"));

    task(QStringLiteral("tasks.disable-compat-appraiser"),
         QStringLiteral("Disable Microsoft Compatibility Appraiser task"),
         QStringLiteral("Disables the Application Experience Compatibility Appraiser task, which inventories installed software and hardware for compatibility telemetry."),
         QStringLiteral("\\Microsoft\\Windows\\Application Experience\\Microsoft Compatibility Appraiser"));

    task(QStringLiteral("tasks.disable-error-report-queue"),
         QStringLiteral("Disable Windows Error Reporting upload task"),
         QStringLiteral("Disables the Windows Error Reporting QueueReporting task, which uploads queued error reports to Microsoft."),
         QStringLiteral("\\Microsoft\\Windows\\Windows Error Reporting\\QueueReporting"));

    task(QStringLiteral("tasks.disable-feedback-siuf"),
         QStringLiteral("Disable Feedback Hub telemetry task"),
         QStringLiteral("Disables the Feedback Siuf DmClient task, which schedules feedback prompts and uploads diagnostic feedback data."),
         QStringLiteral("\\Microsoft\\Windows\\Feedback\\Siuf\\DmClient"));

    task(QStringLiteral("tasks.disable-maps-update"),
         QStringLiteral("Disable offline Maps update task"),
         QStringLiteral("Disables the Maps MapsUpdateTask, which downloads offline map data updates in the background."),
         QStringLiteral("\\Microsoft\\Windows\\Maps\\MapsUpdateTask"));

    task(QStringLiteral("tasks.disable-program-data-updater"),
         QStringLiteral("Disable Program Data Updater task"),
         QStringLiteral("Disables the Application Experience ProgramDataUpdater task, which collects program telemetry for the compatibility service."),
         QStringLiteral("\\Microsoft\\Windows\\Application Experience\\ProgramDataUpdater"));

    task(QStringLiteral("tasks.disable-startup-app-task"),
         QStringLiteral("Disable startup-app telemetry task"),
         QStringLiteral("Disables the Application Experience StartupAppTask, which measures and reports startup application performance."),
         QStringLiteral("\\Microsoft\\Windows\\Application Experience\\StartupAppTask"));

    task(QStringLiteral("tasks.disable-usb-ceip"),
         QStringLiteral("Disable USB telemetry task (UsbCeip)"),
         QStringLiteral("Disables the UsbCeip task, which collects USB device telemetry for the Customer Experience Improvement Program."),
         QStringLiteral("\\Microsoft\\Windows\\Customer Experience Improvement Program\\UsbCeip"));

    const auto reg = [&c](const QString& id, const QString& cat, const QString& sub,
                          const QString& name, const QString& desc, Impact impact, bool admin,
                          const QString& iconName, const QString& path, const QString& value,
                          quint32 applied, std::optional<quint32> revert,
                          const QStringList& warnings) {
        TweakSpec t;
        t.id = id;
        t.category = cat;
        t.subCategory = sub;
        t.name = name;
        t.description = desc;
        t.iconName = iconName;
        t.impact = impact;
        t.requiresAdmin = admin;
        t.warnings = warnings;
        t.registryOps = { dwordOp(path, value, applied, revert) };
        c.push_back(t);
    };

    reg(QStringLiteral("gpu.nvidia.fts_gr535_disable"), QStringLiteral("advanced"),
        QStringLiteral("NVIDIA"), QStringLiteral("Disable NVIDIA GR535 legacy sharpening"),
        QStringLiteral("Sets EnableGR535=0 on the NVIDIA display driver to disable the legacy GR535 image-sharpening path."),
        Impact::Low, true, QStringLiteral("cpu"),
        QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\nvlddmkm\\FTS"),
        QStringLiteral("EnableGR535"), 0u, std::nullopt, {});

    reg(QStringLiteral("gpu.nvidia-per-cpu-core-dpc"), QStringLiteral("advanced"),
        QStringLiteral("NVIDIA"), QStringLiteral("NVIDIA per-CPU-core DPC handling"),
        QStringLiteral("Enables RmGpsPsEnablePerCpuCoreDpc on the NVIDIA driver so GPU deferred procedure calls spread across CPU cores."),
        Impact::Low, true, QStringLiteral("cpu"),
        QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\nvlddmkm"),
        QStringLiteral("RmGpsPsEnablePerCpuCoreDpc"), 1u, std::nullopt, {});



    reg(QStringLiteral("perf.mmcss-always-on"), QStringLiteral("advanced"),
        QStringLiteral("Multimedia"), QStringLiteral("Force MMCSS always-on"),
        QStringLiteral("Sets AlwaysOn=1 on the Multimedia System Profile so the Multimedia Class Scheduler Service stays active."),
        Impact::Low, true, QStringLiteral("activity"),
        QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile"),
        QStringLiteral("AlwaysOn"), 1u, std::nullopt, {});

    reg(QStringLiteral("priv.disable-toast-notifications"), QStringLiteral("general"),
        QStringLiteral("Notifications"), QStringLiteral("Disable Windows toast notifications"),
        QStringLiteral("Sets NOC_GLOBAL_SETTING_TOASTS_ENABLED=0 to turn off Windows toast notifications globally for the current user."),
        Impact::Low, false, QStringLiteral("bell-off"),
        QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Notifications\\Settings"),
        QStringLiteral("NOC_GLOBAL_SETTING_TOASTS_ENABLED"), 0u, std::nullopt, {});

    // Single-service disable: Start=4 (disabled) on apply, Start=3 (manual) on revert.
    const auto svc = [&c](const QString& id, const QString& cat, const QString& name,
                          const QString& desc, Impact impact, const QString& service,
                          const QStringList& warnings) {
        TweakSpec t;
        t.id = id;
        t.category = cat;
        t.name = name;
        t.description = desc;
        t.iconName = QStringLiteral("settings");
        t.impact = impact;
        t.requiresAdmin = true;
        t.warnings = warnings;
        t.registryOps = { dwordOp(
            QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\") + service,
            QStringLiteral("Start"), 4u, 3u) };
        c.push_back(t);
    };

    svc(QStringLiteral("svc.disable-ajrouter"), QStringLiteral("advanced"),
        QStringLiteral("Disable AllJoyn Router"),
        QStringLiteral("IoT device-discovery framework (AllJoyn / AllSeen Alliance) used for smart-home device pairing. Safe to disable without integrated IoT devices."),
        Impact::Low, QStringLiteral("AJRouter"), {});
    svc(QStringLiteral("svc.disable-alg"), QStringLiteral("advanced"),
        QStringLiteral("Disable Application Layer Gateway"),
        QStringLiteral("Provides third-party protocol plug-in support for Internet Connection Sharing and the Windows firewall. Rarely required on a single PC."),
        Impact::Low, QStringLiteral("ALG"), {});
    svc(QStringLiteral("svc.disable-axinstsv"), QStringLiteral("advanced"),
        QStringLiteral("Disable ActiveX Installer service"),
        QStringLiteral("Installs ActiveX controls requested by standard user accounts. Legacy Internet Explorer feature."),
        Impact::Low, QStringLiteral("AxInstSV"), {});
    svc(QStringLiteral("svc.disable-cscservice"), QStringLiteral("advanced"),
        QStringLiteral("Disable Offline Files"),
        QStringLiteral("Caches network share files for offline access. An enterprise roaming feature unused on home PCs."),
        Impact::Low, QStringLiteral("CscService"), {});
    svc(QStringLiteral("svc.disable-lmhosts"), QStringLiteral("advanced"),
        QStringLiteral("Disable TCP/IP NetBIOS Helper"),
        QStringLiteral("Legacy NetBIOS-over-TCP/IP name resolution and LMHOSTS lookup. Not needed on modern networks."),
        Impact::Low, QStringLiteral("lmhosts"), {});
    svc(QStringLiteral("svc.disable-nettcpportsharing"), QStringLiteral("advanced"),
        QStringLiteral("Disable Net.Tcp Port Sharing"),
        QStringLiteral("Lets multiple processes share a single Net.Tcp port. A WCF developer feature."),
        Impact::Low, QStringLiteral("NetTcpPortSharing"), {});
    svc(QStringLiteral("svc.disable-qwave"), QStringLiteral("advanced"),
        QStringLiteral("Disable Quality Windows Audio Video Experience (qWAVE)"),
        QStringLiteral("Quality of service for streaming audio and video across a home network. Off by default and rarely used."),
        Impact::Low, QStringLiteral("QWAVE"), {});
    svc(QStringLiteral("svc.disable-sensorservice"), QStringLiteral("advanced"),
        QStringLiteral("Disable Sensor Service"),
        QStringLiteral("Manages device sensors such as ambient-light and orientation. Desktop PCs have no such sensors."),
        Impact::Low, QStringLiteral("SensorService"), {});
    svc(QStringLiteral("svc.disable-shared-access"), QStringLiteral("advanced"),
        QStringLiteral("Disable Internet Connection Sharing (ICS)"),
        QStringLiteral("Shares this PC's internet connection with other devices. Rarely used on a personal machine."),
        Impact::Low, QStringLiteral("SharedAccess"), {});
    svc(QStringLiteral("svc.disable-snmptrap"), QStringLiteral("advanced"),
        QStringLiteral("Disable SNMP Trap"),
        QStringLiteral("Receives SNMP trap messages for enterprise network monitoring."),
        Impact::Low, QStringLiteral("SNMPTRAP"), {});
    svc(QStringLiteral("svc.disable-ssdpsrv"), QStringLiteral("advanced"),
        QStringLiteral("Disable SSDP Discovery"),
        QStringLiteral("Discovers UPnP devices on the network. Disable together with UPnP Device Host."),
        Impact::Low, QStringLiteral("SSDPSRV"), {});
    svc(QStringLiteral("svc.disable-tzautoupdate"), QStringLiteral("advanced"),
        QStringLiteral("Disable Auto Time Zone Updater"),
        QStringLiteral("Automatically sets the time zone from your location. Disabled by default in Windows."),
        Impact::Low, QStringLiteral("tzautoupdate"), {});
    svc(QStringLiteral("svc.disable-upnphost"), QStringLiteral("advanced"),
        QStringLiteral("Disable UPnP Device Host"),
        QStringLiteral("Hosts UPnP devices on this computer for other network devices to find."),
        Impact::Low, QStringLiteral("upnphost"), {});
    svc(QStringLiteral("svc.disable-userdatasvc"), QStringLiteral("advanced"),
        QStringLiteral("Disable User Data Access"),
        QStringLiteral("Gives apps access to contacts, calendar and other user data. Disable if apps do not need it."),
        Impact::Low, QStringLiteral("UserDataSvc"), {});
    svc(QStringLiteral("svc.disable-walletservice"), QStringLiteral("advanced"),
        QStringLiteral("Disable WalletService"),
        QStringLiteral("Backend service for the Windows Wallet app."),
        Impact::Low, QStringLiteral("WalletService"), {});

    svc(QStringLiteral("perf.disable_sysmain"), QStringLiteral("advanced"),
        QStringLiteral("Disable SysMain (Superfetch)"),
        QStringLiteral("Superfetch/SysMain preloads frequently used apps into RAM. Helpful on HDDs, unnecessary on SSDs."),
        Impact::Medium, QStringLiteral("SysMain"), {});
    svc(QStringLiteral("perf.disable-prefetch"), QStringLiteral("advanced"),
        QStringLiteral("Disable Prefetch / Superfetch (SysMain)"),
        QStringLiteral("Disables the Prefetch/Superfetch (SysMain) service that preloads apps based on usage patterns."),
        Impact::Medium, QStringLiteral("SysMain"), {});
    svc(QStringLiteral("perf.disable-search-indexer"), QStringLiteral("advanced"),
        QStringLiteral("Disable Windows Search indexer (WSearch)"),
        QStringLiteral("Windows Search indexer that continuously indexes files. Disabling frees CPU and disk at the cost of slower search."),
        Impact::Medium, QStringLiteral("WSearch"),
        { QStringLiteral("File search becomes slower without the index.") });
    svc(QStringLiteral("perf.disable-windows-error-reporting"), QStringLiteral("advanced"),
        QStringLiteral("Disable Windows Error Reporting service (WerSvc)"),
        QStringLiteral("Windows Error Reporting service that queues and uploads crash reports to Microsoft."),
        Impact::Low, QStringLiteral("WerSvc"), {});

    svc(QStringLiteral("debloat.disable_print_spooler"), QStringLiteral("debloating"),
        QStringLiteral("Disable Print Spooler"),
        QStringLiteral("Print Spooler service. Disable only if you never print from this PC."),
        Impact::Medium, QStringLiteral("Spooler"),
        { QStringLiteral("Printing will not work until the Print Spooler is re-enabled.") });
    svc(QStringLiteral("debloat.disable_wsearch"), QStringLiteral("debloating"),
        QStringLiteral("Disable Windows Search Indexing"),
        QStringLiteral("Windows Search indexing service (WSearch). Frees CPU and disk at the cost of slower file search."),
        Impact::Medium, QStringLiteral("WSearch"),
        { QStringLiteral("File search becomes slower without the index.") });
    svc(QStringLiteral("deb.services.wsearch"), QStringLiteral("debloating"),
        QStringLiteral("Disable Windows Search Indexing (service)"),
        QStringLiteral("Disables the Windows Search indexing service (WSearch)."),
        Impact::Medium, QStringLiteral("WSearch"),
        { QStringLiteral("File search becomes slower without the index.") });
    svc(QStringLiteral("deb.services.parental"), QStringLiteral("debloating"),
        QStringLiteral("Disable Windows Parental Control (WpcMonSvc)"),
        QStringLiteral("Windows Parental Controls monitor service. Unneeded without Family Safety."),
        Impact::Low, QStringLiteral("WpcMonSvc"), {});

    svc(QStringLiteral("net.driver.ipv6_disable"), QStringLiteral("networking"),
        QStringLiteral("Disable IPv6 (Tcpip6 Driver)"),
        QStringLiteral("Disables the IPv6 protocol driver (Tcpip6)."),
        Impact::Medium, QStringLiteral("Tcpip6"),
        { QStringLiteral("Some network features and apps require IPv6 and may stop working.") });
    svc(QStringLiteral("net.driver.ndisvirtualbus_disable"), QStringLiteral("networking"),
        QStringLiteral("Disable Hyper-V Virtual NIC Bus"),
        QStringLiteral("Disables the Hyper-V virtual network bus driver used by virtual switches."),
        Impact::Low, QStringLiteral("ndisvirtualbus"), {});
    svc(QStringLiteral("net.driver.netbios_disable"), QStringLiteral("networking"),
        QStringLiteral("Disable NetBIOS Driver"),
        QStringLiteral("Disables the legacy NetBIOS name-resolution driver."),
        Impact::Low, QStringLiteral("NetBIOS"), {});

    // Multi-service disable: Start=4 on apply, Start=3 on revert for each service.
    const auto svcMulti = [&c](const QString& id, const QString& cat, const QString& name,
                               const QString& desc, Impact impact, const QStringList& services,
                               const QStringList& warnings) {
        TweakSpec t;
        t.id = id;
        t.category = cat;
        t.name = name;
        t.description = desc;
        t.iconName = QStringLiteral("settings");
        t.impact = impact;
        t.requiresAdmin = true;
        t.warnings = warnings;
        for (const QString& s : services)
            t.registryOps.push_back(dwordOp(
                QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\") + s,
                QStringLiteral("Start"), 4u, 3u));
        c.push_back(t);
    };

    svcMulti(QStringLiteral("deb.services.xbox_bundle"), QStringLiteral("debloating"),
        QStringLiteral("Disable Xbox Service Bundle + WMP Network"),
        QStringLiteral("Disables Xbox Live authentication, game save and networking services and the legacy Windows Media Player network-sharing daemon."),
        Impact::Medium,
        { QStringLiteral("XblAuthManager"), QStringLiteral("XblGameSave"),
          QStringLiteral("XboxNetApiSvc"), QStringLiteral("WMPNetworkSvc") },
        { QStringLiteral("Xbox Game Pass, achievements and cloud saves will stop working.") });

    // Registry-policy tweaks recovered from vth-backup.
    {
        TweakSpec t;
        t.id = QStringLiteral("app.explorer.disable_quick_access");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Explorer");
        t.name = QStringLiteral("Disable Quick Access and recent files");
        t.description = QStringLiteral("Sets HubMode=1, ShowFrequent=0 and ShowRecent=0 so the Quick Access pane vanishes and Explorer stops recording the folders you open.");
        t.iconName = QStringLiteral("folder-minus");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        const QString ex = QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer");
        t.registryOps = {
            dwordOp(ex, QStringLiteral("HubMode"), 1u, 0u),
            dwordOp(ex, QStringLiteral("ShowFrequent"), 0u, 1u),
            dwordOp(ex, QStringLiteral("ShowRecent"), 0u, 1u),
        };
        c.push_back(t);
    }
    {
        TweakSpec t;
        t.id = QStringLiteral("app.explorer.hide_tray_notifications");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("System tray");
        t.name = QStringLiteral("Hide system tray notifications (policy)");
        t.description = QStringLiteral(
            "Hides every icon in the notification area beside the clock, and stops "
            "the balloons that appear from it.");
        t.iconName = QStringLiteral("bell-off");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.registryOps = { dwordOp(
            QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"),
            QStringLiteral("NoTrayItemsDisplay"), 1u, std::nullopt) };
        c.push_back(t);
    }
    {
        TweakSpec t;
        t.id = QStringLiteral("shell.classic-context-menu");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Context menu");
        t.name = QStringLiteral("Classic Win10 context menu");
        t.description = QStringLiteral("Registers an empty default value under the Win11 shell-extension proxy CLSID so the classic full right-click menu returns. Restart Explorer to take effect.");
        t.iconName = QStringLiteral("menu");
        t.impact = Impact::Low;
        t.requiresReboot = false;
        t.registryOps = { stringOp(
            QStringLiteral("HKCU\\Software\\Classes\\CLSID\\{86ca1aa0-34aa-4e8b-a509-50c905bae2a2}\\InprocServer32"),
            QString(), QString(), std::nullopt) };
        c.push_back(t);
    }
    {
        TweakSpec t;
        t.id = QStringLiteral("priv.telemetry.wdi_extended");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Telemetry");
        t.name = QStringLiteral("Disable extended WDI diagnostic GUIDs");
        t.description = QStringLiteral("Sets ScenarioExecutionEnabled=0 and EnabledScenarioExecutionLevel=0 for six additional Windows Diagnostic Infrastructure scenario GUIDs so those diagnostic scenarios do not run.");
        t.iconName = QStringLiteral("shield-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        const QStringList wdiGuids = {
            QStringLiteral("{affc81e2-612a-4f70-6fb2-916ff5c7e3f8}"),
            QStringLiteral("{D113E4AA-2D07-41b1-8D9B-C065194A791D}"),
            QStringLiteral("{dc42ff48-e40d-4a60-8675-e71f7e64aa9a}"),
            QStringLiteral("{eb73b633-3f4e-4ba0-8f60-8f3c6f53168f}"),
            QStringLiteral("{ecfb03d1-58ee-4cc7-a1b5-9bc6febcb915}"),
            QStringLiteral("{ffc42108-4920-4acf-a4fc-8abdcc68ada4}"),
        };
        for (const QString& g : wdiGuids) {
            const QString p = QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\WDI\\") + g;
            t.registryOps.push_back(dwordOp(p, QStringLiteral("ScenarioExecutionEnabled"), 0u, std::nullopt));
            t.registryOps.push_back(dwordOp(p, QStringLiteral("EnabledScenarioExecutionLevel"), 0u, std::nullopt));
        }
        c.push_back(t);
    }

    reg(QStringLiteral("perf.mem.disable_cfg"), QStringLiteral("advanced"),
        QStringLiteral("Mitigations"), QStringLiteral("Disable Control Flow Guard (CFG)"),
        QStringLiteral("Sets EnableCfg=0 in Memory Management to disable the Control Flow Guard exploit mitigation."),
        Impact::Critical, true, QStringLiteral("shield-off"),
        QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"),
        QStringLiteral("EnableCfg"), 0u, std::nullopt,
        { QStringLiteral("Disables the Control Flow Guard exploit mitigation and reduces system security.") });

    reg(QStringLiteral("perf.mem.disable_aslr"), QStringLiteral("advanced"),
        QStringLiteral("Mitigations"), QStringLiteral("Disable ASLR image randomization"),
        QStringLiteral("Sets MoveImages=0 in Memory Management to disable Address Space Layout Randomization for images."),
        Impact::Critical, true, QStringLiteral("shield-off"),
        QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"),
        QStringLiteral("MoveImages"), 0u, std::nullopt,
        { QStringLiteral("Disables Address Space Layout Randomization, a core exploit mitigation. Significantly reduces security.") });

    svc(QStringLiteral("deb.services.update_orchestrator"), QStringLiteral("debloating"),
        QStringLiteral("Disable Update Orchestrator Service (UsoSvc)"),
        QStringLiteral("Update Orchestrator service that schedules and manages Windows Update."),
        Impact::Medium, QStringLiteral("UsoSvc"),
        { QStringLiteral("Interferes with Windows Update scheduling and installation.") });

    {
        TweakSpec t;
        t.id = QStringLiteral("perf.kernel.priority_irq");
        t.category = QStringLiteral("advanced");
        t.name = QStringLiteral("IRQ priority and scheduling control");
        t.description = QStringLiteral("Configures PriorityControl values for IRQ priority assignment, thread boost type, scheduling model, DPC threshold, deep IO coalescing and scheduler-assist overrides.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.warnings = { QStringLiteral("Custom IRQ priorities can starve low-priority drivers and cause USB or networking hitches.") };
        const QString pc = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\PriorityControl");
        t.registryOps = {
            dwordOp(pc, QStringLiteral("ConvertibleSlateMode"), 0u, std::nullopt),
            dwordOp(pc, QStringLiteral("IRQ00Priority"), 1u, std::nullopt),
            dwordOp(pc, QStringLiteral("IRQ16Priority"), 2u, std::nullopt),
            dwordOp(pc, QStringLiteral("IRQ23Priority"), 3u, std::nullopt),
            dwordOp(pc, QStringLiteral("ThreadBoostType"), 2u, std::nullopt),
            dwordOp(pc, QStringLiteral("ThreadSchedulingModel"), 1u, std::nullopt),
            dwordOp(pc, QStringLiteral("AdjustDpcThreshold"), 0x320u, std::nullopt),
            dwordOp(pc, QStringLiteral("DeepIoCoalescingEnabled"), 1u, std::nullopt),
            dwordOp(pc, QStringLiteral("SchedulerAssistThreadFlagOverride"), 1u, std::nullopt),
            dwordOp(pc, QStringLiteral("ForegroundBoost"), 1u, std::nullopt),
            dwordOp(pc, QStringLiteral("IRQ08Priority"), 4u, std::nullopt),
        };
        c.push_back(t);
    }


    svcMulti(QStringLiteral("gpu.driver.legacy_disable"), QStringLiteral("advanced"),
        QStringLiteral("Disable Legacy/Unused Driver Services"),
        QStringLiteral("Disables a set of legacy or typically unused driver services: IPv6, PC speaker, virtual NIC, NetBIOS, parallel and serial ports, infrared, Remote Desktop display, Bluetooth, Fax, IP Helper and smart card."),
        Impact::Medium,
        { QStringLiteral("Tcpip6"), QStringLiteral("Beep"), QStringLiteral("NdisVirtualBus"),
          QStringLiteral("NetBIOS"), QStringLiteral("Parport"), QStringLiteral("Serial"),
          QStringLiteral("IrDA"), QStringLiteral("TermDD"), QStringLiteral("BTHPORT"),
          QStringLiteral("Fax"), QStringLiteral("iphlpsvc"), QStringLiteral("SCardSvr") },
        { QStringLiteral("Disables Bluetooth, smart card, IP Helper and Remote Desktop display services among others. Review the list before applying.") });
}

}
