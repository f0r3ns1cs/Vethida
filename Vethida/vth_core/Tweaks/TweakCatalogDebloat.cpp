#include "TweakEngine.h"

// Debloat items the ETW, services and privacy builders don't cover: app package
// removal, Edge and OneDrive, third-party updaters, unused drivers and
// background apps.
//
// Uninstalling an app package or OneDrive is one-way: reversible is false and
// the revert is a best-effort reinstall, not a guaranteed restore.

namespace vth::tweaks::detail {

    namespace {
        CommandOp psOp(const QString& applyScript, const QString& revertScript) {
            CommandOp op;
            op.program = QStringLiteral("powershell");
            const QStringList base = {
                QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"),
                QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"),
                QStringLiteral("-Command"),
            };
            op.applyArgs = base;
            op.applyArgs << applyScript;
            op.revertArgs = base;
            op.revertArgs << revertScript;
            return op;
        }
    }

    void appendDebloat(QVector<TweakSpec>& c) {
        {
            TweakSpec t;
            t.id = QStringLiteral("debloat.remove-uwp-apps");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("UWP apps");
            t.name = QStringLiteral("Remove preinstalled apps");
            t.description = QStringLiteral("Uninstalls a fixed list of inbox UWP apps for all users and removes their provisioned copies so new accounts don't get them back. Covers Bing weather/news/finance/sports, 3D Builder/Viewer/Print3D, Solitaire, Skype, consumer Teams, Your Phone, Clipchamp, Zune Music/Video, Get Help, Getstarted, Feedback Hub and Mixed Reality Portal, but the list also pulls the Xbox app companion (XboxApp, Xbox.TCUI, XboxIdentityProvider), Camera, Paint, Snipping Tool, Sticky Notes, OneNote/Office Hub, People, Maps, Alarms and Sound Recorder. The load-bearing Store/Game Pass packages (Store, GamingApp, GamingServices, XboxGamingOverlay) are intentionally left in place.");
            t.iconName = QStringLiteral("trash-2");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.reversible = false;
            t.warnings = {
                QStringLiteral("Removed provisioned packages can only be restored from the "
                            "Microsoft Store, not by toggling this off."),
            };
            t.concern = QStringLiteral(
                "the verbatim source list removes Microsoft.XboxApp, Microsoft.Xbox.TCUI and "
                "Microsoft.XboxIdentityProvider, which Game Pass / the Xbox app depend on. Kept "
                "as-is per the source; the truly load-bearing Store/GamePass packages "
                "(WindowsStore, XboxGamingOverlay, GamingApp, GamingServices, StorePurchaseApp) "
                "are NOT in the list.");
            const QString list = QStringLiteral(
                "'Microsoft.3DBuilder','Microsoft.BingWeather','Microsoft.BingNews',"
                "'Microsoft.BingFinance','Microsoft.BingSports','Microsoft.BingTravel',"
                "'Microsoft.CommsPhone','Microsoft.ConnectivityStore','Microsoft.GetHelp',"
                "'Microsoft.Getstarted','Microsoft.Messaging','Microsoft.Microsoft3DViewer',"
                "'Microsoft.MicrosoftOfficeHub','Microsoft.MicrosoftSolitaireCollection',"
                "'Microsoft.MicrosoftStickyNotes','Microsoft.MixedReality.Portal',"
                "'Microsoft.MSPaint','Microsoft.Office.OneNote','Microsoft.OneConnect',"
                "'Microsoft.People','Microsoft.Print3D','Microsoft.ScreenSketch',"
                "'Microsoft.SkypeApp','Microsoft.StorePurchaseApp','Microsoft.Wallet',"
                "'Microsoft.WindowsAlarms','Microsoft.WindowsCamera',"
                "'Microsoft.windowscommunicationsapps','Microsoft.WindowsFeedbackHub',"
                "'Microsoft.WindowsMaps','Microsoft.WindowsSoundRecorder','Microsoft.Xbox.TCUI',"
                "'Microsoft.XboxApp','Microsoft.XboxGameOverlay','Microsoft.XboxIdentityProvider',"
                "'Microsoft.XboxSpeechToTextOverlay','Microsoft.YourPhone','Microsoft.ZuneMusic',"
                "'Microsoft.ZuneVideo','Microsoft.Todos','Clipchamp.Clipchamp',"
                "'Microsoft.549981C3F5F10','Microsoft.PowerAutomateDesktop','MicrosoftTeams'");
            const QString apply = QStringLiteral(
                "$ErrorActionPreference='SilentlyContinue';"
                "foreach($p in @(%1)){"
                "Get-AppxPackage -AllUsers \"*$p*\"|Remove-AppxPackage -AllUsers;"
                "Get-AppxProvisionedPackage -Online|Where-Object{$_.DisplayName -like \"*$p*\"}|"
                "Remove-AppxProvisionedPackage -Online}").arg(list);
            const QString revert = QStringLiteral(
                "$ErrorActionPreference='SilentlyContinue';"
                "Get-AppxPackage -AllUsers|ForEach-Object{"
                "Add-AppxPackage -DisableDevelopmentMode -Register "
                "\"$($_.InstallLocation)\\AppXManifest.xml\"}");
            t.commandOps = { psOp(apply, revert) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("debloat.disable-edge");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Edge");
            t.name = QStringLiteral("Disable Microsoft Edge");
            t.description = QStringLiteral(
                "Redirects msedge.exe through an Image-File-Execution-Options debugger so the "
                "browser can't launch, while leaving the WebView2 runtime intact.");
            t.iconName = QStringLiteral("ban");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.warnings = {
                QStringLiteral("Apps that open links in Edge specifically (some Office/Outlook "
                            "flows) will fail to open them. Set a different default browser first."),
            };
            t.concern = QStringLiteral(
                "dubious: the common IFEO Debugger value is the bare taskkill path, which "
                "IFEO prepends to the launch line and does not reliably terminate Edge. Using a "
                "no-op debugger that simply prevents launch is the dependable form, used here.");
            t.registryOps = {
                stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\"
                                        "Image File Execution Options\\msedge.exe"),
                        QStringLiteral("Debugger"),
                        QStringLiteral("%SystemRoot%\\System32\\systray.exe"), std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("debloat.edge-full-removal");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Edge");
            t.name = QStringLiteral("Strip Edge updater and shortcuts");
            t.description = QStringLiteral(
                "The destructive Edge cleanup the IFEO toggle leaves behind: stops and disables "
                "the EdgeUpdate services and scheduled tasks, deletes the EdgeUpdate directory, "
                "removes Edge desktop/Start/taskbar shortcuts, and drops Edge from the file-type "
                "\"Open with\" lists for web/document extensions.");
            t.iconName = QStringLiteral("trash-2");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.reversible = false;
            t.warnings = {
                QStringLiteral("One-way: the deleted EdgeUpdate directory, shortcuts and "
                            "association entries are not restored on revert (only the services "
                            "and tasks are re-enabled). WebView2 is left intact."),
            };
            const QString apply = QStringLiteral(
                "$ErrorActionPreference='SilentlyContinue';"
                "taskkill /f /im msedge.exe;taskkill /f /im MicrosoftEdgeUpdate.exe;"
                "foreach($s in 'edgeupdate','edgeupdatem','MicrosoftEdgeElevationService'){"
                "sc.exe stop $s|Out-Null;sc.exe config $s start= disabled|Out-Null}"
                "Get-ScheduledTask -TaskName 'MicrosoftEdgeUpdate*'|Disable-ScheduledTask;"
                "Remove-Item 'C:\\Program Files (x86)\\Microsoft\\EdgeUpdate' -Recurse -Force;"
                "$locs=@(\"$env:PUBLIC\\Desktop\",\"$env:USERPROFILE\\Desktop\","
                "\"$env:APPDATA\\Microsoft\\Windows\\Start Menu\\Programs\","
                "\"$env:ProgramData\\Microsoft\\Windows\\Start Menu\\Programs\","
                "\"$env:APPDATA\\Microsoft\\Internet Explorer\\Quick Launch\\User Pinned\\TaskBar\");"
                "foreach($l in $locs){Get-ChildItem $l -Recurse -Filter '*Edge*.lnk' "
                "-ErrorAction SilentlyContinue|Remove-Item -Force}"
                "$exts='.htm','.html','.mht','.mhtml','.pdf','.svg','.xml','.xht','.xhtml','.webp';"
                "foreach($e in $exts){$k=\"HKCU:\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\"
                "Explorer\\FileExts\\$e\\OpenWithProgids\";if(Test-Path $k){"
                "(Get-Item $k).Property|Where-Object{$_ -like 'MSEdge*' -or $_ -like 'AppXEdge*'}|"
                "ForEach-Object{Remove-ItemProperty $k -Name $_ -Force}}}");
            const QString revert = QStringLiteral(
                "$ErrorActionPreference='SilentlyContinue';"
                "foreach($s in 'edgeupdate','edgeupdatem','MicrosoftEdgeElevationService'){"
                "sc.exe config $s start= demand|Out-Null}"
                "Get-ScheduledTask -TaskName 'MicrosoftEdgeUpdate*'|Enable-ScheduledTask");
            t.commandOps = { psOp(apply, revert) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("debloat.remove-onedrive");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("OneDrive");
            t.name = QStringLiteral("Remove OneDrive");
            t.description = QStringLiteral(
                "Runs the OneDrive uninstaller and removes its File Explorer namespace entry.");
            t.iconName = QStringLiteral("cloud-off");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.reversible = false;
            t.warnings = { QStringLiteral("Sign out / sync any pending files first.") };
            const QString apply = QStringLiteral(
                "$ErrorActionPreference='SilentlyContinue';"
                "$s=\"$env:SystemRoot\\SysWOW64\\OneDriveSetup.exe\";"
                "if(-not(Test-Path $s)){$s=\"$env:SystemRoot\\System32\\OneDriveSetup.exe\"}"
                "Start-Process $s '/uninstall' -Wait;"
                "Remove-Item 'HKCU:\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\"
                "Desktop\\NameSpace\\{018D5C66-4533-4307-9B53-224DE2ED1FE6}' -Force");
            const QString revert = QStringLiteral(
                "$ErrorActionPreference='SilentlyContinue';"
                "$s=\"$env:SystemRoot\\SysWOW64\\OneDriveSetup.exe\";"
                "if(-not(Test-Path $s)){$s=\"$env:SystemRoot\\System32\\OneDriveSetup.exe\"}"
                "Start-Process $s -Wait");
            t.commandOps = { psOp(apply, revert) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("debloat.edge-remove-completely");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Edge");
            t.name = QStringLiteral("Uninstall Microsoft Edge completely");
            t.description = QStringLiteral(
                "Fully removes the Edge browser, not just its updater. Runs Edge's own "
                "system-level uninstaller, disables the three Edge services, strips the HKLM "
                "uninstall / StartMenuInternet / RegisteredApplications entries and the MSEdgeHTM "
                "\"open with\" associations, then deletes the Edge, EdgeCore, EdgeUpdate and "
                "EdgeWebView directories under both Program Files roots. This goes further than "
                "\"Strip Edge Updater & Shortcuts\", which deliberately leaves the browser binary "
                "and WebView2 in place.");
            t.iconName = QStringLiteral("trash-2");
            t.impact = Impact::High;
            t.requiresAdmin = true;
            t.reversible = false;
            t.warnings = {
                QStringLiteral("One-way. Edge is uninstalled and its directories deleted; "
                            "reverting only re-enables the services. Reinstalling Edge requires "
                            "Microsoft's installer."),
                QStringLiteral("Removes the WebView2 runtime directory. Apps that embed WebView2 "
                            "(Office add-ins, Teams, many Electron-adjacent apps) can break until "
                            "the runtime is reinstalled."),
                QStringLiteral("Set a different default browser before applying, or web links "
                            "will have no handler."),
            };
            t.concern = QStringLiteral(
                "destructive and explicitly requested: deletes install directories outright. "
                "Windows servicing may reinstall Edge on a future feature update.");
            const QString apply = QStringLiteral(
                "$ErrorActionPreference='SilentlyContinue';"
                "foreach($p in 'msedge','MicrosoftEdgeUpdate','msedgewebview2'){"
                "Stop-Process -Name $p -Force}"
                "foreach($b in \"${env:ProgramFiles(x86)}\\Microsoft\\Edge\\Application\","
                "\"$env:ProgramFiles\\Microsoft\\Edge\\Application\"){"
                "$s=Get-ChildItem $b -Filter setup.exe -Recurse -EA SilentlyContinue|"
                "Select-Object -First 1;"
                "if($s){Start-Process $s.FullName "
                "'--uninstall --system-level --verbose-logging --force-uninstall' -Wait}}"
                "foreach($sv in 'edgeupdate','edgeupdatem','MicrosoftEdgeElevationService'){"
                "sc.exe stop $sv|Out-Null;sc.exe config $sv start= disabled|Out-Null}"
                "Remove-ItemProperty 'HKLM:\\SOFTWARE\\WOW6432Node\\RegisteredApplications' "
                "-Name 'Microsoft Edge' -Force;"
                "foreach($e in '.htm','.html','.mht','.mhtml','.pdf','.shtml','.svg','.webp',"
                "'.xht','.xhtml','.xml'){"
                "Remove-ItemProperty \"HKLM:\\SOFTWARE\\Classes\\$e\\OpenWithProgIds\" "
                "-Name 'MSEdgeHTM' -Force}"
                "foreach($k in "
                "'HKLM:\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Microsoft Edge',"
                "'HKLM:\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Microsoft Edge Update',"
                "'HKLM:\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Microsoft EdgeWebView',"
                "'HKLM:\\SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Microsoft Edge',"
                "'HKLM:\\SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Microsoft Edge Update',"
                "'HKLM:\\SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Microsoft EdgeWebView',"
                "'HKLM:\\SOFTWARE\\Clients\\StartMenuInternet\\Microsoft Edge',"
                "'HKLM:\\SOFTWARE\\WOW6432Node\\Clients\\StartMenuInternet\\Microsoft Edge',"
                "'HKLM:\\SOFTWARE\\Microsoft\\Edge','HKLM:\\SOFTWARE\\WOW6432Node\\Microsoft\\Edge',"
                "'HKLM:\\SOFTWARE\\WOW6432Node\\Microsoft\\EdgeUpdate',"
                "'HKLM:\\SOFTWARE\\Microsoft\\Internet Explorer\\EdgeIntegration',"
                "'HKLM:\\SOFTWARE\\WOW6432Node\\Microsoft\\Internet Explorer\\EdgeIntegration',"
                "'HKLM:\\SOFTWARE\\Microsoft\\Active Setup\\Installed Components\\{9459C573-B17A-45AE-9F64-1857B5D58CEE}'"
                "){Remove-Item $k -Recurse -Force}"
                "foreach($r in \"$env:ProgramFiles\\Microsoft\",\"${env:ProgramFiles(x86)}\\Microsoft\"){"
                "foreach($d in 'Edge','EdgeCore','EdgeUpdate','EdgeWebView','Temp'){"
                "Remove-Item (Join-Path $r $d) -Recurse -Force}}");
            const QString revert = QStringLiteral(
                "$ErrorActionPreference='SilentlyContinue';"
                "foreach($sv in 'edgeupdate','edgeupdatem','MicrosoftEdgeElevationService'){"
                "sc.exe config $sv start= demand|Out-Null}");
            t.commandOps = { psOp(apply, revert) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("debloat.onedrive-purge-leftovers");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("OneDrive");
            t.name = QStringLiteral("Purge OneDrive leftovers");
            t.description = QStringLiteral(
                "Deletes what the OneDrive uninstaller leaves behind: the per-user OneDrive "
                "folder, the C:\\OneDriveTemp staging folder, the local OneDrive app data and "
                "the remaining Explorer namespace entries. Run after \"Remove OneDrive\".");
            t.iconName = QStringLiteral("cloud-off");
            t.impact = Impact::High;
            t.requiresAdmin = true;
            t.reversible = false;
            t.warnings = {
                QStringLiteral("One-way and destructive: deletes the local OneDrive folder and "
                            "everything still in it. Anything not fully synced to the cloud is "
                            "lost. Verify sync status before applying."),
            };
            t.concern = QStringLiteral(
                "destructive and explicitly requested. The source's own version targeted a "
                "literal \"%USERNAME%\" path that never expanded, so it silently deleted nothing; "
                "this resolves the real per-user profile paths instead.");
            const QString apply = QStringLiteral(
                "$ErrorActionPreference='SilentlyContinue';"
                "Stop-Process -Name OneDrive -Force;"
                "foreach($d in \"$env:USERPROFILE\\OneDrive\",'C:\\OneDriveTemp',"
                "\"$env:LOCALAPPDATA\\Microsoft\\OneDrive\","
                "\"$env:PROGRAMDATA\\Microsoft OneDrive\"){Remove-Item $d -Recurse -Force}"
                // HKCR: is not a default PowerShell drive - target the per-user
                // Classes hive (which HKCR merges) so these actually delete.
                "foreach($k in 'HKCU:\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\"
                "Desktop\\NameSpace\\{018D5C66-4533-4307-9B53-224DE2ED1FE6}',"
                "'HKCU:\\SOFTWARE\\Classes\\CLSID\\{018D5C66-4533-4307-9B53-224DE2ED1FE6}',"
                "'HKCU:\\SOFTWARE\\Classes\\Wow6432Node\\CLSID\\{018D5C66-4533-4307-9B53-224DE2ED1FE6}',"
                "'HKLM:\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\"
                "Desktop\\NameSpace\\{018D5C66-4533-4307-9B53-224DE2ED1FE6}'"
                "){Remove-Item $k -Recurse -Force}");
            const QString revert = QStringLiteral(
                "$ErrorActionPreference='SilentlyContinue';"
                "Write-Output 'OneDrive leftovers were deleted; nothing to restore.'");
            t.commandOps = { psOp(apply, revert) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("debloat.disable-thirdparty-updaters");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Services");
            t.name = QStringLiteral("Disable third-party updater services");
            t.description = QStringLiteral("Sets a list of vendor background services to disabled so those apps stop auto-updating. Covers Adobe, Google/Chrome, Brave, Mozilla, Dropbox, Spotify, Steam, Epic, EA/Origin and peripheral suites (Razer, SteelSeries, Logitech, ASUS, Gigabyte, MSI). Note the list is broader than pure updaters: NVDisplay.ContainerLocalSystem is the NVIDIA Display Container that runs the NVIDIA Control Panel (disabling it prevents the Control Panel from opening), and the Steam and Epic Online Services entries affect more than updates. Revert restores services to demand-start, not their original start type.");
            t.iconName = QStringLiteral("refresh-cw-off");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.warnings = {
                QStringLiteral("Those apps will no longer auto-update; update them manually."),
            };
            t.concern = QStringLiteral(
                "revert restores each service to demand-start, not its captured original start "
                "type. Missing services are skipped silently.");
            const QString svc = QStringLiteral(
                "'AdobeARMservice','AdobeUpdateService','edgeupdate','edgeupdatem',"
                "'GoogleChromeElevationService','gupdate','gupdatem','MozillaMaintenance',"
                "'brave','bravem','DropboxUpdate','Spotify','Steam Client Service',"
                "'EpicOnlineServices','OriginWebHelperService','NvTelemetryContainer',"
                "'NVDisplay.ContainerLocalSystem','RazerSynapseUpdater','SteelSeriesGGUpdater',"
                "'LogitechGHubUpdater','ASUSPushService','GigabyteSensorPushService','MSIRGBService'");
            t.commandOps = {
                psOp(QStringLiteral("foreach($s in @(%1)){sc.exe config \"$s\" start= disabled "
                                    "2>$null}").arg(svc),
                    QStringLiteral("foreach($s in @(%1)){sc.exe config \"$s\" start= demand "
                                    "2>$null}").arg(svc)),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("debloat.background-apps-off");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Background apps");
            t.name = QStringLiteral("Disable background apps");
            t.description = QStringLiteral(
                "Stops UWP apps from running tasks in the background and disables the text-input "
                "host preload.");
            t.iconName = QStringLiteral("app-window");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.registryOps = {
                dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\"
                                    "BackgroundAccessApplications"),
                        QStringLiteral("GlobalUserDisabled"), 1, 0),
                dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Search"),
                        QStringLiteral("BackgroundAppGlobalToggle"), 0, 1),
                dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\AppPrivacy"),
                        QStringLiteral("LetAppsRunInBackground"), 2, 0),
                dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\input"),
                        QStringLiteral("IsInputAppPreloadEnabled"), 0, 1),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("debloat.disable-unused-drivers");
            t.category = QStringLiteral("debloating");
            t.name = QStringLiteral("Disable unused device drivers");
            t.description = QStringLiteral(
                "Disables in-box drivers for USB-C connector management, Xbox controllers and "
                "Bluetooth that an unattended desktop typically never uses.");
            t.iconName = QStringLiteral("plug-zap");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("This DISABLES Bluetooth and Xbox controller support. Skip it if "
                            "you use either."),
            };
            t.concern = QStringLiteral(
                "risky/situational: BTHUSB/BthEnum/BthMini disable Bluetooth entirely; "
                "XboxGip/xinputhid disable Xbox controllers. Only useful on a machine that uses "
                "neither. Revert sets demand-start, which the OS re-enables on next device arrival.");
            const QString drv = QStringLiteral(
                "'UcmUcsiCx0101','UcmCx0101','UcmTcpciCx0101','VerifierExt','XboxComposite',"
                "'XboxGip','xinputhid','BTHUSB','BthEnum','BthMini','HidBth',"
                "'Microsoft_Bluetooth_AvrcpTransport'");
            t.commandOps = {
                psOp(QStringLiteral("foreach($d in @(%1)){sc.exe config \"$d\" start= disabled "
                                    "2>$null}").arg(drv),
                    QStringLiteral("foreach($d in @(%1)){sc.exe config \"$d\" start= demand "
                                    "2>$null}").arg(drv)),
            };
            c.push_back(t);
        }
    }

}
