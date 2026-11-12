#include "DeviceIdentityService.h"

#include "SecureStorage.h"
#include "Signature.h"
#include "../Logger.h"
#include "../Runtime/Paths.h"

#include <QtCore/QCryptographicHash>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QMutexLocker>

#include <windows.h>

#include <cstring>
#include <stdexcept>
#include <utility>

namespace vth::services
{
    namespace
    {
        namespace p256 = vth::services::p256;

        VTH_DEFINE_LOG_CATEGORY(catDeviceId, "vth.device_identity")

        constexpr unsigned char kMagic[4] = {0x56, 0x45, 0x44, 0x49};
        constexpr int SeedBytes = p256::PrivateKeySize;
        constexpr int PublicKeyBytes = p256::PublicKeySize;
        constexpr int SignatureBytes = p256::SignatureSize;

        void secureClear(QByteArray& bytes)
        {
            if (!bytes.isEmpty())
                SecureZeroMemory(bytes.data(), static_cast<size_t>(bytes.size()));
        }
    }

    DeviceIdentityService::DeviceIdentityService()
        : m_path(vth::paths::userData() + QStringLiteral("/device-identity.bin"))
    {
    }

    DeviceIdentityService::DeviceIdentityService(const QString& storagePath)
        : m_path(storagePath)
    {
    }

    DeviceIdentityService::~DeviceIdentityService() = default;

    QByteArray DeviceIdentityService::publicKey()
    {
        ensureLoaded();
        QMutexLocker lock(&m_stateLock);
        return m_publicKey;
    }

    QString DeviceIdentityService::publicKeyBase64Url()
    {
        ensureLoaded();
        QMutexLocker lock(&m_stateLock);
        return m_publicKeyBase64Url;
    }

    QString DeviceIdentityService::devicePubkeyId()
    {
        ensureLoaded();
        QMutexLocker lock(&m_stateLock);
        return m_devicePubkeyId;
    }

    bool DeviceIdentityService::hasPersistedIdentity() const
    {
        return QFile::exists(m_path);
    }

    QByteArray DeviceIdentityService::sign(const QByteArray& data)
    {
        ensureLoaded();

        QByteArray seed;
        QByteArray publicKey;
        {
            QMutexLocker lock(&m_stateLock);
            if (m_seed.isEmpty())
                throw std::runtime_error("DeviceIdentityService: keypair was wiped");
            seed      = m_seed;
            publicKey = m_publicKey;
        }
        struct SeedGuard {
            QByteArray& s;
            ~SeedGuard() { secureClear(s); }
        } guard{seed};

        // CNG needs the public point with the private key
        QByteArray signature = p256::sign(seed, publicKey, data);
        if (signature.size() != SignatureBytes)
            throw std::runtime_error("DeviceIdentityService: signing failed");
        return signature;
    }

    QString DeviceIdentityService::signBase64Url(const QByteArray& data)
    {
        return base64UrlEncode(sign(data));
    }

    QByteArray DeviceIdentityService::signUtf8(const QString& s)
    {
        return sign(s.toUtf8());
    }

    QString DeviceIdentityService::signUtf8Base64Url(const QString& s)
    {
        return base64UrlEncode(signUtf8(s));
    }

    bool DeviceIdentityService::verify(const QByteArray& data, const QByteArray& signature)
    {
        if (signature.size() != SignatureBytes)
            return false;

        ensureLoaded();

        QByteArray publicKey;
        {
            QMutexLocker lock(&m_stateLock);
            publicKey = m_publicKey;
        }
        if (publicKey.size() != PublicKeyBytes)
            return false;

        return p256::verify(publicKey, data, signature);
    }

    void DeviceIdentityService::wipe()
    {
        QMutexLocker lock(&m_stateLock);
        secureClear(m_seed);
        secureClear(m_publicKey);
        m_seed.clear();
        m_publicKey.clear();
        m_devicePubkeyId.clear();
        m_publicKeyBase64Url.clear();
        QFile::remove(m_path);
    }

    void DeviceIdentityService::rotate()
    {
        wipe();
        ensureLoaded();
    }

    void DeviceIdentityService::ensureLoaded()
    {
        QMutexLocker lock(&m_stateLock);
        if (!m_seed.isEmpty())
            return;

        QByteArray seed;
        QByteArray pub;
        if (tryLoadFromDisk(seed, pub)) {
            initialiseFields(std::move(seed), std::move(pub));
            return;
        }

        auto fresh = generateKeypair();
        tryPersist(fresh.first, fresh.second);
        initialiseFields(std::move(fresh.first), std::move(fresh.second));
    }

