#include "SessionProjection.h"

#include <QDateTime>
#include <QObject>

namespace vth::services
{
    namespace
    {
        int daysUntil(const QString& iso8601, qint64 nowUtcMs) {
            if (iso8601.isEmpty())
                return 0;
            QDateTime expiry = QDateTime::fromString(iso8601, Qt::ISODate);
            if (!expiry.isValid())
                expiry = QDateTime::fromString(iso8601, Qt::ISODateWithMs);
            if (!expiry.isValid())
                return 0;
            expiry.setTimeSpec(Qt::UTC);
            const qint64 deltaMs = expiry.toMSecsSinceEpoch() - nowUtcMs;
            // Round up so the last partial day still reads as "1 day left".
            return static_cast<int>((deltaMs + 86399999LL) / 86400000LL);
        }
    }

    SessionSnapshot sessionSnapshotFor(const vth::models::LicenseInfo& info, qint64 nowUtcMs) {
        SessionSnapshot s;
        s.status        = info.status;
        // Read from the flag, because a trial is an ordinary Ultimate license.
        s.trial         = info.trial;
        // a trial is Ultimate, only the wording differs
        s.tier          = vth::models::tierFromString(info.tier);
        s.daysRemaining = daysUntil(info.expiresAt, nowUtcMs);
        return s;
    }

    RibbonBadge describeRibbonBadge(const SessionSnapshot& s) {
        using St = vth::models::LicenseStatus;
        using T  = vth::models::ProductTier;

        RibbonBadge b;
        const bool lapsed = (s.status == St::Expired
                          || s.status == St::Revoked
                          || s.status == St::Invalid);

        if (s.trial && !lapsed) {
            b.visible    = true;
            b.urgent     = true;
            b.iconName   = QStringLiteral("clock");
            if (s.daysRemaining <= 0)
                b.text = QObject::tr("Trial - expires today");
            else if (s.daysRemaining == 1)
                b.text = QObject::tr("Trial - 1 day left");
            else
                b.text = QObject::tr("Trial - %1 days left").arg(s.daysRemaining);
            return b;
        }

        if (lapsed) {
            b.visible    = true;
            b.urgent     = true;
            b.iconName   = QStringLiteral("triangle-alert");
            b.text = (s.status == St::Expired)
                ? QObject::tr("License expired")
                : QObject::tr("License issue");
            return b;
        }

        b.visible = true;
        b.urgent  = false;
        switch (s.tier) {
        case T::Ultimate:
            b.text = QObject::tr("Ultimate");
            b.iconName = QStringLiteral("rocket");   // the "cool" top-tier glyph
            break;
        case T::Pro:
            b.text = QObject::tr("Pro");
            b.iconName = QStringLiteral("circle-check-filled");   // a settled, current tier
            break;
        case T::Free:
        default:
            // The tier the reader is on, stated the same way the other two are.
            b.text = QObject::tr("Free");
            b.iconName = QStringLiteral("circle-check-filled");
            break;
        }
        return b;
    }
}
