#include "LicenseToken.h"

#include "JwsVerify.h"
#include "../Services/SecurityConfig.h"

#include <QtCore/QByteArray>

namespace vth::security
{
    TokenStatus LicenseToken::verify(const QString& token,
                                     const QString& expectedIssuer,
                                     const QString& expectedAudience,
                                     int clockSkewSec) noexcept
    {
        if (!services::SecurityConfig::licenseTokenKeysArePopulated())
            return TokenStatus::KeyNotInstalled;

        const QByteArray issuer   = expectedIssuer.toUtf8();
        const QByteArray audience = expectedAudience.toUtf8();

        return verifyJwsWithRotation(
            token.toUtf8(),
            services::SecurityConfig::licenseTokenPrimaryPublicKey(),
            services::SecurityConfig::licenseTokenBackupPublicKey(),
            issuer.constData(),
            audience.constData(),
            clockSkewSec);
    }
}
