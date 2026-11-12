#include "Startup.h"

#include "../Logger.h"
#include "../Runtime/Paths.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStringLiteral>

#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <softpub.h>
#include <winevt.h>
#include <wintrust.h>
#include <mscat.h>

#include <climits>
#include <cwchar>
#include <iterator>
#include <string>
#include <vector>
#include "Registry.h"
#include <QSettings>
#include <taskschd.h>
#include <winsvc.h>
#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "oleaut32.lib")

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "wevtapi.lib")
#pragma comment(lib, "wintrust.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "version.lib")

namespace vth::win::startup
{
    namespace
    {
        VTH_DEFINE_LOG_CATEGORY(catStartup, "vth.startup")

        const QString kRunMirrorSubkey  = QStringLiteral("vethida_disabled");
        const QString kStartupMirrorDir = QStringLiteral("Vethida_Disabled");

        class RegKey {
        public:
            RegKey() = default;
            ~RegKey() { reset(); }
            RegKey(const RegKey&) = delete;
            RegKey& operator=(const RegKey&) = delete;

            HKEY  get() const noexcept { return h_; }
            HKEY* addr()               { reset(); return &h_; }

            void reset() {
                if (h_) { RegCloseKey(h_); h_ = nullptr; }
            }

        private:
            HKEY h_ = nullptr;
        };

        class IconHandle {
        public:
            IconHandle() = default;
            ~IconHandle() { reset(); }
            IconHandle(const IconHandle&) = delete;
            IconHandle& operator=(const IconHandle&) = delete;

            HICON  get() const noexcept { return h_; }
            HICON* addr()               { reset(); return &h_; }

            void reset() {
                if (h_) { DestroyIcon(h_); h_ = nullptr; }
            }

        private:
            HICON h_ = nullptr;
        };

        HKEY hkeyFor(Hive h) noexcept {
            return (h == Hive::LocalMachine) ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
        }

        QString hiveLabel(Hive h) {
            return (h == Hive::LocalMachine) ? QStringLiteral("HKLM") : QStringLiteral("HKCU");
        }

        QString expandEnv(const QString& s) {
            if (s.isEmpty()) return s;
            const std::wstring in = s.toStdWString();
            const DWORD needed = ExpandEnvironmentStringsW(in.c_str(), nullptr, 0);
            if (needed == 0) return s;
            std::wstring out(needed, L'\0');
            const DWORD wrote = ExpandEnvironmentStringsW(in.c_str(), out.data(), needed);
            if (wrote == 0 || wrote > needed) return s;
            out.resize(wrote - 1);
            return QString::fromStdWString(out);
        }

        // Best effort. Empty if nothing on the command line names a file.
        QString resolveExePath(const QString& rawCommand) {
            const QString cmd = expandEnv(rawCommand.trimmed());
            if (cmd.isEmpty()) return {};

            if (cmd.startsWith(QLatin1Char('"'))) {
                const int closing = cmd.indexOf(QLatin1Char('"'), 1);
                if (closing > 1) return cmd.mid(1, closing - 1);
            }

            for (int i = 0; i < cmd.size() - 1; ++i) {
                if (cmd[i] != QLatin1Char(' ')) continue;
                const QChar next = cmd[i + 1];
                if (next == QLatin1Char('-') || next == QLatin1Char('/'))
                    return cmd.left(i);
            }

            // An unquoted path may contain spaces, so try it the way CreateProcess does.
            if (QFileInfo::exists(cmd)) return cmd;
            for (int sp = cmd.indexOf(QLatin1Char(' ')); sp > 0;
                 sp = cmd.indexOf(QLatin1Char(' '), sp + 1)) {
                const QString candidate = cmd.left(sp);
                if (QFileInfo::exists(candidate))
                    return candidate;
                const QString exe = candidate + QStringLiteral(".exe");
                if (QFileInfo::exists(exe))
                    return exe;
            }

            return cmd.contains(QLatin1Char(' ')) ? QString{} : cmd;
        }

        void readVersionInfo(const QString& filePath, StartupEntry& e) {
            const std::wstring path = filePath.toStdWString();
            DWORD handle = 0;
            const DWORD size = GetFileVersionInfoSizeW(path.c_str(), &handle);
            if (size == 0) return;

            std::vector<BYTE> buf(size);
            if (!GetFileVersionInfoW(path.c_str(), 0, size, buf.data())) return;

            struct LangCodePage { WORD lang; WORD code; };
            LangCodePage* trans = nullptr;
            UINT transSize = 0;
            if (!VerQueryValueW(buf.data(), L"\\VarFileInfo\\Translation",
                                reinterpret_cast<LPVOID*>(&trans), &transSize)) {
                return;
            }

            auto query = [&](LangCodePage lcp, const wchar_t* field) -> QString {
                wchar_t sub[128]{};
                swprintf_s(sub, L"\\StringFileInfo\\%04x%04x\\%s", lcp.lang, lcp.code, field);
                wchar_t* val = nullptr;
                UINT len = 0;
                if (VerQueryValueW(buf.data(), sub,
                                   reinterpret_cast<LPVOID*>(&val), &len) && val) {
                    return QString::fromWCharArray(val, static_cast<int>(wcsnlen(val, len)));
                }
                return {};
            };

            const UINT count = transSize / sizeof(LangCodePage);
            for (UINT i = 0; i < count; ++i) {
                if (e.publisher.isEmpty())   e.publisher   = query(trans[i], L"CompanyName");
                if (e.description.isEmpty()) e.description = query(trans[i], L"FileDescription");
            }
        }

