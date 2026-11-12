#include "UpdateManifestVerifier.h"

#include "JwsVerify.h"
#include "../Services/SecurityConfig.h"

namespace vth::security
{
    namespace
    {
        constexpr const char* kIssuer       = "vethida-release";
        constexpr const char* kAudience     = "vethida-updater";
        constexpr int         kClockSkewSec = 300;
    }

    TokenStatus UpdateManifestVerifier::verify(const QByteArray& jwsEnvelope) noexcept
    {
        if (!services::SecurityConfig::updateManifestKeysArePopulated())
            return TokenStatus::KeyNotInstalled;

        return verifyJwsWithRotation(
            jwsEnvelope,
            services::SecurityConfig::updateManifestPrimaryPublicKey(),
            services::SecurityConfig::updateManifestBackupPublicKey(),
            kIssuer,
            kAudience,
            kClockSkewSec);
    }
}
