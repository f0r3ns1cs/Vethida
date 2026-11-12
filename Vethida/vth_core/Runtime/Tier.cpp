#include "Tier.h"

#include "LicenseCache.h"

namespace vth
{
#ifdef VETHIDA_DEV_BUILD
    namespace { bool g_devUnlockSuppressed = false; }
    void setDevUnlockSuppressed(bool suppressed) { g_devUnlockSuppressed = suppressed; }
#endif

    bool devUnlockAll()
    {
#ifdef VETHIDA_DEV_BUILD
        return !g_devUnlockSuppressed;
#else
        return false;
#endif
    }

    bool isPro()
    {
        return devUnlockAll() || vth::license::isPro();
    }

    bool isUltimate()
    {
        return devUnlockAll() || vth::license::isUltimate();
    }

    vth::models::ProductTier currentTier()
    {
        if (isUltimate())
            return vth::models::ProductTier::Ultimate;
        if (isPro())
            return vth::models::ProductTier::Pro;
        return vth::models::ProductTier::Free;
    }

    bool meetsTier(vth::models::ProductTier need)
    {
        return vth::models::tierRank(currentTier())
            >= vth::models::tierRank(need);
    }
}
