#include "DeviceOps.h"

#include "../Runtime/Paths.h"
#include "../Windows/Privileges.h"

#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QMutex>
#include <QtCore/QProcess>
#include <QtCore/QRegularExpression>
#include <QtCore/QHash>
#include <QtCore/QUuid>
#include <QtCore/QSaveFile>

#include <string>
#include <vector>

#include <windows.h>
#include <powrprof.h>
#include "../Windows/BcdBackup.h"
#pragma comment(lib, "powrprof.lib")

#pragma comment(lib, "advapi32.lib")

namespace vth::tweaks::devices
{
    namespace
    {
        HKEY hiveFor(const QString& path, QString& subkeyOut) {
            const int slash = path.indexOf(QLatin1Char('\\'));
            const QString hive = slash < 0 ? path : path.left(slash);
            subkeyOut = slash < 0 ? QString() : path.mid(slash + 1);
            if (hive.compare(QStringLiteral("HKLM"), Qt::CaseInsensitive) == 0) return HKEY_LOCAL_MACHINE;
            if (hive.compare(QStringLiteral("HKCU"), Qt::CaseInsensitive) == 0) return HKEY_CURRENT_USER;
            return nullptr;
        }

        std::wstring wstr(const QString& s) { return s.toStdWString(); }

        // Bare names go to System32, because these run elevated.
        QString pinnedProgram(const QString& program) {
            if (program.contains(QLatin1Char('\\')) || program.contains(QLatin1Char('/')))
                return program;
            wchar_t dir[MAX_PATH] = {};
            const UINT n = GetSystemDirectoryW(dir, MAX_PATH);
            const QString system32 = (n > 0 && n < MAX_PATH)
                ? QString::fromWCharArray(dir, static_cast<int>(n))
                : QStringLiteral("C:\\Windows\\System32");
            QString exe = program;
            if (!exe.endsWith(QStringLiteral(".exe"), Qt::CaseInsensitive))
                exe += QStringLiteral(".exe");
            if (exe.compare(QStringLiteral("powershell.exe"), Qt::CaseInsensitive) == 0)
                return system32 + QStringLiteral("\\WindowsPowerShell\\v1.0\\powershell.exe");
            return system32 + QLatin1Char('\\') + exe;
        }

        QMutex g_snapMutex;

        QString snapshotFilePath() {
            return vth::paths::protectedData() + QStringLiteral("/device-tweak-state.json");
        }
    }

    QMap<QString, QString> loadSnapshot(const QString& tweakId) {
        QMutexLocker lock(&g_snapMutex);
        QMap<QString, QString> out;
        QFile f(snapshotFilePath());
        if (!f.open(QIODevice::ReadOnly)) return out;
        const auto doc = QJsonDocument::fromJson(f.readAll());
        if (!doc.isObject()) return out;
        const QJsonObject root = doc.object();
        const QJsonObject entry = root.value(tweakId).toObject();
        for (auto it = entry.begin(); it != entry.end(); ++it)
            out.insert(it.key(), it.value().toString());
        return out;
    }

