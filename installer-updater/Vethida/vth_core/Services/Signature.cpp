#include "Signature.h"

#include <QtCore/QCryptographicHash>

#include <windows.h>

#include <bcrypt.h>

#include <cstring>

#pragma comment(lib, "bcrypt.lib")

namespace vth::services::p256
{
    namespace
    {
        constexpr int kCoordinateSize = 32;   // P-256 field element

        bool ok(NTSTATUS s) noexcept { return s >= 0; }

        struct AlgHandle {
            BCRYPT_ALG_HANDLE h = nullptr;
            ~AlgHandle() { if (h) BCryptCloseAlgorithmProvider(h, 0); }
        };

        struct KeyHandle {
            BCRYPT_KEY_HANDLE h = nullptr;
            ~KeyHandle() { if (h) BCryptDestroyKey(h); }
        };

        bool openProvider(AlgHandle& alg) noexcept
        {
            return ok(BCryptOpenAlgorithmProvider(&alg.h, BCRYPT_ECDSA_P256_ALGORITHM,
                                                  nullptr, 0));
        }

        // header + X, Y (+ d for private)
        QByteArray eccBlob(ULONG magic, const QByteArray& publicKey,
                           const QByteArray& privateKey)
        {
            QByteArray blob(static_cast<int>(sizeof(BCRYPT_ECCKEY_BLOB))
                            + publicKey.size() + privateKey.size(), '\0');
            auto* header = reinterpret_cast<BCRYPT_ECCKEY_BLOB*>(blob.data());
            header->dwMagic = magic;
            header->cbKey   = kCoordinateSize;

            char* cursor = blob.data() + sizeof(BCRYPT_ECCKEY_BLOB);
            std::memcpy(cursor, publicKey.constData(), static_cast<size_t>(publicKey.size()));
            if (!privateKey.isEmpty())
                std::memcpy(cursor + publicKey.size(), privateKey.constData(),
                            static_cast<size_t>(privateKey.size()));
            return blob;
        }

        QByteArray sha256(const QByteArray& data)
        {
            return QCryptographicHash::hash(data, QCryptographicHash::Sha256);
        }
    }

    KeyPair generateKeyPair() noexcept
    {
        AlgHandle alg;
        if (!openProvider(alg))
            return {};

        KeyHandle key;
        if (!ok(BCryptGenerateKeyPair(alg.h, &key.h, 256, 0)))
            return {};
        if (!ok(BCryptFinalizeKeyPair(key.h, 0)))
            return {};

        ULONG needed = 0;
        if (!ok(BCryptExportKey(key.h, nullptr, BCRYPT_ECCPRIVATE_BLOB,
                                nullptr, 0, &needed, 0)))
            return {};

        QByteArray blob(static_cast<int>(needed), '\0');
        if (!ok(BCryptExportKey(key.h, nullptr, BCRYPT_ECCPRIVATE_BLOB,
                                reinterpret_cast<PUCHAR>(blob.data()), needed, &needed, 0)))
            return {};
        if (blob.size() < static_cast<int>(sizeof(BCRYPT_ECCKEY_BLOB))
                          + PublicKeySize + PrivateKeySize)
            return {};

        const char* cursor = blob.constData() + sizeof(BCRYPT_ECCKEY_BLOB);
        KeyPair pair;
        pair.publicKey  = QByteArray(cursor, PublicKeySize);
        pair.privateKey = QByteArray(cursor + PublicKeySize, PrivateKeySize);
        return pair;
    }

    QByteArray sign(const QByteArray& privateKey,
                    const QByteArray& publicKey,
                    const QByteArray& data) noexcept
    {
        if (privateKey.size() != PrivateKeySize || publicKey.size() != PublicKeySize)
            return {};

        AlgHandle alg;
        if (!openProvider(alg))
            return {};

        const QByteArray blob = eccBlob(BCRYPT_ECDSA_PRIVATE_P256_MAGIC, publicKey, privateKey);
        KeyHandle key;
        if (!ok(BCryptImportKeyPair(alg.h, nullptr, BCRYPT_ECCPRIVATE_BLOB, &key.h,
                                    reinterpret_cast<PUCHAR>(const_cast<char*>(blob.constData())),
                                    static_cast<ULONG>(blob.size()), 0)))
            return {};

        const QByteArray digest = sha256(data);
        QByteArray signature(SignatureSize, '\0');
        ULONG written = 0;
        if (!ok(BCryptSignHash(key.h, nullptr,
                               reinterpret_cast<PUCHAR>(const_cast<char*>(digest.constData())),
                               static_cast<ULONG>(digest.size()),
                               reinterpret_cast<PUCHAR>(signature.data()),
                               static_cast<ULONG>(signature.size()), &written, 0)))
            return {};
        if (written != static_cast<ULONG>(SignatureSize))
            return {};
        return signature;
    }

    bool keyPairIsConsistent(const QByteArray& privateKey,
                             const QByteArray& publicKey) noexcept
    {
        const QByteArray probe = QByteArrayLiteral("vethida-device-identity-probe");
        const QByteArray signature = sign(privateKey, publicKey, probe);
        return !signature.isEmpty() && verify(publicKey, probe, signature);
    }

    bool verify(const QByteArray& publicKey,
                const QByteArray& data,
                const QByteArray& signature) noexcept
    {
        if (publicKey.size() != PublicKeySize || signature.size() != SignatureSize)
            return false;

        AlgHandle alg;
        if (!openProvider(alg))
            return false;

        const QByteArray blob = eccBlob(BCRYPT_ECDSA_PUBLIC_P256_MAGIC, publicKey, {});
        KeyHandle key;
        if (!ok(BCryptImportKeyPair(alg.h, nullptr, BCRYPT_ECCPUBLIC_BLOB, &key.h,
                                    reinterpret_cast<PUCHAR>(const_cast<char*>(blob.constData())),
                                    static_cast<ULONG>(blob.size()), 0)))
            return false;

        const QByteArray digest = sha256(data);
        return ok(BCryptVerifySignature(
            key.h, nullptr,
            reinterpret_cast<PUCHAR>(const_cast<char*>(digest.constData())),
            static_cast<ULONG>(digest.size()),
            reinterpret_cast<PUCHAR>(const_cast<char*>(signature.constData())),
            static_cast<ULONG>(signature.size()), 0));
    }
}