        // Fills in the signature and signer if td verifies.
        bool checkTrust(WINTRUST_DATA& td, StartupEntry& e) {
            td.cbStruct            = sizeof(td);
            td.dwUIChoice          = WTD_UI_NONE;
            td.fdwRevocationChecks = WTD_REVOKE_NONE;
            td.dwStateAction       = WTD_STATEACTION_VERIFY;

            GUID policy = WINTRUST_ACTION_GENERIC_VERIFY_V2;
            const bool verified = WinVerifyTrust(nullptr, &policy, &td) == ERROR_SUCCESS;

            if (verified) {
                e.signature = SignatureStatus::Verified;
                if (auto* pd = WTHelperProvDataFromStateData(td.hWVTStateData)) {
                    if (auto* signer = WTHelperGetProvSignerFromChain(pd, 0, FALSE, 0)) {
                        if (signer->pChainContext &&
                            signer->pChainContext->cChain > 0 &&
                            signer->pChainContext->rgpChain[0]->cElement > 0) {
                            auto* ctx = signer->pChainContext->rgpChain[0]
                                            ->rgpElement[0]->pCertContext;
                            wchar_t name[256]{};
                            if (CertGetNameStringW(ctx, CERT_NAME_SIMPLE_DISPLAY_TYPE,
                                                   0, nullptr, name,
                                                   static_cast<DWORD>(std::size(name)))) {
                                e.signerName = QString::fromWCharArray(name);
                            }
                        }
                    }
                }
            }

            td.dwStateAction = WTD_STATEACTION_CLOSE;
            WinVerifyTrust(nullptr, &policy, &td);
            return verified;
        }

        bool verifyAgainstCatalog(const std::wstring& path, const wchar_t* hashAlgorithm,
                                  StartupEntry& e) {
            const HANDLE file = CreateFileW(path.c_str(), GENERIC_READ,
                                            FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                            OPEN_EXISTING, 0, nullptr);
            if (file == INVALID_HANDLE_VALUE) return false;

            HCATADMIN admin = nullptr;
            if (!CryptCATAdminAcquireContext2(&admin, nullptr, hashAlgorithm, nullptr, 0)) {
                CloseHandle(file);
                return false;
            }

            DWORD size = 0;
            CryptCATAdminCalcHashFromFileHandle2(admin, file, &size, nullptr, 0);
            std::vector<BYTE> hash(size);
            const bool hashed = size > 0
                && CryptCATAdminCalcHashFromFileHandle2(admin, file, &size, hash.data(), 0);
            CloseHandle(file);

            bool verified = false;
            HCATINFO catalog = hashed
                ? CryptCATAdminEnumCatalogFromHash(admin, hash.data(), size, 0, nullptr)
                : nullptr;
            if (catalog) {
                CATALOG_INFO info{};
                info.cbStruct = sizeof(info);
                if (CryptCATCatalogInfoFromContext(catalog, &info, 0)) {
                    const std::wstring tag = QString::fromLatin1(
                        QByteArray(reinterpret_cast<const char*>(hash.data()),
                                   static_cast<int>(size)).toHex().toUpper()).toStdWString();

                    WINTRUST_CATALOG_INFO ci{};
                    ci.cbStruct             = sizeof(ci);
                    ci.pcwszCatalogFilePath = info.wszCatalogFile;
                    ci.pcwszMemberTag       = tag.c_str();
                    ci.pcwszMemberFilePath  = path.c_str();
                    ci.pbCalculatedFileHash = hash.data();
                    ci.cbCalculatedFileHash = size;
                    ci.hCatAdmin            = admin;

                    WINTRUST_DATA td{};
                    td.dwUnionChoice = WTD_CHOICE_CATALOG;
                    td.pCatalog      = &ci;
                    verified = checkTrust(td, e);
                }
                CryptCATAdminReleaseCatalogContext(admin, catalog, 0);
            }
            CryptCATAdminReleaseContext(admin, 0);
            return verified;
        }

        // Many Windows programs are signed in a catalog instead of the file itself.
        void verifySignature(const QString& filePath, StartupEntry& e) {
            const std::wstring path = filePath.toStdWString();

            WINTRUST_FILE_INFO fi{};
            fi.cbStruct      = sizeof(fi);
            fi.pcwszFilePath = path.c_str();

            WINTRUST_DATA td{};
            td.dwUnionChoice = WTD_CHOICE_FILE;
            td.pFile         = &fi;

            if (checkTrust(td, e)
                || verifyAgainstCatalog(path, L"SHA256", e)
                || verifyAgainstCatalog(path, L"SHA1", e))
                return;
            e.signature = SignatureStatus::Unverified;
        }

        void extractIcon(const QString& filePath, StartupEntry& e) {
            if (!QFile::exists(filePath)) return;

            const std::wstring path = filePath.toStdWString();

            IconHandle largeIcon;
            IconHandle smallIcon;
            const UINT got = ExtractIconExW(path.c_str(), 0, largeIcon.addr(),
                                            smallIcon.addr(), 1);
            if (got == 0 || got == UINT_MAX) return;

            HICON pick = largeIcon.get() ? largeIcon.get() : smallIcon.get();
            if (!pick) return;

            QImage img = QImage::fromHICON(pick);
            if (!img.isNull()) e.iconImage = std::move(img);
        }

        void readFileDetails(StartupEntry& e) {
            if (e.resolvedPath.isEmpty() || !QFile::exists(e.resolvedPath)) return;
            readVersionInfo(e.resolvedPath, e);
            verifySignature(e.resolvedPath, e);
            extractIcon(e.resolvedPath, e);
        }

        struct RunKeyDef {
            Hive    hive;
            QString subkey;
            bool    wow64;
        };

