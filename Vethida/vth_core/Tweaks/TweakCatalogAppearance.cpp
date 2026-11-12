#include "TweakEngine.h"

#include <QtCore/QByteArray>

// Appearance and shell. Mostly HKCU values that apply after Explorer restarts or
// on the next sign-in. UserPreferencesMask packs many effects into one binary
// value, so the discrete values below are used instead.

namespace vth::tweaks::detail {

    namespace {
    const QString kDesktop  = QStringLiteral("HKCU\\Control Panel\\Desktop");
    const QString kAdvanced = QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced");

    CommandOp regOp(const QStringList& applyArgs, const QStringList& revertArgs) {
        CommandOp op;
        op.program = QStringLiteral("reg");
        op.applyArgs = applyArgs;
        op.revertArgs = revertArgs;
        return op;
    }
    }

    void appendAppearance(QVector<TweakSpec>& c) {
        // These values also appear in the bundled Explorer, taskbar and clock tweaks
        // with the same applied and revert values, so they need no exclusion pairs.
        {
            TweakSpec t;
            t.id = QStringLiteral("appear.show-file-extensions");
            t.category = QStringLiteral("appearance");
            t.subCategory = QStringLiteral("Explorer");
            t.name = QStringLiteral("Show file extensions");
            t.description = QStringLiteral(
                "Reveals the .exe / .docx / .pdf suffix on every file in Explorer. "
                "Windows hides them by default - a social-engineering hazard for "
                "fake-extension malware. Recommended on for everyone.");
            t.iconName = QStringLiteral("folder-cog");
            t.impact = Impact::Low;
            t.registryOps = {
                dwordOp(kAdvanced, QStringLiteral("HideFileExt"), 0, 1),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("appear.show-hidden-files");
            t.category = QStringLiteral("appearance");
            t.subCategory = QStringLiteral("Explorer");
            t.name = QStringLiteral("Show hidden files");
            t.description = QStringLiteral(
                "Lists files and folders flagged as hidden in Explorer. "
                "System-protected files stay controlled by the separate "
                "super-hidden setting.");
            t.iconName = QStringLiteral("folder-cog");
            t.impact = Impact::Low;
            t.registryOps = {
                dwordOp(kAdvanced, QStringLiteral("Hidden"), 1, 2),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("appear.taskbar-clock-seconds");
            t.category = QStringLiteral("appearance");
            t.subCategory = QStringLiteral("Taskbar");
            t.name = QStringLiteral("Show seconds in taskbar clock");
            t.description = QStringLiteral(
                "Adds a seconds field to the taskbar clock. Slightly more power "
                "draw because the clock redraws every second; applies after the "
                "next Explorer restart.");
            t.iconName = QStringLiteral("clock");
            t.impact = Impact::Low;
            t.registryOps = {
                dwordOp(kAdvanced, QStringLiteral("ShowSecondsInSystemClock"), 1, 0),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("appear.hide-widgets-button");
            t.category = QStringLiteral("appearance");
            t.subCategory = QStringLiteral("Taskbar");
            t.name = QStringLiteral("Hide widgets button");
            t.description = QStringLiteral(
                "Removes the Widgets icon from the Windows 11 taskbar without "
                "disabling the underlying service.");
            t.iconName = QStringLiteral("layout-panel-top");
            t.impact = Impact::Low;
            t.registryOps = {
                dwordOp(kAdvanced, QStringLiteral("TaskbarDa"), 0, 1),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("appear.dark-mode");
            t.category = QStringLiteral("appearance");
            t.subCategory = QStringLiteral("Themes");
            t.name = QStringLiteral("Enable dark mode");
            t.description = QStringLiteral("Switches apps and the Windows shell to the dark theme.");
            t.iconName = QStringLiteral("moon");
            t.impact = Impact::Low;
            const QString p = QStringLiteral(
                "HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize");
            t.registryOps = {
                dwordOp(p, QStringLiteral("AppsUseLightTheme"), 0, 1),
                dwordOp(p, QStringLiteral("SystemUsesLightTheme"), 0, 1),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("appear.visual-effects");
            t.category = QStringLiteral("appearance");
            t.subCategory = QStringLiteral("Visual effects");
            t.name = QStringLiteral("Snappier visual effects");
            t.description = QStringLiteral(
                "Removes menu-open delay, window-drag contents and minimize/maximize animations "
                "while keeping ClearType. Sets Visual Effects to Custom.");
            t.iconName = QStringLiteral("sparkles");
            t.impact = Impact::Low;
            t.registryOps = {
                stringOp(kDesktop, QStringLiteral("MenuShowDelay"), QStringLiteral("0"), QStringLiteral("400")),
                stringOp(kDesktop, QStringLiteral("DragFullWindows"), QStringLiteral("0"), QStringLiteral("1")),
                stringOp(QStringLiteral("HKCU\\Control Panel\\Desktop\\WindowMetrics"),
                        QStringLiteral("MinAnimate"), QStringLiteral("0"), QStringLiteral("1")),
                dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VisualEffects"),
                        QStringLiteral("VisualFXSetting"), 3, 0),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("appear.explorer");
            t.category = QStringLiteral("appearance");
            t.subCategory = QStringLiteral("Explorer");
            t.name = QStringLiteral("Explorer tweaks");
            t.description = QStringLiteral(
                "Shows file extensions, hidden and system files, opens to This PC, and turns off "
                "recent-item tracking, balloon/info tips and label eye-candy.");
            t.iconName = QStringLiteral("folder-cog");
            t.impact = Impact::Low;
            t.registryOps = {
                dwordOp(kAdvanced, QStringLiteral("HideFileExt"), 0, 1),
                dwordOp(kAdvanced, QStringLiteral("Hidden"), 1, 2),
                dwordOp(kAdvanced, QStringLiteral("ShowSuperHidden"), 1, 0),
                dwordOp(kAdvanced, QStringLiteral("LaunchTo"), 1, 2),
                dwordOp(kAdvanced, QStringLiteral("NavPaneShowAllFolders"), 1, 0),
                dwordOp(kAdvanced, QStringLiteral("Start_TrackProgs"), 0, 1),
                dwordOp(kAdvanced, QStringLiteral("Start_TrackDocs"), 0, 1),
                dwordOp(kAdvanced, QStringLiteral("EnableBalloonTips"), 0, 1),
                dwordOp(kAdvanced, QStringLiteral("ShowInfoTip"), 0, 1),
                dwordOp(kAdvanced, QStringLiteral("ShowCompColor"), 0, 1),
                dwordOp(kAdvanced, QStringLiteral("FolderContentsInfoTip"), 0, 1),
                dwordOp(kAdvanced, QStringLiteral("SharingWizardOn"), 0, 1),
                dwordOp(kAdvanced, QStringLiteral("AutoCheckSelect"), 0, 1),
                dwordOp(kAdvanced, QStringLiteral("ListviewAlphaSelect"), 0, 1),
                dwordOp(kAdvanced, QStringLiteral("ListviewShadow"), 0, 1),
                dwordOp(kAdvanced, QStringLiteral("DisablePreviewDesktop"), 1, 0),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("appear.taskbar");
            t.category = QStringLiteral("appearance");
            t.subCategory = QStringLiteral("Taskbar");
            t.name = QStringLiteral("Declutter taskbar");
            t.description = QStringLiteral(
                "Hides Task View, Widgets, Chat, Copilot and the search box from the taskbar.");
            t.iconName = QStringLiteral("layout-panel-top");
            t.impact = Impact::Low;
            t.registryOps = {
                dwordOp(kAdvanced, QStringLiteral("ShowTaskViewButton"), 0, 1),
                dwordOp(kAdvanced, QStringLiteral("TaskbarDa"), 0, 1),
                dwordOp(kAdvanced, QStringLiteral("TaskbarMn"), 0, 1),
                dwordOp(kAdvanced, QStringLiteral("ShowCopilotButton"), 0, 1),
                dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Search"),
                        QStringLiteral("SearchboxTaskbarMode"), 0, 3),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("appear.notifications-off");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Notifications");
            t.name = QStringLiteral("Disable toast notifications");
            t.description = QStringLiteral("Silences all Windows toast/banner notifications.");
            t.iconName = QStringLiteral("bell-off");
            t.impact = Impact::Low;
            t.warnings = { QStringLiteral("You'll stop seeing all notification banners, including from apps you use.") };
            const QString push = QStringLiteral(
                "HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\PushNotifications");
            const QString nset = QStringLiteral(
                "HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Notifications\\Settings");
            t.registryOps = {
                dwordOp(push, QStringLiteral("ToastEnabled"), 0, 1),
                dwordOp(push, QStringLiteral("NoToastApplicationNotification"), 1, 0),
                dwordOp(push, QStringLiteral("NoCloudApplicationNotification"), 1, 0),
                dwordOp(nset, QStringLiteral("NOC_GLOBAL_SETTING_ALLOW_TOASTS_ABOVE_LOCK"), 0, 1),
                dwordOp(nset, QStringLiteral("NOC_GLOBAL_SETTING_ALLOW_CRITICAL_TOASTS_ABOVE_LOCK"), 0, 1),
                dwordOp(nset, QStringLiteral("NOC_GLOBAL_SETTING_TOASTS_ENABLED"), 0, 1),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("appear.classic-context-menu");
            t.category = QStringLiteral("appearance");
            t.subCategory = QStringLiteral("Context menu");
            t.name = QStringLiteral("Classic context menu");
            t.description = QStringLiteral(
                "Restores the full Windows 10-style right-click menu on Windows 11, skipping the "
                "\"Show more options\" truncation.");
            t.iconName = QStringLiteral("mouse-pointer-click");
            t.impact = Impact::Low;
            t.requiresReboot = true;
            const QString key = QStringLiteral(
                "HKCU\\SOFTWARE\\Classes\\CLSID\\{86ca1aa0-34aa-4e8b-a509-50c905bae2a2}");
            t.commandOps = {
                regOp({ QStringLiteral("add"), key + QStringLiteral("\\InprocServer32"),
                        QStringLiteral("/f"), QStringLiteral("/ve") },
                    { QStringLiteral("delete"), key, QStringLiteral("/f") }),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("appear.wallpaper-quality");
            t.category = QStringLiteral("appearance");
            t.subCategory = QStringLiteral("Wallpaper");
            t.name = QStringLiteral("Max wallpaper quality");
            t.description = QStringLiteral(
                "Stops Windows re-compressing the desktop wallpaper to 85% JPEG quality.");
            t.iconName = QStringLiteral("image");
            t.impact = Impact::Low;
            t.registryOps = {
                dwordOp(kDesktop, QStringLiteral("JPEGImportQuality"), 100, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("appear.legacy-shell");
            t.category = QStringLiteral("appearance");
            t.subCategory = QStringLiteral("Legacy shell");
            t.name = QStringLiteral("Legacy Alt-Tab and flyouts");
            t.description = QStringLiteral("Restores the classic small-thumbnail Alt-Tab switcher (AltTabSettings). Also attempts to bring back the legacy Win32 volume/brightness flyout, but that key (EnableMtcUvc) is ignored on current Windows 11 builds, where the modern Quick Settings flyout returns regardless.");
            t.iconName = QStringLiteral("history");
            t.impact = Impact::Low;
            t.concern = QStringLiteral(
                "preference + outdated: classic Alt-Tab is purely taste, and EnableMtcUvc=0 is "
                "ignored on recent Windows 11 builds (the modern Quick Settings flyout returns).");
            t.registryOps = {
                dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer"),
                        QStringLiteral("AltTabSettings"), 1, std::nullopt),
                dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\MTCUVC"),
                        QStringLiteral("EnableMtcUvc"), 0, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("appear.cdrom-autorun-off");
            t.category = QStringLiteral("advanced");
            t.subCategory = QStringLiteral("AutoRun & AutoPlay");
            t.name = QStringLiteral("Disable CD/DVD AutoRun");
            t.description = QStringLiteral("Stops optical media from auto-running on insert.");
            t.iconName = QStringLiteral("disc");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            const QString p = QStringLiteral(
                "HKLM\\SYSTEM\\CurrentControlSet\\Services\\cdrom\\Parameters");
            t.registryOps = {
                dwordOp(p, QStringLiteral("AutoRun"), 0, 1),
                dwordOp(p, QStringLiteral("AutoRunAlwaysDisable"), 1, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("appear.f1-help-off");
            t.category = QStringLiteral("advanced");
            t.subCategory = QStringLiteral("Keyboard");
            t.name = QStringLiteral("Disable F1 help");
            t.description = QStringLiteral("Stops the F1 key from launching the Windows Help viewer.");
            t.iconName = QStringLiteral("circle-help");
            t.impact = Impact::Low;
            const QString key = QStringLiteral(
                "HKCU\\SOFTWARE\\Classes\\Typelib\\{8cec5860-07a1-11d9-b15e-000d56bfe6ee}");
            t.commandOps = {
                regOp({ QStringLiteral("add"), key + QStringLiteral("\\1.0\\0\\win64"),
                        QStringLiteral("/f"), QStringLiteral("/ve"),
                        QStringLiteral("/t"), QStringLiteral("REG_SZ"), QStringLiteral("/d"), QString() },
                    { QStringLiteral("delete"), key, QStringLiteral("/f") }),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("appear.action-center-off");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Notifications");
            t.name = QStringLiteral("Disable Action Center");
            t.description = QStringLiteral(
                "Removes the Action Center flyout, disables the legacy balloon-notification "
                "fallback, and hides feature-advertisement and system-tray promotion balloons.");
            t.iconName = QStringLiteral("bell-off");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.warnings = { QStringLiteral("Removes the notification/quick-actions flyout entirely, not just individual toasts.") };
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ImmersiveShell"),
                        QStringLiteral("UseActionCenterExperience"), 0, std::nullopt),
                dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Explorer"),
                        QStringLiteral("DisableNotificationCenter"), 1, std::nullopt),
                dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Explorer"),
                        QStringLiteral("EnableLegacyBalloonNotifications"), 0, std::nullopt),
                dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Explorer"),
                        QStringLiteral("NoBalloonFeatureAdvertisements"), 1, std::nullopt),
                dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Explorer"),
                        QStringLiteral("NoSystraySystemPromotion"), 1, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("appear.best-perf-effects-mask");
            t.category = QStringLiteral("appearance");
            t.subCategory = QStringLiteral("Visual effects");
            t.name = QStringLiteral("Best-performance visual effects (binary mask)");
            t.description = QStringLiteral(
                "Writes the packed \"adjust for best performance\" UserPreferencesMask "
                "binary value directly, disabling the window/menu fade and slide "
                "animations in one shot.");
            t.iconName = QStringLiteral("sparkles");
            t.impact = Impact::Low;
            t.concern = QStringLiteral(
                "overlaps with appear.visual-effects (VisualFXSetting); this writes the "
                "same intent as a packed binary blob instead of discrete values.");
            RegistryOp m;
            m.path = QStringLiteral("HKCU\\Control Panel\\Desktop");
            m.name = QStringLiteral("UserPreferencesMask");
            m.kind = vth::services::RegistryValueKind::Binary;
            m.appliedValue = QByteArray::fromHex(QByteArrayLiteral("5A12038010000000"));
            t.registryOps = { m };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("appear.classic-flyouts-and-clock");
            t.category = QStringLiteral("appearance");
            t.subCategory = QStringLiteral("Legacy shell");
            t.name = QStringLiteral("Classic flyouts and clock");
            t.description = QStringLiteral("Shows seconds in the taskbar clock and switches to a 24-hour time format -- these still work. The legacy Win32 clock/battery/network flyout keys and the small-taskbar-icons setting it also writes are ignored on current Windows 11 builds and have no effect there.");
            t.iconName = QStringLiteral("history");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            const QString shell = QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ImmersiveShell");
            const QString intl = QStringLiteral("HKCU\\Control Panel\\International");
            t.registryOps = {
                dwordOp(shell, QStringLiteral("UseWin32TrayClockExperience"), 1, 0),
                dwordOp(shell, QStringLiteral("UseWin32BatteryFlyout"), 1, 0),
                dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Control Panel\\Settings\\Network"),
                        QStringLiteral("ReplaceVan"), 2, 0),
                dwordOp(kAdvanced, QStringLiteral("ShowSecondsInSystemClock"), 1, 0),
                dwordOp(kAdvanced, QStringLiteral("TaskbarSmallIcons"), 1, 0),
                dwordOp(kAdvanced, QStringLiteral("NoSystraySystemPromotion"), 1, 0),
                stringOp(intl, QStringLiteral("sTimeFormat"), QStringLiteral("HH:mm:ss"), QStringLiteral("h:mm:ss tt")),
                stringOp(intl, QStringLiteral("sShortTime"), QStringLiteral("HH:mm"), QStringLiteral("h:mm tt")),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("appear.explorer-behavior-extra");
            t.category = QStringLiteral("appearance");
            t.subCategory = QStringLiteral("Explorer");
            t.name = QStringLiteral("Explorer and shell behavior extra");
            t.description = QStringLiteral("A large bundle of shell/Explorer policy and preference values: turns off first-logon, taskbar and Explorer animations, autoplay, feeds/news widgets, People band, boot startup sound, DWM thumbnail hibernation and cursor visualizations; left-aligns and disables taskbar label grouping; sets ClearType; and disables file thumbnails and the thumbnail cache entirely (so Explorer shows generic icons instead of image previews). The thumbnail changes are a noticeable usability trade-off.");
            t.iconName = QStringLiteral("folder-cog");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            const QString pexp = QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer");
            QVector<RegistryOp> o;
            o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"),
                                QStringLiteral("EnableFirstLogonAnimation"), 0, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System"),
                                QStringLiteral("DisableAcrylicBackgroundOnLogon"), 1, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System"),
                                QStringLiteral("DisableLockScreenAppNotifications"), 1, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer"),
                                QStringLiteral("HubMode"), 1, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer"),
                                QStringLiteral("ShowFrequent"), 0, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer"),
                                QStringLiteral("ShowRecent"), 0, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer"),
                                QStringLiteral("HoverSelectDesktops"), 0, std::nullopt));
            o.push_back(stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Control Panel\\Desktop"),
                                 QStringLiteral("ScreenSaveActive"), QStringLiteral("0"), std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Control Panel\\Desktop"),
                                QStringLiteral("EnablePerProcessSystemDPI"), 0, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\FlightedFeatures"),
                                QStringLiteral("ImmersiveContextMenu"), 1, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Authentication\\LogonUI\\BootAnimation"),
                                QStringLiteral("DisableStartupSound"), 1, std::nullopt));
            o.push_back(dwordOp(pexp, QStringLiteral("DisableThumbnails"), 1, std::nullopt));
            o.push_back(dwordOp(pexp, QStringLiteral("NoLowDiskSpaceChecks"), 1, std::nullopt));
            o.push_back(dwordOp(pexp, QStringLiteral("LinkResolveIgnoreLinkInfo"), 1, std::nullopt));
            o.push_back(dwordOp(pexp, QStringLiteral("NoResolveSearch"), 1, std::nullopt));
            o.push_back(dwordOp(pexp, QStringLiteral("NoResolveTrack"), 1, std::nullopt));
            o.push_back(dwordOp(pexp, QStringLiteral("NoInternetOpenWith"), 1, std::nullopt));
            o.push_back(dwordOp(pexp, QStringLiteral("HideSCAMeetNow"), 1, std::nullopt));
            o.push_back(dwordOp(pexp, QStringLiteral("NoThumbnailCache"), 1, std::nullopt));
            o.push_back(dwordOp(pexp, QStringLiteral("DisableThumbnailCache"), 1, std::nullopt));
            o.push_back(dwordOp(pexp, QStringLiteral("NoInstrumentation"), 1, std::nullopt));
            o.push_back(dwordOp(pexp, QStringLiteral("EnableFeeds"), 0, std::nullopt));
            o.push_back(dwordOp(pexp, QStringLiteral("NoAutoTrayNotify"), 1, std::nullopt));
            o.push_back(dwordOp(pexp, QStringLiteral("TaskbarNoNotification"), 1, std::nullopt));
            o.push_back(dwordOp(pexp, QStringLiteral("NoTrayItemsDisplay"), 1, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\PenWorkspace"),
                                QStringLiteral("PenWorkspaceAppSuggestionsEnabled"), 0, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Dsh"),
                                QStringLiteral("AllowNewsAndInterests"), 0, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Feeds"),
                                QStringLiteral("EnableFeeds"), 0, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKCU\\Control Panel\\Desktop"),
                                QStringLiteral("DockMoving"), 0, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKCU\\Control Panel\\Desktop"),
                                QStringLiteral("PaintDesktopVersion"), 1, std::nullopt));
            o.push_back(stringOp(QStringLiteral("HKCU\\Control Panel\\Desktop"),
                                 QStringLiteral("FontSmoothing"), QStringLiteral("2"), std::nullopt));
            o.push_back(dwordOp(kAdvanced, QStringLiteral("TaskbarBadges"), 0, std::nullopt));
            o.push_back(dwordOp(kAdvanced, QStringLiteral("TaskbarAl"), 0, std::nullopt));
            o.push_back(dwordOp(kAdvanced, QStringLiteral("TaskbarGlomLevel"), 2, std::nullopt));
            o.push_back(dwordOp(kAdvanced, QStringLiteral("IconsOnly"), 1, std::nullopt));
            o.push_back(dwordOp(kAdvanced, QStringLiteral("DisallowShaking"), 1, std::nullopt));
            o.push_back(dwordOp(kAdvanced, QStringLiteral("ShowSyncProviderNotifications"), 0, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKCU\\Control Panel\\Cursors"),
                                QStringLiteral("ContactVisualization"), 0, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKCU\\Control Panel\\Cursors"),
                                QStringLiteral("GestureVisualization"), 0, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\AutoplayHandlers"),
                                QStringLiteral("DisableAutoplay"), 1, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Feeds"),
                                QStringLiteral("ShellFeedsTaskbarViewMode"), 2, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Feeds"),
                                QStringLiteral("IsFeedsAvailable"), 0, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Feeds"),
                                QStringLiteral("ShellFeedsTaskbarOpenOnHover"), 0, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced\\People"),
                                QStringLiteral("PeopleBand"), 0, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Applets\\Paint\\Settings"),
                                QStringLiteral("DisableModernPaintBootstrap"), 1, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"),
                                QStringLiteral("TaskbarAnimations"), 0, std::nullopt));
            o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\DWM"),
                                QStringLiteral("AlwaysHibernateThumbnails"), 0, std::nullopt));
            t.registryOps = o;
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("appear.responsiveness");
            t.category = QStringLiteral("advanced");
            t.name = QStringLiteral("Foreground and shutdown responsiveness");
            t.description = QStringLiteral(
                "Removes the 200-second foreground-lock so newly opened windows get focus "
                "immediately, and shortens hung/kill-app timeouts so shutdown doesn't stall.");
            t.iconName = QStringLiteral("timer");
            t.impact = Impact::Low;
            t.concern = QStringLiteral(
                "ForegroundLockTimeout=0 is the real win and is safe. Lowering "
                "WaitToKillAppTimeout/HungAppTimeout and AutoEndTasks=1 can cut off a slow-saving "
                "app on shutdown - small data-loss risk for apps that save on exit.");
            t.registryOps = {
                stringOp(kDesktop, QStringLiteral("WaitToKillAppTimeout"), QStringLiteral("2000"), QStringLiteral("20000")),
                stringOp(kDesktop, QStringLiteral("HungAppTimeout"), QStringLiteral("1000"), QStringLiteral("5000")),
                stringOp(kDesktop, QStringLiteral("AutoEndTasks"), QStringLiteral("1"), QStringLiteral("0")),
                dwordOp(kDesktop, QStringLiteral("ForegroundLockTimeout"), 0, 0x30d40),
                dwordOp(kDesktop, QStringLiteral("ForegroundFlashCount"), 0, 3),
                dwordOp(kDesktop, QStringLiteral("LowLevelHooksTimeout"), 1000, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("appear.camera-led-off");
            t.category = QStringLiteral("advanced");
            t.name = QStringLiteral("Disable camera LED");
            t.description = QStringLiteral(
                "Writes the OEM \"no physical camera LED\" hint to hide the webcam in-use light.");
            t.iconName = QStringLiteral("camera-off");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "snake-oil + privacy-hostile: this OEM key is honored by almost no hardware, "
                "and its only effect when honored is to hide the indicator that the camera is "
                "recording. No performance value; included only for source completeness.");
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\OEM\\Device\\Capture"),
                        QStringLiteral("NoPhysicalCameraLED"), 1, std::nullopt),
            };
            c.push_back(t);
        }
    }
}
