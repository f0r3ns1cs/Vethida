#include "TweakEngine.h"

namespace vth::tweaks::detail
{
    namespace
    {
        // A policy value the machine ships without: applied on apply, deleted on revert.
        RegistryOp d(const QString& path, const QString& name, quint32 v) {
            return dwordOp(path, name, v, std::nullopt);
        }
        // A string value the machine ships without.
        RegistryOp s(const QString& path, const QString& name, const QString& v) {
            return stringOp(path, name, v, std::nullopt);
        }
    }

    void appendOperatorDebloat(QVector<TweakSpec>& c) {

        // Only what priv.telemetry-off and op.privacy.telemetry-feedback don't write.
        {
            TweakSpec t;
            t.id = QStringLiteral("priv.telemetry-extended");
            t.category = QStringLiteral("privacy");
            t.subCategory = QStringLiteral("Diagnostic data");
            t.name = QStringLiteral("Reduce diagnostic data further");
            t.description = QStringLiteral(
                "Extends the diagnostic-data opt-out: turns off desktop analytics, "
                "update-compliance and cloud processing, caps log and crash-dump "
                "collection, clears the diagnostic upload endpoints, and stops the "
                "compatibility appraiser and the event-transcript recorder.");
            t.iconName = QStringLiteral("eye-off");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "complements the live priv.telemetry-off (which owns the six core DataCollection "
                "values AllowTelemetry/MaxTelemetryAllowed/AllowDeviceNameInTelemetry/"
                "DoNotShowFeedbackNotifications/AllowCommercialDataPipeline/DisableOneSettingsDownloads) "
                "with the remaining diagnostic-data writes; no shared value name.");
            const QString dc = QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\DataCollection");
            const QString sp = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorPort");
            t.registryOps = {
                d(dc, QStringLiteral("LimitDiagnosticLogCollection"), 1),
                d(dc, QStringLiteral("DisableTelemetryOptInSettingsUx"), 1),
                d(dc, QStringLiteral("DisableTelemetryOptInChangeNotification"), 1),
                d(dc, QStringLiteral("AllowDesktopAnalyticsProcessing"), 0),
                d(dc, QStringLiteral("AllowUpdateComplianceProcessing"), 0),
                d(dc, QStringLiteral("AllowWUfBCloudProcessing"), 0),
                d(dc, QStringLiteral("DisableDeviceDelete"), 1),
                d(dc, QStringLiteral("EnableExtendedBooksTelemetry"), 0),
                d(dc, QStringLiteral("LimitEnhancedDiagnosticDataWindowsAnalytics"), 0),
                d(dc, QStringLiteral("LimitDumpCollection"), 1),
                d(dc, QStringLiteral("MicrosoftEdgeDataOptIn"), 0),
                s(dc, QStringLiteral("TelemetryProxyServer"), QString()),
                s(dc, QStringLiteral("ConfigureMicrosoft365UploadEndpoint"), QString()),
                d(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\DataCollection"),
                  QStringLiteral("AllowTelemetry"), 0),
                d(sp, QStringLiteral("TelemetryPerformanceEnabled"), 0),
                d(sp, QStringLiteral("TelemetryErrorDataEnabled"), 0),
                d(sp, QStringLiteral("TelemetryDeviceHealthEnabled"), 0),
                d(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\TelemetryController"),
                  QStringLiteral("RunsBlocked"), 1),
                d(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\WMI\\AutoLogger\\AutoLogger-Diagtrack-Listener"),
                  QStringLiteral("Start"), 0),
                d(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Diagnostics\\DiagTrack"),
                  QStringLiteral("DiagTrackAuthorization"), 7),
                s(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"),
                  QStringLiteral("POWERSHELL_TELEMETRY_OPTOUT"), QStringLiteral("1")),
                d(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Error Reporting"),
                  QStringLiteral("AutoApproveOSDumps"), 0),
                d(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Error Reporting"),
                  QStringLiteral("DontSendAdditionalData"), 1),
            };
            c.push_back(t);
        }

