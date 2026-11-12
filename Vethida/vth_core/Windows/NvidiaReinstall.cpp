#include "NvidiaReinstall.h"

#include "Authenticode.h"
#include "NvidiaDriver.h"
#include "../Logger.h"
#include "../Runtime/Paths.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QProcess>
#include <QSaveFile>
#include <QStorageInfo>
#include <QThread>

#include <atomic>

#include <windows.h>
#include <setupapi.h>
#include <devguid.h>

#pragma comment(lib, "setupapi.lib")

namespace vth::win::nvidia::reinstall
{
    namespace
    {
        VTH_DEFINE_LOG_CATEGORY(catReinstall, "vth.nvidia.reinstall")

        QString markerPath() {
            return workspace() + QStringLiteral("/state.json");
        }

        QString system32(const wchar_t* exe) {
            wchar_t dir[MAX_PATH] = {};
            const UINT n = GetSystemDirectoryW(dir, MAX_PATH);
            if (n == 0 || n >= MAX_PATH)
                return QString::fromWCharArray(exe);
            return QString::fromWCharArray(dir, static_cast<int>(n)) + QLatin1Char('\\')
                 + QString::fromWCharArray(exe);
        }

        bool keyExists(HKEY root, const wchar_t* path) {
            HKEY k = nullptr;
            if (RegOpenKeyExW(root, path, 0, KEY_READ | KEY_WOW64_64KEY, &k) != ERROR_SUCCESS)
                return false;
            RegCloseKey(k);
            return true;
        }

        struct DwordValue {
            const wchar_t* key;
            const wchar_t* name;
            DWORD          held;
            QString        id;
        };

        const DwordValue kDelivery[] = {
            // 0 stops Windows Update fetching a driver for a device that has none.
            { L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DriverSearching",
              L"SearchOrderConfig", 0, QStringLiteral("searchOrder") },
            { L"SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate",
              L"ExcludeWUDriversInQualityUpdate", 1, QStringLiteral("excludeDrivers") },
        };

        bool readDword(const DwordValue& v, DWORD& out) {
            DWORD size = sizeof(out);
            return RegGetValueW(HKEY_LOCAL_MACHINE, v.key, v.name,
                                RRF_RT_REG_DWORD | RRF_SUBKEY_WOW6464KEY, nullptr, &out, &size)
                   == ERROR_SUCCESS;
        }

