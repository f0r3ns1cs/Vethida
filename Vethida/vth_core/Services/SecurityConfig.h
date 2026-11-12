#pragma once

#include <QtCore/QString>

namespace vth::services
{
    class SecurityConfig
    {
    public:
        // Crash reports do nothing until this is a real project DSN.
        static constexpr const char* SentryDsn = "SENTRY-DSN-PLACEHOLDER";
        static bool sentryDsnIsPopulated();
    };
}