        // op.privacy.app-access owns location, notifications, diagnostics and account info.
        {
            TweakSpec t;
            t.id = QStringLiteral("priv.app-sensor-consent-deny");
            t.category = QStringLiteral("privacy");
            t.subCategory = QStringLiteral("App permissions");
            t.name = QStringLiteral("Deny apps access to your data and sensors");
            t.description = QStringLiteral(
                "Denies apps access to your camera, contacts, calendar, call history, "
                "email, messages, tasks, your pictures/documents/videos libraries and "
                "the wider file system, and closes the global sensor gate. Grant any "
                "back per app in Settings if something you use needs it.");
            t.iconName = QStringLiteral("shield-off");
            t.impact = Impact::Medium;
            t.requiresAdmin = false;
            t.warnings = {
                QStringLiteral("Apps that need the camera, microphone or your files are blocked until you allow them in Settings."),
            };
            const QString cs = QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\");
            QVector<RegistryOp> o;
            for (const char* cat : { "webcam", "contacts", "videosLibrary", "appointments",
                                     "phoneCallHistory", "email", "userDataTasks", "chat",
                                     "activity", "documentsLibrary", "picturesLibrary",
                                     "broadFileSystemAccess", "phoneCall", "radios" })
                o.push_back(s(cs + QLatin1String(cat), QStringLiteral("Value"), QStringLiteral("Deny")));
            o.push_back(s(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DeviceAccess\\Global\\LooselyCoupled"),
                          QStringLiteral("Value"), QStringLiteral("Deny")));
            t.registryOps = o;
            c.push_back(t);
        }

