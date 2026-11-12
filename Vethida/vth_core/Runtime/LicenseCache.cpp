#include "LicenseCache.h"

#include <atomic>

namespace vth::license
{
    namespace
    {
        // 0 none, 1 pro, 2 ultimate.
        std::atomic<int> g_tier{ 0 };
    }

    void setTier(const QString& tier) noexcept
    {
        const QString t = tier.trimmed().toLower();
        g_tier.store(t == QLatin1String("ultimate") ? 2
                   : t == QLatin1String("pro")      ? 1
                                                    : 0);
    }

    void clear() noexcept
    {
        g_tier.store(0);
    }

    bool isPro() noexcept
    {
        return g_tier.load() >= 1;
    }

    bool isUltimate() noexcept
    {
        return g_tier.load() >= 2;
    }
}
