#include "Settings.h"
#include "Paths.h"

#include <QDir>
#include <QFileInfo>
#include <QSettings>

namespace vth
{
    namespace
    {
        constexpr const char* kUserFileName    = "vethida-user.cfg";
        constexpr const char* kDefaultFileName = "vethida.cfg";

        QString userPath() {
            return vth::paths::userData() + QLatin1Char('/') + QLatin1String(kUserFileName);
        }

        QString defaultsPath() {
            return vth::paths::shippedCfg() + QLatin1Char('/') + QLatin1String(kDefaultFileName);
        }
    }

    QString Settings::userFilePath() {
        return userPath();
    }

    Settings::Settings() {
        user_ = std::make_unique<QSettings>(userPath(), QSettings::IniFormat);

        // Defaults file is optional - first launches run without one.
        if (QFileInfo::exists(defaultsPath())) {
            defaults_ = std::make_unique<QSettings>(defaultsPath(), QSettings::IniFormat);
        }
    }

    Settings::~Settings() {
        sync();
    }

    bool Settings::contains(const QString& key) const {
        if (user_ && user_->contains(key)) return true;
        if (defaults_ && defaults_->contains(key)) return true;
        return false;
    }

    QVariant Settings::value(const QString& key, const QVariant& fallback) const {
        if (user_ && user_->contains(key)) {
            return user_->value(key);
        }
        if (defaults_ && defaults_->contains(key)) {
            return defaults_->value(key);
        }
        return fallback;
    }

    void Settings::setValue(const QString& key, const QVariant& v) {
        if (!user_) return;
        user_->setValue(key, v);
    }

    void Settings::remove(const QString& key) {
        if (!user_) return;
        user_->remove(key);
    }

    void Settings::sync() {
        if (user_) user_->sync();
    }

    QStringList Settings::allKeys() const {
        if (!user_) return {};
        return user_->allKeys();
    }

    QByteArray Settings::windowGeometry(const QString& w) const {
        return value(QStringLiteral("ui/%1/geometry").arg(w)).toByteArray();
    }

    void Settings::setWindowGeometry(const QString& w, const QByteArray& g) {
        setValue(QStringLiteral("ui/%1/geometry").arg(w), g);
    }

    QByteArray Settings::windowState(const QString& w) const {
        return value(QStringLiteral("ui/%1/state").arg(w)).toByteArray();
    }

    void Settings::setWindowState(const QString& w, const QByteArray& s) {
        setValue(QStringLiteral("ui/%1/state").arg(w), s);
    }

}
