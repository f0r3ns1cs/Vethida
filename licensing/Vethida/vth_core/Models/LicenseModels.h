#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <optional>
#include <utility>

namespace vth::models
{

    enum class LicenseStatus {
        Unverified,
        Valid,
        Expired,
        Revoked,
        Invalid,
        GracePeriod,
    };

    struct LicenseInfo {
        QString key;
        // Empty means no license, which isn't the same as a tier.
        QString tier;
        LicenseStatus status = LicenseStatus::Unverified;

        bool trial = false; // tier is still "ultimate", this only changes wording

        QString activatedAt;
        // ISO-8601 UTC, when the license ends (not the token's exp)
        QString expiresAt;

        qint64 lastVerifiedUtcMs = 0;
        std::optional<qint64> gracePeriodEndUtcMs;

        QString devicePubkeyId; // hex of SHA-256(pubkey)[0..16]

        QString fingerprintStatus; // "accept" or "flag" (reject comes back as an error)

        int resetsRemaining = 0; // at 0 they need support

        // pending account deletion, from the heartbeat only
        bool    accountDeleting = false;
        // ISO-8601 UTC. Empty unless accountDeleting.
        QString accountHardDeleteAt;

        QString accountEmail; // browser sign-in only, don't gate anything on it
    };

    // Why activate/heartbeat/deactivate failed. Several wire codes map to one value,
    // the raw code is kept in LicenseOpResult::code.
    enum class LicenseOpError {
        None,
        // Never reached the server: DNS, transport, or the request never queued.
        Network,
        RateLimited,
        // Rejected locally before any request went out.
        KeyFormat,
        KeyUnknown,
        KeyRevoked,
        KeyExpired,
        AccountSuspended,
        MachineLimitReached, // bound to another PC
        // A move is possible but the license has no machine resets left.
        NoResetsRemaining,
        // The hardware no longer matches what this installation registered.
        FingerprintMismatch,
        // The device signature was refused: a wiped or rotated device keypair.
        DeviceAuth,
        ClockSkew, // system clock is off (dead CMOS battery, VM snapshot)
        DeviceDeactivated, // released from the portal, just activate again
        SignInDeclined,
        SignInExpired, // 10 min window passed, or already used
        // Browser sign-in: the account holds no license that can be activated.
        SignInNoLicense,
        TokenRejected, // 2xx but the JWS didn't verify
        Server,
        Unknown,
    };

    struct LicenseOpResult {
        bool           ok    = false;
        LicenseOpError error = LicenseOpError::None;
        QString        code; // raw server code for logs, switch on `error` instead

        static LicenseOpResult success() { return { true, LicenseOpError::None, {} }; }
        static LicenseOpResult failure(LicenseOpError e, QString wireCode = {})
        {
            return { false, e, std::move(wireCode) };
        }

        explicit operator bool() const { return ok; }
    };

    LicenseOpError licenseErrorFromCode(const QString& code);

    // user-facing sentence, no button/page names so it works in both places
    QString licenseErrorLabel(LicenseOpError error);

    class LicenseValidation {
    public:
        // ABCDE-12345-FGHIJ-67890-KLMNO
        static bool isValidKeyFormat(const QString& key);

        // Strips dashes, uppercases, and re-groups every five characters.
        static QString formatKey(QString raw);

        // Leaves the first group visible and masks the rest for display.
        static QString maskKey(const QString& key);
    };

}

Q_DECLARE_METATYPE(vth::models::LicenseStatus)
// So the type can ride a queued connection or a QVariant without registering it first.
Q_DECLARE_METATYPE(vth::models::LicenseOpResult)
