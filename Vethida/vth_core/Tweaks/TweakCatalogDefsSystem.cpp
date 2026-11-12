#include "TweakEngine.h"

namespace vth::tweaks::detail {

void appendDefsSystem(QVector<TweakSpec>& c) {
    {
        TweakSpec t;
        t.id = QStringLiteral("sys.a11y.narrator_config");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Accessibility");
        t.name = QStringLiteral("Disable Narrator and accessibility hints");
        t.description = QStringLiteral("Disables Narrator auto-start, Win+Enter launch, detailed feedback, online services, audio cues, and read hints.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Narrator screen reader will not auto-start"),
            QStringLiteral("Win+Enter launch shortcut disabled"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Narrator\\NoRoam"), QStringLiteral("WinEnterLaunchEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Narrator\\NoRoam"), QStringLiteral("OnlineServicesEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Narrator"), QStringLiteral("IntonationPause"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Narrator"), QStringLiteral("PlayAudioCues"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Narrator"), QStringLiteral("ReadHints"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Narrator\\NarratorHome"), QStringLiteral("AutoStart"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.disk.ahci_power");
        t.category = QStringLiteral("storage");
        t.subCategory = QStringLiteral("Link power management");
        t.name = QStringLiteral("AHCI power settings");
        t.description = QStringLiteral("Disables AHCI link power management for lower storage latency.");
        t.iconName = QStringLiteral("database");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("SATA link stays active; battery-powered laptops lose a small amount of idle runtime from disabled link power management"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\storahci\\Parameters\\Device"), QStringLiteral("IdlePowerMode"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\storahci\\Parameters\\Device"), QStringLiteral("NoLPM"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\PowerSettings\\0012ee47-9041-4b5d-9b77-535fba8b1442\\0b2d69d7-a2a1-449c-9680-f91c70521c60"), QStringLiteral("Attributes"), 2u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\iaStorV\\Parameters"), QStringLiteral("LinkPowerManagementMode"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.apps.disable_teams_chat");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Teams & Chat");
        t.name = QStringLiteral("Disable Microsoft Teams and chat auto-install");
        t.description = QStringLiteral("Hides the Windows Chat icon, disables Teams auto-install, and prevents Microsoft Messenger auto-run.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Windows Chat icon hidden from the taskbar"),
            QStringLiteral("Microsoft Teams will not auto-install"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Chat"), QStringLiteral("ChatIcon"), 3u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\Experience\\ConfigureChatIcon"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Communications"), QStringLiteral("ConfigureChatAutoInstall"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Messenger\\Client"), QStringLiteral("PreventAutoRun"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.bluetooth.optimize");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Bluetooth");
        t.name = QStringLiteral("Bluetooth optimization");
        t.description = QStringLiteral("Disables Bluetooth power management for consistent connection quality, enables interactive connect, and enables advanced discovery mode for faster pairing.");
        t.iconName = QStringLiteral("zap");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Bluetooth radio stays active, increasing battery drain on laptops"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\BTHPORT\\Parameters"), QStringLiteral("PowerManagementEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\BTHPORT\\Parameters"), QStringLiteral("AllowInteractiveConnect"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Bluetooth"), QStringLiteral("AdvancedDiscoveryMode"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\Bluetooth\\AllowPromptedProximalConnections"), QStringLiteral("value"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.boot.compat_tweaks");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Windows 11 upgrade");
        t.name = QStringLiteral("Boot animation and unsupported hardware bypass");
        t.description = QStringLiteral("Enables Windows 10X/11 boot animation, allows Windows 11 upgrades on unsupported CPU/TPM, and suppresses unsupported hardware watermark.");
        t.iconName = QStringLiteral("rotate");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Some updates may be blocked on unsupported hardware"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\BootControl"), QStringLiteral("BootProgressAnimation"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\Setup\\MoSetup"), QStringLiteral("AllowUpgradesWithUnsupportedTPMOrCPU"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Control Panel\\UnsupportedHardwareNotificationCache"), QStringLiteral("SV1"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Control Panel\\UnsupportedHardwareNotificationCache"), QStringLiteral("SV2"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Control Panel\\UnsupportedHardwareNotificationCache"), QStringLiteral("SV3"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.boot.network_gpo_async");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Group Policy");
        t.name = QStringLiteral("Faster boot for networked devices");
        t.description = QStringLiteral("Sets machine and user group policy to asynchronous mode to prevent network-dependent GPO processing from delaying boot.");
        t.iconName = QStringLiteral("rotate");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("GPO changes may require two reboots to take effect"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\policies\\system"), QStringLiteral("SynchronousMachineGroupPolicy"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\policies\\system"), QStringLiteral("SynchronousUserGroupPolicy"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.compat.disable_pca");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Compatibility & telemetry");
        t.name = QStringLiteral("Disable Program Compatibility Assistant");
        t.description = QStringLiteral("Disables PCA, application inventory, Steps Recorder, compatibility engine, UAR, and VDM (16-bit) support.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Legacy app compatibility shims disabled"),
            QStringLiteral("16-bit application support removed"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\Software\\Policies\\Microsoft\\Windows\\AppCompat"), QStringLiteral("DisableInventory"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\Software\\Policies\\Microsoft\\Windows\\AppCompat"), QStringLiteral("SbEnable"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\Software\\Policies\\Microsoft\\Windows\\AppCompat"), QStringLiteral("DisableUAR"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\Software\\Policies\\Microsoft\\Windows\\AppCompat"), QStringLiteral("DisablePCA"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\Software\\Policies\\Microsoft\\Windows\\AppCompat"), QStringLiteral("DisableEngine"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\Software\\Policies\\Microsoft\\Windows\\AppCompat"), QStringLiteral("AITEnable"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\Software\\Policies\\Microsoft\\Windows\\AppCompat"), QStringLiteral("VDMDisallowed"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.dev.developer_mode");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Developer");
        t.name = QStringLiteral("Enable Developer Mode");
        t.description = QStringLiteral("Enables Windows Developer Mode, allows all trusted apps, disables automatic app archiving, and enables shared user app data.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Unsigned UWP apps can be installed"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppModelUnlock"), QStringLiteral("AllowDevelopmentWithoutDevLicense"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppModelUnlock"), QStringLiteral("AllowAllTrustedApps"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\ApplicationManagement\\AllowAutomaticAppArchiving"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\ApplicationManagement\\AllowDeveloperUnlock"), QStringLiteral("value"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\ApplicationManagement\\AllowSharedUserAppData"), QStringLiteral("value"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.devices.disable_f1");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Keyboard");
        t.name = QStringLiteral("Disable F1 help key");
        t.description = QStringLiteral("Disables the F1 Help shortcut by nulling its TypeLib handler for both 32- and 64-bit entries.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Some older apps that rely on F1 for context help may not open their help pane."),
        };
        t.registryOps = {
            stringOp(QStringLiteral("HKCU\\SOFTWARE\\Classes\\Typelib\\{8cec5860-07a1-11d9-b15e-000d56bfe6ee}\\1.0\\0\\win32"), QStringLiteral(""), QStringLiteral(""), std::nullopt),
            stringOp(QStringLiteral("HKCU\\SOFTWARE\\Classes\\Typelib\\{8cec5860-07a1-11d9-b15e-000d56bfe6ee}\\1.0\\0\\win64"), QStringLiteral(""), QStringLiteral(""), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.devices.display");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Display");
        t.name = QStringLiteral("Display and monitor optimization");
        t.description = QStringLiteral("Optimizes display settings: enables HDR playback, disables HDR battery restriction, disables cursor deadzone jumping, disables content-adaptive brightness (CABC), and sets monitor removal behavior to minimize windows.");
        t.iconName = QStringLiteral("monitor");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Content-adaptive brightness disabled, reducing laptop battery life"),
            QStringLiteral("HDR stays enabled on battery"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\VideoSettings"), QStringLiteral("DisableHDROnBattery"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\VideoSettings"), QStringLiteral("EnableAutoEnhanceDuringPlayback"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\VideoSettings"), QStringLiteral("EnableHDRForPlayback"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Control Panel\\Cursors"), QStringLiteral("CursorDeadzoneJumpingSetting"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("DisableCABC"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Control Panel\\Desktop"), QStringLiteral("MonitorRemovalRecalcBehavior"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.devices.iscsi");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("iSCSI");
        t.name = QStringLiteral("iSCSI security hardening");
        t.description = QStringLiteral("Enables iSCSI security policies: allows IQN name changes, restricts additional logins, enables CHAP secret changes, and requires IPSec, mutual CHAP, and one-way CHAP.");
        t.iconName = QStringLiteral("shield");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("iSCSI targets without CHAP/IPSec support will fail to connect"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\iSCSI"), QStringLiteral("ChangeIQNName"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\iSCSI"), QStringLiteral("RestrictAdditionalLogins"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\iSCSI"), QStringLiteral("ChangeCHAPSecret"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\iSCSI"), QStringLiteral("RequireIPSec"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\iSCSI"), QStringLiteral("RequireMutualCHAP"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\iSCSI"), QStringLiteral("RequireOneWayCHAP"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.devices.precision_touchpad");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Touchpad");
        t.name = QStringLiteral("Precision TouchPad extended configuration");
        t.description = QStringLiteral("Configures precision touchpad user preferences: sets cursor speed to maximum, enables edge gestures, keeps touchpad active with mouse, sets natural scroll direction, disables multi-finger tap gestures, and disables mouse acceleration honoring.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Touchpad gesture and scroll behavior changes to optimized defaults"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\PrecisionTouchPad"), QStringLiteral("AAPThreshold"), 2u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\PrecisionTouchPad"), QStringLiteral("CursorSpeed"), 10u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\PrecisionTouchPad"), QStringLiteral("EnableEdgy"), 1u, 0u),
            // On/off values Windows writes as 0 or 1. Revert removes them so Windows
            // applies its own default.
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\PrecisionTouchPad"), QStringLiteral("LeaveOnWithMouse"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\PrecisionTouchPad"), QStringLiteral("PanEnabled"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\PrecisionTouchPad"), QStringLiteral("ScrollDirection"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\PrecisionTouchPad"), QStringLiteral("ZoomEnabled"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\PrecisionTouchPad"), QStringLiteral("ThreeFingerTapEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\PrecisionTouchPad"), QStringLiteral("FourFingerSlideEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\PrecisionTouchPad"), QStringLiteral("FourFingerTapEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\PrecisionTouchPad"), QStringLiteral("ThreeFingerSlideEnabled"), 2u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\PrecisionTouchPad"), QStringLiteral("HonorMouseAccelSetting"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\PrecisionTouchPad"), QStringLiteral("RightClickZoneWidth"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\PrecisionTouchPad"), QStringLiteral("RightClickZoneHeight"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\PrecisionTouchPad\\Status"), QStringLiteral("Enabled"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.devices.printer");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Printing");
        t.name = QStringLiteral("Printer and print dialog optimization");
        t.description = QStringLiteral("Reduces LPR timeout, enables printer status updates and the modern print dialog, and turns off Windows Protected Print Mode. Note that disabling WPP removes Windows 11's sandboxed printing protection in exchange for broader legacy-driver compatibility.");
        t.iconName = QStringLiteral("printer");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Windows Protected Print Mode disabled, reducing print driver isolation"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Print\\Monitors\\Standard TCP/IP Port\\Ports"), QStringLiteral("LprAckTimeout"), 2u, 180u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Print\\Monitors\\Standard TCP/IP Port\\Ports"), QStringLiteral("StatusUpdateEnabled"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Print"), QStringLiteral("EnableModernPrintDialog"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows NT\\Printers\\WPP"), QStringLiteral("WindowsProtectedPrintMode"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.diag.crash_display");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Crash / BSOD");
        t.name = QStringLiteral("Crash display and diagnostic text suppression");
        t.description = QStringLiteral("Shows display parameters on BSOD for debugging and suppresses Windows SelfHost diagnostic prompts.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("BSOD screen shows additional technical parameters"),
            QStringLiteral("Insider Preview diagnostic prompts suppressed"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\CrashControl"), QStringLiteral("DisplayParameters"), 1u, 0u),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\WindowsSelfHost\\UI\\Strings"), QStringLiteral("DiagnosticErrorText"), QStringLiteral(""), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\WindowsSelfHost\\UI\\Strings"), QStringLiteral("DiagnosticLinkText"), QStringLiteral(""), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("system.disable_hibernation");
        t.category = QStringLiteral("power");
        t.subCategory = QStringLiteral("Hibernation");
        t.name = QStringLiteral("Disable hibernation");
        t.description = QStringLiteral("Disables hibernation and removes the hiberfil.sys file, reclaiming disk space equal to your RAM size.");
        t.iconName = QStringLiteral("moon");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Hibernation option will be removed from the power menu"),
            QStringLiteral("Fast Startup will also stop working (it depends on hibernation)"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power"), QStringLiteral("HibernateEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power"), QStringLiteral("HibernateEnabledDefault"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power"), QStringLiteral("HiberFileSizePercent"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.input.mouse_accel");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Mouse");
        t.name = QStringLiteral("Disable mouse acceleration");
        t.description = QStringLiteral("Disables mouse acceleration for a 1:1 relationship between hand and cursor movement.");
        t.iconName = QStringLiteral("mouse-pointer");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Enhance Pointer Precision turned off; cursor feel will change"),
        };
        t.registryOps = {
            // 1 / 6 / 10 are the Windows defaults for Enhance Pointer Precision.
            stringOp(QStringLiteral("HKCU\\Control Panel\\Mouse"), QStringLiteral("MouseSpeed"), QStringLiteral("0"), QStringLiteral("1")),
            stringOp(QStringLiteral("HKCU\\Control Panel\\Mouse"), QStringLiteral("MouseThreshold1"), QStringLiteral("0"), QStringLiteral("6")),
            stringOp(QStringLiteral("HKCU\\Control Panel\\Mouse"), QStringLiteral("MouseThreshold2"), QStringLiteral("0"), QStringLiteral("10")),
            dwordOp(QStringLiteral("HKCU\\Control Panel\\Mouse"), QStringLiteral("MouseSensitivity"), 10u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.disk.disable_quota");
        t.category = QStringLiteral("storage");
        t.subCategory = QStringLiteral("Disk quota");
        t.name = QStringLiteral("Disable Windows Disk Quota");
        t.description = QStringLiteral("Disables disk quota enforcement and event logging.");
        t.iconName = QStringLiteral("usb");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Per-user disk quotas and quota event logging will not be enforced"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows NT\\DiskQuota"), QStringLiteral("Enforce"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows NT\\DiskQuota"), QStringLiteral("Enable"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows NT\\DiskQuota"), QStringLiteral("LogEventOverLimit"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows NT\\DiskQuota"), QStringLiteral("LogEventOverThreshold"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.disk.disable_readyboost");
        t.category = QStringLiteral("storage");
        t.subCategory = QStringLiteral("ReadyBoost");
        t.name = QStringLiteral("Disable ReadyBoost");
        t.description = QStringLiteral("Disables Windows ReadyBoost caching. Not needed on SSD systems.");
        t.iconName = QStringLiteral("zap");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("ReadyBoost USB caching unavailable; only helpful on HDD systems"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\EMDMgmt"), QStringLiteral("GroupPolicyDisallowCaches"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\EMDMgmt"), QStringLiteral("AllowNewCachesByDefault"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.explorer.encrypted_colors");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Explorer");
        t.name = QStringLiteral("Show encrypted and compressed file colors");
        t.description = QStringLiteral("Shows encrypted and compressed files in different colors in File Explorer for easier identification.");
        t.iconName = QStringLiteral("folder-open");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Changes how encrypted and compressed files appear in File Explorer"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), QStringLiteral("ShowEncryptCompressedColor"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.explorer.no_lowdisk");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Notifications");
        t.name = QStringLiteral("Disable low disk space notifications");
        t.description = QStringLiteral("Suppresses the low disk space warning balloon notifications.");
        t.iconName = QStringLiteral("usb");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("You will no longer be warned when a drive is running out of space"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"), QStringLiteral("NoLowDiskSpaceChecks"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.update.extended_controls");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Windows Update");
        t.name = QStringLiteral("Disable automatic Windows updates");
        t.description = QStringLiteral("Stops automatic Windows Update downloads/installs (NoAutoUpdate=1) and defers feature updates up to a year. This is a strong change: security patches will not install on their own, so you must check for and install updates manually.");
        t.iconName = QStringLiteral("download");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Security updates will not install automatically"),
            QStringLiteral("You must manually check for and install updates regularly"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate"), QStringLiteral("NoAutoUpdate"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate"), QStringLiteral("ExcludeWUDriversInQualityUpdate"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate"), QStringLiteral("DeferFeatureUpdates"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate"), QStringLiteral("DeferFeatureUpdatesPeriodInDays"), 365u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\RemovalTools\\MRT"), QStringLiteral("DontOfferThroughWUAU"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Speech"), QStringLiteral("AllowSpeechModelUpdate"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.input.accessibility_disable");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Accessibility");
        t.name = QStringLiteral("Disable accessibility shortcuts");
        t.description = QStringLiteral("Disables all accessibility keyboard shortcuts: StickyKeys, FilterKeys, ToggleKeys, MouseKeys, HighContrast, SoundSentry, TimeOut, ColorFiltering hotkey, Magnifier invert, and Ease of Access self-scan/self-voice.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("StickyKeys, FilterKeys, ToggleKeys and other accessibility shortcuts disabled"),
            QStringLiteral("Re-enable if you rely on accessibility features"),
        };
        t.registryOps = {
            stringOp(QStringLiteral("HKCU\\Control Panel\\Accessibility\\StickyKeys"), QStringLiteral("Flags"), QStringLiteral("0"), QStringLiteral("510")),
            stringOp(QStringLiteral("HKCU\\Control Panel\\Accessibility\\Keyboard Response"), QStringLiteral("Flags"), QStringLiteral("0"), QStringLiteral("122")),
            stringOp(QStringLiteral("HKCU\\Control Panel\\Accessibility\\ToggleKeys"), QStringLiteral("Flags"), QStringLiteral("0"), QStringLiteral("62")),
            stringOp(QStringLiteral("HKCU\\Control Panel\\Accessibility\\MouseKeys"), QStringLiteral("Flags"), QStringLiteral("0"), QStringLiteral("62")),
            stringOp(QStringLiteral("HKCU\\Control Panel\\Accessibility\\HighContrast"), QStringLiteral("Flags"), QStringLiteral("0"), QStringLiteral("126")),
            stringOp(QStringLiteral("HKCU\\Control Panel\\Accessibility\\SoundSentry"), QStringLiteral("Flags"), QStringLiteral("0"), QStringLiteral("2")),
            stringOp(QStringLiteral("HKCU\\Control Panel\\Accessibility\\TimeOut"), QStringLiteral("Flags"), QStringLiteral("0"), QStringLiteral("2")),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\ColorFiltering"), QStringLiteral("HotkeyEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\ScreenMagnifier"), QStringLiteral("Invert"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Ease of Access"), QStringLiteral("selfscan"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Ease of Access"), QStringLiteral("selfvoice"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Control Panel\\Accessibility\\SlateLaunch"), QStringLiteral("LaunchAT"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.input.cursor_visualization");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Cursor");
        t.name = QStringLiteral("Disable cursor and gesture visualization");
        t.description = QStringLiteral("Disables contact visualization, gesture visualization, cursor deadzone jumping, and mouse hover/trails for cleaner cursor behavior with minimal visual overhead.");
        t.iconName = QStringLiteral("palette");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Touch contact rings and gesture trails no longer display"),
            QStringLiteral("Mouse trails turned off"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Control Panel\\Cursors"), QStringLiteral("ContactVisualization"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Control Panel\\Cursors"), QStringLiteral("GestureVisualization"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Control Panel\\Cursors"), QStringLiteral("CursorDeadzoneJumpingSetting"), 0u, 1u),
            stringOp(QStringLiteral("HKCU\\Control Panel\\Mouse"), QStringLiteral("MouseHoverTime"), QStringLiteral("0"), QStringLiteral("400")),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\mouhid\\Parameters"), QStringLiteral("TreatAbsolutePointerAsAbsolute"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\mouhid\\Parameters"), QStringLiteral("TreatAbsoluteAsRelative"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.input.edge_swipe_disable");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Touch");
        t.name = QStringLiteral("Disable touch screen edge swipe");
        t.description = QStringLiteral("Disables the touch screen edge swipe gesture that triggers the Windows action center and task switcher. Prevents accidental activations during full-screen applications.");
        t.iconName = QStringLiteral("globe");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Touch screen edge swipes no longer open Action Center or Task View"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\LockDown\\AllowEdgeSwipe"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\EdgeUI"), QStringLiteral("AllowEdgeSwipe"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.input.input_experience_disable");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Input");
        t.name = QStringLiteral("Disable Windows input experience preload");
        t.description = QStringLiteral("Prevents the Windows Input Experience service from preloading and running in the background. Disables the Dsh prelaunch feature to reduce memory usage and startup overhead.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Touch keyboard and input panel take slightly longer to appear on first use"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\input"), QStringLiteral("IsInputAppPreloadEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Dsh"), QStringLiteral("IsPrelaunchEnabled"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.input.keyboard_extended");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Keyboard");
        t.name = QStringLiteral("Extended keyboard configuration");
        t.description = QStringLiteral("Sets the fastest key-repeat delay and rate and enables NumLock at sign-in. Note it also enables CrashOnCtrlScroll, a debugging feature that lets a Ctrl+ScrollLock+ScrollLock keystroke deliberately trigger a BSOD/crash dump - not an optimization.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Ctrl+Scroll will trigger kernel debug dump on USB keyboards"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Control Panel\\Keyboard"), QStringLiteral("TypematicDelay"), 1u, 250u),
            dwordOp(QStringLiteral("HKCU\\Control Panel\\Keyboard"), QStringLiteral("TypematicRate"), 1u, 30u),
            stringOp(QStringLiteral("HKCU\\Control Panel\\Keyboard"), QStringLiteral("InitialKeyboardIndicators"), QStringLiteral("2"), QStringLiteral("0")),
            stringOp(QStringLiteral("HKCU\\Control Panel\\Accessibility\\Keyboard Preference"), QStringLiteral("On"), QStringLiteral("1"), QStringLiteral("0")),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\kbdhid\\Parameters"), QStringLiteral("CrashOnCtrlScroll"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.input.mouse_accel_disable");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Mouse");
        t.name = QStringLiteral("Disable mouse acceleration");
        t.description = QStringLiteral("Disables Windows mouse acceleration (enhance pointer precision) for raw 1:1 mouse input. Pointer distance then depends only on how far the mouse moved, not on how fast it was moved.");
        t.iconName = QStringLiteral("zap");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Enhance Pointer Precision turned off; cursor feel will change across the desktop"),
        };
        t.registryOps = {
            stringOp(QStringLiteral("HKCU\\Control Panel\\Mouse"), QStringLiteral("MouseSpeed"), QStringLiteral("0"), QStringLiteral("1")),
            stringOp(QStringLiteral("HKCU\\Control Panel\\Mouse"), QStringLiteral("MouseThreshold1"), QStringLiteral("0"), QStringLiteral("6")),
            stringOp(QStringLiteral("HKCU\\Control Panel\\Mouse"), QStringLiteral("MouseThreshold2"), QStringLiteral("0"), QStringLiteral("10")),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"), QStringLiteral("NoEnhancedPointerPrecision"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.input.mouse_extended");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Mouse");
        t.name = QStringLiteral("Extended mouse optimization");
        t.description = QStringLiteral("Disables cursor suppression, configures controller processor cursor settings (sensitivity, update interval, magnetism delay, velocity), sets default user profile mouse settings for consistent behavior across login screens, and enables raw mouse throttling for reduced input latency.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Cursor behavior changes at the login screen"),
            QStringLiteral("Controller/IR-remote cursor magnetism disabled"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"), QStringLiteral("EnableCursorSuppression"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Input\\Settings\\ControllerProcessor\\CursorSpeed"), QStringLiteral("CursorSensitivity"), 10000u, 2000u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Input\\Settings\\ControllerProcessor\\CursorSpeed"), QStringLiteral("CursorUpdateInterval"), 1u, 8u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Input\\Settings\\ControllerProcessor\\CursorSpeed"), QStringLiteral("IRRemoteNavigationDelta"), 1u, 5u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Input\\Settings\\ControllerProcessor\\CursorMagnetism"), QStringLiteral("MagnetismDelayInMilliseconds"), 2u, 50u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Input\\Settings\\ControllerProcessor\\CursorMagnetism"), QStringLiteral("MagnetismUpdateIntervalInMilliseconds"), 1u, 16u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Input\\Settings\\ControllerProcessor\\CursorMagnetism"), QStringLiteral("AttractionRectInsetInDIPS"), 5u, 10u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Input\\Settings\\ControllerProcessor\\CursorMagnetism"), QStringLiteral("DistanceThresholdInDIPS"), 40u, 80u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Input\\Settings\\ControllerProcessor\\CursorMagnetism"), QStringLiteral("VelocityInDIPSPerSecond"), 360u, 720u),
            dwordOp(QStringLiteral("HKCU\\Control Panel\\Mouse"), QStringLiteral("RawMouseThrottleEnabled"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Control Panel\\Mouse"), QStringLiteral("RawMouseThrottleForced"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Control Panel\\Mouse"), QStringLiteral("RawMouseThrottleDuration"), 20u, std::nullopt),
            stringOp(QStringLiteral("HKU\\.DEFAULT\\Control Panel\\Mouse"), QStringLiteral("MouseSpeed"), QStringLiteral("0"), QStringLiteral("1")),
            stringOp(QStringLiteral("HKU\\.DEFAULT\\Control Panel\\Mouse"), QStringLiteral("MouseThreshold1"), QStringLiteral("0"), QStringLiteral("6")),
            stringOp(QStringLiteral("HKU\\.DEFAULT\\Control Panel\\Mouse"), QStringLiteral("MouseThreshold2"), QStringLiteral("0"), QStringLiteral("10")),
            stringOp(QStringLiteral("HKU\\.DEFAULT\\Control Panel\\Mouse"), QStringLiteral("MouseHoverTime"), QStringLiteral("0"), QStringLiteral("400")),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\mouhid\\Parameters"), QStringLiteral("TreatAbsolutePointerAsAbsolute"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\mouhid\\Parameters"), QStringLiteral("TreatAbsoluteAsRelative"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.input.pen_ink");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Pen & Ink");
        t.name = QStringLiteral("Pen and ink workspace optimization");
        t.description = QStringLiteral("Configures pen arbitration, disables pen visualization overlays, disables Windows Ink Workspace and app suggestions, disables inking with touch on embedded controls, sets fast press-and-hold and fast double-tap timings.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Windows Ink Workspace disabled"),
            QStringLiteral("Pen hold timings and tap behavior changes"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows\\Pen"), QStringLiteral("PenArbitrationType"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows\\Pen"), QStringLiteral("PenInteractionModel"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Control Panel\\Cursors"), QStringLiteral("PenVisualization"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Pen"), QStringLiteral("EnableEmbeddedInkControl"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\WindowsInkWorkspace\\AllowWindowsInkWorkspace"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\WindowsInkWorkspace\\AllowSuggestedAppsInWindowsInkWorkspace"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\PenWorkspace"), QStringLiteral("PenWorkspaceAppSuggestionsEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\TabletTip\\EmbeddedInkControl"), QStringLiteral("EnableInkingWithTouch"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\WindowsInkWorkspace"), QStringLiteral("AllowSuggestedAppsInWindowsInkWorkspace"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\WindowsInkWorkspace"), QStringLiteral("AllowWindowsInkWorkspace"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Control Panel\\Pen"), QStringLiteral("HoldActivationTime"), 8192u, 3000u),
            dwordOp(QStringLiteral("HKCU\\Control Panel\\Pen"), QStringLiteral("HoldWidth"), 8u, 4u),
            dwordOp(QStringLiteral("HKCU\\Control Panel\\Pen"), QStringLiteral("DoubleTapTime"), 0u, 300u),
            dwordOp(QStringLiteral("HKCU\\Control Panel\\Pen"), QStringLiteral("DoubleClickHeight"), 64u, 16u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.input.tablet_tip");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Touch keyboard");
        t.name = QStringLiteral("TabletPC touch keyboard optimization");
        t.description = QStringLiteral("Optimizes the Windows touch keyboard (TabletTip 1.7): hides IPTIP targets, disables autocorrect/spellcheck/text prediction, disables AC integration, enables shift lock, disables edge target and audio feedback. Applies to both HKCU and HKLM. Also disables TIPC tracking.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Touch typing may be less convenient without predictions"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\TabletTip\\1.7"), QStringLiteral("HideIPTIPTarget"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\TabletTip\\1.7"), QStringLiteral("HideIPTIPTouchTarget"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\TabletTip\\1.7"), QStringLiteral("IncludeRareChar"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\TabletTip\\1.7"), QStringLiteral("DisableEdgeTarget"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\TabletTip\\1.7"), QStringLiteral("DisableACIntegration"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\TabletTip\\1.7"), QStringLiteral("EnableAutocorrection"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\TabletTip\\1.7"), QStringLiteral("EnableSpellchecking"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\TabletTip\\1.7"), QStringLiteral("EnableTextPrediction"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\TabletTip\\1.7"), QStringLiteral("EnablePredictionSpaceInsertion"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\TabletTip\\1.7"), QStringLiteral("EnableDoubleTapSpace"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\TabletTip\\1.7"), QStringLiteral("EnableInkingWithTouch"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\TabletTip\\1.7"), QStringLiteral("EnableShiftLock"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\TabletTip\\1.7"), QStringLiteral("EnableAutoShiftEngage"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\TabletTip\\1.7"), QStringLiteral("EnableKeyAudioFeedback"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\TabletTip\\1.7"), QStringLiteral("EnableDesktopModeAutoInvoke"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\TabletTip\\1.7"), QStringLiteral("HideIPTIPTarget"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\TabletTip\\1.7"), QStringLiteral("HideIPTIPTouchTarget"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\TabletTip\\1.7"), QStringLiteral("DisableEdgeTarget"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\TabletTip\\1.7"), QStringLiteral("DisableACIntegration"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\TabletTip\\1.7"), QStringLiteral("EnableAutocorrection"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\TabletTip\\1.7"), QStringLiteral("EnableSpellchecking"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\TabletTip\\1.7"), QStringLiteral("EnableTextPrediction"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Input\\TIPC"), QStringLiteral("Enabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Input\\TIPC"), QStringLiteral("Enabled"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.input.touchpad_gestures");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Touchpad");
        t.name = QStringLiteral("Disable touchpad multi-finger gestures");
        t.description = QStringLiteral("Disables three-finger tap, four-finger slide and tap gestures on precision touchpads. Configures touchpad right-click zone and space bar offset for optimized layout.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Three- and four-finger touchpad gestures (task view, desktop switch) disabled"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\PrecisionTouchPad"), QStringLiteral("ThreeFingerTapEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\PrecisionTouchPad"), QStringLiteral("FourFingerSlideEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\PrecisionTouchPad"), QStringLiteral("FourFingerTapEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\PrecisionTouchPad"), QStringLiteral("HorizontalOffset"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\PrecisionTouchPad"), QStringLiteral("RightClickZoneHeight"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\PrecisionTouchPad"), QStringLiteral("RightClickZoneWidth"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\PrecisionTouchPad"), QStringLiteral("SpaceBarOffset"), 1000u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.input.touch_pen");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Touch & Pen");
        t.name = QStringLiteral("Touch and pen input optimization");
        t.description = QStringLiteral("Optimizes touch prediction latency and sample time, configures precision touchpad, disables pen training overlay, and sets fast gesture execution delay.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Touch and pen responsiveness may feel different after optimization"),
            QStringLiteral("Palm rejection behavior changes"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\TouchPrediction"), QStringLiteral("Latency"), 1u, 8u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\TouchPrediction"), QStringLiteral("SampleTime"), 1u, 8u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\TouchPrediction"), QStringLiteral("UseHWTimeStamp"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Wisp\\Touch"), QStringLiteral("SampleTime"), 2u, 10u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Wisp\\Touch"), QStringLiteral("PalmCheck"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Control Panel\\Desktop"), QStringLiteral("GestureExecutionDelay"), 0u, 100u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\PenTraining"), QStringLiteral("DisablePenTraining"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\PrecisionTouchPad"), QStringLiteral("AAPDisabled"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.input.windows_input_settings");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Input privacy");
        t.name = QStringLiteral("Disable Windows input personalization and autocorrect");
        t.description = QStringLiteral("Comprehensive Windows Input settings: disables autocorrect, autocapitalize, spellcheck, text prediction, personalization, emoji suggestions, hardware keyboard mode, and touch keyboard audio feedback across HKLM, HKCU, and HKU\\\\.DEFAULT.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Typing assistance features unavailable"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Input\\Settings"), QStringLiteral("AutoCorrection"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Input\\Settings"), QStringLiteral("AutoCap"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Input\\Settings"), QStringLiteral("Spellcheck"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Input\\Settings"), QStringLiteral("Prediction"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Input\\Settings"), QStringLiteral("DisablePersonalization"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Input\\Settings"), QStringLiteral("EmojiSuggestion"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Input\\Settings"), QStringLiteral("EnableHwkbAutocorrection"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Input\\Settings"), QStringLiteral("EnableHwkbTextPrediction"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Input\\Settings"), QStringLiteral("HarvestContacts"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Input\\Settings"), QStringLiteral("InsightsEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Input\\Settings"), QStringLiteral("TouchKeyboard_EnableKeyAudioFeedback"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Input\\Settings"), QStringLiteral("EnableHwkbMode"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Input\\Settings"), QStringLiteral("HTREnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Input\\Settings"), QStringLiteral("LMDataLoggerEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Input\\Settings"), QStringLiteral("AutoCorrection"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Input\\Settings"), QStringLiteral("AutoCap"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Input\\Settings"), QStringLiteral("Spellcheck"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Input\\Settings"), QStringLiteral("Prediction"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Input\\Settings"), QStringLiteral("DisablePersonalization"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Input\\Settings"), QStringLiteral("EmojiSuggestion"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Input\\Settings"), QStringLiteral("EnableHwkbAutocorrection"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Input\\Settings"), QStringLiteral("EnableHwkbTextPrediction"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Input\\Settings"), QStringLiteral("HarvestContacts"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Input\\Settings"), QStringLiteral("InsightsEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Input\\Settings"), QStringLiteral("TouchKeyboard_EnableKeyAudioFeedback"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Input\\Settings"), QStringLiteral("EnableHwkbMode"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Input\\Settings"), QStringLiteral("HTREnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Input\\Settings"), QStringLiteral("LMDataLoggerEnabled"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.kernel.console_vt");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Console");
        t.name = QStringLiteral("Enable console virtual terminal");
        t.description = QStringLiteral("Enables ANSI escape sequence processing in the Windows console (VirtualTerminalLevel=1).");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Legacy console apps that output raw escape codes may render differently"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\CONSOLE"), QStringLiteral("VirtualTerminalLevel"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.kernel.disable_ngen");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Disable .NET NGEN background compilation");
        t.description = QStringLiteral("Disables the .NET Native Image Generator (NGEN) background compilation for both 32-bit and 64-bit frameworks.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral(".NET cold start time may increase"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\CI\\NGEN"), QStringLiteral("C:\\Windows\\Microsoft.NET\\Framework\\v4.0.30319\\mscorsvw.exe"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\CI\\NGEN"), QStringLiteral("C:\\Windows\\Microsoft.NET\\Framework64\\v4.0.30319\\mscorsvw.exe"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.kernel.offline_maps");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Maps");
        t.name = QStringLiteral("Disable offline maps auto-update");
        t.description = QStringLiteral("Disables automatic offline map downloads and restricts updates to WiFi only.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Offline maps may become outdated"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\Maps"), QStringLiteral("AutoUpdateEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\Maps"), QStringLiteral("UpdateOnlyOnWifi"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.kernel.system_policies");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Group Policy");
        t.name = QStringLiteral("System policy performance optimization");
        t.description = QStringLiteral("A grab-bag of Group Policy values (disable HTML-Help DEP, delete roaming cache at logoff, suppress lock-screen app notifications, disable RSoP logging). Note it enables online font providers rather than disabling them, and the overall performance benefit is unproven.");
        t.iconName = QStringLiteral("shield");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Lock-screen app notifications suppressed"),
            QStringLiteral("Roaming-profile cache deleted at logoff on domain-joined machines"),
            QStringLiteral("RSoP logging disabled, reducing Group Policy diagnostics"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System"), QStringLiteral("EnableFontProviders"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System"), QStringLiteral("DisableHHDEP"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System"), QStringLiteral("SlowLinkDetectEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System"), QStringLiteral("DeleteRoamingCache"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System"), QStringLiteral("DisableLockScreenAppNotifications"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System"), QStringLiteral("RSoPLogging"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.kernel.update_policy");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Windows Update");
        t.name = QStringLiteral("Control Windows Update behavior");
        t.description = QStringLiteral("Sets updates to notify-before-download (AUOptions=2) and prevents forced reboots while a user is logged in.");
        t.iconName = QStringLiteral("rotate");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Security updates require manual approval and can be delayed"),
            QStringLiteral("Forced reboots blocked while users are signed in"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate\\AU"), QStringLiteral("NoAutoUpdate"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate\\AU"), QStringLiteral("AUOptions"), 2u, 4u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate\\AU"), QStringLiteral("NoAutoRebootWithLoggedOnUsers"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.input.keyboard_optimize");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Keyboard");
        t.name = QStringLiteral("Optimize keyboard response");
        t.description = QStringLiteral("Sets the fastest key-repeat delay and speed. Note it also enables CrashOnCtrlScroll on both PS/2 and USB keyboards - a debugging feature where Ctrl+ScrollLock+ScrollLock deliberately triggers a system crash/dump.");
        t.iconName = QStringLiteral("keyboard");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Held keys repeat immediately and at maximum rate"),
        };
        t.registryOps = {
            // Windows defaults: KeyboardDelay 1, KeyboardSpeed 31. KeyboardSpeed is
            // already stock, so the real work is in the ops below.
            stringOp(QStringLiteral("HKCU\\Control Panel\\Keyboard"), QStringLiteral("KeyboardDelay"), QStringLiteral("0"), QStringLiteral("1")),
            stringOp(QStringLiteral("HKCU\\Control Panel\\Keyboard"), QStringLiteral("KeyboardSpeed"), QStringLiteral("31"), QStringLiteral("31")),
            dwordOp(QStringLiteral("HKCU\\Control Panel\\Keyboard"), QStringLiteral("PrintScreenKeyForSnippingEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\kbdclass\\Parameters"), QStringLiteral("KeyboardDataQueueSize"), 60u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\i8042prt\\Parameters"), QStringLiteral("CrashOnCtrlScroll"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\kbdhid\\Parameters"), QStringLiteral("CrashOnCtrlScroll"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.limit-windows-update-bandwidth");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Windows Update");
        t.name = QStringLiteral("Cap Windows Update download bandwidth");
        // Delivery Optimization measures DOMaxForegroundDownloadBandwidth in
        // KILOBYTES per second, per the Delivery Optimization reference. 6250 is
        // 50 Mbps. Any change to this value changes the description with it.
        t.description = QStringLiteral("Caps Delivery Optimization's foreground downloads at 50 Mbps, which is the ceiling Windows Update and Store downloads share.");
        t.iconName = QStringLiteral("player-track-next");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Windows Update and Store downloads are held to 50 Mbps, which is slower than a fast connection would manage"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\DeliveryOptimization"), QStringLiteral("DOMaxForegroundDownloadBandwidth"), 6250u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.maps.extended_disable");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Maps");
        t.name = QStringLiteral("Extended offline maps policy disable");
        t.description = QStringLiteral("Additional maps policy settings: disables map data auto-download via policy and prevents metered connection map downloads.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Offline maps will not auto-download or auto-update"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Maps"), QStringLiteral("AutoDownloadAndUpdateMapData"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Maps"), QStringLiteral("AllowUntriggeredNetworkTrafficOnSettingsPage"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\Maps\\AllowOfflineMapsDownloadOverMeteredConnection"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\Maps\\EnableOfflineMapsAutoUpdate"), QStringLiteral("value"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.media.evr_config");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Media");
        t.name = QStringLiteral("Enhanced video renderer optimization");
        t.description = QStringLiteral("Configures EVR for optimal video playback: enables throttled output and half-interlace, disables Bob workaround.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Legacy Enhanced Video Renderer playback may drop frames on interlaced content"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\EVR"), QStringLiteral("ForceThrottleOutput"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\EVR"), QStringLiteral("ForceHalfInterlace"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\EVR"), QStringLiteral("ForceBob"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\EVR"), QStringLiteral("DisableBobWorkaround"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\EVR"), QStringLiteral("ForceAllowDropToHalfInterlace"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\EVR"), QStringLiteral("ForceAllowThrottleOutput"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.mgmt.mdm_enrollment");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("MDM");
        t.name = QStringLiteral("Register Windows MDM enrollment");
        t.description = QStringLiteral("Registers a fake MDM enrollment to prevent automatic MDM re-enrollment prompts on Azure AD joined devices.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Enterprise MDM enrollment may be blocked"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Enrollments\\FFFFFFFF-FFFF-FFFF-FFFF-FFFFFFFFFFFF"), QStringLiteral("EnrollmentState"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Enrollments\\FFFFFFFF-FFFF-FFFF-FFFF-FFFFFFFFFFFF"), QStringLiteral("EnrollmentType"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Enrollments\\FFFFFFFF-FFFF-FFFF-FFFF-FFFFFFFFFFFF"), QStringLiteral("IsFederated"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.network.disable_workfolders");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Offline files");
        t.name = QStringLiteral("Disable Work Folders and Offline Files cache");
        t.description = QStringLiteral("Disables Work Folders auto-provisioning and Offline Files (CSC) including background sync, cache viewer, and logon/logoff sync.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Offline file caching disabled"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\WorkFolders"), QStringLiteral("AutoProvision"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\Software\\Policies\\Microsoft\\Windows\\NetCache"), QStringLiteral("Enabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\Software\\Policies\\Microsoft\\Windows\\NetCache"), QStringLiteral("BackgroundSyncEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\Software\\Policies\\Microsoft\\Windows\\NetCache"), QStringLiteral("NoCacheViewer"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\Software\\Policies\\Microsoft\\Windows\\NetCache"), QStringLiteral("NoConfigCache"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\Software\\Policies\\Microsoft\\Windows\\NetCache"), QStringLiteral("NoMakeAvailableOffline"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\Software\\Policies\\Microsoft\\Windows\\NetCache"), QStringLiteral("SyncAtLogoff"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\Software\\Policies\\Microsoft\\Windows\\NetCache"), QStringLiteral("SyncAtLogon"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.notify.push_optimize");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Notifications");
        t.name = QStringLiteral("Push notification optimization");
        t.description = QStringLiteral("Disables critical toasts above lock, notification sounds, cloud notifications, tile notifications, and HMD notifications.");
        t.iconName = QStringLiteral("activity");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Toast notifications silenced"),
            QStringLiteral("Cloud and tile push notifications blocked"),
            QStringLiteral("Critical toasts will not show above the lock screen"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Notifications\\Settings"), QStringLiteral("NOC_GLOBAL_SETTING_ALLOW_CRITICAL_TOASTS_ABOVE_LOCK"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Notifications\\Settings"), QStringLiteral("NOC_GLOBAL_SETTING_ALLOW_NOTIFICATION_SOUND"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\CurrentVersion\\PushNotifications"), QStringLiteral("NoToastApplicationNotificationOnLockScreen"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\CurrentVersion\\PushNotifications"), QStringLiteral("NoCloudApplicationNotification"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\PushNotifications"), QStringLiteral("NoTileApplicationNotification"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Shell\\USB"), QStringLiteral("NotifyOnWeakCharger"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Shell\\USB"), QStringLiteral("NotifyOnUsbErrors"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("system.ntfs-disable-compression");
        t.category = QStringLiteral("storage");
        t.subCategory = QStringLiteral("NTFS");
        t.name = QStringLiteral("Disable NTFS filesystem compression");
        t.description = QStringLiteral("Disables filesystem-level compression. Recommended off on SSDs and modern multi-GB RAM systems.");
        t.iconName = QStringLiteral("database");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\FileSystem"), QStringLiteral("DisableCompression"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.perf.reduce_delays");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Responsiveness");
        t.name = QStringLiteral("Reduce system delays and timeouts");
        t.description = QStringLiteral("Minimizes shutdown/app kill timeouts, disables shutdown reason dialogs, enables auto-end of hung tasks, reduces menu show delay and foreground lock timeout.");
        t.iconName = QStringLiteral("clock");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Unsaved work may be lost on shutdown"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Reliability"), QStringLiteral("ShutdownReasonOn"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Reliability"), QStringLiteral("ShutdownReasonUI"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Windows"), QStringLiteral("ShutdownWarningDialogTimeout"), 1u, 10u),
            stringOp(QStringLiteral("HKCU\\Control Panel\\Desktop"), QStringLiteral("AutoEndTasks"), QStringLiteral("1"), QStringLiteral("0")),
            stringOp(QStringLiteral("HKCU\\Control Panel\\Desktop"), QStringLiteral("MenuShowDelay"), QStringLiteral("0"), QStringLiteral("400")),
            dwordOp(QStringLiteral("HKCU\\Control Panel\\Desktop"), QStringLiteral("ForegroundLockTimeout"), 0u, 200000u),
            stringOp(QStringLiteral("HKCU\\Control Panel\\Desktop"), QStringLiteral("WaitToKillAppTimeout"), QStringLiteral("1"), QStringLiteral("20000")),
            stringOp(QStringLiteral("HKCU\\Control Panel\\Desktop"), QStringLiteral("HungAppTimeout"), QStringLiteral("1000"), QStringLiteral("5000")),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control"), QStringLiteral("WaitToKillServiceTimeout"), QStringLiteral("1"), QStringLiteral("5000")),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\PnP"), QStringLiteral("PollBootPartitionTimeout"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.power.extended");
        t.category = QStringLiteral("power");
        t.subCategory = QStringLiteral("C-States");
        t.name = QStringLiteral("Extended power and processor optimization");
        t.description = QStringLiteral("Disables CPPC, PEP performance states, CPU quota, I/O count operations, ASPM, NV cache optimization, and Bluetooth quick pair.");
        t.iconName = QStringLiteral("zap");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Higher CPU and PCIe power consumption"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Processor"), QStringLiteral("CPPCEnable"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Processor"), QStringLiteral("AllowPepPerfStates"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Quota System"), QStringLiteral("EnableCpuQuota"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\I/O System"), QStringLiteral("CountOperations"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\pci\\Parameters"), QStringLiteral("ASPMOptOut"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\NvCache"), QStringLiteral("OptimizeBootAndResume"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\NvCache"), QStringLiteral("EnablePowerModeState"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Throttle"), QStringLiteral("PerfEnablePackageIdle"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Bluetooth"), QStringLiteral("QuickPair"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.privacy.disable_clipboard");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Clipboard");
        t.name = QStringLiteral("Disable clipboard history and cross-device sync");
        t.description = QStringLiteral("Disables clipboard history, cross-device clipboard sync, and cloud clipboard integration.");
        t.iconName = QStringLiteral("eye-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Win+V clipboard history no longer works"),
            QStringLiteral("Clipboard no longer syncs between devices"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Clipboard"), QStringLiteral("EnableClipboardHistory"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\Experience\\AllowClipboardHistory"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System"), QStringLiteral("AllowCrossDeviceClipboard"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System"), QStringLiteral("AllowClipboardHistory"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.privacy.disable_findmydevice");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Privacy");
        t.name = QStringLiteral("Disable Find My Device");
        t.description = QStringLiteral("Disables the Find My Device feature and location sync across all policy and settings locations.");
        t.iconName = QStringLiteral("map-pin-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Remote device location disabled"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\FindMyDevice"), QStringLiteral("AllowFindMyDevice"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\FindMyDevice"), QStringLiteral("LocationSyncEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\Experience\\AllowFindMyDevice"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Settings\\FindMyDevice"), QStringLiteral("LocationSyncEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\MdmCommon\\SettingValues"), QStringLiteral("LocationSyncEnabled"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.privacy.disable_resume");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Privacy");
        t.name = QStringLiteral("Disable cross-device resume");
        t.description = QStringLiteral("Disables activity sync across Microsoft account devices.");
        t.iconName = QStringLiteral("eye-off");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Cross-device resume for phone and PC activity disabled"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\CrossDeviceResume\\Configuration"), QStringLiteral("IsResumeAllowed"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.process.priority");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Process priority");
        t.name = QStringLiteral("Critical process priority boost");
        t.description = QStringLiteral("Sets high CPU and I/O priority for csrss.exe (Windows subsystem), explorer.exe, and audiodg.exe (audio engine).");
        t.iconName = QStringLiteral("folder-open");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("High-priority system processes may starve others"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\csrss.exe\\PerfOptions"), QStringLiteral("CpuPriorityClass"), 3u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\csrss.exe\\PerfOptions"), QStringLiteral("IoPriority"), 3u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\audiodg.exe\\PerfOptions"), QStringLiteral("CpuPriorityClass"), 3u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\explorer.exe\\PerfOptions"), QStringLiteral("CpuPriorityClass"), 3u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.security.disable_rdp");
        t.category = QStringLiteral("security");
        t.subCategory = QStringLiteral("Remote access");
        t.name = QStringLiteral("Disable Remote Desktop and Remote Assistance");
        t.description = QStringLiteral("Disables RDP connections, Remote Assistance, and hardens terminal server policies.");
        t.iconName = QStringLiteral("house");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Cannot remotely access via RDP"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Terminal Server"), QStringLiteral("fDenyTSConnections"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Terminal Server"), QStringLiteral("updateRDStatus"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Terminal Server"), QStringLiteral("fSingleSessionPerUser"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows NT\\Terminal Services"), QStringLiteral("fAllowToGetHelp"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows NT\\Terminal Services"), QStringLiteral("fDenyTSConnections"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows NT\\Terminal Services\\Client"), QStringLiteral("fClientDisableUDP"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Remote Assistance"), QStringLiteral("fAllowToGetHelp"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.security.disable_wpbt");
        t.category = QStringLiteral("security");
        t.subCategory = QStringLiteral("Firmware");
        t.name = QStringLiteral("Disable Windows Platform Binary Table execution");
        t.description = QStringLiteral("Prevents UEFI firmware from injecting binaries at boot via WPBT.");
        t.iconName = QStringLiteral("shield");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("OEM recovery/anti-theft tools may not function"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager"), QStringLiteral("DisableWpbtExecution"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.security.no_password_questions");
        t.category = QStringLiteral("security");
        t.subCategory = QStringLiteral("Accounts");
        t.name = QStringLiteral("Disable local password reset questions");
        t.description = QStringLiteral("Disables the security questions for local account password reset on the login screen.");
        t.iconName = QStringLiteral("shield");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Password recovery via security questions unavailable"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"), QStringLiteral("NoLocalPasswordResetQuestions"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System"), QStringLiteral("NoLocalPasswordResetQuestions"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.security.tpm_policy");
        t.category = QStringLiteral("security");
        t.subCategory = QStringLiteral("TPM");
        t.name = QStringLiteral("TPM management policy");
        t.description = QStringLiteral("Sets TPM OS managed auth level to 0 and disables Active Directory backup for TPM owner information.");
        t.iconName = QStringLiteral("shield");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("TPM recovery may be harder without AD backup"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\TPM"), QStringLiteral("OSManagedAuthLevel"), 0u, 2u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\TPM"), QStringLiteral("ActiveDirectoryBackup"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\TPM"), QStringLiteral("RequireActiveDirectoryBackup"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.security.vuln_harden");
        t.category = QStringLiteral("security");
        t.subCategory = QStringLiteral("LSA hardening");
        t.name = QStringLiteral("Windows Security hardening");
        t.description = QStringLiteral("Hardens LSA: restricts anonymous SAM/network access, disables CredSSP debug logging, and disables legacy audit policy.");
        t.iconName = QStringLiteral("shield");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Anonymous SAM and network share enumeration blocked, which may break legacy file sharing"),
            QStringLiteral("CredSSP debug logging disabled"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Lsa"), QStringLiteral("RestrictAnonymousSAM"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Lsa"), QStringLiteral("RestrictAnonymous"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Lsa\\Credssp"), QStringLiteral("DebugLogLevel"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Lsa"), QStringLiteral("SCENoApplyLegacyAuditPolicy"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.services.delayed_time");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Increase delayed services start time");
        t.description = QStringLiteral("Sets delayed auto-start services to wait 360 seconds (6 min) before starting, reducing boot-time CPU contention.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Delayed-start services (Superfetch, BITS, etc.) take up to 6 minutes to begin after login"),
        };
        t.registryOps = {
            // 120 seconds is the Windows default delayed-autostart delay.
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control"), QStringLiteral("AutoStartDelay"), QStringLiteral("360"), QStringLiteral("120")),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.services.msi_wsh");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Enable MSI in SafeMode and Windows script hosting");
        t.description = QStringLiteral("Registers MSIServer for Minimal and Network SafeMode, and ensures Windows Script Host is enabled.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Windows Script Host re-enabled; .vbs/.js scripts can run from Explorer"),
        };
        t.registryOps = {
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\SafeBoot\\Network\\MSIServer"), QStringLiteral(""), QStringLiteral("Service"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\SafeBoot\\Minimal\\MSIServer"), QStringLiteral(""), QStringLiteral("Service"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows Script Host\\Settings"), QStringLiteral("Enabled"), QStringLiteral("1"), QStringLiteral("0")),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.time.service_optimize");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Time sync");
        t.name = QStringLiteral("Optimize time sync intervals");
        t.description = QStringLiteral("Shortens the Windows Time service polling intervals for more accurate system clock.");
        t.iconName = QStringLiteral("clock");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("More frequent NTP sync traffic (negligible bandwidth impact)"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\W32Time\\Config"), QStringLiteral("MaxPollInterval"), 10u, 15u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\W32Time\\Config"), QStringLiteral("MinPollInterval"), 6u, 10u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.ui.disable_mobility_center");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("UI");
        t.name = QStringLiteral("Disable Windows Mobility Center");
        t.description = QStringLiteral("Disables the Windows Mobility Center on desktop systems.");
        t.iconName = QStringLiteral("house");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Windows Mobility Center (Win+X) unavailable; affects some laptop workflows"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\MobilityCenter"), QStringLiteral("NoMobilityCenter"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.ui.disable_snap_assist");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Snap");
        t.name = QStringLiteral("Disable Snap Assist and snap bar");
        t.description = QStringLiteral("Disables Snap Assist flyout, Snap Bar, and drag-from-maximize.");
        t.iconName = QStringLiteral("eye-off");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Snap Layouts flyout and Snap Bar no longer appear when arranging windows"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), QStringLiteral("EnableSnapAssistFlyout"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), QStringLiteral("EnableSnapBar"), 0u, 1u),
            stringOp(QStringLiteral("HKCU\\Control Panel\\Desktop"), QStringLiteral("DragFromMaximize"), QStringLiteral("0"), QStringLiteral("1")),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.ui.disable_spotlight");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Lock screen / Spotlight");
        t.name = QStringLiteral("Disable Windows Spotlight");
        t.description = QStringLiteral("Disables all Windows Spotlight features: third-party suggestions, lock screen, action center, settings, welcome experience, and tips.");
        t.iconName = QStringLiteral("shield");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Windows Spotlight rotating lock-screen images disabled"),
            QStringLiteral("Tips and suggestions no longer appear in Action Center and Settings"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\Experience\\AllowThirdPartySuggestionsInWindowsSpotlight"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\Experience\\AllowWindowsSpotlight"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\Experience\\AllowSpotlightCollection"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\Experience\\AllowWindowsSpotlightOnActionCenter"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\Experience\\AllowWindowsSpotlightOnSettings"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\Experience\\AllowWindowsSpotlightWindowsWelcomeExperience"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\Experience\\AllowWindowsTips"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\Experience\\ConfigureWindowsSpotlightOnLockScreen"), QStringLiteral("value"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.ui.disable_stickers");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("UI");
        t.name = QStringLiteral("Disable Windows desktop stickers");
        t.description = QStringLiteral("Disables the Windows 11 desktop stickers feature.");
        t.iconName = QStringLiteral("house");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Desktop stickers feature disabled; existing stickers no longer render"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\Stickers\\EnableStickers"), QStringLiteral("value"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.ui.msconfig_noreboot");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("UI");
        t.name = QStringLiteral("Suppress MSConfig reboot prompt");
        t.description = QStringLiteral("Disables the MSConfig reboot prompt after configuration changes.");
        t.iconName = QStringLiteral("rotate");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("MSConfig no longer prompts to reboot after changes; you must restart manually"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Shared Tools\\MsConfig"), QStringLiteral("NoRebootUI"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.ui.no_internet_openwith");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Explorer");
        t.name = QStringLiteral("Disable internet 'open with' dialog");
        t.description = QStringLiteral("Prevents the 'Look for an app in the Store' dialog when opening files with no associated application.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("'Look for an app in the Store' dialog no longer appears for unknown file types"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"), QStringLiteral("NoInternetOpenWith"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.ui.shortcut_arrow_remove");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Icons");
        t.name = QStringLiteral("Remove shortcut arrow overlay icon");
        t.description = QStringLiteral("Points Shell Icons\\\\29 to a blank .ico file, removing the small arrow overlay on shortcut icons. Requires a blank.ico file at C:\\\\Windows\\\\blank.ico.");
        t.iconName = QStringLiteral("folder-open");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Shortcut arrow overlay icons become invisible; requires C:\\\\Windows\\\\blank.ico to exist"),
            QStringLiteral("Shortcuts visually indistinguishable from regular files"),
        };
        t.registryOps = {
            // Removed on revert: "Shell Icons\29" doesn't exist on a stock install.
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Icons"), QStringLiteral("29"), QStringLiteral("C:\\Windows\\blank.ico"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.usb.port_optimize");
        t.category = QStringLiteral("power");
        t.subCategory = QStringLiteral("USB power");
        t.name = QStringLiteral("USB port optimization");
        t.description = QStringLiteral("Disables USB selective suspend at the driver level for consistent peripheral responsiveness.");
        t.iconName = QStringLiteral("usb");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("USB devices stay fully powered, increasing idle power draw on laptops"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\USB\\DisableSelectiveSuspend"), QStringLiteral("DisableSelectiveSuspend"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\USBXHCI\\Parameters"), QStringLiteral("IoQueueWorkItem"), 10u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\xusb22\\Parameters"), QStringLiteral("IoQueueWorkItem"), 10u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\usbhub\\hubg"), QStringLiteral("DisableOnSoftRemove"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\usbflags"), QStringLiteral("fid_D1Latency"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\usbflags"), QStringLiteral("fid_D2Latency"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\usbflags"), QStringLiteral("fid_D3Latency"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Enum\\USB"), QStringLiteral("AllowIdleIrpInD3"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Enum\\USB"), QStringLiteral("EnhancedPowerManagementEnabled"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.virt.disable_appv");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Virtualization");
        t.name = QStringLiteral("Disable App-V client and virtualization");
        t.description = QStringLiteral("Disables Microsoft Application Virtualization (APP-V) client, auto-cleanup, publishing refresh UI, reporting, dynamic virtualization, and CEIP telemetry.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("APP-V packages will not load"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\AppV\\Client"), QStringLiteral("Enabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\AppV\\Client\\PackageManagement"), QStringLiteral("AutoCleanupEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\AppV\\Client\\Publishing"), QStringLiteral("EnablePublishingRefreshUI"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\AppV\\Client\\Reporting"), QStringLiteral("ReportingEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\AppV\\Client\\Virtualization"), QStringLiteral("EnableDynamicVirtualization"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\AppV\\Client\\Virtualization"), QStringLiteral("ProcessesUsingVirtualComponents"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\AppV\\CEIP"), QStringLiteral("CEIPEnable"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.vr.holographic");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Mixed Reality");
        t.name = QStringLiteral("Holographic and Mixed Reality optimization");
        t.description = QStringLiteral("Disables virtual monitor pre-allocation, enables desktop app launch in mixed reality, disables screensaver mode, audio mirroring, and display throttling.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Windows Mixed Reality screensaver mode and audio mirroring disabled"),
            QStringLiteral("Virtual monitor preallocation off; first-launch latency may rise for VR apps"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Holographic"), QStringLiteral("PreallocateVirtualMonitors"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Holographic"), QStringLiteral("DesktopAppLaunchEnabled"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Holographic"), QStringLiteral("ScreensaverModeEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\HoloSI\\Audio"), QStringLiteral("AudioMirroringEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Holographic\\DisplayThrottling"), QStringLiteral("ThrottleExperience"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.wu.delivery_optimization");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Windows Update");
        t.name = QStringLiteral("Disable Delivery Optimization (P2P updates)");
        t.description = QStringLiteral("Disables Windows Update Delivery Optimization peer-to-peer update sharing across all policy and settings locations.");
        t.iconName = QStringLiteral("rotate");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Windows Update downloads directly from Microsoft instead of peers, using more WAN bandwidth"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DeliveryOptimization\\Settings"), QStringLiteral("DownloadMode"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\DeliveryOptimization"), QStringLiteral("DODownloadMode"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DeliveryOptimization\\Config"), QStringLiteral("DODownloadMode"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.wu.driver_updates_extended");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Windows Update");
        t.name = QStringLiteral("Extended driver update controls");
        t.description = QStringLiteral("Prevents driver searching via Windows Update, disables device metadata from network, and disables driver co-installers.");
        t.iconName = QStringLiteral("rotate");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Drivers will not auto-update via WU"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\DriverSearching"), QStringLiteral("DontSearchWindowsUpdate"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\DriverSearching"), QStringLiteral("DontPromptForWindowsUpdate"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\DriverSearching"), QStringLiteral("DriverUpdateWizardWuSearchEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\DeviceInstall\\Settings"), QStringLiteral("PreventDeviceMetadataFromNetwork"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Device Metadata"), QStringLiteral("PreventDeviceMetadataFromNetwork"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Device Installer"), QStringLiteral("DisableCoInstallers"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.wu.extended_policy");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Windows Update");
        t.name = QStringLiteral("Extended Windows Update policy");
        t.description = QStringLiteral("Disables auto-restart notifications, restricts metered network downloads, disables dual scan, configures notification levels, disables featured software and recommended updates.");
        t.iconName = QStringLiteral("rotate");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Updates may be delayed"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate"), QStringLiteral("SetAutoRestartNotificationDisable"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate"), QStringLiteral("AUPowerManagement"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate"), QStringLiteral("DisableDualScan"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate"), QStringLiteral("ConfigureDeadlineNoAutoReboot"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate\\AU"), QStringLiteral("NoAUShutdownOption"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate\\AU"), QStringLiteral("EnableFeaturedSoftware"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate\\AU"), QStringLiteral("AutoInstallMinorUpdates"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate\\AU"), QStringLiteral("IncludeRecommendedUpdates"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\WindowsUpdate\\UX\\Settings"), QStringLiteral("RestartNotificationsAllowed2"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\WindowsUpdate\\UX\\Settings"), QStringLiteral("TrayIconVisibility"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.wu.feature_update_control");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Windows Update");
        t.name = QStringLiteral("Feature update deadline and deferral");
        t.description = QStringLiteral("Configures feature update deadlines and defers them by 7 days.");
        t.iconName = QStringLiteral("rotate");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("New Windows features delayed"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate"), QStringLiteral("ConfigureDeadlineForFeatureUpdates"), 7u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate"), QStringLiteral("ConfigureDeadlineGracePeriodForFeatureUpdates"), 7u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate"), QStringLiteral("DeferFeatureUpdatesPeriodInDays"), 7u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.wu.mrt_disable");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Windows Update");
        t.name = QStringLiteral("Disable Malicious Software Removal Tool");
        t.description = QStringLiteral("Prevents MRT from being offered via Windows Update, disables infection reporting and feedback notifications.");
        t.iconName = QStringLiteral("rotate");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("MRT monthly malware scan disabled"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\MRT"), QStringLiteral("DontOfferThroughWUAU"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\MRT"), QStringLiteral("DontReportInfectionInformation"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\MRT"), QStringLiteral("DoNotShowFeedbackNotifications"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.wu.ux_settings");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Windows Update");
        t.name = QStringLiteral("Windows Update UX: smart active hours and notify-only");
        t.description = QStringLiteral("Sets SmartActiveHoursState=2 (manual active hours), UxOption=1 (notify to schedule restart), ActiveHoursStart=13 (1 PM), ActiveHoursEnd=7 (7 AM).");
        t.iconName = QStringLiteral("rotate");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Auto-restart after updates disabled"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\WindowsUpdate\\UX\\Settings"), QStringLiteral("SmartActiveHoursState"), 2u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\WindowsUpdate\\UX\\Settings"), QStringLiteral("UxOption"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\WindowsUpdate\\UX\\Settings"), QStringLiteral("ActiveHoursStart"), 13u, 8u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\WindowsUpdate\\UX\\Settings"), QStringLiteral("ActiveHoursEnd"), 7u, 17u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("system.bcd-skip-boot-menu-timeout");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Boot");
        t.declaredTier = vth::models::ProductTier::Ultimate;
        t.name = QStringLiteral("Skip the boot OS-selection menu");
        t.description = QStringLiteral("Sets bcdedit timeout 0. The OS-selection menu doesn't wait for a user choice.");
        t.iconName = QStringLiteral("stopwatch");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        CommandOp bo0;
        bo0.program = QStringLiteral("bcdedit");
        bo0.applyArgs = { QStringLiteral("/set"), QStringLiteral("timeout"), QStringLiteral("0") };
        bo0.revertArgs = { QStringLiteral("/deletevalue"), QStringLiteral("timeout") };
        t.commandOps.push_back(bo0);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.bcd.tsc_timers");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Timers");
        t.declaredTier = vth::models::ProductTier::Ultimate;
        t.name = QStringLiteral("BCD TSC and timer optimization");
        t.description = QStringLiteral("Uses bcdedit to set tscsyncpolicy Enhanced and useplatformtick yes. Note the description's 'drops HPET' framing is misleading: useplatformtick yes forces a platform tick source, and the latency effect is hardware-dependent and unproven.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        CommandOp bo0;
        bo0.program = QStringLiteral("bcdedit");
        bo0.applyArgs = { QStringLiteral("/set"), QStringLiteral("tscsyncpolicy"), QStringLiteral("Enhanced") };
        bo0.revertArgs = { QStringLiteral("/deletevalue"), QStringLiteral("tscsyncpolicy") };
        t.commandOps.push_back(bo0);
        CommandOp bo1;
        bo1.program = QStringLiteral("bcdedit");
        bo1.applyArgs = { QStringLiteral("/set"), QStringLiteral("useplatformtick"), QStringLiteral("yes") };
        bo1.revertArgs = { QStringLiteral("/deletevalue"), QStringLiteral("useplatformtick") };
        t.commandOps.push_back(bo1);
        c.push_back(t);
    }


    {
        TweakSpec t;
        t.id = QStringLiteral("sys.input.controller_polling");
        t.category = QStringLiteral("general");
        t.name = QStringLiteral("USB controller polling rate optimization");
        t.description = QStringLiteral("Optimizes USB HID polling rate settings and disables controller power management for lowest input delay.");
        t.iconName = QStringLiteral("gamepad-2");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Very slight increase in USB bus and CPU usage"),
            QStringLiteral("May reduce battery life on wireless controllers"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\mouclass\\Parameters"), QStringLiteral("MouseDataQueueSize"), 32u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\kbdclass\\Parameters"), QStringLiteral("KeyboardDataQueueSize"), 32u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\HidUsb\\Parameters"), QStringLiteral("EnhancedPowerManagementEnabled"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.devices.cdrom");
        t.category = QStringLiteral("storage");
        t.name = QStringLiteral("CD/Optical drive maximum read speed");
        t.description = QStringLiteral("Configures CDFS driver for maximum read performance: increases prefetch and prefetch-tail buffers to 16KB and sets cache size to 64KB.");
        t.iconName = QStringLiteral("usb");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Increases memory used by the CDFS driver"),
            QStringLiteral("No effect on systems without an optical drive"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\FileSystem\\CDFS"), QStringLiteral("PrefetchTail"), 16384u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\FileSystem\\CDFS"), QStringLiteral("Prefetch"), 16384u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\FileSystem\\CDFS"), QStringLiteral("CacheSize"), 65535u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.devices.disk_extended");
        t.category = QStringLiteral("storage");
        t.name = QStringLiteral("Extended disk and storage optimization");
        t.description = QStringLiteral("Disables leap second support, disables StorPort telemetry (performance, error data, device health), disables NVMe performance throttling, disables iaStor link power management on all ports, and disables swapfile.");
        t.iconName = QStringLiteral("usb");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Swapfile disabled; UWP/Store apps may crash under memory pressure"),
            QStringLiteral("Higher idle power draw from SATA drives"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\LeapSecondInformation"), QStringLiteral("Enabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorPort"), QStringLiteral("TelemetryPerformanceEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorPort"), QStringLiteral("TelemetryErrorDataEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorPort"), QStringLiteral("TelemetryDeviceHealthEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorPort"), QStringLiteral("HmbAllocationPolicy"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Classpnp"), QStringLiteral("NVMeDisablePerfThrottling"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\services\\iaStor\\Parameters\\Port0"), QStringLiteral("LPM"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\services\\iaStor\\Parameters\\Port0"), QStringLiteral("LPMDSTATE"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\services\\iaStor\\Parameters\\Port0"), QStringLiteral("DIPM"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\services\\iaStor\\Parameters\\Port1"), QStringLiteral("LPM"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\services\\iaStor\\Parameters\\Port1"), QStringLiteral("LPMDSTATE"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\services\\iaStor\\Parameters\\Port1"), QStringLiteral("DIPM"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"), QStringLiteral("SwapfileControl"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.devices.filesystem_extended");
        t.category = QStringLiteral("storage");
        t.name = QStringLiteral("Extended NTFS and filesystem optimization");
        t.description = QStringLiteral("Comprehensive filesystem settings beyond basic NTFS: enables long paths, forces non-paged pool allocation, sets MFT zone reservation to maximum, increases file name and path cache sizes, enables symlink evaluation, disables TRIM notification, disables spot corruption handling, and increases copy buffer size.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("TRIM (DisableDeleteNotification) disabled, may reduce SSD long-term performance"),
            QStringLiteral("NTFS spot-corruption auto-repair turned off"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\FileSystem"), QStringLiteral("FilterSupportedFeaturesMode"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\FileSystem"), QStringLiteral("NtfsForceNonPagedPoolAllocation"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\FileSystem"), QStringLiteral("NtfsParallelFlushThreshold"), 4u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\FileSystem"), QStringLiteral("NtfsParallelFlushWorkers"), 4u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\FileSystem"), QStringLiteral("SymlinkLocalToLocalEvaluation"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\FileSystem"), QStringLiteral("SymlinkLocalToRemoteEvaluation"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\FileSystem"), QStringLiteral("SymlinkRemoteToLocalEvaluation"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\FileSystem"), QStringLiteral("SymlinkRemoteToRemoteEvaluation"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\FileSystem"), QStringLiteral("DisableDeleteNotification"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\FileSystem"), QStringLiteral("LongPathsEnabled"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\FileSystem"), QStringLiteral("NtfsDisableSpotCorruptionHandling"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\FileSystem"), QStringLiteral("NtfsMftZoneReservation"), 4u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\FileSystem"), QStringLiteral("RefsDisableLastAccessUpdate"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\FileSystem"), QStringLiteral("ContigFileAllocSize"), 5430u, 64u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\FileSystem"), QStringLiteral("FileNameCache"), 32768u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\FileSystem"), QStringLiteral("PathCache"), 1024u, 256u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\FileSystem"), QStringLiteral("DontVerifyRandomDrivers"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), QStringLiteral("CopyBufferSize"), 1048576u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Disk"), QStringLiteral("DisableWriteCache"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\PartMgr"), QStringLiteral("EnableWriteCache"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.devices.usb_polling");
        t.category = QStringLiteral("storage");
        t.name = QStringLiteral("USB storage minimum polling interval");
        t.description = QStringLiteral("Sets the USB storage polling interval to 1ms for fastest device response time.");
        t.iconName = QStringLiteral("usb");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Slight increase in CPU usage from more frequent USB polling"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\usbstor"), QStringLiteral("PollingInterval"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.dev.powershell_policy");
        t.category = QStringLiteral("advanced");
        t.name = QStringLiteral("PowerShell unrestricted execution policy");
        t.description = QStringLiteral("Sets PowerShell to unrestricted, enables scripts, disables module/script block logging and transcription.");
        t.iconName = QStringLiteral("code");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Unsigned scripts execute without prompt"),
            QStringLiteral("Script execution auditing disabled"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\PowerShell"), QStringLiteral("EnableScripts"), 1u, 0u),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\PowerShell"), QStringLiteral("ExecutionPolicy"), QStringLiteral("Unrestricted"), QStringLiteral("RemoteSigned")),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\PowerShell\\ModuleLogging"), QStringLiteral("EnableModuleLogging"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\PowerShell\\ScriptBlockLogging"), QStringLiteral("EnableScriptBlockLogging"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\PowerShell\\Transcription"), QStringLiteral("EnableTranscripting"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Explorer"), QStringLiteral("ShowRunAsDifferentUserInStart"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.diag.crash_logging");
        t.category = QStringLiteral("advanced");
        t.name = QStringLiteral("Crash control logging and no overwrite");
        t.description = QStringLiteral("Enables crash event logging (LogEvent=1), keeps existing minidumps instead of overwriting them (Overwrite=0), and sets CrashDumpEnabled=3 (small memory dump).");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        // HKLM\SYSTEM is not writable by a standard user. Without this the tweak
        // is offered without elevation and every op fails at the first write.
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Disk space usage from accumulated minidumps"),
        };
        // HKLM, since CurrentControlSet only exists there. Revert values are the
        // Windows defaults: LogEvent 1, Overwrite 1, CrashDumpEnabled 7 (automatic
        // memory dump). DisableEmoticon and AlwaysKeepMemoryDump are absent on a clean
        // install, so revert removes them.
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\CrashControl"), QStringLiteral("LogEvent"), 1u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\CrashControl"), QStringLiteral("Overwrite"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\CrashControl"), QStringLiteral("DisableEmoticon"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\CrashControl"), QStringLiteral("CrashDumpEnabled"), 3u, 7u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\CrashControl"), QStringLiteral("AlwaysKeepMemoryDump"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.dxgi.advanced");
        t.category = QStringLiteral("gpu");
        t.name = QStringLiteral("DXGI advanced settings");
        t.description = QStringLiteral("Forces DXGI adapter enumeration and sets DWM overlay test mode for better GPU utilization.");
        t.iconName = QStringLiteral("monitor");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Uses a DWM overlay test mode that future Windows or GPU driver updates could change"),
            QStringLiteral("A few games or capture tools may render black or flicker when overlays are used more aggressively"),
            QStringLiteral("If you see new display glitches, reverting this tweak is the first thing to try"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("ForceAdapterEnumeration"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\DWM"), QStringLiteral("OverlayTestMode"), 5u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\DirectX"), QStringLiteral("ForceFlipDiscard"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("TdrDelay"), 60u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Scheduler"), QStringLiteral("EnablePreemption"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.gpu.extended_optimize");
        t.category = QStringLiteral("gpu");
        t.name = QStringLiteral("Extended GPU driver optimization");
        t.description = QStringLiteral("Enables GPU timer, disables power-saving, enables DPI IOMMU contiguous mapping, enables HW GPU scheduling, disables runtime power management, and sets zero latency for FlTransition/D3PC.");
        t.iconName = QStringLiteral("monitor");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("GPU timeout detection disabled"),
            QStringLiteral("Higher GPU power consumption"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\DirectX\\GraphicsSettings"), QStringLiteral("SwapEffectUpgradeCache"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("TdrLevel"), 0u, 3u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("UseGpuTimer"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("PowerSavingTweaks"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("EnableRuntimePowerManagement"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("FlTransitionLatency"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("D3PCLatency"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("RMDeepLlEntryLatencyUsec"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("PciLatencyTimerControl"), 32u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("LOWLATENCY"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("DpiMapIommuContiguous"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("HwSchMode"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers"), QStringLiteral("PlatformSupportMiracast"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.kernel.dpc_optimize");
        t.category = QStringLiteral("advanced");
        t.name = QStringLiteral("Kernel DPC and session manager optimization");
        t.description = QStringLiteral("Optimizes kernel DPC settings: zeroes DPC watchdog, enables timer distribution, disables auto-boost. Optimizes session manager: disables MCA/MCE, page combining, increases heap decommit threshold and I/O page lock limit.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Kernel watchdog thresholds reduced"),
            QStringLiteral("Auto-boost disabled"),
            QStringLiteral("MCA/MCE hardware error reporting disabled"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("DpcWatchdogProfileOffset"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("DpcTimeout"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("DpcWatchdogPeriod"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("DisableExceptionChainValidation"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("DisableAutoBoost"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("DistributeTimers"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel"), QStringLiteral("ThreadDpcEnable"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager"), QStringLiteral("EnableMCA"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager"), QStringLiteral("EnableMCE"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager"), QStringLiteral("HeapDeCommitFreeBlockThreshold"), 262144u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"), QStringLiteral("DisablePageCombining"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"), QStringLiteral("IoPageLockLimit"), 134217728u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.recovery.disable_winre");
        t.category = QStringLiteral("advanced");
        t.name = QStringLiteral("Disable Windows recovery environment setup");
        t.description = QStringLiteral("Disables WinRE via group policy. Recovery must use external media.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("WinRE unavailable for automatic repair"),
            QStringLiteral("Must use external recovery media"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\WinRE"), QStringLiteral("DisableSetup"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.security.disable_bitlocker");
        t.category = QStringLiteral("security");
        t.name = QStringLiteral("Disable BitLocker and device encryption");
        t.description = QStringLiteral("Prevents automatic device encryption and disables BitLocker on fixed drives, removable drives, and OS volumes.");
        t.iconName = QStringLiteral("shield");
        t.impact = Impact::Critical;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("All disk encryption disabled"),
            QStringLiteral("Data unprotected if device is lost"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\FVE"), QStringLiteral("RDVAllowBDE"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\FVE"), QStringLiteral("RDVEnforceBDE"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\FVE"), QStringLiteral("RDVEnableBDE"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\FVE"), QStringLiteral("EnableBDE"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\FVE"), QStringLiteral("EnableBDEWithNoTPM"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\FVE"), QStringLiteral("UseAdvancedStartup"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\FVE"), QStringLiteral("PreventDeviceEncryption"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\BitLocker"), QStringLiteral("PreventDeviceEncryption"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\FVE"), QStringLiteral("DisableExternalDMAUnderLock"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.virt.disable_vbs");
        t.category = QStringLiteral("advanced");
        t.name = QStringLiteral("Disable virtualization-based security");
        t.description = QStringLiteral("Disables VBS, Device Guard, Credential Guard, and HVCI at the registry level across all policy locations.");
        t.iconName = QStringLiteral("shield");
        t.impact = Impact::Critical;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("VBS/HVCI protection disabled"),
            QStringLiteral("Credential Guard disabled"),
            QStringLiteral("Kernel memory integrity protection removed"),
            QStringLiteral("vanguard"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\DeviceGuard"), QStringLiteral("EnableVirtualizationBasedSecurity"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\DeviceGuard"), QStringLiteral("RequirePlatformSecurityFeature"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\DeviceGuard"), QStringLiteral("RequireMicrosoftSignedBootChain"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\DeviceGuard"), QStringLiteral("HypervisorEnforcedCodeIntegrity"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\DeviceGuard\\Scenarios\\CredentialGuard"), QStringLiteral("Enabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\DeviceGuard\\Scenarios\\HypervisorEnforcedCodeIntegrity"), QStringLiteral("Enabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\DeviceGuard\\EnableVirtualizationBasedSecurity"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\DeviceGuard\\RequirePlatformSecurityFeatures"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\VirtualizationBasedTechnology\\HypervisorEnforcedCodeIntegrity"), QStringLiteral("value"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.wu.disable_secureboot_update");
        t.category = QStringLiteral("advanced");
        t.name = QStringLiteral("Disable Secure Boot updates");
        t.description = QStringLiteral("Prevents Secure Boot updates and opts out of SBAT revocation.");
        t.iconName = QStringLiteral("refresh-cw");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Vulnerable bootloaders remain trusted"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\SecureBoot"), QStringLiteral("AvailableUpdates"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\SecureBoot\\SBAT"), QStringLiteral("OptOut"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.bcd.apic_interrupts");
        t.category = QStringLiteral("advanced");
        t.declaredTier = vth::models::ProductTier::Ultimate;
        t.name = QStringLiteral("BCD APIC and interrupt configuration");
        t.description = QStringLiteral("Forces x2APIC mode, resets interrupt access policies, and ignores firmware-supplied PCI routing.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        CommandOp bo0;
        bo0.program = QStringLiteral("bcdedit");
        bo0.applyArgs = { QStringLiteral("/set"), QStringLiteral("x2apicpolicy"), QStringLiteral("Enable") };
        bo0.revertArgs = { QStringLiteral("/deletevalue"), QStringLiteral("x2apicpolicy") };
        t.commandOps.push_back(bo0);
        CommandOp bo1;
        bo1.program = QStringLiteral("bcdedit");
        bo1.applyArgs = { QStringLiteral("/set"), QStringLiteral("configaccesspolicy"), QStringLiteral("Default") };
        bo1.revertArgs = { QStringLiteral("/deletevalue"), QStringLiteral("configaccesspolicy") };
        t.commandOps.push_back(bo1);
        CommandOp bo2;
        bo2.program = QStringLiteral("bcdedit");
        bo2.applyArgs = { QStringLiteral("/set"), QStringLiteral("MSI"), QStringLiteral("Default") };
        bo2.revertArgs = { QStringLiteral("/deletevalue"), QStringLiteral("MSI") };
        t.commandOps.push_back(bo2);
        CommandOp bo3;
        bo3.program = QStringLiteral("bcdedit");
        bo3.applyArgs = { QStringLiteral("/set"), QStringLiteral("usephysicaldestination"), QStringLiteral("No") };
        bo3.revertArgs = { QStringLiteral("/deletevalue"), QStringLiteral("usephysicaldestination") };
        t.commandOps.push_back(bo3);
        CommandOp bo4;
        bo4.program = QStringLiteral("bcdedit");
        bo4.applyArgs = { QStringLiteral("/set"), QStringLiteral("usefirmwarepcisettings"), QStringLiteral("No") };
        bo4.revertArgs = { QStringLiteral("/deletevalue"), QStringLiteral("usefirmwarepcisettings") };
        t.commandOps.push_back(bo4);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.bcd.boot_behavior");
        t.category = QStringLiteral("advanced");
        t.declaredTier = vth::models::ProductTier::Ultimate;
        t.name = QStringLiteral("Optimize boot behavior");
        t.description = QStringLiteral("Silences the boot UI, uses the legacy boot menu, drops the timeout to zero, and disables automatic recovery.");
        t.iconName = QStringLiteral("zap");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        CommandOp bo0;
        bo0.program = QStringLiteral("bcdedit");
        bo0.applyArgs = { QStringLiteral("/set"), QStringLiteral("quietboot"), QStringLiteral("yes") };
        bo0.revertArgs = { QStringLiteral("/deletevalue"), QStringLiteral("quietboot") };
        t.commandOps.push_back(bo0);
        CommandOp bo1;
        bo1.program = QStringLiteral("bcdedit");
        bo1.applyArgs = { QStringLiteral("/set"), QStringLiteral("bootmenupolicy"), QStringLiteral("Legacy") };
        bo1.revertArgs = { QStringLiteral("/deletevalue"), QStringLiteral("bootmenupolicy") };
        t.commandOps.push_back(bo1);
        CommandOp bo2;
        bo2.program = QStringLiteral("bcdedit");
        bo2.applyArgs = { QStringLiteral("/set"), QStringLiteral("timeout"), QStringLiteral("0") };
        bo2.revertArgs = { QStringLiteral("/deletevalue"), QStringLiteral("timeout") };
        t.commandOps.push_back(bo2);
        CommandOp bo3;
        bo3.program = QStringLiteral("bcdedit");
        bo3.applyArgs = { QStringLiteral("/set"), QStringLiteral("recoveryenabled"), QStringLiteral("No") };
        bo3.revertArgs = { QStringLiteral("/deletevalue"), QStringLiteral("recoveryenabled") };
        t.commandOps.push_back(bo3);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.bcd.debug_telemetry");
        t.category = QStringLiteral("advanced");
        t.declaredTier = vth::models::ProductTier::Ultimate;
        t.name = QStringLiteral("Disable boot debug and telemetry");
        t.description = QStringLiteral("Disables kernel debugger attach, Emergency Management Services, and TPM boot-entropy collection.");
        t.iconName = QStringLiteral("shield");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        CommandOp bo0;
        bo0.program = QStringLiteral("bcdedit");
        bo0.applyArgs = { QStringLiteral("/set"), QStringLiteral("debug"), QStringLiteral("No") };
        bo0.revertArgs = { QStringLiteral("/deletevalue"), QStringLiteral("debug") };
        t.commandOps.push_back(bo0);
        CommandOp bo1;
        bo1.program = QStringLiteral("bcdedit");
        bo1.applyArgs = { QStringLiteral("/set"), QStringLiteral("ems"), QStringLiteral("No") };
        bo1.revertArgs = { QStringLiteral("/deletevalue"), QStringLiteral("ems") };
        t.commandOps.push_back(bo1);
        CommandOp bo2;
        bo2.program = QStringLiteral("bcdedit");
        bo2.applyArgs = { QStringLiteral("/set"), QStringLiteral("tpmbootentropy"), QStringLiteral("ForceDisable") };
        bo2.revertArgs = { QStringLiteral("/deletevalue"), QStringLiteral("tpmbootentropy") };
        t.commandOps.push_back(bo2);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.bcd.dep_optout");
        t.category = QStringLiteral("advanced");
        t.declaredTier = vth::models::ProductTier::Ultimate;
        t.name = QStringLiteral("Set DEP to OptOut mode");
        t.description = QStringLiteral("Switches Data Execution Prevention from AlwaysOn to OptOut so individual apps can disable it. This is a security trade-off, not a performance optimization.");
        t.iconName = QStringLiteral("shield");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Lets programs opt out of Data Execution Prevention, weakening exploit protection. No measurable performance benefit on modern CPUs."),
        };
        t.concern = QStringLiteral("DEP weakening; opt-in only, never recommended.");
        CommandOp bo0;
        bo0.program = QStringLiteral("bcdedit");
        bo0.applyArgs = { QStringLiteral("/set"), QStringLiteral("nx"), QStringLiteral("OptOut") };
        bo0.revertArgs = { QStringLiteral("/deletevalue"), QStringLiteral("nx") };
        t.commandOps.push_back(bo0);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.bcd.disable_virt");
        t.category = QStringLiteral("advanced");
        t.declaredTier = vth::models::ProductTier::Ultimate;
        t.name = QStringLiteral("Disable virtualization and security overhead (HVCI / VBS off)");
        t.description = QStringLiteral("Turns off the Hyper-V hypervisor, Virtual Secure Mode, VM extensions, and isolated user contexts. Can be a real gain on Win11 22H2+ - but it disables HVCI/memory integrity and Virtualization-Based Security, and anti-cheats detect the change.");
        t.iconName = QStringLiteral("shield");
        t.impact = Impact::Critical;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        // Critical and needs a reboot. Revert deletes the values, which restores the
        // default (Automatic).
        t.warnings = {
            QStringLiteral("Disables HVCI / memory integrity and Virtualization-Based Security - a real reduction in kernel exploit protection."),
            QStringLiteral("Anti-cheats (Vanguard, EAC, BattlEye) detect hypervisor-off and can BLOCK the game or ban the account. Check every game you play before enabling."),
            QStringLiteral("Breaks WSL2, Windows Sandbox, Hyper-V VMs, and Credential Guard until reverted and rebooted."),
        };
        t.concern = QStringLiteral("highest-risk gate: anti-cheat lockout + HVCI/VBS loss. Never default-on; per-game anti-cheat warning is mandatory.");
        CommandOp bo0;
        bo0.program = QStringLiteral("bcdedit");
        bo0.applyArgs = { QStringLiteral("/set"), QStringLiteral("hypervisorlaunchtype"), QStringLiteral("off") };
        bo0.revertArgs = { QStringLiteral("/deletevalue"), QStringLiteral("hypervisorlaunchtype") };
        t.commandOps.push_back(bo0);
        CommandOp bo1;
        bo1.program = QStringLiteral("bcdedit");
        bo1.applyArgs = { QStringLiteral("/set"), QStringLiteral("vsmlaunchtype"), QStringLiteral("Off") };
        bo1.revertArgs = { QStringLiteral("/deletevalue"), QStringLiteral("vsmlaunchtype") };
        t.commandOps.push_back(bo1);
        CommandOp bo2;
        bo2.program = QStringLiteral("bcdedit");
        bo2.applyArgs = { QStringLiteral("/set"), QStringLiteral("vm"), QStringLiteral("No") };
        bo2.revertArgs = { QStringLiteral("/deletevalue"), QStringLiteral("vm") };
        t.commandOps.push_back(bo2);
        CommandOp bo3;
        bo3.program = QStringLiteral("bcdedit");
        bo3.applyArgs = { QStringLiteral("/set"), QStringLiteral("isolatedcontext"), QStringLiteral("No") };
        bo3.revertArgs = { QStringLiteral("/deletevalue"), QStringLiteral("isolatedcontext") };
        t.commandOps.push_back(bo3);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.bcd.extended");
        t.category = QStringLiteral("advanced");
        t.declaredTier = vth::models::ProductTier::Ultimate;
        t.name = QStringLiteral("Extended BCD boot configuration");
        t.description = QStringLiteral("Thirteen-element BCD bundle: extended input, no FIPS enforcement, SOS, PAE, ELAM off, highest display mode, no boot UX animation.");
        t.iconName = QStringLiteral("zap");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        CommandOp bo0;
        bo0.program = QStringLiteral("bcdedit");
        bo0.applyArgs = { QStringLiteral("/set"), QStringLiteral("extendedinput"), QStringLiteral("Yes") };
        bo0.revertArgs = { QStringLiteral("/deletevalue"), QStringLiteral("extendedinput") };
        t.commandOps.push_back(bo0);
        CommandOp bo1;
        bo1.program = QStringLiteral("bcdedit");
        bo1.applyArgs = { QStringLiteral("/set"), QStringLiteral("forcefipscrypto"), QStringLiteral("No") };
        bo1.revertArgs = { QStringLiteral("/deletevalue"), QStringLiteral("forcefipscrypto") };
        t.commandOps.push_back(bo1);
        CommandOp bo2;
        bo2.program = QStringLiteral("bcdedit");
        bo2.applyArgs = { QStringLiteral("/set"), QStringLiteral("halbreakpoint"), QStringLiteral("No") };
        bo2.revertArgs = { QStringLiteral("/deletevalue"), QStringLiteral("halbreakpoint") };
        t.commandOps.push_back(bo2);
        CommandOp bo3;
        bo3.program = QStringLiteral("bcdedit");
        bo3.applyArgs = { QStringLiteral("/set"), QStringLiteral("graphicsmodedisabled"), QStringLiteral("No") };
        bo3.revertArgs = { QStringLiteral("/deletevalue"), QStringLiteral("graphicsmodedisabled") };
        t.commandOps.push_back(bo3);
        CommandOp bo4;
        bo4.program = QStringLiteral("bcdedit");
        bo4.applyArgs = { QStringLiteral("/set"), QStringLiteral("noumex"), QStringLiteral("Yes") };
        bo4.revertArgs = { QStringLiteral("/deletevalue"), QStringLiteral("noumex") };
        t.commandOps.push_back(bo4);
        CommandOp bo5;
        bo5.program = QStringLiteral("bcdedit");
        bo5.applyArgs = { QStringLiteral("/set"), QStringLiteral("sos"), QStringLiteral("Yes") };
        bo5.revertArgs = { QStringLiteral("/deletevalue"), QStringLiteral("sos") };
        t.commandOps.push_back(bo5);
        CommandOp bo6;
        bo6.program = QStringLiteral("bcdedit");
        bo6.applyArgs = { QStringLiteral("/set"), QStringLiteral("onecpu"), QStringLiteral("No") };
        bo6.revertArgs = { QStringLiteral("/deletevalue"), QStringLiteral("onecpu") };
        t.commandOps.push_back(bo6);
        CommandOp bo7;
        bo7.program = QStringLiteral("bcdedit");
        bo7.applyArgs = { QStringLiteral("/set"), QStringLiteral("pae"), QStringLiteral("ForceEnable") };
        bo7.revertArgs = { QStringLiteral("/deletevalue"), QStringLiteral("pae") };
        t.commandOps.push_back(bo7);
        CommandOp bo8;
        bo8.program = QStringLiteral("bcdedit");
        bo8.applyArgs = { QStringLiteral("/set"), QStringLiteral("disableelamdrivers"), QStringLiteral("Yes") };
        bo8.revertArgs = { QStringLiteral("/deletevalue"), QStringLiteral("disableelamdrivers") };
        t.commandOps.push_back(bo8);
        CommandOp bo9;
        bo9.program = QStringLiteral("bcdedit");
        bo9.applyArgs = { QStringLiteral("/set"), QStringLiteral("highestmode"), QStringLiteral("Yes") };
        bo9.revertArgs = { QStringLiteral("/deletevalue"), QStringLiteral("highestmode") };
        t.commandOps.push_back(bo9);
        CommandOp bo10;
        bo10.program = QStringLiteral("bcdedit");
        bo10.applyArgs = { QStringLiteral("/set"), QStringLiteral("uselegacyapicmode"), QStringLiteral("No") };
        bo10.revertArgs = { QStringLiteral("/deletevalue"), QStringLiteral("uselegacyapicmode") };
        t.commandOps.push_back(bo10);
        CommandOp bo11;
        bo11.program = QStringLiteral("bcdedit");
        bo11.applyArgs = { QStringLiteral("/set"), QStringLiteral("allowedinmemorysettings"), QStringLiteral("0") };
        bo11.revertArgs = { QStringLiteral("/deletevalue"), QStringLiteral("allowedinmemorysettings") };
        t.commandOps.push_back(bo11);
        CommandOp bo12;
        bo12.program = QStringLiteral("bcdedit");
        bo12.applyArgs = { QStringLiteral("/set"), QStringLiteral("bootux"), QStringLiteral("disabled") };
        bo12.revertArgs = { QStringLiteral("/deletevalue"), QStringLiteral("bootux") };
        t.commandOps.push_back(bo12);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.bcd.memory_layout");
        t.category = QStringLiteral("advanced");
        t.declaredTier = vth::models::ProductTier::Ultimate;
        t.name = QStringLiteral("BCD memory layout optimization");
        t.description = QStringLiteral("Reshapes kernel memory layout: opts out of 5-level paging, raises the user VA ceiling, and changes low-memory use.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        CommandOp bo0;
        bo0.program = QStringLiteral("bcdedit");
        bo0.applyArgs = { QStringLiteral("/set"), QStringLiteral("linearaddress57"), QStringLiteral("OptOut") };
        bo0.revertArgs = { QStringLiteral("/deletevalue"), QStringLiteral("linearaddress57") };
        t.commandOps.push_back(bo0);
        CommandOp bo1;
        bo1.program = QStringLiteral("bcdedit");
        bo1.applyArgs = { QStringLiteral("/set"), QStringLiteral("increaseuserva"), QStringLiteral("268435328") };
        bo1.revertArgs = { QStringLiteral("/deletevalue"), QStringLiteral("increaseuserva") };
        t.commandOps.push_back(bo1);
        CommandOp bo2;
        bo2.program = QStringLiteral("bcdedit");
        bo2.applyArgs = { QStringLiteral("/set"), QStringLiteral("firstmegabytepolicy"), QStringLiteral("UseAll") };
        bo2.revertArgs = { QStringLiteral("/deletevalue"), QStringLiteral("firstmegabytepolicy") };
        t.commandOps.push_back(bo2);
        CommandOp bo3;
        bo3.program = QStringLiteral("bcdedit");
        bo3.applyArgs = { QStringLiteral("/set"), QStringLiteral("avoidlowmemory"), QStringLiteral("0x8000000") };
        bo3.revertArgs = { QStringLiteral("/deletevalue"), QStringLiteral("avoidlowmemory") };
        t.commandOps.push_back(bo3);
        CommandOp bo4;
        bo4.program = QStringLiteral("bcdedit");
        bo4.applyArgs = { QStringLiteral("/set"), QStringLiteral("nolowmem"), QStringLiteral("yes") };
        bo4.revertArgs = { QStringLiteral("/deletevalue"), QStringLiteral("nolowmem") };
        t.commandOps.push_back(bo4);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("system.bcd-no-bootlog");
        t.category = QStringLiteral("advanced");
        t.declaredTier = vth::models::ProductTier::Ultimate;
        t.name = QStringLiteral("Disable boot logging");
        t.description = QStringLiteral("Sets bcdedit bootlog No. Stops the boot loader from writing Ntbtlog.txt.");
        t.iconName = QStringLiteral("file-text");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        CommandOp bo0;
        bo0.program = QStringLiteral("bcdedit");
        bo0.applyArgs = { QStringLiteral("/set"), QStringLiteral("bootlog"), QStringLiteral("No") };
        bo0.revertArgs = { QStringLiteral("/deletevalue"), QStringLiteral("bootlog") };
        t.commandOps.push_back(bo0);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("sys.disable-system-restore");
        t.category = QStringLiteral("debloating");
        t.name = QStringLiteral("Disable System Restore");
        t.description = QStringLiteral("Removes the System Protection schedule and frees the disk used by restore points.");
        t.iconName = QStringLiteral("history");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Removes Vethida's pre-apply safety net - without it, reverting a bad tweak depends on backups you've made elsewhere."),
            QStringLiteral("All existing restore points are deleted."),
        };
        CommandOp po;
        po.program = QStringLiteral("powershell");
        po.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("Disable-ComputerRestore -Drive C:\\ -ErrorAction SilentlyContinue") };
        po.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"), QStringLiteral("-Command"), QStringLiteral("Enable-ComputerRestore -Drive C:\\ -ErrorAction SilentlyContinue") };
        t.commandOps.push_back(po);
        c.push_back(t);
    }
}

} // namespace vth::tweaks::detail
