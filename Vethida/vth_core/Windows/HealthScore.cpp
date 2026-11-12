#include "HealthScore.h"

#include "../SystemInfo.h"
#include "../Services/RegistryEditor.h"
#include "../Tweaks/TweakEngine.h"

#include <QHash>
#include <QObject>

#include <algorithm>
#include <cmath>

namespace vth::win::health
{
    namespace
    {
        // Every tweak whose state is cheap to read counts toward the subsystem it
        // changes, weighted by impact.
        double weightForImpact(tweaks::Impact impact)
        {
            switch (impact) {
            case tweaks::Impact::High:   return 0.16;
            case tweaks::Impact::Medium: return 0.10;
            default:                     return 0.05;
            }
        }

        // Unmapped categories fall back to "misc" rather than scoring nothing.
        const char* subsystemForCategory(const QString& category)
        {
            static const QHash<QString, const char*> kMap = {
                { QStringLiteral("gpu"),           "gpu" },
                { QStringLiteral("advanced"),      "cpu" },
                { QStringLiteral("debloating"),    "svc" },
                { QStringLiteral("browseroffice"), "svc" },
                { QStringLiteral("privacy"),       "bg" },
                { QStringLiteral("security"),      "bg" },
                { QStringLiteral("power"),         "pwr" },
                { QStringLiteral("networking"),    "net" },
                { QStringLiteral("storage"),       "sto" },
                { QStringLiteral("appearance"),    "ui" },
                { QStringLiteral("general"),       "ui" },
                { QStringLiteral("bios"),          "boot" },
            };
            return kMap.value(category, "misc");
        }

        // Per-subsystem caps, so many small tweaks can't max out the score. The
        // values are estimates.
        struct SubsystemCap { const char* key; double cap; };
        const SubsystemCap kCaps[] = {
            { "gpu",  2.2 },
            { "cpu",  1.7 },
            { "bg",   1.2 },
            { "pwr",  1.1 },
            { "net",  0.8 },
            { "svc",  0.7 },
            { "ui",   0.6 },
            { "sto",  0.6 },
            { "boot", 0.3 },
            { "misc", 0.3 },
        };

        // Later changes in a subsystem count for less. A subsystem missing from
        // kCaps scores nothing.
        double saturate(const QHash<QString, double>& summed)
        {
            double total = 0.0;
            for (const SubsystemCap& c : kCaps) {
                const double sum = summed.value(QLatin1String(c.key), 0.0);
                if (sum > 0.0)
                    total += c.cap * (1.0 - std::exp(-sum / c.cap));
            }
            return total;
        }

        // Same keys as hardwareVendorKey().
        QString vendorOfAdapter(const QString& gpuName)
        {
            const QString name = gpuName.toLower();
            if (name.contains(QLatin1String("nvidia"))
                || name.contains(QLatin1String("geforce"))
                || name.contains(QLatin1String("quadro"))) {
                return QStringLiteral("nvidia");
            }
            if (name.contains(QLatin1String("amd"))
                || name.contains(QLatin1String("radeon"))) {
                return QStringLiteral("amd");
            }
            return {};
        }

        void evaluateCatalog(const QString& machineVendor,
                             double& gainPercent, double& ceilingPercent)
        {
            vth::services::RegistryEditor reg;
            QHash<QString, double> applied;
            QHash<QString, double> available;

            for (const tweaks::TweakSpec& spec : tweaks::catalog()) {
                if (tweaks::isStateUndetectable(spec))
                    continue;
                if (!spec.dynamicOps.isEmpty())
                    continue;
                if (tweaks::needsUserConfirmation(spec))
                    continue;
                const QString vendor = tweaks::hardwareVendorKey(spec);
                if (!vendor.isEmpty() && !machineVendor.isEmpty()
                    && vendor != machineVendor) {
                    continue;
                }

                const QLatin1String sub(subsystemForCategory(spec.category));
                const double weight = weightForImpact(spec.impact);
                available[sub] += weight;
                if (tweaks::detect(spec, reg) == tweaks::DetectState::On)
                    applied[sub] += weight;
            }

            gainPercent    = saturate(applied);
            ceilingPercent = saturate(available);
        }
    }

    QString scoreBand(int score)
    {
        if (score >= 90) return QObject::tr("Fully optimized");
        if (score >= 70) return QObject::tr("Well optimized");
        if (score >= 40) return QObject::tr("Partly optimized");
        if (score >= 15) return QObject::tr("Lightly optimized");
        return QObject::tr("Not optimized yet");
    }

    Report evaluate()
    {
        Report r;

        const SystemInfo info = collect();
        r.gpuName  = info.gpuName.simplified();
        r.cpuName  = info.cpuBrand.simplified();
        r.ramBytes = info.ramTotalBytes;

        r.osName = info.osBuild.isEmpty()
            ? info.osEdition
            : QStringLiteral("%1 (%2)").arg(info.osEdition, info.osBuild);

        double gain = 0.0, ceiling = 0.0;
        evaluateCatalog(vendorOfAdapter(r.gpuName), gain, ceiling);

        r.score = ceiling > 0.0
            ? std::clamp(static_cast<int>(100.0 * gain / ceiling + 0.5), 0, 100)
            : 0;

        return r;
    }
}
