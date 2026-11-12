#include "Consent.h"

#include "Sentry.h"
#include "ServiceRegistry.h"
#include "Settings.h"

#include <QStringLiteral>

namespace vth::consent
{
    namespace
    {
        constexpr const char* kPrefDecided = "consent/decided";
        constexpr const char* kPrefVersion = "consent/version";

        // bump to ask everyone again. 3 = crash reports only (usage data removed)
        constexpr int kConsentVersion = 3;
    }

    bool decided()
    {
        auto* settings = ServiceRegistry::instance().get<Settings>();
        if (!settings) return false;
        if (!settings->value(QString::fromLatin1(kPrefDecided), false).toBool())
            return false;
        return settings->value(QString::fromLatin1(kPrefVersion), 0).toInt()
            >= kConsentVersion;
    }

    void record(bool crashOptIn)
    {
        auto* settings = ServiceRegistry::instance().get<Settings>();
        if (!settings) return;
        settings->setValue(QString::fromLatin1(kPrefDecided), true);
        settings->setValue(QString::fromLatin1(kPrefVersion), kConsentVersion);
        // The old usage-data preference, which sent nothing of its own.
        settings->remove(QStringLiteral("telemetry/optIn"));
        settings->sync();

        if (auto* s = ServiceRegistry::instance().get<vth::sentry::Sentry>())
            s->setCrashEnabled(crashOptIn);
    }
}
