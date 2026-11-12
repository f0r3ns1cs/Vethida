#include "ManagedSettings.h"

#include "../Logger.h"
#include "../Runtime/Paths.h"
#include "../Services/RegistryEditor.h"
#include "../Tweaks/TweakEngine.h"
#include "Registry.h"

#include <QByteArray>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QXmlStreamReader>

#include <algorithm>
#include <cstring>
#include <iterator>
#include <utility>
#include <vector>

#include <windows.h>

#include <wincrypt.h>

#include <lm.h>
#include <userenv.h>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "userenv.lib")

namespace vth::win::managed
{
    namespace
    {
        VTH_DEFINE_LOG_CATEGORY(catManaged, "vth.managed")

        QString systemRoot()
        {
            wchar_t dir[MAX_PATH] = {};
            const UINT n = GetWindowsDirectoryW(dir, MAX_PATH);
            if (n == 0 || n >= MAX_PATH)
                return QStringLiteral("C:/Windows");
            return QString::fromWCharArray(dir, static_cast<int>(n));
        }

        QString normalizeKey(QString key)
        {
            while (key.startsWith(QLatin1Char('\\')))
                key.remove(0, 1);
            while (key.endsWith(QLatin1Char('\\')))
                key.chop(1);
            return key.toLower();
        }

        struct Authority {
            bool    present = false;
            QString who;
        };

        // A failed lookup below counts as "not managed", so a failed call doesn't
        // block the repair on a personal PC.
        QString adDomainName()
        {
            using GetJoinFn = NET_API_STATUS(NET_API_FUNCTION*)(LPCWSTR, LPWSTR*, PNETSETUP_JOIN_STATUS);
            using FreeFn    = NET_API_STATUS(NET_API_FUNCTION*)(LPVOID);

            const HMODULE net = LoadLibraryExW(L"netapi32.dll", nullptr,
                                               LOAD_LIBRARY_SEARCH_SYSTEM32);
            if (!net)
                return {};

            QString name;
            const auto getJoin = reinterpret_cast<GetJoinFn>(
                GetProcAddress(net, "NetGetJoinInformation"));
            const auto freeBuf = reinterpret_cast<FreeFn>(
                GetProcAddress(net, "NetApiBufferFree"));
            if (getJoin && freeBuf) {
                LPWSTR               buffer = nullptr;
                NETSETUP_JOIN_STATUS status = NetSetupUnknownStatus;
                if (getJoin(nullptr, &buffer, &status) == NERR_Success) {
                    if (status == NetSetupDomainName && buffer)
                        name = QString::fromWCharArray(buffer);
                    if (buffer)
                        freeBuf(buffer);
                }
            }
            FreeLibrary(net);
            return name;
        }

        // Only a device join counts. A workplace join is just a work account.
        QString entraTenantName()
        {
            using GetAadFn  = NET_API_STATUS(NET_API_FUNCTION*)(LPCWSTR, PDSREG_JOIN_INFO*);
            using FreeAadFn = VOID(NET_API_FUNCTION*)(PDSREG_JOIN_INFO);

            const HMODULE net = LoadLibraryExW(L"netapi32.dll", nullptr,
                                               LOAD_LIBRARY_SEARCH_SYSTEM32);
            if (!net)
                return {};

            QString name;
            const auto getAad  = reinterpret_cast<GetAadFn>(
                GetProcAddress(net, "NetGetAadJoinInformation"));
            const auto freeAad = reinterpret_cast<FreeAadFn>(
                GetProcAddress(net, "NetFreeAadJoinInformation"));
            if (getAad && freeAad) {
                PDSREG_JOIN_INFO info = nullptr;
                if (getAad(nullptr, &info) == 0 && info) {
                    if (info->joinType == DSREG_DEVICE_JOIN) {
                        name = info->pszTenantDisplayName
                             ? QString::fromWCharArray(info->pszTenantDisplayName)
                             : QString();
                        if (name.isEmpty())
                            name = QStringLiteral("Microsoft Entra ID");
                    }
                    freeAad(info);
                }
            }
            FreeLibrary(net);
            return name;
        }