        bool writeDword(const DwordValue& v, DWORD value) {
            HKEY k = nullptr;
            if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, v.key, 0, nullptr, 0,
                                KEY_SET_VALUE | KEY_WOW64_64KEY, nullptr, &k, nullptr) != ERROR_SUCCESS)
                return false;
            const LONG r = RegSetValueExW(k, v.name, 0, REG_DWORD,
                                          reinterpret_cast<const BYTE*>(&value), sizeof(value));
            RegCloseKey(k);
            return r == ERROR_SUCCESS;
        }

        void deleteValue(const DwordValue& v) {
            HKEY k = nullptr;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, v.key, 0, KEY_SET_VALUE | KEY_WOW64_64KEY, &k)
                == ERROR_SUCCESS) {
                RegDeleteValueW(k, v.name);
                RegCloseKey(k);
            }
        }

        struct Run {
            bool    started = false;
            bool    finished = false;
            int     code = -1;
            QString output;
        };

        Run runTool(const QString& program, const QStringList& args, const QString& cwd,
                    int timeoutMs) {
            Run r;
            QProcess p;
            p.setProgram(program);
            p.setArguments(args);
            if (!cwd.isEmpty())
                p.setWorkingDirectory(cwd);
            p.setProcessChannelMode(QProcess::MergedChannels);
            p.start(QIODevice::ReadOnly);
            if (!p.waitForStarted(15000))
                return r;
            r.started = true;
            if (!p.waitForFinished(timeoutMs)) {
                qCWarning(catReinstall) << program << "still running after" << timeoutMs << "ms";
                return r;
            }
            r.finished = p.exitStatus() == QProcess::NormalExit;
            r.code = p.exitCode();
            r.output = QString::fromLocal8Bit(p.readAll()).trimmed();
            return r;
        }

        // Binding finishes in the background after an installer exits.
        bool waitForVersion(const QString& expected, int seconds) {
            for (int i = 0; i < seconds; ++i) {
                const QString bound = boundDriverVersion();
                if (!bound.isEmpty() && (expected.isEmpty() || bound == expected))
                    return true;
                QThread::sleep(1);
            }
            return false;
        }
    }

    QString workspace() {
        const QString dir = vth::paths::userData() + QStringLiteral("/nvidia-reinstall");
        QDir().mkpath(dir);
        return dir;
    }

    Preflight preflight(qint64 downloadedBytes) {
        Preflight p;

        SYSTEM_POWER_STATUS power{};
        if (GetSystemPowerStatus(&power) && power.ACLineStatus == 0
            && power.BatteryLifePercent != 255 && power.BatteryLifePercent < 30) {
            p.blocker = QStringLiteral("The battery is at %1% and the charger is unplugged. Plug "
                                       "the PC in first: losing power while the driver is being "
                                       "replaced leaves the screen on a basic driver.")
                            .arg(power.BatteryLifePercent);
            return p;
        }

        if (keyExists(HKEY_LOCAL_MACHINE,
                      L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Component Based Servicing\\RebootPending")
            || keyExists(HKEY_LOCAL_MACHINE,
                         L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate\\Auto Update\\RebootRequired")) {
            p.blocker = QStringLiteral("Windows is waiting for a restart to finish installing "
                                       "updates. Restart first, then run the reinstall: a driver "
                                       "install on top of a pending update can fail halfway.");
            return p;
        }

        // The package unpacks to about twice its download, the installer stages its
        // own copy, and the exported old driver is about 1.5 GB.
        const qint64 backup = 1536ll * 1024 * 1024;
        const qint64 need = qMax<qint64>(3ll * 1024 * 1024 * 1024, downloadedBytes * 3) + backup;
        const QStorageInfo work(workspace());
        if (work.isValid() && work.bytesAvailable() < need) {
            p.blocker = QStringLiteral("There is not enough free space (about %1 GB is needed). "
                                       "Free up some space, then try again.")
                            .arg((need + 1024ll * 1024 * 1024 - 1) / (1024ll * 1024 * 1024));
            return p;
        }

        if (!QFileInfo::exists(system32(L"pnputil.exe"))) {
            p.blocker = QStringLiteral("Windows' driver installer (pnputil) is missing from this PC, "
                                       "so there would be no way to put a driver back if the install "
                                       "failed. Use \"Run system file check\" in System > Fixes, then "
                                       "try again.");
            return p;
        }

        if (GetSystemMetrics(SM_REMOTESESSION))
            p.warnings << QStringLiteral("This is a remote session; the connection may drop while "
                                         "the display driver is replaced.");
        return p;
    }

    QString verifyPackage(const QString& extractDir) {
        const QString setup = extractDir + QStringLiteral("/setup.exe");
        if (!QFileInfo::exists(setup))
            return QStringLiteral("The downloaded package is incomplete (its installer is missing).");
        if (!vth::win::authenticode::signedBy(setup, QStringLiteral("NVIDIA Corporation")))
            return QStringLiteral("The downloaded package is not signed by NVIDIA, so Vethida did "
                                  "not use it. Nothing was changed.");
        const QDir driverDir(extractDir + QStringLiteral("/Display.Driver"));
        if (!driverDir.exists() || driverDir.entryList({ QStringLiteral("*.inf") }, QDir::Files).isEmpty())
            return QStringLiteral("The downloaded package has no display driver in it.");
        return {};
    }

    QString nvidiaVersionFromWindows(const QString& windowsVersion) {
        const QStringList parts = windowsVersion.split(QLatin1Char('.'));
        if (parts.size() != 4)
            return {};
        const QString digits = parts.at(2) + parts.at(3).rightJustified(4, QLatin1Char('0'));
        if (digits.size() < 5)
            return {};
        const QString last5 = digits.right(5);
        return QString::number(last5.left(3).toInt()) + QLatin1Char('.') + last5.right(2);
    }

    Marker readMarker() {
        Marker m;
        QFile f(markerPath());
        if (!f.open(QIODevice::ReadOnly))
            return m;
        const QJsonObject o = QJsonDocument::fromJson(f.readAll()).object();
        m.present        = !o.isEmpty();
        m.version        = o.value(QStringLiteral("version")).toString();
        m.extractDir     = o.value(QStringLiteral("extractDir")).toString();
        m.stage          = o.value(QStringLiteral("stage")).toString();
        m.startedUtc     = QDateTime::fromString(o.value(QStringLiteral("started")).toString(), Qt::ISODate);
        m.driverDelivery = o.value(QStringLiteral("driverDelivery")).toObject();
        m.previousDir     = o.value(QStringLiteral("previousDir")).toString();
        m.previousVersion = o.value(QStringLiteral("previousVersion")).toString();
        return m;
    }

    void writeMarker(const Marker& m) {
        QJsonObject o;
        o[QStringLiteral("version")]        = m.version;
        o[QStringLiteral("extractDir")]     = m.extractDir;
        o[QStringLiteral("stage")]          = m.stage;
        o[QStringLiteral("started")]        = m.startedUtc.toString(Qt::ISODate);
        o[QStringLiteral("driverDelivery")] = m.driverDelivery;
        o[QStringLiteral("previousDir")]     = m.previousDir;
        o[QStringLiteral("previousVersion")] = m.previousVersion;
        QSaveFile f(markerPath());
        if (!f.open(QIODevice::WriteOnly)
            || f.write(QJsonDocument(o).toJson(QJsonDocument::Compact)) < 0
            || !f.commit())
            qCWarning(catReinstall) << "could not save the reinstall state";
    }

    void clearWorkspace() {
        QDir(workspace()).removeRecursively();
    }

    QJsonObject holdDriverDelivery() {
        QJsonObject prior;
        for (const DwordValue& v : kDelivery) {
            DWORD current = 0;
            QJsonObject entry;
            entry[QStringLiteral("existed")] = readDword(v, current);
            entry[QStringLiteral("value")]   = static_cast<qint64>(current);
            prior[v.id] = entry;
            writeDword(v, v.held);
        }
        return prior;
    }

    void restoreDriverDelivery(const QJsonObject& prior) {
        for (const DwordValue& v : kDelivery) {
            if (!prior.contains(v.id))
                continue;
            const QJsonObject entry = prior.value(v.id).toObject();
            if (entry.value(QStringLiteral("existed")).toBool())
                writeDword(v, static_cast<DWORD>(entry.value(QStringLiteral("value")).toInteger()));
            else
                deleteValue(v);
        }
    }

    namespace
    {
        struct BoundDriver {
            QString version;   // NVIDIA's form
            QString infPath;   // e.g. "oem42.inf"
        };

        BoundDriver boundDriver() {
            BoundDriver found;
            HDEVINFO set = SetupDiGetClassDevsW(&GUID_DEVCLASS_DISPLAY, nullptr, nullptr, DIGCF_PRESENT);
            if (set == INVALID_HANDLE_VALUE)
                return found;
            SP_DEVINFO_DATA dev{};
            dev.cbSize = sizeof(dev);
            for (DWORD i = 0; SetupDiEnumDeviceInfo(set, i, &dev); ++i) {
                wchar_t hwid[2048] = {};
                if (!SetupDiGetDeviceRegistryPropertyW(set, &dev, SPDRP_HARDWAREID, nullptr,
                                                       reinterpret_cast<PBYTE>(hwid), sizeof(hwid) - sizeof(wchar_t), nullptr))
                    continue;
                if (!QString::fromWCharArray(hwid).contains(QLatin1String("VEN_10DE"), Qt::CaseInsensitive))
                    continue;
                HKEY drv = SetupDiOpenDevRegKey(set, &dev, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_READ);
                if (drv == INVALID_HANDLE_VALUE)
                    continue;
                wchar_t provider[256] = {};
                wchar_t version[64] = {};
                wchar_t inf[MAX_PATH] = {};
                DWORD psize = sizeof(provider) - sizeof(wchar_t);
                DWORD vsize = sizeof(version) - sizeof(wchar_t);
                DWORD isize = sizeof(inf) - sizeof(wchar_t);
                const bool hasProvider = RegQueryValueExW(drv, L"ProviderName", nullptr, nullptr,
                                                          reinterpret_cast<LPBYTE>(provider), &psize) == ERROR_SUCCESS;
                const bool hasVersion = RegQueryValueExW(drv, L"DriverVersion", nullptr, nullptr,
                                                         reinterpret_cast<LPBYTE>(version), &vsize) == ERROR_SUCCESS;
                const bool hasInf = RegQueryValueExW(drv, L"InfPath", nullptr, nullptr,
                                                     reinterpret_cast<LPBYTE>(inf), &isize) == ERROR_SUCCESS;
                RegCloseKey(drv);
                if (hasProvider && hasVersion
                    && QString::fromWCharArray(provider).contains(QLatin1String("NVIDIA"), Qt::CaseInsensitive)) {
                    found.version = nvidiaVersionFromWindows(QString::fromWCharArray(version));
                    if (hasInf)
                        found.infPath = QString::fromWCharArray(inf);
                    break;
                }
            }
            SetupDiDestroyDeviceInfoList(set);
            return found;
        }

        std::atomic<bool> g_running{ false };
    }

    QString boundDriverVersion() {
        return boundDriver().version;
    }

    bool isRunning() { return g_running.load(); }
    void setRunning(bool running) { g_running.store(running); }

    QString backupBoundDriver(QString* versionOut) {
        const BoundDriver bound = boundDriver();
        if (bound.version.isEmpty() || bound.infPath.isEmpty())
            return {};
        // Only a driver store package can be exported.
        if (!bound.infPath.startsWith(QLatin1String("oem"), Qt::CaseInsensitive))
            return {};

        const QString dir = workspace() + QStringLiteral("/previous");
        QDir(dir).removeRecursively();
        QDir().mkpath(dir);
        const Run exp = runTool(system32(L"pnputil.exe"),
                                { QStringLiteral("/export-driver"), bound.infPath,
                                  QDir::toNativeSeparators(dir) },
                                QString(), 10 * 60 * 1000);
        QDirIterator it(dir, { QStringLiteral("*.inf") }, QDir::Files, QDirIterator::Subdirectories);
        if (!exp.finished || exp.code != 0 || !it.hasNext()) {
            qCWarning(catReinstall).noquote()
                << "could not export" << bound.infPath << "code" << exp.code << exp.output.left(300);
            QDir(dir).removeRecursively();
            return {};
        }
        qCInfo(catReinstall).noquote() << "exported" << bound.infPath << "driver" << bound.version;
        if (versionOut)
            *versionOut = bound.version;
        return dir;
    }

    InstallOutcome installAndVerify(const QString& extractDir, const QString& expectedVersion,
                                    const QString& previousDir, const QString& previousVersion,
                                    const StageFn& stage) {
        InstallOutcome out;
        const auto say = [&stage](const QString& s) { if (stage) stage(s); };
        QStringList log;

        for (int attempt = 1; attempt <= 2; ++attempt) {
            say(attempt == 1 ? QStringLiteral("Installing the driver...")
                             : QStringLiteral("Installing the driver again..."));
            const vth::win::nvidia::InstallResult inst = vth::win::nvidia::runSilentInstall(extractDir);
            log << QStringLiteral("setup attempt %1: ok=%2 code=%3 %4")
                       .arg(attempt).arg(inst.ok).arg(inst.exitCode).arg(inst.error);
            say(QStringLiteral("Checking the new driver..."));
            if (waitForVersion(expectedVersion, 90)) {
                out.ok = true;
                out.restartRequired = true;
                out.message = QStringLiteral("Driver %1 is installed. Restart Windows to finish.")
                                  .arg(expectedVersion);
                out.detail = log.join(QStringLiteral("; "));
                return out;
            }
        }

        say(QStringLiteral("Installing the display driver directly..."));
        const Run pnp = runTool(system32(L"pnputil.exe"),
                                { QStringLiteral("/add-driver"),
                                  QDir::toNativeSeparators(extractDir + QStringLiteral("/Display.Driver/*.inf")),
                                  QStringLiteral("/install") },
                                QString(), 20 * 60 * 1000);
        log << QStringLiteral("pnputil: started=%1 finished=%2 code=%3 %4")
                   .arg(pnp.started).arg(pnp.finished).arg(pnp.code).arg(pnp.output.left(400));
        if (waitForVersion(expectedVersion, 90)) {
            out.ok = true;
            out.restartRequired = true;
            out.message = QStringLiteral("Driver %1 is installed (the display driver only; the "
                                         "NVIDIA installer would not finish). Restart Windows to "
                                         "finish.").arg(expectedVersion);
            out.detail = log.join(QStringLiteral("; "));
            return out;
        }

        if (boundDriverVersion().isEmpty() && !previousDir.isEmpty()
            && QFileInfo::exists(previousDir)) {
            say(QStringLiteral("Putting your previous driver back..."));
            const Run back = runTool(system32(L"pnputil.exe"),
                                     { QStringLiteral("/add-driver"),
                                       QDir::toNativeSeparators(previousDir + QStringLiteral("/*.inf")),
                                       QStringLiteral("/subdirs"), QStringLiteral("/install") },
                                     QString(), 20 * 60 * 1000);
            log << QStringLiteral("previous: started=%1 finished=%2 code=%3 %4")
                       .arg(back.started).arg(back.finished).arg(back.code).arg(back.output.left(400));
            if (waitForVersion(previousVersion, 90)) {
                out.restartRequired = true;
                out.restoredPrevious = true;
                out.detail = log.join(QStringLiteral("; "));
                qCWarning(catReinstall).noquote() << "new driver failed, previous restored:" << out.detail;
                out.message = QStringLiteral("Driver %1 would not install, so Vethida put back driver %2, "
                                             "the one you had before. Restart Windows to finish. "
                                             "Your graphics work as they did.")
                                  .arg(expectedVersion, previousVersion);
                return out;
            }
        }

        const QString bound = boundDriverVersion();
        out.detail = log.join(QStringLiteral("; ")) + QStringLiteral("; bound=") + bound;
        qCWarning(catReinstall).noquote() << "install not verified:" << out.detail;
        out.message = bound.isEmpty()
            ? QStringLiteral("The new driver did not install, and Windows is using its basic display "
                             "driver for now. Restart Windows, then use \"Finish the install\" here: "
                             "Vethida kept the prepared driver, so nothing has to be downloaded again.")
            : QStringLiteral("Windows kept driver %1 instead of %2. Restart Windows and use \"Finish the "
                             "install\" here to try again with the prepared driver.")
                  .arg(bound, expectedVersion);
        return out;
    }

    bool recoverAtStartup() {
        Marker m = readMarker();
        if (!m.present)
            return false;
        if (!m.driverDelivery.isEmpty()) {
            restoreDriverDelivery(m.driverDelivery);
            m.driverDelivery = QJsonObject();
            writeMarker(m);
            qCInfo(catReinstall) << "driver delivery handed back to Windows Update after an unfinished run";
        }
        if (m.extractDir.isEmpty() || !QFileInfo::exists(m.extractDir + QStringLiteral("/setup.exe"))) {
            clearWorkspace();
            return false;
        }
        // Already finished by another route (Windows Update, a manual install).
        if (!m.version.isEmpty() && boundDriverVersion() == m.version) {
            clearWorkspace();
            return false;
        }
        return true;
    }
}

namespace vth::win::nvidia::reinstall
{
    void clearMarker() {
        QFile::remove(markerPath());
    }
}

