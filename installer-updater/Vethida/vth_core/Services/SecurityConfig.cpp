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
}
