#include "SecurityConfig.h"

namespace vth::services
{
    namespace
    {
        bool isAllZero(const QByteArray& b)
        {
            for (char x : b)
                if (x != 0)
                    return false;
            return true;
        }

        bool kidIsPlaceholder(const char* kid)
        {
            return QString::fromLatin1(kid)
                .contains(QStringLiteral("PLACEHOLDER"), Qt::CaseInsensitive);
        }
    }

    QByteArray SecurityConfig::licenseTokenPrimaryPublicKey()
    {
        // PLACEHOLDER, 64 zero bytes (raw X || Y), replaced for release builds
        return QByteArray(64, '\0');
    }

    QByteArray SecurityConfig::licenseTokenBackupPublicKey()
    {
        return QByteArray(64, '\0');
    }

    bool SecurityConfig::licenseTokenKeysArePopulated()
    {
        return !isAllZero(licenseTokenPrimaryPublicKey())
            && !isAllZero(licenseTokenBackupPublicKey())
            && !kidIsPlaceholder(LicenseTokenPrimaryKid)
            && !kidIsPlaceholder(LicenseTokenBackupKid);
    }

    QByteArray SecurityConfig::updateManifestPrimaryPublicKey()
    {
        return QByteArray(64, '\0');
    }

    QByteArray SecurityConfig::updateManifestBackupPublicKey()
    {
        return QByteArray(64, '\0');
    }

    bool SecurityConfig::updateManifestKeysArePopulated()
    {
        return !isAllZero(updateManifestPrimaryPublicKey())
            && !isAllZero(updateManifestBackupPublicKey())
            && !kidIsPlaceholder(UpdateManifestPrimaryKid)
            && !kidIsPlaceholder(UpdateManifestBackupKid);
    }

    bool SecurityConfig::updatePublisherIsPinned()
    {
        return !kidIsPlaceholder(UpdatePublisherCommonName)
            && QString::fromLatin1(UpdatePublisherCommonName).trimmed().length() > 0;
    }

    bool SecurityConfig::sentryDsnIsPopulated()
    {
        return !kidIsPlaceholder(SentryDsn) && QString::fromLatin1(SentryDsn).startsWith(
            QStringLiteral("https://"));
    }

    QStringList SecurityConfig::allowedOrigins()
    {
        return {QStringLiteral("https://api.vethida.com")};
    }

    QStringList SecurityConfig::apiSpkiPins()
    {
        return {
            QStringLiteral("0000000000000000PLACEHOLDER00000000000000000000000000000000primary"),
            QStringLiteral("0000000000000000PLACEHOLDER0000000000000000000000000000000000backup"),
        };
    }

    bool SecurityConfig::apiCertPinsArePopulated()
    {
        const auto pins = apiSpkiPins();
        if (pins.isEmpty())
            return false;
        for (const QString& pin : pins) {
            if (pin.length() != 64)
                return false;
            if (pin.contains(QStringLiteral("PLACEHOLDER"), Qt::CaseInsensitive))
                return false;
            for (QChar c : pin) {
                const bool hex = (c >= u'0' && c <= u'9')
                              || (c >= u'a' && c <= u'f')
                              || (c >= u'A' && c <= u'F');
                if (!hex)
                    return false;
            }
        }
        return true;
    }
}
