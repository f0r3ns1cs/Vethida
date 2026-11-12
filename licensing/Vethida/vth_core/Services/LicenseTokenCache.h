#pragma once

#include <QtCore/QString>

#include <optional>

namespace vth::services
{
    // DPAPI-sealed license token, used offline until it expires
    class LicenseTokenCache
    {
    public:
        static void save(const QString& token);

        // The cached token, or nullopt if no cache exists / DPAPI unwrap failed.
        static std::optional<QString> load();

        // Last successful heartbeat (UTC ms), sealed so it can't be edited. Used by
        // the offline cutoff and the heartbeat scheduler. 0 = never.
        static void  saveLastHeartbeatUtcMs(qint64 utcMs);
        static qint64 loadLastHeartbeatUtcMs();


        static void deleteToken();
    };
}