    void saveSnapshot(const QString& tweakId, const QMap<QString, QString>& kv) {
        QMutexLocker lock(&g_snapMutex);
        vth::paths::ensureDir(vth::paths::protectedData());
        QJsonObject root;
        {
            QFile f(snapshotFilePath());
            if (f.open(QIODevice::ReadOnly)) {
                const auto doc = QJsonDocument::fromJson(f.readAll());
                if (doc.isObject()) root = doc.object();
            }
        }
        QJsonObject entry;
        for (auto it = kv.begin(); it != kv.end(); ++it)
            entry.insert(it.key(), it.value());
        root.insert(tweakId, entry);
        QSaveFile f(snapshotFilePath());
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            f.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
            f.commit();
        }
    }

    void clearSnapshot(const QString& tweakId) {
        QMutexLocker lock(&g_snapMutex);
        QFile f(snapshotFilePath());
        if (!f.open(QIODevice::ReadOnly)) return;
        auto doc = QJsonDocument::fromJson(f.readAll());
        f.close();
        if (!doc.isObject()) return;
        QJsonObject root = doc.object();
        if (!root.contains(tweakId)) return;
        root.remove(tweakId);
        QSaveFile w(snapshotFilePath());
        if (w.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            w.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
            w.commit();
        }
    }

    std::optional<quint32> regReadDword(const QString& path, const QString& name) {
        QString sub;
        HKEY hive = hiveFor(path, sub);
        if (!hive) return std::nullopt;
        HKEY key = nullptr;
        if (RegOpenKeyExW(hive, wstr(sub).c_str(), 0, KEY_READ, &key) != ERROR_SUCCESS)
            return std::nullopt;
        DWORD v = 0, sz = sizeof(v), type = 0;
        const LSTATUS rc = RegQueryValueExW(key, wstr(name).c_str(), nullptr, &type,
                                            reinterpret_cast<LPBYTE>(&v), &sz);
        RegCloseKey(key);
        if (rc != ERROR_SUCCESS || type != REG_DWORD) return std::nullopt;
        return static_cast<quint32>(v);
    }

    std::optional<QString> regReadString(const QString& path, const QString& name) {
        QString sub;
        HKEY hive = hiveFor(path, sub);
        if (!hive) return std::nullopt;
        HKEY key = nullptr;
        if (RegOpenKeyExW(hive, wstr(sub).c_str(), 0, KEY_READ, &key) != ERROR_SUCCESS)
            return std::nullopt;
        DWORD sz = 0, type = 0;
        if (RegQueryValueExW(key, wstr(name).c_str(), nullptr, &type, nullptr, &sz) != ERROR_SUCCESS
            || (type != REG_SZ && type != REG_EXPAND_SZ)) {
            RegCloseKey(key);
            return std::nullopt;
        }
        std::vector<wchar_t> buf(sz / sizeof(wchar_t) + 2, L'\0');
        DWORD got = sz;
        const LSTATUS rc = RegQueryValueExW(key, wstr(name).c_str(), nullptr, &type,
                                            reinterpret_cast<LPBYTE>(buf.data()), &got);
        RegCloseKey(key);
        if (rc != ERROR_SUCCESS) return std::nullopt;
        return QString::fromWCharArray(buf.data());
    }

    namespace
    {
        bool isHklm(const QString& path) {
            return path.startsWith(QStringLiteral("HKLM"), Qt::CaseInsensitive);
        }
        HKEY openWrite(const QString& path) {
            if (isHklm(path) && !vth::win::isElevated()) return nullptr;
            QString sub;
            HKEY hive = hiveFor(path, sub);
            if (!hive) return nullptr;
            HKEY key = nullptr;
            DWORD disp = 0;
            if (RegCreateKeyExW(hive, wstr(sub).c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE,
                                KEY_WRITE | KEY_READ | KEY_WOW64_64KEY, nullptr, &key, &disp)
                != ERROR_SUCCESS)
                return nullptr;
            return key;
        }
    }

    bool regWriteDword(const QString& path, const QString& name, quint32 value) {
        HKEY key = openWrite(path);
        if (!key) return false;
        const DWORD v = value;
        const LSTATUS rc = RegSetValueExW(key, wstr(name).c_str(), 0, REG_DWORD,
                                          reinterpret_cast<const BYTE*>(&v), sizeof(v));
        RegCloseKey(key);
        return rc == ERROR_SUCCESS;
    }

    bool regWriteString(const QString& path, const QString& name, const QString& value) {
        HKEY key = openWrite(path);
        if (!key) return false;
        const std::wstring w = wstr(value);
        const LSTATUS rc = RegSetValueExW(key, wstr(name).c_str(), 0, REG_SZ,
            reinterpret_cast<const BYTE*>(w.c_str()),
            static_cast<DWORD>((w.size() + 1) * sizeof(wchar_t)));
        RegCloseKey(key);
        return rc == ERROR_SUCCESS;
    }

    bool regDeleteValue(const QString& path, const QString& name) {
        if (isHklm(path) && !vth::win::isElevated()) return false;
        QString sub;
        HKEY hive = hiveFor(path, sub);
        if (!hive) return false;
        HKEY key = nullptr;
        if (RegOpenKeyExW(hive, wstr(sub).c_str(), 0, KEY_SET_VALUE | KEY_WOW64_64KEY, &key)
            != ERROR_SUCCESS)
            return true;   // key gone => value gone
        const LSTATUS rc = RegDeleteValueW(key, wstr(name).c_str());
        RegCloseKey(key);
        return rc == ERROR_SUCCESS || rc == ERROR_FILE_NOT_FOUND;
    }

    QStringList regSubKeys(const QString& path) {
        QStringList out;
        QString sub;
        HKEY hive = hiveFor(path, sub);
        if (!hive) return out;
        HKEY key = nullptr;
        if (RegOpenKeyExW(hive, wstr(sub).c_str(), 0,
                          KEY_READ | KEY_ENUMERATE_SUB_KEYS | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS)
            return out;
        for (DWORD i = 0;; ++i) {
            wchar_t name[256] = {};
            DWORD len = 256;
            const LSTATUS rc = RegEnumKeyExW(key, i, name, &len, nullptr, nullptr, nullptr, nullptr);
            if (rc == ERROR_NO_MORE_ITEMS) break;
            if (rc == ERROR_SUCCESS) out << QString::fromWCharArray(name, static_cast<int>(len));
        }
        RegCloseKey(key);
        return out;
    }

    QVector<NicAdapter> enumeratePhysicalNics() {
        QVector<NicAdapter> out;
        const QString classRoot = QStringLiteral(
            "HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e972-e325-11ce-bfc1-08002be10318}");
        for (const QString& sub : regSubKeys(classRoot)) {
            bool numeric = sub.size() == 4;
            for (QChar c : sub) if (!c.isDigit()) numeric = false;
            if (!numeric) continue;
            const QString keyPath = classRoot + QLatin1Char('\\') + sub;
            const auto guid = regReadString(keyPath, QStringLiteral("NetCfgInstanceId"));
            if (!guid || guid->isEmpty()) continue;
            // Virtual adapters (loopback, WAN miniports, VPN) set *NdisDeviceType != 0.
            if (const auto ndt = regReadString(keyPath, QStringLiteral("*NdisDeviceType")))
                if (ndt->trimmed() != QStringLiteral("0")) continue;
            NicAdapter a;
            a.classKeyPath = keyPath;
            a.instanceGuid = *guid;
            a.description  = regReadString(keyPath, QStringLiteral("DriverDesc")).value_or(QString());
            out.push_back(a);
        }
        return out;
    }

    QStringList enumerateGpuClassKeys(const QString& venToken) {
        QStringList out;
        const QString classRoot = QStringLiteral(
            "HKLM\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}");
        const QString token = venToken.toUpper();
        for (const QString& sub : regSubKeys(classRoot)) {
            bool numeric = sub.size() == 4;
            for (QChar c : sub) if (!c.isDigit()) numeric = false;
            if (!numeric) continue;
            const QString keyPath = classRoot + QLatin1Char('\\') + sub;
            const auto did = regReadString(keyPath, QStringLiteral("MatchingDeviceId"));
            if (did && did->toUpper().contains(token))
                out << keyPath;
        }
        return out;
    }

    QVector<PciInstance> enumeratePciInstances() {
        QVector<PciInstance> out;
        const QString pciRoot = QStringLiteral("HKLM\\SYSTEM\\CurrentControlSet\\Enum\\PCI");
        for (const QString& dev : regSubKeys(pciRoot)) {
            const QString devPath = pciRoot + QLatin1Char('\\') + dev;
            for (const QString& inst : regSubKeys(devPath)) {
                const QString instPath = devPath + QLatin1Char('\\') + inst;
                PciInstance p;
                p.instanceKeyPath = instPath;
                p.service   = regReadString(instPath, QStringLiteral("Service")).value_or(QString()).toLower();
                p.classGuid = regReadString(instPath, QStringLiteral("ClassGUID")).value_or(QString()).toLower();
                out.push_back(p);
            }
        }
        return out;
    }

    bool runHidden(const QString& program, const QStringList& args, QString* out, int timeoutMs) {
        QProcess p;
        p.setProcessChannelMode(QProcess::MergedChannels);
        p.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments* a) {
            a->flags |= 0x08000000;   // CREATE_NO_WINDOW
        });
        p.start(pinnedProgram(program), args);
        if (!p.waitForStarted(10000)) return false;
        if (!p.waitForFinished(timeoutMs)) { p.kill(); p.waitForFinished(2000); return false; }
        if (out) *out = QString::fromLocal8Bit(p.readAll());
        return p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0;
    }

    bool runPowershell(const QString& script, QString* out, int timeoutMs) {
        return runHidden(QStringLiteral("powershell"),
            { QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"),
              QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"),
              QStringLiteral("-Command"), script }, out, timeoutMs);
    }

    namespace
    {
        // A setting named by GUID, or by one of the powercfg aliases the catalog uses.
        std::optional<GUID> powerGuid(const QString& nameOrGuid) {
            const QUuid u = QUuid::fromString(nameOrGuid.startsWith(QLatin1Char('{'))
                                                  ? nameOrGuid
                                                  : QLatin1Char('{') + nameOrGuid + QLatin1Char('}'));
            if (!u.isNull())
                return static_cast<GUID>(u);
            static const QHash<QString, QString> kAliases = {
                { QStringLiteral("SUB_PROCESSOR"), QStringLiteral("54533251-82be-4824-96c1-47b60b740d00") },
                { QStringLiteral("IDLEDISABLE"),   QStringLiteral("5d76a2ca-e8c0-402f-a133-2158492d58ad") },
            };
            const QString g = kAliases.value(nameOrGuid.toUpper());
            if (g.isEmpty())
                return std::nullopt;
            return static_cast<GUID>(QUuid::fromString(QLatin1Char('{') + g + QLatin1Char('}')));
        }
    }

    std::optional<quint32> powercfgQueryAcIndex(const QString& subAlias, const QString& settingAlias) {
        // The power API first, because powercfg's text depends on the display language.
        if (const auto sub = powerGuid(subAlias), setting = powerGuid(settingAlias); sub && setting) {
            GUID* active = nullptr;
            if (PowerGetActiveScheme(nullptr, &active) == ERROR_SUCCESS && active) {
                DWORD value = 0;
                const DWORD r = PowerReadACValueIndex(nullptr, active, &*sub, &*setting, &value);
                LocalFree(active);
                if (r == ERROR_SUCCESS)
                    return static_cast<quint32>(value);
            }
        }
        QString out;
        // /qh, because /query leaves out hidden settings like IDLEDISABLE.
        if (!runHidden(QStringLiteral("powercfg"),
                { QStringLiteral("/qh"), QStringLiteral("scheme_current"), subAlias, settingAlias },
                &out, 15000))
            return std::nullopt;
        static const QRegularExpression re(
            QStringLiteral("Current AC Power Setting Index:\\s*0x([0-9a-fA-F]+)"));
        const auto m = re.match(out);
        if (!m.hasMatch()) return std::nullopt;
        bool ok = false;
        const quint32 v = m.captured(1).toUInt(&ok, 16);
        return ok ? std::optional<quint32>(v) : std::nullopt;
    }

    bool powercfgSetAcIndex(const QString& subAlias, const QString& settingAlias, quint32 value) {
        const bool set = runHidden(QStringLiteral("powercfg"),
            { QStringLiteral("/setacvalueindex"), QStringLiteral("scheme_current"),
              subAlias, settingAlias, QString::number(value) }, nullptr, 15000);
        const bool act = runHidden(QStringLiteral("powercfg"),
            { QStringLiteral("/setactive"), QStringLiteral("scheme_current") }, nullptr, 15000);
        return set && act;
    }

    std::optional<bool> memoryCompressionEnabled() {
        QString out;
        if (!runPowershell(QStringLiteral("(Get-MMAgent).MemoryCompression"), &out, 30000))
            return std::nullopt;
        const QString t = out.trimmed();
        if (t.startsWith(QStringLiteral("True"), Qt::CaseInsensitive)) return true;
        if (t.startsWith(QStringLiteral("False"), Qt::CaseInsensitive)) return false;
        return std::nullopt;
    }

    bool setMemoryCompression(bool enabled) {
        return runPowershell(enabled
            ? QStringLiteral("Enable-MMAgent -MemoryCompression")
            : QStringLiteral("Disable-MMAgent -MemoryCompression"), nullptr, 30000);
    }

    std::optional<bool> pagefileIsAutomatic() {
        QString out;
        if (!runPowershell(
                QStringLiteral("(Get-CimInstance Win32_ComputerSystem).AutomaticManagedPagefile"),
                &out, 30000))
            return std::nullopt;
        const QString t = out.trimmed();
        if (t.startsWith(QStringLiteral("True"), Qt::CaseInsensitive)) return true;
        if (t.startsWith(QStringLiteral("False"), Qt::CaseInsensitive)) return false;
        return std::nullopt;
    }

    bool setFixedPagefile() {
        static const QString kScript = QStringLiteral(
            "$ErrorActionPreference='Stop';"
            "$cs=Get-WmiObject Win32_ComputerSystem;"
            "if($cs.AutomaticManagedPagefile){$cs.AutomaticManagedPagefile=$false;$cs.Put()|Out-Null}"
            "$ram=[math]::Round((Get-WmiObject Win32_ComputerSystem).TotalPhysicalMemory/1MB);"
            "$sz=[math]::Min([math]::Max($ram,4096),16384);"
            "$d=$env:SystemDrive+'\\pagefile.sys';"
            "$pf=Get-WmiObject Win32_PageFileSetting -Filter \"Name='$($d.Replace('\\','\\\\'))'\";"
            "if($pf){$pf.InitialSize=$sz;$pf.MaximumSize=$sz;$pf.Put()|Out-Null}"
            "else{Set-WmiInstance -Class Win32_PageFileSetting -Arguments @{Name=$d;InitialSize=$sz;MaximumSize=$sz}|Out-Null}");
        return runPowershell(kScript, nullptr, 30000);
    }

    bool setAutomaticPagefile() {
        static const QString kScript = QStringLiteral(
            "$ErrorActionPreference='Stop';"
            "$cs=Get-WmiObject Win32_ComputerSystem;"
            "if(-not $cs.AutomaticManagedPagefile){$cs.AutomaticManagedPagefile=$true;$cs.Put()|Out-Null}");
        return runPowershell(kScript, nullptr, 30000);
    }

    std::optional<quint64> physicalRamKb() {
        MEMORYSTATUSEX statex{};
        statex.dwLength = sizeof(statex);
        if (!GlobalMemoryStatusEx(&statex))
            return std::nullopt;
        return static_cast<quint64>(statex.ullTotalPhys / 1024ull);
    }
}

