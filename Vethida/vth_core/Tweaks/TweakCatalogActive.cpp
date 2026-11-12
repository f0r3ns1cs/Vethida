#include "TweakEngine.h"
#include "DeviceOps.h"

#include "../Windows/NvidiaAnsel.h"

#include <QtCore/QByteArray>

namespace vth::tweaks::detail
{
    namespace
    {
        void addD(QVector<RegistryOp>& ops, const QString& path, const QString& name, quint32 v) {
            ops.push_back(dwordOp(path, name, v, std::nullopt));
        }

        // PowerShell, for state outside the registry. The engine pins it to System32.
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

        namespace dev = vth::tweaks::devices;

        struct RegTarget {
            QString path;
            QString name;
            quint32 value = 0;
        };

        QString snapKey(const QString& path, const QString& name) {
            return path + QLatin1Char('|') + name;
        }

        // Writes each target, snapshotting the prior value first so revert is exact.
        bool applyTargets(const QString& id, const QVector<RegTarget>& targets) {
            QMap<QString, QString> snap;
            bool ok = true;
            for (const RegTarget& t : targets) {
                QString prior = QStringLiteral("-");
                if (const auto v = dev::regReadDword(t.path, t.name))
                    prior = QString::number(*v);
                snap.insert(snapKey(t.path, t.name), prior);
                ok = dev::regWriteDword(t.path, t.name, t.value) && ok;
            }
            dev::saveSnapshot(id, snap);
            return ok;
        }

        // Restores each snapshotted prior value, or deletes values that did not exist.
        bool revertTargets(const QString& id) {
            const QMap<QString, QString> snap = dev::loadSnapshot(id);
            bool ok = true;
            for (auto it = snap.begin(); it != snap.end(); ++it) {
                const int bar = it.key().lastIndexOf(QLatin1Char('|'));
                if (bar < 0) continue;
                const QString path = it.key().left(bar);
                const QString name = it.key().mid(bar + 1);
                if (it.value() == QStringLiteral("-"))
                    ok = dev::regDeleteValue(path, name) && ok;
                else
                    ok = dev::regWriteDword(path, name, it.value().toUInt()) && ok;
            }
            dev::clearSnapshot(id);
            return ok;
        }

        DetectState detectTargets(const QVector<RegTarget>& targets) {
            if (targets.isEmpty()) return DetectState::Unknown;
            int on = 0, total = 0;
            for (const RegTarget& t : targets) {
                ++total;
                const auto v = dev::regReadDword(t.path, t.name);
                if (v && *v == t.value) ++on;
            }
            return on == total ? DetectState::On : DetectState::Off;
        }

        // Re-enumerated each call, so hot-plugged adapters are picked up.
        DynamicOp targetOp(const QString& id, std::function<QVector<RegTarget>()> build) {
            DynamicOp op;
            op.label  = id;
            op.detect = [build]() { return detectTargets(build()); };
            op.apply  = [id, build]() { return applyTargets(id, build()); };
            op.revert = [id]() { return revertTargets(id); };
            return op;
        }

        RegTarget dwordT(const QString& path, const QString& name, quint32 v) {
            return RegTarget{ path, name, v };
        }
    }

