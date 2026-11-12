#include "LicenseStore.h"

#include "SecureStorage.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace vth::services
{
    namespace
    {
        constexpr const char* kStorageKey = "license_info";

        vth::models::LicenseInfo deserialize(const QJsonObject& o)
        {
            using namespace vth::models;
            LicenseInfo info;
            info.key = o.value(QStringLiteral("key")).toString();
            info.tier = o.value(QStringLiteral("tier")).toString();
            info.status = static_cast<LicenseStatus>(o.value(QStringLiteral("status")).toInt(0));
            info.activatedAt = o.value(QStringLiteral("activatedAt")).toString();
            info.expiresAt = o.value(QStringLiteral("expiresAt")).toString();
            info.lastVerifiedUtcMs = static_cast<qint64>(o.value(QStringLiteral("lastVerifiedUtcMs")).toDouble(0));
            info.devicePubkeyId = o.value(QStringLiteral("devicePubkeyId")).toString();
            info.fingerprintStatus = o.value(QStringLiteral("fingerprintStatus")).toString();
            info.resetsRemaining = o.value(QStringLiteral("resetsRemaining")).toInt(0);
            // saved so the warning shows before the first heartbeat
            info.accountDeleting = o.value(QStringLiteral("accountDeleting")).toBool(false);
            info.accountHardDeleteAt = o.value(QStringLiteral("accountHardDeleteAt")).toString();
            // browser sign-in only
            info.accountEmail = o.value(QStringLiteral("accountEmail")).toString();
            return info;
        }

        QByteArray serialize(const vth::models::LicenseInfo& info)
        {
            QJsonObject o;
            o[QStringLiteral("key")] = info.key;
            o[QStringLiteral("tier")] = info.tier;
            o[QStringLiteral("status")] = static_cast<int>(info.status);
            o[QStringLiteral("activatedAt")] = info.activatedAt;
            o[QStringLiteral("expiresAt")] = info.expiresAt;
            o[QStringLiteral("lastVerifiedUtcMs")] = static_cast<double>(info.lastVerifiedUtcMs);
            o[QStringLiteral("devicePubkeyId")] = info.devicePubkeyId;
            o[QStringLiteral("fingerprintStatus")] = info.fingerprintStatus;
            o[QStringLiteral("resetsRemaining")] = info.resetsRemaining;
            o[QStringLiteral("accountDeleting")] = info.accountDeleting;
            o[QStringLiteral("accountHardDeleteAt")] = info.accountHardDeleteAt;
            o[QStringLiteral("accountEmail")] = info.accountEmail;
            return QJsonDocument(o).toJson(QJsonDocument::Compact);
        }
    }

    vth::models::LicenseInfo LicenseStore::load()
    {
        const auto bytes = SecureStorage::getJson(QString::fromLatin1(kStorageKey));
        if (!bytes.has_value())
            return {};
        const QJsonDocument doc = QJsonDocument::fromJson(*bytes);
        if (!doc.isObject())
            return {};
        return deserialize(doc.object());
    }

    void LicenseStore::save(const vth::models::LicenseInfo& info)
    {
        SecureStorage::setJson(QString::fromLatin1(kStorageKey), serialize(info));
    }

    void LicenseStore::deleteLicense()
    {
        SecureStorage::deleteKey(QString::fromLatin1(kStorageKey));
    }
}
