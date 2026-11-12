#include "pch.h"
#include "FixCatalog.h"

#include <QCoreApplication>

#include <iterator>

namespace vth::ui::models
{
    namespace
    {
        using vth::win::fixes::FixId;

        struct RawFix
        {
            const char* group;     // nullptr = same group as the row above
            const char* name;
            const char* desc;
            FixId       id;
            const char* confirm;   // nullptr = run without confirmation
            bool        alwaysAsk = false;
        };
        const RawFix kFixes[] = {
            { QT_TR_NOOP("Sound, printing and devices"),
              QT_TR_NOOP("Restart audio services"),
              QT_TR_NOOP("No sound, or \"no audio output device is installed\" when there clearly is one. Restarts the audio services so devices are found again."),
              FixId::RestartAudioServices, nullptr },
            { nullptr,
              QT_TR_NOOP("Restore Bluetooth services"),
              QT_TR_NOOP("Bluetooth missing from Settings, or devices that will not pair. Puts the Bluetooth services back to their default startup."),
              FixId::RestoreBluetoothServices,
              QT_TR_NOOP("This resets the Bluetooth driver and the Bluetooth support, pairing, radio, and audio services to their default startup. A restart is required to finish. Continue?") },
            { nullptr,
              QT_TR_NOOP("Clear print queue"),
              QT_TR_NOOP("A print job stuck at \"deleting\" that blocks everything behind it. Stops the spooler, empties the queue, and starts it again."),
              FixId::ClearPrintQueue,
              QT_TR_NOOP("Every queued print job will be discarded, including any that are only paused. Continue?"), true },

            { QT_TR_NOOP("Network"),
              QT_TR_NOOP("Renew network address"),
              QT_TR_NOOP("Connected, but nothing loads. Asks your router for a fresh address and empties the DNS and ARP caches. No restart needed - try this before the stack repair below."),
              FixId::RenewNetworkAddress, nullptr },
            { nullptr,
              QT_TR_NOOP("Fix the false no-internet warning"),
              QT_TR_NOOP("A globe in the taskbar and apps insisting you are offline while browsing works fine. Some privacy tools switch off the check Windows uses to answer that question; this puts it back."),
              FixId::RepairConnectivityDetection,
              QT_TR_NOOP("This restores the connectivity check to its Windows defaults and briefly restarts the network status services. Continue?") },
            { nullptr,
              QT_TR_NOOP("Clear proxy settings"),
              QT_TR_NOOP("Every browser fails while the connection itself is fine - usually a proxy left behind by a VPN or a \"booster\" app. Clears it for your account and for system services."),
              FixId::ClearProxySettings,
              QT_TR_NOOP("If this PC connects through a proxy on purpose - some work and school networks do - clearing it drops internet for this account until the proxy is set up again. Continue?"), true },
            { nullptr,
              QT_TR_NOOP("Restore Wi-Fi and network services"),
              QT_TR_NOOP("An empty Wi-Fi list, or a connection that never gets an address. Turns the DHCP, DNS and network detection services back on after a debloat disabled them."),
              FixId::RestoreNetworkServices, nullptr },
            { nullptr,
              QT_TR_NOOP("Repair network stack"),
              QT_TR_NOOP("Nothing else worked. Resets Winsock and TCP/IP to a clean state and flushes DNS."),
              FixId::RepairNetworkStack,
              QT_TR_NOOP("This resets Winsock and the TCP/IP stack. A restart is required afterwards and you may need to reconnect to your network. Continue?"), true },
            { nullptr,
              QT_TR_NOOP("Reset firewall rules"),
              QT_TR_NOOP("A game or app that cannot connect because something added a blocking rule. Restores the rules Windows ships with; the firewall itself stays on."),
              FixId::ResetFirewallRules,
              QT_TR_NOOP("This removes every custom firewall rule on this PC, including ones your own applications added, and restores the Windows defaults. The firewall stays enabled. Continue?"), true },

            { QT_TR_NOOP("Windows and apps"),
              QT_TR_NOOP("Reset Windows Update"),
              QT_TR_NOOP("Updates stuck at a percentage, or failing with a code that means nothing. Clears the download cache and restarts the update services."),
              FixId::ResetWindowsUpdate,
              QT_TR_NOOP("This stops the Windows Update services, clears the update cache, and restarts them. Continue?") },
            { nullptr,
              QT_TR_NOOP("Reset Microsoft Store cache"),
              QT_TR_NOOP("A Store that opens to a blank page or fails every download. Clears its cache so it rebuilds on next open."),
              FixId::ResetStoreCache, nullptr },
            { nullptr,
              QT_TR_NOOP("Repair Start menu and search"),
              QT_TR_NOOP("A Start menu that will not open, a search box that swallows what you type, or a taskbar that ignores clicks. Re-registers the shell from the files already on this PC."),
              FixId::RepairStartMenuAndSearch,
              QT_TR_NOOP("This re-registers the Start menu, search, and taskbar components and restarts Windows Explorer - your desktop and taskbar will briefly disappear. Nothing is downloaded and your files are untouched. Continue?") },
            { nullptr,
              QT_TR_NOOP("Fix system clock"),
              QT_TR_NOOP("Sign-ins, game launchers, and websites failing with certificate errors. A clock that is wrong by minutes is enough to break all three."),
              FixId::SyncSystemClock, nullptr },
            { nullptr,
              QT_TR_NOOP("Repair the Settings app"),
              QT_TR_NOOP("Settings that closes as soon as it opens, or opens to a blank window. Registers it again from the files already on this PC."),
              FixId::RepairSettingsApp, nullptr },
            { nullptr,
              QT_TR_NOOP("Clear \"managed by your organization\""),
              QT_TR_NOOP("\"Some of these settings are managed by your organization\" on a PC nobody manages. Restores the Windows defaults for every setting a group policy value is holding."),
              FixId::ClearOrganizationPolicies,
              QT_TR_NOOP("This clears the group policy settings stored on this PC, including ones Vethida applied, and leaves a copy in %LOCALAPPDATA%\\Rens Engineering\\Vethida\\policy-backup. A restart is required to finish. Continue?"), true },

            { QT_TR_NOOP("Turned off by other tools"),
              QT_TR_NOOP("Let apps use the microphone and camera"),
              QT_TR_NOOP("Discord, games or calls that cannot hear you, or a camera that shows nothing. Privacy tools often block both for every app; this allows them again."),
              FixId::RestoreMicCameraAccess, nullptr },
            { nullptr,
              QT_TR_NOOP("Fix Xbox multiplayer and party chat"),
              QT_TR_NOOP("\"Teredo is unable to qualify\" in the Xbox app, or party chat and Game Pass multiplayer that cannot connect. Puts the Teredo connection Xbox networking uses back to Windows' defaults."),
              FixId::FixXboxNetworking, nullptr },
            { nullptr,
              QT_TR_NOOP("Stop the ms-gamingoverlay pop-up"),
              QT_TR_NOOP("\"You'll need a new app to open this ms-gamingoverlay link\" every time a game starts. Game Bar was removed; this stops Windows trying to open it."),
              FixId::FixGamingOverlayPopup, nullptr },
            { nullptr,
              QT_TR_NOOP("Restore Xbox and gaming services"),
              QT_TR_NOOP("Game Pass games that will not install or launch, or an Xbox sign-in that loops. Turns the gaming services a debloat disabled back on."),
              FixId::RestoreGameServices, nullptr },
            { nullptr,
              QT_TR_NOOP("Fix anti-cheat that will not start"),
              QT_TR_NOOP("Vanguard, FACEIT, EasyAntiCheat or BattlEye refusing to run. Turns off test signing, disabled driver checks and kernel debugging, which anti-cheat will not run alongside."),
              FixId::FixAntiCheatBlockers,
              QT_TR_NOOP("This changes three boot settings back to Windows' defaults. Vethida backs up the boot configuration first. A restart is required to finish. Continue?"), true },
            { nullptr,
              QT_TR_NOOP("Reset system timer settings"),
              QT_TR_NOOP("Stutter or uneven frame pacing after timer tweaks. Puts the boot timer settings back to Windows' defaults, which suit every current processor."),
              FixId::ResetTimerSettings,
              QT_TR_NOOP("Vethida backs up the boot configuration, then removes the timer overrides. A restart is required to finish. Continue?") },
            { nullptr,
              QT_TR_NOOP("Reset keyboard and mouse buffers"),
              QT_TR_NOOP("Keys that drop presses or a mouse that skips when things get busy. Puts back input buffers another tool shrank too far."),
              FixId::ResetInputBuffers, nullptr },
            { nullptr,
              QT_TR_NOOP("Repair the Microsoft Store"),
              QT_TR_NOOP("A Store that will not install anything, Microsoft account sign-ins that fail in the Store or the Xbox app, or a Store that is gone altogether. Turns its services back on, registers it again, and reinstalls it if it was removed."),
              FixId::RepairMicrosoftStore, nullptr },
            { nullptr,
              QT_TR_NOOP("Restore Windows Installer"),
              QT_TR_NOOP("Installers failing with \"the Windows Installer service could not be accessed\". Turns the service back on and registers it again."),
              FixId::RestoreWindowsInstaller, nullptr },
            { nullptr,
              QT_TR_NOOP("Restore the Power service"),
              QT_TR_NOOP("Power plans that will not change and sleep settings that do nothing. Every one of them goes through this service."),
              FixId::RestorePowerService, nullptr },
            { nullptr,
              QT_TR_NOOP("Restore the Windows Event Log"),
              QT_TR_NOOP("Wi-Fi settings that will not open, Task Scheduler errors, or installers that fail for no stated reason. Much of Windows stops working without this service."),
              FixId::RestoreEventLog, nullptr },
            { nullptr,
              QT_TR_NOOP("Restore the page file"),
              QT_TR_NOOP("Games that crash with \"out of memory\" on a PC with plenty of RAM. Hands the page file back to Windows, which sizes it on its own."),
              FixId::RestorePageFile,
              QT_TR_NOOP("Windows manages the page file on the system drive again, the way it does on a clean install. A restart is required to finish. Continue?") },
            { nullptr,
              QT_TR_NOOP("Fix typing in Start and search"),
              QT_TR_NOOP("Typing does nothing in the Start menu search, the emoji panel or the touch keyboard. Turns the text input service back on."),
              FixId::FixTypingInSearch, nullptr },
            { nullptr,
              QT_TR_NOOP("Fix window snapping"),
              QT_TR_NOOP("Windows that will not snap when dragged to a screen edge, even with Snap windows on in Settings. Turns back on the snapping settings Settings does not show."),
              FixId::FixWindowSnapping, nullptr },

            { QT_TR_NOOP("Explorer and search"),
              QT_TR_NOOP("Rebuild icon and thumbnail cache"),
              QT_TR_NOOP("Blank, wrong, or black icons, and thumbnails that will not appear. Clears both caches and restarts Explorer."),
              FixId::RebuildIconCache,
              QT_TR_NOOP("This closes and restarts Windows Explorer; your desktop and taskbar will briefly disappear. Continue?") },
            { nullptr,
              QT_TR_NOOP("Reset folder views"),
              QT_TR_NOOP("Every folder opening in the wrong view, or your column widths resetting themselves. Clears the stored per-folder settings so each folder goes back to its default."),
              FixId::ResetFolderViews,
              QT_TR_NOOP("Every folder returns to its default view, sort order, and column layout. Explorer will restart. Continue?"), true },
            { nullptr,
              QT_TR_NOOP("Rebuild search index"),
              QT_TR_NOOP("Search finding nothing for files you can see in the folder. Clears the index so Windows builds it again."),
              FixId::RebuildSearchIndex,
              QT_TR_NOOP("Windows rebuilds the index in the background afterwards, which can take a while on a large drive. Search results will be incomplete until it finishes. Continue?") },
            { nullptr,
              QT_TR_NOOP("Clear font cache"),
              QT_TR_NOOP("Blocky, garbled, or missing text in menus and title bars. Rebuilds the cache Windows renders text from."),
              FixId::ClearFontCache, nullptr },
            { nullptr,
              QT_TR_NOOP("Repair Recycle Bin"),
              QT_TR_NOOP("\"The Recycle Bin on C: is corrupted\" every time you sign in. Rebuilds it on each drive."),
              FixId::RepairRecycleBin,
              QT_TR_NOOP("Anything still in the Recycle Bin will be permanently deleted and cannot be restored afterwards. Empty it of anything you want first. Continue?"), true },

            { QT_TR_NOOP("Graphics and games"),
              QT_TR_NOOP("Clear shader caches"),
              QT_TR_NOOP("Stutter, flickering, or corrupted textures that started after a driver update. Clears the compiled shaders the old driver left behind; each game rebuilds its own on first launch."),
              FixId::ClearShaderCaches, nullptr },
            { nullptr,
              QT_TR_NOOP("Clear standby memory"),
              QT_TR_NOOP("Memory shown as \"in use\" long after the program closed, and stutter in a game that needs it back. Frees the cached standby list."),
              FixId::ClearStandbyMemory, nullptr },
            { nullptr,
              QT_TR_NOOP("Restore the NVIDIA Control Panel"),
              QT_TR_NOOP("The classic NVIDIA Control Panel gone from the desktop's right-click menu after a driver update. Current NVIDIA drivers no longer include it; this installs it from the Microsoft Store and puts the menu entry back."),
              FixId::RestoreNvidiaControlPanel,
              QT_TR_NOOP("This installs the NVIDIA Control Panel from the Microsoft Store if it is missing, turns on the NVIDIA display service it needs, and adds it to the desktop's right-click menu. Continue?") },

            { QT_TR_NOOP("Storage and power"),
              QT_TR_NOOP("Clear temporary files"),
              QT_TR_NOOP("Space taken by the temp directories and the crash reports Windows queues and never sends."),
              FixId::ClearTemporaryFiles, nullptr },
            { nullptr,
              QT_TR_NOOP("Compact component store"),
              QT_TR_NOOP("A Windows folder that keeps growing on an older install. Removes component versions that updates have superseded."),
              FixId::CompactComponentStore,
              QT_TR_NOOP("This can take ten minutes or more with heavy disk activity. Installed updates stay removable afterwards. Continue?") },
            { nullptr,
              QT_TR_NOOP("Restore default power plans"),
              QT_TR_NOOP("Power plans missing from Settings, or a machine that sleeps when it should not. Puts back the plans Windows ships with."),
              FixId::RestoreDefaultPowerPlans,
              QT_TR_NOOP("This removes every custom power plan on this PC, including Vethida's performance plan, which you would need to install again. Continue?"), true },

            { QT_TR_NOOP("Deep repair"),
              QT_TR_NOOP("Run system file check"),
              QT_TR_NOOP("Crashes and errors with no obvious cause. Scans every protected system file and repairs what is damaged."),
              FixId::RunSystemFileCheck,
              QT_TR_NOOP("The scan can take several minutes to finish. Continue?") },
            { nullptr,
              QT_TR_NOOP("Schedule disk check"),
              QT_TR_NOOP("Freezes, or files that will not open. Queues a full disk check on the system drive for the next restart."),
              FixId::ScheduleDiskCheck,
              QT_TR_NOOP("This queues a disk check that runs on the system drive at the next restart. Continue?") },
            { nullptr,
              QT_TR_NOOP("Securely wipe free space"),
              QT_TR_NOOP("Before selling or handing on a PC. Overwrites deleted-file remnants so they cannot be recovered."),
              FixId::SecureWipeFreeSpace,
              QT_TR_NOOP("This overwrites all free space on the system drive and can take a very long time (potentially hours) with heavy disk activity. Continue?"), true },
        };
    }

    const QVector<FixDef>& fixCatalog()
    {
        static const QVector<FixDef> catalog = []() {
            QVector<FixDef> out;
            out.reserve(static_cast<int>(std::size(kFixes)));
            for (const RawFix& f : kFixes) {
                FixDef d;
                d.group       = f.group ? QCoreApplication::translate("vth::ui::FixesView", f.group)
                                        : QString();
                d.name        = QCoreApplication::translate("vth::ui::FixesView", f.name);
                d.description = QCoreApplication::translate("vth::ui::FixesView", f.desc);
                d.id          = f.id;
                d.confirm     = f.confirm
                                ? QCoreApplication::translate("vth::ui::FixesView", f.confirm)
                                : QString();
                d.alwaysAsk   = f.alwaysAsk;
                out.append(std::move(d));
            }
            return out;
        }();
        return catalog;
    }

    const FixDef* findFix(vth::win::fixes::FixId id)
    {
        for (const FixDef& f : fixCatalog())
            if (f.id == id)
                return &f;
        return nullptr;
    }
}