        const QVector<RunKeyDef> kRunKeys = {
            { Hive::CurrentUser,
              QStringLiteral("Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
              false },
            { Hive::CurrentUser,
              QStringLiteral("Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce"),
              false },
            { Hive::LocalMachine,
              QStringLiteral("Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
              false },
            { Hive::LocalMachine,
              QStringLiteral("Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce"),
              false },
            { Hive::LocalMachine,
              QStringLiteral("Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Run"),
              true },
            { Hive::LocalMachine,
              QStringLiteral("Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\RunOnce"),
              true },
        };

        // The id uses the live subkey, so an entry keeps it when moved to the mirror.
        QString runKeyId(const RunKeyDef& def, const QString& valueName) {
            return QStringLiteral("run|%1|%2|%3").arg(hiveLabel(def.hive), def.subkey, valueName);
        }

        void enumerateRunKeyValues(const RunKeyDef& def, const QString& subkeyPath,
                                   bool enabled, QVector<StartupEntry>& out) {
            RegKey key;
            const std::wstring sub = subkeyPath.toStdWString();
            if (RegOpenKeyExW(hkeyFor(def.hive), sub.c_str(), 0, KEY_READ, key.addr())
                    != ERROR_SUCCESS) {
                return;
            }

            DWORD valueCount = 0;
            DWORD maxNameLen = 0;
            DWORD maxData    = 0;
            if (RegQueryInfoKeyW(key.get(), nullptr, nullptr, nullptr, nullptr, nullptr,
                                 nullptr, &valueCount, &maxNameLen, &maxData,
                                 nullptr, nullptr) != ERROR_SUCCESS) {
                return;
            }

            std::vector<wchar_t> nameBuf(maxNameLen + 1);
            std::vector<BYTE>    dataBuf(maxData + sizeof(wchar_t));

            for (DWORD i = 0; i < valueCount; ++i) {
                DWORD nameLen = static_cast<DWORD>(nameBuf.size());
                DWORD dataLen = static_cast<DWORD>(dataBuf.size());
                DWORD type    = 0;
                if (RegEnumValueW(key.get(), i, nameBuf.data(), &nameLen,
                                  nullptr, &type, dataBuf.data(), &dataLen)
                        != ERROR_SUCCESS) {
                    continue;
                }
                if (type != REG_SZ && type != REG_EXPAND_SZ) continue;

                const QString valueName = QString::fromWCharArray(nameBuf.data(), nameLen);
                if (valueName.isEmpty()) continue;

                QString rawValue;
                if (dataLen >= sizeof(wchar_t)) {
                    const auto* w = reinterpret_cast<const wchar_t*>(dataBuf.data());
                    rawValue = QString::fromWCharArray(
                        w, static_cast<int>(dataLen / sizeof(wchar_t)));
                    while (!rawValue.isEmpty() && rawValue.back() == QChar(0))
                        rawValue.chop(1);
                }

                StartupEntry e;
                e.id           = runKeyId(def, valueName);
                e.displayName  = valueName;
                e.command      = rawValue;
                e.resolvedPath = resolveExePath(rawValue);
                e.fileName     = e.resolvedPath.isEmpty()
                                     ? QString{}
                                     : QFileInfo(e.resolvedPath).fileName();
                e.location     = Location::RunKey;
                e.enabled      = enabled;
                e.runKeyHive   = def.hive;
                e.runKeySubkey = def.subkey;
                e.runKeyValue  = valueName;
                e.runKeyWow64  = def.wow64;
                readFileDetails(e);

                out.append(std::move(e));
            }
        }

        // Windows reads the live key, so a value in both places keeps its live row
        // and gets marked staleDisableMirror.
        void enumerateRunKeys(QVector<StartupEntry>& out) {
            for (const auto& def : kRunKeys) {
                const int liveBegin = out.size();
                enumerateRunKeyValues(def, def.subkey, true, out);
                const int liveEnd = out.size();

                QVector<StartupEntry> mirrored;
                enumerateRunKeyValues(def, def.subkey + QLatin1Char('\\') + kRunMirrorSubkey,
                                      false, mirrored);

                for (StartupEntry& m : mirrored) {
                    bool live = false;
                    for (int i = liveBegin; i < liveEnd; ++i) {
                        if (out[i].id != m.id) continue;
                        out[i].staleDisableMirror = true;
                        live = true;
                        break;
                    }
                    if (!live) out.append(std::move(m));
                }
            }
        }

        QString knownStartupDir(REFKNOWNFOLDERID id) {
            PWSTR raw = nullptr;
            if (FAILED(SHGetKnownFolderPath(id, KF_FLAG_DEFAULT, nullptr, &raw)))
                return {};
            QString out = QString::fromWCharArray(raw);
            CoTaskMemFree(raw);
            return out;
        }

        void enumerateOneStartupDir(const QString& dir, bool enabled, QVector<StartupEntry>& out) {
            if (dir.isEmpty()) return;

            QDir d(dir);
            if (!d.exists()) return;

            const QFileInfoList entries = d.entryInfoList(
                QDir::Files | QDir::NoDotAndDotDot, QDir::Name);

            for (const QFileInfo& fi : entries) {
                StartupEntry e;
                e.id                = QStringLiteral("sf|%1").arg(fi.absoluteFilePath());
                e.displayName       = fi.fileName();
                e.command           = fi.absoluteFilePath();
                e.fileName          = fi.fileName();
                e.location          = Location::StartupFolder;
                e.enabled           = enabled;
                e.startupFolderPath = fi.absoluteFilePath();

                // Shortcut targets aren't resolved, so a .lnk only gets its icon.
                const QString ext = fi.suffix().toLower();
                if (ext == QLatin1String("exe") || ext == QLatin1String("dll") ||
                    ext == QLatin1String("scr") || ext == QLatin1String("com") ||
                    ext == QLatin1String("bat") || ext == QLatin1String("cmd") ||
                    ext == QLatin1String("ps1") || ext == QLatin1String("vbs")) {
                    e.resolvedPath = fi.absoluteFilePath();
                    readFileDetails(e);
                } else {
                    extractIcon(fi.absoluteFilePath(), e);
                }

                out.append(std::move(e));
            }
        }

        void enumerateStartupFolders(QVector<StartupEntry>& out) {
            const QString user   = knownStartupDir(FOLDERID_Startup);
            const QString common = knownStartupDir(FOLDERID_CommonStartup);

            enumerateOneStartupDir(user,   true, out);
            enumerateOneStartupDir(common, true, out);

            if (!user.isEmpty())
                enumerateOneStartupDir(user + QLatin1Char('/') + kStartupMirrorDir, false, out);
            if (!common.isEmpty())
                enumerateOneStartupDir(common + QLatin1Char('/') + kStartupMirrorDir, false, out);
        }

        ActionResult mapRegError(LONG r) noexcept {
            if (r == ERROR_SUCCESS)        return ActionResult::Ok;
            if (r == ERROR_ACCESS_DENIED)  return ActionResult::AccessDenied;
            if (r == ERROR_FILE_NOT_FOUND) return ActionResult::NotFound;
            return ActionResult::OtherError;
        }

        ActionResult moveRunKeyValue(const StartupEntry& e, bool enabling) {
            const QString mirror = e.runKeySubkey + QLatin1Char('\\') + kRunMirrorSubkey;
            const QString fromSub = enabling ? mirror : e.runKeySubkey;
            const QString toSub   = enabling ? e.runKeySubkey : mirror;

            RegKey src;
            const std::wstring fromW = fromSub.toStdWString();
            LONG r = RegOpenKeyExW(hkeyFor(e.runKeyHive), fromW.c_str(),
                                   0, KEY_QUERY_VALUE | KEY_SET_VALUE, src.addr());
            if (r != ERROR_SUCCESS) return mapRegError(r);

            const std::wstring valueW = e.runKeyValue.toStdWString();
            DWORD type  = 0;
            DWORD bytes = 0;
            r = RegQueryValueExW(src.get(), valueW.c_str(), nullptr, &type, nullptr, &bytes);
            if (r != ERROR_SUCCESS) return mapRegError(r);
            if (type != REG_SZ && type != REG_EXPAND_SZ) return ActionResult::OtherError;

            std::vector<BYTE> data(bytes);
            r = RegQueryValueExW(src.get(), valueW.c_str(), nullptr, &type,
                                 data.data(), &bytes);
            if (r != ERROR_SUCCESS) return mapRegError(r);

            RegKey dst;
            const std::wstring toW = toSub.toStdWString();
            r = RegCreateKeyExW(hkeyFor(e.runKeyHive), toW.c_str(), 0, nullptr,
                                REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, nullptr,
                                dst.addr(), nullptr);
            if (r != ERROR_SUCCESS) return mapRegError(r);

            // Write before deleting, so a failure can't lose the value.
            r = RegSetValueExW(dst.get(), valueW.c_str(), 0, type, data.data(), bytes);
            if (r != ERROR_SUCCESS) return mapRegError(r);

            r = RegDeleteValueW(src.get(), valueW.c_str());
            if (r != ERROR_SUCCESS && r != ERROR_FILE_NOT_FOUND) return mapRegError(r);
            return ActionResult::Ok;
        }

        ActionResult moveStartupFile(const StartupEntry& e, bool enabling) {
            const QFileInfo srcInfo(e.startupFolderPath);
            if (!srcInfo.exists()) return ActionResult::NotFound;

            QDir parent = srcInfo.absoluteDir();

            QString destDir;
            if (enabling) {
                QDir up = parent;
                if (!up.cdUp()) return ActionResult::OtherError;
                destDir = up.absolutePath();
            } else {
                destDir = parent.absoluteFilePath(kStartupMirrorDir);
                QDir().mkpath(destDir);
            }

            // Refuse rather than overwrite what's already there.
            const QString destPath = destDir + QLatin1Char('/') + srcInfo.fileName();
            if (QFile::exists(destPath))
                return ActionResult::AlreadyExists;
            if (!QFile::rename(e.startupFolderPath, destPath))
                return ActionResult::OtherError;
            return ActionResult::Ok;
        }

        // Reads <Data Name="name">value</Data>.
        qint64 extractDataValue(const QString& xml, QStringView name) {
            const QString marker = QStringLiteral("Name=\"") + name.toString()
                                 + QStringLiteral("\">");
            const int idx = xml.indexOf(marker);
            if (idx < 0) return 0;
            const int start = idx + marker.size();
            const int end   = xml.indexOf(QLatin1Char('<'), start);
            if (end < 0) return 0;
            bool ok = false;
            const qint64 v = QStringView(xml).mid(start, end - start).toLongLong(&ok);
            return ok ? v : 0;
        }
    }

    namespace {
        ActionResult setTaskEnabled(const StartupEntry& e, bool enabled);
        ActionResult setDelayedStartServiceEnabled(const StartupEntry& entry, bool enable);
    }

    QString locationLabel(Location l) {
        switch (l) {
            case Location::RunKey:        return QStringLiteral("Run key");
            case Location::StartupFolder: return QStringLiteral("Startup folder");
            case Location::ScheduledTask: return QStringLiteral("Scheduled task");
            case Location::DelayedStartService: return QStringLiteral("Service (delayed auto)");
        }
        return {};
    }

    QString signatureStatusLabel(SignatureStatus s) {
        switch (s) {
            case SignatureStatus::Verified:   return QStringLiteral("Verified");
            case SignatureStatus::Unverified: return QStringLiteral("Unverified");
            case SignatureStatus::Unknown:    break;
        }
        return QStringLiteral("Unknown");
    }

    QString staleMirrorNotice() {
        return QStringLiteral("Disabled by Vethida but still present in the live Run key.");
    }

    QString actionResultLabel(ActionResult r) {
        switch (r) {
            case ActionResult::Ok:
                return QStringLiteral("Change applied.");
            case ActionResult::AccessDenied:
                return QStringLiteral(
                    "Windows refused the change. The app that owns this entry, or "
                    "security software, is protecting it.");
            case ActionResult::NotFound:
                return QStringLiteral("Entry no longer exists.");
            case ActionResult::AlreadyExists:
                return QStringLiteral(
                    "A file with this name is already there. Move or delete it, "
                    "then try again.");
            case ActionResult::OtherError:
                return QStringLiteral("Windows could not change this entry.");
        }
        return {};
    }

    QVector<StartupEntry> enumerate() {
        QVector<StartupEntry> out;
        enumerateRunKeys(out);
        enumerateStartupFolders(out);
        return out;
    }

    ActionResult setEnabled(const StartupEntry& entry, bool enabled) {
        if (entry.enabled == enabled) return ActionResult::Ok;

        switch (entry.location) {
            case Location::RunKey:        return moveRunKeyValue(entry, enabled);
            case Location::StartupFolder: return moveStartupFile(entry, enabled);
            case Location::ScheduledTask: return setTaskEnabled(entry, enabled);
            case Location::DelayedStartService: return setDelayedStartServiceEnabled(entry, enabled);
        }
        return ActionResult::OtherError;
    }

    BootMetrics bootMetrics() {
        BootMetrics out;

        const wchar_t* channel = L"Microsoft-Windows-Diagnostics-Performance/Operational";
        const wchar_t* xpath   = L"*[System[EventID=100]]";

        EVT_HANDLE q = EvtQuery(nullptr, channel, xpath,
                                EvtQueryChannelPath | EvtQueryReverseDirection);
        if (!q) {
            qCInfo(catStartup) << "EvtQuery(Diagnostics-Performance) failed:" << GetLastError();
            return out;
        }

        EVT_HANDLE evt = nullptr;
        DWORD returned = 0;
        if (!EvtNext(q, 1, &evt, 5000, 0, &returned) || returned == 0 || !evt) {
            EvtClose(q);
            return out;
        }

        DWORD bufSize   = 0;
        DWORD propCount = 0;
        EvtRender(nullptr, evt, EvtRenderEventXml, 0, nullptr, &bufSize, &propCount);
        if (bufSize == 0) {
            EvtClose(evt);
            EvtClose(q);
            return out;
        }
        std::vector<wchar_t> buf(bufSize / sizeof(wchar_t) + 1);
        if (!EvtRender(nullptr, evt, EvtRenderEventXml,
                       static_cast<DWORD>(buf.size() * sizeof(wchar_t)),
                       buf.data(), &bufSize, &propCount)) {
            EvtClose(evt);
            EvtClose(q);
            return out;
        }
        EvtClose(evt);
        EvtClose(q);

        const QString xml = QString::fromWCharArray(buf.data());
        out.bootTimeMs     = extractDataValue(xml, u"BootTime");
        out.mainPathBootMs = extractDataValue(xml, u"MainPathBootTime");
        out.postBootMs     = extractDataValue(xml, u"BootPostBootTime");
        out.valid = (out.bootTimeMs > 0 || out.mainPathBootMs > 0 || out.postBootMs > 0);
        return out;
    }
}

namespace vth::win::startup::self
{
    namespace
    {
        // A scheduled task, because Windows skips the Run key for an app that needs
        // administrator rights.
        const QString kTaskName = QStringLiteral("Vethida\\Start at sign-in");

