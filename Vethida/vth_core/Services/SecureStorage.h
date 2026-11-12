#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QString>

#include <optional>

namespace vth::services
{
    // Encrypted per-user files, through DPAPI.
    class SecureStorage
    {
    public:
        static std::optional<QString> get(const QString& key);
        static void set(const QString& key, const QString& value);
        static void setJson(const QString& key, const QByteArray& json);
        static std::optional<QByteArray> getJson(const QString& key);

        static void deleteKey(const QString& key);

    private:
        static QString filePath(const QString& key);
        static QString sanitizeKey(const QString& key);
    };
}
