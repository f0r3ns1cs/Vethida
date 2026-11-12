#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QMutex>
#include <QtCore/QString>

#include <utility>

namespace vth::services
{
    // Per-install P-256 device key, private part DPAPI-sealed in local app data.
    // Signs every /v1/desktop/* request. Server id = hex(sha256(pubkey)[0..16]).
    // Wiping it breaks the binding, the user has to deactivate and reactivate.
    class DeviceIdentityService
    {
    public:
        DeviceIdentityService();
        explicit DeviceIdentityService(const QString& storagePath);
        ~DeviceIdentityService();

        // The 64-byte P-256 public point, X then Y.
        QByteArray publicKey();

        // The public key base64url-encoded (86 chars, no padding).
        QString publicKeyBase64Url();

        // 32 hex chars
        QString devicePubkeyId();

        bool hasPersistedIdentity() const;

        // Sign data with the device private key (64-byte detached signature).
        QByteArray sign(const QByteArray& data);

        QString signBase64Url(const QByteArray& data);
        QByteArray signUtf8(const QString& s);
        QString signUtf8Base64Url(const QString& s);

        // 64-byte r || s, false on anything invalid
        bool verify(const QByteArray& data, const QByteArray& signature);

        // Wipe the in-memory and on-disk keypair.
        void wipe();

        // Force-rotate the keypair.
        void rotate();

        QString describeForDiagnostics();

    private:
        static constexpr int MagicSize = 4;
        // 2 = P-256 (old Ed25519 blobs are rejected and a new key is made)
        static constexpr unsigned char CurrentVersion = 0x02;
        static constexpr int SeedSize = 32;
        static constexpr int PublicKeySize = 64;
        static constexpr int BlobSize = MagicSize + 1 + SeedSize + PublicKeySize; // 101 bytes

        QString m_path;
        QMutex m_stateLock;

        QByteArray m_seed;
        QByteArray m_publicKey;
        QString m_devicePubkeyId;
        QString m_publicKeyBase64Url;

        void ensureLoaded();
        bool tryLoadFromDisk(QByteArray& seedOut, QByteArray& publicKeyOut);
        void tryPersist(const QByteArray& seed, const QByteArray& publicKey);
        void initialiseFields(QByteArray seed, QByteArray publicKey);

        static bool parseBlob(const QByteArray& plaintext, QByteArray& seedOut, QByteArray& publicKeyOut);
        static std::pair<QByteArray, QByteArray> generateKeypair();
        static QString derivePubkeyId(const QByteArray& publicKey);
        static QString base64UrlEncode(const QByteArray& bytes);
    };
}
