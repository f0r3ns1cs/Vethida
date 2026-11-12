#include "Apps.h"

#include "../Logger.h"
#include "Registry.h"

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QProcess>
#include <QRegularExpression>
#include <QStringLiteral>
#include <QUrl>
#include <QUuid>
#include <QXmlStreamReader>

#include <climits>
#include <iterator>
#include <vector>

#include <windows.h>
#include <shellapi.h>
#include "Bloatware.h"

namespace vth::win::apps
{
    namespace
    {
        VTH_DEFINE_LOG_CATEGORY(catApps, "vth.apps")

        // Absolute paths, because this runs elevated and PATH could be hijacked.
        QString system32Dir() {
            wchar_t dir[MAX_PATH] = {};
            const UINT n = GetSystemDirectoryW(dir, MAX_PATH);
            return (n > 0 && n < MAX_PATH)
                ? QString::fromWCharArray(dir, static_cast<int>(n))
                : QStringLiteral("C:\\Windows\\System32");
        }

        QString pinnedPowerShell() {
            return system32Dir()
                + QStringLiteral("\\WindowsPowerShell\\v1.0\\powershell.exe");
        }

        class IconHandle {
        public:
            void reset(HICON h = nullptr) noexcept {
                if (h_) DestroyIcon(h_);
                h_ = h;
            }
            IconHandle() = default;
            ~IconHandle() noexcept { if (h_) DestroyIcon(h_); }

            IconHandle(const IconHandle&)            = delete;
            IconHandle& operator=(const IconHandle&) = delete;

            HICON  get()  const noexcept { return h_; }
            HICON* addr() noexcept       { return &h_; }
            explicit operator bool() const noexcept { return h_ != nullptr; }

        private:
            HICON h_ = nullptr;
        };

        // Takes a registry DisplayIcon string, "path" or "path,index".
        QImage extractIconFor(const QString& displayIcon) {
            if (displayIcon.isEmpty()) return {};

            QString path = displayIcon.trimmed();
            int     index = 0;

            if (path.startsWith(QChar::fromLatin1('"')) && path.endsWith(QChar::fromLatin1('"')) && path.size() >= 2) {
                path = path.mid(1, path.size() - 2).trimmed();
            }

            const int comma = path.lastIndexOf(QChar::fromLatin1(','));
            if (comma > 0) {
                bool ok = false;
                const int parsed = path.mid(comma + 1).trimmed().toInt(&ok);
                if (ok) {
                    QString left = path.left(comma).trimmed();
                    if (left.startsWith(QChar::fromLatin1('"')) && left.endsWith(QChar::fromLatin1('"')) && left.size() >= 2) {
                        left = left.mid(1, left.size() - 2).trimmed();
                    }
                    path  = left;
                    index = parsed >= 0 ? parsed : 0;
                }
            }

            if (path.isEmpty()) return {};

            wchar_t expandedBuf[2048] = {0};
            const std::wstring pathW = path.toStdWString();
            const DWORD len = ExpandEnvironmentStringsW(
                pathW.c_str(), expandedBuf,
                static_cast<DWORD>(std::size(expandedBuf)));
            if (len == 0 || len > std::size(expandedBuf)) return {};

            const QString expanded = QString::fromWCharArray(expandedBuf);
            if (!QFileInfo::exists(expanded)) return {};

            IconHandle hLarge;
            IconHandle hSmall;
            const UINT extracted = ExtractIconExW(
                expandedBuf, index,
                hLarge.addr(),
                hSmall.addr(),
                1);
            if (extracted == 0 || extracted == UINT_MAX) {
                return {};
            }

            // Large first, because a small icon looks blurry scaled up.
            const HICON chosen = hLarge ? hLarge.get() : hSmall.get();
            if (!chosen) return {};

            return QImage::fromHICON(chosen);
        }

