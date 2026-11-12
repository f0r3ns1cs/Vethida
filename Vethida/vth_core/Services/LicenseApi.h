#pragma once

#include "../Models/ProductTier.h"

#include <QtCore/QString>

// Vethida's licensing API. Stripe sells the plans, and the API issues the keys and
// answers activate, validate and deactivate.
namespace vth::licensing
{
    inline constexpr const char* kApiBase = "https://api.vethida.com/v1/licenses/";

    // Every answer's meta.product. A key from anywhere else is refused.
    inline constexpr const char* kProduct = "vethida";

    // No offline grace: no connection = Free. A server error keeps the plan.
    inline constexpr int    kRecheckIntervalMs = 60 * 60 * 1000;
    inline constexpr qint64 kStaleAfterMs      = 10LL * 60 * 1000;
    inline constexpr int    kOfflineRetryMs    = 60 * 1000;

    inline vth::models::ProductTier tierFromName(const QString& tier)
    {
        if (tier == QLatin1String("ultimate"))
            return vth::models::ProductTier::Ultimate;
        if (tier == QLatin1String("pro"))
            return vth::models::ProductTier::Pro;
        return vth::models::ProductTier::None;
    }
}
