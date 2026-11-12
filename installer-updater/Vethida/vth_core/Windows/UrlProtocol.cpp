#include "UrlProtocol.h"

#include <QDir>
#include <QRegularExpression>
#include <QSettings>

namespace vth::win
{
    namespace
    {
        constexpr auto kRoot = "HKEY_CURRENT_USER\\Software\\Classes\\vethida";
        constexpr auto kCommand =
            "HKEY_CURRENT_USER\\Software\\Classes\\vethida\\shell\\open\\command";
        constexpr auto kIcon = "HKEY_CURRENT_USER\\Software\\Classes\\vethida\\DefaultIcon";

        QString quotedPath(const QString& executablePath) {
            // "%1" here is for the shell, not QString::arg
            return QStringLiteral("\"") + QDir::toNativeSeparators(executablePath)
                 + QStringLiteral("\" \"%1\"");
        }

        QString quotedIcon(const QString& executablePath) {
            return QStringLiteral("\"%1\",0").arg(QDir::toNativeSeparators(executablePath));
        }

        bool isCanonicalLicenseKey(const QString& key) { // uuid
            static const QRegularExpression re(QStringLiteral(
                "^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$"),
                QRegularExpression::CaseInsensitiveOption);
            return re.match(key).hasMatch();
        }

        QString activationPayload(const QString& url) {
            static const QString kPrefix = QStringLiteral("vethida://activate-");

            const QString trimmed = url.trimmed();
            if (trimmed.size() <= kPrefix.size()) return {};
            if (!trimmed.startsWith(kPrefix, Qt::CaseInsensitive)) return {};

            QString payload = trimmed.mid(kPrefix.size());
            if (auto idx = payload.indexOf(QLatin1Char('/')); idx >= 0) payload.truncate(idx);
            if (auto idx = payload.indexOf(QLatin1Char('?')); idx >= 0) payload.truncate(idx);
            if (auto idx = payload.indexOf(QLatin1Char('#')); idx >= 0) payload.truncate(idx);
            return payload;
        }
    }

    bool registerUrlProtocol(const QString& executablePath) {
        if (executablePath.isEmpty()) return false;

        QSettings root(QString::fromLatin1(kRoot), QSettings::NativeFormat);
        root.setValue(QStringLiteral("Default"), QStringLiteral("URL:Vethida activation"));
        root.setValue(QStringLiteral("URL Protocol"), QString());
        root.sync();
        if (root.status() != QSettings::NoError) return false;

        QSettings cmd(QString::fromLatin1(kCommand), QSettings::NativeFormat);
        cmd.setValue(QStringLiteral("Default"), quotedPath(executablePath));
        cmd.sync();
        if (cmd.status() != QSettings::NoError) return false;

        QSettings icon(QString::fromLatin1(kIcon), QSettings::NativeFormat);
        icon.setValue(QStringLiteral("Default"), quotedIcon(executablePath));
        icon.sync();
        return icon.status() == QSettings::NoError;
    }

    bool unregisterUrlProtocol() {
        QSettings root(QString::fromLatin1(kRoot), QSettings::NativeFormat);
        root.clear();
        root.sync();
        return root.status() == QSettings::NoError;
    }

    bool isUrlProtocolRegistered(const QString& executablePath) {
        QSettings cmd(QString::fromLatin1(kCommand), QSettings::NativeFormat);
        return cmd.value(QStringLiteral("Default")).toString() == quotedPath(executablePath);
    }

    std::optional<QString> parseLicenseKeyFromUrl(const QString& url) {
        const QString key = activationPayload(url).trimmed();
        if (!isCanonicalLicenseKey(key)) return std::nullopt;
        return key;
    }

    std::optional<QString> findActivationUrlInArgs(const QStringList& args) {
        static const QString kFlag = QStringLiteral("--activate-url=");
        for (const auto& arg : args) {
            if (arg.startsWith(QStringLiteral("vethida://"), Qt::CaseInsensitive)) {
                return arg;
            }
            if (arg.startsWith(kFlag, Qt::CaseInsensitive)) {
                return arg.mid(kFlag.size());
            }
        }
        return std::nullopt;
    }
}