        void enumerateUninstallHive(HKEY                 root,
                                    const wchar_t*       subPath,
                                    Source               source,
                                    Architecture         arch,
                                    QVector<AppInfo>&    out) {
            HKEY hKey = nullptr;
            if (RegOpenKeyExW(root, subPath, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
                return;
            }

            DWORD subkeyCount  = 0;
            DWORD maxSubkeyLen = 0;
            if (RegQueryInfoKeyW(hKey, nullptr, nullptr, nullptr,
                                 &subkeyCount, &maxSubkeyLen, nullptr, nullptr,
                                 nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) {
                RegCloseKey(hKey);
                return;
            }

            std::vector<wchar_t> nameBuf(maxSubkeyLen + 2);

            for (DWORD i = 0; i < subkeyCount; ++i) {
                DWORD nameLen = static_cast<DWORD>(nameBuf.size());
                if (RegEnumKeyExW(hKey, i, nameBuf.data(), &nameLen,
                                  nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) {
                    continue;
                }
                HKEY subKey = nullptr;
                if (RegOpenKeyExW(hKey, nameBuf.data(), 0, KEY_READ, &subKey)
                        != ERROR_SUCCESS) {
                    continue;
                }

                const QString displayName = reg::readStringAt(subKey, L"DisplayName");
                if (displayName.isEmpty()) {
                    RegCloseKey(subKey);
                    continue;
                }
                if (reg::readDwordAt(subKey, L"SystemComponent") == 1) {
                    RegCloseKey(subKey);
                    continue;
                }
                if (!reg::readStringAt(subKey, L"ParentKeyName").isEmpty()) {
                    RegCloseKey(subKey);
                    continue;
                }

                AppInfo info;
                info.type                 = Type::Desktop;
                info.displayName          = displayName;
                info.publisher            = reg::readStringAt(subKey, L"Publisher");
                info.version              = reg::readStringAt(subKey, L"DisplayVersion");
                info.installLocation      = reg::readStringAt(subKey, L"InstallLocation");
                info.uninstallString      = reg::readStringAt(subKey, L"UninstallString");
                info.quietUninstallString = reg::readStringAt(subKey, L"QuietUninstallString");
                info.modifyPath           = reg::readStringAt(subKey, L"ModifyPath");
                info.displayIcon          = reg::readStringAt(subKey, L"DisplayIcon");
                info.comments             = reg::readStringAt(subKey, L"Comments");
                info.helpLink             = reg::readStringAt(subKey, L"HelpLink");
                info.contactInfo          = reg::readStringAt(subKey, L"Contact");
                info.windowsInstaller     = reg::readDwordAt(subKey, L"WindowsInstaller") == 1;

                const QString dateStr = reg::readStringAt(subKey, L"InstallDate");
                if (dateStr.length() == 8) {
                    info.installDate =
                        QDate::fromString(dateStr, QStringLiteral("yyyyMMdd"));
                }

                const DWORD sizeKB = reg::readDwordAt(subKey, L"EstimatedSize");
                info.estimatedBytes = static_cast<quint64>(sizeKB) * 1024ULL;

                info.source       = source;
                info.architecture = arch;

                info.iconImage = extractIconFor(reg::readStringAt(subKey, L"DisplayIcon"));
                if (info.iconImage.isNull() && !info.installLocation.isEmpty()) {
                    QDir dir(info.installLocation);
                    const QStringList exes =
                        dir.entryList({QStringLiteral("*.exe")},
                                      QDir::Files | QDir::NoSymLinks,
                                      QDir::Time);
                    if (!exes.isEmpty()) {
                        info.iconImage = extractIconFor(dir.absoluteFilePath(exes.first()));
                    }
                }

                out.append(std::move(info));
                RegCloseKey(subKey);
            }

            RegCloseKey(hKey);
        }

        // The publisher is an X.500 name, such as CN=Microsoft Corporation, O=...
        QString cleanPublisher(const QString& raw) {
            if (!raw.startsWith(QStringLiteral("CN=")))
                return raw;

            const QString value = raw.mid(3);
            if (value.startsWith(QLatin1Char('"'))) {
                const int closing = value.indexOf(QLatin1Char('"'), 1);
                return closing > 0 ? value.mid(1, closing - 1).trimmed()
                                   : value.mid(1).trimmed();
            }

            QString out;
            out.reserve(value.size());
            for (int i = 0; i < value.size(); ++i) {
                const QChar c = value.at(i);
                if (c == QLatin1Char('\\') && i + 1 < value.size()) {
                    out.append(value.at(++i));
                    continue;
                }
                if (c == QLatin1Char(','))
                    break;
                out.append(c);
            }
            return out.trimmed();
        }

        // Store accounts registered to a person can have a GUID as their subject.
        bool isGuid(const QString& value) {
            return !QUuid::fromString(value).isNull();
        }

        struct ManifestFacts {
            QString displayName;
            QString publisher;
            QString logo;      // relative path, e.g. "Assets\\StoreLogo.png"
        };

        // ms-resource references can't be resolved here.
        QString unlessResource(const QString& value) {
            return value.startsWith(QLatin1String("ms-resource"), Qt::CaseInsensitive)
                ? QString() : value;
        }

        ManifestFacts manifestFacts(const QString& installLocation) {
            ManifestFacts facts;
            if (installLocation.isEmpty())
                return facts;

            QFile manifest(
                QDir(installLocation).filePath(QStringLiteral("AppxManifest.xml")));
            if (!manifest.open(QIODevice::ReadOnly))
                return facts;

            QXmlStreamReader xml(&manifest);
            bool inProperties = false;

            while (!xml.atEnd()) {
                const QXmlStreamReader::TokenType token = xml.readNext();
                const QString name = xml.name().toString();

                if (token == QXmlStreamReader::StartElement) {
                    if (name == QLatin1String("Properties")) {
                        inProperties = true;
                    } else if (inProperties && name == QLatin1String("DisplayName")) {
                        facts.displayName = unlessResource(xml.readElementText().trimmed());
                    } else if (inProperties
                               && name == QLatin1String("PublisherDisplayName")) {
                        facts.publisher = unlessResource(xml.readElementText().trimmed());
                    } else if (inProperties && name == QLatin1String("Logo")) {
                        facts.logo = xml.readElementText().trimmed();
                    }
                } else if (token == QXmlStreamReader::EndElement
                           && name == QLatin1String("Properties")) {
                    break;
                }
            }
            return facts;
        }

        // On disk the logo usually only exists as variants like
        // StoreLogo.scale-200.png, so take the largest non-contrast one.
        QImage loadUwpLogo(const QString& installLocation, const QString& logoRel) {
            if (installLocation.isEmpty() || logoRel.isEmpty())
                return {};

            QString rel = logoRel;
            rel.replace(QLatin1Char('\\'), QLatin1Char('/'));
            const QFileInfo base(QDir(installLocation).filePath(rel));

            QImage img;
            if (base.exists()) {
                img = QImage(base.filePath());
            } else {
                const QDir dir = base.dir();
                const QString pattern =
                    base.completeBaseName() + QStringLiteral(".*.") + base.suffix();
                static const QRegularExpression kQualifier(QStringLiteral("-(\\d+)"));
                int best = -1;
                QString bestFile;
                for (const QString& name : dir.entryList({ pattern }, QDir::Files)) {
                    const bool contrast =
                        name.contains(QStringLiteral("contrast"), Qt::CaseInsensitive);
                    const QRegularExpressionMatch m = kQualifier.match(name);
                    const int score = (contrast ? 0 : 100000)
                                    + (m.hasMatch() ? m.captured(1).toInt() : 0);
                    if (score > best) {
                        best = score;
                        bestFile = name;
                    }
                }
                if (!bestFile.isEmpty())
                    img = QImage(dir.filePath(bestFile));
            }

            if (img.isNull())
                return {};
            if (img.width() > 64 || img.height() > 64)
                img = img.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            return img;
        }

        // "Microsoft.WindowsCalculator" becomes "Windows Calculator".
        QString prettyIdentityName(const QString& identity) {
            QString name = identity;
            const int dot = name.indexOf(QLatin1Char('.'));
            if (dot > 0 && dot + 1 < name.size())
                name = name.mid(dot + 1);
            name.replace(QLatin1Char('.'), QLatin1Char(' '));

            QString out;
            out.reserve(name.size() + 8);
            for (int i = 0; i < name.size(); ++i) {
                const QChar c = name.at(i);
                if (i > 0 && c.isUpper()
                    && (name.at(i - 1).isLower() || name.at(i - 1).isDigit())) {
                    out.append(QLatin1Char(' '));
                }
                out.append(c);
            }
            return out.trimmed();
        }

        Architecture archFromPackageFullName(const QString& packageFullName) {
            // Format: Name_Version_Architecture_..._PublisherID
            const auto parts = packageFullName.split(QChar::fromLatin1('_'), Qt::SkipEmptyParts);
            if (parts.size() < 3) return Architecture::Unknown;
            const QString a = parts.at(2).toLower();
            if (a == QStringLiteral("x64"))     return Architecture::X64;
            if (a == QStringLiteral("x86"))     return Architecture::X86;
            if (a == QStringLiteral("arm64"))   return Architecture::Arm64;
            if (a == QStringLiteral("neutral")) return Architecture::Neutral;
            return Architecture::Unknown;
        }

        QVector<AppInfo> enumerateUwpPackages() {
            QVector<AppInfo> out;

            QProcess proc;
            proc.setProgram(pinnedPowerShell());
            proc.setArguments({
                QStringLiteral("-NoProfile"),
                QStringLiteral("-NonInteractive"),
                QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"),
                QStringLiteral("-Command"),
                QStringLiteral(
                    "Get-AppxPackage | "
                    "Where-Object { -not $_.IsFramework -and "
                    "-not $_.IsResourcePackage } | "
                    "Select-Object Name, Publisher, Version, PackageFullName, "
                    "InstallLocation, NonRemovable, SignatureKind | "
                    "ConvertTo-Json -Compress -Depth 3 | "
                    // Redirected output uses the legacy code page, so escape
                    // everything outside ASCII to keep the JSON valid.
                    "ForEach-Object { [regex]::Replace($_, '[^\\x00-\\x7F]', "
                    "{ param($m) '\\u{0:x4}' -f [int][char]$m.Value }) }"),
            });
            proc.setProcessChannelMode(QProcess::SeparateChannels);
            proc.start();
            if (!proc.waitForStarted(10000)) {
                qCWarning(catApps).noquote()
                    << "Get-AppxPackage failed to start:" << proc.errorString();
                return out;
            }
            if (!proc.waitForFinished(30000)) {
                proc.kill();
                proc.waitForFinished();
                qCWarning(catApps) << "Get-AppxPackage timed out";
                return out;
            }
            if (proc.exitCode() != 0) {
                qCWarning(catApps).noquote()
                    << "Get-AppxPackage failed (rc =" << proc.exitCode()
                    << "):" << QString::fromUtf8(proc.readAllStandardError());
                return out;
            }

            const QByteArray stdoutBytes = proc.readAllStandardOutput();
            if (stdoutBytes.isEmpty()) {
                return out;
            }

            QJsonParseError err;
            const QJsonDocument doc = QJsonDocument::fromJson(stdoutBytes, &err);
            if (err.error != QJsonParseError::NoError) {
                qCWarning(catApps).noquote()
                    << "Get-AppxPackage JSON parse:" << err.errorString();
                return out;
            }

            QJsonArray array;
            if (doc.isArray()) {
                array = doc.array();
            } else if (doc.isObject()) {
                array.append(doc.object());
            } else {
                return out;
            }

            out.reserve(array.size());
            for (const auto& v : array) {
                if (!v.isObject()) continue;
                const QJsonObject o = v.toObject();

                AppInfo info;
                info.type              = Type::UWP;
                info.packageName       = o.value(QStringLiteral("Name")).toString();
                info.publisher         = cleanPublisher(
                    o.value(QStringLiteral("Publisher")).toString());
                info.version           = o.value(QStringLiteral("Version")).toString();
                info.installLocation   = o.value(QStringLiteral("InstallLocation")).toString();
                info.packageFullName   = o.value(QStringLiteral("PackageFullName")).toString();
                info.nonRemovable     = o.value(QStringLiteral("NonRemovable")).toBool();
                info.architecture      = archFromPackageFullName(info.packageFullName);
                info.source            = Source::UwpPerUser;

                // An integer on Windows PowerShell and a word on newer hosts.
                const QJsonValue kind = o.value(QStringLiteral("SignatureKind"));
                info.systemComponent = kind.isString()
                    ? kind.toString().compare(QStringLiteral("System"),
                                              Qt::CaseInsensitive) == 0
                    : kind.toInt(-1) == 4;

                // No list shows system components, so skip their manifests.
                const ManifestFacts facts = info.systemComponent
                    ? ManifestFacts{} : manifestFacts(info.installLocation);
                info.displayName = !facts.displayName.isEmpty()
                    ? facts.displayName
                    : prettyIdentityName(info.packageName);

                if (info.publisher.isEmpty() || isGuid(info.publisher))
                    info.publisher = facts.publisher;

                info.iconImage = loadUwpLogo(info.installLocation, facts.logo);

                if (info.displayName.isEmpty()) continue;
                out.append(std::move(info));
            }

            return out;
        }
    }

    QString typeLabel(Type t) {
        switch (t) {
            case Type::Desktop: return QStringLiteral("Desktop");
            case Type::UWP:     return QStringLiteral("UWP");
        }
        return {};
    }

    QString sourceLabel(Source s) {
        switch (s) {
            case Source::PerMachine: return QStringLiteral("Per-machine");
            case Source::PerUser:    return QStringLiteral("Per-user");
            case Source::UwpPerUser: return QStringLiteral("UWP (current user)");
        }
        return {};
    }

    QString architectureLabel(Architecture a) {
        switch (a) {
            case Architecture::X64:     return QStringLiteral("64-bit");
            case Architecture::X86:     return QStringLiteral("32-bit");
            case Architecture::Arm64:   return QStringLiteral("ARM64");
            case Architecture::Neutral: return QStringLiteral("Neutral");
            case Architecture::Unknown: break;
        }
        return QStringLiteral("Unknown");
    }

    QString uwpRemoveResultLabel(UwpRemoveResult r) {
        switch (r) {
            case UwpRemoveResult::Ok:           return QStringLiteral("Removed.");
            case UwpRemoveResult::AccessDenied: return QStringLiteral(
                "Windows protects this app from removal. Apps that are part of "
                "Windows itself cannot be uninstalled.");
            case UwpRemoveResult::NotFound:     return QStringLiteral("Package not found.");
            case UwpRemoveResult::InvalidName:  return QStringLiteral(
                "Invalid package name (rejected before PowerShell call).");
            case UwpRemoveResult::OtherError:   return QStringLiteral(
                "Windows could not remove this app. Close it if it is running "
                "and try again.");
        }
        return {};
    }

    QImage defaultAppIcon() {
        SHSTOCKICONINFO stock{};
        stock.cbSize = sizeof(stock);
        if (SHGetStockIconInfo(SIID_APPLICATION, SHGSI_ICON | SHGSI_LARGEICON,
                               &stock) != S_OK
            || !stock.hIcon) {
            return {};
        }
        const QImage img = QImage::fromHICON(stock.hIcon);
        DestroyIcon(stock.hIcon);
        return img;
    }

    QVector<AppInfo> enumerate() {
        QVector<AppInfo> apps = enumerateDesktop();
        apps += enumerateStore();
        return apps;
    }

    QVector<AppInfo> enumerateStore() {
        return enumerateUwpPackages();
    }

    QVector<AppInfo> enumerateDesktop() {
        QVector<AppInfo> apps;

        enumerateUninstallHive(
            HKEY_LOCAL_MACHINE,
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
            Source::PerMachine,
            Architecture::X64,
            apps);

        enumerateUninstallHive(
            HKEY_LOCAL_MACHINE,
            L"Software\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
            Source::PerMachine,
            Architecture::X86,
            apps);

        enumerateUninstallHive(
            HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
            Source::PerUser,
            Architecture::Unknown,
            apps);

        return apps;
    }

    bool launchUninstall(const AppInfo& app) {
        if (app.type == Type::UWP) {
            return removeUwpPackage(app.packageFullName) == UwpRemoveResult::Ok;
        }

        const QString cmd = !app.uninstallString.isEmpty()
                                ? app.uninstallString
                                : app.quietUninstallString;
        if (cmd.isEmpty()) return false;

        // Native arguments, because QProcess would escape the quotes in a typical
        // uninstall string in a way cmd.exe doesn't understand.
        QProcess proc;
        proc.setProgram(system32Dir() + QStringLiteral("\\cmd.exe"));
        proc.setNativeArguments(QStringLiteral("/c \"%1\"").arg(cmd));
        return proc.startDetached();
    }

    UwpRemoveResult removeUwpPackage(const QString& packageFullName) {
        static const QRegularExpression kValid(QStringLiteral(R"(^[A-Za-z0-9._-]+$)"));
        if (!kValid.match(packageFullName).hasMatch()) {
            return UwpRemoveResult::InvalidName;
        }

        QProcess proc;
        proc.setProgram(pinnedPowerShell());
        proc.setArguments({
            QStringLiteral("-NoProfile"),
            QStringLiteral("-NonInteractive"),
            QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"),
            QStringLiteral("-Command"),
            QStringLiteral("Remove-AppxPackage -Package '%1'").arg(packageFullName),
        });
        proc.setProcessChannelMode(QProcess::SeparateChannels);
        proc.start();
        if (!proc.waitForStarted(10000)) {
            return UwpRemoveResult::OtherError;
        }
        if (!proc.waitForFinished(60000)) {
            proc.kill();
            proc.waitForFinished();
            return UwpRemoveResult::OtherError;
        }
        if (proc.exitCode() == 0) {
            return UwpRemoveResult::Ok;
        }

        const QString errStr = QString::fromUtf8(proc.readAllStandardError());
        qCWarning(catApps).noquote()
            << "Remove-AppxPackage" << packageFullName << ":" << errStr;

        if (errStr.contains(QStringLiteral("not found"), Qt::CaseInsensitive) ||
            errStr.contains(QStringLiteral("0x80073CF0"))) {
            return UwpRemoveResult::NotFound;
        }
        if (errStr.contains(QStringLiteral("denied"), Qt::CaseInsensitive) ||
            errStr.contains(QStringLiteral("administrator"), Qt::CaseInsensitive) ||
            errStr.contains(QStringLiteral("0x80070005"))) {
            return UwpRemoveResult::AccessDenied;
        }
        return UwpRemoveResult::OtherError;
    }

    bool openInstallLocation(const AppInfo& app) {
        if (app.installLocation.isEmpty()) return false;
        const QString clean = QDir::cleanPath(app.installLocation);
        if (!QFileInfo::exists(clean)) return false;
        return QDesktopServices::openUrl(QUrl::fromLocalFile(clean));
    }
}

namespace vth::win::apps
{
    QString categoryLabel(Category c) {
        switch (c) {
            case Category::User:      return QStringLiteral("User");
            case Category::Bloatware: return QStringLiteral("Bloat");
            case Category::System:    return QStringLiteral("System");
            case Category::Microsoft: return QStringLiteral("Microsoft");
        }
        return {};
    }
    Category classify(const AppInfo& app) {
        if (app.type == Type::UWP) {
            const QString identity = app.packageName.isEmpty()
                ? app.displayName : app.packageName;
            if (vth::win::bloatware::findByName(identity)) {
                return Category::Bloatware;
            }
            if (app.systemComponent) {
                return Category::System;
            }
            if (identity.startsWith(QStringLiteral("Microsoft."), Qt::CaseInsensitive)
             || identity.startsWith(QStringLiteral("MicrosoftCorporationII."), Qt::CaseInsensitive)
             || identity.startsWith(QStringLiteral("MicrosoftWindows."), Qt::CaseInsensitive)
             || app.publisher.contains(QStringLiteral("Microsoft Corporation"), Qt::CaseInsensitive)
             || app.publisher.contains(QStringLiteral("CN=Microsoft"), Qt::CaseInsensitive)) {
                return Category::Microsoft;
            }
            return Category::User;
        }
        const QString pub = app.publisher;
        if (pub.contains(QStringLiteral("Microsoft"),         Qt::CaseInsensitive)
         || pub.contains(QStringLiteral("Microsoft Corporation"), Qt::CaseInsensitive)) {
            return Category::Microsoft;
        }
        if (!app.displayName.isEmpty()
            && vth::win::bloatware::findByName(app.displayName)) {
            return Category::Bloatware;
        }
        return Category::User;
    }
}

