#include "SecurityConfig.h"

namespace vth::services
{
    bool SecurityConfig::sentryDsnIsPopulated()
    {
        return QString::fromLatin1(SentryDsn).startsWith(QStringLiteral("https://"));
    }
}

namespace vth::services
{
namespace
{
        bool kidIsPlaceholder(const char* kid)
        {
            return QString::fromLatin1(kid)
                .contains(QStringLiteral("PLACEHOLDER"), Qt::CaseInsensitive);
        }
}
}