    namespace {
struct BcdEdit {
    QString                element;
    std::optional<QString> applied;
};
DynamicOp bcdOp(const QString& id, const QVector<BcdEdit>& edits) {
    DynamicOp op;
    op.label  = id;
    op.detect = []() { return DetectState::Unknown; };
    op.apply  = [id, edits]() {
        QMap<QString, QString> snap;
        for (const BcdEdit& e : edits) {
            const auto prior = dev::bcdReadElement(e.element);
            snap.insert(e.element, prior ? QStringLiteral("v:") + *prior : QStringLiteral("-"));
        }
        dev::saveSnapshot(id, snap);
        bool ok = true;
        for (const BcdEdit& e : edits) {
            if (e.applied.has_value())
                ok = dev::bcdSetElement(e.element, *e.applied) && ok;
            else if (snap.value(e.element) != QStringLiteral("-"))
                ok = dev::bcdDeleteElement(e.element) && ok;
        }
        return ok;
    };
    op.revert = [id, edits]() {
        const QMap<QString, QString> snap = dev::loadSnapshot(id);
        bool ok = true;
        for (const BcdEdit& e : edits) {
            const QString v = snap.value(e.element, QStringLiteral("-"));
            if (v.startsWith(QStringLiteral("v:")))
                ok = dev::bcdSetElement(e.element, v.mid(2)) && ok;
            else if (e.applied.has_value())
                ok = dev::bcdDeleteElement(e.element) && ok;
        }
        dev::clearSnapshot(id);
        return ok;
    };
    return op;
}
}

void appendActive(QVector<TweakSpec>& c) {
        const QString kAdvanced = QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced");
        const QString kFs   = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\FileSystem");
        const QString kGd  = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers");

        {
            TweakSpec t;
            t.id = QStringLiteral("mm.system-responsiveness");
            t.category = QStringLiteral("general");
            t.subCategory = QStringLiteral("Multimedia (MMCSS)");
            t.name = QStringLiteral("Give games and media more processor time");
            t.description = QStringLiteral(
                "Windows keeps 20% of the processor free for background work while games "
                "and media play. This lowers it to 10%.");
            t.iconName = QStringLiteral("gauge");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"
                                       "\\Multimedia\\SystemProfile"),
                        QStringLiteral("SystemResponsiveness"), 10, 20),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("appear.show-file-extensions");
            t.category = QStringLiteral("appearance");
            t.subCategory = QStringLiteral("Explorer");
            t.name = QStringLiteral("Show file extensions");
            t.description = QStringLiteral(
                "Shows the extension (.exe, .pdf, .docx) on every file in File Explorer, so"
                " a program disguised as a document is easy to spot.");
            t.iconName = QStringLiteral("folder-cog");
            t.impact = Impact::Low;
            t.registryOps = {
                dwordOp(kAdvanced, QStringLiteral("HideFileExt"), 0, 1),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("storage.ntfs-long-paths");
            t.category = QStringLiteral("storage");
            t.subCategory = QStringLiteral("File paths");
            t.name = QStringLiteral("Enable long paths");
            t.description = QStringLiteral(
                "Lets programs that support it use file paths longer than 260 characters. "
                "Other programs are unaffected.");
            t.iconName = QStringLiteral("file");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.registryOps = {
                dwordOp(kFs, QStringLiteral("LongPathsEnabled"), 1, 0),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("usb.selective-suspend-off");
            t.category = QStringLiteral("power");
            t.subCategory = QStringLiteral("USB power");
            t.name = QStringLiteral("Disable USB selective suspend");
            t.description = QStringLiteral(
                "Stops Windows putting idle USB devices to sleep, so mice, keyboards and "
                "headsets respond without a wake-up delay. Some devices and power plans "
                "override it.");
            t.iconName = QStringLiteral("usb");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.effects = {
                QStringLiteral("USB devices draw a little more power at idle."),
            };
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\USB"),
                        QStringLiteral("DisableSelectiveSuspend"), 1, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("perf.win32priorityseparation");
            t.category = QStringLiteral("advanced");
            t.name = QStringLiteral("Give the active window more processor time");
            t.description = QStringLiteral(
                "Gives the window you are working in a bigger share of the processor "
                "than the ones behind it, and switches between them more often.");
            t.iconName = QStringLiteral("cpu");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\PriorityControl"),
                        QStringLiteral("Win32PrioritySeparation"), 0x26, 0x2),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("priv.telemetry-off");
            t.category = QStringLiteral("privacy");
            t.subCategory = QStringLiteral("Telemetry");
            t.name = QStringLiteral("Disable Windows telemetry");
            t.description = QStringLiteral(
                "Sets Windows' diagnostic data to the minimum and turns off the app "
                "inventory and compatibility data collectors.");
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
            t.id = QStringLiteral("sec.defender-deprioritize");
            t.category = QStringLiteral("security");
            t.subCategory = QStringLiteral("Defender");
            t.name = QStringLiteral("Deprioritize Defender scans");
            t.description = QStringLiteral(
                "Keeps Defender running, but at the lowest processor and disk priority, so "
                "its background scans no longer slow down what you are doing.");
            t.iconName = QStringLiteral("shield");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "safer alternative to a full disable: protection stays on, only its scheduling "
                "priority drops. Real-time scans are still synchronous, so the win is bounded.");
            const QString ifeo = QStringLiteral(
                "HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution "
                "Options\\MsMpEng.exe\\PerfOptions");
            t.registryOps = {
                dwordOp(ifeo, QStringLiteral("CpuPriorityClass"), 1, std::nullopt),
                dwordOp(ifeo, QStringLiteral("IoPriority"), 0, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.enable-hags");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("GPU scheduling");
            t.name = QStringLiteral("Hardware-accelerated GPU scheduling");
            t.description = QStringLiteral(
                "Lets the graphics card schedule its own work instead of the processor "
                "doing it. Can lower latency in games on recent graphics cards.");
            t.iconName = QStringLiteral("monitor");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "GPU-dependent: a win on RTX 40+/RX 7000+, but adds latency on RTX 20/30 "
                "and RX 5000/6000. Often already on by default on supported GPUs.");
            t.registryOps = { dwordOp(kGd, QStringLiteral("HwSchMode"), 2, 1) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("net.throttling-index-off");
            t.category = QStringLiteral("networking");
            t.subCategory = QStringLiteral("Network throttling");
            t.name = QStringLiteral("Relax network throttling");
            t.description = QStringLiteral(
                "Loosens the limit Windows puts on network traffic while games and media "
                "play, without removing it.");
            t.iconName = QStringLiteral("network");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            // Don't use 0xFFFFFFFF, it raises DPC latency and hurts audio. 20 is enough.
            t.concern = QStringLiteral(
                "uses 20, not the popular 0xFFFFFFFF which degrades audio "
                "DPC scheduling. Benefit is modest and NIC-dependent.");
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"
                                       "\\Multimedia\\SystemProfile"),
                        QStringLiteral("NetworkThrottlingIndex"), 20, 0xAu),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("debloat.disable-gamebar");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Xbox & Game Bar");
            t.name = QStringLiteral("Disable Xbox Game Bar");
            t.description = QStringLiteral(
                "Turns off the Xbox Game Bar overlay, its startup tips and automatic Game "
                "Mode.");
            t.iconName = QStringLiteral("gamepad-2");
            t.impact = Impact::Low;
            const QString gb = QStringLiteral("HKCU\\Software\\Microsoft\\GameBar");
            t.registryOps = {
                dwordOp(gb, QStringLiteral("AllowAutoGameMode"), 0, 1),
                dwordOp(gb, QStringLiteral("ShowStartupPanel"), 0, 1),
                dwordOp(gb, QStringLiteral("UseNexusForGameBarEnabled"), 0, 1),
                dwordOp(gb, QStringLiteral("GamePanelStartupTipIndex"), 3, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("browser.edge-harden");
            t.category = QStringLiteral("browseroffice");
            t.subCategory = QStringLiteral("Edge");
            t.name = QStringLiteral("Harden Microsoft Edge");
            t.description = QStringLiteral("Applies Microsoft's own Edge policies: no first-run or default-browser prompts, no startup boost or background running, strict tracking prevention and sleeping tabs, and Rewards, Collections, the sidebar, sync, sign-in and Copilot turned off.");
            t.warnings = {
                QStringLiteral("Edge's built-in password manager and autofill are turned off too."),
            };
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
            t.id = QStringLiteral("update.block-windows-update");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Windows Update");
            t.name = QStringLiteral("Disable Windows updates");
            t.description = QStringLiteral(
                "Turns Windows Update off: no automatic scanning, downloading or "
                "installing, no forced restarts, and no drivers from Windows Update. "
                "The self-repair component that would switch updating back on is "
                "stopped too.");
            t.iconName = QStringLiteral("refresh-cw");
            t.impact = Impact::Critical;
            t.askFirst = true;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("Security patches stop arriving while this is on."),
            };
            t.concern = QStringLiteral(
                "risky: a blanket update disable means no security "
                "patches while it is applied. Never suggest it unprompted.");

            const QString ux   = QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\WindowsUpdate\\UX\\Settings");
            const QString waas = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\WaaSMedicSvc");
            const QString au   = QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate\\AU");
            const QString upol = QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\WindowsUpdate\\UpdatePolicy\\Settings");
            const QString pm   = QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\PolicyManager\\current\\device\\Update");
            const QString pStart = QStringLiteral("2025-01-01T00:00:00Z");
            const QString pEnd   = QStringLiteral("2051-12-31T00:00:00Z");

            // REG_BINARY is not captured, so revert writes the Windows default.
            RegistryOp failure;
            failure.path         = waas;
            failure.name         = QStringLiteral("FailureActions");
            failure.kind         = vth::services::RegistryValueKind::Binary;
            failure.appliedValue = QByteArray(44, '\0');
            static const unsigned char kFaDefault[44] = {
                0x84,0x03,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
                0x03,0x00,0x00,0x00, 0x14,0x00,0x00,0x00, 0x01,0x00,0x00,0x00,
                0xC0,0xD4,0x01,0x00, 0x01,0x00,0x00,0x00, 0xE0,0x93,0x04,0x00,
                0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
            };
            failure.revertValue = QByteArray(reinterpret_cast<const char*>(kFaDefault),
                                             static_cast<int>(sizeof(kFaDefault)));

            t.registryOps = {
                stringOp(ux, QStringLiteral("PauseFeatureUpdatesStartTime"), pStart, std::nullopt),
                stringOp(ux, QStringLiteral("PauseFeatureUpdatesEndTime"),   pEnd,   std::nullopt),
                stringOp(ux, QStringLiteral("PauseQualityUpdatesStartTime"), pStart, std::nullopt),
                stringOp(ux, QStringLiteral("PauseQualityUpdatesEndTime"),   pEnd,   std::nullopt),
                stringOp(ux, QStringLiteral("PauseUpdatesStartTime"),        pStart, std::nullopt),
                stringOp(ux, QStringLiteral("PauseUpdatesExpiryTime"),       pEnd,   std::nullopt),
                dwordOp(ux, QStringLiteral("ActiveHoursStart"),           0x0Du,   0x0Du),
                dwordOp(ux, QStringLiteral("ActiveHoursEnd"),             0x07u,   0x07u),
                dwordOp(ux, QStringLiteral("FlightSettingsMaxPauseDays"), 0x2727u, 0x2727u),
                dwordOp(waas, QStringLiteral("Start"), 4u, 3u),
                failure,
                dwordOp(au, QStringLiteral("AUOptions"),                       1u, 4u),
                dwordOp(au, QStringLiteral("NoAutoUpdate"),                    1u, 0u),
                dwordOp(au, QStringLiteral("NoAUShutdownOption"),              1u, 0u),
                dwordOp(au, QStringLiteral("AlwaysAutoRebootAtScheduledTime"), 0u, 0u),
                dwordOp(au, QStringLiteral("NoAutoRebootWithLoggedOnUsers"),   1u, 1u),
                dwordOp(au, QStringLiteral("AutoInstallMinorUpdates"),         0u, 1u),
                dwordOp(au, QStringLiteral("UseWUServer"),                     0u, std::nullopt),
                dwordOp(upol, QStringLiteral("PausedFeatureStatus"), 1u, 0u),
                dwordOp(upol, QStringLiteral("PausedQualityStatus"), 1u, 0u),
                stringOp(upol, QStringLiteral("PausedQualityDate"), pStart, std::nullopt),
                stringOp(upol, QStringLiteral("PausedFeatureDate"), pStart, std::nullopt),
                dwordOp(pm, QStringLiteral("ExcludeWUDriversInQualityUpdate"), 1u, 0u),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("perf.uwp.resource_policy_uncap");
            t.category = QStringLiteral("advanced");
            t.subCategory = QStringLiteral("Resource scheduling");
            t.name = QStringLiteral("Remove app resource throttling");
            t.description = QStringLiteral(
                "Raises every one of Windows' built-in app and background-task resource "
                "classes to top priority and lifts their processor, disk and memory "
                "limits, so the resource manager stops holding any category of app or "
                "task back.");
            t.iconName = QStringLiteral("zap");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.warnings = {
                QStringLiteral("Any app, a misbehaving one included, can take processor, disk and battery without limits."),
            };
            t.effects = {
                QStringLiteral("Lifting every limit at once can make the PC less responsive under load."),
            };
            t.concern = QStringLiteral(
                "blunt: raising every resource class to top priority can "
                "cause priority inversion, and the gain is not measured. Never recommended "
                "unprompted.");

            static const char* const kPolicySets[] = {
                "ApplicationService", "ApplicationServiceElastic",
                "ApplicationServiceHighPriElastic", "ApplicationServiceHighPriority",
                "ApplicationServiceRemote", "AppToAppTarget", "BackgroundAudioPlayer",
                "BackgroundCachedFileUpdater", "BackgroundTaskCompletion",
                "BackgroundTaskDebug", "BackgroundTransfer",
                "BackgroundTransferNetworkState", "Balloon", "CalendarProviderAsChild",
                "CallingEvent", "CallingEventHighPriority", "ChatMessageNotification",
                "ComponentTarget", "ContinuousBackgroundExecution",
                "CortanaSpeechBackground", "CreateProcess", "DefaultModernBackgroundTask",
                "DefaultPPLE", "DefaultPPLE2", "EmCreateProcess",
                "EmCreateProcessNormalPriority", "EmptyHost", "EmptyHostHighPriority",
                "EmptyHostPPLE", "FileProviderTarget", "ForegroundAgent",
                "ForegroundCachedFileUpdater", "ForegroundTaskCompletion", "Frozen",
                "GenericExtendedExecution", "GeofenceTask", "HighPriorityBackgroundAgent",
                "HighPriorityBackgroundDemoted", "HighPriorityBackgroundTransfer",
                "IoTStartupTask", "JumboForegroundAgent", "LmaBackgroundTaskCompletion",
                "LmaDefaultModernBackgroundTask", "LmaPrelaunchForeground",
                "LmaUiDebugModeForeground", "LmaUiFrozen", "LmaUiFrozenDNCS",
                "LmaUiFrozenDNK", "LmaUiFrozenHighPriority", "LmaUiModernForeground",
                "LmaUiModernForegroundLarge", "LmaUiPaused", "LmaUiPausedDNK",
                "LmaUiPausedHighPriority", "LmaUiPausing", "LongRunningBluetooth",
                "LongRunningControlChannel", "LongRunningSensor", "MediaProcessing",
                "OemBackgroundAgent", "OemTask", "PendingDefaultPPLE", "PiP",
                "PreinstallTask", "PrelaunchForeground", "PushTriggerTask",
                "ResourceIntensive", "ShareDataPackageHost", "ShortRunningBluetooth",
                "TaskCompletionHighPriority", "UiComposer", "UiDebugModeForeground",
                "UiForegroundDNK", "UiFrozen", "UiFrozenDNCS", "UiFrozenDNK",
                "UiFrozenHighPriority", "UiLockScreen", "UiModernForeground",
                "UiModernForegroundExtended", "UiModernForegroundLarge", "UiOverlay",
                "UiPaused", "UiPausedDNK", "UiPausedHighPriority", "UiPausing",
                "UiPausingLowPriority", "UiShellCustom1", "UiShellCustom2",
                "UiShellCustom3", "UiShellCustom4", "VideoTranscoding",
                "VoipActiveCallBackground", "VoipActiveCallBackgroundPriority",
                "VoipActiveCallForeground", "VoipForegroundWorker",
                "VoipSuspendedBackground", "VoipWorker", "Vpn", "WebAuthSignIn",
            };
            const auto add6 = [](QVector<RegistryOp>& ops, const QString& setName) {
                const QString p = QStringLiteral(
                    "HKLM\\SYSTEM\\ResourcePolicyStore\\ResourceSets\\PolicySets\\") + setName;
                ops.push_back(stringOp(p, QStringLiteral("CPU"),               QStringLiteral("UnmanagedAboveNormal"), std::nullopt));
                ops.push_back(stringOp(p, QStringLiteral("ExternalResources"), QStringLiteral("ResourceIntensive"),    std::nullopt));
                ops.push_back(stringOp(p, QStringLiteral("Flags"),             QStringLiteral("Foreground"),           std::nullopt));
                ops.push_back(stringOp(p, QStringLiteral("Importance"),        QStringLiteral("Critical"),             std::nullopt));
                ops.push_back(stringOp(p, QStringLiteral("IO"),                QStringLiteral("NoCap"),                std::nullopt));
                ops.push_back(stringOp(p, QStringLiteral("Memory"),            QStringLiteral("NoCap"),                std::nullopt));
            };
            QVector<RegistryOp> o;
            o.reserve(static_cast<int>(sizeof(kPolicySets) / sizeof(kPolicySets[0])) * 6);
            for (const char* s : kPolicySets)
                add6(o, QString::fromLatin1(s));
            t.registryOps = o;
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("debloat.edge-remove-completely");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Microsoft Edge");
            t.name = QStringLiteral("Remove Microsoft Edge");
            t.description = QStringLiteral(
                "Removes Microsoft Edge, its updater and the WebView2 runtime. Runs "
                "Edge's own uninstaller first, then clears the file associations, "
                "Start menu entries and program folders it leaves behind.");
            t.iconName = QStringLiteral("globe");
            t.impact = Impact::High;
            t.requiresAdmin = true;
            t.reversible = false;
            t.warnings = {
                QStringLiteral("Getting Edge back means downloading it from Microsoft."),
                QStringLiteral("Removes the WebView2 runtime too. Teams, some Office add-ins and other apps built on it break until it is reinstalled from Get apps."),
                QStringLiteral("Set another default browser first, or web links will have nothing to open them."),
            };
            t.concern = QStringLiteral(
                "destructive: deletes install folders outright, and "
                "Windows servicing may reinstall Edge on a later feature update.");
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
            t.id = QStringLiteral("gpu.nvidia.clock_gating_disable");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("NVIDIA");
            t.name = QStringLiteral("Disable NVIDIA clock gating");
            t.description = QStringLiteral(
                "Stops the NVIDIA driver from stepping parts of the GPU down at idle, "
                "so it holds its performance clocks instead of ramping back up on "
                "demand.");
            t.iconName = QStringLiteral("bolt-off");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.effects = {
                QStringLiteral("The GPU stays at high clocks on the desktop, drawing more power and making more fan noise."),
                QStringLiteral("Battery life is shorter on a laptop."),
            };
            t.concern = QStringLiteral(
                "hardware-specific and undocumented: drives an internal driver power-feature "
                "flag whose meaning NVIDIA has never published, and the effect is not "
                "measured. Never suggest unprompted.");
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                const QStringList keys = dev::enumerateGpuClassKeys(QStringLiteral("VEN_10DE"));
                for (const QString& k : keys)
                    v << dwordT(k, QStringLiteral("RMPowerFeature"), 0x55455555u);
                return v;
            }) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("gpu.nvidia.ansel_disable");
            t.category = QStringLiteral("gpu");
            t.subCategory = QStringLiteral("NVIDIA");
            t.name = QStringLiteral("Disable NVIDIA Ansel");
            t.description = QStringLiteral(
                "Turns off the NVIDIA Ansel in-game photo overlay, including the "
                "capture hooks the driver loads into games for it.");
            t.iconName = QStringLiteral("monitor");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.effects = {
                QStringLiteral("The Ansel photo overlay no longer opens in games."),
            };
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SOFTWARE\\NVIDIA Corporation\\Global\\Ansel"),
                        QStringLiteral("EnableAnsel"), 0u, std::nullopt),
            };
            DynamicOp ansel;
            ansel.label  = QStringLiteral("gpu.nvidia.ansel_disable/nvcamera");
            ansel.apply  = []() { vth::win::nvidia::toggleAnsel(true);  return true; };
            ansel.revert = []() { vth::win::nvidia::toggleAnsel(false); return true; };
            t.dynamicOps = { ansel };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("net.ctcp-congestion");
            t.category = QStringLiteral("networking");
            t.name = QStringLiteral("Use Compound TCP");
            t.description = QStringLiteral(
                "Switches Windows to Compound TCP, which speeds up sending on fast "
                "connections.");
            t.iconName = QStringLiteral("network");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "outdated-leaning: modern Windows defaults to CUBIC, which is generally as "
                "good or better; CTCP is unlikely to help on a typical connection.");
            CommandOp op;
            op.program = QStringLiteral("netsh");
            op.applyArgs  = { QStringLiteral("int"), QStringLiteral("tcp"), QStringLiteral("set"),
                              QStringLiteral("supplemental"), QStringLiteral("template=internet"),
                              QStringLiteral("congestionprovider=ctcp") };
            op.revertArgs = { QStringLiteral("int"), QStringLiteral("tcp"), QStringLiteral("set"),
                              QStringLiteral("supplemental"), QStringLiteral("template=internet"),
                              QStringLiteral("congestionprovider=default") };
            t.commandOps = { op };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("net.hit-registration-priority");
            t.category = QStringLiteral("networking");
            t.name = QStringLiteral("Check local names first");
            t.description = QStringLiteral(
                "Has Windows check its own cache and hosts file before DNS and older "
                "NetBIOS lookups, and drops source-routed packets, an old attack technique.");
            t.iconName = QStringLiteral("network");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "the stock priorities (499/500/2000/2001) already rank the sources in the "
                "same relative order as the tweaked values (4/5/6/7), so the reorder likely "
                "has no measurable effect; the source-routing hardening is the real change. "
                "Never suggest unprompted.");
            const QString sp = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\ServiceProvider");
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"),
                        QStringLiteral("DisableIPSourceRouting"), 2, std::nullopt),
                dwordOp(sp, QStringLiteral("LocalPriority"), 4, 499u),
                dwordOp(sp, QStringLiteral("HostsPriority"), 5, 500u),
                dwordOp(sp, QStringLiteral("DnsPriority"), 6, 2000u),
                dwordOp(sp, QStringLiteral("NetbtPriority"), 7, 2001u),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("net.disable-nic-bindings");
            t.category = QStringLiteral("networking");
            t.subCategory = QStringLiteral("Adapter bindings");
            t.name = QStringLiteral("Remove unused network protocols");
            t.description = QStringLiteral(
                "Removes link-layer discovery, network mapping, file and printer sharing "
                "and the multiplexor protocol from every network adapter.");
            t.iconName = QStringLiteral("network");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.warnings = {
                QStringLiteral("File and printer sharing on your local network stops working."),
            };
            t.concern = QStringLiteral(
                "leaves the QoS packet scheduler (ms_pacer) and IPv6 (ms_tcpip6) bound on "
                "purpose: the former carries the DSCP game-traffic marking, the latter breaks "
                "modern services if removed. Disables file/printer sharing - skip if you share "
                "folders on a LAN.");
            CommandOp op;
            op.program = QStringLiteral("powershell");
            op.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"),
                             QStringLiteral("-Command"),
                             QStringLiteral("$ErrorActionPreference='SilentlyContinue';"
                             "'ms_lldp','ms_lltdio','ms_rspndr','ms_implat','ms_server','ms_msclient'|"
                             "%{Set-NetAdapterBinding -Name '*' -ComponentID $_ -Disabled}") };
            op.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"),
                              QStringLiteral("-Command"),
                              QStringLiteral("$ErrorActionPreference='SilentlyContinue';"
                              "'ms_lldp','ms_lltdio','ms_rspndr','ms_implat','ms_server','ms_msclient'|"
                              "%{Set-NetAdapterBinding -Name '*' -ComponentID $_ -Enabled}") };
            t.commandOps = { op };
            c.push_back(t);
        }

        const QString kKern = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\kernel");
        const QString kPc   = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\PriorityControl");

        {
            TweakSpec t;
            t.id = QStringLiteral("storage.nvme.classpnp_disable_perf_throttling");
            t.category = QStringLiteral("storage");
            t.subCategory = QStringLiteral("NVMe");
            t.name = QStringLiteral("Disable NVMe performance throttling");
            t.description = QStringLiteral(
                "Disables the storage class driver's thermal and power performance "
                "throttling for NVMe drives. Widely cited in community storage references "
                "though not officially documented; turning throttling off can raise "
                "sustained drive temperatures.");
            t.iconName = QStringLiteral("activity");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.warnings = {
                QStringLiteral("The drive can reach a higher temperature under long, heavy load."),
            };
            t.concern = QStringLiteral(
                "not officially documented but widely cited in community references as a real "
                "classpnp.sys parameter; disabling throttling can raise sustained drive "
                "temperatures. Effect not measured.");
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control\\Classpnp"),
                        QStringLiteral("NVMeDisablePerfThrottling"), 1u, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("perf.svchost-split-threshold");
            t.category = QStringLiteral("advanced");
            t.subCategory = QStringLiteral("Services");
            t.name = QStringLiteral("Separate service host processes");
            t.description = QStringLiteral(
                "Raises the memory limit Windows uses to decide whether to group system "
                "services into shared host processes, so each service runs in its own "
                "process. That isolates a faulting service and makes per-service resource "
                "use easier to read, at the cost of extra memory.");
            t.iconName = QStringLiteral("server");
            t.impact = Impact::Medium;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.effects = {
                QStringLiteral("Uses more memory, most noticeable on a PC with little RAM."),
            };
            t.concern = QStringLiteral(
                "often a no-op: Windows 10/11 with 3.5 GB RAM or more already splits services "
                "by default and this threshold already equals installed RAM, so the write "
                "usually changes nothing. Effect not measured.");
            t.dynamicOps = { targetOp(t.id, []() {
                QVector<RegTarget> v;
                if (const auto kb = dev::physicalRamKb()) {
                    const quint32 threshold = *kb > 0xFFFFFFFFull
                        ? 0xFFFFFFFFu : static_cast<quint32>(*kb);
                    v << dwordT(QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Control"),
                                QStringLiteral("SvcHostSplitThresholdInKB"), threshold);
                }
                return v;
            }) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("kernel.disable-tsx");
            t.category = QStringLiteral("advanced");
            t.name = QStringLiteral("Disable TSX");
            t.description = QStringLiteral(
                "Turns off the processor's Transactional Synchronization Extensions, which "
                "closes the TSX Asynchronous Abort class of side-channel attacks. Almost no "
                "desktop software uses TSX, so there is no practical slowdown.");
            t.iconName = QStringLiteral("cpu");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.requiresReboot = true;
            t.concern = QStringLiteral(
                "security hardening, not a speed change; on CPUs without TSX it does nothing.");
            t.registryOps = { dwordOp(kKern, QStringLiteral("DisableTsx"), 1u, 0u) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("kernel.scheduler-quantum-tuning");
            t.category = QStringLiteral("advanced");
            t.name = QStringLiteral("Scheduler and interrupt priority");
            t.description = QStringLiteral(
                "Sets interrupt priority hints, tells Windows this is a desktop rather than"
                " a tablet, and changes how the program you are using gets boosted. The "
                "interrupt settings are undocumented.");
            t.iconName = QStringLiteral("cpu");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "the interrupt-priority values and the scheduler-assist override are "
                "undocumented kernel internals with no published measurement. "
                "Win32PrioritySeparation is left out because the live foreground-quantum "
                "tweak owns that value.");
            t.registryOps = {
                dwordOp(kPc, QStringLiteral("ConvertibleSlateMode"), 0u, std::nullopt),
                dwordOp(kPc, QStringLiteral("IRQ00Priority"), 1u, std::nullopt),
                dwordOp(kPc, QStringLiteral("IRQ08Priority"), 4u, std::nullopt),
                dwordOp(kPc, QStringLiteral("IRQ16Priority"), 2u, std::nullopt),
                dwordOp(kPc, QStringLiteral("IRQ23Priority"), 3u, std::nullopt),
                dwordOp(kPc, QStringLiteral("IRQ8Priority"), 1u, std::nullopt),
                dwordOp(kPc, QStringLiteral("ForegroundBoost"), 1u, std::nullopt),
                dwordOp(kPc, QStringLiteral("ThreadBoostType"), 2u, std::nullopt),
                dwordOp(kPc, QStringLiteral("ThreadSchedulingModel"), 1u, std::nullopt),
                dwordOp(kPc, QStringLiteral("AdjustDpcThreshold"), 0x00000320u, std::nullopt),
                dwordOp(kPc, QStringLiteral("DeepIoCoalescingEnabled"), 1u, std::nullopt),
                dwordOp(kPc, QStringLiteral("SchedulerAssistThreadFlagOverride"), 1u, std::nullopt),
            };
            c.push_back(t);
        }

        {
        TweakSpec t;
        t.id = QStringLiteral("bcd.constant-boot-tick");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Boot");
        t.name = QStringLiteral("Constant boot timer");
        t.description = QStringLiteral(
            "Keeps the system timer ticking at a steady rate from boot, for more "
            "consistent timing.");
        t.iconName = QStringLiteral("timer");
        t.impact = Impact::Medium;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.effects = {
            QStringLiteral("Idle power draw rises slightly."),
        };
        t.concern = QStringLiteral(
            "edits the boot configuration; latency effect is hardware-dependent and often "
            "nil on modern systems. The enhanced tscsyncpolicy is the wrong choice on some "
            "multi-socket systems - revert restores the prior policy exactly. A store backup "
            "is taken before the write.");
        t.dynamicOps = { bcdOp(t.id, {
            BcdEdit{ QStringLiteral("useplatformtick"), QStringLiteral("yes") },
            BcdEdit{ QStringLiteral("tscsyncpolicy"), QStringLiteral("Enhanced") },
        }) };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bcd.classic-boot-screen");
        t.category = QStringLiteral("appearance");
        t.subCategory = QStringLiteral("Boot");
        t.name = QStringLiteral("Classic text-mode boot");
        t.description = QStringLiteral(
            "Replaces the animated startup logo with the classic text-mode boot: no "
            "animation, a plain progress screen, the legacy startup menu, and the list "
            "of drivers as they load.");
        t.iconName = QStringLiteral("monitor");
        t.impact = Impact::Low;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.concern = QStringLiteral(
            "cosmetic boot change; the driver list is a diagnostic aid, not a speed change. "
            "A store backup is taken before the write.");
        t.dynamicOps = { bcdOp(t.id, {
            BcdEdit{ QStringLiteral("quietboot"), QStringLiteral("yes") },
            BcdEdit{ QStringLiteral("bootux"), QStringLiteral("Disabled") },
            BcdEdit{ QStringLiteral("bootmenupolicy"), QStringLiteral("Legacy") },
            BcdEdit{ QStringLiteral("sos"), QStringLiteral("yes") },
        }) };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bcd.disable-dep");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Boot");
        t.name = QStringLiteral("Disable Data Execution Prevention");
        t.description = QStringLiteral(
            "Turns off Data Execution Prevention, which stops code from running in "
            "memory meant only for data. It is one of Windows' oldest and most "
            "effective exploit defenses.");
        t.iconName = QStringLiteral("shield");
        t.impact = Impact::Critical;
        t.askFirst = true;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Removes a core exploit protection. Only for a PC that runs nothing untrusted."),
            QStringLiteral("Some anti-cheat and security software refuses to run without it."),
        };
        t.concern = QStringLiteral(
            "severe security downgrade, explicitly owner-requested: disables DEP for the "
            "entire system. Never suggest unprompted; never include in an automatic set.");
        t.dynamicOps = { bcdOp(t.id, {
            BcdEdit{ QStringLiteral("nx"), QStringLiteral("AlwaysOff") },
        }) };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bcd.disable-elam");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Boot");
        t.name = QStringLiteral("Disable Early Launch Anti-Malware");
        t.description = QStringLiteral(
            "Stops Windows loading Early Launch Anti-Malware drivers at boot. This is "
            "what lets security software check other boot-start drivers as they load, "
            "part of how the system blocks boot-time malware. Turning it off removes "
            "that early check.");
        t.iconName = QStringLiteral("shield");
        t.impact = Impact::High;
        t.askFirst = true;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Boot drivers load without the early malware check."),
        };
        t.effects = {
            QStringLiteral("Takes effect after a restart."),
        };
        t.concern = QStringLiteral(
            "security downgrade, explicitly owner-requested: disables the Early Launch "
            "Anti-Malware boot check. Never suggest unprompted.");
        t.dynamicOps = { bcdOp(t.id, {
            BcdEdit{ QStringLiteral("disableelamdrivers"), QStringLiteral("yes") },
        }) };
        c.push_back(t);
    }

    {
        TweakSpec t;
        t.id = QStringLiteral("bcd.disable-x2apic");
        t.category = QStringLiteral("advanced");
        t.subCategory = QStringLiteral("Boot");
        t.name = QStringLiteral("Disable processor x2APIC");
        t.description = QStringLiteral(
            "Forces the processor's interrupt controller into legacy mode instead of "
            "x2APIC. This is a niche compatibility change with no measured benefit, and "
            "it can stop the machine booting on hardware that requires x2APIC.");
        t.iconName = QStringLiteral("cpu");
        t.impact = Impact::High;
        t.askFirst = true;
        t.requiresAdmin = true;
        t.requiresReboot = true;
        t.warnings = {
            QStringLiteral("Can stop Windows from booting on PCs that need x2APIC, including many with high core counts. Keep recovery media at hand."),
        };
        t.concern = QStringLiteral(
            "harmful on high-core-count CPUs and platforms that require x2APIC; no measured "
            "benefit. Revert restores the prior policy exactly. Never suggest unprompted.");
        t.dynamicOps = { bcdOp(t.id, {
            BcdEdit{ QStringLiteral("x2apicpolicy"), QStringLiteral("Disable") },
        }) };
        c.push_back(t);
    }

    const QString kAdv2 = QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced");

        {
            TweakSpec t;
            t.id = QStringLiteral("appear.dark-mode");
            t.category = QStringLiteral("appearance");
            t.subCategory = QStringLiteral("Theme");
            t.name = QStringLiteral("Dark mode");
            t.description = QStringLiteral(
                "Switches Windows apps and system surfaces to the dark color scheme.");
            t.iconName = QStringLiteral("monitor");
            t.impact = Impact::Low;
            const QString p = QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize");
            t.registryOps = {
                dwordOp(p, QStringLiteral("AppsUseLightTheme"), 0u, 1u),
                dwordOp(p, QStringLiteral("SystemUsesLightTheme"), 0u, 1u),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("appear.24hr-clock");
            t.category = QStringLiteral("appearance");
            t.subCategory = QStringLiteral("Clock");
            t.name = QStringLiteral("24-hour clock");
            t.description = QStringLiteral(
                "Shows the taskbar clock and time fields in 24-hour format.");
            t.iconName = QStringLiteral("clock");
            t.impact = Impact::Low;
            const QString p = QStringLiteral("HKCU\\Control Panel\\International");
            t.registryOps = {
                stringOp(p, QStringLiteral("sTimeFormat"), QStringLiteral("HH:mm:ss"), QStringLiteral("h:mm:ss tt")),
                stringOp(p, QStringLiteral("sShortTime"), QStringLiteral("HH:mm"), QStringLiteral("h:mm tt")),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("appear.clock-seconds");
            t.category = QStringLiteral("appearance");
            t.subCategory = QStringLiteral("Clock");
            t.name = QStringLiteral("Show seconds in taskbar clock");
            t.description = QStringLiteral(
                "Adds seconds to the taskbar clock.");
            t.iconName = QStringLiteral("clock");
            t.impact = Impact::Low;
            t.concern = QStringLiteral(
                "the once-a-second redraw wakes the CPU slightly more often; a minor, "
                "usually-unmeasurable power cost.");
            t.registryOps = {
                dwordOp(kAdv2, QStringLiteral("ShowSecondsInSystemClock"), 1u, 0u),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("appear.small-taskbar");
            t.category = QStringLiteral("appearance");
            t.subCategory = QStringLiteral("Taskbar");
            t.name = QStringLiteral("Small taskbar icons");
            t.description = QStringLiteral(
                "Shrinks the taskbar to the smaller icon size.");
            t.iconName = QStringLiteral("layout-grid");
            t.impact = Impact::Low;
            t.concern = QStringLiteral(
                "no effect on Windows 11, which removed this setting; Windows 10 only.");
            t.registryOps = {
                dwordOp(kAdv2, QStringLiteral("TaskbarSmallIcons"), 1u, 0u),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("appear.hide-taskbar-search");
            t.category = QStringLiteral("appearance");
            t.subCategory = QStringLiteral("Taskbar");
            t.name = QStringLiteral("Hide taskbar search");
            t.description = QStringLiteral(
                "Removes the search box and button from the taskbar.");
            t.iconName = QStringLiteral("eye-off");
            t.impact = Impact::Low;
            t.registryOps = {
                dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Search"),
                        QStringLiteral("SearchboxTaskbarMode"), 0u, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("appear.classic-alt-tab");
            t.category = QStringLiteral("appearance");
            t.subCategory = QStringLiteral("Window switching");
            t.name = QStringLiteral("Classic Alt+Tab");
            t.description = QStringLiteral(
                "Restores the classic pre-Aero Alt+Tab switcher.");
            t.iconName = QStringLiteral("layout-grid");
            t.impact = Impact::Low;
            t.registryOps = {
                dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer"),
                        QStringLiteral("AltTabSettings"), 1u, 0u),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("appear.classic-tray-flyouts");
            t.category = QStringLiteral("appearance");
            t.subCategory = QStringLiteral("System tray");
            t.name = QStringLiteral("Classic tray flyouts");
            t.description = QStringLiteral(
                "Restores the older clock, network, volume and battery pop-ups in the "
                "system tray in place of the current ones.");
            t.iconName = QStringLiteral("layout-grid");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "Windows 10-era: on Windows 11 most of these flyouts no longer exist, so the "
                "writes have no effect there.");
            const QString imm = QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ImmersiveShell");
            t.registryOps = {
                dwordOp(imm, QStringLiteral("UseWin32TrayClockExperience"), 1u, 0u),
                dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Control Panel\\Settings\\Network"),
                        QStringLiteral("ReplaceVan"), 2u, 0u),
                dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\MTCUVC"),
                        QStringLiteral("EnableMtcUvc"), 0u, 1u),
                dwordOp(imm, QStringLiteral("UseWin32BatteryFlyout"), 1u, 0u),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("appear.black-accent");
            t.category = QStringLiteral("appearance");
            t.subCategory = QStringLiteral("Theme");
            t.name = QStringLiteral("Black menu accent");
            t.description = QStringLiteral(
                "Forces menu accents to black instead of following the system accent color.");
            t.iconName = QStringLiteral("sliders-horizontal");
            t.impact = Impact::Low;
            t.concern = QStringLiteral(
                "sets only the menu accent.");
            t.registryOps = {
                dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Accent"),
                        QStringLiteral("AccentColorMenu"), 0u, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("appear.plain-signin-bg");
            t.category = QStringLiteral("appearance");
            t.subCategory = QStringLiteral("Sign-in");
            t.name = QStringLiteral("Plain sign-in background");
            t.description = QStringLiteral(
                "Replaces the sign-in screen's background image with a solid color.");
            t.iconName = QStringLiteral("lock");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System"),
                        QStringLiteral("DisableLogonBackgroundImage"), 1u, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("appear.disable-notification-center");
            t.category = QStringLiteral("appearance");
            t.subCategory = QStringLiteral("Notifications");
            t.name = QStringLiteral("Disable notification center");
            t.description = QStringLiteral(
                "Turns off the Windows notification center and the promotional balloon tips "
                "the system shows in the tray.");
            t.iconName = QStringLiteral("eye-off");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.effects = {
                QStringLiteral("Notifications and the notification panel stop appearing."),
            };
            const QString ex = QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Explorer");
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ImmersiveShell"),
                        QStringLiteral("UseActionCenterExperience"), 0u, 1u),
                dwordOp(ex, QStringLiteral("DisableNotificationCenter"), 1u, std::nullopt),
                dwordOp(ex, QStringLiteral("EnableLegacyBalloonNotifications"), 0u, std::nullopt),
                dwordOp(ex, QStringLiteral("NoBalloonFeatureAdvertisements"), 1u, std::nullopt),
                dwordOp(ex, QStringLiteral("NoSystraySystemPromotion"), 1u, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("priv.hide-last-user");
            t.category = QStringLiteral("privacy");
            t.subCategory = QStringLiteral("Sign-in");
            t.name = QStringLiteral("Hide last signed-in user");
            t.description = QStringLiteral(
                "Stops the sign-in screen from showing the last account name and the locked "
                "user, so someone at the machine cannot read who uses it.");
            t.iconName = QStringLiteral("eye-off");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.effects = {
                QStringLiteral("The sign-in screen asks for a user name every time."),
            };
            const QString sys = QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System");
            t.registryOps = {
                dwordOp(sys, QStringLiteral("DontDisplayLastUserName"), 1u, 0u),
                dwordOp(sys, QStringLiteral("DontDisplayLockedUserId"), 3u, 0u),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("appear.win10-context-menu");
            t.category = QStringLiteral("appearance");
            t.subCategory = QStringLiteral("Context menu");
            t.name = QStringLiteral("Windows 10 context menu");
            t.description = QStringLiteral(
                "Restores the full right-click menu from Windows 10 in place of the "
                "shortened Windows 11 one.");
            t.iconName = QStringLiteral("settings");
            t.impact = Impact::Low;
            t.effects = {
                QStringLiteral("Takes effect after you sign out or restart File Explorer."),
            };
            t.concern = QStringLiteral(
                "Windows 11 only; no effect on Windows 10.");
            CommandOp op;
            op.program = QStringLiteral("powershell");
            op.applyArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"),
                             QStringLiteral("-Command"),
                             QStringLiteral("$ErrorActionPreference='SilentlyContinue';"
                             "New-Item -Path 'HKCU:\\Software\\Classes\\CLSID\\{86ca1aa0-34aa-4e8b-a509-50c905bae2a2}\\InprocServer32' "
                             "-Force -Value '' | Out-Null") };
            op.revertArgs = { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"),
                              QStringLiteral("-Command"),
                              QStringLiteral("$ErrorActionPreference='SilentlyContinue';"
                              "Remove-Item -Path 'HKCU:\\Software\\Classes\\CLSID\\{86ca1aa0-34aa-4e8b-a509-50c905bae2a2}' "
                              "-Recurse -Force") };
            t.commandOps = { op };
            c.push_back(t);
        }
    }
}

namespace vth::tweaks::detail
{
namespace
{
}
}