        // Absolute, because this runs elevated and PATH could be hijacked.
        QString schtasksPath()
        {
            wchar_t dir[MAX_PATH] = {};
            const UINT n = GetSystemDirectoryW(dir, MAX_PATH);
            if (n == 0 || n >= MAX_PATH)
                return QStringLiteral("schtasks.exe");
            return QString::fromWCharArray(dir) + QStringLiteral("\\schtasks.exe");
        }

        int runSchtasks(const QStringList& args)
        {
            QProcess p;
            p.start(schtasksPath(), args);
            if (!p.waitForStarted(5000)) {
                qCWarning(catStartup) << "schtasks did not start:" << p.errorString();
                return -1;
            }
            if (!p.waitForFinished(15000)) {
                p.kill();
                p.waitForFinished(2000);
                qCWarning(catStartup) << "schtasks timed out";
                return -1;
            }
            const int code = p.exitStatus() == QProcess::NormalExit ? p.exitCode() : -1;
            // /Query also exits non-zero when the task doesn't exist.
            if (code != 0 && !args.contains(QStringLiteral("/Query"))) {
                qCWarning(catStartup).noquote()
                    << "schtasks" << args.value(0) << "exited" << code << ":"
                    << QString::fromLocal8Bit(p.readAllStandardError()).trimmed()
                    << QString::fromLocal8Bit(p.readAllStandardOutput()).trimmed();
            }
            return code;
        }

