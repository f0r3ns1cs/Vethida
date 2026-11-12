#pragma once

#include <QtCore/QByteArray>

// ECDSA P-256 / SHA-256 via CNG. Raw sizes everywhere, no DER.

namespace vth::services::p256
{
    inline constexpr int PublicKeySize  = 64;   // X || Y
    inline constexpr int PrivateKeySize = 32;   // d
    inline constexpr int SignatureSize  = 64;   // r || s

    struct KeyPair {
        QByteArray privateKey;
        QByteArray publicKey;

        bool isValid() const {
            return privateKey.size() == PrivateKeySize
                && publicKey.size() == PublicKeySize;
        }
    };

    KeyPair generateKeyPair() noexcept;

    // CNG needs the public point to import a private key, hence both
    QByteArray sign(const QByteArray& privateKey,
                    const QByteArray& publicKey,
                    const QByteArray& data) noexcept;

    bool verify(const QByteArray& publicKey,
                const QByteArray& data,
                const QByteArray& signature) noexcept;

    // sign + verify a test message to check the two halves belong together
    bool keyPairIsConsistent(const QByteArray& privateKey,
                             const QByteArray& publicKey) noexcept;
}
