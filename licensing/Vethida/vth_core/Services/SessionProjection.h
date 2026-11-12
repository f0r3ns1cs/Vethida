#pragma once

#include "../Models/LicenseModels.h"
#include "../Models/ProductTier.h"

#include <QString>

namespace vth::services
{
    // license state for the UI (titlebar badge)
    struct SessionSnapshot {
        vth::models::ProductTier   tier   = vth::models::ProductTier::None;
        vth::models::LicenseStatus status = vth::models::LicenseStatus::Unverified;
        bool    trial         = false;
        int     daysRemaining = 0;   // until trial/subscription lapse (<= 0 = lapsed)
    };


    struct RibbonBadge {
        bool    visible    = false;
        QString text;
        QString iconName;            // empty = no glyph
        bool    urgent     = false;  // amber (trial/lapsed) vs neutral tier chip
    };

    // Both are pure functions of their input.
    SessionSnapshot sessionSnapshotFor(const vth::models::LicenseInfo& info, qint64 nowUtcMs);
    RibbonBadge describeRibbonBadge(const SessionSnapshot& snap);
}
