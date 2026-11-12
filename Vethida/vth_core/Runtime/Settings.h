#pragma once

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QVariant>

#include <memory>

class QSettings;

namespace vth
{

    class Settings {
    public:
        Settings();
        ~Settings();

        Settings(const Settings&) = delete;
        Settings& operator=(const Settings&) = delete;

        static QString userFilePath();

        bool     contains(const QString& key) const;
        QVariant value(const QString& key, const QVariant& fallback = {}) const;
        void     setValue(const QString& key, const QVariant& value);
        void     remove(const QString& key);
        void     sync();

        QStringList allKeys() const;

        QByteArray windowGeometry(const QString& windowName) const;
        void       setWindowGeometry(const QString& windowName, const QByteArray& g);
        QByteArray windowState(const QString& windowName) const;
        void       setWindowState(const QString& windowName, const QByteArray& s);

    private:
        std::unique_ptr<QSettings> defaults_;  // shipped, read-only
        std::unique_ptr<QSettings> user_;      // %LOCALAPPDATA% override
    };

}
