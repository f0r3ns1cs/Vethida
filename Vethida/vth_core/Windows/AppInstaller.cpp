#include "AppInstaller.h"

#include "../Http.h"
#include "../Logger.h"
#include "../Runtime/Paths.h"
#include "Authenticode.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QHostAddress>
#include <QHostInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutex>
#include <QProcess>
#include <QRegularExpression>
#include <QScopeGuard>
#include <QSettings>
#include <QStringList>
#include <QUrl>
#include <QUuid>
#include <QVersionNumber>

#include <string>

#include <windows.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <objbase.h>

#pragma comment(lib, "ole32.lib")

namespace vth::win::installer
{
    namespace
    {
        VTH_DEFINE_LOG_CATEGORY(catInstaller, "vth.installer")

        using Locate = DirectSource::Locate;
        using Kind   = DirectSource::Kind;

        const QRegularExpression& validId() {
            static const QRegularExpression re(QStringLiteral(R"(^[A-Za-z0-9._+-]+$)"));
            return re;
        }

        constexpr int kNotStarted = -100001;
        constexpr int kTimedOut   = -100002;

        // winget's own codes, as signed HRESULTs.
        constexpr int kWgNoPackageFound     = static_cast<int>(0x8A150014);
        constexpr int kWgUpdateNotApplicable = static_cast<int>(0x8A15002B);
        constexpr int kWgAlreadyInstalled    = static_cast<int>(0x8A150061);
        constexpr int kWgHashMismatch        = static_cast<int>(0x8A150011);
        constexpr int kWgSourceDataMissing   = static_cast<int>(0x8A15000F);
        constexpr int kWgSourceOpenFailed    = static_cast<int>(0x8A150044);
        constexpr int kWgDownloadFailed      = static_cast<int>(0x8A150008);
        constexpr int kWgInstallerFailed     = static_cast<int>(0x8A150010);
        // ERROR_NEEDS_REMEDIATION, after a source update that couldn't finish.
        constexpr int kNeedsRemediation     = static_cast<int>(0x80073CFC);

        QString systemDir() {
            wchar_t dir[MAX_PATH] = {};
            const UINT n = GetSystemDirectoryW(dir, MAX_PATH);
            if (n == 0 || n >= MAX_PATH)
                return QStringLiteral("C:\\Windows\\System32");
            return QString::fromWCharArray(dir, static_cast<int>(n));
        }

        struct Proc {
            int        code = kNotStarted;
            QByteArray output;
        };

        Proc runProgram(const QString& program, const QStringList& args, int timeoutMs) {
            Proc r;
            QProcess p;
            p.setProgram(program);
            p.setArguments(args);
            p.setProcessChannelMode(QProcess::MergedChannels);
            // Stdin stays closed, so nothing can hang waiting for input.
            p.start(QIODevice::ReadOnly);
            if (!p.waitForStarted(15000)) {
                qCWarning(catInstaller).noquote() << "could not start" << program << p.errorString();
                return r;
            }
            if (!p.waitForFinished(timeoutMs)) {
                p.kill();
                p.waitForFinished(3000);
                r.code = kTimedOut;
                return r;
            }
            r.output = p.readAll();
            r.code   = p.exitCode();
            return r;
        }

        int runPowerShell(const QString& script, int timeoutMs) {
            return runProgram(systemDir() + QStringLiteral("\\WindowsPowerShell\\v1.0\\powershell.exe"),
                              { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"),
                                QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"),
                                QStringLiteral("-Command"), script },
                              timeoutMs).code;
        }

        // -1 if the service isn't installed.
        int serviceStart(const wchar_t* name) {
            const std::wstring key =
                std::wstring(L"SYSTEM\\CurrentControlSet\\Services\\") + name;
            DWORD v = 0, size = sizeof(v);
            if (RegGetValueW(HKEY_LOCAL_MACHINE, key.c_str(), L"Start",
                             RRF_RT_REG_DWORD, nullptr, &v, &size) != ERROR_SUCCESS)
                return -1;
            return static_cast<int>(v);
        }

        // Without these services the Package Manager can't start or repair itself.
        QString deploymentBlocker() {
            static const wchar_t* const kServices[] = {
                L"AppXSvc", L"ClipSVC", L"StateRepository",
            };
            for (const wchar_t* s : kServices) {
                if (serviceStart(s) == SERVICE_DISABLED) {
                    return QStringLiteral(
                        "Windows' app deployment services are turned off (%1 is disabled)")
                        .arg(QString::fromWCharArray(s));
                }
            }
            return {};
        }

        // The package folder first, because this runs elevated and the per-user
        // alias lives in a folder the user can write to.
        QString wingetPath() {
            QDir dir(qEnvironmentVariable("ProgramFiles") + QStringLiteral("\\WindowsApps"));
            const QStringList packages = dir.entryList(
                { QStringLiteral("Microsoft.DesktopAppInstaller_*_*__8wekyb3d8bbwe") },
                QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::Reversed);
            for (const QString& pkg : packages) {
                const QString exe = dir.filePath(pkg + QStringLiteral("\\winget.exe"));
                if (QFileInfo::exists(exe))
                    return exe;
            }
            const QString base = qEnvironmentVariable("LOCALAPPDATA");
            if (!base.isEmpty()) {
                const QString alias = base + QStringLiteral("\\Microsoft\\WindowsApps\\winget.exe");
                if (QFileInfo::exists(alias))
                    return alias;
            }
            return {};
        }

        Proc runWinget(const QStringList& args, int timeoutMs) {
            const QString program = wingetPath();
            if (program.isEmpty())
                return {};
            return runProgram(program, args, timeoutMs);
        }

        struct WingetState {
            bool    usable = false;
            QString problem;
        };

        // Checked once per process. On a fresh install App Installer may not be
        // registered for this account yet, so register it if needed.
        WingetState wingetState() {
            static QMutex mu;
            static bool settled = false;
            static WingetState state;
            QMutexLocker lock(&mu);
            if (settled)
                return state;
            settled = true;

            if (wingetPath().isEmpty()) {
                const QString blocker = deploymentBlocker();
                if (blocker.isEmpty()) {
                    qCInfo(catInstaller) << "winget not registered; registering App Installer";
                    runPowerShell(QStringLiteral(
                        "$ErrorActionPreference='SilentlyContinue';"
                        "Add-AppxPackage -RegisterByFamilyName -MainPackage "
                        "Microsoft.DesktopAppInstaller_8wekyb3d8bbwe"), 120000);
                }
                if (wingetPath().isEmpty()) {
                    state.problem = blocker.isEmpty()
                        ? QStringLiteral("The Windows Package Manager is not installed on this PC")
                        : blocker;
                    return state;
                }
            }

            const Proc ver = runWinget({ QStringLiteral("--version") }, 30000);
            if (ver.code != 0) {
                state.problem = QStringLiteral("The Windows Package Manager is installed but would not start");
                return state;
            }
            qCInfo(catInstaller).noquote() << "winget" << QString::fromUtf8(ver.output).trimmed();
            state.usable = true;
            return state;
        }

