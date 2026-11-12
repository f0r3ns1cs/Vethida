#include "TweakEngine.h"

namespace vth::tweaks::detail {

void appendDefsServicesPort(QVector<TweakSpec>& c) {
    {
        TweakSpec t;
        t.id = QStringLiteral("debloat.svc.kiosk_assigned_access_disable");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Kiosk mode");
        t.name = QStringLiteral("Disable Assigned Access (kiosk) manager service");
        t.description = QStringLiteral("Disables the Assigned Access Manager Service, which runs Windows' single-app/multi-app kiosk mode.");
        t.iconName = QStringLiteral("device-desktop");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = { QStringLiteral("Kiosk (assigned access) mode can no longer be configured or run.") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AssignedAccessManagerSvc"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("debloat.svc.bluetooth_extra_disable");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Bluetooth");
        t.name = QStringLiteral("Disable Bluetooth audio gateway and user service");
        t.description = QStringLiteral("Disables the Bluetooth Audio Gateway Service (phone-as-headset/hands-free audio) and the per-user Bluetooth User Service template. Complements Vethida's existing Bluetooth support/AVCTP disable.");
        t.iconName = QStringLiteral("bluetooth");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = { QStringLiteral("Bluetooth hands-free audio and some per-user Bluetooth features stop working.") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\BTAGService"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\BluetoothUserService"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("debloat.svc.game_broadcast_helpers_disable");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Gaming & graphics");
        t.name = QStringLiteral("Disable game broadcast and graphics perf services");
        t.description = QStringLiteral("Disables the per-user GameDVR/Broadcast User Service (Xbox Game Bar streaming/recording) and the Graphics Performance Monitor Service.");
        t.iconName = QStringLiteral("device-gamepad-2");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = { QStringLiteral("Xbox Game Bar broadcasting/streaming and third-party tools that rely on the graphics performance monitor may stop working.") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\BcastDVRUserService"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\GraphicsPerfSvc"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("debloat.svc.lltd_discovery_drivers_disable");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Network discovery");
        t.name = QStringLiteral("Disable link-layer Topology/LLDP drivers");
        t.description = QStringLiteral("Disables the Link-Layer Discovery Protocol driver (MsLldp) and the Link-Layer Topology Discovery mapper/responder drivers (lltdio, rspndr) that back the 'Network Map' feature in Network and Sharing Center.");
        t.iconName = QStringLiteral("network");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = { QStringLiteral("The full graphical Network Map in Network and Sharing Center will no longer populate; other devices' network mapping tools may stop seeing this PC.") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\MsLldp"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\lltdio"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\rspndr"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("debloat.svc.misc_legacy_helpers_disable");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Legacy helpers");
        t.name = QStringLiteral("Disable telephony, ALG, profile roaming and cellular time");
        t.description = QStringLiteral("Disables the Telephony (TAPI) service, the Application Layer Gateway (legacy NAT protocol helper), Local Profile Assistance (Enterprise State Roaming) and Cellular Time sync.");
        t.iconName = QStringLiteral("phone-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = { QStringLiteral("Breaks legacy dial-up/modem TAPI apps, some older NAT protocol pass-through (FTP/H.323 helpers), Enterprise State Roaming profile sync, and cellular-network time sync.") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\TapiSrv"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\ALG"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\wlpasvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\autotimesvc"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("debloat.svc.ras_vpn_legacy_drivers_disable");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Legacy RAS/VPN drivers");
        t.name = QStringLiteral("Disable legacy RAS/VPN miniport drivers");
        t.description = QStringLiteral("Disables the NDISWAN core driver and the built-in WAN miniport drivers for PPTP, IKEv2, L2TP, PPPoE and SSTP VPN connections, plus their RAS ARP (IPv4/IPv6) and asynchronous media helper drivers. These only load when Windows' native dial-up/VPN client establishes a connection using one of these protocols.");
        t.iconName = QStringLiteral("route");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = { QStringLiteral("Breaks Windows' built-in VPN client for PPTP, IKEv2, L2TP, SSTP and PPPoE connections; third-party VPN clients with their own drivers are unaffected.") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\NdisWan"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\PptpMiniport"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\RasAgileVpn"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Rasl2tp"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\RasPppoe"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\RasSstp"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AsyncMac"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\wanarp"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Wanarpv6"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("debloat.svc.windows_update_policy_extra_disable");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Windows Update");
        t.name = QStringLiteral("Block Windows Update internet locations and defer upgrades");
        t.description = QStringLiteral("Sets the Group Policy values that block Windows Update from contacting Microsoft's Windows Update servers directly, and configures feature-update deferral (DeferUpgrade) with a deferral period of 0 months.");
        t.iconName = QStringLiteral("settings-off");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.concern = QStringLiteral("DoNotConnectToWindowsUpdateInternetLocations blocks Windows Update's direct internet path entirely; on a machine with no WSUS/Intune update source configured, this effectively stops all updates. High-risk, situational policy change.");
        t.warnings = {
            QStringLiteral("Windows Update can no longer reach Microsoft's update servers directly; only works if another update source (WSUS, Intune) is configured."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate"), QStringLiteral("DoNotConnectToWindowsUpdateInternetLocations"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate"), QStringLiteral("DeferUpgrade"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate"), QStringLiteral("DeferUpdatePeriod"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("debloat.svc.windows_update_core_disable");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Windows Update");
        t.name = QStringLiteral("Disable core Windows Update services");
        t.description = QStringLiteral("Disables the Windows Update service (wuauserv), the Update Orchestrator Service (UsoSvc) that schedules update scans/installs, and the Windows Update Medic Service (WaaSMedicSvc) that repairs Windows Update components. Together these stop all update checking, downloading and installing.");
        t.iconName = QStringLiteral("cloud-off");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.concern = QStringLiteral("Fully disables Windows Update, including security patches, until manually reverted - a significant security trade-off most users should not take lightly.");
        t.warnings = {
            QStringLiteral("No update checks, downloads or installs occur at all, including security patches, until re-enabled."),
            QStringLiteral("WaaSMedicSvc is a protected system-repair service; Windows may periodically reset its Start value on its own."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\wuauserv"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\UsoSvc"), QStringLiteral("Start"), 4u, 2u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\WaaSMedicSvc"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("debloat.svc.appx_store_disable");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Microsoft Store");
        t.name = QStringLiteral("Disable AppX/Store deployment services");
        t.description = QStringLiteral("Disables the Windows License Manager Service, AppX Deployment Service, Client License Service (ClipSVC) and Microsoft Store Install Service that install, license and update UWP/Store apps.");
        t.iconName = QStringLiteral("shopping-bag");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.concern = QStringLiteral("Breaks the Microsoft Store, winget package installs, and licensing for every installed UWP app - only apply on machines that never use the Store.");
        t.warnings = {
            QStringLiteral("The Microsoft Store app and winget will fail to install or update packages."),
            QStringLiteral("Already-installed Store apps may lose license validation."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\LicenseManager"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\AppXSVC"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\ClipSVC"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\InstallService"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("debloating.services.disable_push_notifications");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Notifications");
        t.name = QStringLiteral("Disable Windows push notifications service");
        t.description = QStringLiteral("Disables the per-user Windows Push Notifications User Service (WpnUserService), the backend that delivers toast, tile, and raw push notifications to apps.");
        t.iconName = QStringLiteral("bell-off");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.concern = QStringLiteral("This is the delivery backend for essentially all toast/tile notifications system-wide (mail, calendar, Store apps, Action Center). Disabling it is a much larger step than turning off notifications in Settings and will surprise most users.");
        t.warnings = { QStringLiteral("All toast, tile, and Action Center notifications from apps stop being delivered") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\WpnUserService"), QStringLiteral("Start"), 4u, 2u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("debloating.services.disable_cross_device_sync");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Disable cross-device and consent sync services");
        t.description = QStringLiteral("Disables a group of per-user background services used for cross-device experiences and consent handling: Continue-on-device flow, consent UX, contact/PIM index maintenance, device-picker UI, per-user text messaging relay, user-data storage sync, and the Connected Devices Platform.");
        t.iconName = QStringLiteral("devices");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Phone Link / \"Continue on PC\", Nearby Share device-picker prompts, and cross-device handoff stop working"),
            QStringLiteral("SMS relay through Phone Link and People/contacts sync between devices stop working"),
            QStringLiteral("Any app that reads synced user data (contacts, messages) via these services may fail"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\DevicesFlowUserSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\ConsentUxUserSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\PimIndexMaintenanceSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\DevicePickerUserSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\MessagingService"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\UnistoreSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\CDPSvc"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("debloating.services.disable_capture_service");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Disable Windows capture service");
        t.description = QStringLiteral("Disables the per-user Windows Capture Service, used by Snipping Tool, Snip & Sketch, and Xbox Game Bar to capture the screen.");
        t.iconName = QStringLiteral("camera");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.concern = QStringLiteral("Breaks Snipping Tool / Snip & Sketch screenshots and Xbox Game Bar screen capture; only disable if you use a third-party screenshot tool.");
        t.warnings = { QStringLiteral("Snipping Tool, Snip & Sketch, and Xbox Game Bar screen capture stop working") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\CaptureService"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("debloating.services.disable_device_image_helpers");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Devices");
        t.name = QStringLiteral("Disable portable device and image acquisition helpers");
        t.description = QStringLiteral("Disables the Portable Device Enumerator Service and Still Image Acquisition Events, which watch for USB cameras/scanners/media players and launch AutoPlay/import prompts for them.");
        t.iconName = QStringLiteral("device-usb");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = { QStringLiteral("Group Policy-managed portable device restrictions stop being enforced"), QStringLiteral("Scanners and WIA-based cameras no longer trigger the auto-launch/import prompt when connected") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\WPDBusEnum"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\WiaRpc"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("debloating.services.disable_secondary_logon");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Security");
        t.name = QStringLiteral("Disable secondary logon service");
        t.description = QStringLiteral("Disables the Secondary Logon (seclogon) service, which lets processes be started under alternate credentials such as an administrator or TrustedInstaller account.");
        t.iconName = QStringLiteral("key");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.concern = QStringLiteral("Breaks \"Run as different user\", RunAs.exe, and any workflow (including some IT/dev tooling) that launches processes under alternate credentials; verify you don't rely on this before disabling.");
        t.warnings = { QStringLiteral("\"Run as different user\" from the right-click menu and runas.exe stop working") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\seclogon"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("debloating.services.disable_print_workflow");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Printing");
        t.name = QStringLiteral("Disable print workflow service");
        t.description = QStringLiteral("Disables the per-user Print Workflow service that runs modern print-time app extensions (e.g. print preview add-ins from the Microsoft Store).");
        t.iconName = QStringLiteral("printer");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = { QStringLiteral("Store-delivered print workflow app extensions (custom print previews/finishing options) stop running") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\PrintWorkflowUserSvc"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("debloating.services.disable_hid_shellhw");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Disable HID hotkeys and shell hardware detection");
        t.description = QStringLiteral("Disables the Human Interface Device Service (multimedia hotkeys on keyboards/remotes) and Shell Hardware Detection (AutoPlay and hot-plug hardware notifications).");
        t.iconName = QStringLiteral("keyboard");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = { QStringLiteral("Dedicated multimedia hotkeys on keyboards and remotes stop working"), QStringLiteral("AutoPlay prompts for USB drives and discs stop appearing") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\hidserv"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\ShellHWDetection"), QStringLiteral("Start"), 4u, 2u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("debloating.services.disable_mobile_hotspot_wwan");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Networking");
        t.name = QStringLiteral("Disable Mobile Hotspot and WWAN AutoConfig");
        t.description = QStringLiteral("Disables the Windows Mobile Hotspot Service (icssvc) and WWAN AutoConfig, which manage Mobile Hotspot sharing and cellular (WWAN) modem connections.");
        t.iconName = QStringLiteral("wifi");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = { QStringLiteral("The Mobile Hotspot feature stops working"), QStringLiteral("Built-in cellular/LTE modem connectivity stops working; irrelevant on PCs without a cellular modem") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\icssvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\WwanSvc"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("debloating.services.disable_radio_wifidirect");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Networking");
        t.name = QStringLiteral("Disable radio management and Wi-Fi direct services");
        t.description = QStringLiteral("Disables the Radio Management Service (airplane-mode radio state coordination) and the Wi-Fi Direct Services Connection Manager (wireless display/Miracast and Wi-Fi Direct pairing).");
        t.iconName = QStringLiteral("antenna");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.concern = QStringLiteral("Can prevent Wi-Fi/Bluetooth radios from responding to the physical/software airplane-mode switch, and breaks wireless display (Miracast) / Wi-Fi Direct pairing.");
        t.warnings = { QStringLiteral("Airplane mode radio toggling may stop working correctly"), QStringLiteral("Wireless display (Miracast) and Wi-Fi Direct device pairing stop working") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\RmSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\WFDSConMgrSvc"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("debloating.services.disable_misc_rare");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Disable miscellaneous rarely-used services");
        t.description = QStringLiteral("Disables a batch of niche services most home PCs never use: Windows Encryption Provider Host, IP Translation Configuration, Performance Logs & Alerts, Volumetric Audio Compositor, WARP JIT support, Microsoft iSCSI Initiator, Remote Access Auto Connection Manager, Windows Connect Now, and Secure Socket Tunneling Protocol.");
        t.iconName = QStringLiteral("trash-2");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Performance Monitor \"Data Collector Sets\" stop being able to run/schedule"),
            QStringLiteral("iSCSI SAN connections and SSTP VPN connections stop working if you use either"),
            QStringLiteral("Windows Sonic/spatial-audio composition and WPS wireless setup (Windows Connect Now) stop working"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\WEPHOSTSVC"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\IpxlatCfgSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\pla"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\VacSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\WarpJITSvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\MSiSCSI"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\RasAuto"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Wcncsvc"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\SstpSvc"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("debloating.services.disable_camera_frameserver");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Disable Windows camera frame server");
        t.description = QStringLiteral("Disables the Windows Camera Frame Server, which brokers shared access to a webcam so multiple apps (or one app plus Windows Hello) can use it at once.");
        t.iconName = QStringLiteral("camera");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.concern = QStringLiteral("Many modern camera apps (Camera app, Teams, browsers) route through the Frame Server rather than opening the webcam directly; disabling it can break camera access entirely, not just multi-app sharing.");
        t.warnings = { QStringLiteral("Webcam access from the Camera app, video-call apps, and browsers may stop working") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\FrameServer"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("debloating.services.disable_volume_shadow_copy");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Backup");
        t.name = QStringLiteral("Disable Volume Shadow Copy service");
        t.description = QStringLiteral("Disables the Volume Shadow Copy service (VSS), which coordinates point-in-time snapshots of volumes.");
        t.iconName = QStringLiteral("database-export");
        t.impact = Impact::Critical;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.concern = QStringLiteral("VSS underpins System Restore, File History, Windows Backup, and most third-party backup/imaging tools, and many installers use it to snapshot files during upgrades. Disabling it can silently break restore points and backup software; only for advanced users who understand the tradeoff.");
        t.warnings = {
            QStringLiteral("System Restore can no longer create or use restore points"),
            QStringLiteral("File History and most backup/imaging software (built-in and third-party) stop working"),
            QStringLiteral("Some installers/updaters that snapshot files via VSS may fail or behave unexpectedly"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\VSS"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("debloating.services.disable_windows_backup_svc");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Backup");
        t.name = QStringLiteral("Disable Windows Backup service (SDRSVC)");
        t.description = QStringLiteral("Disables SDRSVC, the Windows Backup service used by the modern Windows Backup app and the legacy \"Backup and Restore (Windows 7)\" control panel.");
        t.iconName = QStringLiteral("database-export");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = { QStringLiteral("The Windows Backup app and \"Backup and Restore (Windows 7)\" stop working until re-enabled") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\SDRSVC"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("debloating.services.disable_token_broker");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Security");
        t.name = QStringLiteral("Disable Web Account Manager (TokenBroker)");
        t.description = QStringLiteral("Disables the TokenBroker service (Web Account Manager), which brokers single sign-on tokens for Microsoft accounts and work/school accounts used by the Microsoft Store and many UWP apps.");
        t.iconName = QStringLiteral("key");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.concern = QStringLiteral("Breaks Microsoft Store sign-in and any UWP/Store app that relies on Web Account Manager SSO (Xbox app, Office activation, OneDrive app, some third-party apps using Microsoft/AAD login). Only for users who do not use a Microsoft account or Store apps.");
        t.warnings = { QStringLiteral("Microsoft Store and UWP apps that sign in with a Microsoft or work/school account stop being able to authenticate") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\TokenBroker"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("debloating.services.disable_hyperv_compute_mgmt");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Virtualization");
        t.name = QStringLiteral("Disable Hyper-V compute and management services");
        t.description = QStringLiteral("Disables the Hyper-V Host Compute Service (vmcompute) and Hyper-V Virtual Machine Management (vmms), the core services that create and run virtual machines. Complements Vethida's existing Hyper-V integration-services tweak, which covers the vmic*/HvHost guest-integration daemons but not these two host-side services.");
        t.iconName = QStringLiteral("server");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.concern = QStringLiteral("vmcompute is also the backend WSL2, Windows Sandbox, and container tools (e.g. Docker Desktop's Hyper-V backend) rely on. Disabling these breaks WSL2 and all Hyper-V VMs, not just classic Hyper-V Manager virtual machines.");
        t.warnings = { QStringLiteral("All Hyper-V virtual machines stop working"), QStringLiteral("WSL2 and Windows Sandbox stop working, since both run on the Hyper-V compute service") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\vmcompute"), QStringLiteral("Start"), 4u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\vmms"), QStringLiteral("Start"), 4u, 2u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("debloating.services.disable_defrag");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Storage");
        t.name = QStringLiteral("Disable optimize drives service");
        t.description = QStringLiteral("Disables the Optimize Drives (defragsvc) service that performs scheduled background drive optimization and defragmentation.");
        t.iconName = QStringLiteral("database");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = { QStringLiteral("The Optimize Drives app (dfrgui.exe) and scheduled drive optimization stop working until re-enabled"), QStringLiteral("On mechanical hard drives, fragmentation can build up over time without periodic optimization") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\defragsvc"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("debloating.services.disable_push_to_install");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Disable push-to-install");
        t.description = QStringLiteral("Blocks the Push-to-Install feature that lets other devices, like the Microsoft Store website or a paired phone, remotely trigger app installs onto this PC.");
        t.iconName = QStringLiteral("download-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\PushToInstall"), QStringLiteral("DisablePushToInstall"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("debloating.onedrive.block_kfm_optin");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("OneDrive");
        t.name = QStringLiteral("Block OneDrive known folder move opt-in");
        t.description = QStringLiteral("Prevents OneDrive from prompting users to redirect their Desktop, Documents, and Pictures folders into OneDrive via Known Folder Move.");
        t.iconName = QStringLiteral("cloud-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Users who already opted into Known Folder Move keep their existing redirection; this only blocks new opt-in prompts"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\OneDrive"), QStringLiteral("KFMBlockOptIn"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("svc.fontcache-off");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Disable Windows font cache service");
        t.description = QStringLiteral("Disables the Windows Font Cache service (FontCache), which pre-renders and caches font data for GDI/DirectWrite so apps don't have to parse font files themselves.");
        t.iconName = QStringLiteral("typography");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = { QStringLiteral("Apps may take slightly longer to render text the first time, since each process has to build its own font cache instead of sharing the system-wide one") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\FontCache"), QStringLiteral("Start"), 4u, 2u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("svc.data-usage-off");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Disable data usage tracking service");
        t.description = QStringLiteral("Disables the Data Usage service (DusmSvc), which tracks per-app and per-network data consumption for the Settings > Network > Data usage page and metered-connection limit warnings.");
        t.iconName = QStringLiteral("chart-bar-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = { QStringLiteral("The Settings app's Data usage page stops reporting consumption, and metered-connection data-limit notifications no longer fire") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\DusmSvc"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("svc.event-collector-off");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Disable Windows event collector service");
        t.description = QStringLiteral("Disables the Windows Event Collector service (Wecsvc), which manages event subscriptions that forward events from remote computers to this machine's Event Log.");
        t.iconName = QStringLiteral("history");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = { QStringLiteral("Breaks Windows Event Forwarding subscriptions from remote machines; irrelevant to most home/single-PC setups") };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Wecsvc"), QStringLiteral("Start"), 4u, 3u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.telemetry-allowtelemetry-mirrors");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Telemetry");
        t.name = QStringLiteral("Disable legacy AllowTelemetry mirror values");
        t.description = QStringLiteral("Writes AllowTelemetry=0 into two secondary registry locations (the AppCompat policy key and the legacy pre-1607 DataCollection policy path) that some older diagnostics components still consult, on top of the primary DataCollection telemetry policy Vethida already manages elsewhere.");
        t.iconName = QStringLiteral("eye-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\AppCompat"), QStringLiteral("AllowTelemetry"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\DataCollection"), QStringLiteral("AllowTelemetry"), 0u, std::nullopt),
        };
        c.push_back(t);
    }
}

}
