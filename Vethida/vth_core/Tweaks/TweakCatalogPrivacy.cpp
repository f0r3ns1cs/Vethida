#include "TweakEngine.h"

// Privacy and telemetry, category "privacy"; security hardening is category
// "security" beside it. Location, camera and mic consent is covered by the
// capability-consent entries below. UE-V and the PolicyManager copy of
// DataCollection are left out as redundant.

namespace vth::tweaks::detail {

void appendPrivacy(QVector<TweakSpec>& c) {
    {
        TweakSpec t;
        t.id = QStringLiteral("priv.telemetry-off");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Telemetry");
        t.name = QStringLiteral("Disable Windows telemetry");
        t.description = QStringLiteral(
            "Sets the diagnostic-data policy to its minimum and disables the "
            "application-inventory/compatibility telemetry collectors.");
        t.iconName = QStringLiteral("eye-off");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        const QString dc = QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\DataCollection");
        const QString ac = QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\AppCompat");
        t.registryOps = {
            dwordOp(dc, QStringLiteral("AllowTelemetry"), 0, std::nullopt),
            dwordOp(dc, QStringLiteral("MaxTelemetryAllowed"), 0, std::nullopt),
            dwordOp(dc, QStringLiteral("AllowDeviceNameInTelemetry"), 0, std::nullopt),
            dwordOp(dc, QStringLiteral("DoNotShowFeedbackNotifications"), 1, std::nullopt),
            dwordOp(dc, QStringLiteral("AllowCommercialDataPipeline"), 0, std::nullopt),
            dwordOp(dc, QStringLiteral("DisableOneSettingsDownloads"), 1, std::nullopt),
            dwordOp(ac, QStringLiteral("AITEnable"), 0, std::nullopt),
            dwordOp(ac, QStringLiteral("DisableInventory"), 1, std::nullopt),
            dwordOp(ac, QStringLiteral("DisablePCA"), 1, std::nullopt),
            dwordOp(ac, QStringLiteral("DisableUAR"), 1, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.copilot-recall-off");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Copilot & Recall");
        t.name = QStringLiteral("Disable Copilot and Recall");
        t.description = QStringLiteral(
            "Turns off Windows Copilot and the Recall snapshot/AI-data-analysis feature.");
        t.iconName = QStringLiteral("bot");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsCopilot"),
                    QStringLiteral("TurnOffWindowsCopilot"), 1, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsAI"),
                    QStringLiteral("DisableAIDataAnalysis"), 1, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsAI"),
                    QStringLiteral("TurnOffSavingSnapshots"), 1, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.cortana-off");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Search & Cortana");
        t.name = QStringLiteral("Disable Cortana and web search");
        t.description = QStringLiteral(
            "Disables Cortana and removes web results from Start-menu search.");
        t.iconName = QStringLiteral("search");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        const QString ws = QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search");
        t.registryOps = {
            dwordOp(ws, QStringLiteral("AllowCortana"), 0, std::nullopt),
            dwordOp(ws, QStringLiteral("AllowCortanaAboveLock"), 0, std::nullopt),
            dwordOp(ws, QStringLiteral("AllowSearchToUseLocation"), 0, std::nullopt),
            dwordOp(ws, QStringLiteral("DisableWebSearch"), 1, std::nullopt),
            dwordOp(ws, QStringLiteral("ConnectedSearchUseWeb"), 0, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.content-delivery-off");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Suggested content & ads");
        t.name = QStringLiteral("Disable suggested content and ads");
        t.description = QStringLiteral(
            "Turns off Start-menu suggestions, lock-screen spotlight ads, silent app "
            "installs and the rest of the Content Delivery Manager surface.");
        t.iconName = QStringLiteral("megaphone-off");
        t.impact = Impact::Low;
        t.requiresAdmin = false;
        const QString cdm = QStringLiteral(
            "HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager");
        QVector<RegistryOp> ops;
        for (const QString& name : {
            QStringLiteral("ContentDeliveryAllowed"),
            QStringLiteral("FeatureManagementEnabled"),
            QStringLiteral("OEMPreInstalledAppsEnabled"),
            QStringLiteral("PreInstalledAppsEnabled"),
            QStringLiteral("PreInstalledAppsEverEnabled"),
            QStringLiteral("RotatingLockScreenEnabled"),
            QStringLiteral("RotatingLockScreenOverlayEnabled"),
            QStringLiteral("SilentInstalledAppsEnabled"),
            QStringLiteral("SoftLandingEnabled"),
            QStringLiteral("SystemPaneSuggestionsEnabled"),
            QStringLiteral("SubscribedContent-310093Enabled"),
            QStringLiteral("SubscribedContent-338387Enabled"),
            QStringLiteral("SubscribedContent-338388Enabled"),
            QStringLiteral("SubscribedContent-338389Enabled"),
            QStringLiteral("SubscribedContent-338393Enabled"),
            QStringLiteral("SubscribedContent-353694Enabled"),
            QStringLiteral("SubscribedContent-353696Enabled"),
            QStringLiteral("SubscribedContent-353698Enabled") })
            ops.push_back(dwordOp(cdm, name, 0, 1));
        t.registryOps = ops;
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.advertising-id-off");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Advertising ID");
        t.name = QStringLiteral("Disable Advertising ID");
        t.description = QStringLiteral("Disables the per-user advertising ID used for ad personalization.");
        t.iconName = QStringLiteral("eye-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\AdvertisingInfo"),
                    QStringLiteral("DisabledByGroupPolicy"), 1, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.activity-history-off");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Activity history");
        t.name = QStringLiteral("Disable Activity History / timeline");
        t.description = QStringLiteral(
            "Stops Windows collecting and uploading your activity history.");
        t.iconName = QStringLiteral("eye-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        const QString sys = QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System");
        t.registryOps = {
            dwordOp(sys, QStringLiteral("EnableActivityFeed"), 0, std::nullopt),
            dwordOp(sys, QStringLiteral("PublishUserActivities"), 0, std::nullopt),
            dwordOp(sys, QStringLiteral("UploadUserActivities"), 0, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.consent-store-privacy");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Capability consent");
        t.name = QStringLiteral("Deny extra capability consent + GameDVR/Speech privacy");
        t.description = QStringLiteral(
            "Denies notification-listener/location/app-diagnostics/account-info capability "
            "consent, disables Game DVR at the policy level, turns off Game Bar's auto "
            "mode and mode-change notifications, opts out of the browser's regional-language "
            "header, online speech recognition and inking/typing personalization, and "
            "disables event-transcript telemetry.");
        t.iconName = QStringLiteral("eye-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.registryOps = {
            stringOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\DirectX\\UserGpuPreferences"),
                     QStringLiteral("DirectXUserGlobalSettings"), QStringLiteral("VRROptimizeEnable=0;"), std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SettingSync"),
                    QStringLiteral("SyncPolicy"), 5, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\ApplicationManagement\\AllowGameDVR"),
                    QStringLiteral("value"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\GameBar"),
                    QStringLiteral("AutoGameModeEnabled"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\GameBar"),
                    QStringLiteral("ShowGameModeNotifications"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Control Panel\\International\\User Profile"),
                    QStringLiteral("HttpAcceptLanguageOptOut"), 1, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"),
                    QStringLiteral("DisableTailoredExperiencesWithDiagnosticData"), 1, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Speech_OneCore\\Settings\\OnlineSpeechPrivacy"),
                    QStringLiteral("HasAccepted"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Personalization\\Settings"),
                    QStringLiteral("AcceptedPrivacyPolicy"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\InputPersonalization"),
                    QStringLiteral("RestrictImplicitInkCollection"), 1, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\InputPersonalization"),
                    QStringLiteral("RestrictImplicitTextCollection"), 1, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\InputPersonalization\\TrainedDataStore"),
                    QStringLiteral("HarvestContacts"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Diagnostics\\DiagTrack"),
                    QStringLiteral("ShowedToastAtLevel"), 1, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Diagnostics\\DiagTrack\\EventTranscriptKey"),
                    QStringLiteral("EnableEventTranscript"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Privacy"),
                    QStringLiteral("TailoredExperiencesWithDiagnosticDataEnabled"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Siuf\\Rules"),
                    QStringLiteral("NumberOfSIUFInPeriod"), 0, std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\userNotificationListener"),
                     QStringLiteral("Value"), QStringLiteral("Deny"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\location"),
                     QStringLiteral("Value"), QStringLiteral("Deny"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\appDiagnostics"),
                     QStringLiteral("Value"), QStringLiteral("Deny"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\userAccountInformation"),
                     QStringLiteral("Value"), QStringLiteral("Deny"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.telemetry-data-collection-extra");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Telemetry");
        t.name = QStringLiteral("Extra DataCollection and text-input telemetry");
        t.description = QStringLiteral(
            "Locks down the DataCollection policy tree beyond the basic telemetry toggle "
            "(diagnostic log limits, Desktop Analytics/Update Compliance/WUfB cloud "
            "processing, dump collection, Edge data opt-in), disables StorPort device-health "
            "telemetry, opts out of PowerShell's own telemetry, and turns off hardware/text/"
            "IME input-suggestion logging.");
        t.iconName = QStringLiteral("eye-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        const QString dc2 = QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\DataCollection");
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\TelemetryController"),
                    QStringLiteral("RunsBlocked"), 1, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorPort"),
                    QStringLiteral("TelemetryPerformanceEnabled"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorPort"),
                    QStringLiteral("TelemetryErrorDataEnabled"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\StorPort"),
                    QStringLiteral("TelemetryDeviceHealthEnabled"), 0, std::nullopt),
            dwordOp(dc2, QStringLiteral("LimitDiagnosticLogCollection"), 1, std::nullopt),
            dwordOp(dc2, QStringLiteral("DisableTelemetryOptInSettingsUx"), 1, std::nullopt),
            dwordOp(dc2, QStringLiteral("AllowDesktopAnalyticsProcessing"), 0, std::nullopt),
            dwordOp(dc2, QStringLiteral("AllowUpdateComplianceProcessing"), 0, std::nullopt),
            dwordOp(dc2, QStringLiteral("AllowWUfBCloudProcessing"), 0, std::nullopt),
            dwordOp(dc2, QStringLiteral("DisableDeviceDelete"), 1, std::nullopt),
            dwordOp(dc2, QStringLiteral("EnableExtendedBooksTelemetry"), 0, std::nullopt),
            stringOp(dc2, QStringLiteral("TelemetryProxyServer"), QString(), std::nullopt),
            stringOp(dc2, QStringLiteral("ConfigureMicrosoft365UploadEndpoint"), QString(), std::nullopt),
            dwordOp(dc2, QStringLiteral("DisableTelemetryOptInChangeNotification"), 1, std::nullopt),
            dwordOp(dc2, QStringLiteral("LimitEnhancedDiagnosticDataWindowsAnalytics"), 0, std::nullopt),
            dwordOp(dc2, QStringLiteral("LimitDumpCollection"), 1, std::nullopt),
            dwordOp(dc2, QStringLiteral("MicrosoftEdgeDataOptIn"), 0, std::nullopt),
            stringOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"),
                     QStringLiteral("POWERSHELL_TELEMETRY_OPTOUT"), QStringLiteral("1"), std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Peernet"),
                    QStringLiteral("Disabled"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\WiFi\\AllowWiFiHotSpotReporting"),
                    QStringLiteral("value"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\TextInput"),
                    QStringLiteral("AllowHardwareKeyboardTextSuggestions"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\TextInput"),
                    QStringLiteral("AllowTextInputSuggestionUpdate"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\TextInput"),
                    QStringLiteral("AllowKeyboardTextSuggestions"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\InputPersonalization"),
                    QStringLiteral("Installed"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\InputPersonalization"),
                    QStringLiteral("Shutdown"), 1, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\InputPersonalization"),
                    QStringLiteral("HarvestContacts"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\InputPersonalization\\TrainedDataStore"),
                    QStringLiteral("InsightsEnabled"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\InputPersonalization\\TrainedDataStore"),
                    QStringLiteral("LMDataLoggerEnabled"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\TextInput\\AllowHardwareKeyboardTextSuggestions"),
                    QStringLiteral("value"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\TextInput\\AllowKeyboardTextSuggestions"),
                    QStringLiteral("value"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\TextInput\\AllowTextInputSuggestionUpdate"),
                    QStringLiteral("value"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\TextInput\\AllowIMELogging"),
                    QStringLiteral("value"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CPSS\\UserPolicy\\InkingAndTypingPersonalization"),
                    QStringLiteral("DefaultValue"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\IME\\Shared"),
                    QStringLiteral("UserDict"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\IME\\Shared"),
                    QStringLiteral("UseHistorybasedPredictiveInput"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\IME\\Shared"),
                    QStringLiteral("misconvlogging"), 0, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.system-policy-privacy-misc");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("System privacy");
        t.name = QStringLiteral("System policy privacy miscellany");
        t.description = QStringLiteral("Disables cross-device experiences (CDP/Continuum), DISABLES clipboard history and cross-device clipboard entirely (does not merely keep it local), turns off location and sensors, Find My Device, CEIP/SQM telemetry, Windows Error Reporting, cloud consumer-content/Spotlight, voice activation, and driver-not-found WER reports across the three policy locations Windows checks.");
        t.iconName = QStringLiteral("eye-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System"),
                    QStringLiteral("AllowClipboardHistory"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System"),
                    QStringLiteral("EnableCdp"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\ScheduledDiagnostics"),
                    QStringLiteral("EnabledExecution"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\ScheduledDiagnostics"),
                    QStringLiteral("EnabledExecution"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\current\\device\\Bluetooth"),
                    QStringLiteral("AllowAdvertising"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\LocationAndSensors"),
                    QStringLiteral("DisableLocation"), 1, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\LocationAndSensors"),
                    QStringLiteral("DisableLocationScripting"), 1, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\LocationAndSensors"),
                    QStringLiteral("DisableSensors"), 1, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\LocationAndSensors"),
                    QStringLiteral("DisableWindowsLocationProvider"), 1, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\System\\AllowLocation"),
                    QStringLiteral("value"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\current\\device\\System"),
                    QStringLiteral("AllowLocation"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\SQMClient\\Windows"),
                    QStringLiteral("CEIPEnable"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\SQMClient\\Windows"),
                    QStringLiteral("DisableOptinExperience"), 1, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\CloudContent"),
                    QStringLiteral("DisableSoftLanding"), 1, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\CloudContent"),
                    QStringLiteral("DisableWindowsConsumerFeatures"), 1, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\CloudContent"),
                    QStringLiteral("DisableWindowsSpotlightFeatures"), 1, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\CloudContent"),
                    QStringLiteral("DisableConsumerAccountStateContent"), 1, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\CloudContent"),
                    QStringLiteral("DisableCloudOptimizedContent"), 1, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Assistance\\Client\\1.0"),
                    QStringLiteral("NoExplicitFeedback"), 1, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Assistance\\Client\\1.0"),
                    QStringLiteral("NoActiveHelp"), 1, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\EdgeUI"),
                    QStringLiteral("DisableMFUTracking"), 1, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\FindMyDevice"),
                    QStringLiteral("AllowFindMyDevice"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Settings\\FindMyDevice"),
                    QStringLiteral("LocationSyncEnabled"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\DeviceInstall\\Settings"),
                    QStringLiteral("DisableSendGenericDriverNotFoundToWER"), 1, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SYSTEM\\DriverDatabase\\Policies\\Settings"),
                    QStringLiteral("DisableSendGenericDriverNotFoundToWER"), 1, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\DeviceHealthAttestationService"),
                    QStringLiteral("DisableSendGenericDriverNotFoundToWER"), 1, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Attachments"),
                    QStringLiteral("SaveZoneInformation"), 1, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CloudExperienceHost\\Intent\\business"),
                    QStringLiteral("Intent"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CloudExperienceHost\\Intent\\creative"),
                    QStringLiteral("Intent"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CloudExperienceHost\\Intent\\entertainment"),
                    QStringLiteral("Intent"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CloudExperienceHost\\Intent\\family"),
                    QStringLiteral("Intent"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CloudExperienceHost\\Intent\\gaming"),
                    QStringLiteral("Intent"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CloudExperienceHost\\Intent\\schoolwork"),
                    QStringLiteral("Intent"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Clipboard"),
                    QStringLiteral("AllowCrossDeviceClipboard"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Clipboard"),
                    QStringLiteral("EnableClipboardHistory"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SearchSettings"),
                    QStringLiteral("SafeSearchMode"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SearchSettings"),
                    QStringLiteral("IsDeviceSearchHistoryEnabled"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Policies\\Microsoft\\WindowsMediaPlayer"),
                    QStringLiteral("PreventCodecDownload"), 1, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Speech_OneCore\\Settings\\VoiceActivation\\UserPreferenceForAllApps"),
                    QStringLiteral("AgentActivationEnabled"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Speech_OneCore\\Settings\\VoiceActivation\\UserPreferenceForAllApps"),
                    QStringLiteral("AgentActivationOnLockScreenEnabled"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Speech_OneCore\\Settings\\VoiceActivation\\UserPreferenceForAllApps"),
                    QStringLiteral("AgentActivationLastUsed"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Windows Search"),
                    QStringLiteral("CortanaConsent"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\Windows Error Reporting"),
                    QStringLiteral("DontSendAdditionalData"), 1, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"),
                    QStringLiteral("DisableWindowsConsumerFeatures"), 1, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Policies\\Microsoft\\Windows\\Explorer"),
                    QStringLiteral("ClearTilesOnExit"), 1, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\InputPersonalization"),
                    QStringLiteral("AllowInputPersonalization"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\current\\device\\Browser"),
                    QStringLiteral("AllowAddressBarDropdown"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Maps"),
                    QStringLiteral("AutoDownloadAndUpdateMapData"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Maps"),
                    QStringLiteral("AllowUntriggeredNetworkTrafficOnSettingsPage"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Messaging"),
                    QStringLiteral("AllowMessageSync"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Speech"),
                    QStringLiteral("AllowSpeechModelUpdate"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\PCHealth\\ErrorReporting"),
                    QStringLiteral("DoReport"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Error Reporting"),
                    QStringLiteral("Disabled"), 1, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\Windows Error Reporting\\Escalation"),
                    QStringLiteral("DisableEscalation"), 1, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\default\\Settings\\AllowOnlineTips"),
                    QStringLiteral("value"), 0, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.accessibility-and-misc-privacy");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Misc privacy");
        t.name = QStringLiteral("Accessibility defaults and misc privacy");
        t.description = QStringLiteral(
            "Turns off dynamic scrollbars and resets the sticky/mouse/toggle-keys and "
            "keyboard-response accessibility filters to disabled; skips the first-run "
            "privacy-experience page and disables telemetry experimentation, handwriting "
            "data sharing and error reports, DRM online checks, WER dump auto-approval, "
            "lock-screen camera access, Defender's summary notification, and the "
            "\"you might like\" app-alert nag.");
        t.iconName = QStringLiteral("eye-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Control Panel\\Accessibility"),
                    QStringLiteral("DynamicScrollbars"), 0, std::nullopt),
            stringOp(QStringLiteral("HKCU\\Control Panel\\Accessibility\\MouseKeys"),
                     QStringLiteral("Flags"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Control Panel\\Accessibility\\StickyKeys"),
                     QStringLiteral("Flags"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Control Panel\\Accessibility\\Keyboard Response"),
                     QStringLiteral("Flags"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Control Panel\\Accessibility\\ToggleKeys"),
                     QStringLiteral("Flags"), QStringLiteral("0"), std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\OOBE"),
                    QStringLiteral("DisablePrivacyExperience"), 1, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows Defender Security Center\\"
                                   "Virus and threat protection"),
                    QStringLiteral("SummaryNotificationDisabled"), 1, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Explorer"),
                    QStringLiteral("NoNewAppAlert"), 1, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\current\\device\\System"),
                    QStringLiteral("AllowExperimentation"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Personalization\\Settings"),
                    QStringLiteral("RestrictImplicitInkCollection"), 1, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Siuf\\Rules"),
                    QStringLiteral("PeriodInNanoSeconds"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Diagnostics\\DiagTrack"),
                    QStringLiteral("DiagTrackAuthorization"), 7, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Personalization"),
                    QStringLiteral("NoLockScreenCamera"), 1, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\WMDRM"),
                    QStringLiteral("DisableOnline"), 1, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Error Reporting"),
                    QStringLiteral("AutoApproveOSDumps"), 0, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Error Reporting"),
                    QStringLiteral("DontSendAdditionalData"), 1, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\TabletPC"),
                    QStringLiteral("PreventHandwritingDataSharing"), 1, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\HandwritingErrorReports"),
                    QStringLiteral("PreventHandwritingErrorReports"), 1, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\TextInput"),
                    QStringLiteral("AllowLinguisticDataCollection"), 0, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.login-screen-privacy");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("Sign-in screen");
        t.name = QStringLiteral("Login screen privacy");
        t.description = QStringLiteral(
            "Hides the last-signed-in username and locked-user identity on the sign-in "
            "screen, shows last-logon info, and removes the logon background image.");
        t.iconName = QStringLiteral("lock");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        const QString sysp = QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System");
        const QString psys = QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System");
        t.registryOps = {
            dwordOp(sysp, QStringLiteral("DontDisplayLastUserName"), 1, 0),
            dwordOp(sysp, QStringLiteral("DontDisplayLockedUserId"), 3, 1),
            dwordOp(sysp, QStringLiteral("DisplayLastLogonInfo"), 1, 0),
            dwordOp(psys, QStringLiteral("DisableLogonBackgroundImage"), 1, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.ime-telemetry-off");
        t.category = QStringLiteral("privacy");
        t.subCategory = QStringLiteral("IME");
        t.name = QStringLiteral("Disable IME telemetry");
        t.description = QStringLiteral(
            "Turns off the IME cloud-candidate and lexicon-update network features that "
            "send typing data to Microsoft.");
        t.iconName = QStringLiteral("eye-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        const QString ime = QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\IME\\Shared");
        t.registryOps = {
            dwordOp(ime, QStringLiteral("Enable Cloud Candidate"), 0, std::nullopt),
            dwordOp(ime, QStringLiteral("Enable Lexicon Update"), 0, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("priv.consent-and-sync-extra");
        t.category = QStringLiteral("privacy");
        t.name = QStringLiteral("Block app access to personal data");
        t.description = QStringLiteral(
            "Stops apps reaching your camera, contacts, calendar, email, messages, call "
            "history and personal folders. Also turns off syncing your settings and saved "
            "passwords between PCs, online search results in the Start menu, and Edge and "
            "OneDrive starting themselves in the background.");
        t.iconName = QStringLiteral("eye-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        const QString ss = QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\SettingSync");
        const QString cap = QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\");
        QVector<RegistryOp> o;
        o.push_back(dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager"),
                            QStringLiteral("OemPreInstalledAppsEnabled"), 0, std::nullopt));
        o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\PushToInstall"),
                            QStringLiteral("DisablePushToInstall"), 1, std::nullopt));
        o.push_back(stringOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DeviceAccess\\Global\\LooselyCoupled"),
                             QStringLiteral("Value"), QStringLiteral("Deny"), std::nullopt));
        for (const QString& cap_name : {
                QStringLiteral("webcam"), QStringLiteral("contacts"), QStringLiteral("videosLibrary"),
                QStringLiteral("appointments"), QStringLiteral("phoneCallHistory"), QStringLiteral("email"),
                QStringLiteral("userDataTasks"), QStringLiteral("chat"), QStringLiteral("activity"),
                QStringLiteral("documentsLibrary"), QStringLiteral("picturesLibrary"),
                QStringLiteral("broadFileSystemAccess"), QStringLiteral("phoneCall"), QStringLiteral("radios") })
            o.push_back(stringOp(cap + cap_name, QStringLiteral("Value"), QStringLiteral("Deny"), std::nullopt));
        o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SmartGlass"),
                            QStringLiteral("UserAuthPolicy"), 0, std::nullopt));
        o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SmartGlass"),
                            QStringLiteral("BluetoothPolicy"), 0, std::nullopt));
        o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\AppPrivacy"),
                            QStringLiteral("LetAppsActivateWithVoice"), 2, std::nullopt));
        o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\AppPrivacy"),
                            QStringLiteral("LetAppsGetDiagnosticInfo"), 2, std::nullopt));
        o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search"),
                            QStringLiteral("ConnectedSearchUseWebOverMeteredConnections"), 0, std::nullopt));
        o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search"),
                            QStringLiteral("AllowCloudSearch"), 0, std::nullopt));
        o.push_back(dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search"),
                            QStringLiteral("DeviceHistoryEnabled"), 0, std::nullopt));
        o.push_back(dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search"),
                            QStringLiteral("HistoryViewEnabled"), 0, std::nullopt));
        o.push_back(dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search"),
                            QStringLiteral("BingSearchEnabled"), 0, std::nullopt));
        o.push_back(dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search"),
                            QStringLiteral("AllowCloudSearch"), 0, std::nullopt));
        o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppHost"),
                            QStringLiteral("EnableWebContentEvaluation"), 0, std::nullopt));
        o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Explorer"),
                            QStringLiteral("DisableThumbsDBOnNetworkFolders"), 1, std::nullopt));
        o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Explorer"),
                            QStringLiteral("DisableSearchBoxSuggestions"), 1, std::nullopt));
        o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Authentication\\LogonUI\\SessionData"),
                            QStringLiteral("AllowLockScreen"), 0, std::nullopt));
        o.push_back(stringOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\AutoComplete"),
                             QStringLiteral("AutoSuggest"), QStringLiteral("no"), std::nullopt));
        o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows Defender\\Spynet"),
                            QStringLiteral("SpyNetReporting"), 0, std::nullopt));
        o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows Defender\\Spynet"),
                            QStringLiteral("DontReportInfectionInformation"), 1, std::nullopt));
        o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows Defender\\Spynet"),
                            QStringLiteral("DontOfferThroughWUAU"), 1, std::nullopt));
        o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\EdgeUpdate"),
                            QStringLiteral("DoNotUpdateToEdgeWithChromium"), 1, std::nullopt));
        o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\MicrosoftEdge\\Main"),
                            QStringLiteral("AllowPrelaunch"), 0, std::nullopt));
        o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\MicrosoftEdge\\TabPreloader"),
                            QStringLiteral("AllowTabPreloading"), 0, std::nullopt));
        o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\EdgeUpdate"),
                            QStringLiteral("CreateDesktopShortcutDefault"), 0, std::nullopt));
        o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Onedrive"),
                            QStringLiteral("DisableLibrariesDefaultSaveToOneDrive"), 1, std::nullopt));
        o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Onedrive"),
                            QStringLiteral("DisableFileSync"), 1, std::nullopt));
        o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Onedrive"),
                            QStringLiteral("DisableMeteredNetworkFileSync"), 0, std::nullopt));
        o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\OneDrive"),
                            QStringLiteral("KFMBlockOptIn"), 1, std::nullopt));
        o.push_back(dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\OneDrive"),
                            QStringLiteral("DisablePersonalSync"), 1, std::nullopt));
        o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows"),
                            QStringLiteral("DisableFileSyncNGSC"), 1, std::nullopt));
        o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Reliability"),
                            QStringLiteral("TimeStampEnabled"), 0, std::nullopt));
        o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\CurrentVersion\\PushNotifications"),
                            QStringLiteral("NoTileApplicationNotification"), 1, std::nullopt));
        o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\CurrentVersion\\PushNotifications"),
                            QStringLiteral("DisallowNotificationMirroring"), 1, std::nullopt));
        o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Shell\\ActionCenter\\Quick Actions"),
                            QStringLiteral("PinnedQuickActionSlotCount"), 0, std::nullopt));
        o.push_back(dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Windows Search"),
                            QStringLiteral("CortanaConsent"), 0, std::nullopt));
        o.push_back(dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Windows Search"),
                            QStringLiteral("EnableDynamicContentInWSB"), 0, std::nullopt));
        o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\WindowsInkWorkspace"),
                            QStringLiteral("AllowWindowsInkWorkspace"), 0, std::nullopt));
        o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Explorer"),
                            QStringLiteral("NoUseStoreOpenWith"), 1, std::nullopt));
        o.push_back(dwordOp(ss, QStringLiteral("DisableSettingSync"), 2, std::nullopt));
        o.push_back(dwordOp(ss, QStringLiteral("DisableSettingSyncUserOverride"), 1, std::nullopt));
        o.push_back(dwordOp(ss, QStringLiteral("DisableSyncOnPaidNetwork"), 1, std::nullopt));
        o.push_back(dwordOp(ss, QStringLiteral("DisableCredentialsSettingSync"), 2, std::nullopt));
        o.push_back(dwordOp(ss, QStringLiteral("DisableCredentialsSettingSyncUserOverride"), 1, std::nullopt));
        o.push_back(dwordOp(ss, QStringLiteral("DisableApplicationSettingSync"), 2, std::nullopt));
        o.push_back(dwordOp(ss, QStringLiteral("DisableApplicationSettingSyncUserOverride"), 1, std::nullopt));
        t.registryOps = o;
        c.push_back(t);
    }
}

} // namespace vth::tweaks::detail