        // Only what op.privacy.typing-speech-inking doesn't write.
        {
            TweakSpec t;
            t.id = QStringLiteral("priv.text-input-collection-off");
            t.category = QStringLiteral("privacy");
            t.subCategory = QStringLiteral("Typing and handwriting");
            t.name = QStringLiteral("Stop text and handwriting learning");
            t.description = QStringLiteral(
                "Turns off keyboard text suggestions and the learning behind them, "
                "the input-method cloud lookup and lexicon updates, the typing "
                "insight loggers, and handwriting data and error-report sharing.");
            t.iconName = QStringLiteral("keyboard");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            const QString ti = QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\TextInput");
            const QString pm = QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\TextInput\\");
            const QString ime = QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\IME\\Shared");
            t.registryOps = {
                d(ti, QStringLiteral("AllowLinguisticDataCollection"), 0),
                d(ti, QStringLiteral("AllowHardwareKeyboardTextSuggestions"), 0),
                d(ti, QStringLiteral("AllowTextInputSuggestionUpdate"), 0),
                d(ti, QStringLiteral("AllowKeyboardTextSuggestions"), 0),
                d(pm + QStringLiteral("AllowHardwareKeyboardTextSuggestions"), QStringLiteral("value"), 0),
                d(pm + QStringLiteral("AllowKeyboardTextSuggestions"), QStringLiteral("value"), 0),
                d(pm + QStringLiteral("AllowTextInputSuggestionUpdate"), QStringLiteral("value"), 0),
                d(pm + QStringLiteral("AllowIMELogging"), QStringLiteral("value"), 0),
                d(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\InputPersonalization"), QStringLiteral("Installed"), 0),
                d(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\InputPersonalization"), QStringLiteral("Shutdown"), 1),
                d(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\InputPersonalization"), QStringLiteral("HarvestContacts"), 0),
                d(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\InputPersonalization\\TrainedDataStore"), QStringLiteral("InsightsEnabled"), 0),
                d(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\InputPersonalization\\TrainedDataStore"), QStringLiteral("LMDataLoggerEnabled"), 0),
                d(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CPSS\\UserPolicy\\InkingAndTypingPersonalization"), QStringLiteral("DefaultValue"), 0),
                d(ime, QStringLiteral("UserDict"), 0),
                d(ime, QStringLiteral("UseHistorybasedPredictiveInput"), 0),
                d(ime, QStringLiteral("misconvlogging"), 0),
                s(ime, QStringLiteral("Enable Cloud Candidate"), QStringLiteral("0")),
                s(ime, QStringLiteral("Enable Lexicon Update"), QStringLiteral("0")),
                d(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\TabletPC"), QStringLiteral("PreventHandwritingDataSharing"), 1),
                d(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\HandwritingErrorReports"), QStringLiteral("PreventHandwritingErrorReports"), 1),
            };
            c.push_back(t);
        }

        // op.debloat.copilot-cortana owns AllowCortana, ConnectedSearchUseWeb and CortanaConsent.
        {
            TweakSpec t;
            t.id = QStringLiteral("debloat.web-search-off");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Search");
            t.name = QStringLiteral("Turn off web results in search");
            t.description = QStringLiteral(
                "Stops Start-menu and Explorer search from reaching the web and the "
                "cloud, turns off Bing results and search history, and removes the "
                "\"look for an app in the Store\" and search-box suggestion prompts.");
            t.iconName = QStringLiteral("search");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            const QString ws = QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search");
            const QString wsu = QStringLiteral("HKCU\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search");
            const QString ex = QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Explorer");
            t.registryOps = {
                d(ws, QStringLiteral("ConnectedSearchUseWebOverMeteredConnections"), 0),
                d(ws, QStringLiteral("DisableWebSearch"), 1),
                d(ws, QStringLiteral("AllowCloudSearch"), 0),
                d(ws, QStringLiteral("AllowCortanaAboveLock"), 0),
                d(ws, QStringLiteral("AllowSearchToUseLocation"), 0),
                d(wsu, QStringLiteral("DeviceHistoryEnabled"), 0),
                d(wsu, QStringLiteral("HistoryViewEnabled"), 0),
                d(wsu, QStringLiteral("BingSearchEnabled"), 0),
                d(wsu, QStringLiteral("AllowCloudSearch"), 0),
                d(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Windows Search"),
                  QStringLiteral("EnableDynamicContentInWSB"), 0),
                d(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppHost"),
                  QStringLiteral("EnableWebContentEvaluation"), 0),
                d(ex, QStringLiteral("DisableSearchBoxSuggestions"), 1),
                d(ex, QStringLiteral("DisableThumbsDBOnNetworkFolders"), 1),
                d(ex, QStringLiteral("NoNewAppAlert"), 1),
                d(ex, QStringLiteral("NoUseStoreOpenWith"), 1),
                stringOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\AutoComplete"),
                         QStringLiteral("AutoSuggest"), QStringLiteral("no"), QStringLiteral("yes")),
            };
            c.push_back(t);
        }

        // op.debloat.suggested-content owns the per-user SubscribedContent values.
        {
            TweakSpec t;
            t.id = QStringLiteral("debloat.app-suggestions-off");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Suggested apps");
            t.name = QStringLiteral("Stop automatic and suggested app installs");
            t.description = QStringLiteral(
                "Stops Windows from silently installing suggested and promoted apps, "
                "turns off push-to-install and the Start suggestion pane, and denies "
                "apps voice activation and diagnostic access.");
            t.iconName = QStringLiteral("sparkles");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            const QString cdm = QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager");
            const QString ap = QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\AppPrivacy");
            const QString sg = QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SmartGlass");
            t.registryOps = {
                d(cdm, QStringLiteral("SilentInstalledAppsEnabled"), 0),
                d(cdm, QStringLiteral("PreInstalledAppsEnabled"), 0),
                d(cdm, QStringLiteral("OemPreInstalledAppsEnabled"), 0),
                d(cdm, QStringLiteral("SystemPaneSuggestionsEnabled"), 0),
                d(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\PushToInstall"), QStringLiteral("DisablePushToInstall"), 1),
                d(ap, QStringLiteral("LetAppsActivateWithVoice"), 2),
                d(ap, QStringLiteral("LetAppsGetDiagnosticInfo"), 2),
                d(sg, QStringLiteral("UserAuthPolicy"), 0),
                d(sg, QStringLiteral("BluetoothPolicy"), 0),
            };
            c.push_back(t);
        }

        // op.debloat.sync-activity turns off the per-user sync groups.
        {
            TweakSpec t;
            t.id = QStringLiteral("debloat.settings-sync-policy-off");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Sync");
            t.name = QStringLiteral("Enforce settings sync off");
            t.description = QStringLiteral(
                "Turns off syncing settings, saved credentials and app data across "
                "your devices at the policy level, and stops it being switched back "
                "on per user.");
            t.iconName = QStringLiteral("refresh-cw");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            const QString ss = QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\SettingSync");
            t.registryOps = {
                d(ss, QStringLiteral("DisableSettingSync"), 2),
                d(ss, QStringLiteral("DisableSettingSyncUserOverride"), 1),
                d(ss, QStringLiteral("DisableSyncOnPaidNetwork"), 1),
                d(ss, QStringLiteral("DisableCredentialsSettingSync"), 2),
                d(ss, QStringLiteral("DisableCredentialsSettingSyncUserOverride"), 1),
                d(ss, QStringLiteral("DisableApplicationSettingSync"), 2),
                d(ss, QStringLiteral("DisableApplicationSettingSyncUserOverride"), 1),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("debloat.onedrive-sync-off");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("OneDrive");
            t.name = QStringLiteral("Turn off OneDrive syncing");
            t.description = QStringLiteral(
                "Stops OneDrive from syncing files and from being the default save "
                "location, and blocks the prompt to move your folders into it. Leaves "
                "OneDrive installed - use the removal tool to take it off entirely.");
            t.iconName = QStringLiteral("database");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "redundant on a machine where op.debloat.remove-onedrive already removed "
                "OneDrive; kept for the case where the user wants sync off but the client "
                "present.");
            const QString od = QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Onedrive");
            t.registryOps = {
                d(od, QStringLiteral("DisableLibrariesDefaultSaveToOneDrive"), 1),
                d(od, QStringLiteral("DisableFileSync"), 1),
                d(od, QStringLiteral("DisableMeteredNetworkFileSync"), 0),
                d(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\OneDrive"), QStringLiteral("KFMBlockOptIn"), 1),
                d(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\OneDrive"), QStringLiteral("DisablePersonalSync"), 1),
                d(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows"), QStringLiteral("DisableFileSyncNGSC"), 1),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("debloat.notifications-lockscreen-off");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Notifications");
            t.name = QStringLiteral("Quiet notifications and the lock screen");
            t.description = QStringLiteral(
                "Turns off tile, cloud and mirrored notifications and the startup-app "
                "toast, empties the quick-actions panel, removes the Ink Workspace "
                "button, removes the lock screen and its camera, and turns off the "
                "Defender summary notification and the unexpected-shutdown timestamp "
                "writes.");
            t.iconName = QStringLiteral("bell");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.effects = {
                QStringLiteral("The PC goes straight to sign-in when it wakes."),
            };
            const QString pn = QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\CurrentVersion\\PushNotifications");
            const QString pers = QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Personalization");
            t.registryOps = {
                d(pn, QStringLiteral("NoTileApplicationNotification"), 1),
                d(pn, QStringLiteral("DisallowNotificationMirroring"), 1),
                d(pn, QStringLiteral("NoCloudApplicationNotification"), 1),
                d(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Notifications\\Settings\\Windows.SystemToast.StartupApp"),
                  QStringLiteral("Enabled"), 0),
                d(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Shell\\ActionCenter\\Quick Actions"),
                  QStringLiteral("PinnedQuickActionSlotCount"), 0),
                d(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\WindowsInkWorkspace"),
                  QStringLiteral("AllowWindowsInkWorkspace"), 0),
                d(pers, QStringLiteral("NoLockScreen"), 1),
                d(pers, QStringLiteral("NoLockScreenCamera"), 1),
                d(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Authentication\\LogonUI\\SessionData"),
                  QStringLiteral("AllowLockScreen"), 0),
                d(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows Defender Security Center\\Virus and threat protection"),
                  QStringLiteral("SummaryNotificationDisabled"), 1),
                d(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Reliability"),
                  QStringLiteral("TimeStampEnabled"), 0),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("sec.reduce-defender-cloud");
            t.category = QStringLiteral("security");
            t.subCategory = QStringLiteral("Defender");
            t.name = QStringLiteral("Reduce Defender cloud reporting");
            t.description = QStringLiteral(
                "Stops Microsoft Defender from sending sample files to the cloud and "
                "opts out of the malware-removal tool's reporting. Defender keeps "
                "scanning locally, but loses the cloud-assisted detection that catches "
                "brand-new threats faster.");
            t.iconName = QStringLiteral("shield");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.warnings = {
                QStringLiteral("Defender reacts more slowly to brand-new threats."),
            };
            t.concern = QStringLiteral(
                "security downgrade: SpyNetReporting=0 and SubmitSamplesConsent=2 cut cloud "
                "sample submission; the MRT values stop malicious-software-removal reporting. "
                "Never suggest unprompted.");
            const QString sn = QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows Defender\\Spynet");
            const QString mrt = QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\MRT");
            t.registryOps = {
                d(sn, QStringLiteral("SpyNetReporting"), 0),
                d(sn, QStringLiteral("SubmitSamplesConsent"), 2),
                d(mrt, QStringLiteral("DontReportInfectionInformation"), 1),
                d(mrt, QStringLiteral("DontOfferThroughWUAU"), 1),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("sec.disable-biometrics");
            t.category = QStringLiteral("security");
            t.subCategory = QStringLiteral("Sign-in");
            t.name = QStringLiteral("Disable biometric sign-in");
            t.description = QStringLiteral(
                "Turns off Windows Hello face and fingerprint sign-in for every "
                "account on this PC. Sign-in falls back to PIN or password.");
            t.iconName = QStringLiteral("fingerprint");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("Face and fingerprint sign-in stop working. Make sure a PIN or password is set first."),
            };
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Biometrics"),
                        QStringLiteral("Enabled"), 0u, 1u),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("debloat.edge-legacy-preload-off");
            t.category = QStringLiteral("browseroffice");
            t.subCategory = QStringLiteral("Edge");
            t.name = QStringLiteral("Stop Edge updates adding a desktop shortcut");
            t.description = QStringLiteral(
                "Stops the Microsoft Edge updater from putting an Edge shortcut back "
                "on the desktop each time it updates.");
            t.iconName = QStringLiteral("brand-edge");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.registryOps = {
                d(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\EdgeUpdate"), QStringLiteral("CreateDesktopShortcutDefault"), 0),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("priv.online-features-off");
            t.category = QStringLiteral("privacy");
            t.subCategory = QStringLiteral("Online features");
            t.name = QStringLiteral("Opt out of experiments and online extras");
            t.description = QStringLiteral(
                "Skips the setup privacy screen on new accounts, opts out of the "
                "Windows experimentation program, stops name resolution across "
                "networks at once, turns off peer-to-peer update sharing, and blocks "
                "online DRM license retrieval.");
            t.iconName = QStringLiteral("world");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.warnings = {
                QStringLiteral("Some protected music and video may not play."),
            };
            t.registryOps = {
                d(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\OOBE"), QStringLiteral("DisablePrivacyExperience"), 1),
                d(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\current\\device\\System"), QStringLiteral("AllowExperimentation"), 0),
                d(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows NT\\DNSClient"), QStringLiteral("DisableSmartNameResolution"), 1),
                d(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DeliveryOptimization\\Config"), QStringLiteral("DODownloadMode"), 0),
                d(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\WMDRM"), QStringLiteral("DisableOnline"), 1),
            };
            c.push_back(t);
        }
    }
}

