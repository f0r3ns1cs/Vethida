#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QtGlobal>

namespace vth::security
{
    // shared by the license token and the update manifest checks
    enum class TokenStatus : quint8 {
        Valid,
        Malformed,          // not a three-part ES256 JWS, or a claim failed to parse
        SignatureMismatch,
        Expired,            // now > exp + skew
        NotYetValid,        // now + skew < nbf
        IssuerMismatch,
        AudienceMismatch,
        KeyNotInstalled,    // key is absent or not 64 bytes
        UnknownFailure,
    };

    // ES256 compact JWS. Tries the backup key only on a signature mismatch (key rotation).
    // Pass null/empty issuer or audience to skip that check.
    TokenStatus verifyJwsWithRotation(const QByteArray& jws,
                                      const QByteArray& primaryKey,
                                      const QByteArray& backupKey,
                                      const char* expectedIssuer,
                                      const char* expectedAudience,
                                      int clockSkewSeconds) noexcept;
}
