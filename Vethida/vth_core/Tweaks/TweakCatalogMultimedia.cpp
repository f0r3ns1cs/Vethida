#include "TweakEngine.h"

// Multimedia / MMCSS: SystemProfile under Windows NT\CurrentVersion\Multimedia.
//
// SystemResponsiveness and NetworkThrottlingIndex already ship in the starter
// set. Per-task values (Tasks\Games, Tasks\Pro Audio) are NOT shipped: the Games
// task is already "High" by default, and GPU Priority, SFIO Priority and Clock
// Rate are Microsoft-non-functional. DWM MMCSS registration needs the DwmEnableMMCSS
// API, not registry.

namespace vth::tweaks::detail {

    namespace {
        const QString kSP = QStringLiteral(
            "HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile");
    }

    void appendMultimedia(QVector<TweakSpec>& c) {
        {
            TweakSpec t;
            t.id = QStringLiteral("mmcss.no-lazy-mode");
            t.category = QStringLiteral("advanced");
            t.subCategory = QStringLiteral("MMCSS");
            t.name = QStringLiteral("MMCSS no-lazy mode");
            t.description = QStringLiteral(
                "Stops the multimedia scheduler from scaling its priority boost back "
                "during idle, keeping real-time threads boosted continuously.");
            t.iconName = QStringLiteral("activity");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.warnings = { QStringLiteral("Increases idle CPU usage.") };
            t.registryOps = { dwordOp(kSP, QStringLiteral("NoLazyMode"), 1, std::nullopt) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("audio.disable-ducking");
            t.category = QStringLiteral("general");
            t.subCategory = QStringLiteral("Audio");
            t.name = QStringLiteral("Disable audio communications ducking");
            t.description = QStringLiteral(
                "Stops Windows lowering other apps' volume when it detects call/voice "
                "activity (sets the Communications option to \"Do nothing\").");
            t.iconName = QStringLiteral("volume-2");
            t.impact = Impact::Low;
            t.requiresAdmin = false;
            t.registryOps = {
                dwordOp(QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Multimedia\\Audio"),
                        QStringLiteral("UserDuckingPreference"), 3, 0),
            };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("mmcss.always-on");
            t.category = QStringLiteral("advanced");
            t.name = QStringLiteral("MMCSS always on");
            t.description = QStringLiteral("Forces the multimedia class scheduler to stay active.");
            t.iconName = QStringLiteral("activity");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "placebo: an undocumented value with no published measurement.");
            t.registryOps = { dwordOp(kSP, QStringLiteral("AlwaysOn"), 1, std::nullopt) };
            c.push_back(t);
        }

        {
            TweakSpec t;
            t.id = QStringLiteral("mmcss.scheduler-period");
            t.category = QStringLiteral("advanced");
            t.name = QStringLiteral("MMCSS scheduler period");
            t.description = QStringLiteral("Sets the multimedia scheduler period to its minimum.");
            t.iconName = QStringLiteral("activity");
            t.impact = Impact::Low;
            t.requiresAdmin = true;
            t.concern = QStringLiteral(
                "placebo: undocumented value with no published measurement.");
            t.registryOps = { dwordOp(kSP, QStringLiteral("SchedulerPeriod"), 1, std::nullopt) };
            c.push_back(t);
        }
    }
}
