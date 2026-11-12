#pragma once

#include <QString>
#include <QStringList>

#include <optional>

namespace vth::win
{

    // vethida://activate-<KEY> links, registered under HKCU. Any page can open these,
    // so the key is untrusted and the user still has to confirm.

    bool registerUrlProtocol(const QString& executablePath);
    bool unregisterUrlProtocol();
    bool isUrlProtocolRegistered(const QString& executablePath);

    std::optional<QString> parseLicenseKeyFromUrl(const QString& url);

    std::optional<QString> findActivationUrlInArgs(const QStringList& args);

}