        QString currentUser()
        {
            wchar_t name[256] = {};
            DWORD size = 256;
            if (!GetUserNameW(name, &size))
                return {};
            const QString domain = qEnvironmentVariable("USERDOMAIN");
            const QString user = QString::fromWCharArray(name);
            return domain.isEmpty() ? user : domain + QLatin1Char('\\') + user;
        }

        QString xmlEscape(QString s)
        {
            return s.replace(QLatin1Char('&'), QStringLiteral("&amp;"))
                    .replace(QLatin1Char('<'), QStringLiteral("&lt;"))
                    .replace(QLatin1Char('>'), QStringLiteral("&gt;"))
                    .replace(QLatin1Char('"'), QStringLiteral("&quot;"));
        }

        // PT0S removes the default three-day limit, which would close Vethida.
        QString taskXml(const QString& user, const QString& exe)
        {
            return QStringLiteral(
                "<?xml version=\"1.0\" encoding=\"UTF-16\"?>\n"
                "<Task version=\"1.2\" xmlns=\"http://schemas.microsoft.com/windows/2004/02/mit/task\">\n"
                "  <RegistrationInfo><Description>Opens Vethida when you sign in.</Description></RegistrationInfo>\n"
                "  <Triggers><LogonTrigger><Enabled>true</Enabled><UserId>%1</UserId></LogonTrigger></Triggers>\n"
                "  <Principals><Principal id=\"Author\"><UserId>%1</UserId>"
                "<LogonType>InteractiveToken</LogonType><RunLevel>HighestAvailable</RunLevel></Principal></Principals>\n"
                "  <Settings>\n"
                "    <MultipleInstancesPolicy>IgnoreNew</MultipleInstancesPolicy>\n"
                "    <DisallowStartIfOnBatteries>false</DisallowStartIfOnBatteries>\n"
                "    <StopIfGoingOnBatteries>false</StopIfGoingOnBatteries>\n"
                "    <ExecutionTimeLimit>PT0S</ExecutionTimeLimit>\n"
                "    <AllowHardTerminate>true</AllowHardTerminate>\n"
                "    <StartWhenAvailable>false</StartWhenAvailable>\n"
                "    <Enabled>true</Enabled>\n"
                "  </Settings>\n"
                "  <Actions Context=\"Author\"><Exec><Command>%2</Command></Exec></Actions>\n"
                "</Task>\n").arg(xmlEscape(user), xmlEscape(exe));
        }
    }

