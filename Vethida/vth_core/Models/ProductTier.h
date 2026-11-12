#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>

namespace vth::models
{

    // None means entitlement has not been resolved yet. It is not a tier anyone holds.
    enum class ProductTier {
        None,
        Free,
        Pro,
        Ultimate,
    };

    inline int tierRank(ProductTier tier) {
        switch (tier) {
            case ProductTier::None:     return 0;
            case ProductTier::Free:     return 1;
            case ProductTier::Pro:      return 2;
            case ProductTier::Ultimate: return 3;
        }
        return 0;
    }

    inline ProductTier tierFromRank(int rank) {
        if (rank >= tierRank(ProductTier::Ultimate)) return ProductTier::Ultimate;
        if (rank >= tierRank(ProductTier::Pro))      return ProductTier::Pro;
        if (rank >= tierRank(ProductTier::Free))     return ProductTier::Free;
        return ProductTier::None;
    }

    inline QString tierLabel(ProductTier tier) {
        switch (tier) {
            case ProductTier::None:     return QStringLiteral("Not activated");
            case ProductTier::Free:     return QStringLiteral("Free");
            case ProductTier::Pro:      return QStringLiteral("Pro");
            case ProductTier::Ultimate: return QStringLiteral("Ultimate");
        }
        return QString();
    }

    // "Vethida Pro", "Vethida Ultimate".
    inline QString tierProductName(ProductTier tier) {
        if (tier == ProductTier::None)
            return QStringLiteral("Vethida");
        return QStringLiteral("Vethida %1").arg(tierLabel(tier));
    }

    // An unrecognized or empty string is None. currentTier() resolves that to Free.
    inline ProductTier tierFromString(const QString& raw) {
        const QString t = raw.trimmed().toLower();
        if (t == QLatin1String("ultimate"))
            return ProductTier::Ultimate;
        if (t == QLatin1String("pro"))
            return ProductTier::Pro;
        if (t == QLatin1String("free"))
            return ProductTier::Free;
        return ProductTier::None;
    }

}

Q_DECLARE_METATYPE(vth::models::ProductTier)
