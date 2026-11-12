#pragma once

#include <QString>

namespace vth::win::authenticode
{
    // signer name if the signature is valid and trusted, otherwise empty
    QString signer(const QString& file);

    // signer(file) contains `expected`, case-insensitively.
    bool signedBy(const QString& file, const QString& expected);
}
