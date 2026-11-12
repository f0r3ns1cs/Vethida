#include "LicenseCache.h"

#include "../Logger.h"
#include "../Services/LicenseTokenCache.h"
#include "../Services/SecurityConfig.h"
#include "LicenseToken.h"

#include <QtCore/QByteArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QStringList>
#include <QtCore/QTimeZone>

#include <shared_mutex>

namespace vth::license
{
    namespace
    {
        VTH_DEFINE_LOG_CATEGORY(catLic, "vth.license.cache")

        // expected iss/aud (not from vth::site on purpose)
        constexpr const char* kExpectedIssuer = "https://vethida.com";
        constexpr const char* kExpectedAudience = "vethida.desktop";

        const QStringList& proTiers()
        {
            static const QStringList tiers = {
                QStringLiteral("pro"),
                QStringLiteral("ultimate"),
            };
            return tiers;
        }

        std::shared_mutex& mutex()
        {
            static std::shared_mutex m;
            return m;
        }

        LicenseInfo& storage()
        {
            static LicenseInfo info;
            return info;
        }

        // guarded by mutex(), empty until bindToDevice = reject everything
        QString& expectedDeviceId()
        {
            static QString id;
            return id;
        }

        // The cached token only counts if the last heartbeat was recent enough, so blocking
        // the API in the hosts file doesn't buy the whole token lifetime. 0 = fresh activation.
        bool withinOfflineCutoff()
        {
            const qint64 last =
                vth::services::LicenseTokenCache::loadLastHeartbeatUtcMs();
            if (last == 0)
                return true;
            const qint64 ageMs = QDateTime::currentMSecsSinceEpoch() - last;
            const qint64 capMs =
                static_cast<qint64>(vth::services::SecurityConfig::OfflineHardCutoffHours)
                * 60 * 60 * 1000;
            return ageMs <= capMs;
        }

        bool isLive(const LicenseInfo& info)
        {
            return info.valid
                && info.expiresAt.isValid()
                && info.expiresAt > QDateTime::currentDateTimeUtc()
                && withinOfflineCutoff();
        }

        QByteArray base64UrlDecode(const QString& value)
        {
            return QByteArray::fromBase64(
                value.toLatin1(),
                QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
        }

        QDateTime parseEpochSeconds(qint64 epochSec)
        {
            if (epochSec <= 0)
                return {};
            return QDateTime::fromSecsSinceEpoch(epochSec, QTimeZone::UTC);
        }

        LicenseInfo parseClaims(const QJsonObject& payload)
        {
            LicenseInfo info;
            info.keyHash = payload.value(QStringLiteral("keyHash")).toString();
            info.tier = payload.value(QStringLiteral("tier")).toString().toLower();
            info.activatedAt = payload.value(QStringLiteral("activatedAt")).toString();
            info.devicePubkeyId = payload.value(QStringLiteral("devicePubkeyId")).toString();

            if (const QJsonValue issued = payload.value(QStringLiteral("iat")); issued.isDouble())
                info.issuedAt = parseEpochSeconds(static_cast<qint64>(issued.toDouble()));
            if (const QJsonValue expires = payload.value(QStringLiteral("exp")); expires.isDouble())
                info.expiresAt = parseEpochSeconds(static_cast<qint64>(expires.toDouble()));

            return info;
        }

        LicenseInfo verifyAndParse(const QString& jwsToken)
        {
            LicenseInfo info;
            if (jwsToken.isEmpty())
                return info;

            const security::TokenStatus status = security::LicenseToken::verify(
                jwsToken,
                QString::fromLatin1(kExpectedIssuer),
                QString::fromLatin1(kExpectedAudience));

            if (status != security::TokenStatus::Valid) {
                qCInfo(catLic) << "verify rejected token; status" << static_cast<int>(status);
                return info;
            }

            const QStringList parts = jwsToken.split(QChar::fromLatin1('.'));
            if (parts.size() != 3) {
                qCWarning(catLic) << "verified token has malformed shape";
                return info;
            }

            QJsonParseError error{};
            const QJsonDocument doc = QJsonDocument::fromJson(base64UrlDecode(parts.at(1)), &error);
            if (error.error != QJsonParseError::NoError || !doc.isObject()) {
                qCWarning(catLic) << "verified token payload did not parse:" << error.errorString();
                return info;
            }

            info = parseClaims(doc.object());
            if (info.keyHash.isEmpty() || info.tier.isEmpty() || info.devicePubkeyId.isEmpty()) {
                qCWarning(catLic) << "verified token missing required claims";
                return {};
            }

            // exp is required: a token with no exp passes the verifier and never expires here.
            if (!info.expiresAt.isValid()) {
                qCWarning(catLic) << "verified token has no usable exp claim";
                return {};
            }

            // token has to be for this machine (case-insensitive, both are hex)
            const QString expected = [] {
                std::shared_lock lock(mutex());
                return expectedDeviceId();
            }();
            if (expected.isEmpty()) {
                qCWarning(catLic) << "no device binding set; refusing token";
                return {};
            }
            if (info.devicePubkeyId.compare(expected, Qt::CaseInsensitive) != 0) {
                qCWarning(catLic) << "token was issued to a different device; refusing";
                return {};
            }

            info.valid = true;
            info.rawJws = jwsToken;
            return info;
        }
    }

    void bindToDevice(const QString& devicePubkeyId) noexcept
    {
        std::unique_lock lock(mutex());
        expectedDeviceId() = devicePubkeyId;
    }

    LicenseInfo cache() noexcept
    {
        std::shared_lock lock(mutex());
        LicenseInfo snapshot = storage();
        // expired = cleared, so `valid` always means "entitled right now"
        if (!isLive(snapshot))
            snapshot.valid = false;
        return snapshot;
    }

    QString cachedToken() noexcept
    {
        std::shared_lock lock(mutex());
        return isLive(storage()) ? storage().rawJws : QString{};
    }

    void populateOnBoot() noexcept
    {
        const auto token = vth::services::LicenseTokenCache::load();
        if (!token.has_value() || token->isEmpty()) {
            qCInfo(catLic) << "no sealed license token; cache stays empty";
            return;
        }

        LicenseInfo fresh = verifyAndParse(*token);
        if (fresh.valid)
            fresh.lastVerifiedUtcMs = QDateTime::currentMSecsSinceEpoch();

        {
            std::unique_lock lock(mutex());
            storage() = fresh;
        }
    }

    bool update(const QString& jwsToken) noexcept
    {
        LicenseInfo fresh = verifyAndParse(jwsToken);
        if (!fresh.valid)
            return false;
        fresh.lastVerifiedUtcMs = QDateTime::currentMSecsSinceEpoch();

        {
            std::unique_lock lock(mutex());
            storage() = fresh;
        }

        vth::services::LicenseTokenCache::save(jwsToken);
        return true;
    }

    void clear() noexcept
    {
        {
            std::unique_lock lock(mutex());
            storage() = LicenseInfo{};
        }
        vth::services::LicenseTokenCache::deleteToken();
    }

    bool isPro() noexcept
    {
        std::shared_lock lock(mutex());
        // Case-insensitive, matching the Ultimate check in Tier.cpp.
        return isLive(storage()) && proTiers().contains(storage().tier, Qt::CaseInsensitive);
    }

    bool isUltimate() noexcept
    {
        std::shared_lock lock(mutex());
        return isLive(storage())
            && storage().tier.compare(QStringLiteral("ultimate"), Qt::CaseInsensitive) == 0;
    }
}
