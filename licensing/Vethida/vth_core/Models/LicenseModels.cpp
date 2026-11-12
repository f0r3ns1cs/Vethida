#include "LicenseModels.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QStringList>

namespace vth::models
{

    // Codes from api.vethida.com lib/errors.ts. Unknown codes map to Unknown.
    LicenseOpError licenseErrorFromCode(const QString& code)
    {
        if (code.isEmpty())                        return LicenseOpError::Unknown;

        if (code == QLatin1String("license_unknown"))       return LicenseOpError::KeyUnknown;
        if (code == QLatin1String("license_revoked"))       return LicenseOpError::KeyRevoked;
        if (code == QLatin1String("license_expired"))       return LicenseOpError::KeyExpired;
        if (code == QLatin1String("account_suspended"))     return LicenseOpError::AccountSuspended;
        if (code == QLatin1String("machine_limit_reached")) return LicenseOpError::MachineLimitReached;
        if (code == QLatin1String("no_resets_remaining"))   return LicenseOpError::NoResetsRemaining;
        if (code == QLatin1String("fingerprint_mismatch"))  return LicenseOpError::FingerprintMismatch;
        if (code == QLatin1String("invalid_license_key"))   return LicenseOpError::KeyFormat;
        if (code == QLatin1String("rate_limited"))          return LicenseOpError::RateLimited;

        // The transport codes ApiClient synthesises when nothing was reached.
        if (code == QLatin1String("network_error")
            || code == QLatin1String("network_unavailable")) return LicenseOpError::Network;

        // device auth. clock skew and deactivated get their own messages
        if (code == QLatin1String("device_auth_clock_skew")) return LicenseOpError::ClockSkew;
        if (code == QLatin1String("device_deactivated"))     return LicenseOpError::DeviceDeactivated;

        // browser sign-in. already_used counts as expired for the user
        if (code == QLatin1String("auth_denied"))          return LicenseOpError::SignInDeclined;
        if (code == QLatin1String("auth_session_expired")
            || code == QLatin1String("auth_already_used")) return LicenseOpError::SignInExpired;
        if (code == QLatin1String("invalid_device_code"))  return LicenseOpError::SignInExpired;
        if (code == QLatin1String("device_auth_malformed")
            || code == QLatin1String("device_auth_missing")
            || code == QLatin1String("device_not_found")
            || code == QLatin1String("device_signature_invalid")
            || code == QLatin1String("invalid_device_public_key")) {
            return LicenseOpError::DeviceAuth;
        }

        // bad requests are our bug, shown as a server error
        if (code == QLatin1String("internal_error")
            || code == QLatin1String("invalid_json")
            || code == QLatin1String("invalid_fingerprint")
            || code == QLatin1String("payload_too_large")
            || code.startsWith(QLatin1String("http_5"))) {
            return LicenseOpError::Server;
        }

        return LicenseOpError::Unknown;
    }

    QString licenseErrorLabel(LicenseOpError error)
    {
        switch (error) {
        case LicenseOpError::None:
            return {};
        case LicenseOpError::Network:
            return QObject::tr("Couldn't reach the licensing server. Check the "
                               "connection and try again.");
        case LicenseOpError::RateLimited:
            return QObject::tr("Too many attempts. Wait a few minutes and try again.");
        case LicenseOpError::KeyFormat:
            return QObject::tr("That key isn't in the right format. It is five "
                               "groups of five characters.");
        case LicenseOpError::KeyUnknown:
            return QObject::tr("No license matches that key. Check it for typos.");
        case LicenseOpError::KeyRevoked:
            return QObject::tr("This license was revoked, usually after a refund. "
                               "Contact support if that's unexpected.");
        case LicenseOpError::KeyExpired:
            return QObject::tr("This license has expired. Renewing it re-enables "
                               "activation.");
        case LicenseOpError::AccountSuspended:
            return QObject::tr("The account that owns this license is suspended. "
                               "Contact support.");
        case LicenseOpError::MachineLimitReached:
            return QObject::tr("This license is already active on another PC. "
                               "Deactivate it there, or from the machines list on "
                               "my.vethida.com, then try again.");
        case LicenseOpError::NoResetsRemaining:
            return QObject::tr("This license has no machine resets left. Contact "
                               "support to move it to another PC.");
        case LicenseOpError::FingerprintMismatch:
            return QObject::tr("This PC's hardware no longer matches what this "
                               "license was activated on. Moving it here uses a "
                               "machine reset.");
        case LicenseOpError::DeviceAuth:
            return QObject::tr("This installation's device identity was refused. "
                               "Deactivate the license on my.vethida.com, then "
                               "activate again here.");
        case LicenseOpError::ClockSkew:
            return QObject::tr("This PC's clock is too far out for the licensing "
                               "server to accept it. Correct the date and time, "
                               "then try again.");
        case LicenseOpError::DeviceDeactivated:
            return QObject::tr("This PC was deactivated on your account. Activate "
                               "the key again to use the license here.");
        case LicenseOpError::SignInDeclined:
            return QObject::tr("The sign-in request was declined in the browser. "
                               "Nothing was activated on this PC.");
        case LicenseOpError::SignInExpired:
            return QObject::tr("The sign-in request timed out. Start it again and "
                               "approve it in the browser within ten minutes.");
        case LicenseOpError::SignInNoLicense:
            return QObject::tr("That account has no license that can be activated. "
                               "Buy or claim one on my.vethida.com, then sign in "
                               "again.");
        case LicenseOpError::TokenRejected:
            return QObject::tr("The server's response didn't pass verification, so "
                               "it wasn't trusted. Try again, and contact support "
                               "if it keeps happening.");
        case LicenseOpError::Server:
            return QObject::tr("The licensing server had a problem. Try again in a "
                               "few minutes.");
        case LicenseOpError::Unknown:
            break;
        }
        return QObject::tr("Something went wrong. Try again, and contact support if "
                           "it keeps happening.");
    }

    namespace {
        const QRegularExpression& keyPattern()
        {
            static const QRegularExpression rx(
                QStringLiteral("^[A-Z0-9]{5}-[A-Z0-9]{5}-[A-Z0-9]{5}-[A-Z0-9]{5}-[A-Z0-9]{5}$"));
            return rx;
        }
    }

    bool LicenseValidation::isValidKeyFormat(const QString& key)
    {
        return keyPattern().match(key).hasMatch();
    }

    QString LicenseValidation::formatKey(QString raw)
    {
        raw = raw.remove(QLatin1Char('-')).toUpper();
        QStringList parts;
        for (int i = 0; i < raw.size(); i += 5) {
            parts << raw.mid(i, 5);
        }
        return parts.join(QLatin1Char('-'));
    }

    QString LicenseValidation::maskKey(const QString& key)
    {
        if (key.isEmpty() || key.size() < 5) {
            return key;
        }
        const QString prefix = key.left(5);
        return QStringLiteral("%1-XXXXX-XXXXX-XXXXX-XXXXX").arg(prefix);
    }

}
