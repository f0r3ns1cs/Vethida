#include "TweakEngine.h"


namespace vth::tweaks::detail {

    void appendDocsExtras(QVector<TweakSpec>& c) {
        {
            TweakSpec t;
            t.id = QStringLiteral("update.stop-driver-updates");
            t.category = QStringLiteral("debloating");
            t.subCategory = QStringLiteral("Driver updates");
            t.name = QStringLiteral("Stop automatic driver updates");
            t.description = QStringLiteral(
                "Stops Windows Update from delivering and silently replacing hardware drivers, "
                "and blocks device-metadata downloads - so a cumulative update can't overwrite "
                "a curated GPU/chipset driver.");
            t.iconName = QStringLiteral("refresh-cw-off");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "you become responsible for driver updates (Vethida's GPU automation covers the "
                "GPU). No security downside - this only affects drivers, not OS patches.");
            t.registryOps = {
                dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate"),
                        QStringLiteral("ExcludeWUDriversInQualityUpdate"), 1, std::nullopt),
                dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DriverSearching"),
                        QStringLiteral("SearchOrderConfig"), 0, 1),
                dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\DriverSearching"),
                        QStringLiteral("DontSearchWindowsUpdate"), 1, std::nullopt),
                dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\DriverSearching"),
                        QStringLiteral("DriverUpdateWizardWuSearchEnabled"), 0, std::nullopt),
                dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Device Metadata"),
                        QStringLiteral("PreventDeviceMetadataFromNetwork"), 1, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("game.disable-fso");
            t.category = QStringLiteral("general");
            t.subCategory = QStringLiteral("Fullscreen optimizations");
            t.name = QStringLiteral("Disable fullscreen optimizations");
            t.description = QStringLiteral("Requests true fullscreen-exclusive instead of Windows' fullscreen-optimization (borderless flip) path, via the global GameConfigStore FSE behavior flags. This is a hint the game and GPU driver may honor or ignore, and modern flip-model FSO is often equal or better - measure per game. A per-title compatibility-tab override is more precise. Per-user only.");
            t.iconName = QStringLiteral("monitor");
            t.impact = Impact::Low;
            t.concern = QStringLiteral(
                "FSO vs FSE is workload-dependent; modern flip-model FSO is often equal or "
                "better. A per-game compatibility-tab override is more precise than this global "
                "switch. Try it, measure, keep if it helps.");
            const QString gcs = QStringLiteral("HKCU\\System\\GameConfigStore");
            t.registryOps = {
                dwordOp(gcs, QStringLiteral("GameDVR_FSEBehaviorMode"), 2, 0),
                dwordOp(gcs, QStringLiteral("GameDVR_FSEBehavior"), 2, std::nullopt),
                dwordOp(gcs, QStringLiteral("GameDVR_HonorUserFSEBehaviorMode"), 1, 0),
                dwordOp(gcs, QStringLiteral("GameDVR_DXGIHonorFSEWindowsCompatible"), 1, 0),
                dwordOp(gcs, QStringLiteral("GameDVR_EFSEFeatureFlags"), 0, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("update.block-windows-update");
            t.category = QStringLiteral("advanced");
            t.name = QStringLiteral("Block Windows Update");
            t.description = QStringLiteral(
                "Disables automatic Windows Update entirely - no auto-download, no auto-install, "
                "no connection to the update service.");
            t.iconName = QStringLiteral("ban");
            t.impact = Impact::High;
            t.requiresAdmin = true;
            t.warnings = {
                QStringLiteral("You will stop receiving SECURITY updates. Only for isolated or "
                            "manually-maintained machines; re-enable to patch."),
            };
            t.concern = QStringLiteral(
                "risky: blanket WU block means no security patches. Prefer pausing updates in "
                "Settings for short windows; this is the permanent hammer.");
            const QString wu = QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate");
            const QString au = wu + QStringLiteral("\\AU");
            t.registryOps = {
                dwordOp(au, QStringLiteral("NoAutoUpdate"), 1, 0),
                dwordOp(au, QStringLiteral("AUOptions"), 1, std::nullopt),
                dwordOp(wu, QStringLiteral("DoNotConnectToWindowsUpdateInternetLocations"), 1, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("update.pause-updates-long");
            t.category = QStringLiteral("debloating");
            t.name = QStringLiteral("Pause Windows updates");
            // The pause expiry below is an implementation detail of how long
            // "paused" lasts; a customer-facing description that prints the
            // literal year is stating the mechanism instead of the effect.
            t.description = QStringLiteral(
                "Defers feature, quality and driver updates without disabling the update "
                "service. Security updates still arrive.");
            t.iconName = QStringLiteral("pause");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "softer than Block Windows Update - the service stays alive and you can still "
                "check manually. You still won't get automatic security patches while paused.");
            const QString ux = QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\WindowsUpdate\\UX\\Settings");
            const QString start = QStringLiteral("2025-01-01T00:00:00Z");
            const QString end = QStringLiteral("2051-12-31T00:00:00Z");
            t.registryOps = {
                stringOp(ux, QStringLiteral("PauseFeatureUpdatesStartTime"), start, std::nullopt),
                stringOp(ux, QStringLiteral("PauseFeatureUpdatesEndTime"), end, std::nullopt),
                stringOp(ux, QStringLiteral("PauseQualityUpdatesStartTime"), start, std::nullopt),
                stringOp(ux, QStringLiteral("PauseQualityUpdatesEndTime"), end, std::nullopt),
                stringOp(ux, QStringLiteral("PauseUpdatesStartTime"), start, std::nullopt),
                stringOp(ux, QStringLiteral("PauseUpdatesExpiryTime"), end, std::nullopt),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("update.au-policy-suite");
            t.category = QStringLiteral("debloating");
            // "AU Policy Suite" is the registry area's name, not the setting's.
            t.name = QStringLiteral("Ask before installing updates");
            t.description = QStringLiteral(
                "Windows tells you an update is ready instead of installing it on its own, "
                "and stops restarting your PC while you are signed in. Driver updates and "
                "Windows version upgrades are held back, and the Store stops updating apps "
                "by itself. Security updates still arrive - you choose when they install.");
            t.iconName = QStringLiteral("refresh-cw-off");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "overlaps with update.block-windows-update and update.pause-updates-long; "
                "AUOptions=2 (notify before download) is softer than that tweak's full block.");
            const QString wu = QStringLiteral("HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate");
            const QString au = wu + QStringLiteral("\\AU");
            t.registryOps = {
                dwordOp(au, QStringLiteral("AUOptions"), 2, std::nullopt),
                dwordOp(au, QStringLiteral("AutoInstallMinorUpdates"), 0, std::nullopt),
                dwordOp(au, QStringLiteral("NoAutoUpdate"), 1, std::nullopt),
                dwordOp(au, QStringLiteral("NoAutoRebootWithLoggedOnUsers"), 1, std::nullopt),
                dwordOp(wu, QStringLiteral("DoNotConnectToWindowsUpdateInternetLocations"), 1, std::nullopt),
                dwordOp(wu, QStringLiteral("SetDisableUXWUAccess"), 1, std::nullopt),
                dwordOp(wu, QStringLiteral("ExcludeWUDriversInQualityUpdate"), 1, std::nullopt),
                dwordOp(wu, QStringLiteral("DeferUpgrade"), 1, std::nullopt),
                dwordOp(wu, QStringLiteral("DeferUpdatePeriod"), 0, std::nullopt),
                dwordOp(QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WindowsStore\\WindowsUpdate"),
                        QStringLiteral("AutoDownload"), 2, std::nullopt),
            };
            c.push_back(t);
        }
    }
}
