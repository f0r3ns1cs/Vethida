#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include <chrono>

namespace vth::services
{
    // Keys/pins ship as PLACEHOLDER values so check-release-ready.mjs can catch
    // an unpopulated build.
    class SecurityConfig
    {
    public:
        static constexpr const char* ApiBaseUrl = "https://api.vethida.com";
        static constexpr std::chrono::milliseconds ApiTimeout{15000};
        static constexpr int ApiMaxRetries = 3;
        static constexpr std::chrono::milliseconds ApiRetryBaseDelay{1000};

        static constexpr const char* LicenseKeyPattern =
            "^[A-Z0-9]{5}-[A-Z0-9]{5}-[A-Z0-9]{5}-[A-Z0-9]{5}-[A-Z0-9]{5}$";
        // Heartbeat policy. The token's exp decides expiry, OfflineHardCutoffHours
        // caps how long we go without reaching the server either way.
        static constexpr int HeartbeatIntervalMinutes = 15;
        static constexpr int TokenLifetimeHours       = 24;
        static constexpr int OfflineHardCutoffHours   = 72;
        static constexpr int BackoffInitialSeconds    = 30;
        static constexpr int BackoffMaxMinutes        = 15;

        static constexpr std::chrono::seconds LicenseVerifyInterval{
            HeartbeatIntervalMinutes * 60};

        // P-256 keys (raw X || Y) for the license tokens
        static constexpr const char* LicenseTokenPrimaryKid = "rens-license-v1-PLACEHOLDER";
        static constexpr const char* LicenseTokenBackupKid  = "rens-license-v2-PLACEHOLDER";

        static QByteArray licenseTokenPrimaryPublicKey();
        static QByteArray licenseTokenBackupPublicKey();

        static bool licenseTokenKeysArePopulated();

        static constexpr const char* UpdateManifestPrimaryKid = "vethida-update-v1-PLACEHOLDER";
        static constexpr const char* UpdateManifestBackupKid  = "vethida-update-v2-PLACEHOLDER";

        static QByteArray updateManifestPrimaryPublicKey();
        static QByteArray updateManifestBackupPublicKey();
        static bool updateManifestKeysArePopulated();

        // Our Authenticode cert CN, exact match. WinVerifyTrust alone accepts anyone's
        // signed binary.
        static constexpr const char* UpdatePublisherCommonName =
            "PLACEHOLDER-Authenticode-Subject-CN";
        static bool updatePublisherIsPinned();

        static constexpr qint64 AuditMaxFileSize = 10 * 1024 * 1024;
        static constexpr int AuditMaxFiles = 5;

        static constexpr const char* IntegrityHashAlgorithm = "SHA256";
        static constexpr bool EnableAntiDebug = true;

        static QStringList allowedOrigins();
        static constexpr bool EnforceHttps = true;

        // SPKI SHA-256 pins for api.vethida.com (current + backup)
        static QStringList apiSpkiPins();

        // True once the pins are real 64-char lowercase-hex SHA-256 values.
        static bool apiCertPinsArePopulated();

        // placeholder = Sentry stays off
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