        // Replaces winget's catalog package, once per process.
        bool repairSource() {
            static QMutex mu;
            static bool attempted = false;
            static bool repaired  = false;
            QMutexLocker lock(&mu);
            if (attempted)
                return repaired;
            attempted = true;
            if (!deploymentBlocker().isEmpty())
                return false;
            qCInfo(catInstaller) << "repairing winget source";
            runPowerShell(QStringLiteral(
                "$ErrorActionPreference='SilentlyContinue';"
                "Get-AppxPackage -AllUsers -Name Microsoft.Winget.Source | "
                "Remove-AppxPackage -AllUsers"), 120000);
            runWinget({ QStringLiteral("source"), QStringLiteral("reset"), QStringLiteral("--force"),
                        QStringLiteral("--disable-interactivity") }, 120000);
            const Proc upd = runWinget({ QStringLiteral("source"), QStringLiteral("update"),
                                         QStringLiteral("--disable-interactivity") }, 300000);
            repaired = upd.code == 0;
            return repaired;
        }

        bool isSourceFailure(int code) {
            return code == kNeedsRemediation || code == kWgSourceDataMissing
                || code == kWgSourceOpenFailed;
        }

        // Skips progress bars drawn with block characters.
        QString lastMeaningfulLine(const QByteArray& out) {
            const QStringList lines =
                QString::fromUtf8(out).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
            for (int i = lines.size() - 1; i >= 0; --i) {
                const QString line = lines.at(i).trimmed();
                for (const QChar c : line)
                    if (c.isLetter())
                        return line;
            }
            return {};
        }

        // Resolving to nowhere means a hosts-file entry or a DNS filter.
        bool hostBlocked(const QString& host) {
            const QHostInfo info = QHostInfo::fromName(host);
            if (info.error() != QHostInfo::NoError)
                return false;
            const auto addrs = info.addresses();
            if (addrs.isEmpty())
                return false;
            for (const QHostAddress& a : addrs)
                if (!a.isNull() && !a.isLoopback() && a != QHostAddress(QHostAddress::AnyIPv4)
                    && a != QHostAddress(QHostAddress::AnyIPv6))
                    return false;
            return true;
        }

        QString explainNetwork(const QString& url, const QString& transportError) {
            const QString host = QUrl(url).host();
            if (!host.isEmpty() && hostBlocked(host))
                return QStringLiteral("%1 is blocked on this PC, by a hosts-file entry or a DNS "
                                      "filter. Allow it, then try again.").arg(host);
            if (!host.isEmpty() && QHostInfo::fromName(host).error() != QHostInfo::NoError)
                return QStringLiteral("This PC could not look up %1. Check the internet "
                                      "connection, then try again.").arg(host);
            return QStringLiteral("The download from %1 did not complete (%2). Check the "
                                  "internet connection, then try again.")
                .arg(host.isEmpty() ? url : host, transportError);
        }

        // Empty if the folder couldn't be created.
        QString newDownloadDir() {
            const QString dir = vth::paths::protectedData() + QStringLiteral("/downloads/")
                              + QUuid::createUuid().toString(QUuid::Id128);
            return QDir().mkpath(dir) ? dir : QString();
        }

        // Returns empty on success, otherwise the reason. hash is optional.
        QString downloadTo(const QString& url, const QString& file, const StatusFn& status,
                           QCryptographicHash* hash = nullptr) {
            QFile sink(file);
            if (!sink.open(QIODevice::WriteOnly))
                return QStringLiteral("Vethida could not save the download. Check that your "
                                      "drive is not full.");
            int lastPct = -1;
            const auto dl = vth::http::download(
                url,
                [&sink, hash](const char* data, qint64 size) {
                    if (hash)
                        hash->addData(QByteArray::fromRawData(data, static_cast<int>(size)));
                    return sink.write(data, size) == size;
                },
                {},
                [&](qint64 got, qint64 total) {
                    if (total <= 0 || !status) return;
                    const int pct = static_cast<int>(got * 100 / total);
                    if (pct != lastPct && pct % 5 == 0) {
                        lastPct = pct;
                        status(QStringLiteral("Downloading %1%").arg(pct));
                    }
                });
            sink.close();
            if (dl.ok)
                return {};
            return dl.sinkRefused
                ? QStringLiteral("The download could not be saved. Check that your drive is not full.")
                : explainNetwork(url, dl.errorString);
        }

        QString msiServiceDisabledMessage(const QString& name) {
            return QStringLiteral("The Windows Installer service is disabled, so %1 cannot "
                                  "be installed. Run \"Restore Windows Installer\" in "
                                  "System > Fixes, then try again.").arg(name);
        }

        struct Resolved {
            QString url;
            QString fileName;
            QString problem;
        };

        QString fileNameFrom(const QString& url, Kind kind) {
            QString name = QUrl(url).fileName();
            const QString ext = kind == Kind::Msi ? QStringLiteral(".msi")
                              : kind == Kind::Zip ? QStringLiteral(".zip")
                                                  : QStringLiteral(".exe");
            if (name.isEmpty() || !name.endsWith(ext, Qt::CaseInsensitive))
                name = QStringLiteral("setup") + ext;
            return name;
        }

        Resolved resolve(const DirectSource& src) {
            Resolved r;
            switch (src.locate) {
            case Locate::Fixed:
                r.url = src.url;
                break;

            case Locate::GitHubLatest: {
                const QString api = QStringLiteral("https://api.github.com/repos/%1/releases/latest").arg(src.url);
                const auto resp = vth::http::get(api, { { QStringLiteral("Accept"),
                                                          QStringLiteral("application/vnd.github+json") } });
                if (!resp.ok) {
                    r.problem = explainNetwork(api, resp.errorString);
                    return r;
                }
                const QRegularExpression re(src.pattern);
                const QJsonArray assets = QJsonDocument::fromJson(resp.body).object()
                                              .value(QStringLiteral("assets")).toArray();
                for (const QJsonValue& a : assets) {
                    const QJsonObject o = a.toObject();
                    if (re.match(o.value(QStringLiteral("name")).toString()).hasMatch()) {
                        r.url = o.value(QStringLiteral("browser_download_url")).toString();
                        break;
                    }
                }
                break;
            }

            case Locate::PageLink: {
                const auto resp = vth::http::get(src.url);
                if (!resp.ok) {
                    r.problem = explainNetwork(src.url, resp.errorString);
                    return r;
                }
                const auto m = QRegularExpression(src.pattern).match(QString::fromUtf8(resp.body));
                if (m.hasMatch())
                    r.url = QUrl(src.url).resolved(QUrl(m.captured(1))).toString();
                break;
            }

            case Locate::NodeLts: {
                const QString index = QStringLiteral("https://nodejs.org/dist/index.json");
                const auto resp = vth::http::get(index);
                if (!resp.ok) {
                    r.problem = explainNetwork(index, resp.errorString);
                    return r;
                }
                for (const QJsonValue& v : QJsonDocument::fromJson(resp.body).array()) {
                    const QJsonObject o = v.toObject();
                    if (o.value(QStringLiteral("lts")).isString()) {
                        const QString ver = o.value(QStringLiteral("version")).toString();
                        r.url = QStringLiteral("https://nodejs.org/dist/%1/node-%1-x64.msi").arg(ver);
                        break;
                    }
                }
                break;
            }
            }
            if (r.url.isEmpty() && r.problem.isEmpty())
                r.problem = QStringLiteral("The publisher's download page has changed, so Vethida "
                                           "could not find the current installer");
            r.fileName = fileNameFrom(r.url, src.kind);
            return r;
        }

