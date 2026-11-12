#pragma once

// QLatin1StringView, because the build defines QT_NO_CAST_FROM_ASCII.
#include <QLatin1StringView>

namespace vth::site
{
    inline constexpr QLatin1StringView kHome{"https://vethida.com"};

    inline constexpr QLatin1StringView kPricing{"https://vethida.com/pricing"};
    inline constexpr QLatin1StringView kSupport{"https://vethida.com/support"};
    inline constexpr QLatin1StringView kFaq{"https://vethida.com/faq"};
    inline constexpr QLatin1StringView kChangelog{"https://vethida.com/changelog"};

    inline constexpr QLatin1StringView kEula{"https://vethida.com/legal/eula"};
    inline constexpr QLatin1StringView kPrivacy{"https://vethida.com/legal/privacy"};
    inline constexpr QLatin1StringView kThirdParty{"https://vethida.com/legal/third-party"};

    // Read at start and once a day. The update feed is in Runtime/AutoUpdate.h.
    inline constexpr QLatin1StringView kWithdrawnTweaks{
        "https://vethida.com/desktop/withdrawn-tweaks.json"};

    inline constexpr QLatin1StringView kPortalLicenses{
        "https://api.vethida.com/licenses/lost"};
    inline constexpr QLatin1StringView kPortalBilling{
        "https://vethida.com/billing"};

    inline constexpr QLatin1StringView kDiscord{"https://discord.gg/CjnYVU9cFb"};
}
