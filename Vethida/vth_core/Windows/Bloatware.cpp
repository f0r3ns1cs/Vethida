#include "Bloatware.h"

#include <QStringLiteral>

namespace vth::win::bloatware
{
    QString severityLabel(Severity s) {
        switch (s) {
            case Severity::Recommended: return QStringLiteral("Recommended to remove");
            case Severity::Optional:    return QStringLiteral("Remove if you don't use it");
            case Severity::Risky:       return QStringLiteral("Review first");
        }
        return {};
    }

    const QVector<Entry>& catalog() {
        static const QVector<Entry> kCatalog = {
            {
                QStringLiteral("Microsoft.MicrosoftSolitaireCollection"),
                QStringLiteral("Microsoft Solitaire Collection"),
                QStringLiteral("Microsoft"),
                Severity::Recommended,
                QStringLiteral(
                    "Microsoft's bundled Solitaire game with ads and "
                    "in-app purchases."),
                QStringLiteral(
                    "No effect on system; removes the game and its ad "
                    "loader."),
            },
            {
                QStringLiteral("Microsoft.WindowsFeedbackHub"),
                QStringLiteral("Feedback Hub"),
                QStringLiteral("Microsoft"),
                Severity::Recommended,
                QStringLiteral(
                    "Sends user feedback to Microsoft."),
                QStringLiteral(
                    "You won't be able to send feedback through the Hub. "
                    "Bug reports via other channels are unaffected."),
            },
            {
                QStringLiteral("Microsoft.GetHelp"),
                QStringLiteral("Get Help"),
                QStringLiteral("Microsoft"),
                Severity::Recommended,
                QStringLiteral(
                    "Microsoft support assistant; routes most queries to "
                    "the web."),
                QStringLiteral(
                    "Microsoft online support still works in any browser."),
            },
            {
                QStringLiteral("Microsoft.Getstarted"),
                QStringLiteral("Tips"),
                QStringLiteral("Microsoft"),
                Severity::Recommended,
                QStringLiteral(
                    "Tips & tutorials about Windows."),
                QStringLiteral(
                    "Removes the Tips popups and the Tips app."),
            },
            {
                QStringLiteral("Microsoft.MixedReality.Portal"),
                QStringLiteral("Mixed Reality Portal"),
                QStringLiteral("Microsoft"),
                Severity::Recommended,
                QStringLiteral(
                    "Windows Mixed Reality VR portal - Microsoft has "
                    "deprecated WMR as of Windows 11 24H2."),
                QStringLiteral(
                    "Only matters if you use a (now-unsupported) WMR "
                    "headset."),
            },
            {
                QStringLiteral("Microsoft.Microsoft3DViewer"),
                QStringLiteral("3D Viewer"),
                QStringLiteral("Microsoft"),
                Severity::Recommended,
                QStringLiteral(
                    "Viewer for .glb / .fbx 3D model files."),
                QStringLiteral(
                    "3D files no longer have a default app - use any "
                    "alternative if you actually open them."),
            },
            {
                QStringLiteral("Microsoft.MSPaint"),
                QStringLiteral("Paint 3D"),
                QStringLiteral("Microsoft"),
                Severity::Recommended,
                QStringLiteral(
                    "Paint 3D, which Microsoft has discontinued."),
                QStringLiteral(
                    "The regular Paint app is separate and stays."),
            },
            {
                QStringLiteral("Microsoft.SkypeApp"),
                QStringLiteral("Skype"),
                QStringLiteral("Microsoft"),
                Severity::Recommended,
                QStringLiteral(
                    "Skype, which Microsoft retired in May 2025."),
                QStringLiteral(
                    "Skype no longer works, so nothing is lost."),
            },
            {
                QStringLiteral("Microsoft.YourPhone"),
                QStringLiteral("Phone Link"),
                QStringLiteral("Microsoft"),
                Severity::Optional,
                QStringLiteral(
                    "Phone Link, which shows your phone's messages, calls and "
                    "photos on the PC."),
                QStringLiteral(
                    "Remove it if you don't connect a phone to this PC."),
            },
            {
                QStringLiteral("Microsoft.BingNews"),
                QStringLiteral("News"),
                QStringLiteral("Microsoft"),
                Severity::Recommended,
                QStringLiteral(
                    "Microsoft News, also shown in the Widgets panel."),
                QStringLiteral(
                    "The news section of Widgets stops working."),
            },
            {
                QStringLiteral("Microsoft.BingWeather"),
                QStringLiteral("Weather"),
                QStringLiteral("Microsoft"),
                Severity::Recommended,
                QStringLiteral(
                    "Microsoft Weather, also shown in the Widgets panel."),
                QStringLiteral(
                    "The weather section of Widgets stops working."),
            },

            {
                QStringLiteral("Microsoft.WindowsAlarms"),
                QStringLiteral("Clock"),
                QStringLiteral("Microsoft"),
                Severity::Optional,
                QStringLiteral(
                    "Clock, alarms, timers, and Focus Sessions."),
                QStringLiteral(
                    "If you use alarms or Focus Sessions on Windows, "
                    "leave installed."),
            },
            {
                QStringLiteral("Microsoft.WindowsMaps"),
                QStringLiteral("Maps"),
                QStringLiteral("Microsoft"),
                Severity::Optional,
                QStringLiteral(
                    "Bing Maps client with offline-map support."),
                QStringLiteral(
                    "Most users use online maps in a browser."),
            },
            {
                QStringLiteral("Microsoft.WindowsSoundRecorder"),
                QStringLiteral("Sound Recorder"),
                QStringLiteral("Microsoft"),
                Severity::Optional,
                QStringLiteral(
                    "Audio recording app."),
                QStringLiteral(
                    "Remove if you don't record audio; alternatives are "
                    "abundant."),
            },
            {
                QStringLiteral("Microsoft.ZuneMusic"),
                QStringLiteral("Media Player (Groove Music)"),
                QStringLiteral("Microsoft"),
                Severity::Optional,
                QStringLiteral(
                    "Media Player, the default music app."),
                QStringLiteral(
                    "Pick another music player first, or audio files will "
                    "have nothing to open them."),
            },
            {
                QStringLiteral("Microsoft.ZuneVideo"),
                QStringLiteral("Movies & TV"),
                QStringLiteral("Microsoft"),
                Severity::Optional,
                QStringLiteral(
                    "Movies & TV, for videos bought from the Microsoft Store."),
                QStringLiteral(
                    "Other video players and browsers are not affected."),
            },
            {
                QStringLiteral("Microsoft.People"),
                QStringLiteral("People"),
                QStringLiteral("Microsoft"),
                Severity::Optional,
                QStringLiteral(
                    "Contacts app, ties into Mail / Calendar."),
                QStringLiteral(
                    "Mail / Calendar can read contacts directly without "
                    "the People app."),
            },
            {
                QStringLiteral("Microsoft.Office.OneNote"),
                QStringLiteral("OneNote (Store)"),
                QStringLiteral("Microsoft"),
                Severity::Optional,
                QStringLiteral(
                    "The older Store version of OneNote."),
                QStringLiteral(
                    "OneNote from Microsoft 365 is a separate app and is not "
                    "affected."),
            },

            {
                QStringLiteral("Microsoft.XboxGamingOverlay"),
                QStringLiteral("Xbox Game Bar"),
                QStringLiteral("Microsoft"),
                Severity::Recommended,
                QStringLiteral(
                    "The Xbox Game Bar overlay (Win+G), which runs in the "
                    "background while you play."),
                QStringLiteral(
                    "Win+G stops opening the overlay. Game Pass and your "
                    "games are not affected."),
            },
            {
                QStringLiteral("Microsoft.XboxGameOverlay"),
                QStringLiteral("Xbox Game Bar Plug-in"),
                QStringLiteral("Microsoft"),
                Severity::Recommended,
                QStringLiteral(
                    "Xbox Game Bar overlay plug-in companion."),
                QStringLiteral(
                    "Removed alongside Xbox Game Bar."),
            },
            {
                QStringLiteral("Microsoft.Xbox.TCUI"),
                QStringLiteral("Xbox TCUI"),
                QStringLiteral("Microsoft"),
                Severity::Recommended,
                QStringLiteral(
                    "Pop-ups some older Xbox games use for friends and "
                    "achievements."),
                QStringLiteral(
                    "A few older Xbox games may not show friends or "
                    "achievements."),
            },
            {
                QStringLiteral("Microsoft.XboxIdentityProvider"),
                QStringLiteral("Xbox Identity Provider"),
                QStringLiteral("Microsoft"),
                Severity::Risky,
                QStringLiteral(
                    "Authenticates the user with Xbox Live for first-party "
                    "Microsoft games (Halo, Forza, Minecraft, Sea of Thieves)."),
                QStringLiteral(
                    "Microsoft-published games launched from PC Game Pass "
                    "or the Microsoft Store will fail to log in. Keep this "
                    "if you use any of those titles."),
            },
            {
                QStringLiteral("Microsoft.XboxSpeechToTextOverlay"),
                QStringLiteral("Xbox Speech To Text Overlay"),
                QStringLiteral("Microsoft"),
                Severity::Recommended,
                QStringLiteral(
                    "Speech-to-text overlay companion to Game Bar."),
                QStringLiteral(
                    "Speech-to-text within Game Bar disabled."),
            },
            {
                QStringLiteral("Microsoft.GamingApp"),
                QStringLiteral("Xbox (PC Game Pass)"),
                QStringLiteral("Microsoft"),
                Severity::Optional,
                QStringLiteral(
                    "Xbox PC client - required for PC Game Pass and "
                    "first-party Microsoft titles installed via the Store."),
                QStringLiteral(
                    "Remove only if you do not use PC Game Pass or buy "
                    "first-party games from the Microsoft Store."),
            },

            {
                QStringLiteral("Microsoft.OutlookForWindows"),
                QStringLiteral("Outlook (new)"),
                QStringLiteral("Microsoft"),
                Severity::Recommended,
                QStringLiteral(
                    "The new Outlook, preinstalled since Windows 11 24H2."),
                QStringLiteral(
                    "Classic Outlook and Microsoft 365 Outlook are separate "
                    "apps and are not affected."),
            },
            {
                QStringLiteral("Microsoft.MicrosoftOfficeHub"),
                QStringLiteral("Get Office"),
                QStringLiteral("Microsoft"),
                Severity::Recommended,
                QStringLiteral(
                    "A tile that advertises Microsoft 365."),
                QStringLiteral(
                    "Office apps you already have are not affected."),
            },
            {
                QStringLiteral("Microsoft.OfficeHub"),
                QStringLiteral("My Office (legacy)"),
                QStringLiteral("Microsoft"),
                Severity::Recommended,
                QStringLiteral(
                    "An older tile that advertises Office."),
                QStringLiteral(
                    "Office apps you already have are not affected."),
            },
            {
                QStringLiteral("Microsoft.Todos"),
                QStringLiteral("Microsoft To Do"),
                QStringLiteral("Microsoft"),
                Severity::Optional,
                QStringLiteral(
                    "Microsoft To Do, a task list."),
                QStringLiteral(
                    "Remove it if you don't use it."),
            },
            {
                QStringLiteral("Microsoft.MicrosoftStickyNotes"),
                QStringLiteral("Sticky Notes"),
                QStringLiteral("Microsoft"),
                Severity::Optional,
                QStringLiteral(
                    "Desktop sticky notes; integrates with OneDrive."),
                QStringLiteral(
                    "Remove if you don't use Sticky Notes."),
            },
            {
                QStringLiteral("Microsoft.PowerAutomateDesktop"),
                QStringLiteral("Power Automate"),
                QStringLiteral("Microsoft"),
                Severity::Recommended,
                QStringLiteral(
                    "Power Automate, for building automated office workflows."),
                QStringLiteral(
                    "Keep it if you build Power Automate flows."),
            },
            {
                QStringLiteral("Microsoft.MicrosoftJournal"),
                QStringLiteral("Journal"),
                QStringLiteral("Microsoft"),
                Severity::Optional,
                QStringLiteral(
                    "Pen-input note-taking app for tablets / 2-in-1s."),
                QStringLiteral(
                    "Tablet pen experiences using other apps unaffected."),
            },
            {
                QStringLiteral("Microsoft.Whiteboard"),
                QStringLiteral("Whiteboard"),
                QStringLiteral("Microsoft"),
                Severity::Optional,
                QStringLiteral(
                    "Microsoft Whiteboard - collaborative canvas, mostly "
                    "used inside Teams meetings."),
                QStringLiteral(
                    "Standalone use disappears; Teams Whiteboard works "
                    "without this app."),
            },
            {
                QStringLiteral("Microsoft.OneConnect"),
                QStringLiteral("Mobile Plans"),
                QStringLiteral("Microsoft"),
                Severity::Recommended,
                QStringLiteral(
                    "Mobile Plans, for buying cellular data. Only useful on a "
                    "PC with a SIM card."),
                QStringLiteral(
                    "Wi-Fi and wired connections are not affected."),
            },
            {
                QStringLiteral("MicrosoftCorporationII.QuickAssist"),
                QStringLiteral("Quick Assist"),
                QStringLiteral("Microsoft"),
                Severity::Optional,
                QStringLiteral(
                    "Microsoft remote-assistance tool - same purpose as "
                    "TeamViewer / Chrome Remote Desktop."),
                QStringLiteral(
                    "Remove if you don't accept incoming Quick Assist sessions."),
            },
            {
                QStringLiteral("MicrosoftWindows.CrossDevice"),
                QStringLiteral("Cross Device Service"),
                QStringLiteral("Microsoft"),
                Severity::Optional,
                QStringLiteral(
                    "The background service behind Phone Link and other "
                    "phone-to-PC features."),
                QStringLiteral(
                    "Phone Link stops working. Remove Phone Link first."),
            },
            {
                QStringLiteral("Clipchamp.Clipchamp"),
                QStringLiteral("Clipchamp"),
                QStringLiteral("Microsoft"),
                Severity::Recommended,
                QStringLiteral(
                    "Clipchamp, Microsoft's video editor, which pushes a paid "
                    "plan."),
                QStringLiteral(
                    "Keep it if you edit videos with it."),
            },
            {
                QStringLiteral("Microsoft.MSTeams"),
                QStringLiteral("Microsoft Teams (consumer)"),
                QStringLiteral("Microsoft"),
                Severity::Optional,
                QStringLiteral(
                    "Teams for personal use, preinstalled with Windows."),
                QStringLiteral(
                    "Work and school Teams is a separate app and is not "
                    "affected."),
            },

            {
                QStringLiteral("Microsoft.Copilot"),
                QStringLiteral("Copilot"),
                QStringLiteral("Microsoft"),
                Severity::Recommended,
                QStringLiteral(
                    "The Copilot app."),
                QStringLiteral(
                    "The Copilot button on the taskbar is a separate setting."),
            },
            {
                QStringLiteral("MicrosoftWindows.Client.WebExperience"),
                QStringLiteral("Widgets"),
                QStringLiteral("Microsoft"),
                Severity::Recommended,
                QStringLiteral(
                    "The Widgets panel and its news feed."),
                QStringLiteral(
                    "The Widgets button disappears from the taskbar."),
            },

            {
                QStringLiteral("SpotifyAB.SpotifyMusic"),
                QStringLiteral("Spotify"),
                QStringLiteral("Spotify AB"),
                Severity::Optional,
                QStringLiteral(
                    "Spotify, if it came preinstalled or you added it from "
                    "the Store."),
                QStringLiteral(
                    "Remove it only if you don't use Spotify here. "
                    "spotify.com works in a browser."),
            },
            {
                QStringLiteral("4DF9E0F8.Netflix"),
                QStringLiteral("Netflix"),
                QStringLiteral("Netflix, Inc."),
                Severity::Optional,
                QStringLiteral(
                    "The Netflix app."),
                QStringLiteral(
                    "netflix.com works in any browser."),
            },
            {
                QStringLiteral("BytedancePte.Ltd.TikTok"),
                QStringLiteral("TikTok"),
                QStringLiteral("ByteDance"),
                Severity::Optional,
                QStringLiteral(
                    "The TikTok app."),
                QStringLiteral(
                    "tiktok.com works in any browser."),
            },
            {
                QStringLiteral("Disney.37853FC22B2CE"),
                QStringLiteral("Disney+"),
                QStringLiteral("The Walt Disney Company"),
                Severity::Optional,
                QStringLiteral(
                    "The Disney+ app."),
                QStringLiteral(
                    "disneyplus.com works in any browser."),
            },
            {
                QStringLiteral("Facebook.317180B0BB486"),
                QStringLiteral("Facebook"),
                QStringLiteral("Facebook"),
                Severity::Optional,
                QStringLiteral(
                    "The Facebook app."),
                QStringLiteral(
                    "facebook.com works in any browser."),
            },
            {
                QStringLiteral("Microsoft.Advertising.Xaml"),
                QStringLiteral("Microsoft Advertising SDK"),
                QStringLiteral("Microsoft"),
                Severity::Recommended,
                QStringLiteral(
                    "An ad component some free Store games use."),
                QStringLiteral(
                    "Free games that show ads may show a blank ad space."),
            },

            {
                QStringLiteral("Microsoft.WindowsCalculator"),
                QStringLiteral("Calculator"),
                QStringLiteral("Microsoft"),
                Severity::Optional,
                QStringLiteral(
                    "Calculator."),
                QStringLiteral(
                    "Nothing opens Calculator any more. Keep it if you use "
                    "it."),
            },
            {
                QStringLiteral("Microsoft.ScreenSketch"),
                QStringLiteral("Snipping Tool"),
                QStringLiteral("Microsoft"),
                Severity::Optional,
                QStringLiteral(
                    "Snipping Tool, which takes screenshots (Win+Shift+S)."),
                QStringLiteral(
                    "Win+Shift+S stops working. Print Screen still works."),
            },
            {
                QStringLiteral("Microsoft.Windows.Photos"),
                QStringLiteral("Photos"),
                QStringLiteral("Microsoft"),
                Severity::Optional,
                QStringLiteral(
                    "Photos, the default picture viewer."),
                QStringLiteral(
                    "Pick another picture viewer first, or images will have "
                    "nothing to open them."),
            },
            {
                QStringLiteral("Microsoft.WindowsCommunicationsApps"),
                QStringLiteral("Mail and Calendar"),
                QStringLiteral("Microsoft"),
                Severity::Optional,
                QStringLiteral(
                    "The old Mail and Calendar apps, which Microsoft is "
                    "replacing with the new Outlook."),
                QStringLiteral(
                    "Keep them if your email accounts are set up there."),
            },
            {
                QStringLiteral("Microsoft.OutlookForBusiness"),
                QStringLiteral("Outlook for Business"),
                QStringLiteral("Microsoft"),
                Severity::Optional,
                QStringLiteral(
                    "A placeholder for the new Outlook for work accounts."),
                QStringLiteral(
                    "Classic Outlook is not affected."),
            },
            {
                QStringLiteral("MicrosoftCorporationII.MicrosoftFamily"),
                QStringLiteral("Microsoft Family"),
                QStringLiteral("Microsoft"),
                Severity::Recommended,
                QStringLiteral(
                    "Family Safety dashboard. Useful if you administer "
                    "child accounts; otherwise unused."),
                QStringLiteral(
                    "Existing family-account restrictions stay enforced "
                    "by Microsoft account policy; the local UI for "
                    "managing them disappears."),
            },

            {
                QStringLiteral("Microsoft.WindowsCamera"),
                QStringLiteral("Camera"),
                QStringLiteral("Microsoft"),
                Severity::Risky,
                QStringLiteral(
                    "Camera, the built-in webcam app."),
                QStringLiteral(
                    "Apps that use the webcam still work, but nothing opens "
                    "the camera on its own."),
            },
            {
                QStringLiteral("Microsoft.WindowsStore"),
                QStringLiteral("Microsoft Store"),
                QStringLiteral("Microsoft"),
                Severity::Risky,
                QStringLiteral(
                    "The Microsoft Store."),
                QStringLiteral(
                    "Store apps stop updating and getting the Store back is "
                    "hard. Most people should keep it."),
            },
        };
        return kCatalog;
    }

    const Entry* findByName(const QString& packageName) {
        for (const auto& e : catalog()) {
            if (e.name.compare(packageName, Qt::CaseInsensitive) == 0) return &e;
        }
        return nullptr;
    }
}
