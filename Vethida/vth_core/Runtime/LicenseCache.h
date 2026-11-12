#pragma once

#include <QtCore/QString>

// Current tier, set by LicenseService, read from any thread by Tier.cpp.
namespace vth::license
{
    // "pro", "ultimate", or empty for no paid tier.
    void setTier(const QString& tier) noexcept;
    void clear() noexcept;

    bool isPro() noexcept;        // pro or ultimate
    bool isUltimate() noexcept;
}
