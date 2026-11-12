#include "ProtectedServices.h"

#include <QHash>
#include <QSet>
#include <QStringLiteral>

namespace vth::win::services
{
    namespace
    {

        const QHash<QString, QString>& protectedTable() {
            static const QHash<QString, QString> kTable = {
                {QStringLiteral("rpcss"),
                QStringLiteral("Remote Procedure Call. Disabling breaks every COM call in Windows; the system will not boot to a usable desktop.")},
                {QStringLiteral("rpceptmapper"),
                QStringLiteral("RPC Endpoint Mapper. Required by RpcSs; disabling produces the same outcome.")},
                {QStringLiteral("dcomlaunch"),
                QStringLiteral("DCOM Server Process Launcher. Hosts the COM activation service; disabling kills WMI, Task Scheduler, and most management tooling.")},
                {QStringLiteral("plugplay"),
                QStringLiteral("Plug and Play. Drives device enumeration; disabling will not detect new hardware and breaks resume from sleep.")},
                {QStringLiteral("power"),
                QStringLiteral("Power. Manages power policy notifications; disabling breaks the power plan switcher and sleep handling.")},
                {QStringLiteral("brokerinfrastructure"),
                QStringLiteral("Background Tasks Infrastructure. Required for UWP package activation; disabling crashes the Start menu and Settings app.")},
                {QStringLiteral("coremessagingregistrar"),
                QStringLiteral("CoreMessaging. Required by user-mode subsystems for RPC-style messaging; disabling cascades into UI failures across shell + UWP.")},
                {QStringLiteral("systemeventsbroker"),
                QStringLiteral("System Events Broker. Coordinates background tasks for UWP apps; disabling causes Settings, Cortana, and Store to fail to launch.")},
                {QStringLiteral("statesvc"),
                QStringLiteral("State Repository Service. Manages state for UWP application packages; disabling renders the Store and many built-in apps unusable.")},
                {QStringLiteral("staterepository"),
                QStringLiteral("State Repository Service (legacy display name). Same protection as StateSvc.")},
                {QStringLiteral("user manager"),
                QStringLiteral("User Manager. Manages per-user runtime state for UWP and the shell; disabling prevents new user sessions from logging in.")},
                {QStringLiteral("usermanager"),
                QStringLiteral("User Manager. Manages per-user runtime state for UWP and the shell; disabling prevents new user sessions from logging in.")},
                {QStringLiteral("profsvc"),
                QStringLiteral("User Profile Service. Loads/unloads user profiles at logon; disabling causes the temp profile fallback at every login.")},
                {QStringLiteral("eventsystem"),
                QStringLiteral("COM+ Event System. Required by countless system components; disabling produces unreliable system behavior without an obvious symptom.")},
                {QStringLiteral("eventlog"),
                QStringLiteral("Windows Event Log. Disabling breaks the boot sequence on most builds; many services refuse to start without it.")},

                {QStringLiteral("samss"),
                QStringLiteral("Security Accounts Manager. Holds the local account database; disabling is not allowed by Windows itself (the SCM rejects the request) and is here as belt-and-braces.")},
                {QStringLiteral("lsm"),
                QStringLiteral("Local Session Manager. Manages user-mode logon sessions; disabling produces a permanent lock-screen-loop on next boot.")},
                {QStringLiteral("cryptsvc"),
                QStringLiteral("Cryptographic Services. Required for code signing, certificate validation, and Windows Update. Disabling breaks AC handshakes for several anti-cheats (Vanguard).")},
                {QStringLiteral("keyiso"),
                QStringLiteral("CNG Key Isolation. Stores private keys for crypto operations; disabling breaks BitLocker, Wi-Fi enterprise auth, and signed driver enumeration.")},
                {QStringLiteral("vaultsvc"),
                QStringLiteral("Credential Manager. Stores Windows-saved credentials; disabling cascades into network share failures and stored Wi-Fi profile loss.")},
                {QStringLiteral("sens"),
                QStringLiteral("System Event Notification Service. Disabling breaks logon scripts, Group Policy refresh, and several COM components.")},
                {QStringLiteral("seclogon"),
                QStringLiteral("Secondary Logon. Required by 'Run as administrator' from non-elevated sessions; disabling blocks UAC elevation in many flows.")},
                {QStringLiteral("gpsvc"),
                QStringLiteral("Group Policy Client. Disabling is technically possible but breaks every domain-joined and many local Group Policy invocations; do not toggle.")},

                {QStringLiteral("dhcp"),
                QStringLiteral("DHCP Client. Disabling severs the machine from any non-static-IP network - including most home networks. Reachable only with a manual IP config.")},
                {QStringLiteral("dnscache"),
                QStringLiteral("DNS Client. Disabling does not stop DNS resolution but cripples performance and breaks Windows Update + many enterprise scenarios.")},
                {QStringLiteral("nlasvc"),
                QStringLiteral("Network Location Awareness. Powers the public/private/domain network classification; disabling causes the firewall to apply 'public' rules everywhere.")},
                {QStringLiteral("netprofm"),
                QStringLiteral("Network List Service. Sister to NlaSvc; disabling produces unstable network category detection.")},
                {QStringLiteral("nsi"),
                QStringLiteral("Network Store Interface Service. Disabling kills all network connectivity (TCP/IP stack-internal RPC).")},
                {QStringLiteral("mpssvc"),
                QStringLiteral("Windows Defender Firewall. Disabling is sometimes done deliberately for gaming, but the SCM enforces dependencies - and several anti-cheats reject hosts with the firewall service stopped. Mark protected; offer firewall-off as a separate Security tweak with explicit consent.")},
                {QStringLiteral("bfe"),
                QStringLiteral("Base Filtering Engine. Underlying WFP layer used by mpssvc and IPsec; disabling kills firewall + VPN simultaneously.")},
                {QStringLiteral("netman"),
                QStringLiteral("Network Connections. Required for Wi-Fi scan, VPN, and some adapter configuration; disabling causes adapters to vanish from the Network and Sharing Center.")},
                {QStringLiteral("wcmsvc"),
                QStringLiteral("Windows Connection Manager. Coordinates Wi-Fi/Ethernet/cellular failover and VPN auto-trigger; disabling reverts Windows to Win7-era network behavior.")},

                {QStringLiteral("audiosrv"),
                QStringLiteral("Windows Audio. Required for any audio output. Disabling is sometimes attempted as a 'latency' tweak; the latency cost of leaving it running is single-digit microseconds and audio hardware itself fails without it.")},
                {QStringLiteral("audioendpointbuilder"),
                QStringLiteral("Windows Audio Endpoint Builder. Audiosrv depends on this; disabling kills audio device enumeration entirely.")},
                {QStringLiteral("amd external events utility"),
                QStringLiteral("AMD External Events Utility. Required for Adrenalin features, undervolting persistence, and FreeSync handshake. Some tweak guides advise disabling; users get hard-locks on Ryzen + Adrenalin systems.")},

                {QStringLiteral("schedule"),
                QStringLiteral("Task Scheduler. Disabling breaks defrag, Windows Update servicing, time sync, anti-cheat hooks, and many third-party installer post-install steps.")},
                {QStringLiteral("themes"),
                QStringLiteral("Themes. Disabling reverts the desktop to Windows Classic; the official 'gaming optimization' use case is not worth the broken DWM compositing fallbacks on Win11.")},
                {QStringLiteral("trustedinstaller"),
                QStringLiteral("Windows Modules Installer. Required for any Windows Update, .NET install, optional features. Disabling breaks the servicing stack outright.")},

                {QStringLiteral("vgc"),
                QStringLiteral("Riot Vanguard. Required to launch Valorant and (since 2024) League of Legends in many regions; disabling locks the user out of those titles.")},
                {QStringLiteral("vgk"),
                QStringLiteral("Riot Vanguard kernel driver. Sister to vgc; disabling has the same effect.")},
                {QStringLiteral("easyanticheat"),
                QStringLiteral("EasyAntiCheat. Required for Fortnite, Apex Legends, Rust, Dead by Daylight, and many other titles.")},
                {QStringLiteral("easyanticheat_eos"),
                QStringLiteral("EasyAntiCheat (EOS variant). Same protection as EasyAntiCheat.")},
                {QStringLiteral("battleyeservice"),
                QStringLiteral("BattlEye. Required for PUBG, Rainbow Six Siege, Destiny 2, ARMA, DayZ, and many other titles.")},
                {QStringLiteral("battleye"),
                QStringLiteral("BattlEye (alternate name). Same protection.")},
                {QStringLiteral("eslwire"),
                QStringLiteral("ESL Wire / FACEIT anti-cheat client. Required for FACEIT-eligible play.")},
                {QStringLiteral("faceit"),
                QStringLiteral("FACEIT Anti-Cheat. Disabling locks the user out of FACEIT matchmaking.")},
                {QStringLiteral("sppsvc"),
                QStringLiteral("Software Protection. Disabling causes Windows to revert to unactivated state at the next license-state poll.")},
                {QStringLiteral("clipsvc"),
                QStringLiteral("Client License Service. Required for Microsoft Store activation and some MSIX-licensed apps.")},

                {QStringLiteral("usosvc"),
                QStringLiteral("Update Orchestrator Service. Disabling can be done deliberately to control Windows Update timing, but breaks several shell flows; user must opt in via the explicit Updates tweak, not have it touched as a side-effect.")},
                {QStringLiteral("wlansvc"),
                QStringLiteral("WLAN AutoConfig. Required for Wi-Fi. Some 'wired only' setups disable it deliberately - but the protection prevents Vethida from doing so silently.")},
                {QStringLiteral("appmgmt"),
                QStringLiteral("Application Management. Required for MSI install/uninstall via Group Policy and the Settings -> Apps page.")},
                {QStringLiteral("appidsvc"),
                QStringLiteral("Application Identity. Required for AppLocker and some signature-validation flows.")},

                {QStringLiteral("windefend"),
                QStringLiteral("Windows Defender Antivirus Service. Several anti-cheats (Vanguard, FACEIT) refuse to launch on hosts with Defender stopped. Marked protected; the Security tweak page disables it explicitly with consent.")},
                {QStringLiteral("wdnissvc"),
                QStringLiteral("Network Inspection System (Defender). Tied to WinDefend; disabling produces the same anti-cheat side effects.")},
                {QStringLiteral("securityhealthservice"),
                QStringLiteral("Windows Security Health Service. Hosts the Windows Security UI; disabling produces 'Cannot open Windows Security' errors and leaves users without an obvious recovery path.")},

                {QStringLiteral("wpnservice"),
                QStringLiteral("Windows Push Notifications System Service. Required for toast notifications + Action Center.")},
                {QStringLiteral("wpnuserservice"),
                QStringLiteral("Windows Push Notifications User Service. Per-user companion to WpnService.")},
                {QStringLiteral("w32time"),
                QStringLiteral("Windows Time. Required by Kerberos, some game-launcher TLS handshakes, and any auth that depends on monotonic clock skew.")},

                {QStringLiteral("storsvc"),
                QStringLiteral("Storage Service. Required for storage device enumeration and the Settings -> Storage page.")},
                {QStringLiteral("winmgmt"),
                QStringLiteral("Windows Management Instrumentation. Disabling cripples PowerShell, Task Manager performance tab, anti-cheat hooks, and Group Policy refresh.")},
                {QStringLiteral("fontcache"),
                QStringLiteral("Windows Font Cache. Disabling causes severe text-rendering perf regressions and hangs in the shell.")},
                {QStringLiteral("camsvc"),
                QStringLiteral("Capability Access Manager. Mediates camera/microphone/location consent - disabling produces undefined consent behavior.")},
                {QStringLiteral("appreadiness"),
                QStringLiteral("App Readiness. Prepares apps for first launch; disabling causes the first-launch hang on Settings, Microsoft Store, and several built-in apps.")},
                {QStringLiteral("timebrokersvc"),
                QStringLiteral("Time Broker. Coordinates background tasks for UWP; disabling produces silent UWP-app crashes.")},
            };
            return kTable;
        }

    }

    namespace
    {
        // Listed for their warning, but not refused.
        const QSet<QString>& riskyOnly() {
            static const QSet<QString> k = {
                QStringLiteral("clipsvc"),          // Store activation
                QStringLiteral("storsvc"),          // Settings > Storage
                QStringLiteral("wpnuserservice"),   // push notifications
                QStringLiteral("themes"),           // visual styles
                QStringLiteral("usosvc"),           // Windows Update timing
                QStringLiteral("fontcache"),        // text-render perf, self-rebuilding
                QStringLiteral("timebrokersvc"),    // UWP background tasks
            };
            return k;
        }
    }

    Protection protectionOf(QStringView serviceName) {
        const QString key = serviceName.toString().toLower();
        if (!protectedTable().contains(key))
            return Protection::None;
        return riskyOnly().contains(key) ? Protection::Risky : Protection::Critical;
    }

    bool isProtected(QStringView serviceName) {
        return protectionOf(serviceName) == Protection::Critical;
    }

    QString protectedReason(QStringView serviceName) {
        return protectedTable().value(serviceName.toString().toLower());
    }
}