    bool DeviceIdentityService::tryLoadFromDisk(QByteArray& seedOut, QByteArray& publicKeyOut)
    {
        QFile f(m_path);
        if (!f.exists() || !f.open(QIODevice::ReadOnly))
            return false;
        const QByteArray plaintext = SecureStorage::dpapiUnprotect(f.readAll());
        if (plaintext.isEmpty())
            return false;
        return parseBlob(plaintext, seedOut, publicKeyOut);
    }

    bool DeviceIdentityService::parseBlob(const QByteArray& plaintext,
                                          QByteArray& seedOut,
                                          QByteArray& publicKeyOut)
    {
        if (plaintext.size() != BlobSize)
            return false;
        for (int i = 0; i < MagicSize; ++i) {
            if (static_cast<unsigned char>(plaintext.at(i)) != kMagic[i])
                return false;
        }
        if (static_cast<unsigned char>(plaintext.at(MagicSize)) != CurrentVersion)
            return false;

        QByteArray extractedSeed = plaintext.mid(MagicSize + 1, SeedSize);
        QByteArray storedPublic = plaintext.mid(MagicSize + 1 + SeedSize, PublicKeySize);
        if (extractedSeed.size() != SeedSize || storedPublic.size() != PublicKeySize)
            return false;

        // sign/verify round trip to catch a corrupt or tampered blob
        if (!p256::keyPairIsConsistent(extractedSeed, storedPublic)) {
            secureClear(extractedSeed);
            return false;
        }

        seedOut = std::move(extractedSeed);
        publicKeyOut = std::move(storedPublic);
        return true;
    }

    std::pair<QByteArray, QByteArray> DeviceIdentityService::generateKeypair()
    {
        const p256::KeyPair pair = p256::generateKeyPair();
        if (!pair.isValid())
            throw std::runtime_error("DeviceIdentityService: secure random generation failed");
        return {pair.privateKey, pair.publicKey};
    }

    void DeviceIdentityService::tryPersist(const QByteArray& seed, const QByteArray& publicKey)
    {
        QByteArray plaintext(BlobSize, '\0');
        struct PlainGuard {
            QByteArray& p;
            ~PlainGuard() { secureClear(p); }
        } guard{plaintext};

        for (int i = 0; i < MagicSize; ++i)
            plaintext[i] = static_cast<char>(kMagic[i]);
        plaintext[MagicSize] = static_cast<char>(CurrentVersion);
        memcpy(plaintext.data() + MagicSize + 1, seed.constData(), SeedSize);
        memcpy(plaintext.data() + MagicSize + 1 + SeedSize, publicKey.constData(), PublicKeySize);

        const QByteArray ciphertext = SecureStorage::dpapiProtect(plaintext);
        if (ciphertext.isEmpty()) {
            qCWarning(catDeviceId) << "DPAPI encryption failed; identity will be regenerated on next launch";
            return;
        }

        vth::paths::ensureDir(QFileInfo(m_path).absolutePath());
        // write to .tmp then rename
        const QString tmp = m_path + QStringLiteral(".tmp");
        QFile f(tmp);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)
            || f.write(ciphertext) != ciphertext.size()) {
            qCWarning(catDeviceId) << "write to tmp identity file failed";
            return;
        }
        f.close();
        QFile::remove(m_path);
        if (!QFile::rename(tmp, m_path))
            qCWarning(catDeviceId) << "rename tmp->final failed; identity will be regenerated on next launch";
    }

    void DeviceIdentityService::initialiseFields(QByteArray seed, QByteArray publicKey)
    {
        m_seed = std::move(seed);
        m_publicKey = std::move(publicKey);
        m_publicKeyBase64Url = base64UrlEncode(m_publicKey);
        m_devicePubkeyId = derivePubkeyId(m_publicKey);
    }

    QString DeviceIdentityService::derivePubkeyId(const QByteArray& publicKey)
    {
        const QByteArray hash = QCryptographicHash::hash(publicKey, QCryptographicHash::Sha256);
        return QString::fromLatin1(hash.left(16).toHex());
    }

    QString DeviceIdentityService::base64UrlEncode(const QByteArray& bytes)
    {
        QByteArray b64 = bytes.toBase64();
        while (b64.endsWith('='))
            b64.chop(1);
        b64.replace('+', '-');
        b64.replace('/', '_');
        return QString::fromLatin1(b64);
    }

    QString DeviceIdentityService::describeForDiagnostics()
    {
        if (!hasPersistedIdentity())
            return QStringLiteral("device identity: not generated");
        ensureLoaded();
        QString pubkeyId;
        {
            QMutexLocker lock(&m_stateLock);
            pubkeyId = m_devicePubkeyId;
        }
        QFileInfo fi(m_path);
        const QString when = fi.exists()
            ? fi.birthTime().toUTC().toString(QStringLiteral("yyyy-MM-dd"))
            : QStringLiteral("unknown");
        return QStringLiteral("device_pubkey_id=%1 (generated %2)").arg(pubkeyId, when);
    }
}