        // Not from the Enrollments registry key, which has entries even on a PC
        // that was never enrolled.
        bool mdmEnrolled()
        {
            using IsRegisteredFn = HRESULT(WINAPI*)(BOOL*, DWORD, LPWSTR);

            const HMODULE mdm = LoadLibraryExW(L"mdmregistration.dll", nullptr,
                                               LOAD_LIBRARY_SEARCH_SYSTEM32);
            if (!mdm)
                return false;

            bool enrolled = false;
            if (const auto isRegistered = reinterpret_cast<IsRegisteredFn>(
                    GetProcAddress(mdm, "IsDeviceRegisteredWithManagement"))) {
                BOOL registered = FALSE;
                if (SUCCEEDED(isRegistered(&registered, 0, nullptr)))
                    enrolled = registered != FALSE;
            }
            FreeLibrary(mdm);
            return enrolled;
        }

        Authority detectAuthority()
        {
            Authority a;
            if (const QString domain = adDomainName(); !domain.isEmpty()) {
                a.present = true;
                a.who     = QStringLiteral("the %1 domain").arg(domain);
                return a;
            }
            if (const QString tenant = entraTenantName(); !tenant.isEmpty()) {
                a.present = true;
                a.who     = tenant;
                return a;
            }
            if (mdmEnrolled()) {
                a.present = true;
                a.who     = QStringLiteral("a device management service");
            }
            return a;
        }

        // Which registry values are policy settings, according to Windows itself:
        // the .admx files and the PolicyManager map.
        struct PolicyIndex {
            QSet<QString> machineValues;    // "key\x1fvaluename", from the .admx corpus
            QSet<QString> userValues;
            QSet<QString> dedicatedValues;  // from PolicyManager, Software\Policies only
            QSet<QString> machineListKeys;  // a list element owns every value under its key
            QSet<QString> userListKeys;
            int           files = 0;

            bool empty() const {
                return machineValues.isEmpty() && userValues.isEmpty()
                    && dedicatedValues.isEmpty();
            }
        };

        bool underDedicatedTree(const QString& subkeyLower)
        {
            return subkeyLower.startsWith(QLatin1String("software\\policies\\"));
        }

        void addValue(PolicyIndex& index, const QString& policyClass,
                      const QString& key, const QString& valueName)
        {
            if (key.isEmpty() || valueName.isEmpty())
                return;
            const QString entry = normalizeKey(key) + QLatin1Char('\x1f') + valueName.toLower();
            if (policyClass != QLatin1String("User"))
                index.machineValues.insert(entry);
            if (policyClass != QLatin1String("Machine"))
                index.userValues.insert(entry);
        }

        void addListKey(PolicyIndex& index, const QString& policyClass, const QString& key)
        {
            if (key.isEmpty())
                return;
            const QString entry = normalizeKey(key);
            if (policyClass != QLatin1String("User"))
                index.machineListKeys.insert(entry);
            if (policyClass != QLatin1String("Machine"))
                index.userListKeys.insert(entry);
        }

        void readAdmxFile(const QString& path, PolicyIndex& index)
        {
            QFile f(path);
            if (!f.open(QIODevice::ReadOnly))
                return;

            QXmlStreamReader xml(&f);

            // An element's key is its own, else the nearest defaultKey, else the
            // key on the <policy>.
            QString     policyClass;
            QStringList keyStack;
            int         depth = 0;

            while (!xml.atEnd()) {
                const QXmlStreamReader::TokenType token = xml.readNext();

                if (token == QXmlStreamReader::StartElement) {
                    const QXmlStreamAttributes attrs = xml.attributes();
                    const QString name = xml.name().toString();

                    if (name == QLatin1String("policy")) {
                        policyClass = attrs.value(QLatin1String("class")).toString();
                        keyStack.clear();
                        keyStack.append(attrs.value(QLatin1String("key")).toString());
                        depth = 1;
                        addValue(index, policyClass, keyStack.constLast(),
                                 attrs.value(QLatin1String("valueName")).toString());
                        continue;
                    }
                    if (depth == 0)
                        continue;

                    ++depth;
                    QString inherited = keyStack.isEmpty() ? QString() : keyStack.constLast();
                    if (attrs.hasAttribute(QLatin1String("defaultKey")))
                        inherited = attrs.value(QLatin1String("defaultKey")).toString();
                    keyStack.append(inherited);

                    const QString own = attrs.hasAttribute(QLatin1String("key"))
                                      ? attrs.value(QLatin1String("key")).toString()
                                      : inherited;
                    if (name == QLatin1String("list"))
                        addListKey(index, policyClass, own);
                    else if (attrs.hasAttribute(QLatin1String("valueName")))
                        addValue(index, policyClass, own,
                                 attrs.value(QLatin1String("valueName")).toString());
                } else if (token == QXmlStreamReader::EndElement) {
                    if (depth == 0)
                        continue;
                    if (--depth == 0) {
                        policyClass.clear();
                        keyStack.clear();
                    } else if (!keyStack.isEmpty()) {
                        keyStack.removeLast();
                    }
                }
            }
        }