    bool launchesAtLogon()
    {
        return runSchtasks({ QStringLiteral("/Query"), QStringLiteral("/TN"), kTaskName }) == 0;
    }

    bool setLaunchAtLogon(bool enabled)
    {
        if (!enabled) {
            if (!launchesAtLogon())
                return true;
            runSchtasks({ QStringLiteral("/Delete"), QStringLiteral("/TN"), kTaskName,
                          QStringLiteral("/F") });
            return !launchesAtLogon();
        }

        const QString user = currentUser();
        if (user.isEmpty())
            return false;
        const QString exe = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());

        // protected folder, not %TEMP% (the task runs elevated). Not QTemporaryFile,
        // schtasks can't open it while it's locked.
        const QString defDir = vth::paths::protectedData() + QStringLiteral("/tmp");
        vth::paths::ensureDir(defDir);
        const QString defPath = defDir + QStringLiteral("/start-at-sign-in.xml");
        {
            QFile file(defPath);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
                return false;
            const QString xml = taskXml(user, exe);
            const quint8 bom[2] = { 0xFF, 0xFE };
            file.write(reinterpret_cast<const char*>(bom), 2);
            file.write(reinterpret_cast<const char*>(xml.utf16()), xml.size() * 2);
        }

        runSchtasks({ QStringLiteral("/Create"), QStringLiteral("/TN"), kTaskName,
                      QStringLiteral("/XML"), QDir::toNativeSeparators(defPath),
                      QStringLiteral("/F") });
        QFile::remove(defPath);
        return launchesAtLogon();
    }
}

namespace vth::win::startup
{
namespace
{
        class ComScope {
        public:
            ComScope() {
                const HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
                uninit_ = SUCCEEDED(hr);
                ok_ = uninit_ || hr == RPC_E_CHANGED_MODE;
            }
            ~ComScope() { if (uninit_) CoUninitialize(); }
            bool ok() const noexcept { return ok_; }
        private:
            bool ok_     = false;
            bool uninit_ = false;
        };
        template <typename T>
        class ComPtr {
        public:
            ComPtr() = default;
            ~ComPtr() { reset(); }
            ComPtr(const ComPtr&) = delete;
            ComPtr& operator=(const ComPtr&) = delete;

            ComPtr(ComPtr&& o) noexcept : p_(o.p_) { o.p_ = nullptr; }
            ComPtr& operator=(ComPtr&& o) noexcept {
                if (this != &o) { reset(); p_ = o.p_; o.p_ = nullptr; }
                return *this;
            }

            T*  get()       const noexcept { return p_; }
            T** addr()                     { reset(); return &p_; }
            T*  operator->() const noexcept { return p_; }
            explicit operator bool() const noexcept { return p_ != nullptr; }

            void reset() {
                if (p_) { p_->Release(); p_ = nullptr; }
            }

