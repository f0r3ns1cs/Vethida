#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QStringList>

namespace vth::services
{
    // Keys/pins are PLACEHOLDER until a release fills them in
    class SecurityConfig
    {
    public:
        static constexpr const char* UpdateManifestPrimaryKid = "vethida-update-v1-PLACEHOLDER";
        static constexpr const char* UpdateManifestBackupKid  = "vethida-update-v2-PLACEHOLDER";

        static QByteArray updateManifestPrimaryPublicKey();
        static QByteArray updateManifestBackupPublicKey();
        static bool updateManifestKeysArePopulated();

        // Subject CN of our code signing cert, exact match. (also mentioned on the download page)
        static constexpr const char* UpdatePublisherCommonName =
            "PLACEHOLDER-Authenticode-Subject-CN";
        static bool updatePublisherIsPinned();

        static constexpr qint64 AuditMaxFileSize = 10 * 1024 * 1024;
        static constexpr int AuditMaxFiles = 5;

        static constexpr const char* IntegrityHashAlgorithm = "SHA256";
        static constexpr bool EnableAntiDebug = true;

        // sentry stays off while this is a placeholder
        static constexpr const char* SentryDsn = "SENTRY-DSN-PLACEHOLDER";
        static bool sentryDsnIsPopulated();

        static constexpr int MaxProfileSize = 1'048'576;
        static constexpr int MaxConfigJsonSize = 8'192;
        static constexpr int MaxProcessNameLength = 256;
        static constexpr int MaxServiceNameLength = 256;
        static constexpr int MaxTaskPathLength = 512;
        static constexpr int MaxUrlLength = 2'048;
        static constexpr int MaxSettingLength = 64;
        static constexpr int MaxSettingValueLength = 256;
        static constexpr int MaxBenchmarkTestLength = 32;
    };
}
