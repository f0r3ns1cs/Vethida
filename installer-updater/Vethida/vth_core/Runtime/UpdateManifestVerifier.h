#pragma once

#include "JwsVerify.h"

#include <QtCore/QByteArray>

namespace vth::security
{
    // Checks the update manifest JWS (own keypair, iss vethida-release, aud vethida-updater).
    // Anything other than Valid means the update isn't offered.
    class UpdateManifestVerifier
    {
    public:
        static TokenStatus verify(const QByteArray& jwsEnvelope) noexcept;
    };
}
