#include "TweakEngine.h"

namespace vth::tweaks::detail {

void appendDefsBrowserOffice(QVector<TweakSpec>& c) {
    {
        TweakSpec t;
        t.id = QStringLiteral("bo.chrome.genai");
        t.category = QStringLiteral("browseroffice");
        t.subCategory = QStringLiteral("Chrome");
        t.name = QStringLiteral("Disable Chrome AI and GenAI features");
        t.description = QStringLiteral("Disables Chrome's AI-powered features including HelpMeWrite, tab organizer, history search, theme creation, DevTools GenAI, and local foundational model settings.");
        t.iconName = QStringLiteral("globe");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Chrome AI features will not be available"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Chrome"), QStringLiteral("HelpMeWriteSettings"), 2u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Chrome"), QStringLiteral("TabOrganizerSettings"), 2u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Chrome"), QStringLiteral("HistorySearchSettings"), 2u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Chrome"), QStringLiteral("CreateThemesSettings"), 2u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Chrome"), QStringLiteral("DevToolsGenAiSettings"), 2u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Chrome"), QStringLiteral("GenAILocalFoundationalModelSettings"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bo.chrome.privacy_sandbox");
        t.category = QStringLiteral("browseroffice");
        t.subCategory = QStringLiteral("Chrome");
        t.name = QStringLiteral("Disable Chrome Privacy Sandbox and ads");
        t.description = QStringLiteral("Disables Chrome's Privacy Sandbox features including ad measurement, ad topics, site-enabled ads, and promotional content.");
        t.iconName = QStringLiteral("eye-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Ad targeting features disabled"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Chrome"), QStringLiteral("PrivacySandboxAdMeasurementEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Chrome"), QStringLiteral("PrivacySandboxAdTopicsEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Chrome"), QStringLiteral("PrivacySandboxPromptEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Chrome"), QStringLiteral("PrivacySandboxSiteEnabledAdsEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Chrome"), QStringLiteral("PromotionalTabsEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Chrome"), QStringLiteral("PromotionsEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Chrome"), QStringLiteral("ShoppingListEnabled"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bo.chrome.report_extension");
        t.category = QStringLiteral("browseroffice");
        t.subCategory = QStringLiteral("Chrome");
        t.name = QStringLiteral("Disable Chrome report extension data");
        t.description = QStringLiteral("Turns off data collection by the Chrome Enterprise Reporting extension (policy, machine ID, extensions, version, Safe Browsing, telemetry, browsing, user ID). This only matters in managed environments where that extension is installed; on a normal home PC it does nothing.");
        t.iconName = QStringLiteral("eye-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Chrome enterprise reporting will not function"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Google\\Chrome\\3rdparty\\extensions\\emahakmocgideepebncgnmlmliepgpgb\\Policy"), QStringLiteral("report_policy_data"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Google\\Chrome\\3rdparty\\extensions\\emahakmocgideepebncgnmlmliepgpgb\\Policy"), QStringLiteral("report_machine_id_data"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Google\\Chrome\\3rdparty\\extensions\\emahakmocgideepebncgnmlmliepgpgb\\Policy"), QStringLiteral("report_extensions_data"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Google\\Chrome\\3rdparty\\extensions\\emahakmocgideepebncgnmlmliepgpgb\\Policy"), QStringLiteral("report_version_data"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Google\\Chrome\\3rdparty\\extensions\\emahakmocgideepebncgnmlmliepgpgb\\Policy"), QStringLiteral("report_safe_browsing_data"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Google\\Chrome\\3rdparty\\extensions\\emahakmocgideepebncgnmlmliepgpgb\\Policy"), QStringLiteral("report_system_telemetry_data"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Google\\Chrome\\3rdparty\\extensions\\emahakmocgideepebncgnmlmliepgpgb\\Policy"), QStringLiteral("report_user_browsing_data"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Google\\Chrome\\3rdparty\\extensions\\emahakmocgideepebncgnmlmliepgpgb\\Policy"), QStringLiteral("report_user_id_data"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\Software\\Policies\\Google\\Chrome\\3rdparty\\extensions\\emahakmocgideepebncgnmlmliepgpgb\\Policy"), QStringLiteral("report_policy_data"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\Software\\Policies\\Google\\Chrome\\3rdparty\\extensions\\emahakmocgideepebncgnmlmliepgpgb\\Policy"), QStringLiteral("report_machine_id_data"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\Software\\Policies\\Google\\Chrome\\3rdparty\\extensions\\emahakmocgideepebncgnmlmliepgpgb\\Policy"), QStringLiteral("report_extensions_data"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\Software\\Policies\\Google\\Chrome\\3rdparty\\extensions\\emahakmocgideepebncgnmlmliepgpgb\\Policy"), QStringLiteral("report_version_data"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\Software\\Policies\\Google\\Chrome\\3rdparty\\extensions\\emahakmocgideepebncgnmlmliepgpgb\\Policy"), QStringLiteral("report_safe_browsing_data"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\Software\\Policies\\Google\\Chrome\\3rdparty\\extensions\\emahakmocgideepebncgnmlmliepgpgb\\Policy"), QStringLiteral("report_system_telemetry_data"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\Software\\Policies\\Google\\Chrome\\3rdparty\\extensions\\emahakmocgideepebncgnmlmliepgpgb\\Policy"), QStringLiteral("report_user_browsing_data"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\Software\\Policies\\Google\\Chrome\\3rdparty\\extensions\\emahakmocgideepebncgnmlmliepgpgb\\Policy"), QStringLiteral("report_user_id_data"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bo.chrome.telemetry");
        t.category = QStringLiteral("browseroffice");
        t.subCategory = QStringLiteral("Chrome");
        t.name = QStringLiteral("Disable Chrome telemetry and data collection");
        t.description = QStringLiteral("Disables Google Chrome metrics reporting, URL-keyed data collection, Safe Browsing surveys, domain reliability, feedback surveys, optimization guide fetching, and DNS prefetching via Group Policy.");
        t.iconName = QStringLiteral("eye-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Chrome diagnostics and crash reporting disabled"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Chrome"), QStringLiteral("MetricsReportingEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Chrome"), QStringLiteral("UrlKeyedAnonymizedDataCollectionEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Chrome"), QStringLiteral("SafeBrowsingSurveysEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Chrome"), QStringLiteral("SafeBrowsingExtendedReportingEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Chrome"), QStringLiteral("SafeBrowsingProxiedRealTimeChecksAllowed"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Chrome"), QStringLiteral("DomainReliabilityAllowed"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Chrome"), QStringLiteral("FeedbackSurveysEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Chrome"), QStringLiteral("UserFeedbackAllowed"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Chrome"), QStringLiteral("OptimizationGuideFetchingEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Chrome"), QStringLiteral("DnsPrefetchingEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Chrome"), QStringLiteral("CloudProfileReportingEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Chrome"), QStringLiteral("BrowserNetworkTimeQueriesEnabled"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("browser.disable_edge_background");
        t.category = QStringLiteral("browseroffice");
        t.subCategory = QStringLiteral("Edge");
        t.name = QStringLiteral("Disable Edge background running");
        t.description = QStringLiteral("Prevents Edge from continuing to run in the background after all windows are closed.");
        t.iconName = QStringLiteral("globe");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Edge extensions and background push notifications stop delivering when all browser windows are closed"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("BackgroundModeEnabled"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bo.edge.firstrun");
        t.category = QStringLiteral("browseroffice");
        t.subCategory = QStringLiteral("Edge");
        t.name = QStringLiteral("Disable Edge first run experience");
        t.description = QStringLiteral("Hides the Edge first-run setup wizard that appears when Edge opens for the first time.");
        t.iconName = QStringLiteral("globe");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("New Windows profiles skip the Edge setup wizard and never see the default-browser or sync prompts"),
        };
        t.registryOps = {
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("HideFirstRunExperience"), QStringLiteral("1"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("GuidedSwitchEnabled"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("ImportOnEachLaunch"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("ShowRecommendationsEnabled"), QStringLiteral("0"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bo.edge.passwords");
        t.category = QStringLiteral("browseroffice");
        t.subCategory = QStringLiteral("Edge");
        t.name = QStringLiteral("Disable Edge password manager");
        t.description = QStringLiteral("Disables the built-in Edge password manager to prevent credential storage in the browser.");
        t.iconName = QStringLiteral("lock");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Saved passwords in Edge are no longer accessible for autofill; you must rely on an external password manager"),
        };
        t.registryOps = {
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("PasswordManagerEnabled"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("PasswordGeneratorEnabled"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("PasswordMonitorAllowed"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("PasswordManagerRestrictLengthEnabled"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("PasswordProtectionWarningTrigger"), QStringLiteral("0"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("browser.disable_edge_startup_boost");
        t.category = QStringLiteral("browseroffice");
        t.subCategory = QStringLiteral("Edge");
        t.name = QStringLiteral("Disable Edge startup boost");
        t.description = QStringLiteral("Prevents Microsoft Edge from pre-launching at login to reduce boot time and memory usage.");
        t.iconName = QStringLiteral("globe");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Edge takes a couple of seconds longer to open the first time after login because it no longer pre-launches"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("StartupBoostEnabled"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bo.edge.suggestions");
        t.category = QStringLiteral("browseroffice");
        t.subCategory = QStringLiteral("Edge");
        t.name = QStringLiteral("Disable Edge suggestions and recommendations");
        t.description = QStringLiteral("Disables search suggestions and content recommendations in Microsoft Edge.");
        t.iconName = QStringLiteral("globe");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Address-bar autocomplete and new-tab content recommendations disappear; URL typos no longer get Bing suggestions"),
        };
        t.registryOps = {
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("SearchSuggestEnabled"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("NewTabPageQuickLinksEnabled"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("NewTabPageBingChatEnabled"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("NewTabPageAppLauncherEnabled"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("NewTabPageCompanyLogoEnabled"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("PinBrowserEssentialsToolbarButton"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("EdgeWorkspacesEnabled"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("GamerModeEnabled"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("ShowRecommendationsEnabled"), QStringLiteral("0"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bo.edge.telemetry");
        t.category = QStringLiteral("browseroffice");
        t.subCategory = QStringLiteral("Edge");
        t.name = QStringLiteral("Disable Edge telemetry");
        t.description = QStringLiteral("Disables Edge metrics reporting and personalisation data collection.");
        t.iconName = QStringLiteral("globe");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Edge crash reports stop reaching Microsoft, so regressions you hit will not be fed back to the Edge team"),
        };
        t.registryOps = {
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("DiagnosticData"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("MetricsReportingEnabled"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("PersonalizationReportingEnabled"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("SendSiteInfoToImproveServices"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("SpotlightExperiencesAndRecommendationsEnabled"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("EdgeManagementEnabled"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("EdgeManagementExtensionsFeedbackEnabled"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\EdgeUpdate"), QStringLiteral("UpdaterExperimentationAndConfigurationServiceControl"), QStringLiteral("0"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bo.office.connected");
        t.category = QStringLiteral("browseroffice");
        t.subCategory = QStringLiteral("Office");
        t.name = QStringLiteral("Disable Office online content");
        t.description = QStringLiteral("Prevents Office applications from fetching online content (templates, help, tips).");
        t.iconName = QStringLiteral("file-text");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Online templates, cloud clip-art, and Office help content no longer load inside Word, Excel, and PowerPoint"),
        };
        t.registryOps = {
            stringOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Common\\General"), QStringLiteral("PreferCloudSaveLocations"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Common\\General"), QStringLiteral("skydrivesigninoption"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Common\\General"), QStringLiteral("notrack"), QStringLiteral("1"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Common\\General"), QStringLiteral("optindisable"), QStringLiteral("1"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Common\\General"), QStringLiteral("signinoptions"), QStringLiteral("3"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Common\\General"), QStringLiteral("disablecomingsoon"), QStringLiteral("1"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Office\\Common\\Graphics"), QStringLiteral("DisableAnimations"), QStringLiteral("1"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Common\\Internet"), QStringLiteral("UseOnlineContent"), QStringLiteral("0"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bo.office.datacollection");
        t.category = QStringLiteral("browseroffice");
        t.subCategory = QStringLiteral("Office");
        t.name = QStringLiteral("Disable Office data collection");
        t.description = QStringLiteral("Disables Office telemetry collection and sets data transmission to the minimum level.");
        t.iconName = QStringLiteral("file-text");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Office admin telemetry dashboards no longer receive data; issues on this PC will not show up in enterprise reports"),
        };
        t.registryOps = {
            stringOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Common"), QStringLiteral("SendCustomerDataOptIn"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Common"), QStringLiteral("SendCustomerDataOptInReason"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Common"), QStringLiteral("qmenable"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Common"), QStringLiteral("updatereliabilitydata"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Office\\16.0\\Common"), QStringLiteral("SendCustomerDataOptIn"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Office\\16.0\\Common"), QStringLiteral("qmenable"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Office\\16.0\\Common"), QStringLiteral("updatereliabilitydata"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Common\\ptwatson"), QStringLiteral("ptwoptin"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\Common\\ClientTelemetry"), QStringLiteral("DisableTelemetry"), QStringLiteral("1"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\Common\\ClientTelemetry"), QStringLiteral("SendTelemetry"), QStringLiteral("3"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bo.office.feedback");
        t.category = QStringLiteral("browseroffice");
        t.subCategory = QStringLiteral("Office");
        t.name = QStringLiteral("Disable Office feedback prompts");
        t.description = QStringLiteral("Disables the Office feedback prompts that ask you to rate features and submit surveys.");
        t.iconName = QStringLiteral("file-text");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("No feedback or suggestion prompts appear; you lose an easy way to report bugs to the Office team"),
        };
        t.registryOps = {
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Office\\16.0\\Common\\Feedback"), QStringLiteral("enabled"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Office\\16.0\\Common\\Feedback"), QStringLiteral("includescreenshot"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Office\\16.0\\Common\\Feedback"), QStringLiteral("includeemail"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Office\\16.0\\Common\\Feedback"), QStringLiteral("feedbackincludelog"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Office\\16.0\\Common\\Feedback"), QStringLiteral("surveyenabled"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Office\\16.0\\Common\\Feedback"), QStringLiteral("updatereliabilitydata"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Common\\Feedback"), QStringLiteral("enabled"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Common\\Feedback"), QStringLiteral("surveyenabled"), QStringLiteral("0"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bo.office.telemetry");
        t.category = QStringLiteral("browseroffice");
        t.subCategory = QStringLiteral("Office");
        t.name = QStringLiteral("Disable Office telemetry");
        t.description = QStringLiteral("Disables diagnostic data collection in Microsoft Office applications.");
        t.iconName = QStringLiteral("file-text");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Office diagnostic data no longer reaches Microsoft; Office crash reports from this machine are not investigated"),
        };
        t.registryOps = {
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Office\\Common\\ClientTelemetry"), QStringLiteral("DisableTelemetry"), QStringLiteral("1"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Office\\Common\\ClientTelemetry"), QStringLiteral("VerboseLogging"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Office\\16.0\\Common\\ClientTelemetry"), QStringLiteral("DisableTelemetry"), QStringLiteral("1"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Common\\clienttelemetry"), QStringLiteral("sendtelemetry"), QStringLiteral("3"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\osm"), QStringLiteral("enablelogging"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\osm"), QStringLiteral("enablefileobfuscation"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\osm"), QStringLiteral("enableupload"), QStringLiteral("0"), std::nullopt),
            stringOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\Common\\ClientTelemetry"), QStringLiteral("DisableTelemetry"), QStringLiteral("1"), std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bo.edge.extended");
        t.category = QStringLiteral("browseroffice");
        t.subCategory = QStringLiteral("Edge");
        t.name = QStringLiteral("Disable Edge bloatware features");
        t.description = QStringLiteral("Disables Edge shopping assistant, collections, Copilot, sidebar, Discover, Follow, wallet, math solver, travel assistant, read aloud, visual search, and other non-essential features.");
        t.iconName = QStringLiteral("globe");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Many Edge sidebar and assistant features disabled"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("EdgeShoppingAssistantEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("EdgeCollectionsEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("EdgeDiscoverEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("EdgeFollowEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("EdgeEDropEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("EdgeEnhanceImagesEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("EdgeWalletCheckoutEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("EdgeWalletEtreeEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("WalletDonationEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("CryptoWalletEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("HubsSidebarEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("StandaloneHubsSidebarEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("EdgeOpenInSidebarEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("MathSolverEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("TravelAssistanceEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("ReadAloudEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("ComposeInlineEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("CopilotCDPPageContext"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("CopilotPageContext"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("DiscoverPageContextEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("ShowMicrosoftRewards"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("ShowOfficeShortcutInFavoritesBar"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("MicrosoftEditorProofingEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("MicrosoftEditorSynonymsEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("MicrosoftOfficeMenuEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("OutlookHubMenuEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("QuickSearchShowMiniMenu"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bo.edge.performance");
        t.category = QStringLiteral("browseroffice");
        t.subCategory = QStringLiteral("Edge");
        t.name = QStringLiteral("Edge performance and efficiency settings");
        t.description = QStringLiteral("Enables sleeping tabs and disables startup boost, efficiency mode, the performance detector and background template updates. It also disables the site-safety services indicator, SERP telemetry, spellcheck and speech recognition and turns on Do-Not-Track. Primarily a performance/privacy toggle; core SmartScreen is not affected.");
        t.iconName = QStringLiteral("globe");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Startup boost disabled may slow Edge launch"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("SleepingTabsEnabled"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("StartupBoostEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("EfficiencyModeEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("PerformanceDetectorEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("BackgroundTemplateListUpdatesEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("Edge3PSerpTelemetryEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("EdgeAssetDeliveryServiceEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("ConfigureDoNotTrack"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("SiteSafetyServicesEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("UrlDiagnosticDataEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("UserFeedbackAllowed"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("SpellcheckEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("SpeechRecognitionEnabled"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bo.edge.update_extended");
        t.category = QStringLiteral("browseroffice");
        t.subCategory = QStringLiteral("Edge");
        t.name = QStringLiteral("Edge update extended policy");
        t.description = QStringLiteral("Switches Edge to the Extended Stable channel (still fully security-patched, but with a slower feature-update cadence) and keeps the WebView2 Runtime updating. Unlike the auto-update-disable tweak, this does not stop security updates.");
        t.iconName = QStringLiteral("globe");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Edge update channel set to extended stable"),
        };
        t.registryOps = {
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\EdgeUpdate"), QStringLiteral("TargetChannel{56EB18F8-B008-4CBD-B6D2-8C97FE7E9062}"), QStringLiteral("extended"), std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\EdgeUpdate"), QStringLiteral("Install{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\EdgeUpdate"), QStringLiteral("Update{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bo.firefox.policies");
        t.category = QStringLiteral("browseroffice");
        t.subCategory = QStringLiteral("Firefox");
        t.name = QStringLiteral("Firefox privacy and telemetry policies");
        t.description = QStringLiteral("Configures Firefox via Group Policy: disables telemetry, studies, Pocket, feedback, network prediction, and sponsored suggestions while enabling tracking protection.");
        t.iconName = QStringLiteral("eye-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Firefox studies and experiments disabled"),
            QStringLiteral("Pocket integration removed"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Mozilla\\Firefox"), QStringLiteral("DisableTelemetry"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Mozilla\\Firefox"), QStringLiteral("DisableFirefoxStudies"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Mozilla\\Firefox"), QStringLiteral("DisablePocket"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Mozilla\\Firefox"), QStringLiteral("DisableFeedbackCommands"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Mozilla\\Firefox"), QStringLiteral("DisableFirefoxAccounts"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Mozilla\\Firefox"), QStringLiteral("DisableFirefoxScreenshots"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Mozilla\\Firefox"), QStringLiteral("DisableProfileImport"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Mozilla\\Firefox"), QStringLiteral("DontCheckDefaultBrowser"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Mozilla\\Firefox"), QStringLiteral("NetworkPrediction"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Mozilla\\Firefox"), QStringLiteral("NoDefaultBookmarks"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Mozilla\\Firefox\\EnableTrackingProtection"), QStringLiteral("Value"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Mozilla\\Firefox\\EnableTrackingProtection"), QStringLiteral("Cryptomining"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Mozilla\\Firefox\\EnableTrackingProtection"), QStringLiteral("Fingerprinting"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Mozilla\\Firefox\\EnableTrackingProtection"), QStringLiteral("EmailTracking"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Mozilla\\Firefox\\FirefoxSuggest"), QStringLiteral("SponsoredSuggestions"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Mozilla\\Firefox\\FirefoxSuggest"), QStringLiteral("WebSuggestions"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Mozilla\\Firefox\\FirefoxSuggest"), QStringLiteral("ImproveSuggest"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Mozilla\\Firefox\\UserMessaging"), QStringLiteral("ExtensionRecommendations"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Mozilla\\Firefox\\UserMessaging"), QStringLiteral("FeatureRecommendations"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Mozilla\\Firefox\\UserMessaging"), QStringLiteral("MoreFromMozilla"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Mozilla\\Firefox\\UserMessaging"), QStringLiteral("SkipOnboarding"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Mozilla\\Firefox\\UserMessaging"), QStringLiteral("WhatsNew"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Mozilla\\Firefox\\FirefoxHome"), QStringLiteral("SponsoredTopSites"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Mozilla\\Firefox\\FirefoxHome"), QStringLiteral("SponsoredPocket"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Mozilla\\Firefox\\FirefoxHome"), QStringLiteral("Pocket"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Mozilla\\Firefox\\FirefoxHome"), QStringLiteral("Snippets"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bo.ie.security_zones");
        t.category = QStringLiteral("browseroffice");
        t.subCategory = QStringLiteral("Internet Explorer");
        t.name = QStringLiteral("Harden IE security zone settings");
        t.description = QStringLiteral("Hardens Internet Explorer security zones: blocks file downloads and restricts ActiveX in the Internet/Restricted zones and enforces certificate-revocation checks. This tightens security (it does not weaken it), though it can break legacy IE web apps and IE itself is deprecated on modern Windows.");
        t.iconName = QStringLiteral("folder-open");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Some legacy web applications may break"),
            QStringLiteral("ActiveX controls in restricted zone blocked"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Zones\\3"), QStringLiteral("1803"), 3u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"), QStringLiteral("CertificateRevocation"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Zones\\3"), QStringLiteral("2001"), 3u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Zones\\4"), QStringLiteral("1201"), 3u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Zones\\4"), QStringLiteral("1803"), 3u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bo.office.clicktorun");
        t.category = QStringLiteral("browseroffice");
        t.subCategory = QStringLiteral("Office");
        t.name = QStringLiteral("Disable Office ClickToRun logging");
        t.description = QStringLiteral("Disables Office ClickToRun log management and extends the timer interval to reduce background activity.");
        t.iconName = QStringLiteral("file-text");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Office ClickToRun diagnostics disabled"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\ClickToRun\\OverRide"), QStringLiteral("DisableLogManagement"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bo.office.firstrun");
        t.category = QStringLiteral("browseroffice");
        t.subCategory = QStringLiteral("Office");
        t.name = QStringLiteral("Disable Office first-run and alert reporting");
        t.description = QStringLiteral("Disables Office first-run movie, alert reporting, ship asserts, and DLP policy tips for Office 2016/365.");
        t.iconName = QStringLiteral("file-text");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Office first-run experience suppressed"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Firstrun"), QStringLiteral("disablemovie"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Common\\alerts"), QStringLiteral("noalertreporting"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Common\\alerts"), QStringLiteral("disableshipasserts"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\dlp"), QStringLiteral("policytipsenabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Common\\shipasserts"), QStringLiteral("disableshipasserts"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bo.office.osm_prevent");
        t.category = QStringLiteral("browseroffice");
        t.subCategory = QStringLiteral("Office");
        t.name = QStringLiteral("Disable Office OSM solution tracking");
        t.description = QStringLiteral("Prevents Office Solution Monitor from tracking add-ins, document files, templates, COM add-ins, and app add-ins across all Office applications.");
        t.iconName = QStringLiteral("file-text");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Office solution monitoring and compatibility reports disabled"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\osm\\preventedapplications"), QStringLiteral("wdsolution"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\osm\\preventedapplications"), QStringLiteral("xlsolution"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\osm\\preventedapplications"), QStringLiteral("pptsolution"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\osm\\preventedapplications"), QStringLiteral("olksolution"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\osm\\preventedapplications"), QStringLiteral("accesssolution"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\osm\\preventedapplications"), QStringLiteral("projectsolution"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\osm\\preventedapplications"), QStringLiteral("publishersolution"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\osm\\preventedapplications"), QStringLiteral("visiosolution"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\osm\\preventedapplications"), QStringLiteral("onenotesolution"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\osm\\preventedsolutiontypes"), QStringLiteral("documentfiles"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\osm\\preventedsolutiontypes"), QStringLiteral("templatefiles"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\osm\\preventedsolutiontypes"), QStringLiteral("comaddins"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\osm\\preventedsolutiontypes"), QStringLiteral("appaddins"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\osm\\preventedsolutiontypes"), QStringLiteral("agave"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bo.office.other_apps");
        t.category = QStringLiteral("browseroffice");
        t.subCategory = QStringLiteral("Office");
        t.name = QStringLiteral("Office other apps tweaks");
        t.description = QStringLiteral("Configures Lync/Skype, OneNote, Visio, Publisher, and Access: disables send tracking, What's New dialogs, startup dialogs, logging, and OneNote diagnostics.");
        t.iconName = QStringLiteral("file-text");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("What's New dialogs suppressed in multiple Office apps"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Lync"), QStringLiteral("disableautomaticsendtracking"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\onenote"), QStringLiteral("dontshowwhatsnew"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\onenote\\general"), QStringLiteral("disablesupportdiagnostics"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\visio"), QStringLiteral("dontshowwhatsnew"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\publisher\\preferences"), QStringLiteral("usestartupdlg"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Office\\16.0\\Access\\Options"), QStringLiteral("EnableLogging"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Access"), QStringLiteral("dontshowwhatsnew"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\excel"), QStringLiteral("dontshowwhatsnew"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Powerpoint"), QStringLiteral("dontshowwhatsnew"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Word"), QStringLiteral("dontshowwhatsnew"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bo.office.outlook");
        t.category = QStringLiteral("browseroffice");
        t.subCategory = QStringLiteral("Office");
        t.name = QStringLiteral("Office Outlook tweaks");
        t.description = QStringLiteral("Configures Outlook: disables logging, calendar logging, spell checking on send, sets encryption and signing defaults, and disables Outlook mobile hyperlink promotion.");
        t.iconName = QStringLiteral("file-text");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Outlook logging and calendar logging disabled"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Outlook"), QStringLiteral("EnableLogging"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Outlook\\Security"), QStringLiteral("InitEncrypt"), 2u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Outlook\\Security"), QStringLiteral("InitSign"), 2u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Office\\16.0\\Outlook\\Options\\Calendar"), QStringLiteral("EnableCalendarLogging"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Office\\16.0\\Outlook\\Options\\Mail"), QStringLiteral("EnableLogging"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Outlook\\options\\spelling"), QStringLiteral("check"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Outlook\\options\\General"), QStringLiteral("DisableOutlookMobileHyperlink"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bo.office.privacy");
        t.category = QStringLiteral("browseroffice");
        t.subCategory = QStringLiteral("Office");
        t.name = QStringLiteral("Disable Office privacy and connected services");
        t.description = QStringLiteral("Configures Office privacy settings: disconnects online services, disables content downloads, diagnostics, AI training, and controller connected services.");
        t.iconName = QStringLiteral("file-text");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Office connected services will be limited"),
            QStringLiteral("AI-powered features disabled"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Common\\privacy"), QStringLiteral("disconnectedstate"), 2u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Common\\privacy"), QStringLiteral("usercontentdisabled"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Common\\privacy"), QStringLiteral("downloadcontentdisabled"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Common\\privacy"), QStringLiteral("controllerconnectedservicesenabled"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Common\\diagnostics"), QStringLiteral("configurediagnostics"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Common\\ai\\training\\general"), QStringLiteral("disabletraining"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bo.office.teams_autostart");
        t.category = QStringLiteral("browseroffice");
        t.subCategory = QStringLiteral("Office");
        t.name = QStringLiteral("Disable Teams auto-startup");
        t.description = QStringLiteral("Prevents Microsoft Teams from auto-launching after Office installation via deployment policy.");
        t.iconName = QStringLiteral("file-text");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Teams must be started manually"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\teams"), QStringLiteral("preventfirstlaunchafterinstall"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bo.office.telemetry_15");
        t.category = QStringLiteral("browseroffice");
        t.subCategory = QStringLiteral("Office");
        t.name = QStringLiteral("Disable Office 2013 telemetry");
        t.description = QStringLiteral("Disables telemetry, data collection, OSM logging, Watson crash reporting, and feedback for Office 2013 (version 15.0).");
        t.iconName = QStringLiteral("file-text");
        t.impact = Impact::Low;
        t.warnings = {
            QStringLiteral("Office 2013 diagnostics and crash reporting disabled"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\15.0\\Common\\clienttelemetry"), QStringLiteral("sendtelemetry"), 3u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\15.0\\osm"), QStringLiteral("enablelogging"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\15.0\\osm"), QStringLiteral("enablefileobfuscation"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\15.0\\osm"), QStringLiteral("enableupload"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\15.0\\Common"), QStringLiteral("SendCustomerDataOptIn"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\15.0\\Common"), QStringLiteral("qmenable"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\15.0\\Common"), QStringLiteral("updatereliabilitydata"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\15.0\\Common\\ptwatson"), QStringLiteral("ptwoptin"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Office\\15.0\\Common\\Feedback"), QStringLiteral("enabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Microsoft\\Office\\15.0\\Common\\Feedback"), QStringLiteral("surveyenabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\15.0\\Common\\Feedback"), QStringLiteral("enabled"), 0u, 1u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\15.0\\Common\\Feedback"), QStringLiteral("surveyenabled"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bo.edge.updater_off");
        t.category = QStringLiteral("browseroffice");
        t.subCategory = QStringLiteral("Edge");
        t.name = QStringLiteral("Stop Edge updating itself");
        t.description = QStringLiteral("Turns off Microsoft Edge's background updater. Edge and WebView2 stop checking for and installing updates on their own schedule, which is the schedule that decides mid-match that now is the moment to download a browser you were not using.");
        t.iconName = QStringLiteral("brand-edge");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Edge stops receiving security updates until you update it by hand or turn this back off"),
            QStringLiteral("WebView2 also stops updating, which affects other apps that embed it"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\EdgeUpdate"), QStringLiteral("UpdateDefault"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\EdgeUpdate"), QStringLiteral("InstallDefault"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\EdgeUpdate"), QStringLiteral("AutoUpdateCheckPeriodMinutes"), 0u, std::nullopt),
            // Per-application override for Edge Stable. UpdateDefault is the
            // fallback; an app with its own Update{guid} value ignores it, so the
            // blanket policy alone leaves Stable updating on some installs.
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\EdgeUpdate"), QStringLiteral("Update{56EB18F8-B008-4CBD-B6D2-8C97FE7E9062}"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    // OneDrive sync policies. OneDrive stays installed, its files stay on disk,
    // and reverting deletes the values.
    // learn.microsoft.com/sharepoint/use-group-policy
    //
    // Two roots that aren't interchangeable: DisableFileSyncNGSC and
    // DisableLibrariesDefaultSaveToOneDrive live under ...\Windows\OneDrive,
    // everything else under ...\Microsoft\OneDrive.
    {
        TweakSpec t;
        t.id = QStringLiteral("bo.onedrive.sync_off");
        t.category = QStringLiteral("browseroffice");
        t.subCategory = QStringLiteral("OneDrive");
        t.name = QStringLiteral("Turn off OneDrive file sync");
        t.description = QStringLiteral("Stops OneDrive syncing files for every account on this PC. OneDrive stays installed and anything already downloaded stays on disk - only the syncing stops. Turn this back off to resume it.");
        t.iconName = QStringLiteral("cloud-off");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Files stop syncing to and from the cloud for every account on this PC"),
            QStringLiteral("Online-only files that have not been downloaded stay unavailable"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\OneDrive"), QStringLiteral("DisableFileSyncNGSC"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bo.onedrive.prompts");
        t.category = QStringLiteral("browseroffice");
        t.subCategory = QStringLiteral("OneDrive");
        t.name = QStringLiteral("Disable OneDrive backup prompts and default save");
        t.description = QStringLiteral("Blocks the recurring prompt to move Desktop, Documents and Pictures into OneDrive, stops OneDrive being the default save location, and stops it contacting the network before anyone has signed in. Syncing itself is unaffected.");
        t.iconName = QStringLiteral("cloud-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Folder backup can no longer be turned on from the OneDrive app"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\OneDrive"), QStringLiteral("KFMBlockOptIn"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\OneDrive"), QStringLiteral("PreventNetworkTrafficPreUserSignIn"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\OneDrive"), QStringLiteral("DisableLibrariesDefaultSaveToOneDrive"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bo.onedrive.personal");
        t.category = QStringLiteral("browseroffice");
        t.subCategory = QStringLiteral("OneDrive");
        t.name = QStringLiteral("Block OneDrive personal account sync");
        t.description = QStringLiteral("Stops personal Microsoft accounts syncing through OneDrive while leaving work and school accounts alone. For a PC where OneDrive is wanted for one and not the other.");
        t.iconName = QStringLiteral("cloud-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("A personal OneDrive already set up on this PC stops syncing"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\OneDrive"), QStringLiteral("DisablePersonalSync"), 1u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bo.brave.policies");
        t.category = QStringLiteral("browseroffice");
        t.name = QStringLiteral("Brave browser privacy and feature policies");
        t.description = QStringLiteral("Configures Brave Browser via Group Policy: disables telemetry, SafeBrowsing, Privacy Sandbox, AI chat, VPN, Brave Wallet, DNS prefetching, and various data collection features.");
        t.iconName = QStringLiteral("eye-off");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("SafeBrowsing and ad-blocking analytics disabled"),
            QStringLiteral("Brave AI Chat will not be available"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\BraveSoftware\\Brave"), QStringLiteral("MetricsReportingEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\BraveSoftware\\Brave"), QStringLiteral("SafeBrowsingEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\BraveSoftware\\Brave"), QStringLiteral("SafeBrowsingProtectionLevel"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\BraveSoftware\\Brave"), QStringLiteral("SafeBrowsingExtendedReportingEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\BraveSoftware\\Brave"), QStringLiteral("PrivacySandboxAdMeasurementEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\BraveSoftware\\Brave"), QStringLiteral("PrivacySandboxAdTopicsEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\BraveSoftware\\Brave"), QStringLiteral("PrivacySandboxPromptEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\BraveSoftware\\Brave"), QStringLiteral("PrivacySandboxSiteEnabledAdsEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\BraveSoftware\\Brave"), QStringLiteral("DnsPrefetchingEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\BraveSoftware\\Brave"), QStringLiteral("DomainReliabilityAllowed"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\BraveSoftware\\Brave"), QStringLiteral("FeedbackSurveysEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\BraveSoftware\\Brave"), QStringLiteral("UserFeedbackAllowed"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\BraveSoftware\\Brave"), QStringLiteral("BackgroundModeEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\BraveSoftware\\Brave"), QStringLiteral("BraveAIChatEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\BraveSoftware\\Brave"), QStringLiteral("BraveVPNDisabled"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\BraveSoftware\\Brave"), QStringLiteral("BraveWalletDisabled"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\BraveSoftware\\Brave"), QStringLiteral("UrlKeyedAnonymizedDataCollectionEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\BraveSoftware\\Brave"), QStringLiteral("PasswordLeakDetectionEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\BraveSoftware\\Brave"), QStringLiteral("OptimizationGuideFetchingEnabled"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bo.chrome.google_update");
        t.category = QStringLiteral("browseroffice");
        t.name = QStringLiteral("Control Google update service");
        t.description = QStringLiteral("Configures Google Update policies to extend the update check interval, set update mode to manual, use direct proxy, and disable Google Toolbar and Google Earth installs.");
        t.iconName = QStringLiteral("refresh-cw");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Chrome stops auto-updating; installed versions lag behind security patches until manually updated"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Update"), QStringLiteral("AutoUpdateCheckPeriodMinutes"), 20800u, 1440u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Update"), QStringLiteral("UpdateDefault"), 3u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Update"), QStringLiteral("InstallDefault"), 4u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Update"), QStringLiteral("CloudPolicyOverridesPlatformPolicy"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Update"), QStringLiteral("Install{4CCED17F-7852-4AFC-9E9E-C89D8795BDD2}"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Update"), QStringLiteral("Install{ADDE8406-A0F3-4AC2-8878-ADC0BD37BD86}"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bo.chrome.security");
        t.category = QStringLiteral("browseroffice");
        t.name = QStringLiteral("Chrome security and feature policies");
        t.description = QStringLiteral("Configures Chrome security: disables SafeBrowsing, background mode, click-to-call, shared clipboard, side search, spell check service, and Chrome variations experiments while enabling hardware acceleration.");
        t.iconName = QStringLiteral("shield");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("SafeBrowsing protection disabled"),
            QStringLiteral("Chrome feature experiments disabled"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Chrome"), QStringLiteral("SafeBrowsingEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Chrome"), QStringLiteral("SafeBrowsingProtectionLevel"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Chrome"), QStringLiteral("BackgroundModeEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Chrome"), QStringLiteral("ClickToCallEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Chrome"), QStringLiteral("SharedClipboardEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Chrome"), QStringLiteral("SideSearchEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Chrome"), QStringLiteral("SpellCheckServiceEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Chrome"), QStringLiteral("SpellcheckEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Chrome"), QStringLiteral("ChromeVariations"), 2u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Chrome"), QStringLiteral("ComponentUpdatesEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Chrome"), QStringLiteral("HardwareAccelerationModeEnabled"), 1u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Chrome"), QStringLiteral("HighEfficiencyModeEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Chrome"), QStringLiteral("DefaultBrowserSettingEnabled"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bo.edge.autoupdate");
        t.category = QStringLiteral("browseroffice");
        t.name = QStringLiteral("Disable Edge auto-updates");
        t.description = QStringLiteral("Prevents Edge from automatically updating by setting update policies. Sets AutoUpdateCheckPeriodMinutes to a very long interval and UpdateDefault to manual-only.");
        t.iconName = QStringLiteral("globe");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Edge stops auto-updating; installed versions fall behind security patches until manually updated"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\EdgeUpdate"), QStringLiteral("AutoUpdateCheckPeriodMinutes"), 20800u, 1440u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\EdgeUpdate"), QStringLiteral("UpdateDefault"), 3u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\EdgeUpdate"), QStringLiteral("Allowsxs"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\EdgeUpdate"), QStringLiteral("CreateDesktopShortcutDefault"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bo.edge.smartscreen");
        t.category = QStringLiteral("browseroffice");
        t.name = QStringLiteral("Disable Edge SmartScreen");
        t.description = QStringLiteral("Disables Edge SmartScreen filter, DNS requests for SmartScreen, PUA blocking, and download protection prompts.");
        t.iconName = QStringLiteral("globe");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("SmartScreen phishing and malware-download protection stops warning you about suspicious sites and files"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("SmartScreenEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("SmartScreenDnsRequestsEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("SmartScreenPuaEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("SmartScreenForTrustedDownloadsEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("NewSmartScreenLibraryEnabled"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("PreventSmartScreenPromptOverride"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge"), QStringLiteral("PreventSmartScreenPromptOverrideForFiles"), 0u, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bo.ie.epm");
        t.category = QStringLiteral("browseroffice");
        t.name = QStringLiteral("Disable IE enhanced protected mode");
        t.description = QStringLiteral("Disables Internet Explorer's Enhanced Protected Mode for improved compatibility with older ActiveX controls and enterprise web applications.");
        t.iconName = QStringLiteral("folder-open");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("IE sandbox isolation is reduced; exploits in rendered pages have an easier path out of the tab process"),
        };
        t.registryOps = {
            stringOp(QStringLiteral("HKCU\\Software\\Microsoft\\Internet Explorer\\Main"), QStringLiteral("Isolation"), QStringLiteral("PMIL"), QStringLiteral("PMEM")),
            stringOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Internet Explorer\\Main"), QStringLiteral("Isolation"), QStringLiteral("PMIL"), QStringLiteral("PMEM")),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bo.office.excel");
        t.category = QStringLiteral("browseroffice");
        t.name = QStringLiteral("Office Excel tweaks");
        t.description = QStringLiteral("Configures Excel: disables boot-to-start, grammar checks, smart tags, LinkedIn assistant, live preview, logging, and Protected View for all sources; disables trusted documents for network locations.");
        t.iconName = QStringLiteral("file-text");
        t.impact = Impact::Medium;
        t.warnings = {
            QStringLiteral("Protected View disabled for Excel documents"),
            QStringLiteral("Network trusted documents disabled"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Excel\\Options"), QStringLiteral("disableboottoofficestart"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Excel\\Options"), QStringLiteral("rungrammar"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Excel\\Options"), QStringLiteral("nosmarttagrecognition"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Excel\\Options"), QStringLiteral("linkedinresumeassistant"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Excel\\Options"), QStringLiteral("EnableLogging"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Excel\\Security\\protectedview"), QStringLiteral("disableinternetfilesinpv"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Excel\\Security\\protectedview"), QStringLiteral("disableunsafelocationsinpv"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Excel\\Security\\protectedview"), QStringLiteral("disableattachmentsinpv"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Excel\\Security\\Trusted Documents"), QStringLiteral("disablenetworktrusteddocuments"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Excel\\Security\\Trusted Documents"), QStringLiteral("disabletrusteddocuments"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bo.office.powerpoint");
        t.category = QStringLiteral("browseroffice");
        t.name = QStringLiteral("Office PowerPoint tweaks");
        t.description = QStringLiteral("Configures PowerPoint: disables boot-to-start, grammar checks, smart tags, LinkedIn assistant, live preview, logging, background spell checking, and Protected View for all sources.");
        t.iconName = QStringLiteral("file-text");
        t.impact = Impact::Medium;
        t.warnings = {
            QStringLiteral("Protected View disabled for PowerPoint documents"),
            QStringLiteral("Background spell checking disabled"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Powerpoint\\Options"), QStringLiteral("disableboottoofficestart"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Powerpoint\\Options"), QStringLiteral("rungrammar"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Powerpoint\\Options"), QStringLiteral("nosmarttagrecognition"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Powerpoint\\Options"), QStringLiteral("linkedinresumeassistant"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Powerpoint\\Options"), QStringLiteral("EnableLogging"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Powerpoint\\Security\\protectedview"), QStringLiteral("disableinternetfilesinpv"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Powerpoint\\Security\\protectedview"), QStringLiteral("disableunsafelocationsinpv"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Powerpoint\\Security\\protectedview"), QStringLiteral("disableattachmentsinpv"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Powerpoint\\Security\\Trusted Documents"), QStringLiteral("disablenetworktrusteddocuments"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Powerpoint\\Security\\Trusted Documents"), QStringLiteral("disabletrusteddocuments"), 1u, 0u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bo.office.updates");
        t.category = QStringLiteral("browseroffice");
        t.name = QStringLiteral("Control Office update settings");
        t.description = QStringLiteral("Disables Office automatic updates, hides update notifications, and prevents Bing and Teams installs via Office updates for both Office 2013 and 2016/365.");
        t.iconName = QStringLiteral("file-text");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.warnings = {
            QStringLiteral("Office stops receiving feature and security updates; exploits in Office components remain unpatched"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Office\\16.0\\Common\\officeupdate"), QStringLiteral("preventbinginstall"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Office\\16.0\\Common\\officeupdate"), QStringLiteral("preventteamsinstall"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Office\\16.0\\Common\\officeupdate"), QStringLiteral("enableautomaticupdates"), 0u, 1u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Office\\16.0\\Common\\officeupdate"), QStringLiteral("hideenabledisableupdates"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Office\\16.0\\Common\\officeupdate"), QStringLiteral("hideupdatenotifications"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Office\\15.0\\Common\\officeupdate"), QStringLiteral("preventbinginstall"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Office\\15.0\\Common\\officeupdate"), QStringLiteral("preventteamsinstall"), 1u, 0u),
            dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Office\\15.0\\Common\\officeupdate"), QStringLiteral("enableautomaticupdates"), 0u, 1u),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bo.office.word");
        t.category = QStringLiteral("browseroffice");
        t.name = QStringLiteral("Office Word tweaks");
        t.description = QStringLiteral("Configures Word: disables boot-to-start, grammar checks, smart tags, LinkedIn assistant, live preview, online video, auto link updates, logging, and Protected View for all sources.");
        t.iconName = QStringLiteral("file-text");
        t.impact = Impact::Medium;
        t.warnings = {
            QStringLiteral("Protected View disabled for Word documents"),
            QStringLiteral("Grammar and spell checking reduced"),
        };
        t.registryOps = {
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Word\\Options"), QStringLiteral("disableboottoofficestart"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Word\\Options"), QStringLiteral("rungrammar"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Word\\Options"), QStringLiteral("nosmarttagrecognition"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Word\\Options"), QStringLiteral("linkedinresumeassistant"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Word\\Options"), QStringLiteral("DontUpdateLinks"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Word\\Options"), QStringLiteral("EnableLogging"), 0u, std::nullopt),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Word\\Security\\protectedview"), QStringLiteral("disableinternetfilesinpv"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Word\\Security\\protectedview"), QStringLiteral("disableunsafelocationsinpv"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Word\\Security\\protectedview"), QStringLiteral("disableattachmentsinpv"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Word\\Security\\Trusted Documents"), QStringLiteral("disablenetworktrusteddocuments"), 1u, 0u),
            dwordOp(QStringLiteral("HKCU\\Software\\Policies\\Microsoft\\Office\\16.0\\Word\\Security\\Trusted Documents"), QStringLiteral("disabletrusteddocuments"), 1u, 0u),
        };
        c.push_back(t);
    }
}

} // namespace vth::tweaks::detail