namespace vth::tweaks::devices
{
namespace
{
std::optional<QString> bcdParseElement(const QString& enumText, const QString& element) {
    const QStringList lines = enumText.split(QLatin1Char('\n'));
    for (const QString& raw : lines) {
        const QString line = raw.trimmed();
        if (!line.startsWith(element, Qt::CaseInsensitive))
            continue;
        const QString rest = line.mid(element.size());
        if (!rest.isEmpty() && !rest.at(0).isSpace())
            continue;
        return rest.trimmed();
    }
    return std::nullopt;
}
}
}

namespace vth::tweaks::devices
{
std::optional<QString> bcdReadElement(const QString& element) {
    QString out;
    if (!runHidden(QStringLiteral("bcdedit"),
            { QStringLiteral("/enum"), QStringLiteral("{current}") }, &out, 15000))
        return std::nullopt;
    return bcdParseElement(out, element);
}
bool bcdSetElement(const QString& element, const QString& value) {
    if (!vth::win::bcd::ensureBackupBeforeBootWrite())
        return false;
    return runHidden(QStringLiteral("bcdedit"),
        { QStringLiteral("/set"), QStringLiteral("{current}"), element, value }, nullptr, 15000);
}
bool bcdDeleteElement(const QString& element) {
    if (!vth::win::bcd::ensureBackupBeforeBootWrite())
        return false;
    return runHidden(QStringLiteral("bcdedit"),
        { QStringLiteral("/deletevalue"), QStringLiteral("{current}"), element }, nullptr, 15000);
}
}
