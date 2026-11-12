#pragma once

#include "../Models/ProductTier.h"

namespace vth
{
    // True only under VETHIDA_DEV_BUILD: every check reports the top tier.
    bool devUnlockAll();

#ifdef VETHIDA_DEV_BUILD
    void setDevUnlockSuppressed(bool suppressed);
#endif

    // Always true without licensing (BuildConfig.h). With it, both false until the
    // licensing API has confirmed a key.
    bool isPro();
    bool isUltimate();

    // Free when there is no valid license.
    vth::models::ProductTier currentTier();

    bool meetsTier(vth::models::ProductTier need);
}
