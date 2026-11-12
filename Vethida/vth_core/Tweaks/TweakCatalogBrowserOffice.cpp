#include "TweakEngine.h"

// Browser and Office enterprise policies. None exist by default, so every
// revert is a delete. Disabling Edge outright is debloat.disable-edge.
// WebView2 is left alone, many apps depend on it.

namespace vth::tweaks::detail {

namespace {
void addD(QVector<RegistryOp>& ops, const QString& path, const QString& name, quint32 v) {
    ops.push_back(dwordOp(path, name, v, std::nullopt));
}
}

void appendBrowserOffice(QVector<TweakSpec>& c) {
    {
        TweakSpec t;
        t.id = QStringLiteral("browser.edge-harden");
        t.category = QStringLiteral("browseroffice");
        t.subCategory = QStringLiteral("Edge");
        t.name = QStringLiteral("Harden Microsoft Edge");
        t.description = QStringLiteral("Applies Microsoft's Edge enterprise-policy set: skips first-run and default-browser nags, disables background/startup boost and component updates, sets Tracking Prevention to Strict, enables sleeping tabs, and turns off telemetry, Rewards, Collections, the Hubs sidebar, sync, sign-in and the Copilot/GenAI surface. Also disables Edge's built-in password manager and address/credit-card autofill, so use a separate password manager if you rely on those. Policy-based; reverting deletes the keys and restores Edge defaults.");
        t.iconName = QStringLiteral("shield-check");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        const QString e = QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Edge");
        QVector<RegistryOp> o;
        addD(o, e, QStringLiteral("HideFirstRunExperience"), 1);
        addD(o, e, QStringLiteral("DefaultBrowserSettingEnabled"), 0);
        addD(o, e, QStringLiteral("DefaultBrowserSettingsCampaignEnabled"), 0);
        addD(o, e, QStringLiteral("StartupBoostEnabled"), 0);
        addD(o, e, QStringLiteral("BackgroundModeEnabled"), 0);
        addD(o, e, QStringLiteral("ComponentUpdatesEnabled"), 0);
        addD(o, e, QStringLiteral("EdgeShoppingAssistantEnabled"), 0);
        addD(o, e, QStringLiteral("SleepingTabsEnabled"), 1);
        addD(o, e, QStringLiteral("EfficiencyModeEnabled"), 0);
        addD(o, e, QStringLiteral("PerformanceDetectorEnabled"), 0);
        addD(o, e, QStringLiteral("PersonalizationReportingEnabled"), 0);
        addD(o, e, QStringLiteral("DiagnosticData"), 0);
        addD(o, e, QStringLiteral("MetricsReportingEnabled"), 0);
        addD(o, e, QStringLiteral("SendSiteInfoToImproveServices"), 0);
        addD(o, e, QStringLiteral("SpotlightExperiencesAndRecommendationsEnabled"), 0);
        addD(o, e, QStringLiteral("UserFeedbackAllowed"), 0);
        addD(o, e, QStringLiteral("ConfigureDoNotTrack"), 1);
        addD(o, e, QStringLiteral("TrackingPrevention"), 3);
        addD(o, e, QStringLiteral("ResolveNavigationErrorsUseWebService"), 0);
        addD(o, e, QStringLiteral("AlternateErrorPagesEnabled"), 0);
        addD(o, e, QStringLiteral("EdgeCollectionsEnabled"), 0);
        addD(o, e, QStringLiteral("ShowMicrosoftRewards"), 0);
        addD(o, e, QStringLiteral("AutofillAddressEnabled"), 0);
        addD(o, e, QStringLiteral("AutofillCreditCardEnabled"), 0);
        addD(o, e, QStringLiteral("PasswordManagerEnabled"), 0);
        addD(o, e, QStringLiteral("SyncDisabled"), 1);
        addD(o, e, QStringLiteral("BrowserSignin"), 0);
        addD(o, e, QStringLiteral("NewTabPageContentEnabled"), 0);
        addD(o, e, QStringLiteral("NewTabPageQuickLinksEnabled"), 0);
        addD(o, e, QStringLiteral("NewTabPageManagedQuickLinks"), 0);
        addD(o, e, QStringLiteral("NewTabPageHideDefaultTopSites"), 1);
        addD(o, e, QStringLiteral("MicrosoftEdgeInsiderPromotionEnabled"), 0);
        addD(o, e, QStringLiteral("ShowRecommendationsEnabled"), 0);
        addD(o, e, QStringLiteral("WebWidgetAllowed"), 0);
        addD(o, e, QStringLiteral("HubsSidebarEnabled"), 0);
        addD(o, e, QStringLiteral("EdgeWorkspacesEnabled"), 0);
        addD(o, e, QStringLiteral("SearchSuggestEnabled"), 0);
        addD(o, e, QStringLiteral("EdgeAssetDeliveryServiceEnabled"), 0);
        addD(o, e, QStringLiteral("CryptoWalletEnabled"), 0);
        addD(o, e, QStringLiteral("CopilotCDPPageContext"), 0);
        addD(o, e, QStringLiteral("DiscoverPageContextEnabled"), 0);
        addD(o, e, QStringLiteral("GenAIDefaultSettings"), 3);
        addD(o, e, QStringLiteral("HoverActionsEnabled"), 0);
        t.registryOps = o;
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("browser.chrome-brave-harden");
        t.category = QStringLiteral("browseroffice");
        t.subCategory = QStringLiteral("Chrome");
        t.name = QStringLiteral("Harden Chrome and Brave");
        t.description = QStringLiteral("Applies enterprise policy to Chrome and Brave: turns off metrics reporting, the Chrome cleanup scanner and its reporting, user feedback and background mode, and forces hardware acceleration on. Also disables the built-in spellchecker and browser component updates (component updates deliver things like the certificate-revocation set and Widevine), so leave this off if you rely on spellcheck. Reverting deletes the policy keys.");
        t.iconName = QStringLiteral("shield-check");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        QVector<RegistryOp> o;
        for (const QString& base : {
                QStringLiteral("HKLM\\SOFTWARE\\Policies\\Google\\Chrome"),
                QStringLiteral("HKLM\\SOFTWARE\\Policies\\BraveSoftware\\Brave") }) {
            addD(o, base, QStringLiteral("MetricsReportingEnabled"), 0);
            addD(o, base, QStringLiteral("ChromeCleanupEnabled"), 0);
            addD(o, base, QStringLiteral("ChromeCleanupReportingEnabled"), 0);
            addD(o, base, QStringLiteral("UserFeedbackAllowed"), 0);
            addD(o, base, QStringLiteral("SpellcheckEnabled"), 0);
            addD(o, base, QStringLiteral("BackgroundModeEnabled"), 0);
            addD(o, base, QStringLiteral("ComponentUpdatesEnabled"), 0);
            addD(o, base, QStringLiteral("HardwareAccelerationModeEnabled"), 1);
        }
        t.registryOps = o;
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("browser.firefox-harden");
        t.category = QStringLiteral("browseroffice");
        t.subCategory = QStringLiteral("Firefox");
        t.name = QStringLiteral("Harden Firefox");
        t.description = QStringLiteral(
            "Disables Firefox telemetry, Studies, the default-browser agent and first-run / "
            "post-update pages via enterprise policy.");
        t.iconName = QStringLiteral("shield-check");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        const QString f = QStringLiteral("HKLM\\SOFTWARE\\Policies\\Mozilla\\Firefox");
        t.registryOps = {
            dwordOp(f, QStringLiteral("DisableTelemetry"), 1, std::nullopt),
            dwordOp(f, QStringLiteral("DisableDefaultBrowserAgent"), 1, std::nullopt),
            dwordOp(f, QStringLiteral("DisableFirefoxStudies"), 1, std::nullopt),
            stringOp(f, QStringLiteral("OverrideFirstRunPage"), QString(), std::nullopt),
            stringOp(f, QStringLiteral("OverridePostUpdatePage"), QString(), std::nullopt),
            dwordOp(f, QStringLiteral("DontCheckDefaultBrowser"), 1, std::nullopt),
        };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("office.telemetry-privacy-off");
        t.category = QStringLiteral("browseroffice");
        t.subCategory = QStringLiteral("Office");
        t.name = QStringLiteral("Disable Office telemetry and connected experiences");
        t.description = QStringLiteral(
            "Turns off Microsoft Office telemetry, the feedback channel, the Customer "
            "Experience program and optional connected experiences, and disables link "
            "auto-update, animations and recent-document tracking across Word, Excel, "
            "PowerPoint and Outlook. Covers Office 2013 (15.0) and 2016/2019/365 (16.0).");
        t.iconName = QStringLiteral("eye-off");
        t.impact = Impact::Low;
        const QString root = QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Office\\");
        QVector<RegistryOp> o;
        for (const QString& ver : { QStringLiteral("15.0"), QStringLiteral("16.0") }) {
            const QString common = root + ver + QStringLiteral("\\Common\\");
            addD(o, common + QStringLiteral("General"), QStringLiteral("ShownFirstRunOptin"), 1);
            addD(o, common + QStringLiteral("General"), QStringLiteral("DisableBootToOfficeStart"), 1);
            addD(o, common + QStringLiteral("ClientTelemetry"), QStringLiteral("DisableTelemetry"), 1);
            addD(o, common + QStringLiteral("ClientTelemetry"), QStringLiteral("VerboseLogging"), 0);
            addD(o, common + QStringLiteral("Feedback"), QStringLiteral("Enabled"), 0);
            addD(o, common + QStringLiteral("Feedback"), QStringLiteral("IncludeEmail"), 0);
            addD(o, common + QStringLiteral("Privacy"), QStringLiteral("QMEnable"), 0);
            addD(o, common + QStringLiteral("Privacy"), QStringLiteral("DisconnectedState"), 2);
            addD(o, common + QStringLiteral("Privacy"), QStringLiteral("UserContentDisabled"), 2);
            addD(o, common + QStringLiteral("Privacy"), QStringLiteral("DownloadContentDisabled"), 2);
            addD(o, common + QStringLiteral("Privacy"), QStringLiteral("ControllerConnectedServicesEnabled"), 2);
            const QString anon = common + QStringLiteral("Privacy\\SettingsStore\\Anonymous");
            addD(o, anon, QStringLiteral("OptionalConnectedExperiencesNoticeVersion"), 2);
            addD(o, anon, QStringLiteral("ConnectedExperiencesNoticeVersion"), 2);
            for (const QString& app : {
                    QStringLiteral("Word"), QStringLiteral("Excel"),
                    QStringLiteral("PowerPoint"), QStringLiteral("Outlook") }) {
                const QString opt = root + ver + QStringLiteral("\\") + app + QStringLiteral("\\Options");
                addD(o, opt, QStringLiteral("DontUpdateLinks"), 1);
                addD(o, opt, QStringLiteral("DisableAnimation"), 1);
                addD(o, opt, QStringLiteral("NoTrack"), 1);
            }
        }
        o.push_back(dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Office\\16.0\\Common\\OfficeUpdate"),
                            QStringLiteral("PreventTeamsInstall"), 1, std::nullopt));
        t.registryOps = o;
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("browser.ie-security-zones");
        t.category = QStringLiteral("browseroffice");
        t.name = QStringLiteral("Internet Explorer zone lockdown");
        t.description = QStringLiteral(
            "Disables scriptlets and miscellaneous active content in the IE Internet zone.");
        t.iconName = QStringLiteral("globe-lock");
        t.impact = Impact::Low;
        t.concern = QStringLiteral(
            "outdated: Internet Explorer is retired; these Zone 3 values only affect the few "
            "legacy components and WebOC hosts that still use the IE engine.");
        const QString z = QStringLiteral(
            "HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Zones\\3");
        t.registryOps = {
            dwordOp(z, QStringLiteral("1206"), 0, std::nullopt),
            dwordOp(z, QStringLiteral("2500"), 0, std::nullopt),
        };
        c.push_back(t);
    }
}

} // namespace vth::tweaks::detail
