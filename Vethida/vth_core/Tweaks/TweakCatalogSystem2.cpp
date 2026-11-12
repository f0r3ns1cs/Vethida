#include "TweakEngine.h"

namespace vth::tweaks::detail {

    namespace {
        void addD(QVector<RegistryOp>& o, const QString& p, const QString& n, quint32 v,
                std::optional<quint32> rev = std::nullopt) {
            o.push_back(dwordOp(p, n, v, rev));
        }
        CommandOp cmd(const QString& program, const QStringList& applyArgs, const QStringList& revertArgs) {
            CommandOp op; op.program = program; op.applyArgs = applyArgs; op.revertArgs = revertArgs;
            return op;
        }
        const QString kKernel = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel");
        const QString kIfeo = QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\");
    }

    void appendSystem2(QVector<TweakSpec>& c) {
        {
            TweakSpec t;
            t.id = QStringLiteral("storage.idle-power-off");
            t.category = QStringLiteral("storage");
            t.subCategory = QStringLiteral("Power management");
            t.name = QStringLiteral("Disable storage idle power management");
            t.description = QStringLiteral(
                "Disables AHCI D3-cold idle and Device Sleep (DEVSLP) on the SATA driver and "
                "deepens the NVMe queue, removing storage wake-up latency.");
            t.iconName = QStringLiteral("database");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral("desktop-oriented; raises SSD idle power on laptops.");
            QVector<RegistryOp> o;
            const QString ahci = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\storahci\\Parameters\\Device");
            const QString nvme = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\stornvme\\Parameters\\Device");
            addD(o, ahci, QStringLiteral("NoIdleD3"), 1, 0);
            addD(o, ahci, QStringLiteral("EnableDEVSLP"), 0, 1);
            addD(o, nvme, QStringLiteral("QueueDepth"), 64);
            t.registryOps = o;
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("storage.ntfs-extra");
            t.category = QStringLiteral("storage");
            t.subCategory = QStringLiteral("NTFS");
            t.name = QStringLiteral("NTFS filesystem values");
            t.description = QStringLiteral(
                "Disables NTFS tunneling (the 8.3 name cache), filesystem-level compression and "
                "the Encrypting File System, trimming metadata overhead.");
            t.iconName = QStringLiteral("database");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "NtfsEncryptionService=0 disables EFS - only safe if you don't use Encrypting "
                "File System (BitLocker is unaffected).");
            const QString fs = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\FileSystem");
            QVector<RegistryOp> o;
            addD(o, fs, QStringLiteral("MaximumTunnelEntries"), 0);
            addD(o, fs, QStringLiteral("NtfsDisableCompression"), 1, 0);
            addD(o, fs, QStringLiteral("NtfsEncryptionService"), 0, 1);
            t.registryOps = o;
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("storage.mft-zone");
            t.category = QStringLiteral("storage");
            t.subCategory = QStringLiteral("NTFS");
            t.name = QStringLiteral("Reserve 50% MFT zone");
            t.description = QStringLiteral("Sets the NTFS MFT zone reservation to its largest tier (mftzone 4, ~50%) to limit MFT fragmentation on volumes with a huge number of tiny files. Microsoft notes the change only takes full effect on a newly formatted volume, and reserving that much zone offers no benefit for typical mixed-file drives.");
            t.iconName = QStringLiteral("database");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.commandOps = {
                cmd(QStringLiteral("fsutil"),
                    { QStringLiteral("behavior"), QStringLiteral("set"), QStringLiteral("mftzone"), QStringLiteral("4") },
                    { QStringLiteral("behavior"), QStringLiteral("set"), QStringLiteral("mftzone"), QStringLiteral("1") }),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("net.tcpip-stack-tuning");
            t.category = QStringLiteral("networking");
            t.subCategory = QStringLiteral("TCP/IP");
            t.name = QStringLiteral("TCP/IP stack values");
            t.description = QStringLiteral(
                "Sets a TTL of 64, widens the ephemeral port range and shortens TIME_WAIT so "
                "short-lived connections recycle faster.");
            t.iconName = QStringLiteral("network");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            const QString tcp = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters");
            t.registryOps = {
                dwordOp(tcp, QStringLiteral("DefaultTTL"), 64, std::nullopt),
                dwordOp(tcp, QStringLiteral("MaxUserPort"), 65534, std::nullopt),
                dwordOp(tcp, QStringLiteral("TcpTimedWaitDelay"), 30, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("net.disable-nic-bindings");
            t.category = QStringLiteral("networking");
            t.subCategory = QStringLiteral("Adapter bindings");
            t.name = QStringLiteral("Disable unneeded NIC bindings");
            t.description = QStringLiteral(
                "Unbinds LLDP, link-layer topology (LLTD), the MS network client/server "
                "(file & printer sharing) and the multiplexor from every adapter, shrinking the "
                "network stack's per-packet path and attack surface.");
            t.iconName = QStringLiteral("network");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "leaves the QoS packet scheduler (ms_pacer) and IPv6 (ms_tcpip6) bound on "
                "purpose: the former carries the DSCP game-traffic marking, the latter breaks "
                "modern services if removed. Disables file/printer sharing - skip if you share "
                "folders on a LAN.");
            t.commandOps = {
                cmd(QStringLiteral("powershell"),
                    { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"),
                    QStringLiteral("-Command"),
                    QStringLiteral("$ErrorActionPreference='SilentlyContinue';"
                    "'ms_lldp','ms_lltdio','ms_rspndr','ms_implat','ms_server','ms_msclient'|"
                    "%{Set-NetAdapterBinding -Name '*' -ComponentID $_ -Disabled}") },
                    { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"),
                    QStringLiteral("-Command"),
                    QStringLiteral("$ErrorActionPreference='SilentlyContinue';"
                    "'ms_lldp','ms_lltdio','ms_rspndr','ms_implat','ms_server','ms_msclient'|"
                    "%{Set-NetAdapterBinding -Name '*' -ComponentID $_ -Enabled}") }),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("appear.disable-lock-screen");
            t.category = QStringLiteral("appearance");
            t.subCategory = QStringLiteral("Lock screen");
            t.name = QStringLiteral("Disable lock screen");
            t.description = QStringLiteral("Skips the lock-screen wallpaper and goes straight to sign-in.");
            t.iconName = QStringLiteral("lock-open");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Personalization"),
                        QStringLiteral("NoLockScreen"), 1, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("appear.numlock-on-boot");
            t.category = QStringLiteral("general");
            t.subCategory = QStringLiteral("Keyboard");
            t.name = QStringLiteral("NumLock on at boot");
            t.description = QStringLiteral("Enables NumLock at the sign-in screen and on every boot.");
            t.iconName = QStringLiteral("keyboard");
            t.impact = Impact::Low;
            t.registryOps = {
                stringOp(QStringLiteral("HKCU\\Control Panel\\Keyboard"),
                        QStringLiteral("InitialKeyboardIndicators"),
                        QStringLiteral("2147483650"), QStringLiteral("2147483648")),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("appear.verbose-status");
            t.category = QStringLiteral("appearance");
            t.subCategory = QStringLiteral("Boot messages");
            t.name = QStringLiteral("Verbose Startup/Shutdown messages");
            t.description = QStringLiteral(
                "Shows detailed status messages during boot and shutdown instead of the generic "
                "spinner - useful for spotting what's slow.");
            t.iconName = QStringLiteral("scroll-text");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"),
                        QStringLiteral("VerboseStatus"), 1, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("appear.disable-nags");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Nags & suggestions");
            t.name = QStringLiteral("Disable setup nags and online tips");
            t.description = QStringLiteral(
                "Stops the \"Finish setting up your device\" full-screen nag and online tips/"
                "suggestions in Settings.");
            t.iconName = QStringLiteral("bell-off");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"),
                        QStringLiteral("AllowOnlineTips"), 0, std::nullopt),
                dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\UserProfileEngagement"),
                        QStringLiteral("ScoobeSystemSettingEnabled"), 0, 1),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("appear.focus-assist");
            t.category = QStringLiteral("general");
            t.subCategory = QStringLiteral("Notifications");
            t.name = QStringLiteral("Enable do not disturb");
            t.description = QStringLiteral(
                "Turns on Focus Assist / Do Not Disturb so toasts are suppressed system-wide.");
            t.iconName = QStringLiteral("bell-off");
            t.impact = Impact::Low;
            t.registryOps = {
                dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Notifications\\Settings"),
                        QStringLiteral("NOC_GLOBAL_SETTING_DND"), 1, 0),
                dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\FocusAssist"),
                        QStringLiteral("QuietHoursActive"), 1, 0),
                dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\FocusAssist"),
                        QStringLiteral("QuietHoursEnabled"), 1, 0),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("debloat.disable-gamebar");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Xbox & Game Bar");
            t.name = QStringLiteral("Disable Xbox Game Bar");
            t.description = QStringLiteral(
                "Turns off the Game Bar overlay, its startup panel/tip, the Nexus launcher and "
                "auto Game Mode.");
            t.iconName = QStringLiteral("gamepad-2");
            t.impact = Impact::Low;
            const QString gb = QStringLiteral("HKCU\\Software\\Microsoft\\GameBar");
            t.registryOps = {
                dwordOp(gb, QStringLiteral("AllowAutoGameMode"), 0, 1),
                dwordOp(gb, QStringLiteral("ShowStartupPanel"), 0, 1),
                dwordOp(gb, QStringLiteral("UseNexusForGameBarEnabled"), 0, 1),
                dwordOp(gb, QStringLiteral("GamePanelStartupTipIndex"), 3, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("advanced.bluetooth-wake-off");
            t.category = QStringLiteral("power");
            t.subCategory = QStringLiteral("Bluetooth");
            t.name = QStringLiteral("Disable Bluetooth wake and SCO");
            t.description = QStringLiteral(
                "Stops Bluetooth devices from waking the PC and disables SCO call-audio "
                "negotiation.");
            t.iconName = QStringLiteral("bluetooth");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.concern = QStringLiteral("situational; skip if you wake the PC with a Bluetooth device or use BT headset calls.");
            const QString bt = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\BthPort\\Parameters");
            t.registryOps = {
                dwordOp(bt, QStringLiteral("SystemRemoteWakeSupported"), 0, 1),
                dwordOp(bt, QStringLiteral("RemoteWakeEnabled"), 0, 1),
                dwordOp(bt, QStringLiteral("ScoEnabled"), 0, 1),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("power.sleep-timers-off");
            t.category = QStringLiteral("power");
            t.subCategory = QStringLiteral("Sleep");
            t.name = QStringLiteral("Disable sleep and standby timers");
            t.description = QStringLiteral(
                "Sets the standby and hibernate idle timeouts to never on both AC and battery.");
            t.iconName = QStringLiteral("power");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.commandOps = {
                cmd(QStringLiteral("powercfg"), { QStringLiteral("/change"), QStringLiteral("standby-timeout-ac"), QStringLiteral("0") },
                                                { QStringLiteral("/change"), QStringLiteral("standby-timeout-ac"), QStringLiteral("30") }),
                cmd(QStringLiteral("powercfg"), { QStringLiteral("/change"), QStringLiteral("standby-timeout-dc"), QStringLiteral("0") },
                                                { QStringLiteral("/change"), QStringLiteral("standby-timeout-dc"), QStringLiteral("15") }),
                cmd(QStringLiteral("powercfg"), { QStringLiteral("/change"), QStringLiteral("hibernate-timeout-ac"), QStringLiteral("0") },
                                                { QStringLiteral("/change"), QStringLiteral("hibernate-timeout-ac"), QStringLiteral("180") }),
                cmd(QStringLiteral("powercfg"), { QStringLiteral("/change"), QStringLiteral("hibernate-timeout-dc"), QStringLiteral("0") },
                                                { QStringLiteral("/change"), QStringLiteral("hibernate-timeout-dc"), QStringLiteral("180") }),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("general.logon-shutdown-behavior");
            t.category = QStringLiteral("general");
            t.subCategory = QStringLiteral("Logon & shutdown");
            t.name = QStringLiteral("Logon/Shutdown behavior");
            t.description = QStringLiteral(
                "Reconnects mapped network drives without a UAC re-prompt, skips the "
                "post-update automatic-restart sign-on screen, removes the desktop-switch "
                "delay, and lets apps that don't respond be closed at shutdown without a "
                "confirmation dialog.");
            t.iconName = QStringLiteral("power");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "DisableCAD=1 removes the option to require Ctrl+Alt+Del before sign-in, "
                "which is a security posture change, not a performance one; review before "
                "applying on a shared or public machine.");
            const QString sysp = QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System");
            t.registryOps = {
                dwordOp(sysp, QStringLiteral("EnableLinkedConnections"), 0, std::nullopt),
                dwordOp(sysp, QStringLiteral("DisableAutomaticRestartSignOn"), 1, std::nullopt),
                dwordOp(sysp, QStringLiteral("DelayedDesktopSwitchTimeout"), 0, std::nullopt),
                dwordOp(sysp, QStringLiteral("DisableCAD"), 1, std::nullopt),
                dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System"),
                        QStringLiteral("AllowBlockingAppsAtShutdown"), 0, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("general.system-behavior-misc");
            t.category = QStringLiteral("general");
            t.subCategory = QStringLiteral("System behavior");
            t.name = QStringLiteral("System behavior miscellany");
            t.description = QStringLiteral(
                "Disables the scheduled maintenance window and driver-framework verbose "
                "logging, restricts Maps updates to Wi-Fi, defers Store auto-download, turns "
                "off File History, reduces System Restore checkpoint frequency, disables the "
                "Remote Assistance invite and web-printing, hides the Insider Program page, "
                "removes the tray-icon startup delay, allows write access previously blocked "
                "by a storage device policy, disables Software Protection ticket generation, "
                "and configures crash-dump behavior to a minimal kernel dump without "
                "auto-reboot.");
            t.iconName = QStringLiteral("settings");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "StorageDevicePolicies WriteProtect=0 and the CrashControl block change "
                "security/diagnostic posture, not just performance; PCHC/Software Protection "
                "Platform values are undocumented licensing-adjacent internals.");
            const QString crash = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\CrashControl");
            QVector<RegistryOp> o;
            o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate"),
                                QStringLiteral("TargetReleaseVersion"), 1, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Schedule\\Maintenance"),
                                QStringLiteral("MaintenanceDisabled"), 1, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\WUDF"),
                                QStringLiteral("LogLevel"), 0, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKLM\\SYSTEM\\Maps"),
                                QStringLiteral("UpdateOnlyOnWifi"), 1, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\WindowsStore"),
                                QStringLiteral("AutoDownload"), 2, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\FileHistory"),
                                QStringLiteral("Disabled"), 1, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Windows"),
                                QStringLiteral("DisplayVersion"), 1, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\SystemRestore"),
                                QStringLiteral("RPSessionInterval"), 0, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\SystemRestore"),
                                QStringLiteral("SystemRestorePointCreationFrequency"), 0, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Remote Assistance"),
                                QStringLiteral("fAllowToGetHelp"), 0, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search"),
                                QStringLiteral("PreventUsingAdvancedIndexingOptions"), 0, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\WindowsUpdate\\UX\\Settings"),
                                QStringLiteral("SmartActiveHoursState"), 2, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\WindowsUpdate\\UX\\Settings"),
                                QStringLiteral("UxOption"), 1, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\WindowsUpdate\\UX\\Settings"),
                                QStringLiteral("DeferUpgrade"), 1, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorageDevicePolicies"),
                                QStringLiteral("WriteProtect"), 0, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Serialize"),
                                QStringLiteral("StartupDelayInMSec"), 0, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows NT\\Printers"),
                                QStringLiteral("DisableWebPrinting"), 1, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\WindowsSelfHost\\UI\\Visibility"),
                                QStringLiteral("HideInsiderPage"), 1, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows NT\\CurrentVersion\\Software Protection Platform"),
                                QStringLiteral("NoGenTicket"), 1, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PCHC"),
                                QStringLiteral("PreviousUninstall"), 1, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\NetworkProvider"),
                                QStringLiteral("RestoreConnection"), 0, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Shell\\USB"),
                                QStringLiteral("NotifyOnWeakCharger"), 1, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Windows"),
                                QStringLiteral("LegacyDefaultPrinterMode"), 1, std::nullopt));
            o.push_back(dwordOp(crash, QStringLiteral("DisableEmoticon"), 1, std::nullopt));
            o.push_back(dwordOp(crash, QStringLiteral("DisplayParameters"), 1, std::nullopt));
            o.push_back(dwordOp(crash, QStringLiteral("CrashDumpEnabled"), 3, std::nullopt));
            o.push_back(dwordOp(crash, QStringLiteral("LogEvent"), 1, std::nullopt));
            o.push_back(dwordOp(crash, QStringLiteral("Overwrite"), 0, std::nullopt));
            o.push_back(dwordOp(crash, QStringLiteral("AlwaysKeepMemoryDump"), 0, std::nullopt));
            o.push_back(dwordOp(crash, QStringLiteral("AutoReboot"), 0, std::nullopt));
            t.registryOps = o;
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("debloat.onedrive-edge-shortcuts-off");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Background apps");
            t.name = QStringLiteral("Disable OneDrive background app and Edge shortcut default");
            t.description = QStringLiteral("Disables background activity for the OneDrive/immersive control panel, Edge, Windows Security UI and Store apps, and sets DisableSmartNameResolution=1 - a system-wide DNS change that turns off LLMNR/NetBIOS smart multi-homed fallback (a mild security hardening), not an Edge-only setting as the name implies.");
            t.iconName = QStringLiteral("cloud-off");
            t.impact = Impact::Low;
            t.requiresAdmin = false;
            t.registryOps = {
                dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\BackgroundAccessApplications\\"
                                       "windows.immersivecontrolpanel_cw5n1h2txyewy"),
                        QStringLiteral("DisabledByUser"), 1, std::nullopt),
                dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\BackgroundAccessApplications\\"
                                       "Microsoft.MicrosoftEdge_8wekyb3d8bbwe"),
                        QStringLiteral("DisabledByUser"), 1, std::nullopt),
                dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\BackgroundAccessApplications\\"
                                       "Microsoft.Windows.SecHealthUI_cw5n1h2txyewy"),
                        QStringLiteral("DisabledByUser"), 1, std::nullopt),
                dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\BackgroundAccessApplications\\"
                                       "Microsoft.WindowsStore_8wekyb3d8bbwe"),
                        QStringLiteral("DisabledByUser"), 1, std::nullopt),
                dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows NT\\DNSClient"),
                        QStringLiteral("DisableSmartNameResolution"), 1, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("kernel.dpc-latency-tuning");
            t.category = QStringLiteral("advanced");
            t.name = QStringLiteral("DPC and interrupt latency");
            t.description = QStringLiteral(
                "Tightens the kernel DPC path: minimum per-core DPC queue depth/rate, serialized "
                "timer expiration, split large caches, and a larger shared ready queue. Disables "
                "the DPC watchdog profiling that runs on every interrupt.");
            t.iconName = QStringLiteral("cpu");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "risky: DpcTimeout=0 and DpcWatchdogPeriod=0 disable the watchdog, which hides "
                "genuinely misbehaving drivers (no bugcheck on a stuck DPC). InterruptSteeringDisabled "
                "means the OS no longer load-balances interrupts - pair with MSI/affinity tweaks.");
            QVector<RegistryOp> o;
            addD(o, kKernel, QStringLiteral("DpcWatchdogProfileOffset"), 0);
            addD(o, kKernel, QStringLiteral("DpcWatchdogPeriod"), 0);
            addD(o, kKernel, QStringLiteral("DpcTimeout"), 0);
            addD(o, kKernel, QStringLiteral("IdealDpcRate"), 1);
            addD(o, kKernel, QStringLiteral("MaximumDpcQueueDepth"), 1);
            addD(o, kKernel, QStringLiteral("MinimumDpcRate"), 1);
            addD(o, kKernel, QStringLiteral("InterruptSteeringDisabled"), 1, 0);
            addD(o, kKernel, QStringLiteral("SerializeTimerExpiration"), 1, 0);
            addD(o, kKernel, QStringLiteral("SplitLargeCaches"), 1);
            addD(o, kKernel, QStringLiteral("MaximumSharedReadyQueueSize"), 128);
            t.registryOps = o;
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("kernel.process-priorities");
            t.category = QStringLiteral("advanced");
            t.name = QStringLiteral("Process priority (IFEO)");
            t.description = QStringLiteral(
                "Raises the raw-input/compositor/kernel processes (csrss, dwm, ntoskrnl) and "
                "demotes background noise (search indexer, svchost, update client, LSASS) via "
                "Image File Execution Options PerfOptions.");
            t.iconName = QStringLiteral("cpu");
            t.impact = Impact::Critical;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "risky: the source elevates csrss/dwm to Realtime (CpuPriorityClass=4). Realtime "
                "can starve kernel threads and, in rare cases, hang the session. Apply "
                "on a system you can recover.");
            QVector<RegistryOp> o;
            for (const QString& exe : { QStringLiteral("csrss.exe"), QStringLiteral("dwm.exe"),
                                        QStringLiteral("ntoskrnl.exe") }) {
                const QString p = kIfeo + exe + QStringLiteral("\\PerfOptions");
                addD(o, p, QStringLiteral("CpuPriorityClass"), 4);
                addD(o, p, QStringLiteral("IoPriority"), 3);
            }
            for (const QString& exe : { QStringLiteral("audiodg.exe"), QStringLiteral("lsass.exe"),
                                        QStringLiteral("SearchIndexer.exe"), QStringLiteral("svchost.exe"),
                                        QStringLiteral("TrustedInstaller.exe"), QStringLiteral("wuauclt.exe") }) {
                const QString p = kIfeo + exe + QStringLiteral("\\PerfOptions");
                addD(o, p, QStringLiteral("CpuPriorityClass"), 1);
                addD(o, p, QStringLiteral("IoPriority"), 0);
                addD(o, p, QStringLiteral("PagePriority"), 1);
            }
            t.registryOps = o;
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("power.ultimate-plan");
            t.category = QStringLiteral("general");
            t.name = QStringLiteral("Ultimate performance power plan");
            t.description = QStringLiteral(
                "Unlocks Windows' hidden Ultimate Performance power plan and switches to it. "
                "It removes the power-saving delays High Performance still allows. The two "
                "plans are close, so expect a small difference rather than a large one.");
            t.iconName = QStringLiteral("zap");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            // Revert switches to Balanced, not the plan that was active before.
            t.concern = QStringLiteral("no measured effect vs High Performance; not an optimization, never recommended.");
            t.commandOps = {
                cmd(QStringLiteral("powercfg"),
                    { QStringLiteral("-duplicatescheme"), QStringLiteral("e9a42b02-d5df-448d-aa00-03f14749eb61") },
                    {}),
                cmd(QStringLiteral("powercfg"),
                    { QStringLiteral("/setactive"), QStringLiteral("e9a42b02-d5df-448d-aa00-03f14749eb61") },
                    { QStringLiteral("/setactive"), QStringLiteral("381b4222-f694-41f0-9685-ff5bb260df2e") }),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("general.legacy-environment-tuning-set");
            t.category = QStringLiteral("advanced");
            t.name = QStringLiteral("Legacy environment-variable set");
            t.description = QStringLiteral(
                "Writes a large set of pseudo-environment-variable registry strings under "
                "Session Manager\\Environment covering CPU/IO/memory/network/GPU/audio "
                "interrupt and queue policies.");
            t.iconName = QStringLiteral("gauge");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "unverified/likely non-functional: none of these names (CPU_MAX_PENDING_IO, "
                "IO_COMPLETION_POLICY, GPU_MAX_PENDING_INTERRUPTS, etc.) are documented Windows "
                "environment variables or known kernel tunables - the Session Manager\\"
                "Environment key holds process-launch environment strings, not a system-wide "
                "settings store, so most of these have no effect on anything Windows reads. "
                "Two exceptions are real: DISABLE_DYNAMIC_TICK mirrors the documented bcdedit "
                "disabledynamictick flag (already shipped as bcd.disable-dynamic-tick) and "
                "MKL_DEBUG_CPU_TYPE is a real Intel MKL library variable, irrelevant outside "
                "MKL-linked applications. Kept for completeness per the source; do not expect "
                "a measurable effect from the rest.");
            QVector<RegistryOp> o;
            const QString env = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment");
            const QStringList kv = {
                QStringLiteral("CPU_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"),
                QStringLiteral("CPU_MAX_PENDING_IO"), QStringLiteral("0"),
                QStringLiteral("CPU_IDLE_POLICY"), QStringLiteral("0"),
                QStringLiteral("CPU_BOOST_POLICY"), QStringLiteral("2"),
                QStringLiteral("CPU_MAX_FREQUENCY"), QStringLiteral("100"),
                QStringLiteral("CPU_INTERRUPT_BALANCE_POLICY"), QStringLiteral("1"),
                QStringLiteral("MKL_DEBUG_CPU_TYPE"), QStringLiteral("10"),
                QStringLiteral("IO_COMPLETION_POLICY"), QStringLiteral("0"),
                QStringLiteral("IO_REQUEST_LIMIT"), QStringLiteral("1024"),
                QStringLiteral("DISK_MAX_PENDING_IO"), QStringLiteral("0"),
                QStringLiteral("IO_PRIORITY"), QStringLiteral("0"),
                QStringLiteral("DISK_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"),
                QStringLiteral("IO_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"),
                QStringLiteral("POWER_THROTTLE_POLICY"), QStringLiteral("0"),
                QStringLiteral("POWER_IDLE_TIMEOUT"), QStringLiteral("0"),
                QStringLiteral("CPU_POWER_POLICY"), QStringLiteral("1"),
                QStringLiteral("DISABLE_DYNAMIC_TICK"), QStringLiteral("yes"),
                QStringLiteral("MEMORY_MAX_ALLOCATION"), QStringLiteral("0"),
                QStringLiteral("MEMORY_LATENCY_POLICY"), QStringLiteral("0"),
                QStringLiteral("MEMORY_PREFETCH_POLICY"), QStringLiteral("2"),
                QStringLiteral("MEMORY_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"),
                QStringLiteral("DWM_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"),
                QStringLiteral("DWM_COMPOSITOR_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"),
                QStringLiteral("NETWORK_BUFFER_SIZE"), QStringLiteral("512"),
                QStringLiteral("NETWORK_INTERRUPT_COALESCING"), QStringLiteral("0"),
                QStringLiteral("NETWORK_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"),
                QStringLiteral("TIMER_RESOLUTION"), QStringLiteral("0"),
                QStringLiteral("THREAD_SCHEDULER_POLICY"), QStringLiteral("0"),
                QStringLiteral("GPU_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"),
                QStringLiteral("NETWORK_ADAPTER_PENDING_INTERRUPTS"), QStringLiteral("0"),
                QStringLiteral("NETWORK_ADAPTER_MAX_PENDING_IO"), QStringLiteral("0"),
                QStringLiteral("NETWORK_ADAPTER_INTERRUPT_MODERATION"), QStringLiteral("0"),
                QStringLiteral("NETWORK_ADAPTER_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"),
                QStringLiteral("STORAGE_DEVICE_PENDING_INTERRUPTS"), QStringLiteral("0"),
                QStringLiteral("STORAGE_DEVICE_MAX_PENDING_IO"), QStringLiteral("0"),
                QStringLiteral("STORAGE_DEVICE_COMPLETION_POLICY"), QStringLiteral("0"),
                QStringLiteral("STORAGE_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"),
                QStringLiteral("STORAGE_DEVICE_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"),
                QStringLiteral("USB_DEVICE_PENDING_INTERRUPTS"), QStringLiteral("0"),
                QStringLiteral("USB_DEVICE_MAX_PENDING_IO"), QStringLiteral("0"),
                QStringLiteral("USB_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"),
                QStringLiteral("USB_DEVICE_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"),
                QStringLiteral("PCIE_DEVICE_PENDING_INTERRUPTS"), QStringLiteral("0"),
                QStringLiteral("PCIE_DEVICE_MAX_PENDING_IO"), QStringLiteral("0"),
                QStringLiteral("PCIE_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"),
                QStringLiteral("PCIE_DEVICE_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"),
                QStringLiteral("GPU_PENDING_INTERRUPTS"), QStringLiteral("0"),
                QStringLiteral("GPU_MAX_PENDING_COMPUTE"), QStringLiteral("0"),
                QStringLiteral("GPU_MAX_PENDING_RENDER"), QStringLiteral("0"),
                QStringLiteral("AUDIO_DEVICE_PENDING_INTERRUPTS"), QStringLiteral("0"),
                QStringLiteral("AUDIO_DEVICE_BUFFER_SIZE"), QStringLiteral("512"),
                QStringLiteral("AUDIO_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"),
                QStringLiteral("AUDIO_DEVICE_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"),
                QStringLiteral("DEVICE_PENDING_INTERRUPTS"), QStringLiteral("0"),
                QStringLiteral("DEVICE_MAX_PENDING_IO"), QStringLiteral("0"),
                QStringLiteral("DEVICE_COMPLETION_POLICY"), QStringLiteral("0"),
                QStringLiteral("DEVICE_MAX_PENDING_INTERRUPTS"), QStringLiteral("0"),
            };
            for (int i = 0; i + 1 < kv.size(); i += 2)
                o.push_back(stringOp(env, kv.at(i), kv.at(i + 1), std::nullopt));
            t.registryOps = o;
            c.push_back(t);
        }
    }
}
