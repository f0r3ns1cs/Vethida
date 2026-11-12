#include "TweakEngine.h"

namespace vth::tweaks::detail {

void appendDefsPrivacy(QVector<TweakSpec>& c) {
    {
        TweakSpec t;
        t.id = QStringLiteral("priv.advertising.extended_cloud");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Cloud content & Spotlight");
        t.name = QStringLiteral("Disable extended cloud content and Spotlight");
        t.description = QStringLiteral("Disables Windows Spotlight configuration, welcome experience, action center and settings spotlight, enterprise spotlight, CloudExperienceHost intent categories, and extended Content Delivery Manager values.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Windows Spotlight fully disabled"),
            QStringLiteral("Content delivery and pre-installed apps disabled"),
            QStringLiteral("OEM apps removed from delivery pipeline"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\CloudContent"), QStringLiteral("ConfigureWindowsSpotlight"), 2u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\CloudContent"), QStringLiteral("IncludeEnterpriseSpotlight"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\CloudContent"), QStringLiteral("DisableWindowsSpotlightWindowsWelcomeExperience"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\CloudContent"), QStringLiteral("DisableWindowsSpotlightOnActionCenter"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\CloudContent"), QStringLiteral("DisableWindowsSpotlightOnSettings"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\CloudContent"), QStringLiteral("ConfigureWindowsSpotlight"), 2u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\CloudContent"), QStringLiteral("IncludeEnterpriseSpotlight"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\CloudContent"), QStringLiteral("DisableWindowsSpotlightWindowsWelcomeExperience"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\CloudContent"), QStringLiteral("DisableWindowsSpotlightOnActionCenter"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\CloudContent"), QStringLiteral("DisableWindowsSpotlightOnSettings"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\Experience\\DisableCloudOptimizedContent"), QStringLiteral("value"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\Experience\\AllowWindowsConsumerFeatures"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\Experience\\DisableConsumerAccountStateContent"), QStringLiteral("value"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\CloudExperienceHost\\Intent\\creative"), QStringLiteral("Intent"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\CloudExperienceHost\\Intent\\entertainment"), QStringLiteral("Intent"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\CloudExperienceHost\\Intent\\family"), QStringLiteral("Intent"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\CloudExperienceHost\\Intent\\gaming"), QStringLiteral("Intent"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\CloudExperienceHost\\Intent\\schoolwork"), QStringLiteral("Intent"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\CloudExperienceHost\\Intent\\business"), QStringLiteral("Intent"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\CloudExperienceHost\\Intent\\development"), QStringLiteral("Intent"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("FeatureManagementEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("SlideshowEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("OemPreInstalledAppsEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("PreInstalledAppsEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("RotatingLockScreenEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("RotatingLockScreenOverlayEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("SilentInstalledAppsEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("ContentDeliveryAllowed"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("SubscribedContentEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("PreInstalledAppsEverEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("RemediationRequired"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("ShowSyncProviderNotifications"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("SubscribedContent-310093Enabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("SubscribedContent-338389Enabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("SubscribedContent-88000326Enabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("SubscribedContent-280815Enabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("SubscribedContent-310091Enabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("SubscribedContent-314559Enabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("SubscribedContent-338387Enabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("SubscribedContent-338388Enabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("SubscribedContent-88000045Enabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("SubscribedContent-88000530Enabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("SubscribedContent-353698Enabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("SubscribedContent-314563Enabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("SubscribedContent-310092Enabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("SubscribedContent-338380Enabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("SubscribedContent-338381Enabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("SubscribedContent-202914Enabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("SubscribedContent-88000161Enabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("SubscribedContent-88000163Enabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("SubscribedContent-88000165Enabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\Experience\\AllowManualMDMUnenrollment"), QStringLiteral("value"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.advertising.oobe_ink");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("OOBE & suggestions");
        t.name = QStringLiteral("Disable OOBE ads and ink suggestions");
        t.description = QStringLiteral("Disables privacy experience during OOBE, Scoobe system settings prompts, Windows Ink app suggestions, and lock screen overlays.");
        t.iconName = QStringLiteral("eye-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("OOBE privacy prompts disabled"),
            QStringLiteral("Ink workspace suggestions removed"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\OOBE"), QStringLiteral("DisablePrivacyExperience"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\UserProfileEngagement"), QStringLiteral("ScoobeSystemSettingEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\PenWorkspace"), QStringLiteral("PenWorkspaceAppSuggestionsEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Personalization"), QStringLiteral("LockScreenOverlaysDisabled"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.appdiag.speech");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Speech & voice");
        t.name = QStringLiteral("Restrict online speech and voice activation");
        t.description = QStringLiteral("Configures online speech privacy acceptance and disables voice activation preference.");
        t.iconName = QStringLiteral("eye-off");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Voice activation disabled"),
            QStringLiteral("Online speech features restricted"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Speech_OneCore\\Settings\\OnlineSpeechPrivacy"), QStringLiteral("HasAccepted"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Speech_OneCore\\Preferences"), QStringLiteral("VoiceActivationOn"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.appv.disable");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Services");
        t.name = QStringLiteral("Disable App-V virtualization");
        t.description = QStringLiteral("Sets the App-V client policy to disabled. App-V (Microsoft Application Virtualization) is an enterprise application-streaming feature that is inactive on consumer Windows, so this has no measurable performance effect; it only prevents App-V-published apps from launching where App-V is in use.");
        t.iconName = QStringLiteral("eye-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Microsoft App-V virtualized applications (enterprise-only) will not launch"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\AppV\\Client"), QStringLiteral("Enabled"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.search.comprehensive");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Search & Cortana");
        t.name = QStringLiteral("Comprehensive search privacy");
        t.description = QStringLiteral("Disables Bing/web search, MSA/AAD cloud search, search indexing on battery, removable-drive indexing, search history, web results, and connected-search privacy in one step.");
        t.iconName = QStringLiteral("search");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Start menu search no longer shows Bing web results or cloud (OneDrive/Outlook) files"),
            QStringLiteral("Search indexing is disabled on battery and removable drives"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Search"), QStringLiteral("BingSearchEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Search"), QStringLiteral("WebControlStatus"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Search"), QStringLiteral("WebControlSecondaryStatus"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Search"), QStringLiteral("HasAboveLockTips"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Search"), QStringLiteral("CortanaConsent"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Search"), QStringLiteral("AllowSearchToUseLocation"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\SearchSettings"), QStringLiteral("IsMSACloudSearchEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\SearchSettings"), QStringLiteral("IsAADCloudSearchEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search"), QStringLiteral("DisableWebSearch"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search"), QStringLiteral("ConnectedSearchUseWeb"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search"), QStringLiteral("ConnectedSearchUseWebOverMeteredConnections"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search"), QStringLiteral("AllowCloudSearch"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search"), QStringLiteral("ConnectedSearchPrivacy"), 3u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search"), QStringLiteral("ConnectedSearchSafeSearch"), 3u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search"), QStringLiteral("PreventIndexOnBattery"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search"), QStringLiteral("DisableRemovableDriveIndexing"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search"), QStringLiteral("BackgroundAppGlobalToggle"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search"), QStringLiteral("AllowSearchToUseLocation"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\SearchCompanion"), QStringLiteral("DisableContentFileUpdate"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.activity.history");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Activity history");
        t.name = QStringLiteral("Disable Activity History");
        t.description = QStringLiteral("Stops Windows from collecting and uploading activity history, disables CDP, tailored experiences, Explorer recent/frequent tracking, and recent-docs history.");
        t.iconName = QStringLiteral("clock");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Windows Timeline, cross-device resume, and the recent/frequent files lists stop working"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System"), QStringLiteral("EnableActivityFeed"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System"), QStringLiteral("PublishUserActivities"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System"), QStringLiteral("UploadUserActivities"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System"), QStringLiteral("EnableCdp"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Privacy"), QStringLiteral("TailoredExperiencesWithDiagnosticDataEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer"), QStringLiteral("ShowFrequent"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer"), QStringLiteral("ShowRecent"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer"), QStringLiteral("TelemetrySalt"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"), QStringLiteral("NoRecentDocsHistory"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.advertising.notifications");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Notifications");
        t.name = QStringLiteral("Disable promotional notifications");
        t.description = QStringLiteral("Disables toast notifications, notification sound, lock-screen toasts, the individual system toast providers, and promotional subscribed content.");
        t.iconName = QStringLiteral("bell-off");
        t.impact = Impact::Medium;
        t.warnings = {
            QStringLiteral("All toast notifications are disabled system-wide; important system alerts may be missed"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\PushNotifications"), QStringLiteral("ToastEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Notifications\\Settings"), QStringLiteral("NOC_GLOBAL_SETTING_ALLOW_NOTIFICATION_SOUND"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Notifications\\Settings"), QStringLiteral("NOC_GLOBAL_SETTING_ALLOW_CRITICAL_TOASTS_ABOVE_LOCK"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Notifications\\Settings"), QStringLiteral("NOC_GLOBAL_SETTING_ALLOW_TOASTS_ABOVE_LOCK"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Notifications\\Settings\\QuietHours"), QStringLiteral("Enabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Notifications\\Settings\\windows.immersivecontrolpanel_cw5n1h2txyewy!microsoft.windows.immersivecontrolpanel"), QStringLiteral("Enabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Notifications\\Settings\\Windows.SystemToast.AutoPlay"), QStringLiteral("Enabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Notifications\\Settings\\Windows.SystemToast.LowDisk"), QStringLiteral("Enabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Notifications\\Settings\\Windows.SystemToast.Print.Notification"), QStringLiteral("Enabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Notifications\\Settings\\Windows.SystemToast.SecurityAndMaintenance"), QStringLiteral("Enabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Notifications\\Settings\\Windows.SystemToast.WiFiNetworkManager"), QStringLiteral("Enabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("SubscribedContent-338389Enabled"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.advertising.id");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Advertising ID");
        t.name = QStringLiteral("Disable Advertising ID");
        t.description = QStringLiteral("Disables the per-user advertising ID, enforces it via Group Policy, disables Bluetooth advertising, and opts out of language-based tracking.");
        t.iconName = QStringLiteral("eye-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Ads in free Store apps become generic instead of personalized"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\AdvertisingInfo"), QStringLiteral("Enabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\AdvertisingInfo"), QStringLiteral("DisabledByGroupPolicy"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\current\\device\\Bluetooth"), QStringLiteral("AllowAdvertising"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Control Panel\\International\\User Profile"), QStringLiteral("HttpAcceptLanguageOptOut"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.telemetry.app_compat");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Telemetry");
        t.name = QStringLiteral("Disable application compatibility telemetry");
        t.description = QStringLiteral("Disables the Application Compatibility appraiser, inventory collector, UAR, and client telemetry.");
        t.iconName = QStringLiteral("eye-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Application compatibility inventory stops; upgrade advisor checks may miss known issues"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\ClientTelemetry"), QStringLiteral("DontRetryOnError"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\ClientTelemetry"), QStringLiteral("IsCensusDisabled"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\ClientTelemetry"), QStringLiteral("TaskEnableRun"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\AppCompat"), QStringLiteral("AITEnable"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\AppCompat"), QStringLiteral("DisableUAR"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\AppCompat"), QStringLiteral("DisableInventory"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.track.app_suggestions");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("App suggestions");
        t.name = QStringLiteral("Disable app suggestions and pre-installed bloat");
        t.description = QStringLiteral("Disables all Content Delivery Manager suggestions, pre-installed apps, and silent app installations.");
        t.iconName = QStringLiteral("package-off");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Start menu no longer shows suggested apps or promoted Store installs"),
            QStringLiteral("Preinstalled promotional apps (e.g. Candy Crush) will not auto-restore"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("SilentInstalledAppsEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("SoftLandingEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("SystemPaneSuggestionsEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("OemPreInstalledAppsEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("PreInstalledAppsEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("PreInstalledAppsEverEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("SubscribedContent-338388Enabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("SubscribedContent-338389Enabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("SubscribedContent-353694Enabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("SubscribedContent-353696Enabled"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.ai.bing_suggestions");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Search & Cortana");
        t.name = QStringLiteral("Disable Bing AI and search suggestions");
        t.description = QStringLiteral("Disables Bing AI suggestions, web search in Start Menu, and Edge AI features.");
        t.iconName = QStringLiteral("robot-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Start menu search returns local results only; no Bing web answers or AI suggestions"),
            QStringLiteral("Edge sidebar hubs (Copilot, Discover) hidden"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Policies\\Microsoft\\Windows\\Explorer"), QStringLiteral("DisableSearchBoxSuggestions"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Policies\\Microsoft\\Windows\\Explorer"), QStringLiteral("BingSearchEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("HubsSidebarEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("PersonalizationReportingEnabled"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.location.biometrics");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Biometrics");
        t.name = QStringLiteral("Disable biometrics");
        t.description = QStringLiteral("Disables the Windows biometric framework (fingerprint, facial recognition).");
        t.iconName = QStringLiteral("fingerprint");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Windows Hello fingerprint and facial recognition will stop working"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Biometrics"), QStringLiteral("Enabled"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.appdiag.capability_access");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("App permissions");
        t.name = QStringLiteral("Restrict capability access (all sensors/data)");
        t.description = QStringLiteral("Denies all 23 hardware/data access capabilities (camera, microphone, location, contacts, etc.) at the user level and disables background apps globally. This is a blanket switch - it will break legitimate apps that need these until re-granted individually in Settings.");
        t.iconName = QStringLiteral("shield");
        t.impact = Impact::High;
        // One blanket tweak rather than 23 per-capability ones. High impact, so it's
        // never applied in bulk.
        t.concern = QStringLiteral("blanket deny-all breaks legitimate apps; opt-in only, never recommended. Per-item toggles would be better.");
        t.warnings = {
            QStringLiteral("Camera, microphone, and location are denied to all apps until re-granted"),
            QStringLiteral("Background apps (notifications, live tiles, sync) are disabled globally"),
        };
        t.registryOps = {
            stringOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\activity"), QStringLiteral("Value"), QStringLiteral("Deny"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\appDiagnostics"), QStringLiteral("Value"), QStringLiteral("Deny"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\appointments"), QStringLiteral("Value"), QStringLiteral("Deny"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\bluetoothSync"), QStringLiteral("Value"), QStringLiteral("Deny"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\broadFileSystemAccess"), QStringLiteral("Value"), QStringLiteral("Deny"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\cellularData"), QStringLiteral("Value"), QStringLiteral("Deny"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\chat"), QStringLiteral("Value"), QStringLiteral("Deny"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\contacts"), QStringLiteral("Value"), QStringLiteral("Deny"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\documentsLibrary"), QStringLiteral("Value"), QStringLiteral("Deny"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\email"), QStringLiteral("Value"), QStringLiteral("Deny"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\gazeInput"), QStringLiteral("Value"), QStringLiteral("Deny"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\location"), QStringLiteral("Value"), QStringLiteral("Deny"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\microphone"), QStringLiteral("Value"), QStringLiteral("Deny"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\musicLibrary"), QStringLiteral("Value"), QStringLiteral("Deny"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\phoneCall"), QStringLiteral("Value"), QStringLiteral("Deny"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\phoneCallHistory"), QStringLiteral("Value"), QStringLiteral("Deny"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\picturesLibrary"), QStringLiteral("Value"), QStringLiteral("Deny"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\radios"), QStringLiteral("Value"), QStringLiteral("Deny"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\userAccountInformation"), QStringLiteral("Value"), QStringLiteral("Deny"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\userDataTasks"), QStringLiteral("Value"), QStringLiteral("Deny"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\userNotificationListener"), QStringLiteral("Value"), QStringLiteral("Deny"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\videosLibrary"), QStringLiteral("Value"), QStringLiteral("Deny"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\webcam"), QStringLiteral("Value"), QStringLiteral("Deny"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DeviceAccess\\Global\\LooselyCoupled"), QStringLiteral("Value"), QStringLiteral("Deny"), std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\BackgroundAccessApplications"), QStringLiteral("GlobalUserDisabled"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Search"), QStringLiteral("BackgroundAppGlobalToggle"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.appdiag.cdp");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Connected devices");
        t.name = QStringLiteral("Disable connected devices platform");
        t.description = QStringLiteral("Disables the Connected Devices Platform session, NearShare, and Rome SDK authorization policies used for cross-device experiences.");
        t.iconName = QStringLiteral("link");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Phone Link, Nearby Sharing, and cross-device clipboard/handoff stop working"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CDP"), QStringLiteral("CdpSessionUserAuthzPolicy"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CDP"), QStringLiteral("NearShareChannelUserAuthzPolicy"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CDP"), QStringLiteral("RomeSdkChannelUserAuthzPolicy"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System"), QStringLiteral("EnableCdp"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.disable-clipboard-cloud-sync");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Clipboard");
        t.name = QStringLiteral("Disable cross-device clipboard sync");
        t.description = QStringLiteral("Stops clipboard contents from syncing to the user's Microsoft Account-bound device set.");
        t.iconName = QStringLiteral("clipboard");
        t.impact = Impact::Low;
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Clipboard"), QStringLiteral("EnableCloudClipboard"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.advertising.cloud_content");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Cloud content & Spotlight");
        t.name = QStringLiteral("Disable cloud content and consumer features");
        t.description = QStringLiteral("Prevents Windows from showing suggested apps, tips, Spotlight, and promotional content across CloudContent policies and the Content Delivery Manager.");
        t.iconName = QStringLiteral("cloud-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Suggested-app installs, lock-screen Spotlight, and first-run promotional tips no longer appear"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\CloudContent"), QStringLiteral("DisableThirdPartySuggestions"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\CloudContent"), QStringLiteral("DisableWindowsConsumerFeatures"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\CloudContent"), QStringLiteral("DisableSoftLanding"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\CloudContent"), QStringLiteral("DisableWindowsSpotlightFeatures"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\CloudContent"), QStringLiteral("DisableCloudOptimizedContent"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\CloudContent"), QStringLiteral("DisableConsumerAccountStateContent"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\CloudContent"), QStringLiteral("DisableTailoredExperiencesWithDiagnosticData"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("SubscribedContent-338393Enabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("SubscribedContent-353694Enabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("SubscribedContent-353696Enabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("SubscribedContent-338394Enabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("SubscribedContent-338396Enabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("SoftLandingEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("SystemPaneSuggestionsEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"), QStringLiteral("DisableWindowsConsumerFeatures"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.cortana.copilot");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Copilot & AI");
        t.name = QStringLiteral("Disable Windows Copilot");
        t.description = QStringLiteral("Disables Windows Copilot, hides its taskbar button, and blocks it via both HKCU and HKLM policies.");
        t.iconName = QStringLiteral("bot");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Windows Copilot sidebar and taskbar icon removed; AI assistant unavailable"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), QStringLiteral("ShowCopilotButton"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\WindowsCopilot"), QStringLiteral("TurnOffWindowsCopilot"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\Software\\Policies\\Microsoft\\Windows\\WindowsCopilot"), QStringLiteral("TurnOffWindowsCopilot"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("etw.disable-diagtrack-listener");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Telemetry");
        t.name = QStringLiteral("Disable diagtrack-listener autologger");
        t.description = QStringLiteral("Stops the DiagTrack ETW autologger session by setting its Start key to 0.");
        t.iconName = QStringLiteral("shield");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Half-fix without the matching DiagTrack service-disabled tweak - the service rebuilds the autologger at boot"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\AutoLogger-Diagtrack-Listener"), QStringLiteral("Start"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.activity.event_logging");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Event logging");
        t.name = QStringLiteral("Disable event logging upload");
        t.description = QStringLiteral("Disables protected event logging, the EventLog setup channel, and event-log upload to Microsoft.");
        t.iconName = QStringLiteral("file-text");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Event log uploads and protected logging disabled; local Event Viewer still works"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\EventLog\\ProtectedEventLogging"), QStringLiteral("EnableProtectedEventLogging"), 0u, std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\EventLog\\Setup"), QStringLiteral("Enabled"), QStringLiteral("0"), std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System"), QStringLiteral("UploadPolicy"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.appdiag.inking");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Input & typing");
        t.name = QStringLiteral("Disable inking and typing telemetry");
        t.description = QStringLiteral("Stops Windows from collecting inking, typing, voice, and input-insight data, and disables Skype/Media Player usage tracing.");
        t.iconName = QStringLiteral("pencil");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Typing autocorrect and handwriting recognition no longer learn from your input"),
            QStringLiteral("Voice activation (\\\"Hey Cortana\\\"-style wake) is disabled"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\InputPersonalization"), QStringLiteral("RestrictImplicitInkCollection"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\InputPersonalization"), QStringLiteral("RestrictImplicitTextCollection"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\InputPersonalization\\TrainedDataStore"), QStringLiteral("HarvestContacts"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Personalization\\Settings"), QStringLiteral("AcceptedPrivacyPolicy"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Input"), QStringLiteral("IsInputAppPreloadEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Input\\Settings"), QStringLiteral("VoiceTypingEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Input\\Settings"), QStringLiteral("InsightsEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Input\\TIPC"), QStringLiteral("Enabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Input\\TIPC"), QStringLiteral("Enabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\InputPersonalization"), QStringLiteral("AllowInputPersonalization"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\InputPersonalization"), QStringLiteral("RestrictImplicitInkCollection"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Messaging"), QStringLiteral("AllowMessageSync"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Speech"), QStringLiteral("AllowSpeechModelUpdate"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Speech_OneCore\\Settings\\VoiceActivation\\UserPreferenceForAllApps"), QStringLiteral("AgentActivationEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Speech_OneCore\\Settings\\VoiceActivation\\UserPreferenceForAllApps"), QStringLiteral("AgentActivationOnLockScreenEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Speech_OneCore\\Settings\\VoiceActivation\\UserPreferenceForAllApps"), QStringLiteral("AgentActivationLastUsed"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Tracing\\WPPMediaPerApp\\Skype\\ETW"), QStringLiteral("TraceLevelThreshold"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Tracing\\WPPMediaPerApp\\Skype\\ETW"), QStringLiteral("EnableTracing"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Tracing\\WPPMediaPerApp\\Skype"), QStringLiteral("EnableTracing"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\MediaPlayer\\Preferences"), QStringLiteral("UsageTracking"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.cortana.recall");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Recall");
        t.name = QStringLiteral("Disable Windows Recall");
        t.description = QStringLiteral("Disables Windows Recall, blocks its enablement, and stops snapshot saving and AI data analysis.");
        t.iconName = QStringLiteral("camera-off");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("You will lose the ability to search your timeline of past activity via Windows Recall"),
            QStringLiteral("Other AI features tied to on-device data analysis may also stop working"),
            QStringLiteral("Major Windows updates can reset this policy and require reapplying the tweak"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\Software\\Policies\\Microsoft\\Windows\\WindowsAI"), QStringLiteral("AllowRecallEnablement"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\WindowsAI"), QStringLiteral("DisableAIDataAnalysis"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\WindowsAI"), QStringLiteral("TurnOffSavingSnapshots"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.cortana.search");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Search & Cortana");
        t.name = QStringLiteral("Disable search box suggestions and Bing");
        t.description = QStringLiteral("Disables Cortana, web/Bing search suggestions in the Windows search box, device search history, and the Edge sidebar/startup-boost/background/GPU policies.");
        t.iconName = QStringLiteral("search");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Windows search box no longer shows web suggestions or Bing results"),
            QStringLiteral("Edge pre-launch, background mode, and GPU acceleration are disabled"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search"), QStringLiteral("AllowCortana"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search"), QStringLiteral("AllowCortanaAboveLock"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search"), QStringLiteral("DisableWebSearch"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search"), QStringLiteral("ConnectedSearchUseWeb"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search"), QStringLiteral("ConnectedSearchUseWebOverMeteredConnections"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Search"), QStringLiteral("CortanaEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Search"), QStringLiteral("CortanaConsent"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Search"), QStringLiteral("SearchboxTaskbarMode"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Search"), QStringLiteral("BingSearchEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Search"), QStringLiteral("DeviceHistoryEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Search"), QStringLiteral("HistoryViewEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Search"), QStringLiteral("AllowSearchToUseLocation"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), QStringLiteral("ShowCortanaButton"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows"), QStringLiteral("DisableSearchBoxSuggestions"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Policies\\Microsoft\\Windows\\Explorer"), QStringLiteral("DisableSearchBoxSuggestions"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\SearchSettings"), QStringLiteral("IsDynamicSearchBoxEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\SearchSettings"), QStringLiteral("SafeSearchMode"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\SearchSettings"), QStringLiteral("IsDeviceSearchHistoryEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\Software\\Policies\\Microsoft\\Edge"), QStringLiteral("HubsSidebarEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\Software\\Policies\\Microsoft\\Edge"), QStringLiteral("StartupBoostEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\Software\\Policies\\Microsoft\\Edge"), QStringLiteral("BackgroundModeEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\Software\\Policies\\Microsoft\\Edge"), QStringLiteral("HardwareAccelerationModeEnabled"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.track.tailored_experiences");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Telemetry");
        t.name = QStringLiteral("Disable tailored experiences and diagnostics");
        t.description = QStringLiteral("Disables tailored experiences that use diagnostic data to personalize ads, tips, and recommendations.");
        t.iconName = QStringLiteral("eye-off");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Personalized tips, ads, and Start menu suggestions based on your usage are disabled"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Policies\\Microsoft\\Windows\\CloudContent"), QStringLiteral("DisableTailoredExperiencesWithDiagnosticData"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Policies\\Microsoft\\Windows\\CloudContent"), QStringLiteral("DisableWindowsConsumerFeatures"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Privacy"), QStringLiteral("TailoredExperiencesWithDiagnosticDataEnabled"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.ai.voice_activation");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Speech & voice");
        t.name = QStringLiteral("Disable voice activation and online speech");
        t.description = QStringLiteral("Disables voice activation, online speech recognition, and microphone access for Cortana/Copilot.");
        t.iconName = QStringLiteral("microphone-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Hey Cortana / Copilot wake-word and voice typing stop functioning"),
            QStringLiteral("Apps can no longer activate by voice above the lock screen"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Speech_OneCore\\Settings\\VoiceActivation\\UserPreferenceForAllApps"), QStringLiteral("AgentActivationEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsActivateWithVoice"), 2u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsActivateWithVoiceAboveLock"), 2u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Speech_OneCore\\Settings\\OnlineSpeechPrivacy"), QStringLiteral("HasAccepted"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.activity.vs_telemetry");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Telemetry");
        t.name = QStringLiteral("Disable Visual Studio telemetry");
        t.description = QStringLiteral("Disables Visual Studio telemetry via the global TurnOffSwitch and the SQM opt-in for VS 14.0-17.0.");
        t.iconName = QStringLiteral("code");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Visual Studio usage and crash telemetry to Microsoft disabled"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\Software\\Policies\\Microsoft\\VisualStudio\\SQM"), QStringLiteral("OptIn"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\VisualStudio\\Telemetry"), QStringLiteral("TurnOffSwitch"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\VSCommon\\14.0\\SQM"), QStringLiteral("OptIn"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\WOW6432Node\\Microsoft\\VSCommon\\14.0\\SQM"), QStringLiteral("OptIn"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\VSCommon\\15.0\\SQM"), QStringLiteral("OptIn"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\WOW6432Node\\Microsoft\\VSCommon\\15.0\\SQM"), QStringLiteral("OptIn"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\VSCommon\\16.0\\SQM"), QStringLiteral("OptIn"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\WOW6432Node\\Microsoft\\VSCommon\\16.0\\SQM"), QStringLiteral("OptIn"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\VSCommon\\17.0\\SQM"), QStringLiteral("OptIn"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\WOW6432Node\\Microsoft\\VSCommon\\17.0\\SQM"), QStringLiteral("OptIn"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.appdiag.wer");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Error reporting");
        t.name = QStringLiteral("Disable Windows Error Reporting");
        t.description = QStringLiteral("Fully disables WER crash reporting, logging, consent, escalation, and additional-data upload across all policy paths.");
        t.iconName = QStringLiteral("alert-triangle");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("App crash and hang reports no longer uploaded to Microsoft; local crash debugging via AeDebug is disabled"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\Software\\Microsoft\\Windows\\Windows Error Reporting"), QStringLiteral("LoggingDisabled"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\Software\\Microsoft\\Windows\\Windows Error Reporting"), QStringLiteral("Disabled"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\Software\\Microsoft\\Windows\\Windows Error Reporting"), QStringLiteral("ChangeDumpTypeByTelemetryLevel"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\Software\\Microsoft\\Windows\\Windows Error Reporting"), QStringLiteral("DontSendAdditionalData"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\Software\\Microsoft\\Windows\\Windows Error Reporting\\Consent"), QStringLiteral("DefaultConsent"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\Software\\Microsoft\\Windows\\Windows Error Reporting\\Consent"), QStringLiteral("DefaultOverrideBehavior"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\Software\\Policies\\Microsoft\\Windows\\Windows Error Reporting"), QStringLiteral("Disabled"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\Software\\Policies\\Microsoft\\Windows\\Windows Error Reporting"), QStringLiteral("DoReport"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\Windows Error Reporting\\Escalation"), QStringLiteral("DisableEscalation"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\Windows Error Reporting"), QStringLiteral("Disabled"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\Windows Error Reporting"), QStringLiteral("DontSendAdditionalData"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\PCHealth\\ErrorReporting"), QStringLiteral("DoReport"), 0u, std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug"), QStringLiteral("Auto"), QStringLiteral("0"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.location.prompts");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Location");
        t.name = QStringLiteral("Disable location access prompts");
        t.description = QStringLiteral("Disables global prompts when apps request location access.");
        t.iconName = QStringLiteral("map-pin-off");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Location permission prompts suppressed"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\location"), QStringLiteral("ShowGlobalPrompts"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.permissions.appprivacy_policy");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("App permissions");
        t.name = QStringLiteral("Enforce AppPrivacy policy restrictions");
        t.description = QStringLiteral("Sets Group Policy-level AppPrivacy restrictions for all capabilities: camera, microphone, location, contacts, calendar, call history, email, tasks, messaging, radios, notifications, account info, diagnostics, gaze input, graphics capture, voice activation, motion, background spatial perception, and device sync.");
        t.iconName = QStringLiteral("eye-off");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("UWP/Store apps denied access to contacts, calendar, email, messaging, notifications, and location at the Group Policy level"),
            QStringLiteral("Graphics capture APIs (screen sharing, OBS capture) denied for Store apps"),
            QStringLiteral("Users cannot re-grant these permissions from Settings without removing the policy"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsAccessCalendar"), 2u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsAccessContacts"), 2u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsAccessCallHistory"), 2u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsAccessEmail"), 2u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsAccessTasks"), 2u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsAccessMessaging"), 2u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsAccessRadios"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsAccessNotifications"), 2u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsAccessAccountInfo"), 2u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsGetDiagnosticInfo"), 2u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsAccessGazeInput"), 2u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsAccessGraphicsCaptureProgrammatic"), 2u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsAccessGraphicsCaptureWithoutBorder"), 2u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsActivateWithVoice"), 2u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsActivateWithVoiceAboveLock"), 2u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsAccessMotion"), 2u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsAccessBackgroundSpatialPerception"), 2u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsSyncWithDevices"), 2u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsAccessLocation"), 2u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsAccessPhone"), 2u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsAccessTrustedDevices"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsAccessCalendar"), 2u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsAccessContacts"), 2u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsAccessCallHistory"), 2u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsAccessEmail"), 2u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsAccessTasks"), 2u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsAccessMessaging"), 2u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsAccessRadios"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsAccessNotifications"), 2u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsAccessAccountInfo"), 2u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsGetDiagnosticInfo"), 2u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsAccessGazeInput"), 2u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsAccessGraphicsCaptureProgrammatic"), 2u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsAccessGraphicsCaptureWithoutBorder"), 2u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsActivateWithVoice"), 2u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsActivateWithVoiceAboveLock"), 2u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsAccessMotion"), 2u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsAccessBackgroundSpatialPerception"), 2u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsSyncWithDevices"), 2u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsAccessLocation"), 2u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsAccessPhone"), 2u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Windows\\AppPrivacy"), QStringLiteral("LetAppsAccessTrustedDevices"), 0u, 1u),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\graphicsCaptureProgrammatic"), QStringLiteral("Value"), QStringLiteral("Deny"), QStringLiteral("Allow")),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\graphicsCaptureWithoutBorder"), QStringLiteral("Value"), QStringLiteral("Deny"), QStringLiteral("Allow")),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\activity"), QStringLiteral("Value"), QStringLiteral("Deny"), QStringLiteral("Allow")),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\appDiagnostics"), QStringLiteral("Value"), QStringLiteral("Deny"), QStringLiteral("Allow")),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\appointments"), QStringLiteral("Value"), QStringLiteral("Deny"), QStringLiteral("Allow")),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\contacts"), QStringLiteral("Value"), QStringLiteral("Deny"), QStringLiteral("Allow")),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\email"), QStringLiteral("Value"), QStringLiteral("Deny"), QStringLiteral("Allow")),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\chat"), QStringLiteral("Value"), QStringLiteral("Deny"), QStringLiteral("Allow")),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\phoneCall"), QStringLiteral("Value"), QStringLiteral("Deny"), QStringLiteral("Allow")),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\phoneCallHistory"), QStringLiteral("Value"), QStringLiteral("Deny"), QStringLiteral("Allow")),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\callhistory"), QStringLiteral("Value"), QStringLiteral("Deny"), QStringLiteral("Allow")),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\userAccountInformation"), QStringLiteral("Value"), QStringLiteral("Deny"), QStringLiteral("Allow")),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\userDataTasks"), QStringLiteral("Value"), QStringLiteral("Deny"), QStringLiteral("Allow")),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\userNotificationListener"), QStringLiteral("Value"), QStringLiteral("Deny"), QStringLiteral("Allow")),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\gazeInput"), QStringLiteral("Value"), QStringLiteral("Deny"), QStringLiteral("Allow")),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\location"), QStringLiteral("Value"), QStringLiteral("Deny"), QStringLiteral("Allow")),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\sensors.custom"), QStringLiteral("Value"), QStringLiteral("Deny"), QStringLiteral("Allow")),
            stringOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\graphicsCaptureProgrammatic"), QStringLiteral("Value"), QStringLiteral("Deny"), QStringLiteral("Allow")),
            stringOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\graphicsCaptureWithoutBorder"), QStringLiteral("Value"), QStringLiteral("Deny"), QStringLiteral("Allow")),
            stringOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\callhistory"), QStringLiteral("Value"), QStringLiteral("Deny"), QStringLiteral("Allow")),
            stringOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\calendar"), QStringLiteral("Value"), QStringLiteral("Deny"), QStringLiteral("Allow")),
            stringOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\sensors.custom"), QStringLiteral("Value"), QStringLiteral("Deny"), QStringLiteral("Allow")),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.settingsync.disable_all");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Sync");
        t.name = QStringLiteral("Disable Windows settings sync");
        t.description = QStringLiteral("Prevents Windows from syncing accessibility, browser, credentials, desktop theme, language, package state, personalization, start layout, and other settings across devices.");
        t.iconName = QStringLiteral("eye-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Accessibility, theme, language, and saved passwords no longer sync across Microsoft-account devices"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\SettingSync\\Groups\\Accessibility"), QStringLiteral("Enabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\SettingSync\\Groups\\AppSync"), QStringLiteral("Enabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\SettingSync\\Groups\\BrowserSettings"), QStringLiteral("Enabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\SettingSync\\Groups\\Credentials"), QStringLiteral("Enabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\SettingSync\\Groups\\DesktopTheme"), QStringLiteral("Enabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\SettingSync\\Groups\\Language"), QStringLiteral("Enabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\SettingSync\\Groups\\PackageState"), QStringLiteral("Enabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\SettingSync\\Groups\\Personalization"), QStringLiteral("Enabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\SettingSync\\Groups\\StartLayout"), QStringLiteral("Enabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\SettingSync\\Groups\\Windows"), QStringLiteral("Enabled"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.telemetry.account_sync");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Sync");
        t.name = QStringLiteral("Disable Windows account synchronization");
        t.description = QStringLiteral("Disables UE-V (User Experience Virtualization) agent, Windows 8 app settings sync, setting sync policies, and browser syncing via PolicyManager.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("UE-V roaming disabled"),
            QStringLiteral("Browser sync disabled"),
            QStringLiteral("All setting sync disabled at policy level"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\UEV\\Agent\\Configuration\\WindowsSettings"), QStringLiteral("DesktopSettings"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\UEV\\Agent\\Configuration\\WindowsSettings"), QStringLiteral("EaseOfAccessSettings"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\UEV\\Agent\\Configuration\\WindowsSettings"), QStringLiteral("ThemeSettings"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\UEV\\Agent\\Configuration\\WindowsSettings"), QStringLiteral("RoamingCredentialSettings"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\UEV\\Agent\\Configuration\\WindowsSettings"), QStringLiteral("NetworkPrinters"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\UEV\\Agent\\Configuration"), QStringLiteral("DontSyncWindows8AppSettings"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\UEV\\Agent\\Configuration"), QStringLiteral("SyncProviderPingEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\UEV\\Agent\\Configuration"), QStringLiteral("SyncUnlistedWindows8Apps"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\UEV\\Agent\\Configuration"), QStringLiteral("SyncEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\UEV\\Agent\\Configuration"), QStringLiteral("CustomerExperienceImprovementProgram"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\UEV\\Agent"), QStringLiteral("Enabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\Software\\Policies\\Microsoft\\Windows\\SettingSync"), QStringLiteral("DisableSettingSync"), 2u, 0u),
            dwordOp(QStringLiteral("HKLM\\Software\\Policies\\Microsoft\\Windows\\SettingSync"), QStringLiteral("DisableSettingSyncUserOverride"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\Experience\\DoNotSyncBrowserSettings"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\Experience\\AllowSyncMySettings"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\Experience\\PreventUsersFromTurningOnBrowserSyncing"), QStringLiteral("value"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.telemetry.credssp_logging");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Telemetry");
        t.name = QStringLiteral("Disable CredSSP debug logging");
        t.description = QStringLiteral("Disables CredSSP debug logging by setting the autologger DebugLogLevel to 0.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("CredSSP debug logging disabled"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\CredSSP"), QStringLiteral("DebugLogLevel"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.telemetry.defender_summary");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Notifications");
        t.name = QStringLiteral("Disable Defender summary notifications");
        t.description = QStringLiteral("Disables summary notifications for Windows Defender virus and threat protection.");
        t.iconName = QStringLiteral("shield");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Defender summary notifications disabled"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows Defender Security Center\\Virus and threat protection"), QStringLiteral("SummaryNotificationDisabled"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.telemetry.event_logging_extended");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Telemetry");
        t.name = QStringLiteral("Disable extended event logging");
        t.description = QStringLiteral("Disables WUDF logging, NT Reliability timestamps, CloudExperienceHost ETW logging, Wacom analytics, and extended SQM/CEIP settings.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Reliability Monitor and WUDF driver logs stop collecting; reduces post-mortem diagnostics"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\WUDF"), QStringLiteral("LogEnable"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\WUDF"), QStringLiteral("LogLevel"), 0u, 3u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CloudExperienceHost"), QStringLiteral("ETWLoggingEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows NT\\Reliability"), QStringLiteral("TimeStampEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows NT\\Reliability"), QStringLiteral("IncludeShutdownErrs"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows NT\\Reliability"), QStringLiteral("SnapShot"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\UnattendSettings\\SQMClient"), QStringLiteral("CEIPEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\Software\\Policies\\Microsoft\\Internet Explorer\\SQM"), QStringLiteral("DisableCustomerImprovementProgram"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\AppV\\CEIP"), QStringLiteral("CEIPEnable"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.telemetry.extended_diagnostics");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Telemetry");
        t.name = QStringLiteral("Disable extended diagnostic data collection");
        t.description = QStringLiteral("Disables diagnostic tracing, boot/shutdown CKCL settings, scheduled diagnostics, remote server queries, console tracing, StorPort telemetry, TelemetryController, and additional DataCollection policy values.");
        t.iconName = QStringLiteral("eye-off");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Scheduled diagnostic troubleshooters and online tip recommendations disabled"),
            QStringLiteral("DirectX compatibility database updates and OneSettings downloads blocked"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Diagnostics\\Performance"), QStringLiteral("DisableDiagnosticTracing"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Diagnostics\\Performance\\BootCKCLSettings"), QStringLiteral("Start"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Diagnostics\\Performance\\ShutdownCKCLSettings"), QStringLiteral("Start"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\ScriptedDiagnosticsProvider\\Policy"), QStringLiteral("DisableQueryRemoteServer"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\ScriptedDiagnosticsProvider\\Policy"), QStringLiteral("EnableQueryRemoteServer"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\ScriptedDiagnosticsProvider\\Policy"), QStringLiteral("EnableQueryRemoteServer"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\ScheduledDiagnostics"), QStringLiteral("EnabledExecution"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Tracing"), QStringLiteral("EnableConsoleTracing"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"), QStringLiteral("NoRemoteRecursiveEvents"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorPort"), QStringLiteral("TelemetryPerformanceEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorPort"), QStringLiteral("TelemetryErrorDataEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorPort"), QStringLiteral("TelemetryDeviceHealthEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\TelemetryController"), QStringLiteral("RunsBlocked"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\System\\AllowDeviceNameInDiagnosticData"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\DataCollection"), QStringLiteral("DisableTelemetryOptInChangeNotification"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\DataCollection"), QStringLiteral("DisableTelemetryOptInSettingsUx"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\DataCollection"), QStringLiteral("DisableDeviceDelete"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Policies\\DataCollection"), QStringLiteral("DisableOneSettingsDownloads"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Policies\\DataCollection"), QStringLiteral("DisableDeviceDelete"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\DataCollection"), QStringLiteral("DisableOneSettingsDownloads"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\DataCollection"), QStringLiteral("DisableDeviceDelete"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\System\\AllowMicrosoftManagedDesktopProcessing"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\System\\AllowUpdateComplianceProcessing"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\System\\AllowWUfBCloudProcessing"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\System\\AllowCommercialDataPipeline"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\System\\AllowDesktopAnalyticsProcessing"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\System\\DisableDeviceDelete"), QStringLiteral("value"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\System\\DisableDirectXDatabaseUpdate"), QStringLiteral("value"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\System\\DisableOneSettingsDownloads"), QStringLiteral("value"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\System\\EnableOneSettingsAuditing"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\System\\ConfigureTelemetryOptInChangeNotification"), QStringLiteral("value"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\System\\ConfigureTelemetryOptInSettingsUx"), QStringLiteral("value"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\System\\AllowEmbeddedMode"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\System\\AllowFontProviders"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\System\\AllowUserToResetPhone"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\Troubleshooting\\AllowRecommendations"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\Settings\\AllowOnlineTips"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SmartGlass"), QStringLiteral("BluetoothPolicy"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\safer\\codeidentifiers"), QStringLiteral("authenticodeenabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"), QStringLiteral("ShowSyncProviderNotifications"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.telemetry.feedback");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Feedback");
        t.name = QStringLiteral("Disable Windows feedback and SIUF");
        t.description = QStringLiteral("Disables SIUF (System Initiated User Feedback) prompts, Visual Studio feedback dialogs, and extended feedback policy settings.");
        t.iconName = QStringLiteral("palette");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Feedback prompts fully suppressed"),
            QStringLiteral("VS feedback dialog disabled"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Siuf\\Rules"), QStringLiteral("NumberOfSIUFInPeriod"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Siuf\\Rules"), QStringLiteral("PeriodInNanoSeconds"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\VisualStudio\\Feedback"), QStringLiteral("DisableFeedbackDialog"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\VisualStudio\\Feedback"), QStringLiteral("DisableEmailInput"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\VisualStudio\\Feedback"), QStringLiteral("DisableScreenshotCapture"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\System\\FeedbackHubAlwaysSaveDiagnosticsLocally"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\Experience\\DoNotShowFeedbackNotifications"), QStringLiteral("value"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Policies\\DataCollection"), QStringLiteral("DoNotShowFeedbackNotifications"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\DataCollection"), QStringLiteral("DoNotShowFeedbackNotifications"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.telemetry.input_extended");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Input & typing");
        t.name = QStringLiteral("Disable extended input and typing telemetry");
        t.description = QStringLiteral("Disables linguistic data collection, keyboard text suggestions, hardware keyboard suggestions, IME logging, inking/typing personalization CPSS policies, handwriting data sharing and error reports.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Text suggestions disabled"),
            QStringLiteral("IME learning disabled"),
            QStringLiteral("Handwriting personalization disabled"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\TextInput"), QStringLiteral("AllowLinguisticDataCollection"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\TextInput"), QStringLiteral("AllowHardwareKeyboardTextSuggestions"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\TextInput"), QStringLiteral("AllowTextInputSuggestionUpdate"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\TextInput"), QStringLiteral("AllowKeyboardTextSuggestions"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\InputPersonalization"), QStringLiteral("RestrictImplicitInkCollection"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\InputPersonalization"), QStringLiteral("RestrictImplicitTextCollection"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\InputPersonalization"), QStringLiteral("RestrictImplicitInkCollection"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\InputPersonalization"), QStringLiteral("RestrictImplicitTextCollection"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\TextInput\\AllowHardwareKeyboardTextSuggestions"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\TextInput\\AllowKeyboardTextSuggestions"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\TextInput\\AllowTextInputSuggestionUpdate"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\TextInput\\AllowLinguisticDataCollection"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\TextInput\\AllowIMELogging"), QStringLiteral("value"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CPSS\\UserPolicy\\ImproveInkingAndTyping"), QStringLiteral("DefaultValue"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CPSS\\UserPolicy\\InkingAndTypingPersonalization"), QStringLiteral("DefaultValue"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CPSS\\DevicePolicy\\InkingAndTypingPersonalization"), QStringLiteral("DefaultValue"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CPSS\\DevicePolicy\\ImproveInkingAndTyping"), QStringLiteral("DefaultValue"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\TabletPC"), QStringLiteral("PreventHandwritingDataSharing"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\HandwritingErrorReports"), QStringLiteral("PreventHandwritingErrorReports"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\IME\\Shared"), QStringLiteral("UserDict"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\IME\\Shared"), QStringLiteral("UseHistorybasedPredictiveInput"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\IME\\Shared"), QStringLiteral("misconvlogging"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.telemetry.messaging_health");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Telemetry");
        t.name = QStringLiteral("Disable messaging cloud sync and device health");
        t.description = QStringLiteral("Disables messaging cloud service sync and Device Health Attestation Service.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Messaging cloud sync disabled"),
            QStringLiteral("Device health attestation disabled"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Messaging"), QStringLiteral("CloudServiceSyncEnabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\DeviceHealthAttestationService"), QStringLiteral("EnableDeviceHealthAttestationService"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.telemetry.onedrive_policy");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("OneDrive");
        t.name = QStringLiteral("Disable OneDrive via Group Policy");
        t.description = QStringLiteral("Disables OneDrive file sync, NGSC sync, metered network sync, and prevents libraries from defaulting to OneDrive save locations via Group Policy.");
        t.iconName = QStringLiteral("download");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("OneDrive file sync blocked entirely; documents saved locally no longer back up to the cloud"),
            QStringLiteral("Office auto-save to OneDrive fails"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\OneDrive"), QStringLiteral("DisableFileSync"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\OneDrive"), QStringLiteral("DisableFileSyncNGSC"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\OneDrive"), QStringLiteral("DisableMeteredNetworkFileSync"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\OneDrive"), QStringLiteral("DisableLibrariesDefaultSaveToOneDrive"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\OneDrive"), QStringLiteral("DisablePersonalSync"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.telemetry.storage_sense");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Storage");
        t.name = QStringLiteral("Disable Storage Sense telemetry");
        t.description = QStringLiteral("Turns off Storage Sense, Windows' automatic disk-cleanup feature (temp files, Recycle Bin, etc.). This is a maintenance toggle, not a telemetry or privacy setting; disabling it means Windows no longer frees space automatically.");
        t.iconName = QStringLiteral("usb");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Storage Sense automatic cleanup disabled"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\StorageSense"), QStringLiteral("AllowStorageSenseGlobal"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\StorageSense"), QStringLiteral("AllowStorageSenseTemporaryFilesCleanup"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.telemetry.wer_extended");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Error reporting");
        t.name = QStringLiteral("Disable extended Windows Error Reporting");
        t.description = QStringLiteral("Disables additional WER features including archive, UI, auto-approve dumps, WMR, unattend WER settings, and extended PCHealth error reporting DW settings.");
        t.iconName = QStringLiteral("settings");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Windows Error Reporting archive, UI prompts, and second-level data collection all disabled"),
            QStringLiteral("App crash diagnostics no longer uploaded or cached locally for later review"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\Windows Error Reporting"), QStringLiteral("AutoApproveOSDumps"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\Windows Error Reporting"), QStringLiteral("DontShowUI"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\Windows Error Reporting"), QStringLiteral("LoggingDisabled"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\Windows Error Reporting"), QStringLiteral("ConfigureArchive"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\Windows Error Reporting"), QStringLiteral("DisableArchive"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\Windows Error Reporting"), QStringLiteral("BypassDataThrottling"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Error Reporting"), QStringLiteral("DontShowUI"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Error Reporting"), QStringLiteral("AutoApproveOSDumps"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Error Reporting"), QStringLiteral("ConfigureArchive"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Error Reporting"), QStringLiteral("DisableArchive"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Error Reporting"), QStringLiteral("DontSendAdditionalData"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Error Reporting"), QStringLiteral("LoggingDisabled"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Error Reporting"), QStringLiteral("BypassDataThrottling"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Error Reporting"), QStringLiteral("Disabled"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Error Reporting"), QStringLiteral("DontShowUI"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Error Reporting"), QStringLiteral("DoReport"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Error Reporting"), QStringLiteral("LoggingDisabled"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\Windows Error Reporting\\Assert Filtering Policy"), QStringLiteral("ReportAndContinue"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\Windows Error Reporting\\BrokerUp"), QStringLiteral("WWAJSE"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\Windows Error Reporting\\Consent"), QStringLiteral("NewUserDefaultConsent"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\Windows Error Reporting\\WMR"), QStringLiteral("Disable"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\UnattendSettings\\Windows Error Reporting"), QStringLiteral("DisableWER"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\UnattendSettings\\Windows Error Reporting\\Consent"), QStringLiteral("DefaultConsent"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\PCHealth\\ErrorReporting"), QStringLiteral("ForceQueueMode"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\PCHealth\\ErrorReporting\\DW"), QStringLiteral("DWNoExternalURL"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\PCHealth\\ErrorReporting\\DW"), QStringLiteral("DWNoFileCollection"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\PCHealth\\ErrorReporting\\DW"), QStringLiteral("DWNoSecondLevelCollection"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.update.extended_controls");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Windows Update");
        t.name = QStringLiteral("Windows Update extended privacy controls");
        t.description = QStringLiteral("Extends Windows Update privacy controls: excludes driver updates from quality updates, disables Delivery Optimization peer-to-peer bandwidth sharing, prevents Malicious Software Removal Tool delivery, and disables speech model updates. Preserves user control over update UX while reducing background data transmission.");
        t.iconName = QStringLiteral("rotate");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Driver updates no longer delivered through Windows quality updates"),
            QStringLiteral("Malicious Software Removal Tool (MRT) no longer auto-delivered"),
            QStringLiteral("Speech recognition model updates blocked"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate"), QStringLiteral("ExcludeWUDriversInQualityUpdate"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate"), QStringLiteral("SetDisablePauseUXAccess"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate"), QStringLiteral("SetDisableUXWUAccess"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate\\AU"), QStringLiteral("NoAutoRebootWithLoggedOnUsers"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DeliveryOptimization\\Config"), QStringLiteral("DODownloadMode"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\MRT"), QStringLiteral("DontOfferThroughWUAU"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Speech"), QStringLiteral("AllowSpeechModelUpdate"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.appdiag.office_telemetry");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Scheduled tasks");
        t.name = QStringLiteral("Disable Office diagnostic telemetry");
        t.description = QStringLiteral("Disables Microsoft Office diagnostic data collection across Office 2013/2016/365 (QM, feedback, Outlook/Word logging, OSM upload, client telemetry) and the Office telemetry scheduled tasks.");
        t.iconName = QStringLiteral("file-minus");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        CommandOp to0;
        to0.program = QStringLiteral("schtasks");
        to0.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Office\\OfficeTelemetryAgentFallBack"), QStringLiteral("/Disable") };
        to0.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Office\\OfficeTelemetryAgentFallBack"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to0);
        CommandOp to1;
        to1.program = QStringLiteral("schtasks");
        to1.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Office\\OfficeTelemetryAgentLogOn"), QStringLiteral("/Disable") };
        to1.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Office\\OfficeTelemetryAgentLogOn"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to1);
        CommandOp to2;
        to2.program = QStringLiteral("schtasks");
        to2.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Office\\OfficeTelemetryAgentFallBack2016"), QStringLiteral("/Disable") };
        to2.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Office\\OfficeTelemetryAgentFallBack2016"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to2);
        CommandOp to3;
        to3.program = QStringLiteral("schtasks");
        to3.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Office\\OfficeTelemetryAgentLogOn2016"), QStringLiteral("/Disable") };
        to3.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Office\\OfficeTelemetryAgentLogOn2016"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to3);
        CommandOp to4;
        to4.program = QStringLiteral("schtasks");
        to4.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Office\\Office 15 Subscription Heartbeat"), QStringLiteral("/Disable") };
        to4.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Office\\Office 15 Subscription Heartbeat"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to4);
        CommandOp to5;
        to5.program = QStringLiteral("schtasks");
        to5.applyArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Office\\Office 16 Subscription Heartbeat"), QStringLiteral("/Disable") };
        to5.revertArgs = { QStringLiteral("/Change"), QStringLiteral("/TN"), QStringLiteral("\\Microsoft\\Office\\Office 16 Subscription Heartbeat"), QStringLiteral("/Enable") };
        t.commandOps.push_back(to5);
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.telemetry.cert_chain");
        t.category = QStringLiteral("privacy");
        t.name = QStringLiteral("Restrict certificate chain engine network access");
        t.description = QStringLiteral("Reduces certificate chain revocation check timeouts and cross-cert download intervals to minimize network telemetry from certificate validation.");
        t.iconName = QStringLiteral("wifi");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Certificate revocation checks time out aggressively; revoked certs may briefly appear valid on slow networks"),
            QStringLiteral("Cross-certificate updates throttled to weekly"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\SystemCertificates\\ChainEngine\\Config"), QStringLiteral("ChainRevAccumulativeUrlRetrievalTimeoutMilliseconds"), 14u, 20000u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\SystemCertificates\\ChainEngine\\Config"), QStringLiteral("ChainUrlRetrievalTimeoutMilliseconds"), 14u, 15000u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\SystemCertificates\\ChainEngine\\Config"), QStringLiteral("CrossCertDownloadIntervalHours"), 168u, 672u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\SystemCertificates\\ChainEngine\\Config"), QStringLiteral("Options"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.telemetry.crash_dumps");
        t.category = QStringLiteral("privacy");
        t.name = QStringLiteral("Disable crash dump collection");
        t.description = QStringLiteral("Disables crash dump generation, display, auto-reboot on crash, and sets error mode to suppress dialogs.");
        t.iconName = QStringLiteral("eye-off");
        t.impact = Impact::High;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Kernel crash dumps no longer written; BSOD troubleshooting will have no memory dump to analyze"),
            QStringLiteral("Auto-reboot after BSOD disabled; system hangs on crash screen until manually restarted"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\CrashControl"), QStringLiteral("DisplayDisabled"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\CrashControl"), QStringLiteral("AutoReboot"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\CrashControl"), QStringLiteral("CrashDumpEnabled"), 0u, 7u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\CrashControl"), QStringLiteral("DumpLogLevel"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\CrashControl"), QStringLiteral("EnableLogFile"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\CrashControl"), QStringLiteral("LogEvent"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\CrashControl"), QStringLiteral("MinidumpsCount"), 0u, 50u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\CrashControl"), QStringLiteral("Overwrite"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\CrashControl"), QStringLiteral("AlwaysKeepMemoryDump"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\CrashControl\\LiveKernelReports"), QStringLiteral("DeleteLiveMiniDumps"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Windows"), QStringLiteral("ErrorMode"), 2u, 0u),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Windows"), QStringLiteral("ShellErrorMode"), 0u, 1u),
        };
        c.push_back(t);
    }
}

} // namespace vth::tweaks::detail
