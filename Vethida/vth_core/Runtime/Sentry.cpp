#include "Sentry.h"

#include "../../version.h"
#include "../Logger.h"
#include "../Services/SecurityConfig.h"
#include "../Services/SentryBridge.h"
#include "../SystemInfo.h"
#include "Dispatcher.h"
#include "Paths.h"
#include "ServiceRegistry.h"
#include "Settings.h"
#include "Threading.h"

namespace vth::sentry
{
    namespace
    {
        VTH_DEFINE_LOG_CATEGORY(catSentry, "vth.sentry")

        constexpr const char* kCrashOptInKey = "telemetry/crashOptIn";

        // Coarse hardware class: core-count and memory buckets and the GPU
        // vendor. Never a model, serial or hardware id.
        QString coarseHardwareClass() {
            const vth::win::SystemInfo sys = vth::win::collect();

            QString cores;
            if      (sys.cpuLogicalProcs <= 4)  cores = QStringLiteral("<=4c");
            else if (sys.cpuLogicalProcs <= 8)  cores = QStringLiteral("6-8c");
            else if (sys.cpuLogicalProcs <= 16) cores = QStringLiteral("12-16c");
            else                                 cores = QStringLiteral("16c+");

            const double gib = static_cast<double>(sys.ramTotalBytes) / (1024.0 * 1024.0 * 1024.0);
            QString ram;
            if      (gib <= 8.5)  ram = QStringLiteral("8GB");
            else if (gib <= 16.5) ram = QStringLiteral("16GB");
            else if (gib <= 32.5) ram = QStringLiteral("32GB");
            else                   ram = QStringLiteral("32GB+");

            QString gpu = QStringLiteral("other");
            const QString name = sys.gpuName.toLower();
            if      (name.contains(QStringLiteral("nvidia"))) gpu = QStringLiteral("nvidia");
            else if (name.contains(QStringLiteral("amd")) || name.contains(QStringLiteral("radeon")))
                gpu = QStringLiteral("amd");
            else if (name.contains(QStringLiteral("intel"))) gpu = QStringLiteral("intel");

            return cores + QLatin1Char('/') + ram + QLatin1Char('/') + gpu;
        }

        // Starts the SDK on a worker: the DLL load, sentry_init and the
        // hardware probe stay off the UI thread. Safe to call repeatedly.
        void startTransport() {
            if (!vth::services::SecurityConfig::sentryDsnIsPopulated())
                return;
            Dispatcher::instance().dispatch([]() {
                const QString dbPath = vth::paths::protectedData() + QStringLiteral("/sentry-db");
                vth::paths::ensureDir(dbPath);
                const bool ok = vth::services::SentryBridge::initialize(
                    QString::fromLatin1(vth::services::SecurityConfig::SentryDsn),
                    QStringLiteral(VETHIDA_VERSION_STRING),
                    QStringLiteral("production"),
                    dbPath,
                    /*debug=*/false);
                if (ok) {
                    vth::services::SentryBridge::setTag(
                        QStringLiteral("hw_class"), coarseHardwareClass());
                }
            });
        }
    }

    Sentry::Sentry() = default;

    Sentry::~Sentry() {
        vth::services::SentryBridge::shutdown();
    }

    void Sentry::initialize() {
        VTH_ASSERT_UI_THREAD();
        if (auto* settings = ServiceRegistry::instance().get<Settings>())
            crashEnabled_ = settings->value(QString::fromLatin1(kCrashOptInKey), false).toBool();
        qCInfo(catSentry) << "crash reports" << (crashEnabled_ ? "on" : "off");
        if (crashEnabled_)
            startTransport();
    }

    void Sentry::setCrashEnabled(bool on) {
        VTH_ASSERT_UI_THREAD();
        if (auto* settings = ServiceRegistry::instance().get<Settings>()) {
            settings->setValue(QString::fromLatin1(kCrashOptInKey), on);
            settings->sync();
        }
        if (crashEnabled_ == on)
            return;
        crashEnabled_ = on;
        qCInfo(catSentry) << "crash reports turned" << (on ? "on" : "off");
        if (on) {
            startTransport();
        } else {
            // Stops the crash handler now, not at the next launch.
            Dispatcher::instance().dispatch([]() {
                vth::services::SentryBridge::shutdown();
            });
        }
    }
}
