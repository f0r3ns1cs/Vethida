#include "TweakEngine.h"

namespace vth::tweaks::detail {

void appendDefsAppearance(QVector<TweakSpec>& c) {
    {
        TweakSpec t;
        t.id = QStringLiteral("app.explorer.audio_device_settings");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Audio");
        t.name = QStringLiteral("Audio device detection settings");
        t.description = QStringLiteral("Configures Windows audio: disables communication detection ducking, shows hidden and disconnected audio devices in Sound settings.");
        t.iconName = QStringLiteral("microphone-off");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Other audio will no longer auto-lower during calls or communication activity"),
            QStringLiteral("Sound settings will list hidden and disconnected devices that are normally filtered out"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Multimedia\\Audio"), QStringLiteral("UserDuckingPreference"), 3u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Multimedia\\Audio\\DeviceCpl"), QStringLiteral("ShowHiddenDevices"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Multimedia\\Audio\\DeviceCpl"), QStringLiteral("ShowDisconnectedDevices"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.qol.auto_end_tasks");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Shutdown");
        t.name = QStringLiteral("Auto-end hung tasks on shutdown");
        t.description = QStringLiteral("Automatically kills hung applications during shutdown instead of waiting for the 'End Now' dialog.");
        t.iconName = QStringLiteral("circle-x");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Apps with unsaved work will be force-closed after 2 seconds at shutdown instead of prompting you"),
            QStringLiteral("Services that need longer than 2 seconds to shut down cleanly may be killed mid-write"),
        };
        t.registryOps = {
            stringOp(QStringLiteral("HKCU\\Control Panel\\Desktop"), QStringLiteral("AutoEndTasks"), QStringLiteral("1"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Control Panel\\Desktop"), QStringLiteral("WaitToKillAppTimeout"), QStringLiteral("2000"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Control Panel\\Desktop"), QStringLiteral("HungAppTimeout"), QStringLiteral("1000"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control"), QStringLiteral("WaitToKillServiceTimeout"), QStringLiteral("2000"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.theme.black_accent");
        t.category = QStringLiteral("appearance");
        // Finishes the dark theme by blacking out the title bars and menu highlights
        // that stay colored in dark mode, so it sits with the dark mode switches.
        t.subCategory = QStringLiteral("Dark mode");
        t.name = QStringLiteral("True black accent color");
        t.description = QStringLiteral("Sets the system accent color to true black (#000000) for a consistent dark appearance across title bars and UI elements.");
        t.iconName = QStringLiteral("palette");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Title bars, Start menu highlights, and accent-colored UI elements will become pure black"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\DWM"), QStringLiteral("AccentColor"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\DWM"), QStringLiteral("ColorizationColor"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Accent"), QStringLiteral("AccentColorMenu"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.ui.classic_context_menu");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Context menu");
        t.name = QStringLiteral("Restore classic context menu");
        t.description = QStringLiteral("Restores the full Windows 10-style right-click context menu instead of the truncated Windows 11 version.");
        t.iconName = QStringLiteral("list");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Right-click menus will show all legacy options up front instead of the Win11 compact menu"),
            QStringLiteral("Requires signing out or restarting Explorer to take effect"),
        };
        t.registryOps = {
            stringOp(QStringLiteral("HKCU\\Software\\Classes\\CLSID\\{86ca1aa0-34aa-4e8b-a509-50c905bae2a2}\\InprocServer32"), QStringLiteral(""), QStringLiteral(""), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.lockscreen.clean_logon");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Lock screen");
        t.name = QStringLiteral("Clean logon screen");
        t.description = QStringLiteral("Removes the sign-in background image and acrylic blur and hides lock-screen app notifications for a plain logon screen. Note: it does not hide fast user switching (that value is left at its default).");
        t.iconName = QStringLiteral("lock");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Logon screen will use a solid color instead of your chosen background image"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System"), QStringLiteral("DisableLogonBackgroundImage"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System"), QStringLiteral("HideFastUserSwitching"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System"), QStringLiteral("DisableAcrylicBackgroundOnLogon"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System"), QStringLiteral("DisableLockScreenAppNotifications"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.explorer.cmd_tweaks");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Command Prompt");
        t.name = QStringLiteral("CMD tab completion");
        t.description = QStringLiteral("Enables tab-based path and command completion in Command Prompt.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Tab key in Command Prompt will complete file paths and commands instead of inserting a literal tab character"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Command Processor"), QStringLiteral("CompletionChar"), 9u, 64u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Command Processor"), QStringLiteral("PathCompletionChar"), 9u, 64u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.explorer.compact_mode");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Explorer");
        t.name = QStringLiteral("Enable Compact View Mode");
        t.description = QStringLiteral("Enables compact view mode in Explorer with reduced spacing, disables touch-oriented file explorer improvements, and hides the listview watermark for a cleaner desktop experience.");
        t.iconName = QStringLiteral("folder-open");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Explorer row spacing tightens and touch-friendly padding is removed, which may make tapping items on touchscreens harder"),
            QStringLiteral("The empty-folder watermark text disappears from empty views"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), QStringLiteral("UseCompactMode"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), QStringLiteral("FileExplorerInTouchImprovement"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), QStringLiteral("ListviewWatermark"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.explorer.conflict_check_all");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("File operations");
        t.name = QStringLiteral("Auto-check 'do this for all' in conflicts");
        t.description = QStringLiteral("Automatically checks the 'Do this for all current items' checkbox in file conflict dialogs.");
        t.iconName = QStringLiteral("folder-open");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("File conflict dialogs will default to applying your choice to every remaining item instead of prompting for each"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\OperationStatusManager"), QStringLiteral("ConfirmationCheckBoxDoForAll"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.explorer.control_panel_view");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Control Panel");
        t.name = QStringLiteral("Control Panel icon view");
        t.description = QStringLiteral("Sets Control Panel to show all items in large icon view and starts on the icon view page.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Control Panel will open directly into the All Items icon view instead of Category view"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ControlPanel"), QStringLiteral("AllItemsIconView"), 2u, 0u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ControlPanel"), QStringLiteral("StartupPage"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.explorer.copy_buffer");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("File operations");
        t.name = QStringLiteral("Increase Explorer copy buffer size");
        t.description = QStringLiteral("Increases the file copy buffer size to 1MB for faster file transfers and sets max shell task scheduler worker threads.");
        t.iconName = QStringLiteral("folder-open");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Explorer will use a larger 1 MB copy buffer and more shell worker threads, slightly increasing memory use during transfers"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), QStringLiteral("CopyBufferSize"), 1048576u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellTaskScheduler"), QStringLiteral("MaxWorkerThreadsPerScheduler"), 255u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.visual.custom_fx_mode");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Visual effects");
        t.name = QStringLiteral("Set custom visual effects mode");
        t.description = QStringLiteral("Sets VisualFXSetting=3 (Custom) so individual visual effect settings take precedence over the system preset.");
        t.iconName = QStringLiteral("palette");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Performance Options will switch to Custom mode, letting individual effect toggles override the system preset"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VisualEffects"), QStringLiteral("VisualFXSetting"), 3u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.darkmode.apps");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Dark mode");
        t.name = QStringLiteral("Dark mode for apps");
        t.description = QStringLiteral("Forces dark theme for Windows apps.");
        t.iconName = QStringLiteral("moon");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Apps that follow the system theme will switch to dark backgrounds and light text"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"), QStringLiteral("AppsUseLightTheme"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.darkmode.system");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Dark mode");
        t.name = QStringLiteral("Dark mode for system");
        t.description = QStringLiteral("Forces dark theme for the Windows system chrome (taskbar, Start menu, Action Center).");
        t.iconName = QStringLiteral("moon");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Taskbar, Start menu, and notification area will switch to dark colors"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"), QStringLiteral("SystemUsesLightTheme"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.visual.default_copy_drop");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("File operations");
        t.name = QStringLiteral("Default to copy on drag-and-drop");
        t.description = QStringLiteral("Changes the default drag-and-drop behavior to copy files instead of move, preventing accidental file relocation.");
        t.iconName = QStringLiteral("folder-open");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Drag-and-drop between folders on the same drive will copy instead of move, creating duplicates unless you hold Shift"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Classes\\*"), QStringLiteral("DefaultDropEffect"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Classes\\AllFilesystemObjects"), QStringLiteral("DefaultDropEffect"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.explorer.delayed_apps_priority");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Startup");
        t.name = QStringLiteral("Optimize delayed app launch priority");
        t.description = QStringLiteral("Configures Explorer delayed app launch to minimum IO/page/priority class and zero delay, cutting post-logon overhead.");
        t.iconName = QStringLiteral("zap");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Background apps launched at logon may take marginally longer to initialize because they run at reduced priority."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced\\DelayedApps"), QStringLiteral("BoxedIoPriority"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced\\DelayedApps"), QStringLiteral("BoxedPagePriority"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced\\DelayedApps"), QStringLiteral("BoxedPriorityClass"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced\\DelayedApps"), QStringLiteral("Delay_Sec"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.taskbar.disable_account_notifs");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Start menu");
        t.name = QStringLiteral("Disable start-menu account notifications");
        t.description = QStringLiteral("Stops account-setup nags from appearing on the Start menu.");
        t.iconName = QStringLiteral("user-minus");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Start menu account setup prompts and backup reminders will stop appearing"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), QStringLiteral("Start_AccountNotifications"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.notifications.disable_action_center");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Notifications");
        t.name = QStringLiteral("Disable Action Center");
        t.description = QStringLiteral("Disables the Windows Action Center / Notification Center entirely.");
        t.iconName = QStringLiteral("bell-off");
        t.impact = Impact::Medium;
        t.warnings = {
            QStringLiteral("You will stop seeing Windows and app notifications, including security alerts and message toasts"),
            QStringLiteral("The notification history and quick-action buttons in the sidebar become unavailable"),
            QStringLiteral("Sign out and back in for the change to fully apply"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Policies\\Microsoft\\Windows\\Explorer"), QStringLiteral("DisableNotificationCenter"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.search.disable_all_users");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Search & Cortana");
        t.name = QStringLiteral("Disable Windows Search for all users");
        t.description = QStringLiteral("Disables Windows Search on the taskbar and Start Menu for all users via PolicyManager and Group Policy.");
        t.iconName = QStringLiteral("search");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Windows Search in the taskbar and Start menu will be disabled for every user on this machine"),
            QStringLiteral("Start-menu typing to find apps will stop working until this tweak is reverted"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\Search\\DisableSearch"), QStringLiteral("value"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search"), QStringLiteral("DisableSearch"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("appearance.disable_animations");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Animations");
        t.name = QStringLiteral("Disable window animations");
        t.description = QStringLiteral("Disables taskbar animations and Aero Peek for a snappier desktop experience.");
        t.iconName = QStringLiteral("palette");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Aero Peek taskbar thumbnail previews and desktop peek will stop working"),
            QStringLiteral("Window minimize, restore, and taskbar transitions will snap instead of animating"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\DWM"), QStringLiteral("EnableAeroPeek"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), QStringLiteral("TaskbarAnimations"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.search.disable_bing");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Search & Cortana");
        t.name = QStringLiteral("Disable Bing search in Start menu");
        t.description = QStringLiteral("Disables Bing web search results from appearing in Windows Start Menu and Search, and blocks connected search via Group Policy.");
        t.iconName = QStringLiteral("search");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Start-menu search will no longer include Bing web results; only local apps, settings, and files will match"),
            QStringLiteral("Connected-search features that rely on Microsoft's cloud index will be blocked"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Search"), QStringLiteral("BingSearchEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Search"), QStringLiteral("WebControlStatus"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Search"), QStringLiteral("WebControlSecondaryStatus"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search"), QStringLiteral("DisableWebSearch"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search"), QStringLiteral("ConnectedSearchUseWeb"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search"), QStringLiteral("ConnectedSearchUseWebOverMeteredConnections"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search"), QStringLiteral("BingSearchEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search"), QStringLiteral("AllowCloudSearch"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search"), QStringLiteral("ConnectedSearchPrivacy"), 3u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search"), QStringLiteral("ConnectedSearchSafeSearch"), 3u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.notifications.disable_center_policy");
        t.category = QStringLiteral("debloating");
        t.subCategory = QStringLiteral("Notifications");
        t.name = QStringLiteral("Disable Notification Center (policy)");
        t.description = QStringLiteral("Kills the notification center system-wide via Group Policy, suppresses legacy balloon tips, hides Recommended section from Start Menu, and disables systray promotions.");
        t.iconName = QStringLiteral("activity");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Notification Center (Action Center) flyout will be completely disabled; toast notifications and reminders will no longer appear"),
            QStringLiteral("Start menu will hide the Recommended section, and system tray promotional balloons are suppressed"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Explorer"), QStringLiteral("DisableNotificationCenter"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Explorer"), QStringLiteral("EnableLegacyBalloonNotifications"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Explorer"), QStringLiteral("NoBalloonFeatureAdvertisements"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Explorer"), QStringLiteral("NoSystraySystemPromotion"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Explorer"), QStringLiteral("HideRecommendedSection"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\Explorer"), QStringLiteral("DisableNotificationCenter"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.search.disable_cloud");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Search & Cortana");
        t.name = QStringLiteral("Disable cloud search");
        t.description = QStringLiteral("Disables Microsoft Account and Azure AD cloud search integration in Windows Search settings.");
        t.iconName = QStringLiteral("search");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Windows Search will no longer include results from your Microsoft Account or Azure AD OneDrive and Outlook content"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\SearchSettings"), QStringLiteral("IsMSACloudSearchEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\SearchSettings"), QStringLiteral("IsAADCloudSearchEnabled"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.explorer.disable_delete_confirm");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Recycle Bin");
        t.name = QStringLiteral("Disable Recycle Bin delete confirmation");
        t.description = QStringLiteral("Disables the confirmation dialog when deleting files to the Recycle Bin.");
        t.iconName = QStringLiteral("trash");
        t.impact = Impact::Low;
        t.registryOps = {
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"), QStringLiteral("ConfirmFileDelete"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"), QStringLiteral("ConfirmFileDelete"), QStringLiteral("0"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.taskbar.disable_feeds_autoupdate");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Widgets");
        t.name = QStringLiteral("Disable feeds background refresh");
        t.description = QStringLiteral("Stops the Feeds widget from refreshing content in the background.");
        t.iconName = QStringLiteral("rss");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Widgets content will only refresh when the panel is opened; weather on taskbar may go stale"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Feeds"), QStringLiteral("ShellFeedsTaskbarContentUpdateMode"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.lockscreen.disable_firstlogon_winlogon");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Logon");
        t.name = QStringLiteral("Disable first logon animation");
        t.description = QStringLiteral("Skips the first-sign-in animation for new user profiles (EnableFirstLogonAnimation=0) so new accounts reach the desktop faster.");
        t.iconName = QStringLiteral("monitor");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Takes effect only for NEW user profiles; existing accounts already passed first-logon and will see no change."),
            QStringLiteral("Requires reboot before the suppressed animation state is picked up by the logon UI subsystem."),
            QStringLiteral("Cosmetic-only: does not affect post-first-logon sign-in animations or performance for returning users."),
            QStringLiteral("Reverting deletes the registry value, restoring Windows' default behavior (animation plays) on the next newly created account."),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"), QStringLiteral("EnableFirstLogonAnimation"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.search.disable_history");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Search & Cortana");
        t.name = QStringLiteral("Disable Search History");
        t.description = QStringLiteral("Disables device search history and dynamic search box in Windows Search settings.");
        t.iconName = QStringLiteral("search");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Search will no longer remember your recent queries or suggest past searches as you type"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Search"), QStringLiteral("DeviceHistoryEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\SearchSettings"), QStringLiteral("IsDeviceSearchHistoryEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\SearchSettings"), QStringLiteral("IsDynamicSearchBoxEnabled"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.explorer.disable_history");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Privacy");
        t.name = QStringLiteral("Disable File Explorer history");
        t.description = QStringLiteral("Disables recent documents history, tracking, activity feed, user activities upload, and hides recently added apps.");
        t.iconName = QStringLiteral("folder-open");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Jump lists, Quick Access, and Recent Documents will stop tracking files you open"),
            QStringLiteral("Timeline and cross-device Activity History will be blocked, so resuming work on another Windows device becomes unavailable"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"), QStringLiteral("NoRecentDocsHistory"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"), QStringLiteral("NoRecentDocsMenu"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"), QStringLiteral("NoRecentDocsNetHood"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System"), QStringLiteral("EnableActivityFeed"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System"), QStringLiteral("PublishUserActivities"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System"), QStringLiteral("UploadUserActivities"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer"), QStringLiteral("ShowRecent"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer"), QStringLiteral("ShowFrequent"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.shell.disable_immersive_shell");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Shell");
        t.name = QStringLiteral("Disable immersive shell and tablet mode");
        t.description = QStringLiteral("Disables tablet mode, forces desktop sign-in mode, suppresses convertible slate mode prompts, and disables the immersive context menu in favor of classic menus.");
        t.iconName = QStringLiteral("house");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Tablet mode is disabled and the system will not prompt to switch modes on 2-in-1 devices"),
            QStringLiteral("The Win11 immersive context menu reverts to the legacy full menu without the Show more options step"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ImmersiveShell"), QStringLiteral("TabletMode"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ImmersiveShell"), QStringLiteral("SignInMode"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ImmersiveShell"), QStringLiteral("ConvertibleSlateModePromptPreference"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\FlightedFeatures"), QStringLiteral("ImmersiveContextMenu"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ImmersiveShell"), QStringLiteral("UseActionCenterExperience"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.search.disable_index");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Search indexing");
        t.name = QStringLiteral("Disable search index on battery and removable drives");
        t.description = QStringLiteral("Prevents Windows Search from indexing on battery power and removable drives, and disables Search Companion content updates.");
        t.iconName = QStringLiteral("search");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Search indexing will pause when running on battery, so newly added files take longer to appear in results"),
            QStringLiteral("Files on USB drives and other removable media will not be indexed and must be found by browsing"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search"), QStringLiteral("PreventIndexOnBattery"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search"), QStringLiteral("DisableRemovableDriveIndexing"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\SearchCompanion"), QStringLiteral("DisableContentFileUpdate"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.taskbar.disable_iris_recs");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Start menu");
        t.name = QStringLiteral("Disable Start menu recommendations");
        t.description = QStringLiteral("Disables tips, shortcuts, and new app recommendations shown on the Start menu (Iris recommendations).");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Start menu will stop showing tips, shortcuts, and promotional app suggestions in the recommended area"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), QStringLiteral("Start_IrisRecommendations"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.explorer.disable_iris");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Telemetry");
        t.name = QStringLiteral("Disable Explorer Iris refresh and telemetry");
        t.description = QStringLiteral("Disables Iris client refresh, Explorer startup trace recording, telemetry salt, and Anaheim download promotion.");
        t.iconName = QStringLiteral("folder-open");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Explorer will skip Iris content refresh and startup tracing, disabling Microsoft's promotional banners and telemetry tagging"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"), QStringLiteral("IrisClientRefresh"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"), QStringLiteral("ExplorerStartupTraceRecorded"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"), QStringLiteral("TelemetrySalt"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"), QStringLiteral("ExcludedFromStableAnaheimDownloadPromotionSL"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.explorer.disable_network_search");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Network");
        t.name = QStringLiteral("Disable network device search");
        t.description = QStringLiteral("Sets AutoSearch=0 so File Explorer stops automatically searching for network devices and printers.");
        t.iconName = QStringLiteral("folder-open");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Explorer will no longer auto-discover network devices and printers; you must browse to them manually"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), QStringLiteral("AutoSearch"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.explorer.disable_online_tips");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Privacy");
        t.name = QStringLiteral("Disable online tips");
        t.description = QStringLiteral("Disables new app notifications and search box suggestions that reach out to the internet.");
        t.iconName = QStringLiteral("folder");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Start menu will stop highlighting newly installed apps"),
            QStringLiteral("Search box will no longer show web-based suggestions from Bing"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), QStringLiteral("Start_NotifyNewApps"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Explorer"), QStringLiteral("DisableSearchBoxSuggestions"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.visual.disable_os_animations_extended");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Animations");
        t.name = QStringLiteral("Disable OS animations (extended)");
        t.description = QStringLiteral("Disables additional window animation attributes and DWM animation policies beyond the basic MinAnimate setting, including border padding, max-animate, and DWM animation attribution.");
        t.iconName = QStringLiteral("palette");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Window borders will tighten and maximize/minimize animations will stop"),
            QStringLiteral("DWM animation attribution is disabled, which may change how some GPU-accelerated compositor effects appear"),
        };
        t.registryOps = {
            stringOp(QStringLiteral("HKCU\\Control Panel\\Desktop\\WindowMetrics"), QStringLiteral("PaddedBorderWidth"), QStringLiteral("0"), QStringLiteral("-60")),
            stringOp(QStringLiteral("HKCU\\Control Panel\\Desktop\\WindowMetrics"), QStringLiteral("MaxAnimate"), QStringLiteral("0"), QStringLiteral("1")),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\DWM"), QStringLiteral("DisallowAnimations"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\Dwm"), QStringLiteral("AnimationAttributionEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\Dwm"), QStringLiteral("AnimationAttributionHashingEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\DWM"), QStringLiteral("AnimationAttributionEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\DWM"), QStringLiteral("AnimationAttributionHashingEnabled"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.explorer.disable_preview_handlers");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Explorer");
        t.name = QStringLiteral("Disable preview pane handlers");
        t.description = QStringLiteral("The Explorer preview pane stops rendering file contents.");
        t.iconName = QStringLiteral("eye-off");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Explorer preview pane will stop rendering document, image, and PDF previews"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), QStringLiteral("ShowPreviewHandlers"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.explorer.disable_resource_timers");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Explorer");
        t.name = QStringLiteral("Disable resource-aware timers");
        t.description = QStringLiteral("Disables Explorer resource-aware timers that throttle background operations based on system resource availability.");
        t.iconName = QStringLiteral("clock");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.registryOps = {
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ResourceTimers"), QStringLiteral("EnableResourceAwareTimers"), QStringLiteral("0"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.visual.disable_restore_recalc");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Windows");
        t.name = QStringLiteral("Disable window restore state recalculation");
        t.description = QStringLiteral("Disables the recalculation behavior when restoring previous window states, for faster window restore operations.");
        t.iconName = QStringLiteral("zap");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Windows will skip position recalculation on restore, so apps may reappear at unexpected coordinates when displays change"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Control Panel\\Desktop"), QStringLiteral("RestorePreviousStateRecalcBehavior"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.context.shell_additions");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Context menu");
        t.name = QStringLiteral("Disable shell additions");
        t.description = QStringLiteral("Adds 'Open command window here' and 'Open PowerShell here' entries (with icons) to the folder-background right-click menu.");
        t.iconName = QStringLiteral("folder");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("File Explorer will open to This PC instead of Quick Access by default"),
        };
        t.registryOps = {
            stringOp(QStringLiteral("HKCU\\Software\\Classes\\Directory\\Background\\shell\\cmd"), QStringLiteral("Icon"), QStringLiteral("cmd.exe"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Classes\\Directory\\Background\\shell\\PowerShell"), QStringLiteral("Icon"), QStringLiteral("powershell.exe"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.taskbar.disable_show_desktop");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Taskbar");
        t.name = QStringLiteral("Disable show desktop on far corner");
        t.description = QStringLiteral("Disables the show desktop button on the far right corner of the Windows 11 taskbar.");
        t.iconName = QStringLiteral("list");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Clicking the far-right taskbar corner will no longer minimize all windows; use Win+D instead"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), QStringLiteral("TaskbarSd"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.explorer.disable_slow_link_ui");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Network");
        t.name = QStringLiteral("Disable slow link UI");
        t.description = QStringLiteral("Disables the slow link detection UI that appears when accessing network resources over slow connections.");
        t.iconName = QStringLiteral("folder-open");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Windows will stop showing the slow-connection indicator for offline files when the network link is sluggish"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System"), QStringLiteral("SlowLinkUIEnabled"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.taskbar.disable_snap_groups");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Taskbar");
        t.name = QStringLiteral("Disable Snap Groups in Task View");
        t.description = QStringLiteral("Disables showing snap groups in the taskbar, Alt+Tab, and Task View.");
        t.iconName = QStringLiteral("list");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Snapped window groups will no longer appear as a single entry in Alt+Tab, Task View, and taskbar hover thumbnails"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), QStringLiteral("EnableTaskGroups"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.explorer.disable_snap_flyout");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Snap layouts");
        t.name = QStringLiteral("Disable Snap Layout flyout");
        t.description = QStringLiteral("Stops Win11's snap-layout picker from appearing on maximize-button hover.");
        t.iconName = QStringLiteral("maximize");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Snap layout picker will no longer appear on maximize button hover; Win+Z still works"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), QStringLiteral("EnableSnapAssistFlyout"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.lockscreen.disable_spotlight_content");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Lock screen");
        t.name = QStringLiteral("Disable lock screen Spotlight content");
        t.description = QStringLiteral("Clears Windows Spotlight creative content metadata on the lock screen including image flags, placement IDs, impression tokens, and hotspot image paths for a clean lock screen experience.");
        t.iconName = QStringLiteral("shield");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Spotlight image metadata on the lock screen is cleared, so the daily image tips and Learn more hotspots will no longer appear"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Lock Screen\\Creative"), QStringLiteral("LockImageFlags"), 0u, 1u),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Lock Screen\\Creative"), QStringLiteral("CreativeId"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Lock Screen\\Creative"), QStringLiteral("PortraitAssetPath"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Lock Screen\\Creative"), QStringLiteral("LandscapeAssetPath"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Lock Screen\\Creative"), QStringLiteral("PlacementId"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Lock Screen\\Creative"), QStringLiteral("ImpressionToken"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Lock Screen\\Creative"), QStringLiteral("HotspotImageFolderPath"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Lock Screen\\Creative"), QStringLiteral("CreativeJson"), QStringLiteral("0"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.explorer.disable_store_prompts");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Open With");
        t.name = QStringLiteral("Disable app store prompts");
        t.description = QStringLiteral("Disables 'How do you want to open this file' Store suggestions, new app alerts, and internet-based open-with lookups.");
        t.iconName = QStringLiteral("folder-open");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Opening unknown file types will skip the Store suggestion page in the Open With dialog"),
            QStringLiteral("New-app alerts will no longer be shown when software registers new shell associations"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\Explorer"), QStringLiteral("NoNewAppAlert"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\Explorer"), QStringLiteral("NoUseStoreOpenWith"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\Software\\Policies\\Microsoft\\Windows\\Explorer"), QStringLiteral("NoNewAppAlert"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\Software\\Policies\\Microsoft\\Windows\\Explorer"), QStringLiteral("NoUseStoreOpenWith"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.search.disable_suggestions");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Search & Cortana");
        t.name = QStringLiteral("Disable search box suggestions");
        t.description = QStringLiteral("Disables search box suggestions and link resolution in Windows Search across all policy locations.");
        t.iconName = QStringLiteral("search");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Search box in the taskbar will no longer suggest queries, web completions, or trending topics as you type"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Explorer"), QStringLiteral("DisableSearchBoxSuggestions"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\Explorer"), QStringLiteral("DisableSearchBoxSuggestions"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"), QStringLiteral("NoResolveSearch"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Search"), QStringLiteral("DisableSearchBoxSuggestions"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.taskbar.disable_tablet");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Taskbar");
        t.name = QStringLiteral("Disable tablet-optimized taskbar");
        t.description = QStringLiteral("Disables the tablet-optimized taskbar, auto-hide in tablet mode, thumbnails, and removes Mail/TFL/Copilot pins from taskbar.");
        t.iconName = QStringLiteral("list");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("On 2-in-1 devices the taskbar will no longer auto-expand or auto-hide when detaching the keyboard"),
            QStringLiteral("Default Mail, TFL, and Copilot-PWA pins are removed from the taskbar if they were present"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"), QStringLiteral("TabletPostureTaskbar"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), QStringLiteral("TaskbarAutoHideInTabletMode"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Taskband"), QStringLiteral("NumThumbnails"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Taskband\\AuxilliaryPins"), QStringLiteral("MailPin"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Taskband\\AuxilliaryPins"), QStringLiteral("TFLPin"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Taskband\\AuxilliaryPins"), QStringLiteral("CopilotPWAPin"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.visual.transparency");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Visual effects");
        t.name = QStringLiteral("Disable transparency effects");
        t.description = QStringLiteral("Disables the acrylic and transparency effects in the taskbar, Start menu, and window chrome.");
        t.iconName = QStringLiteral("palette");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Taskbar, Start menu, and Action Center will become fully opaque instead of blurred"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"), QStringLiteral("EnableTransparency"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.theme.dynamic_lighting");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Dynamic Lighting");
        t.name = QStringLiteral("Configure Dynamic Lighting");
        t.description = QStringLiteral("Puts Windows Dynamic Lighting for supported RGB peripherals into a static ambient mode (enables ambient lighting, effect type set to none).");
        t.iconName = QStringLiteral("microphone-off");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Dynamic Lighting effects on supported RGB keyboards and peripherals will be set to a static ambient mode"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Lighting"), QStringLiteral("AmbientLightingEnabled"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Lighting"), QStringLiteral("EffectType"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.taskbar.expandable_and_flashing");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Taskbar");
        t.name = QStringLiteral("Configure taskbar flashing behavior");
        t.description = QStringLiteral("Disables the expandable taskbar feature and configures taskbar button flashing to only flash once for attention.");
        t.iconName = QStringLiteral("list");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Taskbar will not auto-expand to accommodate overflow icons"),
            QStringLiteral("Apps that demand attention will only flash their button once instead of repeatedly"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), QStringLiteral("ExpandableTaskbar"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), QStringLiteral("TaskbarFlashing"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.explorer.advanced_tuning");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Explorer");
        t.name = QStringLiteral("Explorer advanced options");
        t.description = QStringLiteral("Applies a set of advanced Explorer options for a cleaner, faster experience.");
        t.iconName = QStringLiteral("folder");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Each Explorer window will run in a separate process, using slightly more memory"),
            QStringLiteral("Checkbox file selection in Explorer will be turned off"),
            QStringLiteral("Aero Shake (drag a window to minimize others) will stop working"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), QStringLiteral("SeparateProcess"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), QStringLiteral("AutoCheckSelect"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), QStringLiteral("DisallowShaking"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), QStringLiteral("DisableThumbnailCache"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), QStringLiteral("ShowStatusBar"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), QStringLiteral("FullRowSelect"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), QStringLiteral("FolderContentsInfoTip"), 0u, std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\Shell\\Bags\\AllFolders\\Shell"), QStringLiteral("FolderType"), QStringLiteral("NotSpecified"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.explorer.friendly_dates");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Explorer");
        t.name = QStringLiteral("Friendly date display");
        t.description = QStringLiteral("Explorer shows relative dates (Yesterday, Last week) instead of full timestamps.");
        t.iconName = QStringLiteral("calendar-clock");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Explorer Details view will show relative dates like 'Yesterday' instead of exact timestamps"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), QStringLiteral("FriendlyDates"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.gaming.gamedvr_config");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Game DVR");
        t.name = QStringLiteral("Game DVR configuration and hotkey disable");
        t.description = QStringLiteral("Configures Game DVR encoding settings (720p, 4Mbps, 30s buffer) and disables all Game Bar VK hotkeys, reducing background recording overhead.");
        t.iconName = QStringLiteral("video");
        t.impact = Impact::Low;
        t.registryOps = {
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\GameDVR"), QStringLiteral("AudioEncodingBitrate"), QStringLiteral("128000"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\GameDVR"), QStringLiteral("CustomVideoEncodingBitrate"), QStringLiteral("4000000"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\GameDVR"), QStringLiteral("CustomVideoEncodingHeight"), QStringLiteral("720"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\GameDVR"), QStringLiteral("CustomVideoEncodingWidth"), QStringLiteral("1280"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\GameDVR"), QStringLiteral("HistoricalBufferLength"), QStringLiteral("30"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\GameDVR"), QStringLiteral("HistoricalBufferLengthUnit"), QStringLiteral("1"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\GameDVR"), QStringLiteral("VideoEncodingBitrateMode"), QStringLiteral("2"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\GameDVR"), QStringLiteral("VideoEncodingResolutionMode"), QStringLiteral("2"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\GameDVR"), QStringLiteral("VideoEncodingFrameRateMode"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\GameDVR"), QStringLiteral("VKToggleGameBar"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\GameDVR"), QStringLiteral("VKMToggleGameBar"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\GameDVR"), QStringLiteral("VKSaveHistoricalVideo"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\GameDVR"), QStringLiteral("VKMSaveHistoricalVideo"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\GameDVR"), QStringLiteral("VKToggleRecording"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\GameDVR"), QStringLiteral("VKMToggleRecording"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\GameDVR"), QStringLiteral("VKTakeScreenshot"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\GameDVR"), QStringLiteral("VKMTakeScreenshot"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\GameDVR"), QStringLiteral("VKToggleRecordingIndicator"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\GameDVR"), QStringLiteral("VKMToggleRecordingIndicator"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\GameDVR"), QStringLiteral("VKToggleMicrophoneCapture"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\GameDVR"), QStringLiteral("VKMToggleMicrophoneCapture"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\GameDVR"), QStringLiteral("VKToggleCameraCapture"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\GameDVR"), QStringLiteral("VKMToggleCameraCapture"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\GameDVR"), QStringLiteral("VKToggleBroadcast"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\GameDVR"), QStringLiteral("VKMToggleBroadcast"), QStringLiteral("0"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.explorer.hide_onedrive_status");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("OneDrive");
        t.name = QStringLiteral("Hide OneDrive status in navigation pane");
        t.description = QStringLiteral("Hides OneDrive availability/sync status indicators from the File Explorer navigation pane.");
        t.iconName = QStringLiteral("folder-open");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("OneDrive sync state icons (cloud, green check, syncing) will no longer appear next to folders in the navigation pane"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), QStringLiteral("NavPaneShowAllCloudStates"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.explorer.hide_people_bar");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Taskbar");
        t.name = QStringLiteral("Hide People Bar and shoulder tap");
        t.description = QStringLiteral("Hides the People Bar from the taskbar via policy and disables shoulder tap audio notifications.");
        t.iconName = QStringLiteral("folder-open");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("People Bar is hidden from the taskbar by policy and the shoulder-tap notification sound is silenced"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\Start\\HidePeopleBar"), QStringLiteral("value"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\Explorer"), QStringLiteral("HidePeopleBar"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced\\People\\ShoulderTap"), QStringLiteral("ShoulderTapAudio"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.search.immersive_config");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Search & Cortana");
        t.name = QStringLiteral("Disable immersive search features");
        t.description = QStringLiteral("Disables immersive search features including above-lock tips, background app toggle, and location-based search.");
        t.iconName = QStringLiteral("search");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Above-lock search tips and microphone-driven voice search will be disabled"),
            QStringLiteral("Search will not use your location, so nearby-business and place-based results stop working"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Search"), QStringLiteral("HasAboveLockTips"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Search"), QStringLiteral("AnyAboveLockAppsActive"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Search"), QStringLiteral("IsMicrophoneAvailable"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search"), QStringLiteral("BackgroundAppGlobalToggle"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search"), QStringLiteral("AllowSearchToUseLocation"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.notifications.instant_menus");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Menus");
        t.name = QStringLiteral("Instant menus");
        t.description = QStringLiteral("Sets menu show delay to zero for instant pop-up menus.");
        t.iconName = QStringLiteral("zap");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Submenus will pop open instantly, which can feel twitchy when the mouse grazes a menu item"),
        };
        t.registryOps = {
            // 400 ms is the Windows default menu show delay.
            stringOp(QStringLiteral("HKCU\\Control Panel\\Desktop"), QStringLiteral("MenuShowDelay"), QStringLiteral("0"), QStringLiteral("400")),
            stringOp(QStringLiteral("HKCU\\Control Panel\\Mouse"), QStringLiteral("MouseHoverTime"), QStringLiteral("0"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.shell.legacy_flyouts");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("System tray");
        t.name = QStringLiteral("Restore legacy Win32 flyouts");
        t.description = QStringLiteral("Restores the classic Win32 flyout panels for clock, network, volume, and battery indicators in the system tray.");
        t.iconName = QStringLiteral("layout");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.registryOps = {
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ImmersiveShell"), QStringLiteral("UseWin32TrayClockExperience"), QStringLiteral("1"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ImmersiveShell"), QStringLiteral("UseWin32NetworkFlyout"), QStringLiteral("1"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ImmersiveShell"), QStringLiteral("UseWin32VolumeFlyout"), QStringLiteral("1"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ImmersiveShell"), QStringLiteral("UseWin32BatteryFlyout"), QStringLiteral("1"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.explorer.maximize_invoke_prompt");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("File operations");
        t.name = QStringLiteral("Increase multi-invoke prompt limit");
        t.description = QStringLiteral("Increases the threshold for the multi-file open confirmation dialog from 15 to 5000, preventing 'Are you sure you want to open all?' prompts.");
        t.iconName = QStringLiteral("folder-open");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Opening large selections of files at once will no longer prompt for confirmation until 5000 items, so accidental double-clicks can launch thousands of apps"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"), QStringLiteral("MultipleInvokePromptMinimum"), 5000u, 15u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.explorer.memory_checkbox_run");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Run dialog");
        t.name = QStringLiteral("Enable memory space checkbox in run");
        t.description = QStringLiteral("Shows the 'Create this task with separate memory space' checkbox in the Run dialog (Win+R).");
        t.iconName = QStringLiteral("folder-open");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Win+R Run dialog will show a separate-memory-space checkbox used to launch legacy 16-bit apps in their own NTVDM"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"), QStringLiteral("MemCheckBoxInRunDlg"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.explorer.misc_tweaks");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Explorer");
        t.name = QStringLiteral("Explorer miscellaneous tweaks");
        t.description = QStringLiteral("Configures various Explorer settings: disables publishing wizard, MSApp notifications, online tips, hover select, and enables clean shutdown behavior.");
        t.iconName = QStringLiteral("folder-open");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("The legacy Web Publishing Wizard, app-logo channel notifications, and desktop hover-select are all disabled"),
            QStringLiteral("Explorer records clean-shutdown state and stops waiting on roaming profile payloads at logon"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"), QStringLiteral("NoPublishingWizard"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"), QStringLiteral("NoMSAppLogo5ChannelNotify"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"), QStringLiteral("HoverSelectDesktops"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shutdown"), QStringLiteral("CleanShutdown"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer"), QStringLiteral("MachineOobeUpdates"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer"), QStringLiteral("NoWaitOnRoamingPayloads"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.explorer.no_encrypt_on_move");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("File operations");
        t.name = QStringLiteral("Disable encrypt on move");
        t.description = QStringLiteral("Prevents Windows from encrypting files when moving them to an encrypted folder.");
        t.iconName = QStringLiteral("folder-open");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Files moved into an EFS-encrypted folder will stay unencrypted, reducing protection for sensitive data placed there"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"), QStringLiteral("NoEncryptOnMove"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.theme.no_theme_cursor_change");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Themes");
        t.name = QStringLiteral("Prevent theme from changing cursors");
        t.description = QStringLiteral("Prevents Windows themes from overriding custom mouse pointer and desktop icon settings when a new theme is applied.");
        t.iconName = QStringLiteral("palette");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Applying a new theme will no longer override your custom mouse pointer scheme"),
            QStringLiteral("Applying a new theme will no longer override your custom desktop icons"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes"), QStringLiteral("ThemeChangesMousePointers"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes"), QStringLiteral("ThemeChangesDesktopIcons"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.explorer.ntp_config");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Time sync");
        t.name = QStringLiteral("Configure NTP time synchronization");
        t.description = QStringLiteral("Sets the Windows Time client poll interval to 1 day and disables the NTP server provider.");
        t.iconName = QStringLiteral("clock");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.registryOps = {
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\W32Time\\TimeProviders\\NtpServer"), QStringLiteral("Enabled"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\W32Time\\TimeProviders\\NtpClient"), QStringLiteral("SpecialPollInterval"), QStringLiteral("86400"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.qol.numlock_boot");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Keyboard");
        t.name = QStringLiteral("NumLock on at boot");
        t.description = QStringLiteral("Ensures NumLock is enabled at the login screen and on every boot.");
        t.iconName = QStringLiteral("hash");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("NumLock will be forced on at login; laptops without a numpad may see unexpected key behavior"),
        };
        t.registryOps = {
            // InitialKeyboardIndicators is a bitmask. Bit 1 is Num Lock; bit 31
            // (0x80000000) restores the last session's state and is set on a stock
            // install, which is why the default is 2147483648. Windows 11 needs
            // 2147483650 to light Num Lock at sign-in, and that works on Windows 10 too.
            stringOp(QStringLiteral("HKCU\\Control Panel\\Keyboard"), QStringLiteral("InitialKeyboardIndicators"), QStringLiteral("2147483650"), QStringLiteral("2147483648")),
            stringOp(QStringLiteral("HKU\\.DEFAULT\\Control Panel\\Keyboard"), QStringLiteral("InitialKeyboardIndicators"), QStringLiteral("2147483650"), QStringLiteral("2147483648")),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.visual.per_process_dpi");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Display scaling");
        t.name = QStringLiteral("Enable per-process system DPI");
        t.description = QStringLiteral("Enables per-process system DPI awareness via policy for both current user and machine, improving display scaling behavior for legacy applications on high-DPI monitors.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Legacy apps will use per-process DPI awareness, which can look sharper on mixed-DPI setups but may cause some apps to position controls unexpectedly"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\Control Panel\\Desktop"), QStringLiteral("EnablePerProcessSystemDPI"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\Display"), QStringLiteral("EnablePerProcessSystemDPIForProcesses"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Control Panel\\Desktop"), QStringLiteral("EnablePerProcessSystemDPI"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Display"), QStringLiteral("EnablePerProcessSystemDPIForProcesses"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.context.remove_onedrive_explorer");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("OneDrive");
        t.name = QStringLiteral("Remove OneDrive from Explorer navigation");
        t.description = QStringLiteral("Removes OneDrive from the Explorer navigation pane by unpinning its CLSID from the namespace tree.");
        t.iconName = QStringLiteral("download");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("OneDrive entry will be removed from the Explorer navigation pane; files remain accessible via the OneDrive folder"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Classes\\CLSID\\{018D5C66-4533-4307-9B53-224DE2ED1FE6}"), QStringLiteral("System.IsPinnedToNameSpaceTree"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.taskbar.remove_spotlight_icon");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Desktop icons");
        t.name = QStringLiteral("Remove 'learn about this picture' icon");
        t.description = QStringLiteral("Hides the Windows Spotlight 'Learn about this picture' desktop icon.");
        t.iconName = QStringLiteral("photo-off");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("The 'Learn about this picture' Spotlight icon will no longer appear on the desktop"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\HideDesktopIcons\\NewStartPanel"), QStringLiteral("{2cc5ca98-6485-489a-920e-b3e88a6ccce3}"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.explorer.serialize_startup");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Startup");
        t.name = QStringLiteral("Remove Explorer startup delay");
        t.description = QStringLiteral("Sets Explorer startup delay to 0ms for faster Explorer shell initialization during logon.");
        t.iconName = QStringLiteral("folder-open");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Explorer starts without its usual serialize delay, so startup apps launch immediately and may briefly contend for CPU and disk"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Serialize"), QStringLiteral("StartupDelayInMSec"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.taskbar.share_window");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Taskbar");
        t.name = QStringLiteral("Enable share any window from taskbar");
        t.description = QStringLiteral("Enables the ability to share any window directly from the taskbar.");
        t.iconName = QStringLiteral("list");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Right-clicking any app window's taskbar button will expose a Share option for sending content through Windows share targets"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), QStringLiteral("TaskbarSn"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.explorer.show_empty_drives");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Explorer");
        t.name = QStringLiteral("Show empty drives in this PC");
        t.description = QStringLiteral("Keeps empty drives visible in This PC (card readers, optical drives, etc.).");
        t.iconName = QStringLiteral("database");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Empty card readers and optical drives will remain visible in This PC even when no media is inserted"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), QStringLiteral("HideDrivesWithNoMedia"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.explorer.folder_merge_conflicts");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("File operations");
        t.name = QStringLiteral("Show folder merge conflicts");
        t.description = QStringLiteral("Explorer prompts on folder merges instead of silently combining contents.");
        t.iconName = QStringLiteral("alert-circle");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Explorer will prompt before merging folders with the same name instead of combining silently"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), QStringLiteral("HideMergeConflicts"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.desktop.show_thispc_recyclebin");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Desktop icons");
        t.name = QStringLiteral("Show this PC and Recycle Bin on desktop");
        t.description = QStringLiteral("Shows the This PC and Recycle Bin icons on the desktop (for both modern and classic Start layouts).");
        t.iconName = QStringLiteral("house");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("This PC and Recycle Bin icons will appear on the desktop while Network, Control Panel, and User Files icons are hidden"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\HideDesktopIcons\\NewStartPanel"), QStringLiteral("{645FF040-5081-101B-9F08-00AA002F954E}"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\HideDesktopIcons\\NewStartPanel"), QStringLiteral("{20D04FE0-3AEA-1069-A2D8-08002B30309D}"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\HideDesktopIcons\\ClassicStartMenu"), QStringLiteral("{645FF040-5081-101B-9F08-00AA002F954E}"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\HideDesktopIcons\\ClassicStartMenu"), QStringLiteral("{20D04FE0-3AEA-1069-A2D8-08002B30309D}"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.desktop.solid_black_background");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Wallpaper");
        t.name = QStringLiteral("Solid black desktop background");
        t.description = QStringLiteral("Replaces the desktop wallpaper with a pure-black solid color.");
        t.iconName = QStringLiteral("moon");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Desktop wallpaper is replaced with a pure black solid color, overwriting your current wallpaper until the tweak is reverted"),
        };
        t.registryOps = {
            // Absent on a fresh profile, so revert removes it.
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Wallpapers"), QStringLiteral("BackgroundType"), 1u, std::nullopt),
            // WallPaper has no fixed default. The engine snapshots the real value before
            // applying; this empty string is only the fallback when that snapshot is gone.
            stringOp(QStringLiteral("HKCU\\Control Panel\\Desktop"), QStringLiteral("WallPaper"), QStringLiteral(""), QStringLiteral("")),
            // "0 0 0" is the Windows default background color.
            stringOp(QStringLiteral("HKCU\\Control Panel\\Colors"), QStringLiteral("Background"), QStringLiteral("0 0 0"), QStringLiteral("0 0 0")),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.explorer.speedup_remote");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Network");
        t.name = QStringLiteral("Speed up remote computer browsing");
        t.description = QStringLiteral("Disables remote recursive events to speed up browsing network computers in Windows Explorer.");
        t.iconName = QStringLiteral("folder-open");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Explorer stops subscribing to recursive change notifications on network folders, so file lists on remote shares may not auto-refresh after external changes"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"), QStringLiteral("NoRemoteRecursiveEvents"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"), QStringLiteral("NoRemoteRecursiveEvents"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.taskbar.start_menu_tweaks");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Start menu");
        t.name = QStringLiteral("Windows Start menu tweaks");
        t.description = QStringLiteral("Configures Start Menu: hides frequently used apps, recently added apps, and recommended section via PolicyManager and Group Policy.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Start menu hides Most Used, Recently Added, and Recommended sections for all users, leaving only pinned tiles and the all-apps list"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\Start\\HideFrequentlyUsedApps"), QStringLiteral("value"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\Start\\HideRecentlyAddedApps"), QStringLiteral("value"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\Start\\HideRecommendedSection"), QStringLiteral("value"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\Start\\ShowOrHideMostUsedApps"), QStringLiteral("value"), 2u, 0u),
            dwordOp(QStringLiteral("HKLM\\Software\\Policies\\Microsoft\\Windows\\Explorer"), QStringLiteral("ShowOrHideMostUsedApps"), 2u, 0u),
            dwordOp(QStringLiteral("HKLM\\Software\\Policies\\Microsoft\\Windows\\Explorer"), QStringLiteral("NoStartMenuMFUprogramsList"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\Explorer"), QStringLiteral("ShowOrHideMostUsedApps"), 2u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\Explorer"), QStringLiteral("NoStartMenuMFUprogramsList"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("ui.taskbar-end-task");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Taskbar");
        t.name = QStringLiteral("Show 'end task' in taskbar right-click");
        t.description = QStringLiteral("Adds the Win11 End-task developer entry to taskbar context menus.");
        t.iconName = QStringLiteral("square-x");
        t.impact = Impact::Low;
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced\\TaskbarDeveloperSettings"), QStringLiteral("TaskbarEndTask"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.explorer.thumbnail_optimization");
        t.category = QStringLiteral("general");
        t.subCategory = QStringLiteral("Thumbnails");
        t.name = QStringLiteral("Thumbnail cache optimization");
        t.description = QStringLiteral("Disables Explorer thumbnail caching and lowers thumbnail quality to 36. This is a privacy/disk-space choice, not a speed-up: folders with many images or videos regenerate previews on every open and look blockier.");
        t.iconName = QStringLiteral("folder-open");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Explorer stops caching thumbnails, so folders with many images or videos regenerate previews on every open and feel slower to browse"),
            QStringLiteral("Thumbnail quality is lowered from 90 to 36, producing visibly blockier preview images in Explorer"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"), QStringLiteral("NoThumbNailCache"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"), QStringLiteral("NoThumbnailCache"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer"), QStringLiteral("ThumbnailQuality"), 36u, 90u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.taskbar.tweaks");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Taskbar");
        t.name = QStringLiteral("Taskbar advanced tweaks");
        t.description = QStringLiteral("Configures various taskbar settings: disables graph recent items, enables last active click, disables snap assist, and configures extended UI hover time.");
        t.iconName = QStringLiteral("list");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Taskbar stops surfacing recently used documents in jumplists, suppresses new-app highlights, and skips network crawling on the desktop"),
            QStringLiteral("Clicking a grouped taskbar button jumps to the last active window instead of showing the thumbnail list"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\Software\\Policies\\Microsoft\\Windows\\Explorer"), QStringLiteral("DisableGraphRecentItems"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\Explorer"), QStringLiteral("DisableGraphRecentItems"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), QStringLiteral("ExtendedUIHoverTime"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), QStringLiteral("LastActiveClick"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), QStringLiteral("NoNetCrawling"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), QStringLiteral("Start_NotifyNewApps"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.explorer.disable_active_desktop");
        t.category = QStringLiteral("appearance");
        t.name = QStringLiteral("Disable Active Desktop");
        t.description = QStringLiteral("Disables Windows Active Desktop and prevents Active Desktop changes. Also disables slow link UI and cloud download for delete operations.");
        t.iconName = QStringLiteral("folder-open");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Legacy Active Desktop web content on the desktop will no longer render, and web-based wallpapers will stop working"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"), QStringLiteral("ForceActiveDesktopOn"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"), QStringLiteral("NoActiveDesktop"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"), QStringLiteral("NoActiveDesktopChanges"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.explorer.disable_dep");
        t.category = QStringLiteral("general");
        t.name = QStringLiteral("Disable Explorer Data Execution Prevention");
        t.description = QStringLiteral("Disables Data Execution Prevention for Windows Explorer to improve compatibility with older shell extensions.");
        t.iconName = QStringLiteral("shield-off");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Reduces memory protection for the Explorer process."),
        };
        t.registryOps = {
            stringOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\Explorer"), QStringLiteral("NoDataExecutionPrevention"), QStringLiteral("1"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\Software\\Policies\\Microsoft\\Windows\\Explorer"), QStringLiteral("NoDataExecutionPrevention"), QStringLiteral("1"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.explorer.disable_heap_term");
        t.category = QStringLiteral("general");
        t.name = QStringLiteral("Disable heap termination on Explorer corruption");
        t.description = QStringLiteral("Prevents Explorer from terminating when heap corruption is detected, allowing it to recover from minor memory issues.");
        t.iconName = QStringLiteral("shield");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.registryOps = {
            stringOp(QStringLiteral("HKLM\\Software\\Policies\\Microsoft\\Windows\\Explorer"), QStringLiteral("NoHeapTerminationOnCorruption"), QStringLiteral("1"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\Explorer"), QStringLiteral("NoHeapTerminationOnCorruption"), QStringLiteral("1"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.taskbar.end_task_rightclick");
        t.category = QStringLiteral("appearance");
        t.name = QStringLiteral("Enable end task in taskbar right-click");
        t.description = QStringLiteral("Enables the 'End Task' option when right-clicking apps on the taskbar (Developer Settings).");
        t.iconName = QStringLiteral("list");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Right-click menus on taskbar app buttons will show End Task, letting one click forcibly terminate processes without saving"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\DeveloperSettings"), QStringLiteral("TaskbarEndTask"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.visual.font_smoothing");
        t.category = QStringLiteral("appearance");
        t.name = QStringLiteral("ClearType font smoothing");
        t.description = QStringLiteral("Enables ClearType font smoothing for sharper text on LCD displays.");
        t.iconName = QStringLiteral("typography");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Text rendering will switch to ClearType sub-pixel smoothing, which can look colored on some non-LCD displays"),
        };
        t.registryOps = {
            // Both values are the Windows defaults, so this changes nothing on a stock PC.
            // It turns ClearType back on where something else switched it off, and the
            // revert keeps the defaults.
            stringOp(QStringLiteral("HKCU\\Control Panel\\Desktop"), QStringLiteral("FontSmoothing"), QStringLiteral("2"), QStringLiteral("2")),
            dwordOp(QStringLiteral("HKCU\\Control Panel\\Desktop"), QStringLiteral("FontSmoothingType"), 2u, 2u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("app.visual.scroll_inactive_windows");
        t.category = QStringLiteral("appearance");
        t.name = QStringLiteral("Scroll inactive windows");
        t.description = QStringLiteral("Lets the mouse wheel scroll whichever window is under the cursor.");
        t.iconName = QStringLiteral("mouse-pointer");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Mouse wheel will scroll the window under the cursor even when it is not focused"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Control Panel\\Desktop"), QStringLiteral("MouseWheelRouting"), 2u, 0u),
        };
        c.push_back(t);
    }
}

} // namespace vth::tweaks::detail