        // The setting, the value that blocks it, and the management stack's copy.
        void readPolicyManagerNode(HKEY node, PolicyIndex& index)
        {
            static const struct { const wchar_t* path; const wchar_t* name; } kPairs[] = {
                { L"grouppolicypath",    L"grouppolicyname" },
                { L"GPBlockingRegKeyPath", L"GPBlockingRegValueName" },
                { L"RegKeyPathRedirect", L"RegValueNameRedirect" },
            };
            for (const auto& pair : kPairs) {
                const QString path = vth::win::reg::readStringAt(node, pair.path);
                const QString name = vth::win::reg::readStringAt(node, pair.name);
                if (path.isEmpty() || name.isEmpty())
                    continue;
                index.dedicatedValues.insert(normalizeKey(path) + QLatin1Char('\x1f')
                                             + name.toLower());
            }
        }

        void readPolicyManagerMap(PolicyIndex& index)
        {
            HKEY defaults = nullptr;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                              L"SOFTWARE\\Microsoft\\PolicyManager\\default", 0,
                              KEY_READ | KEY_WOW64_64KEY, &defaults) != ERROR_SUCCESS)
                return;

            for (DWORD a = 0; ; ++a) {
                wchar_t area[256] = {};
                DWORD   areaLen = static_cast<DWORD>(std::size(area));
                if (RegEnumKeyExW(defaults, a, area, &areaLen,
                                  nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS)
                    break;

                HKEY areaKey = nullptr;
                if (RegOpenKeyExW(defaults, area, 0, KEY_READ | KEY_WOW64_64KEY,
                                  &areaKey) != ERROR_SUCCESS)
                    continue;

                for (DWORD p = 0; ; ++p) {
                    wchar_t policy[256] = {};
                    DWORD   policyLen = static_cast<DWORD>(std::size(policy));
                    if (RegEnumKeyExW(areaKey, p, policy, &policyLen,
                                      nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS)
                        break;
                    HKEY policyKey = nullptr;
                    if (RegOpenKeyExW(areaKey, policy, 0, KEY_READ | KEY_WOW64_64KEY,
                                      &policyKey) != ERROR_SUCCESS)
                        continue;
                    readPolicyManagerNode(policyKey, index);
                    RegCloseKey(policyKey);
                }
                RegCloseKey(areaKey);
            }
            RegCloseKey(defaults);
        }

        PolicyIndex readPolicyDefinitions()
        {
            PolicyIndex index;
            QDir dir(systemRoot() + QStringLiteral("/PolicyDefinitions"));
            const QStringList files = dir.entryList({ QStringLiteral("*.admx") }, QDir::Files);
            for (const QString& f : files) {
                readAdmxFile(dir.absoluteFilePath(f), index);
                ++index.files;
            }
            readPolicyManagerMap(index);
            return index;
        }

        struct PolicyRoot {
            HKEY    hive;
            QString hiveName;   // spelled the way a .reg file spells it
            QString subkey;
            bool    machine;    // which half of the ADMX index applies
        };

        const QVector<PolicyRoot>& policyRoots()
        {
            static const QVector<PolicyRoot> roots = {
                { HKEY_LOCAL_MACHINE, QStringLiteral("HKEY_LOCAL_MACHINE"),
                  QStringLiteral("SOFTWARE\\Policies"), true },
                { HKEY_LOCAL_MACHINE, QStringLiteral("HKEY_LOCAL_MACHINE"),
                  QStringLiteral("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies"), true },
                { HKEY_CURRENT_USER,  QStringLiteral("HKEY_CURRENT_USER"),
                  QStringLiteral("Software\\Policies"), false },
                { HKEY_CURRENT_USER,  QStringLiteral("HKEY_CURRENT_USER"),
                  QStringLiteral("Software\\Microsoft\\Windows\\CurrentVersion\\Policies"), false },
            };
            return roots;
        }

        // Removing anything here would change the PC's security settings.
        bool inProtectedSubtree(const QString& subkeyLower)
        {
            static const char* const kProtected[] = {
                "software\\policies\\microsoft\\systemcertificates",      // trusted and disallowed certificates
                "software\\policies\\microsoft\\enterprisecertificates",  // the same, per enterprise
                "software\\policies\\microsoft\\cryptography",            // cipher suite order, FIPS mode
                "software\\policies\\microsoft\\fve",                     // BitLocker on an encrypted volume
                "software\\policies\\microsoft\\windows\\ipsec",          // an assigned IPsec policy
                "software\\policies\\microsoft\\windows\\safer",          // software restriction rules
            };
            for (const char* p : kProtected) {
                const QString prefix = QString::fromLatin1(p);
                if (subkeyLower == prefix || subkeyLower.startsWith(prefix + QLatin1Char('\\')))
                    return true;
            }
            return false;
        }

        // Values Windows writes for itself, even with no Group Policy at all.
        bool isWindowsOwnValue(const QString& subkeyLower, const QString& nameLower)
        {
            struct Owned { const char* key; const char* name; };
            static const Owned kOwned[] = {
                { "software\\policies\\microsoft\\tpm", "osmanagedauthlevel" },
                { "software\\policies\\microsoft\\windows\\enhancedstoragedevices",
                  "tcgsecurityactivationdisabled" },
                { "software\\policies\\microsoft\\windows\\network connections",
                  "nc_personalfirewallconfig" },
            };
            for (const Owned& o : kOwned) {
                if (subkeyLower == QString::fromLatin1(o.key)
                    && nameLower == QString::fromLatin1(o.name))
                    return true;
            }
            return false;
        }

        struct FoundValue {
            HKEY       hive = nullptr;
            QString    hiveName;
            QString    subkey;      // full path under the hive
            QString    name;
            DWORD      type = REG_NONE;
            QByteArray data;
        };

        // Holds the buffers, because one 16K name per recursion level would
        // overflow the stack.
        struct Scan {
            QVector<FoundValue>  found;
            int                  leftAlone = 0;
            std::vector<wchar_t> nameBuf = std::vector<wchar_t>(16384);
            QByteArray           dataBuf;
        };

        constexpr int kMaxDepth = 64;

        void collectFrom(const PolicyRoot& root, const QString& subkey, int depth,
                         const PolicyIndex& index, Scan& scan)
        {
            if (depth > kMaxDepth)
                return;

            HKEY key = nullptr;
            if (RegOpenKeyExW(root.hive, reinterpret_cast<LPCWSTR>(subkey.utf16()), 0,
                              KEY_READ | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS)
                return;

            const QString        subkeyLower     = subkey.toLower();
            const bool           guarded         = inProtectedSubtree(subkeyLower);
            const QSet<QString>& values          = root.machine ? index.machineValues
                                                                : index.userValues;
            const QSet<QString>& listKeys        = root.machine ? index.machineListKeys
                                                                : index.userListKeys;
            const bool           dedicated       = underDedicatedTree(subkeyLower);

            // A list policy owns every value under its key. Only under
            // Software\Policies, because Windows populates some list keys elsewhere.
            const bool wholeKeyIsPolicy = dedicated && listKeys.contains(subkeyLower);

            for (DWORD i = 0; ; ++i) {
                DWORD nameLen = static_cast<DWORD>(scan.nameBuf.size());
                DWORD type    = REG_NONE;
                DWORD dataLen = 0;
                LONG  rc = RegEnumValueW(key, i, scan.nameBuf.data(), &nameLen, nullptr,
                                         &type, nullptr, &dataLen);
                if (rc != ERROR_SUCCESS)
                    break;

                const QString name  = QString::fromWCharArray(scan.nameBuf.data(),
                                                              static_cast<int>(nameLen));
                const QString entry = subkeyLower + QLatin1Char('\x1f') + name.toLower();
                const bool described = wholeKeyIsPolicy
                    || values.contains(entry)
                    || (dedicated && index.dedicatedValues.contains(entry));
                if (!described)
                    continue;
                if (guarded || isWindowsOwnValue(subkeyLower, name.toLower())) {
                    ++scan.leftAlone;
                    continue;
                }

                scan.dataBuf.resize(static_cast<int>(dataLen));
                nameLen = static_cast<DWORD>(scan.nameBuf.size());
                rc = RegEnumValueW(key, i, scan.nameBuf.data(), &nameLen, nullptr, &type,
                                   dataLen > 0 ? reinterpret_cast<LPBYTE>(scan.dataBuf.data())
                                               : nullptr,
                                   &dataLen);
                if (rc != ERROR_SUCCESS)
                    continue;
                scan.dataBuf.resize(static_cast<int>(dataLen));

                FoundValue v;
                v.hive     = root.hive;
                v.hiveName = root.hiveName;
                v.subkey   = subkey;
                v.name     = name;
                v.type     = type;
                v.data     = scan.dataBuf;
                scan.found.append(v);
            }

            QStringList children;
            for (DWORD i = 0; ; ++i) {
                wchar_t childBuf[256] = {};
                DWORD   childLen = static_cast<DWORD>(std::size(childBuf));
                if (RegEnumKeyExW(key, i, childBuf, &childLen,
                                  nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS)
                    break;
                children.append(QString::fromWCharArray(childBuf, static_cast<int>(childLen)));
            }
            RegCloseKey(key);

            for (const QString& child : children)
                collectFrom(root, subkey + QLatin1Char('\\') + child, depth + 1, index, scan);
        }

        QString regEscape(QString s)
        {
            s.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
            s.replace(QLatin1Char('"'), QStringLiteral("\\\""));
            return s;
        }

        QString hexBody(const QByteArray& data)
        {
            QStringList bytes;
            bytes.reserve(data.size());
            for (const char c : data)
                bytes.append(QString::asprintf("%02x", static_cast<unsigned char>(c)));
            return bytes.join(QLatin1Char(','));
        }

        // Wraps long hex the way regedit does.
        QString wrapHex(const QString& prefix, const QString& body)
        {
            constexpr int kWidth = 76;
            QString out  = prefix;
            int     used = prefix.size();
            const QStringList parts = body.split(QLatin1Char(','), Qt::SkipEmptyParts);
            for (int i = 0; i < parts.size(); ++i) {
                const QString piece = parts.at(i)
                    + (i + 1 < parts.size() ? QStringLiteral(",") : QString());
                if (used + piece.size() > kWidth) {
                    out += QStringLiteral("\\\r\n  ");
                    used = 2;
                }
                out  += piece;
                used += piece.size();
            }
            return out;
        }

        QString regLine(const FoundValue& v)
        {
            const QString name = v.name.isEmpty()
                ? QStringLiteral("@")
                : QStringLiteral("\"%1\"").arg(regEscape(v.name));

            switch (v.type) {
            case REG_SZ: {
                QString text = QString::fromWCharArray(
                    reinterpret_cast<const wchar_t*>(v.data.constData()),
                    v.data.size() / static_cast<int>(sizeof(wchar_t)));
                text.remove(QChar(u'\0'));
                return name + QStringLiteral("=\"%1\"").arg(regEscape(text));
            }
            case REG_DWORD: {
                quint32 n = 0;
                if (v.data.size() >= static_cast<int>(sizeof(n)))
                    std::memcpy(&n, v.data.constData(), sizeof(n));
                return name + QString::asprintf("=dword:%08x", n);
            }
            case REG_EXPAND_SZ:
                return wrapHex(name + QStringLiteral("=hex(2):"), hexBody(v.data));
            case REG_BINARY:
                return wrapHex(name + QStringLiteral("=hex:"), hexBody(v.data));
            case REG_MULTI_SZ:
                return wrapHex(name + QStringLiteral("=hex(7):"), hexBody(v.data));
            case REG_QWORD:
                return wrapHex(name + QStringLiteral("=hex(b):"), hexBody(v.data));
            default:
                return wrapHex(name + QStringLiteral("=hex(%1):")
                                      .arg(static_cast<ulong>(v.type), 0, 16),
                               hexBody(v.data));
            }
        }

        bool writeRegBackup(const QString& path, const QVector<FoundValue>& values)
        {
            QString text = QStringLiteral("Windows Registry Editor Version 5.00\r\n");
            QString openKey;
            for (const FoundValue& v : values) {
                const QString full = v.hiveName + QLatin1Char('\\') + v.subkey;
                if (full != openKey) {
                    text += QStringLiteral("\r\n[%1]\r\n").arg(full);
                    openKey = full;
                }
                text += regLine(v) + QStringLiteral("\r\n");
            }

            QFile f(path);
            if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
                return false;
            const qint64 size = static_cast<qint64>(text.size()) * static_cast<qint64>(sizeof(char16_t));
            const bool wrote = f.write(QByteArray("\xFF\xFE", 2)) == 2
                && f.write(reinterpret_cast<const char*>(text.utf16()), size) == size;
            f.close();
            return wrote;
        }

        struct PolFile {
            QString path;
            QString gptIni;
            bool    machineHalf = true;
            QString backupName;
        };

        QVector<PolFile> localPolicyFiles()
        {
            const QString    sys32 = systemRoot() + QStringLiteral("/System32");
            QVector<PolFile> found;

            const auto consider = [&found](const QString& gpoRoot, const QString& half,
                                           const QString& label) {
                const QString path = gpoRoot + QLatin1Char('/') + half
                                   + QStringLiteral("/Registry.pol");
                if (!QFileInfo::exists(path))
                    return;
                PolFile p;
                p.path        = path;
                p.gptIni      = gpoRoot + QStringLiteral("/gpt.ini");
                p.machineHalf = half == QLatin1String("Machine");
                p.backupName  = label + QLatin1Char('-') + half + QStringLiteral("-Registry.pol");
                found.append(p);
            };

            const QString localGpo = sys32 + QStringLiteral("/GroupPolicy");
            consider(localGpo, QStringLiteral("Machine"), QStringLiteral("Local"));
            consider(localGpo, QStringLiteral("User"), QStringLiteral("Local"));

            // gpedit.msc's GPOs scoped to an account or group.
            QDir perUser(sys32 + QStringLiteral("/GroupPolicyUsers"));
            const QStringList sids = perUser.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            for (const QString& sid : sids) {
                const QString root = perUser.absoluteFilePath(sid);
                consider(root, QStringLiteral("Machine"), sid);
                consider(root, QStringLiteral("User"), sid);
            }
            return found;
        }

        // Only this bit, so the files stay hidden and system.
        void clearReadOnly(const QString& path)
        {
            const QString native = QDir::toNativeSeparators(path);
            const auto    wide   = reinterpret_cast<LPCWSTR>(native.utf16());
            const DWORD   attrs  = GetFileAttributesW(wide);
            if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_READONLY))
                SetFileAttributesW(wide, attrs & ~DWORD{ FILE_ATTRIBUTE_READONLY });
        }

        // Computer version in the low 16 bits, user in the high 16. Without a bump
        // Windows can treat the emptied store as unchanged.
        void bumpGptVersion(const QString& gptIni, bool machineHalf)
        {
            QFile f(gptIni);
            if (!f.open(QIODevice::ReadOnly))
                return;
            QString text = QString::fromLatin1(f.readAll());
            f.close();

            static const QRegularExpression re(
                QStringLiteral("(?im)^([ \\t]*Version[ \\t]*=[ \\t]*)(\\d+)[ \\t]*$"));
            const QRegularExpressionMatch m = re.match(text);
            if (!m.hasMatch())
                return;

            const quint32 current = m.captured(2).toUInt();
            const quint32 machine = current & 0xFFFFu;
            const quint32 user    = (current >> 16) & 0xFFFFu;
            const quint32 bumped  = machineHalf
                ? ((user << 16) | ((machine + 1) & 0xFFFFu))
                : ((((user + 1) & 0xFFFFu) << 16) | machine);

            text.replace(m.capturedStart(), m.capturedLength(),
                         m.captured(1) + QString::number(bumped));

            clearReadOnly(gptIni);
            if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                f.write(text.toLatin1());
                f.close();
            }
        }

