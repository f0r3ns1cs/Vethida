#pragma once

// Verified license token cache. Loaded from the DPAPI-sealed file at boot,
// updated by activation/heartbeat replies through update().

#include <QtCore/QDateTime>
#include <QtCore/QString>

namespace vth::license
{
    // valid=false = no entitlement
    struct LicenseInfo {
        bool       valid              = false;
        QString    keyHash;              // SHA-256 of the license key
        QString    tier;                 // pro | ultimate; empty when unlicensed
        QDateTime  issuedAt;             // iat claim
        QDateTime  expiresAt;            // exp claim
        QString    activatedAt;          // ISO 8601
        QString    devicePubkeyId;

        // Cache-derived (not JWS claims).
        qint64     lastVerifiedUtcMs   = 0;
        qint64     gracePeriodEndUtcMs = 0;

        // Raw verified JWS that produced this snapshot. Empty when invalid.
        QString    rawJws;
    };

    // Tokens for any other devicePubkeyId are rejected. Call once at boot before
    // populateOnBoot(), until then everything is rejected. Stored once because
    // verify runs on worker threads and ServiceRegistry isn't thread-safe.
    void bindToDevice(const QString& devicePubkeyId) noexcept;

    // Thread-safe. Expiry is checked on every call, not just at boot.
    LicenseInfo cache() noexcept;

    // cache().rawJws when valid and unexpired, empty otherwise.
    QString cachedToken() noexcept;

    // load + verify the sealed token, failure leaves it empty
    void populateOnBoot() noexcept;

    // verify, replace and re-seal
    bool update(const QString& jwsToken) noexcept;

    // clear cache and delete the file
    void clear() noexcept;

    // True iff the cached tier is pro or ultimate.
    bool isPro() noexcept;

    // True iff the cached tier is ultimate.
    bool isUltimate() noexcept;
}
