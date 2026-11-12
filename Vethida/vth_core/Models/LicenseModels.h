#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <utility>

// The license as this PC holds it. The licensing API is the authority.
namespace vth::models
{
    enum class LicenseStatus {
        Unverified,   // no key, or never confirmed by the licensing API
        Valid,        // the licensing API confirmed it
        Expired,      // the subscription behind it ended
        Revoked,      // disabled, usually after a refund
    };

    struct LicenseInfo {
        QString key;
        QString instanceId;       // this PC's activation
        QString tier;             // "pro", "ultimate", or empty
        LicenseStatus status = LicenseStatus::Unverified;

        QString activatedAt;      // ISO-8601 UTC
        QString expiresAt;        // ISO-8601 UTC, empty for a key that does not expire
        qint64  lastVerifiedUtcMs = 0;

        QString customerEmail;
        QString productName;
        int     activationLimit = 0;   // 0 means unlimited
        int     activationUsage = 0;
    };

    enum class LicenseOpError {
        None,
        Network,              // never reached the licensing API
        RateLimited,
        KeyFormat,            // rejected before any request
        KeyUnknown,           // no such key
        WrongProduct,         // a real key, but not for Vethida
        KeyRevoked,
        KeyExpired,
        MachineLimitReached,  // every activation on the key is in use
        Server,
        Unknown,
    };

    struct LicenseOpResult {
        bool           ok    = false;
        LicenseOpError error = LicenseOpError::None;
        QString        detail;   // the API's own error text, for the log

        static LicenseOpResult success() { return { true, LicenseOpError::None, {} }; }
        static LicenseOpResult failure(LicenseOpError e, QString detail = {})
        {
            return { false, e, std::move(detail) };
        }

        explicit operator bool() const { return ok; }
    };

    QString licenseErrorLabel(LicenseOpError error);

    class LicenseValidation {
    public:
        // Five groups of five Crockford base32 characters, each group ending in
        // a Luhn mod 32 check character.
        static bool isValidKeyFormat(const QString& key);

        // Normalizes case, stray characters, look-alikes and hyphens.
        static QString formatKey(const QString& raw);

        // First group visible, the rest masked.
        static QString maskKey(const QString& key);
    };
}

Q_DECLARE_METATYPE(vth::models::LicenseStatus)
Q_DECLARE_METATYPE(vth::models::LicenseOpResult)