        // A valid signature from anyone else is a different program.
        bool signedBy(const QString& file, const QString& expected, QString* signerOut) {
            const QString name = vth::win::authenticode::signer(file);
            if (signerOut)
                *signerOut = name;
            return !name.isEmpty() && name.contains(expected, Qt::CaseInsensitive);
        }

        QString programFiles() {
            const QString pf = qEnvironmentVariable("ProgramW6432");
            return pf.isEmpty() ? qEnvironmentVariable("ProgramFiles") : pf;
        }

        bool makeStartMenuShortcut(const QString& target, const QString& name) {
            wchar_t* folder = nullptr;
            if (FAILED(SHGetKnownFolderPath(FOLDERID_CommonPrograms, 0, nullptr, &folder)))
                return false;
            const QString lnk = QString::fromWCharArray(folder) + QLatin1Char('\\') + name
                              + QStringLiteral(".lnk");
            CoTaskMemFree(folder);

            const HRESULT init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
            bool ok = false;
            IShellLinkW* link = nullptr;
            if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                                           IID_PPV_ARGS(&link)))) {
                const std::wstring t = QDir::toNativeSeparators(target).toStdWString();
                link->SetPath(t.c_str());
                const std::wstring wd = QDir::toNativeSeparators(QFileInfo(target).absolutePath()).toStdWString();
                link->SetWorkingDirectory(wd.c_str());
                IPersistFile* file = nullptr;
                if (SUCCEEDED(link->QueryInterface(IID_PPV_ARGS(&file)))) {
                    const std::wstring l = lnk.toStdWString();
                    ok = SUCCEEDED(file->Save(l.c_str(), TRUE));
                    file->Release();
                }
                link->Release();
            }
            if (SUCCEEDED(init))
                CoUninitialize();
            return ok;
        }

        // Most setup programs pass through Windows Installer's codes.
        QString explainInstallerExit(int code, const QString& name) {
            switch (code) {
            case 1602: return QStringLiteral("The %1 installer was canceled.").arg(name);
            case 1618: return QStringLiteral("Another installation is already running. Wait for it "
                                             "to finish, then try %1 again.").arg(name);
            case 1625: return QStringLiteral("A Windows policy on this PC blocks installing %1.").arg(name);
            case 1633: return QStringLiteral("%1 does not support this version of Windows.").arg(name);
            case 1638: return QStringLiteral("A different version of %1 is already installed. Remove "
                                             "it from Installed apps first.").arg(name);
            case 1603: return QStringLiteral("The %1 installer stopped with a general error. A pending "
                                             "restart is the usual cause; restart and try again.").arg(name);
            case 5:    return QStringLiteral("Windows refused the %1 installer access. Security "
                                             "software may be blocking it.").arg(name);
            case kTimedOut:
                return QStringLiteral("The %1 installer stopped responding and was closed. "
                                      "Restart the PC and try again.").arg(name);
            case kNotStarted:
                return QStringLiteral("Windows would not start the %1 installer. Security software "
                                      "may be blocking it.").arg(name);
            default:
                return QStringLiteral("The %1 installer stopped with error %2.").arg(name).arg(code);
            }
        }

        InstallOutcome outcomeFromExit(const Proc& run, const PackageDef& def) {
            InstallOutcome out;
            // 3010 and 1641 mean it worked and needs a restart.
            if (run.code == 0) {
                out.result = InstallResult::Ok;
                out.message = QStringLiteral("Installed.");
            } else if (run.code == 3010 || run.code == 1641) {
                out.result = InstallResult::Ok;
                out.message = QStringLiteral("Installed. Restart the PC to finish.");
            } else {
                qCWarning(catInstaller).noquote() << def.id << "installer exit" << run.code
                                                  << QString::fromUtf8(run.output).left(500);
                out.message = explainInstallerExit(run.code, def.name);
            }
            return out;
        }

        InstallOutcome installDirect(const PackageDef& def, const StatusFn& status) {
            const DirectSource& src = *def.direct;
            InstallOutcome out;
            const auto say = [&status](const QString& s) { if (status) status(s); };

            if (src.kind == Kind::Msi && serviceStart(L"msiserver") == SERVICE_DISABLED) {
                out.message = msiServiceDisabledMessage(def.name);
                return out;
            }

            say(QStringLiteral("Finding the latest version"));
            const Resolved where = resolve(src);
            if (!where.problem.isEmpty()) {
                out.message = where.problem + QLatin1Char('.');
                return out;
            }

            const QString dir = newDownloadDir();
            if (dir.isEmpty()) {
                out.message = QStringLiteral("Vethida could not create a folder for the download. "
                                             "Check that your drive is not full.");
                return out;
            }
            const auto cleanup = qScopeGuard([dir] { QDir(dir).removeRecursively(); });
            const QString file = dir + QLatin1Char('/') + where.fileName;

            const QString failed = downloadTo(where.url, file, status);
            if (!failed.isEmpty()) {
                out.message = failed;
                return out;
            }

            if (!src.signer.isEmpty()) {
                say(QStringLiteral("Checking the publisher's signature"));
                QString signer;
                if (!signedBy(file, src.signer, &signer)) {
                    qCWarning(catInstaller).noquote() << def.id << "signature check failed; signer:" << signer
                                                      << "url:" << where.url;
                    out.message = QStringLiteral("The downloaded %1 installer is not signed by %2, so "
                                                 "Vethida did not run it.").arg(def.name, src.signer);
                    return out;
                }
            }

            say(QStringLiteral("Installing"));
            Proc run;
            switch (src.kind) {
            case Kind::Exe:
                run = runProgram(file, src.args, 15 * 60 * 1000);
                break;
            case Kind::Msi: {
                QStringList args = { QStringLiteral("/i"), QDir::toNativeSeparators(file),
                                     QStringLiteral("/qn"), QStringLiteral("/norestart") };
                args += src.args;
                run = runProgram(systemDir() + QStringLiteral("\\msiexec.exe"), args, 15 * 60 * 1000);
                break;
            }
            case Kind::Zip: {
                const QString dest = programFiles() + QLatin1Char('\\') + src.zipFolder;
                QDir().mkpath(dest);
                run = runProgram(systemDir() + QStringLiteral("\\tar.exe"),
                                 { QStringLiteral("-xf"), QDir::toNativeSeparators(file),
                                   QStringLiteral("-C"), dest }, 5 * 60 * 1000);
                if (run.code == 0) {
                    const QString exe = dest + QLatin1Char('\\') + src.zipLaunch;
                    if (!QFileInfo::exists(exe)) {
                        out.message = QStringLiteral("The %1 download did not contain the expected "
                                                     "program.").arg(def.name);
                        return out;
                    }
                    makeStartMenuShortcut(exe, def.name);
                }
                break;
            }
            }

            return outcomeFromExit(run, def);
        }

        // One manifest's values, flattened: "InstallerType", "InstallerSwitches.Silent".
        using ManifestMap = QHash<QString, QString>;

        struct ParsedManifest {
            ManifestMap              top;
            QVector<ManifestMap>     installers;
        };

        QString unquote(QString v) {
            v = v.trimmed();
            if (v.size() >= 2 && ((v.startsWith(QLatin1Char('"')) && v.endsWith(QLatin1Char('"')))
                                  || (v.startsWith(QLatin1Char('\'')) && v.endsWith(QLatin1Char('\'')))))
                v = v.mid(1, v.size() - 2);
            return v;
        }

        // The subset of YAML winget manifests use: maps by indentation, and
        // lists of maps under "Installers:" and "NestedInstallerFiles:".
        ParsedManifest parseManifest(const QByteArray& yaml) {
            ParsedManifest m;
            QString topKey;
            bool inInstallers = false;
            int dashIndent = -1;
            QString propKey;
            const auto put = [](ManifestMap& map, const QString& k, const QString& v) {
                if (!v.isEmpty() && !map.contains(k))
                    map.insert(k, v);
            };
            for (QString line : QString::fromUtf8(yaml).split(QLatin1Char('\n'))) {
                if (line.endsWith(QLatin1Char('\r')))
                    line.chop(1);
                const QString trimmed = line.trimmed();
                if (trimmed.isEmpty() || trimmed.startsWith(QLatin1Char('#')))
                    continue;
                int indent = 0;
                while (indent < line.size() && line.at(indent) == QLatin1Char(' '))
                    ++indent;
                QString t = trimmed;
                const bool dash = t.startsWith(QLatin1String("- "));
                const int itemIndent = indent;
                if (dash) {
                    t = t.mid(2).trimmed();
                    indent += 2;
                }
                const int colon = t.indexOf(QLatin1Char(':'));
                if (colon <= 0)
                    continue;
                const QString key = t.left(colon).trimmed();
                const QString value = unquote(t.mid(colon + 1));

                if (indent == 0) {
                    topKey = key;
                    inInstallers = key == QLatin1String("Installers");
                    put(m.top, key, value);
                    continue;
                }
                if (!inInstallers) {
                    put(m.top, topKey + QLatin1Char('.') + key, value);
                    continue;
                }
                if (dash && (dashIndent < 0 || itemIndent == dashIndent)) {
                    dashIndent = itemIndent;
                    m.installers.append(ManifestMap());
                }
                if (m.installers.isEmpty())
                    continue;
                ManifestMap& cur = m.installers.last();
                if (indent == dashIndent + 2) {
                    propKey = key;
                    put(cur, key, value);
                } else if (indent > dashIndent + 2) {
                    put(cur, propKey + QLatin1Char('.') + key, value);
                }
            }
            return m;
        }

        // Falls back to the manifest's default.
        QString field(const ParsedManifest& m, const ManifestMap& inst, const QString& key) {
            const QString v = inst.value(key);
            return v.isEmpty() ? m.top.value(key) : v;
        }

        bool supportedType(const QString& type, const QString& nested) {
            static const QStringList kRunnable = {
                QStringLiteral("msi"), QStringLiteral("wix"), QStringLiteral("inno"),
                QStringLiteral("nullsoft"), QStringLiteral("burn"), QStringLiteral("exe"),
                QStringLiteral("portable"),
            };
            if (type == QLatin1String("zip"))
                return kRunnable.contains(nested);
            return kRunnable.contains(type);
        }

        // Prefers 64-bit, machine-wide, and English or language-neutral.
        int pickInstaller(const ParsedManifest& m) {
            int best = -1, bestScore = -1;
            for (int i = 0; i < m.installers.size(); ++i) {
                const ManifestMap& in = m.installers.at(i);
                const QString arch = field(m, in, QStringLiteral("Architecture")).toLower();
                const QString type = field(m, in, QStringLiteral("InstallerType")).toLower();
                const QString nested = field(m, in, QStringLiteral("NestedInstallerType")).toLower();
                if (!supportedType(type, nested) || field(m, in, QStringLiteral("InstallerUrl")).isEmpty())
                    continue;
                int score = 0;
                if (arch == QLatin1String("x64"))          score += 300;
                else if (arch == QLatin1String("neutral")) score += 200;
                else if (arch == QLatin1String("x86"))     score += 100;
                else continue;
                const QString scope = field(m, in, QStringLiteral("Scope")).toLower();
                if (scope == QLatin1String("machine"))   score += 20;
                else if (scope.isEmpty())                score += 10;
                const QString locale = field(m, in, QStringLiteral("InstallerLocale"));
                if (locale.isEmpty() || locale.startsWith(QLatin1String("en"), Qt::CaseInsensitive))
                    score += 5;
                if (score > bestScore) { bestScore = score; best = i; }
            }
            return best;
        }

        // winget's defaults when a manifest doesn't name its own.
        QStringList silentArgs(const QString& type) {
            if (type == QLatin1String("inno"))
                return { QStringLiteral("/VERYSILENT"), QStringLiteral("/SUPPRESSMSGBOXES"),
                         QStringLiteral("/NORESTART"), QStringLiteral("/SP-") };
            if (type == QLatin1String("nullsoft"))
                return { QStringLiteral("/S") };
            if (type == QLatin1String("burn"))
                return { QStringLiteral("/quiet"), QStringLiteral("/norestart") };
            return {};
        }

        QString newestManifestVersion(const QString& id, QString* problem) {
            const QString path = QStringLiteral("manifests/%1/%2")
                .arg(id.left(1).toLower(), QString(id).replace(QLatin1Char('.'), QLatin1Char('/')));
            const QString api = QStringLiteral("https://api.github.com/repos/microsoft/winget-pkgs/contents/") + path;
            const auto resp = vth::http::get(api, { { QStringLiteral("Accept"),
                                                      QStringLiteral("application/vnd.github+json") } });
            if (!resp.ok) {
                *problem = resp.statusCode == 403 || resp.statusCode == 429
                    ? QStringLiteral("GitHub is limiting how often this PC can read the app catalog. "
                                     "Try again in an hour")
                    : resp.statusCode == 404
                        ? QStringLiteral("The app catalog no longer lists this app")
                        : explainNetwork(api, resp.errorString);
                return {};
            }
            QVersionNumber best;
            QString bestName;
            for (const QJsonValue& v : QJsonDocument::fromJson(resp.body).array()) {
                const QJsonObject o = v.toObject();
                if (o.value(QStringLiteral("type")).toString() != QLatin1String("dir"))
                    continue;
                const QString name = o.value(QStringLiteral("name")).toString();
                const QVersionNumber ver = QVersionNumber::fromString(name);
                // Other subfolders are nested package ids, such as "Preview".
                if (ver.isNull())
                    continue;
                if (bestName.isEmpty() || QVersionNumber::compare(ver, best) > 0) {
                    best = ver;
                    bestName = name;
                }
            }
            if (bestName.isEmpty())
                *problem = QStringLiteral("The app catalog has no installable version of this app");
            return bestName.isEmpty() ? QString() : path + QLatin1Char('/') + bestName;
        }

        InstallOutcome installFromManifest(const PackageDef& def, const StatusFn& status,
                                           bool* reachedInstaller) {
            InstallOutcome out;
            *reachedInstaller = false;
            const auto say = [&status](const QString& s) { if (status) status(s); };

            say(QStringLiteral("Finding the latest version"));
            QString problem;
            const QString folder = newestManifestVersion(def.id, &problem);
            if (folder.isEmpty()) {
                out.message = problem + QLatin1Char('.');
                return out;
            }
            const QString raw = QStringLiteral("https://raw.githubusercontent.com/microsoft/winget-pkgs/master/")
                              + folder + QLatin1Char('/');
            auto resp = vth::http::get(raw + def.id + QStringLiteral(".installer.yaml"));
            if (!resp.ok)   // an older single-file manifest
                resp = vth::http::get(raw + def.id + QStringLiteral(".yaml"));
            if (!resp.ok) {
                out.message = explainNetwork(raw, resp.errorString) + QLatin1Char('.');
                return out;
            }

            const ParsedManifest m = parseManifest(resp.body);
            const int pick = pickInstaller(m);
            if (pick < 0) {
                out.message = QStringLiteral("%1 only offers installers Vethida cannot run without the "
                                             "Windows Package Manager. Install App Installer from the "
                                             "Microsoft Store, then try again.").arg(def.name);
                return out;
            }
            const ManifestMap& in = m.installers.at(pick);
            const QString url    = field(m, in, QStringLiteral("InstallerUrl"));
            const QString sha    = field(m, in, QStringLiteral("InstallerSha256")).toLower();
            const QString type   = field(m, in, QStringLiteral("InstallerType")).toLower();
            const QString nested = field(m, in, QStringLiteral("NestedInstallerType")).toLower();
            const QString nestedPath = field(m, in, QStringLiteral("NestedInstallerFiles.RelativeFilePath"));
            const QString runType = type == QLatin1String("zip") ? nested : type;
            if (sha.size() != 64 || !QUrl(url).scheme().startsWith(QLatin1String("https"))) {
                out.message = QStringLiteral("The catalog entry for %1 has no checksum, so Vethida did "
                                             "not download it.").arg(def.name);
                return out;
            }
            if ((runType == QLatin1String("msi") || runType == QLatin1String("wix"))
                && serviceStart(L"msiserver") == SERVICE_DISABLED) {
                out.message = msiServiceDisabledMessage(def.name);
                return out;
            }
            QStringList args;
            const QString silent = field(m, in, QStringLiteral("InstallerSwitches.Silent"));
            if (!silent.isEmpty())
                args = QProcess::splitCommand(silent);
            else
                args = silentArgs(runType);
            if (runType == QLatin1String("exe") && args.isEmpty()) {
                out.message = QStringLiteral("%1's installer has no silent mode Vethida can use. Install "
                                             "App Installer from the Microsoft Store, then try again.")
                                  .arg(def.name);
                return out;
            }
            args += QProcess::splitCommand(field(m, in, QStringLiteral("InstallerSwitches.Custom")));

            const QString dir = newDownloadDir();
            if (dir.isEmpty()) {
                out.message = QStringLiteral("Vethida could not create a folder for the download. "
                                             "Check that your drive is not full.");
                return out;
            }
            const auto cleanup = qScopeGuard([dir] { QDir(dir).removeRecursively(); });
            const QString ext = type == QLatin1String("zip") ? QStringLiteral(".zip")
                              : (type == QLatin1String("msi") || type == QLatin1String("wix"))
                                  ? QStringLiteral(".msi") : QStringLiteral(".exe");
            const QString file = dir + QStringLiteral("/installer") + ext;

            QCryptographicHash hash(QCryptographicHash::Sha256);
            const QString failed = downloadTo(url, file, status, &hash);
            if (!failed.isEmpty()) {
                out.message = failed;
                return out;
            }
            say(QStringLiteral("Checking the download"));
            if (QString::fromLatin1(hash.result().toHex()) != sha) {
                qCWarning(catInstaller).noquote() << def.id << "manifest hash mismatch for" << url;
                out.message = QStringLiteral("The %1 download did not match the catalog's checksum, so "
                                             "it was not installed. The publisher has probably just "
                                             "released an update; try again later.").arg(def.name);
                return out;
            }

            QString target = file;
            const QString appDir = programFiles() + QLatin1Char('\\') + def.name;
            if (type == QLatin1String("zip")) {
                const QString unpack = runType == QLatin1String("portable") ? appDir
                                                                            : dir + QStringLiteral("/unpacked");
                QDir().mkpath(unpack);
                const Proc x = runProgram(systemDir() + QStringLiteral("\\tar.exe"),
                                          { QStringLiteral("-xf"), QDir::toNativeSeparators(file),
                                            QStringLiteral("-C"), QDir::toNativeSeparators(unpack) },
                                          5 * 60 * 1000);
                if (x.code != 0 || nestedPath.isEmpty()
                    || !QFileInfo::exists(unpack + QLatin1Char('/') + nestedPath)) {
                    out.message = QStringLiteral("The %1 download could not be unpacked.").arg(def.name);
                    return out;
                }
                target = unpack + QLatin1Char('/') + nestedPath;
            }

            say(QStringLiteral("Installing"));
            *reachedInstaller = true;
            Proc run;
            if (runType == QLatin1String("portable")) {
                QString exe = target;
                if (type != QLatin1String("zip")) {
                    QDir().mkpath(appDir);
                    exe = appDir + QLatin1Char('\\') + def.name + QStringLiteral(".exe");
                    QFile::remove(exe);
                    if (!QFile::copy(target, exe)) {
                        out.message = QStringLiteral("Vethida could not copy %1 into Program Files.")
                                          .arg(def.name);
                        return out;
                    }
                }
                makeStartMenuShortcut(exe, def.name);
                run.code = 0;
            } else if (runType == QLatin1String("msi") || runType == QLatin1String("wix")) {
                QStringList msiArgs = { QStringLiteral("/i"), QDir::toNativeSeparators(target) };
                if (silent.isEmpty())
                    msiArgs += { QStringLiteral("/qn"), QStringLiteral("/norestart") };
                msiArgs += args;
                run = runProgram(systemDir() + QStringLiteral("\\msiexec.exe"), msiArgs, 15 * 60 * 1000);
            } else {
                run = runProgram(target, args, 15 * 60 * 1000);
            }

            return outcomeFromExit(run, def);
        }

        QString explainWinget(int code, const QByteArray& output, const QString& name) {
            if (code == kWgNoPackageFound)
                return QStringLiteral("%1 is no longer in the Windows Package Manager catalog.").arg(name);
            if (code == kWgHashMismatch)
                return QStringLiteral("The %1 download did not match the catalog's checksum, so it was "
                                      "not installed. The publisher has probably just released an "
                                      "update; try again later.").arg(name);
            if (code == kWgDownloadFailed)
                return explainNetwork(QStringLiteral("https://cdn.winget.microsoft.com"),
                                      lastMeaningfulLine(output));
            if (isSourceFailure(code))
                return QStringLiteral("The Windows Package Manager's catalog is damaged and could not "
                                      "be repaired. Update App Installer from the Microsoft Store, then "
                                      "try again.");
            if (code == kTimedOut)
                return QStringLiteral("Installing %1 took longer than 15 minutes and was stopped.").arg(name);
            const QString said = lastMeaningfulLine(output);
            if (!said.isEmpty())
                return said;
            return code == kWgInstallerFailed
                ? QStringLiteral("The %1 installer reported an error.").arg(name)
                : QStringLiteral("%1 could not be installed.").arg(name);
        }

        const PackageDef* findPackage(const QString& id) {
            for (const PackageDef& p : catalog())
                if (p.id == id)
                    return &p;
            return nullptr;
        }

        QStringList installedDisplayNames() {
            QStringList names;
            const auto collect = [&names](const QString& root, QSettings::Format fmt) {
                QSettings reg(root, fmt);
                for (const QString& key : reg.childGroups()) {
                    reg.beginGroup(key);
                    const QString n = reg.value(QStringLiteral("DisplayName")).toString();
                    if (!n.isEmpty())
                        names << n;
                    reg.endGroup();
                }
            };
            const QString path = QStringLiteral("\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall");
            collect(QStringLiteral("HKEY_LOCAL_MACHINE") + path, QSettings::Registry64Format);
            collect(QStringLiteral("HKEY_LOCAL_MACHINE") + path, QSettings::Registry32Format);
            collect(QStringLiteral("HKEY_CURRENT_USER") + path, QSettings::NativeFormat);
            return names;
        }

        bool installedLocally(const PackageDef& def, const QStringList& displayNames) {
            if (def.direct && def.direct->kind == Kind::Zip
                && QFileInfo::exists(programFiles() + QLatin1Char('\\') + def.direct->zipFolder
                                     + QLatin1Char('\\') + def.direct->zipLaunch))
                return true;
            if (def.installedName.isEmpty())
                return false;
            const QRegularExpression re(def.installedName, QRegularExpression::CaseInsensitiveOption);
            for (const QString& n : displayNames)
                if (re.match(n).hasMatch())
                    return true;
            return false;
        }

        DirectSource fixed(const QString& url, Kind kind, QStringList args, const QString& signer) {
            DirectSource d;
            d.locate = Locate::Fixed;
            d.url = url;
            d.kind = kind;
            d.args = std::move(args);
            d.signer = signer;
            return d;
        }

        DirectSource github(const QString& repo, const QString& asset, Kind kind,
                            QStringList args, const QString& signer) {
            DirectSource d = fixed(repo, kind, std::move(args), signer);
            d.locate  = Locate::GitHubLatest;
            d.pattern = asset;
            return d;
        }

        DirectSource page(const QString& url, const QString& link, Kind kind,
                          QStringList args, const QString& signer) {
            DirectSource d = fixed(url, kind, std::move(args), signer);
            d.locate  = Locate::PageLink;
            d.pattern = link;
            return d;
        }

        DirectSource zip(const QString& url, const QString& folder, const QString& launch,
                         const QString& signer) {
            DirectSource d = fixed(url, Kind::Zip, {}, signer);
            d.zipFolder = folder;
            d.zipLaunch = launch;
            return d;
        }
    }

    // Every id is an exact winget id. A direct source is listed where the publisher
    // offers a stable download.
    const QVector<PackageDef>& catalog() {
        static const QStringList kInno = { QStringLiteral("/VERYSILENT"), QStringLiteral("/NORESTART"),
                                           QStringLiteral("/SUPPRESSMSGBOXES"), QStringLiteral("/SP-") };
        static const QStringList kNsis = { QStringLiteral("/S") };
        static const QStringList kMsQuiet = { QStringLiteral("/install"), QStringLiteral("/quiet"),
                                              QStringLiteral("/norestart") };
        static const QString kMicrosoft = QStringLiteral("Microsoft Corporation");

        static const QVector<PackageDef> c = {
            { QStringLiteral("Microsoft.Sysinternals.Autoruns"), QStringLiteral("Autoruns"),
              QStringLiteral("Diagnostics & repair"),
              QStringLiteral("Microsoft's full autostart inspector: every task, service, driver and shell hook that runs at boot."),
              QString(),
              zip(QStringLiteral("https://download.sysinternals.com/files/Autoruns.zip"),
                  QStringLiteral("Sysinternals\\Autoruns"), QStringLiteral("Autoruns64.exe"), kMicrosoft) },
            { QStringLiteral("Microsoft.Sysinternals.ProcessExplorer"), QStringLiteral("Process Explorer"),
              QStringLiteral("Diagnostics & repair"),
              QStringLiteral("Handle and DLL search, per-process I/O, and signature verification."),
              QString(),
              zip(QStringLiteral("https://download.sysinternals.com/files/ProcessExplorer.zip"),
                  QStringLiteral("Sysinternals\\Process Explorer"), QStringLiteral("procexp64.exe"), kMicrosoft) },
            { QStringLiteral("REALiX.HWiNFO"), QStringLiteral("HWiNFO"),
              QStringLiteral("Diagnostics & repair"),
              QStringLiteral("Every sensor this machine exposes - temperatures, clocks, power draw, fan speeds - in one place."),
              QStringLiteral("^HWiNFO"), std::nullopt },
            { QStringLiteral("CPUID.CPU-Z"), QStringLiteral("CPU-Z"),
              QStringLiteral("Diagnostics & repair"),
              QStringLiteral("Identifies the processor, memory and mainboard, including the real memory timings."),
              QStringLiteral("CPU-Z"), std::nullopt },
            { QStringLiteral("TechPowerUp.GPU-Z"), QStringLiteral("GPU-Z"),
              QStringLiteral("Diagnostics & repair"),
              QStringLiteral("Graphics adapter identification, BIOS version, and live clock and temperature readings."),
              QStringLiteral("GPU-Z"), std::nullopt },
            { QStringLiteral("CrystalDewWorld.CrystalDiskInfo"), QStringLiteral("CrystalDiskInfo"),
              QStringLiteral("Diagnostics & repair"),
              QStringLiteral("Reads drive health from every disk so a failing one is found before it takes anything with it."),
              QStringLiteral("^CrystalDiskInfo"), std::nullopt },
            { QStringLiteral("Wagnardsoft.DisplayDriverUninstaller"), QStringLiteral("Display Driver Uninstaller (DDU)"),
              QStringLiteral("Diagnostics & repair"),
              QStringLiteral("Removes every trace of a graphics driver before a clean reinstall."),
              QStringLiteral("Display Driver Uninstaller"), std::nullopt },
            { QStringLiteral("RevoUninstaller.RevoUninstaller"), QStringLiteral("Revo Uninstaller"),
              QStringLiteral("Diagnostics & repair"),
              QStringLiteral("Runs an application's own uninstaller, then sweeps up the files it left behind."),
              QStringLiteral("^Revo Uninstaller"),
              fixed(QStringLiteral("https://download.revouninstaller.com/download/revosetup.exe"),
                    Kind::Exe, kInno, QStringLiteral("VS Revo Group")) },

            { QStringLiteral("Guru3D.Afterburner"), QStringLiteral("MSI Afterburner"),
              QStringLiteral("Graphics & desktop"),
              QStringLiteral("Graphics card fan curves, clocks and power limits, plus the on-screen display for frame times."),
              QStringLiteral("^MSI Afterburner"), std::nullopt },
            { QStringLiteral("ToastyX.CustomResolutionUtility"), QStringLiteral("Custom Resolution Utility (CRU)"),
              QStringLiteral("Graphics & desktop"),
              QStringLiteral("Adds a refresh rate or resolution your monitor supports but Windows does not offer."),
              QString(), std::nullopt },
            { QStringLiteral("Open-Shell.Open-Shell-Menu"), QStringLiteral("Open-Shell"),
              QStringLiteral("Graphics & desktop"),
              QStringLiteral("Replaces the Start menu with a classic one, and puts back the Explorer toolbars Windows 11 removed."),
              QStringLiteral("^Open-Shell"),
              github(QStringLiteral("Open-Shell/Open-Shell-Menu"), QStringLiteral(R"(^OpenShellSetup_[\d_]+\.exe$)"),
                     Kind::Exe, { QStringLiteral("/qn"), QStringLiteral("ADDLOCAL=StartMenu") }, QString()) },

            { QStringLiteral("7zip.7zip"), QStringLiteral("7-Zip"), QStringLiteral("Utilities"),
              QStringLiteral("Opens and creates every common archive format."),
              QStringLiteral("^7-Zip"),
              // GitHub has the current build first, and 7-Zip ships unsigned.
              github(QStringLiteral("ip7z/7zip"), QStringLiteral(R"(^7z\d+-x64\.exe$)"),
                     Kind::Exe, kNsis, QString()) },
            { QStringLiteral("VideoLAN.VLC"), QStringLiteral("VLC media player"), QStringLiteral("Utilities"),
              QStringLiteral("Plays virtually any media format without hunting for codecs."),
              QStringLiteral("^VLC media player"),
              // Not get.videolan.org, which answers with a mirror picker.
              page(QStringLiteral("https://download.videolan.org/pub/videolan/vlc/last/win64/"),
                   QStringLiteral(R"re(href="(vlc-[\d.]+-win64\.exe)")re"), Kind::Exe,
                   { QStringLiteral("/S"), QStringLiteral("/L=1033") }, QStringLiteral("VideoLAN")) },
            { QStringLiteral("Notepad++.Notepad++"), QStringLiteral("Notepad++"), QStringLiteral("Utilities"),
              QStringLiteral("Text and code editor."),
              QStringLiteral(R"(^Notepad\+\+)"),
              github(QStringLiteral("notepad-plus-plus/notepad-plus-plus"),
                     QStringLiteral(R"(^npp\.[\d.]+\.Installer\.x64\.exe$)"), Kind::Exe, kNsis,
                     QStringLiteral("Notepad++")) },
            { QStringLiteral("voidtools.Everything"), QStringLiteral("Everything"), QStringLiteral("Utilities"),
              QStringLiteral("Finds any file on any NTFS drive as fast as you can type its name."),
              QStringLiteral(R"(^Everything \d)"),
              page(QStringLiteral("https://www.voidtools.com/downloads/"),
                   QStringLiteral(R"re(href="(/Everything-[\d.]+\.x64-Setup\.exe)")re"), Kind::Exe, kNsis,
                   QStringLiteral("voidtools")) },
            { QStringLiteral("Microsoft.PowerToys"), QStringLiteral("PowerToys"), QStringLiteral("Utilities"),
              QStringLiteral("Microsoft's own utility set: window layouts, a better search, bulk rename, and a color picker."),
              QStringLiteral("^PowerToys"),
              github(QStringLiteral("microsoft/PowerToys"), QStringLiteral(R"(^PowerToysSetup-[\d.]+-x64\.exe$)"),
                     Kind::Exe, { QStringLiteral("/quiet"), QStringLiteral("/norestart") }, kMicrosoft) },

            { QStringLiteral("Mozilla.Firefox"), QStringLiteral("Mozilla Firefox"), QStringLiteral("Browsers"),
              QStringLiteral("Independent browser with strong tracking protection."),
              QStringLiteral("^Mozilla Firefox"),
              fixed(QStringLiteral("https://download.mozilla.org/?product=firefox-latest-ssl&os=win64&lang=en-US"),
                    Kind::Exe, kNsis, QStringLiteral("Mozilla Corporation")) },
            { QStringLiteral("Google.Chrome"), QStringLiteral("Google Chrome"), QStringLiteral("Browsers"),
              QStringLiteral("Google's web browser."),
              QStringLiteral("^Google Chrome$"),
              fixed(QStringLiteral("https://dl.google.com/dl/chrome/install/googlechromestandaloneenterprise64.msi"),
                    Kind::Msi, {}, QStringLiteral("Google LLC")) },
            { QStringLiteral("Brave.Brave"), QStringLiteral("Brave"), QStringLiteral("Browsers"),
              QStringLiteral("Chromium browser with built-in ad and tracker blocking."),
              QStringLiteral("^Brave$"),
              fixed(QStringLiteral("https://laptop-updates.brave.com/latest/winx64"),
                    Kind::Exe, { QStringLiteral("/silent"), QStringLiteral("/install") },
                    QStringLiteral("Brave Software")) },

            { QStringLiteral("Microsoft.VCRedist.2015+.x64"), QStringLiteral("Visual C++ Redistributable (x64)"),
              QStringLiteral("Runtimes"),
              QStringLiteral("VC++ 2015-2022 runtime, 64-bit. Required by a large share of Windows software."),
              QStringLiteral(R"(Visual C\+\+ 2015-20\d\d Redistributable \(x64\))"),
              fixed(QStringLiteral("https://aka.ms/vs/17/release/vc_redist.x64.exe"), Kind::Exe, kMsQuiet, kMicrosoft) },
            { QStringLiteral("Microsoft.VCRedist.2015+.x86"), QStringLiteral("Visual C++ Redistributable (x86)"),
              QStringLiteral("Runtimes"),
              QStringLiteral("VC++ 2015-2022 runtime, 32-bit. Still needed by 32-bit programs on a 64-bit system."),
              QStringLiteral(R"(Visual C\+\+ 2015-20\d\d Redistributable \(x86\))"),
              fixed(QStringLiteral("https://aka.ms/vs/17/release/vc_redist.x86.exe"), Kind::Exe, kMsQuiet, kMicrosoft) },
            { QStringLiteral("Microsoft.DotNet.DesktopRuntime.8"), QStringLiteral(".NET Desktop Runtime 8"),
              QStringLiteral("Runtimes"),
              QStringLiteral("Runs .NET 8 desktop applications."),
              QStringLiteral(R"(Windows Desktop Runtime - 8\.)"),
              fixed(QStringLiteral("https://aka.ms/dotnet/8.0/windowsdesktop-runtime-win-x64.exe"),
                    Kind::Exe, kMsQuiet, kMicrosoft) },
            { QStringLiteral("Microsoft.EdgeWebView2Runtime"), QStringLiteral("Microsoft Edge WebView2 Runtime"),
              QStringLiteral("Runtimes"),
              QStringLiteral("The web engine many apps draw their windows with. Without it they open blank."),
              QStringLiteral(R"(^Microsoft Edge WebView2 Runtime)"),
              fixed(QStringLiteral("https://go.microsoft.com/fwlink/p/?LinkId=2124703"),
                    Kind::Exe, { QStringLiteral("/silent"), QStringLiteral("/install") }, kMicrosoft) },
            { QStringLiteral("Microsoft.DirectX"), QStringLiteral("DirectX runtime"), QStringLiteral("Runtimes"),
              QStringLiteral("The legacy DirectX runtime that older games still ask for."),
              QString(),
              fixed(QStringLiteral("https://download.microsoft.com/download/1/7/1/1718CCC4-6315-4D8E-9543-8E28A4E18C4C/dxwebsetup.exe"),
                    Kind::Exe, { QStringLiteral("/Q") }, kMicrosoft) },

            { QStringLiteral("Git.Git"), QStringLiteral("Git"), QStringLiteral("Development"),
              QStringLiteral("Distributed version control."),
              QStringLiteral(R"(^Git(\s|$))"),
              github(QStringLiteral("git-for-windows/git"), QStringLiteral(R"(^Git-[\d.]+-64-bit\.exe$)"),
                     Kind::Exe, kInno, QStringLiteral("Johannes Schindelin")) },
            { QStringLiteral("Microsoft.VisualStudioCode"), QStringLiteral("Visual Studio Code"),
              QStringLiteral("Development"), QStringLiteral("Code editor."),
              QStringLiteral("^Microsoft Visual Studio Code"),
              fixed(QStringLiteral("https://update.code.visualstudio.com/latest/win32-x64/stable"),
                    Kind::Exe, kInno + QStringList{ QStringLiteral("/MERGETASKS=!runcode") }, kMicrosoft) },
            { QStringLiteral("Python.Python.3.12"), QStringLiteral("Python 3.12"), QStringLiteral("Development"),
              QStringLiteral("Python runtime and tooling."),
              QStringLiteral(R"(^Python 3\.12\.\d+ \(64-bit\))"),
              fixed(QStringLiteral("https://www.python.org/ftp/python/3.12.10/python-3.12.10-amd64.exe"),
                    Kind::Exe, { QStringLiteral("/quiet"), QStringLiteral("InstallAllUsers=1"),
                                 QStringLiteral("PrependPath=1"), QStringLiteral("Include_test=0") },
                    QStringLiteral("Python Software Foundation")) },
            { QStringLiteral("OpenJS.NodeJS.LTS"), QStringLiteral("Node.js (LTS)"), QStringLiteral("Development"),
              QStringLiteral("JavaScript runtime."),
              QStringLiteral(R"(^Node\.js$)"),
              [] { DirectSource d; d.locate = Locate::NodeLts; d.kind = Kind::Msi;
                   d.signer = QStringLiteral("OpenJS Foundation"); return d; }() },
            { QStringLiteral("Microsoft.PowerShell"), QStringLiteral("PowerShell 7"), QStringLiteral("Development"),
              QStringLiteral("Cross-platform PowerShell."),
              QStringLiteral(R"(^PowerShell 7)"),
              github(QStringLiteral("PowerShell/PowerShell"), QStringLiteral(R"(^PowerShell-[\d.]+-win-x64\.msi$)"),
                     Kind::Msi, {}, kMicrosoft) },
        };
        return c;
    }

    QSet<QString> installedAmong(const QStringList& candidates) {
        QSet<QString> found;
        if (candidates.isEmpty())
            return found;

        const QStringList names = installedDisplayNames();
        for (const QString& id : candidates)
            if (const PackageDef* def = findPackage(id); def && installedLocally(*def, names))
                found.insert(id);

        // It knows portable installs the programs list doesn't.
        if (wingetState().usable) {
            const Proc listed = runWinget({ QStringLiteral("list"),
                                            QStringLiteral("--disable-interactivity"),
                                            QStringLiteral("--accept-source-agreements") }, 120000);
            if (listed.code == 0) {
                const QString text = QString::fromUtf8(listed.output);
                for (const QString& id : candidates)
                    if (validId().match(id).hasMatch() && text.contains(id))
                        found.insert(id);
            }
        }
        return found;
    }

    InstallOutcome install(const QString& id, const StatusFn& status) {
        InstallOutcome out;
        const PackageDef* def = validId().match(id).hasMatch() ? findPackage(id) : nullptr;
        if (!def) {
            out.result  = InstallResult::InvalidId;
            out.message = QStringLiteral("Vethida does not know this app.");
            return out;
        }
        const auto say = [&status](const QString& s) { if (status) status(s); };

        if (installedLocally(*def, installedDisplayNames())) {
            out.result  = InstallResult::AlreadyInstalled;
            out.message = QStringLiteral("Already installed.");
            return out;
        }

        say(QStringLiteral("Preparing"));
        const WingetState w = wingetState();
        QString wingetFailure;
        bool wingetRanInstaller = false;
        if (w.usable) {
            QStringList args = {
                QStringLiteral("install"), QStringLiteral("--id"), id, QStringLiteral("-e"),
                QStringLiteral("--silent"), QStringLiteral("--accept-package-agreements"),
                QStringLiteral("--source"), QStringLiteral("winget"),
                QStringLiteral("--disable-interactivity"), QStringLiteral("--accept-source-agreements"),
            };
            say(QStringLiteral("Installing"));
            Proc r = runWinget(args, 15 * 60 * 1000);
            if (isSourceFailure(r.code) && repairSource())
                r = runWinget(args, 15 * 60 * 1000);

            if (r.code == 0) {
                out.result  = InstallResult::Ok;
                out.message = QStringLiteral("Installed.");
                return out;
            }
            if (r.code == kWgAlreadyInstalled || r.code == kWgUpdateNotApplicable) {
                out.result  = InstallResult::AlreadyInstalled;
                out.message = QStringLiteral("Already installed.");
                return out;
            }
            qCWarning(catInstaller).noquote() << "winget install" << id << "rc=" << r.code
                                              << QString::fromUtf8(r.output).left(800);
            wingetFailure = explainWinget(r.code, r.output, def->name);
            wingetRanInstaller = r.code == kWgInstallerFailed;
        }

        InstallOutcome direct;
        if (def->direct) {
            direct = installDirect(*def, status);
            if (direct.result == InstallResult::Ok || direct.result == InstallResult::AlreadyInstalled)
                return direct;
        }

        // Skipped if winget already ran this installer, which would just fail again.
        if (!wingetRanInstaller) {
            bool reached = false;
            const InstallOutcome viaCatalog = installFromManifest(*def, status, &reached);
            if (viaCatalog.result == InstallResult::Ok || reached)
                return viaCatalog;
            if (!def->direct && wingetFailure.isEmpty())
                return viaCatalog;
        }

        if (def->direct)
            return direct;
        out.message = wingetFailure;
        return out;
    }
}