        bool removePolFile(const QString& path)
        {
            clearReadOnly(path);
            return QFile::remove(path);
        }

        // PolicyManager also holds Windows' own defaults, so only touch what the
        // catalog wrote there, and only for tweaks that are fully applied.
        int revertOwnPolicyManagerWrites()
        {
            vth::services::RegistryEditor reg;
            int reverted = 0;

            for (const vth::tweaks::TweakSpec& spec : vth::tweaks::catalog()) {
                const bool touchesPolicyManager = std::any_of(
                    spec.registryOps.cbegin(), spec.registryOps.cend(),
                    [](const vth::tweaks::RegistryOp& op) {
                        return op.path.contains(QLatin1String("\\PolicyManager\\"),
                                                Qt::CaseInsensitive);
                    });
                if (!touchesPolicyManager)
                    continue;
                if (vth::tweaks::detect(spec, reg) != vth::tweaks::DetectState::On)
                    continue;

                for (const vth::tweaks::RegistryOp& op : spec.registryOps) {
                    if (!op.path.contains(QLatin1String("\\PolicyManager\\"), Qt::CaseInsensitive))
                        continue;
                    const bool ok = op.revertValue.has_value()
                        ? reg.setValue(op.path, op.name, *op.revertValue, op.kind)
                        : reg.deleteValue(op.path, op.name);
                    if (ok)
                        ++reverted;
                }
            }
            return reverted;
        }

