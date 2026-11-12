#include "LicenseTokenCache.h"

#include "SecureStorage.h"

#include <QtCore/QDateTime>

namespace vth::services
{
    namespace
    {
        constexpr const char* kStorageKey = "license_token_cache";
        constexpr const char* kBeatKey    = "license_last_heartbeat";
    }

    void LicenseTokenCache::save(const QString& token)
    {
        SecureStorage::set(QString::fromLatin1(kStorageKey), token);
    }

    std::optional<QString> LicenseTokenCache::load()
    {
        return SecureStorage::get(QString::fromLatin1(kStorageKey));
    }

    void LicenseTokenCache::saveLastHeartbeatUtcMs(qint64 utcMs)
    {
        SecureStorage::set(QString::fromLatin1(kBeatKey), QString::number(utcMs));
    }

    qint64 LicenseTokenCache::loadLastHeartbeatUtcMs()
    {
        const auto raw = SecureStorage::get(QString::fromLatin1(kBeatKey));
        if (!raw.has_value())
            return 0;
        bool ok = false;
        const qint64 v = raw->toLongLong(&ok);
        // a future stamp means the clock was changed, treat it as none
        if (!ok || v < 0 || v > QDateTime::currentMSecsSinceEpoch())
            return 0;
        return v;
    }

    void LicenseTokenCache::deleteToken()
    {
        SecureStorage::deleteKey(QString::fromLatin1(kStorageKey));
        SecureStorage::deleteKey(QString::fromLatin1(kBeatKey));
    }
}