        private:
            T* p_ = nullptr;
        };
        QString bstrToQString(BSTR b) {
            if (!b) return {};
            return QString::fromWCharArray(b, SysStringLen(b));
        }
        struct FileMeta {
            QString publisher;
            QString description;
            SignatureStatus signature = SignatureStatus::Unknown;
            QString signerName;
            QImage  iconImage;
        };
        void gatherMeta(const QString& resolvedPath, FileMeta& out) {
            StartupEntry e;
            e.resolvedPath = resolvedPath;
            readFileDetails(e);
            out.publisher   = std::move(e.publisher);
            out.description = std::move(e.description);
            out.signature   = e.signature;
            out.signerName  = std::move(e.signerName);
            out.iconImage   = std::move(e.iconImage);
        }
        QString resolveServiceExe(const QString& imagePath) {
            QString p = imagePath.trimmed();
            if (p.isEmpty()) return {};
            if (p.startsWith(QStringLiteral("\\??\\"))) p = p.mid(4);
            if (p.startsWith(QStringLiteral("\\SystemRoot\\"), Qt::CaseInsensitive)) {
                wchar_t winDir[MAX_PATH] = {};
                if (GetWindowsDirectoryW(winDir, MAX_PATH) > 0) {
                    p = QString::fromWCharArray(winDir) + QLatin1Char('/') + p.mid(12);
                }
            }
            if (p.startsWith(QLatin1Char('"'))) {
                const int closing = p.indexOf(QLatin1Char('"'), 1);
                if (closing > 1) p = p.mid(1, closing - 1);
            } else {
                const int sp = p.indexOf(QLatin1Char(' '));
                if (sp > 0) p = p.left(sp);
            }
            p.replace(QLatin1Char('\\'), QLatin1Char('/'));
            return p;
        }
        void enumerateDelayedStartServices(QVector<StartupEntry>& out) {
            RegKey services;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services",
                              0, KEY_READ, services.addr()) != ERROR_SUCCESS) {
                qCInfo(catStartup) << "Could not open Services key";
                return;
            }
            for (DWORD index = 0;; ++index) {
                wchar_t nameBuf[256] = {};
                DWORD nameLen = static_cast<DWORD>(std::size(nameBuf));
                const LSTATUS r = RegEnumKeyExW(services.get(), index, nameBuf, &nameLen,
                                                nullptr, nullptr, nullptr, nullptr);
                if (r == ERROR_NO_MORE_ITEMS) break;
                if (r != ERROR_SUCCESS) continue;
                RegKey svc;
                if (RegOpenKeyExW(services.get(), nameBuf, 0, KEY_READ, svc.addr())
                        != ERROR_SUCCESS) {
                    continue;
                }
                const DWORD start       = reg::readDwordAt(svc.get(), L"Start", 0xFFFFFFFFu);
                const DWORD delayedAuto = reg::readDwordAt(svc.get(), L"DelayedAutostart", 0);
                const bool delayedActive = (start == SERVICE_AUTO_START && delayedAuto == 1);
                const bool demoted       = (start == SERVICE_DISABLED   && delayedAuto == 1);
                if (!delayedActive && !demoted) continue;
                const QString name        = QString::fromWCharArray(nameBuf,
                                                static_cast<int>(nameLen));
                const QString imagePath   = reg::readStringAt(svc.get(), L"ImagePath", true);
                const QString displayName = reg::readStringAt(svc.get(), L"DisplayName", true);
                QString description       = reg::readStringAt(svc.get(), L"Description", true);
                if (description.startsWith(QLatin1Char('@'))) description.clear();
                StartupEntry e;
                e.id           = QStringLiteral("dsvc|%1").arg(name);
                e.displayName  = displayName.isEmpty() ? name : displayName;
                e.command      = imagePath;
                e.resolvedPath = resolveServiceExe(imagePath);
                e.fileName     = e.resolvedPath.isEmpty()
                                     ? QString{}
                                     : QFileInfo(e.resolvedPath).fileName();
                e.location     = Location::DelayedStartService;
                e.enabled      = delayedActive;
                e.serviceName  = name;
                e.description  = description;
                if (!e.resolvedPath.isEmpty()) {
                    FileMeta meta;
                    gatherMeta(e.resolvedPath, meta);
                    if (e.description.isEmpty()) e.description = std::move(meta.description);
                    e.publisher  = std::move(meta.publisher);
                    e.signature  = meta.signature;
                    e.signerName = std::move(meta.signerName);
                    e.iconImage  = std::move(meta.iconImage);
                }
                out.append(std::move(e));
            }
        }
        ActionResult setDelayedStartServiceEnabled(const StartupEntry& entry, bool enable) {
            const QString subkey = QStringLiteral("SYSTEM\\CurrentControlSet\\Services\\")
                                 + entry.serviceName;
            RegKey key;
            const LSTATUS r = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                            subkey.toStdWString().c_str(),
                                            0, KEY_READ | KEY_SET_VALUE, key.addr());
            if (r == ERROR_FILE_NOT_FOUND) return ActionResult::NotFound;
            if (r == ERROR_ACCESS_DENIED)  return ActionResult::AccessDenied;
            if (r != ERROR_SUCCESS)        return ActionResult::OtherError;
            const DWORD newStart = enable ? SERVICE_AUTO_START : SERVICE_DISABLED;
            const LSTATUS w = RegSetValueExW(key.get(), L"Start", 0, REG_DWORD,
                                             reinterpret_cast<const BYTE*>(&newStart),
                                             sizeof(newStart));
            if (w == ERROR_ACCESS_DENIED) return ActionResult::AccessDenied;
            if (w != ERROR_SUCCESS)       return ActionResult::OtherError;
            return ActionResult::Ok;
        }
        bool taskHasLogonOrBootTrigger(IRegisteredTask* task) {
            ComPtr<ITaskDefinition> def;
            if (FAILED(task->get_Definition(def.addr())) || !def) return false;
            ComPtr<ITriggerCollection> triggers;
            if (FAILED(def->get_Triggers(triggers.addr())) || !triggers) return false;
            LONG count = 0;
            triggers->get_Count(&count);
            for (LONG i = 1; i <= count; ++i) {
                ComPtr<ITrigger> t;
                if (FAILED(triggers->get_Item(i, t.addr())) || !t) continue;
                TASK_TRIGGER_TYPE2 type{};
                if (FAILED(t->get_Type(&type))) continue;
                if (type == TASK_TRIGGER_LOGON || type == TASK_TRIGGER_BOOT) {
                    return true;
                }
            }
            return false;
        }
        QString taskExecAction(IRegisteredTask* task) {
            ComPtr<ITaskDefinition> def;
            if (FAILED(task->get_Definition(def.addr())) || !def) return {};
            ComPtr<IActionCollection> actions;
            if (FAILED(def->get_Actions(actions.addr())) || !actions) return {};
            LONG count = 0;
            actions->get_Count(&count);
            for (LONG j = 1; j <= count; ++j) {
                ComPtr<IAction> action;
                if (FAILED(actions->get_Item(j, action.addr())) || !action) continue;
                TASK_ACTION_TYPE atype{};
                if (FAILED(action->get_Type(&atype)) || atype != TASK_ACTION_EXEC) continue;
                ComPtr<IExecAction> exec;
                if (FAILED(action->QueryInterface(IID_IExecAction,
                                                  reinterpret_cast<void**>(exec.addr())))
                        || !exec) {
                    continue;
                }
                BSTR pathBstr = nullptr;
                BSTR argsBstr = nullptr;
                exec->get_Path(&pathBstr);
                exec->get_Arguments(&argsBstr);
                const QString path = bstrToQString(pathBstr);
                const QString args = bstrToQString(argsBstr);
                SysFreeString(pathBstr);
                SysFreeString(argsBstr);
                if (!path.isEmpty()) {
                    return args.isEmpty() ? path : path + QLatin1Char(' ') + args;
                }
            }
            return {};
        }
        void enumerateTasksRecursive(ITaskFolder* folder, QVector<StartupEntry>& out) {
            if (!folder) return;
            ComPtr<IRegisteredTaskCollection> tasks;
            if (SUCCEEDED(folder->GetTasks(TASK_ENUM_HIDDEN, tasks.addr())) && tasks) {
                LONG count = 0;
                tasks->get_Count(&count);
                for (LONG i = 1; i <= count; ++i) {
                    ComPtr<IRegisteredTask> task;
                    VARIANT idx;
                    VariantInit(&idx);
                    idx.vt   = VT_I4;
                    idx.lVal = i;
                    const HRESULT hr = tasks->get_Item(idx, task.addr());
                    VariantClear(&idx);
                    if (FAILED(hr) || !task) continue;
                    if (!taskHasLogonOrBootTrigger(task.get())) continue;
                    BSTR nameBstr = nullptr;
                    BSTR pathBstr = nullptr;
                    task->get_Name(&nameBstr);
                    task->get_Path(&pathBstr);
                    const QString taskName = bstrToQString(nameBstr);
                    const QString taskPath = bstrToQString(pathBstr);
                    SysFreeString(nameBstr);
                    SysFreeString(pathBstr);
                    VARIANT_BOOL en = VARIANT_FALSE;
                    task->get_Enabled(&en);
                    const QString cmd = taskExecAction(task.get());
                    const QString exe = resolveExePath(cmd);
                    StartupEntry e;
                    e.id           = QStringLiteral("task|%1").arg(taskPath);
                    e.displayName  = taskName;
                    e.command      = cmd;
                    e.resolvedPath = exe;
                    e.fileName     = exe.isEmpty() ? QString{} : QFileInfo(exe).fileName();
                    e.location     = Location::ScheduledTask;
                    e.enabled      = (en == VARIANT_TRUE);
                    e.taskPath     = taskPath;
                    FileMeta meta;
                    gatherMeta(e.resolvedPath, meta);
                    e.publisher   = std::move(meta.publisher);
                    e.description = std::move(meta.description);
                    e.signature   = meta.signature;
                    e.signerName  = std::move(meta.signerName);
                    e.iconImage   = std::move(meta.iconImage);
                    out.append(std::move(e));
                }
            }
            ComPtr<ITaskFolderCollection> sub;
            if (SUCCEEDED(folder->GetFolders(0, sub.addr())) && sub) {
                LONG count = 0;
                sub->get_Count(&count);
                for (LONG i = 1; i <= count; ++i) {
                    ComPtr<ITaskFolder> child;
                    VARIANT idx;
                    VariantInit(&idx);
                    idx.vt   = VT_I4;
                    idx.lVal = i;
                    const HRESULT hr = sub->get_Item(idx, child.addr());
                    VariantClear(&idx);
                    if (FAILED(hr) || !child) continue;
                    enumerateTasksRecursive(child.get(), out);
                }
            }
        }
        void enumerateScheduledTasks(QVector<StartupEntry>& out) {
            ComPtr<ITaskService> svc;
            if (FAILED(CoCreateInstance(CLSID_TaskScheduler, nullptr, CLSCTX_INPROC_SERVER,
                                        IID_ITaskService,
                                        reinterpret_cast<void**>(svc.addr())))
                    || !svc) {
                qCWarning(catStartup) << "CoCreateInstance(TaskScheduler) failed";
                return;
            }
            VARIANT none;
            VariantInit(&none);
            if (FAILED(svc->Connect(none, none, none, none))) {
                qCWarning(catStartup) << "ITaskService::Connect failed";
                return;
            }
            ComPtr<ITaskFolder> root;
            BSTR rootPath = SysAllocString(L"\\");
            const HRESULT hr = svc->GetFolder(rootPath, root.addr());
            SysFreeString(rootPath);
            if (FAILED(hr) || !root) {
                qCWarning(catStartup) << "ITaskService::GetFolder(root) failed";
                return;
            }
            enumerateTasksRecursive(root.get(), out);
        }
        ActionResult setTaskEnabled(const StartupEntry& e, bool enabled) {
            ComScope com;
            if (!com.ok()) return ActionResult::OtherError;
            ComPtr<ITaskService> svc;
            if (FAILED(CoCreateInstance(CLSID_TaskScheduler, nullptr, CLSCTX_INPROC_SERVER,
                                        IID_ITaskService,
                                        reinterpret_cast<void**>(svc.addr())))
                    || !svc) {
                return ActionResult::OtherError;
            }
            VARIANT none;
            VariantInit(&none);
            if (FAILED(svc->Connect(none, none, none, none))) {
                return ActionResult::OtherError;
            }
            const int slash = e.taskPath.lastIndexOf(QLatin1Char('\\'));
            const QString folderPath = (slash <= 0) ? QStringLiteral("\\")
                                                    : e.taskPath.left(slash);
            const QString leafName   = (slash < 0)  ? e.taskPath
                                                    : e.taskPath.mid(slash + 1);
            ComPtr<ITaskFolder> folder;
            BSTR fp = SysAllocString(folderPath.toStdWString().c_str());
            HRESULT hr = svc->GetFolder(fp, folder.addr());
            SysFreeString(fp);
            if (FAILED(hr) || !folder) {
                return (hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED))
                    ? ActionResult::AccessDenied
                    : ActionResult::OtherError;
            }
            ComPtr<IRegisteredTask> task;
            BSTR ln = SysAllocString(leafName.toStdWString().c_str());
            hr = folder->GetTask(ln, task.addr());
            SysFreeString(ln);
            if (FAILED(hr) || !task) {
                return (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
                    ? ActionResult::NotFound
                    : ActionResult::OtherError;
            }
            hr = task->put_Enabled(enabled ? VARIANT_TRUE : VARIANT_FALSE);
            if (FAILED(hr)) {
                return (hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED))
                    ? ActionResult::AccessDenied
                    : ActionResult::OtherError;
            }
            return ActionResult::Ok;
        }
}
}

namespace vth::win::startup::self
{
namespace
{
        const QString kRunKeyPath = QStringLiteral(
            "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run");
        const QString kRunValueName = QStringLiteral("Vethida");
        void removeLegacyRunValue()
        {
            QSettings run(kRunKeyPath, QSettings::NativeFormat);
            if (run.contains(kRunValueName)) {
                run.remove(kRunValueName);
                run.sync();
            }
        }
}
}
