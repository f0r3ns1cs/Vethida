#pragma once

#include "../Models/LicenseModels.h"
#include "../Models/ProductTier.h"

#include <QString>

namespace vth::services
{
    // What the title bar shows for the license.
    struct SessionSnapshot {
        vth::models::ProductTier   tier   = vth::models::ProductTier::None;
        vth::models::LicenseStatus status = vth::models::LicenseStatus::Unverified;
        int     daysRemaining = 0;   // <= 0 means lapsed
        bool    offline = false;
    };

    struct RibbonBadge {
        bool    visible    = false;
        QString text;
        QString iconName;            // empty means no glyph
        bool    urgent     = false;
    };

    SessionSnapshot sessionSnapshotFor(const vth::models::LicenseInfo& info, qint64 nowUtcMs);
    RibbonBadge describeRibbonBadge(const SessionSnapshot& snap);
}