        int deleteValues(const QVector<FoundValue>& values)
        {
            int removed = 0;
            for (const FoundValue& v : values) {
                HKEY key = nullptr;
                if (RegOpenKeyExW(v.hive, reinterpret_cast<LPCWSTR>(v.subkey.utf16()), 0,
                                  KEY_SET_VALUE | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS)
                    continue;
                const LONG rc = RegDeleteValueW(key, reinterpret_cast<LPCWSTR>(v.name.utf16()));
                RegCloseKey(key);
                if (rc == ERROR_SUCCESS)
                    ++removed;
            }
            return removed;
        }

        bool keyIsEmpty(HKEY hive, const QString& subkey)
        {
            HKEY key = nullptr;
            if (RegOpenKeyExW(hive, reinterpret_cast<LPCWSTR>(subkey.utf16()), 0,
                              KEY_READ | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS)
                return false;
            DWORD subKeys = 0;
            DWORD values  = 0;
            const LONG rc = RegQueryInfoKeyW(key, nullptr, nullptr, nullptr, &subKeys,
                                             nullptr, nullptr, &values,
                                             nullptr, nullptr, nullptr, nullptr);
            RegCloseKey(key);
            return rc == ERROR_SUCCESS && subKeys == 0 && values == 0;
        }

        // Only keys a value was taken from, because Windows ships some empty ones.
        void pruneEmptiedKeys(const QVector<FoundValue>& removed)
        {
            QSet<QString> candidates;
            for (const FoundValue& v : removed) {
                for (const PolicyRoot& root : policyRoots()) {
                    if (root.hive != v.hive
                        || !v.subkey.startsWith(root.subkey + QLatin1Char('\\'),
                                                Qt::CaseInsensitive))
                        continue;
                    QString path = v.subkey;
                    while (path.size() > root.subkey.size()) {
                        candidates.insert(root.hiveName + QLatin1Char('\x1f') + path);
                        const int cut = path.lastIndexOf(QLatin1Char('\\'));
                        if (cut <= 0)
                            break;
                        path = path.left(cut);
                    }
                }
            }

            QStringList ordered = QStringList(candidates.cbegin(), candidates.cend());
            std::sort(ordered.begin(), ordered.end(), [](const QString& a, const QString& b) {
                return a.count(QLatin1Char('\\')) > b.count(QLatin1Char('\\'));
            });

            for (const QString& entry : std::as_const(ordered)) {
                const int    split    = entry.indexOf(QLatin1Char('\x1f'));
                const QString hiveName = entry.left(split);
                const QString subkey   = entry.mid(split + 1);
                const HKEY   hive     = hiveName == QLatin1String("HKEY_LOCAL_MACHINE")
                                      ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
                if (!keyIsEmpty(hive, subkey))
                    continue;
                const int cut = subkey.lastIndexOf(QLatin1Char('\\'));
                if (cut <= 0)
                    continue;
                HKEY parent = nullptr;
                if (RegOpenKeyExW(hive, reinterpret_cast<LPCWSTR>(subkey.left(cut).utf16()), 0,
                                  KEY_READ | KEY_WOW64_64KEY, &parent) != ERROR_SUCCESS)
                    continue;
                RegDeleteKeyExW(parent, reinterpret_cast<LPCWSTR>(subkey.mid(cut + 1).utf16()),
                                KEY_WOW64_64KEY, 0);
                RegCloseKey(parent);
            }
        }
    }

    vth::win::fixes::FixResult restoreWindowsDefaults()
    {
        using vth::win::fixes::FixResult;
        FixResult result;

        const Authority authority = detectAuthority();
        if (authority.present) {
            result.message = QStringLiteral(
                "This PC is joined to %1, so those settings belong to whoever "
                "administers it. Nothing was changed.").arg(authority.who);
            return result;
        }

        const PolicyIndex index = readPolicyDefinitions();
        if (index.empty()) {
            result.message = QStringLiteral(
                "The Windows policy definitions could not be read, so there is no "
                "safe way to tell a leftover setting from one Windows needs.");
            qCWarning(catManaged) << "no policy definitions under" << systemRoot();
            return result;
        }

        Scan scan;
        for (const PolicyRoot& root : policyRoots())
            collectFrom(root, root.subkey, 0, index, scan);

        const QVector<PolFile> polFiles = localPolicyFiles();

        if (scan.found.isEmpty() && polFiles.isEmpty()) {
            result.ok      = true;
            result.message = QStringLiteral("No leftover policy settings were found.");
            return result;
        }

        const QString backupDir = vth::paths::protectedData()
            + QStringLiteral("/policy-backup/")
            + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"));
        const QString failedBackup = QStringLiteral(
            "A copy could not be saved to %1, so nothing was changed.").arg(backupDir);

        if (!vth::paths::ensureDir(backupDir)) {
            result.message = failedBackup;
            return result;
        }
        if (!scan.found.isEmpty()
            && !writeRegBackup(backupDir + QStringLiteral("/policy-settings.reg"), scan.found)) {
            result.message = failedBackup;
            return result;
        }
        for (const PolFile& p : polFiles) {
            if (!QFile::copy(p.path, backupDir + QLatin1Char('/') + p.backupName)) {
                result.message = failedBackup;
                return result;
            }
        }

        // The store goes first, or the next policy refresh writes the values back.
        int polRemoved = 0;
        for (const PolFile& p : polFiles) {
            if (!removePolFile(p.path)) {
                qCWarning(catManaged) << "could not remove" << p.path;
                continue;
            }
            ++polRemoved;
            bumpGptVersion(p.gptIni, p.machineHalf);
        }

        const int ownReverted = revertOwnPolicyManagerWrites();
        const int removed     = deleteValues(scan.found);
        pruneEmptiedKeys(scan.found);

        if (!RefreshPolicyEx(TRUE, RP_FORCE) || !RefreshPolicyEx(FALSE, RP_FORCE))
            qCWarning(catManaged) << "policy refresh returned" << GetLastError();

        qCInfo(catManaged).noquote()
            << QStringLiteral("policy reset: %1 values, %2 policy files, %3 management "
                              "entries, %4 left alone, %5 definitions read, copy in %6")
               .arg(removed).arg(polRemoved).arg(ownReverted)
               .arg(scan.leftAlone).arg(index.files).arg(backupDir);

        const int restored = removed + ownReverted;
        if (restored == 0 && polRemoved == 0) {
            result.message = QStringLiteral(
                "Windows would not let the policy settings be removed. Nothing was changed.");
            result.detail  = QStringLiteral("A copy is in %1.").arg(backupDir);
            return result;
        }
        result.ok          = true;
        result.needsReboot = true;
        result.message     = restored > 0
            ? QStringLiteral("Put %1 settings back to their Windows defaults.").arg(restored)
            : QStringLiteral("Cleared the group policy settings stored on this PC.");
        result.detail      = QStringLiteral("A copy is in %1.").arg(backupDir);
        return result;
    }
}
