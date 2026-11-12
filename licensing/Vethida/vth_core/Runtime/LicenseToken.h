#pragma once

#include "JwsVerify.h"

#include <QtCore/QString>

namespace vth::security
{
    // Offline check of the ES256 license token against the SecurityConfig keys
    // (primary, then backup). Valid only if signature, exp, iss and aud all pass.
    class LicenseToken
    {
    public:
        static TokenStatus verify(const QString& token,
                                  const QString& expectedIssuer,
                                  const QString& expectedAudience,
                                  int clockSkewSec = 60) noexcept;
    };
}
