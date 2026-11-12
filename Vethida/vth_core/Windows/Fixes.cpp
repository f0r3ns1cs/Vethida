#include "Fixes.h"

#include "Genesis.h"
#include "ManagedSettings.h"
#include "Privileges.h"
#include "Registry.h"

#include "../Services.h"
#include "../Services/RegistryEditor.h"
#include "../Tweaks/TweakEngine.h"

#include <QByteArrayList>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QLocale>
#include <QProcess>
#include <QStringList>
#include <QVector>

#include <windows.h>

#include <winsock2.h>
#include <iphlpapi.h>
#include <netlistmgr.h>
#include <objbase.h>

#include <cwchar>
#include <initializer_list>
#include <iterator>
#include <string>
#include <utility>
#include "BcdBackup.h"

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ole32.lib")

namespace vth::win::fixes
{
    namespace
    {
        // Absolute paths, because these run elevated and PATH could be hijacked.
        QString system32(const wchar_t* exe) {
            wchar_t dir[MAX_PATH] = {};
            const UINT n = GetSystemDirectoryW(dir, MAX_PATH);
            if (n == 0 || n >= MAX_PATH) return QString::fromWCharArray(exe);
            return QString::fromWCharArray(dir) + QLatin1Char('\\') + QString::fromWCharArray(exe);
        }

        const QString kInternetSettingsKey = QStringLiteral(
            "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings");

        QString windowsDir(const wchar_t* exe) {
            wchar_t dir[MAX_PATH] = {};
            const UINT n = GetWindowsDirectoryW(dir, MAX_PATH);
            if (n == 0 || n >= MAX_PATH) return QString::fromWCharArray(exe);
            return QString::fromWCharArray(dir) + QLatin1Char('\\') + QString::fromWCharArray(exe);
        }

        struct ProcOut {
            bool    ran  = false;
            int     code = -1;
            QString out;
        };

        // A few tools, including sfc, write UTF-16 when piped.
        QString decodeOutput(const QByteArray& raw) {
            int zeros = 0;
            const int probe = qMin<int>(raw.size(), 400);
            for (int i = 1; i < probe; i += 2)
                if (raw.at(i) == '\0')
                    ++zeros;
            if (probe > 8 && zeros * 4 > probe) {
                QString s = QString::fromUtf16(reinterpret_cast<const char16_t*>(raw.constData()),
                                               raw.size() / 2);
                s.remove(QChar(0xFEFF));
                return s;
            }
            return QString::fromLocal8Bit(raw);
        }

        ProcOut runProcess(const QString& program, const QStringList& args, int timeoutMs) {
            QProcess p;
            p.setProcessChannelMode(QProcess::MergedChannels);
            p.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments* a) {
                a->flags |= CREATE_NO_WINDOW;
            });

            ProcOut r;
            p.start(program, args);
            if (!p.waitForStarted(10000)) {
                r.out = QStringLiteral("Could not start %1.").arg(program);
                return r;
            }
            if (!p.waitForFinished(timeoutMs)) {
                p.kill();
                p.waitForFinished(2000);
                r.out = QStringLiteral("%1 did not finish in time.").arg(program);
                return r;
            }
            r.ran  = true;
            r.code = p.exitCode();
            r.out  = decodeOutput(p.readAll()).trimmed();
            return r;
        }

        namespace svc = vth::win::services;

        // Only touches a disabled service. Falls back to the registry for services
        // whose permissions refuse the service manager.
        bool reenableIfDisabled(const QString& name, svc::StartType want, QStringList& changed) {
            if (svc::getStartType(name) != svc::StartType::Disabled)
                return false;
            svc::SetStartTypeResult r = svc::setStartType(name, want);
            if (r == svc::SetStartTypeResult::AccessDenied) {
                const QString path = QStringLiteral("SYSTEM\\CurrentControlSet\\Services\\") + name;
                HKEY key = nullptr;
                if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, reinterpret_cast<LPCWSTR>(path.utf16()),
                                  0, KEY_SET_VALUE, &key) == ERROR_SUCCESS) {
                    const DWORD start = (want == svc::StartType::Manual) ? 3 : 2;
                    if (RegSetValueExW(key, L"Start", 0, REG_DWORD,
                                       reinterpret_cast<const BYTE*>(&start), sizeof(start)) == ERROR_SUCCESS)
                        r = svc::SetStartTypeResult::Ok;
                    if (want == svc::StartType::AutoDelayed) {
                        const DWORD one = 1;
                        RegSetValueExW(key, L"DelayedAutostart", 0, REG_DWORD,
                                       reinterpret_cast<const BYTE*>(&one), sizeof(one));
                    }
                    RegCloseKey(key);
                }
            }
            if (r == svc::SetStartTypeResult::Ok) {
                changed << name;
                return true;
            }
            return false;
        }

        // The first of these tweaks that is applied, or empty. A fix that would undo
        // one defers to it, because the tweak's own undo restores what was there.
        QString appliedOwner(std::initializer_list<const char*> ids) {
            vth::services::RegistryEditor reg;
            for (const char* id : ids)
                if (const auto* spec = vth::tweaks::find(QString::fromLatin1(id)))
                    if (vth::tweaks::detect(*spec, reg) == vth::tweaks::DetectState::On)
                        return spec->name;
            return {};
        }

        FixResult deferToTweak(const QString& tweakName) {
            FixResult r;
            r.message = QStringLiteral("This is switched off by Vethida's \"%1\" tweak. Turn that tweak "
                                       "off on the Tweaks page instead; that puts back exactly what was "
                                       "there before.").arg(tweakName);
            return r;
        }

        QString turnedBackOn(const QStringList& changed) {
            if (changed.isEmpty())
                return {};
            return changed.size() == 1
                ? QStringLiteral("A disabled service was turned back on (%1).").arg(changed.first())
                : QStringLiteral("%1 disabled services were turned back on (%2).")
                      .arg(changed.size()).arg(changed.join(QStringLiteral(", ")));
        }

        constexpr int kShort  = 60'000;        // quick service / netsh calls
        constexpr int kMedium = 600'000;       // 10 min
        constexpr int kLong   = 3'600'000;     // 1 h (SFC + DISM)
        constexpr int kWipe   = 6 * 3'600'000; // 6 h ceiling for a secure wipe

        QString expandEnv(const QString& path) {
            wchar_t buf[MAX_PATH * 2] = {};
            const DWORD n = ExpandEnvironmentStringsW(
                reinterpret_cast<LPCWSTR>(path.utf16()), buf,
                static_cast<DWORD>(MAX_PATH * 2));
            if (n == 0 || n > MAX_PATH * 2)
                return path;
            return QString::fromWCharArray(buf);
        }

        void removeIfExists(const QString& path) {
            QFileInfo fi(path);
            if (!fi.exists())
                return;
            if (fi.isDir())
                QDir(path).removeRecursively();
            else
                QFile::remove(path);
        }

        quint64 directorySize(const QString& path) {
            quint64 total = 0;
            QDirIterator it(path, QDir::Files | QDir::Hidden | QDir::System,
                            QDirIterator::Subdirectories);
            while (it.hasNext()) {
                it.next();
                total += static_cast<quint64>(it.fileInfo().size());
            }
            return total;
        }

        // Leaves the directory itself. Returns the bytes removed.
        quint64 clearDirectory(const QString& path) {
            QDir dir(path);
            if (!dir.exists())
                return 0;
            quint64 freed = 0;
            const QFileInfoList entries = dir.entryInfoList(
                QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
            for (const QFileInfo& fi : entries) {
                const QString target = fi.absoluteFilePath();
                if (fi.isDir()) {
                    const quint64 inner = directorySize(target);
                    if (QDir(target).removeRecursively())
                        freed += inner;
                } else {
                    const quint64 size = static_cast<quint64>(fi.size());
                    if (QFile::remove(target))
                        freed += size;
                }
            }
            return freed;
        }

        QString humanBytes(quint64 bytes) {
            return QLocale::system().formattedDataSize(static_cast<qint64>(bytes));
        }

        QString localAppData() { return expandEnv(QStringLiteral("%LocalAppData%")); }
        QString systemRoot()   { return expandEnv(QStringLiteral("%SystemRoot%")); }

        // net, because unlike sc it waits and handles dependent services. A service
        // that's already stopped fails to stop, so failure isn't reported.
        void serviceStop(const QString& name) {
            runProcess(system32(L"net.exe"),
                       { QStringLiteral("stop"), name, QStringLiteral("/y") }, kShort);
        }
        bool serviceStart(const QString& name) {
            const ProcOut r = runProcess(system32(L"net.exe"),
                                         { QStringLiteral("start"), name }, kShort);
            // Already started counts. net's message depends on the display language.
            return r.ran && (r.code == 0
                             || vth::win::services::getState(name)
                                    == vth::win::services::State::Running);
        }

        QString powerShell() {
            return system32(L"WindowsPowerShell\\v1.0\\powershell.exe");
        }

        ProcOut runPowerShell(const QString& script, int timeoutMs) {
            return runProcess(powerShell(), {
                QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"),
                QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"),
                QStringLiteral("-Command"), script }, timeoutMs);
        }

        // "C:", "D:", ...
        QStringList fixedDrives() {
            QStringList out;
            wchar_t buf[512] = {};
            const DWORD n = GetLogicalDriveStringsW(511, buf);
            if (n == 0 || n > 511)
                return out;
            for (const wchar_t* p = buf; *p; p += wcslen(p) + 1) {
                if (GetDriveTypeW(p) != DRIVE_FIXED)
                    continue;
                QString root = QString::fromWCharArray(p);   // "C:\"
                if (root.endsWith(QLatin1Char('\\')))
                    root.chop(1);
                out << root;
            }
            return out;
        }

        bool deleteKeyTree(HKEY root, const QString& path) {
            return RegDeleteTreeW(root, reinterpret_cast<LPCWSTR>(path.utf16()))
                   == ERROR_SUCCESS;
        }

        void restartExplorer() {
            runProcess(system32(L"taskkill.exe"),
                       { QStringLiteral("/f"), QStringLiteral("/im"),
                         QStringLiteral("explorer.exe") }, kShort);
            QProcess::startDetached(windowsDir(L"explorer.exe"));
        }

        FixResult resetWindowsUpdate() {
            if (const auto* block = vth::tweaks::find(QStringLiteral("update.block-windows-update"))) {
                vth::services::RegistryEditor reg;
                if (vth::tweaks::detect(*block, reg) == vth::tweaks::DetectState::On) {
                    FixResult r;
                    r.message = QStringLiteral("Windows Update is switched off by Vethida's \"%1\" "
                                               "tweak. Turn that off on the Tweaks page first, then "
                                               "run this again.").arg(block->name);
                    return r;
                }
            }

            QStringList changed;
            reenableIfDisabled(QStringLiteral("wuauserv"), svc::StartType::Manual, changed);
            reenableIfDisabled(QStringLiteral("bits"), svc::StartType::Manual, changed);
            reenableIfDisabled(QStringLiteral("cryptsvc"), svc::StartType::Auto, changed);
            reenableIfDisabled(QStringLiteral("msiserver"), svc::StartType::Manual, changed);
            reenableIfDisabled(QStringLiteral("UsoSvc"), svc::StartType::AutoDelayed, changed);
            reenableIfDisabled(QStringLiteral("WaaSMedicSvc"), svc::StartType::Manual, changed);

            const QStringList services = { QStringLiteral("wuauserv"), QStringLiteral("bits"),
                                           QStringLiteral("cryptsvc"), QStringLiteral("msiserver") };
            for (const QString& s : services)
                runProcess(system32(L"net.exe"), { QStringLiteral("stop"), s }, kShort);

            const QString root = expandEnv(QStringLiteral("%SystemRoot%"));
            bool moved = false;
            QStringList inUse;
            for (const QString& dir : { QStringLiteral("SoftwareDistribution"),
                                        QStringLiteral("System32/catroot2") }) {
                const QString src = root + QLatin1Char('/') + dir;
                const QString bak = src + QStringLiteral(".vethida.old");
                QDir(bak).removeRecursively();
                if (!QFileInfo::exists(src))
                    continue;
                if (QDir().rename(src, bak))
                    moved = true;
                else
                    inUse << dir;
            }

            bool allStarted = true;
            for (const QString& s : services)
                allStarted = serviceStart(s) && allStarted;

            FixResult r;
            r.ok = allStarted;
            if (!allStarted) {
                r.needsReboot = true;
                r.message = QStringLiteral(
                    "Cleared the update cache, but an update service did not "
                    "come back up. Restart your PC to finish.");
            } else if (!inUse.isEmpty()) {
                r.message = QStringLiteral("The update services restarted, but the update cache is in "
                                           "use and was not cleared. Restart the PC, then run this again.");
            } else {
                r.message = moved
                    ? QStringLiteral("The update cache is cleared and the update services restarted. "
                                     "Check for updates again.")
                    : QStringLiteral("The update services restarted. There was no update cache to clear.");
            }
            r.detail = turnedBackOn(changed);
            return r;
        }

        FixResult repairNetworkStack() {
            const ProcOut winsock = runProcess(system32(L"netsh.exe"),
                { QStringLiteral("winsock"), QStringLiteral("reset") }, kShort);
            const ProcOut ip = runProcess(system32(L"netsh.exe"),
                { QStringLiteral("int"), QStringLiteral("ip"), QStringLiteral("reset") }, kShort);
            const ProcOut dns = runProcess(system32(L"ipconfig.exe"),
                { QStringLiteral("/flushdns") }, kShort);

            FixResult r;
            r.ok = winsock.ran && winsock.code == 0 && ip.ran;
            r.needsReboot = r.ok;
            if (r.ok)
                r.message = QStringLiteral("Winsock and TCP/IP are reset. Restart the PC to finish.");
            else if (!winsock.ran || winsock.code != 0)
                r.message = QStringLiteral("Windows would not reset Winsock. A VPN or security app "
                                           "that installs its own network filter can block it; "
                                           "uninstall it, restart, and try again.");
            else
                r.message = QStringLiteral("Windows would not run the TCP/IP reset. Restart the PC "
                                           "and try again.");
            r.detail = QStringList{ winsock.out, ip.out, dns.out }.join(QLatin1Char('\n')).trimmed();
            return r;
        }

        FixResult restoreBluetoothServices() {
            if (const QString owner = appliedOwner({ "op.svc.disable-bluetooth", "op.svc.disable-radio" }); !owner.isEmpty())
                return deferToTweak(owner);
            // The Start values of a clean Windows install.
            struct Svc { const wchar_t* name; DWORD start; };
            static const Svc kBtServices[] = {
                { L"BTHPORT",                    3 },
                { L"RmSvc",                      3 },
                { L"DeviceAssociationBrokerSvc", 2 },
                { L"DevicePickerUserSvc",        3 },
                { L"DevicesFlowUserSvc",         3 },
                { L"BthAvctpSvc",                3 },
                { L"BTAGService",                3 },
                { L"bthserv",                    3 },
                { L"BluetoothUserService",       3 },
                { L"ConsentUxUserSvc",           3 },
            };

            QStringList restored;
            QStringList absent;
            for (const Svc& s : kBtServices) {
                const QString path = QStringLiteral("SYSTEM\\CurrentControlSet\\Services\\")
                                   + QString::fromWCharArray(s.name);
                HKEY key = nullptr;
                // Don't create the key, because this PC may not have that component.
                if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, reinterpret_cast<LPCWSTR>(path.utf16()),
                                  0, KEY_SET_VALUE, &key) != ERROR_SUCCESS) {
                    absent <<QString::fromWCharArray(s.name);
                    continue;
                }
                const DWORD v = s.start;
                const LONG rc = RegSetValueExW(key, L"Start", 0, REG_DWORD,
                                               reinterpret_cast<const BYTE*>(&v), sizeof(v));
                RegCloseKey(key);
                (rc == ERROR_SUCCESS ? restored : absent) << QString::fromWCharArray(s.name);
            }

            FixResult r;
            r.ok          = !restored.isEmpty();
            r.needsReboot = true;
            r.message = r.ok
                ? (restored.size() == 1
                       ? QStringLiteral("Restored 1 Bluetooth service to its default startup.")
                       : QStringLiteral("Restored %1 Bluetooth services to their default startup.")
                             .arg(restored.size()))
                : QStringLiteral("No Bluetooth services were found to restore on this system.");
            if (!absent.isEmpty())
                r.detail = QStringLiteral("Skipped (not present or not writable): ")
                         + absent.join(QStringLiteral(", "));
            return r;
        }

        FixResult rebuildIconCache() {
            runProcess(system32(L"taskkill.exe"),
                { QStringLiteral("/f"), QStringLiteral("/im"), QStringLiteral("explorer.exe") }, kShort);

            const QString local = expandEnv(QStringLiteral("%LocalAppData%"));
            removeIfExists(local + QStringLiteral("/IconCache.db"));
            const QString explorerDir = local + QStringLiteral("/Microsoft/Windows/Explorer");
            QDir dir(explorerDir);
            if (dir.exists()) {
                const QStringList caches = dir.entryList(
                    { QStringLiteral("iconcache_*.db"), QStringLiteral("thumbcache_*.db") },
                    QDir::Files | QDir::Hidden);
                for (const QString& f : caches)
                    QFile::remove(dir.absoluteFilePath(f));
            }

            QProcess::startDetached(windowsDir(L"explorer.exe"));

            FixResult r;
            r.ok = true;
            r.message = QStringLiteral("Icon and thumbnail caches cleared; Explorer restarted.");
            return r;
        }

        // CBS.log is always in English, unlike sfc's console text.
        enum class SfcFinding { Unknown, Clean, Repaired, Unrepairable };
        SfcFinding sfcFindingFromCbsLog() {
            QFile log(expandEnv(QStringLiteral("%windir%\\Logs\\CBS\\CBS.log")));
            if (!log.open(QIODevice::ReadOnly))
                return SfcFinding::Unknown;
            const qint64 tail = 4 * 1024 * 1024;
            if (log.size() > tail)
                log.seek(log.size() - tail);
            const QString text = QString::fromUtf8(log.readAll());
            const qsizetype start = text.lastIndexOf(QLatin1String("[SR] Verifying"));
            if (start < 0)
                return SfcFinding::Unknown;
            const QString scan = text.mid(start);
            if (scan.contains(QLatin1String("[SR] Cannot repair")))
                return SfcFinding::Unrepairable;
            if (scan.contains(QLatin1String("[SR] Repairing")))
                return SfcFinding::Repaired;
            if (scan.contains(QLatin1String("[SR] Verify complete")))
                return SfcFinding::Clean;
            return SfcFinding::Unknown;
        }

        FixResult runSystemFileCheck() {
            const ProcOut dism = runProcess(system32(L"Dism.exe"),
                { QStringLiteral("/Online"), QStringLiteral("/Cleanup-Image"),
                  QStringLiteral("/RestoreHealth") }, kLong);
            const ProcOut sfc = runProcess(system32(L"sfc.exe"),
                { QStringLiteral("/scannow") }, kLong);

            // The exit code isn't reliable. The printed text is English only, so on
            // other languages the log answers instead.
            QString said = sfc.out.toLower();
            if (sfc.ran && !said.contains(QLatin1String("integrity violations"))
                && !said.contains(QLatin1String("repair"))) {
                switch (sfcFindingFromCbsLog()) {
                    case SfcFinding::Clean:
                        said = QStringLiteral("did not find any integrity violations"); break;
                    case SfcFinding::Repaired:
                        said = QStringLiteral("successfully repaired"); break;
                    case SfcFinding::Unrepairable:
                        said = QStringLiteral("unable to fix"); break;
                    case SfcFinding::Unknown: break;
                }
            }
            FixResult r;
            r.detail = QStringList{ dism.out.right(800), sfc.out.right(800) }
                           .join(QLatin1Char('\n')).trimmed();
            // The repair source couldn't be fetched from Windows Update.
            const bool dismNoSource = dism.ran
                && (dism.code == static_cast<int>(0x800F081F) || dism.code == static_cast<int>(0x800F0906));

            if (!sfc.ran) {
                r.message = QStringLiteral("The system file check could not run. Restart the PC and "
                                           "try again.");
            } else if (said.contains(QLatin1String("did not find any integrity violations"))) {
                r.ok = true;
                r.message = QStringLiteral("No damaged system files were found.");
            } else if (said.contains(QLatin1String("successfully repaired"))) {
                r.ok = true;
                r.needsReboot = true;
                r.message = QStringLiteral("Damaged system files were found and repaired. Restart "
                                           "the PC to finish.");
            } else if (said.contains(QLatin1String("unable to fix"))) {
                r.message = dismNoSource
                    ? QStringLiteral("Some damaged files could not be repaired because Windows could "
                                     "not download clean copies. Run \"Reset Windows Update\" above, "
                                     "then run this check again.")
                    : QStringLiteral("Some damaged files could not be repaired. Restart the PC and run "
                                     "this check once more; files in use are repaired on the next "
                                     "run.");
            } else if (said.contains(QLatin1String("repair pending"))
                       || said.contains(QLatin1String("could not perform"))) {
                r.message = QStringLiteral("Windows has a repair waiting for a restart. Restart the PC, "
                                           "then run this check again.");
            } else {
                r.message = QStringLiteral("The check finished without a clear result. Restart the PC "
                                           "and run it once more.");
            }
            return r;
        }

        FixResult scheduleDiskCheck() {
            const QString sysDrive = expandEnv(QStringLiteral("%SystemDrive%"));
            const ProcOut r0 = runProcess(system32(L"fsutil.exe"),
                { QStringLiteral("dirty"), QStringLiteral("set"), sysDrive }, kShort);

            FixResult r;
            r.ok = r0.ran && r0.code == 0;
            r.needsReboot = true;
            r.message = r.ok
                ? QStringLiteral("A disk check is queued for %1 on the next restart.").arg(sysDrive)
                : QStringLiteral("Windows would not mark %1 for a check. Restart the PC and try "
                                 "again.").arg(sysDrive);
            r.detail = r0.out;
            return r;
        }

        FixResult clearStandbyMemory() {
            const auto res = vth::win::genesis::purgeStandbyList();
            FixResult r;
            r.ok = res.ok;
            r.message = r.ok
                ? QStringLiteral("Standby memory list purged.")
                : (res.error.isEmpty() ? QStringLiteral("Could not purge the standby list.") : res.error);
            return r;
        }

        FixResult secureWipeFreeSpace() {
            const QString sysDrive = expandEnv(QStringLiteral("%SystemDrive%"));
            const ProcOut r0 = runProcess(system32(L"cipher.exe"),
                { QStringLiteral("/w:") + sysDrive + QLatin1Char('\\') }, kWipe);

            FixResult r;
            r.ok = r0.ran && r0.code == 0;
            r.message = r.ok
                ? QStringLiteral("Free space on %1 was securely overwritten.").arg(sysDrive)
                : QStringLiteral("The wipe stopped before it finished. Nothing was deleted; run it "
                                 "again to complete the overwrite.");
            r.detail = r0.out;
            return r;
        }

        // "No audio output device is installed" / no sound after sleep
        FixResult restartAudioServices() {
            QStringList changed;
            reenableIfDisabled(QStringLiteral("AudioEndpointBuilder"), svc::StartType::Auto, changed);
            reenableIfDisabled(QStringLiteral("Audiosrv"), svc::StartType::Auto, changed);

            serviceStop(QStringLiteral("AudioEndpointBuilder"));
            const bool started = serviceStart(QStringLiteral("Audiosrv"));

            FixResult r;
            r.ok = started;
            r.message = started
                ? QStringLiteral("Audio services restarted, and Windows is looking for your sound "
                                 "devices again.")
                : QStringLiteral("The audio service would not start. Restart the PC; if sound is "
                                 "still missing, run system file check below.");
            r.detail = turnedBackOn(changed);
            return r;
        }

        // A print job that will not cancel. The spooler holds the queue open, so
        // the files can only be removed while it is stopped.
        FixResult clearPrintQueue() {
            if (const QString owner = appliedOwner({ "op.svc.disable-print-spooler" }); !owner.isEmpty())
                return deferToTweak(owner);
            QStringList changed;
            reenableIfDisabled(QStringLiteral("Spooler"), svc::StartType::Auto, changed);
            serviceStop(QStringLiteral("Spooler"));
            const quint64 freed =
                clearDirectory(systemRoot() + QStringLiteral("/System32/spool/PRINTERS"));
            const bool started = serviceStart(QStringLiteral("Spooler"));

            FixResult r;
            r.ok = started;
            r.message = started
                ? QStringLiteral("Print queue cleared and the spooler restarted.")
                : QStringLiteral("Print queue cleared, but the print service would not start. "
                                 "Restart the PC to bring it back.");
            QStringList notes;
            if (freed > 0)
                notes << QStringLiteral("Removed %1 of queued job data.").arg(humanBytes(freed));
            if (!changed.isEmpty())
                notes << turnedBackOn(changed);
            r.detail = notes.join(QLatin1Char(' '));
            return r;
        }

        // "Connected, but no internet." Unlike the full stack repair, this needs no
        // restart.
        FixResult renewNetworkAddress() {
            if (svc::getStartType(QStringLiteral("Dhcp")) == svc::StartType::Disabled) {
                FixResult r;
                r.message = QStringLiteral("The DHCP client service is disabled, so this PC cannot get "
                                           "an address. Run \"Restore Wi-Fi and network services\" "
                                           "first.");
                return r;
            }
            runProcess(system32(L"ipconfig.exe"), { QStringLiteral("/release") }, kShort);
            const ProcOut renew = runProcess(system32(L"ipconfig.exe"),
                                             { QStringLiteral("/renew") }, kMedium);
            const ProcOut dns = runProcess(system32(L"ipconfig.exe"),
                                           { QStringLiteral("/flushdns") }, kShort);
            runProcess(system32(L"netsh.exe"),
                { QStringLiteral("interface"), QStringLiteral("ip"),
                  QStringLiteral("delete"), QStringLiteral("arpcache") }, kShort);

            FixResult r;
            r.ok = renew.ran && dns.ran;
            r.message = r.ok
                ? QStringLiteral("Address renewed and the DNS and ARP caches flushed.")
                : QStringLiteral("Windows would not renew the network address. Restart the router "
                                 "and the PC, then try \"Repair network stack\".");
            r.detail = QStringList{ renew.out, dns.out }.join(QLatin1Char('\n')).trimmed();
            return r;
        }

        QString regReadSz(HKEY key, const wchar_t* name) {
            DWORD type = 0, bytes = 0;
            if (RegQueryValueExW(key, name, nullptr, &type, nullptr, &bytes) != ERROR_SUCCESS)
                return {};
            if ((type != REG_SZ && type != REG_EXPAND_SZ) || bytes == 0)
                return {};
            QVector<wchar_t> buf(static_cast<int>(bytes / sizeof(wchar_t)) + 1, L'\0');
            if (RegQueryValueExW(key, name, nullptr, nullptr,
                                 reinterpret_cast<LPBYTE>(buf.data()), &bytes) != ERROR_SUCCESS)
                return {};
            return QString::fromWCharArray(buf.constData());
        }

        bool regReadDword(HKEY key, const wchar_t* name, DWORD& out) {
            DWORD type = 0, data = 0, bytes = sizeof(data);
            if (RegQueryValueExW(key, name, nullptr, &type,
                                 reinterpret_cast<LPBYTE>(&data), &bytes) != ERROR_SUCCESS)
                return false;
            if (type != REG_DWORD)
                return false;
            out = data;
            return true;
        }

        // Writes only if the value or its type differs. Returns whether it changed.
        bool ensureSz(HKEY key, const wchar_t* name, const QString& want) {
            DWORD type = 0, bytes = 0;
            const LSTATUS probe = RegQueryValueExW(key, name, nullptr, &type, nullptr, &bytes);
            if (probe == ERROR_SUCCESS && type == REG_SZ && regReadSz(key, name) == want)
                return false;
            const QByteArray raw(reinterpret_cast<const char*>(want.utf16()),
                                 (want.size() + 1) * static_cast<int>(sizeof(wchar_t)));
            return RegSetValueExW(key, name, 0, REG_SZ,
                                  reinterpret_cast<const BYTE*>(raw.constData()),
                                  static_cast<DWORD>(raw.size())) == ERROR_SUCCESS;
        }

        bool ensureDword(HKEY key, const wchar_t* name, DWORD want) {
            DWORD have = 0;
            if (regReadDword(key, name, have) && have == want)
                return false;
            return RegSetValueExW(key, name, 0, REG_DWORD,
                                  reinterpret_cast<const BYTE*>(&want), sizeof(want)) == ERROR_SUCCESS;
        }

        struct ScopedSc {
            SC_HANDLE h = nullptr;
            ScopedSc() = default;
            explicit ScopedSc(SC_HANDLE handle) : h(handle) {}
            ~ScopedSc() { if (h) CloseServiceHandle(h); }
            ScopedSc(const ScopedSc&) = delete;
            ScopedSc& operator=(const ScopedSc&) = delete;
        };

        bool waitForState(SC_HANDLE svc, DWORD want, int timeoutMs) {
            constexpr int kStep = 200;
            for (int waited = 0; waited <= timeoutMs; waited += kStep) {
                SERVICE_STATUS_PROCESS st{};
                DWORD needed = 0;
                if (!QueryServiceStatusEx(svc, SC_STATUS_PROCESS_INFO,
                                          reinterpret_cast<LPBYTE>(&st), sizeof(st), &needed))
                    return false;
                if (st.dwCurrentState == want)
                    return true;
                Sleep(kStep);
            }
            return false;
        }

        bool stopOne(SC_HANDLE svc) {
            SERVICE_STATUS_PROCESS st{};
            DWORD needed = 0;
            if (QueryServiceStatusEx(svc, SC_STATUS_PROCESS_INFO,
                                     reinterpret_cast<LPBYTE>(&st), sizeof(st), &needed)
                && st.dwCurrentState == SERVICE_STOPPED)
                return true;
            SERVICE_STATUS ss{};
            if (!ControlService(svc, SERVICE_CONTROL_STOP, &ss)
                && GetLastError() != ERROR_SERVICE_NOT_ACTIVE)
                return false;
            return waitForState(svc, SERVICE_STOPPED, 30'000);
        }

        bool startOne(SC_HANDLE svc) {
            SERVICE_STATUS_PROCESS st{};
            DWORD needed = 0;
            if (QueryServiceStatusEx(svc, SC_STATUS_PROCESS_INFO,
                                     reinterpret_cast<LPBYTE>(&st), sizeof(st), &needed)
                && st.dwCurrentState == SERVICE_RUNNING)
                return true;
            if (!StartServiceW(svc, 0, nullptr)
                && GetLastError() != ERROR_SERVICE_ALREADY_RUNNING)
                return false;
            return waitForState(svc, SERVICE_RUNNING, 30'000);
        }

        // Also stops and restarts the services that depend on this one.
        bool restartServiceTree(const QString& name, QStringList& strandedOut,
                                QString& errorOut) {
            const ScopedSc scm(OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT));
            if (!scm.h) {
                errorOut = QStringLiteral("Could not open the service manager.");
                return false;
            }
            const ScopedSc svc(OpenServiceW(
                scm.h, reinterpret_cast<LPCWSTR>(name.utf16()),
                SERVICE_QUERY_STATUS | SERVICE_ENUMERATE_DEPENDENTS
                    | SERVICE_STOP | SERVICE_START));
            if (!svc.h) {
                errorOut = QStringLiteral("Could not open the %1 service.").arg(name);
                return false;
            }

            QStringList dependents;
            DWORD needed = 0, count = 0;
            EnumDependentServicesW(svc.h, SERVICE_ACTIVE, nullptr, 0, &needed, &count);
            if (needed > 0) {
                QByteArray buf(static_cast<int>(needed), '\0');
                auto* arr = reinterpret_cast<LPENUM_SERVICE_STATUSW>(buf.data());
                if (EnumDependentServicesW(svc.h, SERVICE_ACTIVE, arr, needed,
                                           &needed, &count)) {
                    for (DWORD i = 0; i < count; ++i)
                        dependents << QString::fromWCharArray(arr[i].lpServiceName);
                }
            }

            for (const QString& dep : dependents) {
                const ScopedSc h(OpenServiceW(scm.h, reinterpret_cast<LPCWSTR>(dep.utf16()),
                                              SERVICE_QUERY_STATUS | SERVICE_STOP));
                if (h.h)
                    stopOne(h.h);
            }

            if (!stopOne(svc.h)) {
                errorOut = QStringLiteral("The %1 service would not stop.").arg(name);
                return false;
            }
            if (!startOne(svc.h)) {
                errorOut = QStringLiteral("The %1 service would not start again.").arg(name);
                return false;
            }

            for (auto it = dependents.crbegin(); it != dependents.crend(); ++it) {
                const ScopedSc h(OpenServiceW(scm.h, reinterpret_cast<LPCWSTR>(it->utf16()),
                                              SERVICE_QUERY_STATUS | SERVICE_START));
                if (!h.h || !startOne(h.h))
                    strandedOut << *it;
            }
            return true;
        }

        // Returns 0 if unknown.
        DWORD currentConnectivity() {
            const bool owned = SUCCEEDED(CoInitializeEx(nullptr, COINIT_MULTITHREADED));

            DWORD out = 0;
            INetworkListManager* nlm = nullptr;
            if (SUCCEEDED(CoCreateInstance(__uuidof(NetworkListManager), nullptr,
                                           CLSCTX_ALL, IID_PPV_ARGS(&nlm)))) {
                NLM_CONNECTIVITY c = NLM_CONNECTIVITY_DISCONNECTED;
                if (SUCCEEDED(nlm->GetConnectivity(&c)))
                    out = static_cast<DWORD>(c);
                nlm->Release();
            }
            if (owned)
                CoUninitialize();
            return out;
        }

        bool saysInternet(DWORD connectivity) {
            return (connectivity & (NLM_CONNECTIVITY_IPV4_INTERNET
                                    | NLM_CONNECTIVITY_IPV6_INTERNET)) != 0;
        }

        // Some tools block the connectivity check's hosts in the hosts file. Returns
        // the number of lines changed, or -1 with errorOut set.
        int unblockProbeHosts(const QString& path, QString& errorOut) {
            static const char* const kProbeHosts[] = {
                "msftconnecttest.com",      "www.msftconnecttest.com",
                "ipv6.msftconnecttest.com", "msftncsi.com",
                "www.msftncsi.com",         "dns.msftncsi.com",
                "ipv6.msftncsi.com",
            };
            const auto isProbeHost = [](const QByteArray& host) {
                for (const char* h : kProbeHosts)
                    if (host.compare(h, Qt::CaseInsensitive) == 0)
                        return true;
                return false;
            };

            QFile f(path);
            if (!f.exists())
                return 0;
            if (!f.open(QIODevice::ReadOnly)) {
                errorOut = QStringLiteral("Could not read the hosts file.");
                return -1;
            }
            const QByteArray original = f.readAll();
            f.close();

            QByteArrayList out;
            int changed = 0;
            const QByteArrayList lines = original.split('\n');
            for (const QByteArray& raw : lines) {
                const bool crlf = raw.endsWith('\r');
                const QByteArray line = crlf ? raw.left(raw.size() - 1) : raw;
                const QByteArray trimmed = line.trimmed();

                const auto emitLine = [&out, crlf](const QByteArray& text) {
                    out << (crlf ? text + '\r' : text);
                };

                if (trimmed.isEmpty() || trimmed.startsWith('#')) {
                    emitLine(line);
                    continue;
                }

                // "address host [host...] [# comment]"
                QByteArrayList fields;
                for (const QByteArray& tok : trimmed.split(' ')) {
                    for (const QByteArray& t : tok.split('\t')) {
                        const QByteArray v = t.trimmed();
                        if (!v.isEmpty())
                            fields << v;
                    }
                }
                int stop = fields.size();
                for (int i = 0; i < fields.size(); ++i) {
                    if (fields[i].startsWith('#')) { stop = i; break; }
                }
                if (stop < 2) {
                    emitLine(line);
                    continue;
                }

                QByteArrayList keep;
                bool hit = false;
                for (int i = 1; i < stop; ++i) {
                    if (isProbeHost(fields[i]))
                        hit = true;
                    else
                        keep << fields[i];
                }
                if (!hit) {
                    emitLine(line);
                    continue;
                }

                ++changed;
                emitLine("# Disabled by Vethida - blocked the Windows "
                         "connectivity check: " + line);
                if (!keep.isEmpty())
                    emitLine(fields[0] + '\t' + keep.join(' '));
            }

            if (changed == 0)
                return 0;

            QFile::remove(path + QStringLiteral(".vethida.bak"));
            if (!QFile::copy(path, path + QStringLiteral(".vethida.bak"))) {
                errorOut = QStringLiteral("Could not back up the hosts file, so it was not changed.");
                return -1;
            }

            // In place, so the hosts file keeps its own ACL.
            const auto wpath = reinterpret_cast<LPCWSTR>(path.utf16());
            const DWORD attrs = GetFileAttributesW(wpath);
            const bool readOnly = attrs != INVALID_FILE_ATTRIBUTES
                                  && (attrs & FILE_ATTRIBUTE_READONLY);
            if (readOnly)
                SetFileAttributesW(wpath, attrs & ~FILE_ATTRIBUTE_READONLY);

            bool wrote = false;
            if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                const QByteArray body = out.join('\n');
                wrote = f.write(body) == body.size();
                f.close();
            }
            if (readOnly)
                SetFileAttributesW(wpath, attrs);

            if (!wrote) {
                errorOut = QStringLiteral("Could not write the hosts file.");
                return -1;
            }
            return changed;
        }

        FixResult repairConnectivityDetection() {
            if (const QString owner = appliedOwner({ "op.net.connectivity-probe" }); !owner.isEmpty())
                return deferToTweak(owner);
            const bool internetBefore = saysInternet(currentConnectivity());
            QStringList done;

            // The probe settings Windows ships with.
            struct SzDefault { const wchar_t* name; const char* value; };
            static const SzDefault kDefaults[] = {
                { L"ActiveWebProbeHost",       "www.msftconnecttest.com"  },
                { L"ActiveWebProbePath",       "connecttest.txt"          },
                { L"ActiveWebProbeContent",    "Microsoft Connect Test"   },
                { L"ActiveWebProbeHostV6",     "ipv6.msftconnecttest.com" },
                { L"ActiveWebProbePathV6",     "connecttest.txt"          },
                { L"ActiveWebProbeContentV6",  "Microsoft Connect Test"   },
                { L"ActiveDnsProbeHost",       "dns.msftncsi.com"         },
                { L"ActiveDnsProbeContent",    "131.107.255.255"          },
                { L"ActiveDnsProbeHostV6",     "dns.msftncsi.com"         },
                { L"ActiveDnsProbeContentV6",  "fd3e:4f5a:5b81::1"        },
            };

            int probesFixed = 0;
            bool probingReEnabled = false;
            HKEY key = nullptr;
            const QString internetKey = QStringLiteral(
                "SYSTEM\\CurrentControlSet\\Services\\NlaSvc\\Parameters\\Internet");
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                              reinterpret_cast<LPCWSTR>(internetKey.utf16()),
                              0, KEY_QUERY_VALUE | KEY_SET_VALUE, &key) == ERROR_SUCCESS) {
                probingReEnabled = ensureDword(key, L"EnableActiveProbing", 1);
                for (const SzDefault& d : kDefaults) {
                    if (ensureSz(key, d.name, QString::fromLatin1(d.value)))
                        ++probesFixed;
                }
                RegCloseKey(key);
            }
            if (probingReEnabled)
                done << QStringLiteral("Turned the connectivity check back on.");
            if (probesFixed > 0)
                done << QStringLiteral("Restored %1 connectivity check setting%2 to the "
                                       "Windows default.")
                            .arg(probesFixed).arg(probesFixed == 1 ? "" : "s");

            // The policy overrides everything above.
            int policiesCleared = 0;
            const QString policyKey = QStringLiteral(
                "SOFTWARE\\Policies\\Microsoft\\Windows\\NetworkConnectivityStatusIndicator");
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                              reinterpret_cast<LPCWSTR>(policyKey.utf16()),
                              0, KEY_QUERY_VALUE | KEY_SET_VALUE, &key) == ERROR_SUCCESS) {
                // Only the two that switch the checks off. A managed PC may set others here.
                for (const wchar_t* name : { L"NoActiveProbe", L"DisablePassivePolling" }) {
                    DWORD value = 0;
                    if (regReadDword(key, name, value) && value != 0
                        && RegDeleteValueW(key, name) == ERROR_SUCCESS)
                        ++policiesCleared;
                }
                RegCloseKey(key);
            }
            if (policiesCleared > 0)
                done << QStringLiteral("Removed a policy that had switched the check off.");

            QString hostsError;
            const int unblocked = unblockProbeHosts(
                system32(L"drivers\\etc\\hosts"), hostsError);
            if (unblocked > 0)
                done << QStringLiteral("Unblocked %1 hosts file entr%2 that pointed the "
                                       "check at nowhere.")
                            .arg(unblocked).arg(unblocked == 1 ? "y" : "ies");

            runProcess(system32(L"ipconfig.exe"), { QStringLiteral("/flushdns") }, kShort);

            // The check is in NlaSvc on Windows 10 and netprofm on Windows 11.
            QStringList stranded;
            QString serviceError;
            const bool bounced =
                restartServiceTree(QStringLiteral("NlaSvc"), stranded, serviceError)
                && restartServiceTree(QStringLiteral("netprofm"), stranded, serviceError);

            FixResult r;
            r.ok = true;

            if (done.isEmpty()) {
                r.message = internetBefore
                    ? QStringLiteral("Connectivity detection was already set up correctly.")
                    : QStringLiteral("Connectivity detection is already set up correctly, so "
                                     "something outside this PC is blocking the check.");
            } else if (!bounced) {
                r.needsReboot = true;
                r.message = QStringLiteral("Connectivity detection restored. Restart to "
                                           "finish - the network status services could not "
                                           "be restarted now.");
            } else {
                // It re-checks on its own schedule, so give it a few seconds.
                bool internetAfter = false;
                for (int i = 0; i < 30 && !internetAfter; ++i) {
                    if (i)
                        Sleep(500);
                    internetAfter = saysInternet(currentConnectivity());
                }
                r.message = internetAfter
                    ? QStringLiteral("Connectivity detection restored. Windows reports "
                                     "internet access again.")
                    : QStringLiteral("Connectivity detection restored, but Windows still "
                                     "reports no internet. If everything else works, the "
                                     "check is being blocked outside this PC - a router "
                                     "filter, a DNS blocker such as Pi-hole, or a VPN.");
            }

            if (!hostsError.isEmpty())
                done << hostsError;
            if (!stranded.isEmpty())
                done << QStringLiteral("These services did not start again and may need a "
                                       "restart: %1.").arg(stranded.join(QStringLiteral(", ")));
            if (!bounced && !serviceError.isEmpty())
                done << serviceError;
            r.detail = done.join(QLatin1Char('\n'));
            return r;
        }

        // A proxy left behind by a VPN or similar, which makes every browser fail
        // while the connection itself is fine.
        FixResult clearProxySettings() {
            const ProcOut winhttp = runProcess(system32(L"netsh.exe"),
                { QStringLiteral("winhttp"), QStringLiteral("reset"),
                  QStringLiteral("proxy") }, kShort);

            bool userCleared = false;
            HKEY key = nullptr;
            const QString path = kInternetSettingsKey;
            if (RegOpenKeyExW(HKEY_CURRENT_USER, reinterpret_cast<LPCWSTR>(path.utf16()),
                              0, KEY_SET_VALUE, &key) == ERROR_SUCCESS) {
                const DWORD off = 0;
                userCleared = RegSetValueExW(key, L"ProxyEnable", 0, REG_DWORD,
                                             reinterpret_cast<const BYTE*>(&off),
                                             sizeof(off)) == ERROR_SUCCESS;
                RegDeleteValueW(key, L"ProxyServer");
                RegDeleteValueW(key, L"AutoConfigURL");
                RegCloseKey(key);
            }

            FixResult r;
            r.ok = winhttp.ran || userCleared;
            r.message = r.ok
                ? QStringLiteral("Proxy settings cleared for this account and for system services.")
                : QStringLiteral("Windows would not clear the proxy settings. A VPN or security "
                                 "app that sets the proxy may be putting it back.");
            r.detail = winhttp.out;
            return r;
        }

        // A firewall rule blocking a game or an app. Drops every custom rule.
        FixResult resetFirewallRules() {
            // The reset fails if either of these is disabled.
            QStringList changed;
            reenableIfDisabled(QStringLiteral("BFE"), svc::StartType::Auto, changed);
            reenableIfDisabled(QStringLiteral("mpssvc"), svc::StartType::Auto, changed);
            if (!changed.isEmpty()) {
                serviceStart(QStringLiteral("BFE"));
                serviceStart(QStringLiteral("mpssvc"));
            }
            const ProcOut r0 = runProcess(system32(L"netsh.exe"),
                { QStringLiteral("advfirewall"), QStringLiteral("reset") }, kShort);

            FixResult r;
            r.ok = r0.ran && r0.code == 0;
            r.message = r.ok
                ? QStringLiteral("Firewall rules restored to the Windows defaults. "
                                 "The firewall itself stays on.")
                : changed.isEmpty()
                    ? QStringLiteral("Windows would not reset the firewall rules. Security software "
                                     "that manages the firewall itself can block this.")
                    : QStringLiteral("The firewall services are turned back on. Restart the PC, then "
                                     "run this again to reset the rules.");
            r.needsReboot = !r.ok && !changed.isEmpty();
            r.detail = QString(turnedBackOn(changed) + QLatin1Char(' ') + r0.out).trimmed();
            return r;
        }

        // The Store showing a blank page or failing every download. Does what
        // wsreset.exe does, without it opening a Store window from an elevated process.
        FixResult resetStoreCache() {
            const QString cache = localAppData()
                + QStringLiteral("/Packages/Microsoft.WindowsStore_8wekyb3d8bbwe/LocalCache");
            if (!QFileInfo::exists(cache)) {
                FixResult r;
                r.ok = true;
                r.message = QStringLiteral("No Store cache was present to clear.");
                return r;
            }
            const quint64 freed = clearDirectory(cache);

            FixResult r;
            r.ok = true;
            r.message = QStringLiteral("Store cache cleared. Open the Store again to rebuild it.");
            if (freed > 0)
                r.detail = QStringLiteral("Removed %1.").arg(humanBytes(freed));
            return r;
        }

        // A dead Start menu, search box or taskbar. Re-registers the shell's packages
        // from the manifests already on disk.
        FixResult repairStartMenuAndSearch() {
            static const QString kScript = QStringLiteral(
                "$names = 'Microsoft.Windows.ShellExperienceHost',"
                "'Microsoft.Windows.StartMenuExperienceHost',"
                "'MicrosoftWindows.Client.CBS',"
                "'Microsoft.Windows.Search'; "
                "foreach ($n in $names) { "
                "Get-AppxPackage -AllUsers -Name $n | ForEach-Object { "
                "Add-AppxPackage -DisableDevelopmentMode -Register "
                "(Join-Path $_.InstallLocation 'AppXManifest.xml') "
                "-ErrorAction SilentlyContinue } }");

            const ProcOut ps = runPowerShell(kScript, kMedium);

            restartExplorer();

            FixResult r;
            r.ok = ps.ran;
            r.message = ps.ran
                ? QStringLiteral("Start menu, search, and the taskbar shell were re-registered.")
                : QStringLiteral("Windows PowerShell would not start, so the shell could not be "
                                 "registered again. Security software may be blocking it.");
            r.detail = ps.out;
            return r;
        }

        // A wrong clock also breaks HTTPS, so sign-ins and launchers fail.
        FixResult syncSystemClock() {
            if (const QString owner = appliedOwner({ "op.svc.time-sync-off" }); !owner.isEmpty())
                return deferToTweak(owner);
            // The service is trigger-started on a home machine and may be
            // stopped; a resync against a stopped service just fails.
            runProcess(system32(L"sc.exe"),
                { QStringLiteral("config"), QStringLiteral("w32time"),
                  QStringLiteral("start="), QStringLiteral("demand") }, kShort);
            serviceStart(QStringLiteral("w32time"));

            ProcOut sync = runProcess(system32(L"w32tm.exe"),
                { QStringLiteral("/resync"), QStringLiteral("/force") }, kShort);

            // A PC that has never synced has no peer configured.
            if (!sync.ran || sync.code != 0) {
                runProcess(system32(L"w32tm.exe"), {
                    QStringLiteral("/config"), QStringLiteral("/update"),
                    QStringLiteral("/manualpeerlist:time.windows.com,0x9"),
                    QStringLiteral("/syncfromflags:manual"),
                }, kShort);
                sync = runProcess(system32(L"w32tm.exe"),
                    { QStringLiteral("/resync"), QStringLiteral("/force") }, kShort);
            }

            FixResult r;
            r.ok = sync.ran && sync.code == 0;
            r.message = r.ok
                ? QStringLiteral("System clock synchronized with Microsoft's time servers.")
                : QStringLiteral("The clock could not be synchronized. Check that the date "
                                 "and time zone are roughly right first.");
            r.detail = sync.out;
            return r;
        }

        // "Every folder opens in the wrong view." Returns folders to their defaults.
        FixResult resetFolderViews() {
            static const char* kBagPaths[] = {
                "Software\\Microsoft\\Windows\\Shell\\BagMRU",
                "Software\\Microsoft\\Windows\\Shell\\Bags",
                "Software\\Microsoft\\Windows\\ShellNoRoam\\BagMRU",
                "Software\\Microsoft\\Windows\\ShellNoRoam\\Bags",
                "Software\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\Shell\\BagMRU",
                "Software\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\Shell\\Bags",
            };
            bool any = false;
            for (const char* p : kBagPaths)
                any = deleteKeyTree(HKEY_CURRENT_USER, QString::fromLatin1(p)) || any;

            restartExplorer();

            FixResult r;
            r.ok = true;
            r.message = any
                ? QStringLiteral("Folder view settings cleared; Explorer restarted.")
                : QStringLiteral("No stored folder views were found to clear.");
            return r;
        }

        // Search that returns nothing for files you can see. The flag below tells the
        // indexer its catalog is invalid.
        FixResult rebuildSearchIndex() {
            QStringList changed;
            reenableIfDisabled(QStringLiteral("WSearch"), svc::StartType::AutoDelayed, changed);
            serviceStop(QStringLiteral("WSearch"));

            bool flagged = false;
            HKEY key = nullptr;
            const QString path = QStringLiteral("SOFTWARE\\Microsoft\\Windows Search");
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, reinterpret_cast<LPCWSTR>(path.utf16()),
                              0, KEY_SET_VALUE, &key) == ERROR_SUCCESS) {
                const DWORD zero = 0;
                flagged = RegSetValueExW(key, L"SetupCompletedSuccessfully", 0, REG_DWORD,
                                         reinterpret_cast<const BYTE*>(&zero),
                                         sizeof(zero)) == ERROR_SUCCESS;
                RegCloseKey(key);
            }

            const QString db = expandEnv(QStringLiteral("%ProgramData%"))
                + QStringLiteral("/Microsoft/Search/Data/Applications/Windows/Windows.edb");
            const quint64 freed = QFileInfo::exists(db)
                ? static_cast<quint64>(QFileInfo(db).size()) : 0;
            removeIfExists(db);

            const bool started = serviceStart(QStringLiteral("WSearch"));

            FixResult r;
            r.ok = flagged || started;
            r.message = r.ok
                ? QStringLiteral("Search index cleared. Windows rebuilds it in the background; "
                                 "results will be incomplete for a while.")
                : QStringLiteral("Windows would not let the search index be cleared. Restart the "
                                 "PC and try again.");
            QStringList notes;
            if (freed > 0)
                notes << QStringLiteral("Removed a %1 index database.").arg(humanBytes(freed));
            if (!changed.isEmpty())
                notes << QStringLiteral("Windows Search had been turned off, and is on again.");
            r.detail = notes.join(QLatin1Char(' '));
            return r;
        }

        // garbled menu text. The font service has to be stopped to delete the cache.
        FixResult clearFontCache() {
            QStringList changed;
            reenableIfDisabled(QStringLiteral("FontCache"), svc::StartType::Auto, changed);
            serviceStop(QStringLiteral("FontCache"));
            serviceStop(QStringLiteral("FontCache3.0.0.0"));

            quint64 freed = clearDirectory(
                systemRoot()
                + QStringLiteral("/ServiceProfiles/LocalService/AppData/Local/FontCache"));
            const QString fnt = systemRoot() + QStringLiteral("/System32/FNTCACHE.DAT");
            if (QFileInfo::exists(fnt)) {
                const quint64 size = static_cast<quint64>(QFileInfo(fnt).size());
                if (QFile::remove(fnt))
                    freed += size;
            }

            const bool started = serviceStart(QStringLiteral("FontCache"));

            FixResult r;
            r.ok = started;
            r.message = started
                ? QStringLiteral("Font cache cleared and rebuilt. Sign out and back in if any "
                                 "text still looks wrong.")
                : QStringLiteral("Font cache cleared, but the font service would not start. "
                                 "Restart the PC to bring it back.");
            QStringList notes;
            if (freed > 0)
                notes << QStringLiteral("Removed %1.").arg(humanBytes(freed));
            if (!changed.isEmpty())
                notes << turnedBackOn(changed);
            r.detail = notes.join(QLatin1Char(' '));
            return r;
        }

        // "The Recycle Bin on C: is corrupted." Windows rebuilds the folder on the
        // next delete. This also discards whatever was still in there.
        FixResult repairRecycleBin() {
            QStringList repaired;
            QStringList failed;
            for (const QString& drive : fixedDrives()) {
                const QString bin = drive + QStringLiteral("/$Recycle.Bin");
                if (!QFileInfo::exists(bin))
                    continue;
                if (QDir(bin).removeRecursively())
                    repaired << drive;
                else
                    failed << drive;
            }

            FixResult r;
            r.ok = failed.isEmpty();
            r.message = r.ok
                ? (repaired.isEmpty()
                       ? QStringLiteral("No Recycle Bin needed repairing.")
                       : QStringLiteral("Recycle Bin rebuilt on %1.")
                             .arg(repaired.join(QStringLiteral(", "))))
                : QStringLiteral("Some drives could not be repaired while files were in use.");
            if (!failed.isEmpty())
                r.detail = QStringLiteral("Still in use: ") + failed.join(QStringLiteral(", "));
            return r;
        }

        // Stutter or corrupted textures after a driver update. These rebuild the next
        // time the game runs.
        FixResult clearShaderCaches() {
            const QString local = localAppData();
            const QString localLow = expandEnv(QStringLiteral("%UserProfile%"))
                                   + QStringLiteral("/AppData/LocalLow");
            const QStringList caches = {
                local + QStringLiteral("/D3DSCache"),
                local + QStringLiteral("/NVIDIA/DXCache"),
                local + QStringLiteral("/NVIDIA/GLCache"),
                local + QStringLiteral("/NVIDIA Corporation/NV_Cache"),
                local + QStringLiteral("/Temp/NVIDIA Corporation/NV_Cache"),
                localLow + QStringLiteral("/NVIDIA/PerDriverVersion/DXCache"),
                local + QStringLiteral("/AMD/DxCache"),
                local + QStringLiteral("/AMD/DxcCache"),
                local + QStringLiteral("/AMD/GLCache"),
                local + QStringLiteral("/AMD/VkCache"),
                local + QStringLiteral("/Intel/ShaderCache"),
            };

            quint64 freed = 0;
            for (const QString& c : caches)
                freed += clearDirectory(c);

            FixResult r;
            r.ok = true;
            r.message = freed > 0
                ? QStringLiteral("Shader caches cleared, freeing %1. The first launch of each "
                                 "game will be slower while they rebuild.").arg(humanBytes(freed))
                : QStringLiteral("No shader caches were present to clear.");
            return r;
        }

        // The classic NVIDIA Control Panel, and its entry on the desktop's
        // right-click menu.
        FixResult restoreNvidiaControlPanel() {
            const QString script = QStringLiteral(
                "$ErrorActionPreference='SilentlyContinue';"
                "$gpu=Get-CimInstance Win32_VideoController|?{$_.PNPDeviceID -match 'VEN_10DE'};"
                "if(-not $gpu){'NOGPU';exit 0}"
                "$svc=Get-Service -Name 'NVDisplay.ContainerLocalSystem';"
                "if($svc){Set-Service -Name $svc.Name -StartupType Automatic;Start-Service -Name $svc.Name}"
                "$pkg=Get-AppxPackage -Name 'NVIDIACorp.NVIDIAControlPanel';"
                "if(-not $pkg){"
                  "$wg=Join-Path $env:LOCALAPPDATA 'Microsoft\\WindowsApps\\winget.exe';"
                  "if(-not(Test-Path $wg)){$wg='winget'}"
                  "& $wg install --id 9NF8H0H7WMLT --source msstore --accept-package-agreements "
                  "--accept-source-agreements --disable-interactivity|Out-Null;"
                  "$pkg=Get-AppxPackage -Name 'NVIDIACorp.NVIDIAControlPanel'}"
                "if(-not $pkg){'NOPKG';exit 0}"
                "$pol='HKCU:\\Software\\NVIDIA Corporation\\Global\\NvCplApi\\Policies';"
                "New-Item -Path $pol -Force|Out-Null;"
                "Set-ItemProperty -Path $pol -Name ContextUIPolicy -Value 2 -Type DWord;"
                "$h='Registry::HKEY_CLASSES_ROOT\\Directory\\Background\\shellex\\ContextMenuHandlers\\NvCplDesktopContext';"
                "$native=$false;"
                "$clsid=(Get-Item -Path $h).GetValue('');"
                "if($clsid){$dll=(Get-Item -Path \"Registry::HKEY_CLASSES_ROOT\\CLSID\\$clsid\\InprocServer32\").GetValue('');"
                  "if($dll -and (Test-Path ([Environment]::ExpandEnvironmentVariables($dll)))){$native=$true}}"
                "$verb='HKLM:\\SOFTWARE\\Classes\\Directory\\Background\\shell\\NvidiaControlPanel';"
                "if($native){Remove-Item -Path $verb -Recurse -Force;'NATIVE';exit 0}"
                "$app=(Get-AppxPackageManifest -Package $pkg.PackageFullName).Package.Applications.Application|Select-Object -First 1;"
                "if(-not $app){'NOAPP';exit 0}"
                "$aumid=$pkg.PackageFamilyName+'!'+$app.Id;"
                "New-Item -Path \"$verb\\command\" -Force|Out-Null;"
                "Set-ItemProperty -Path $verb -Name MUIVerb -Value 'NVIDIA Control Panel';"
                "$icon=Join-Path $pkg.InstallLocation 'nvcplui.exe';"
                "if(Test-Path $icon){Set-ItemProperty -Path $verb -Name Icon -Value \"$icon,0\"}"
                "Set-ItemProperty -Path \"$verb\\command\" -Name '(default)' "
                "-Value \"$env:SystemRoot\\explorer.exe shell:AppsFolder\\$aumid\";"
                "'ADDED'");

            const ProcOut p = runPowerShell(script, kMedium);

            const QStringList lines = p.out.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
            const QString verdict = lines.isEmpty() ? QString() : lines.last().trimmed();

            FixResult r;
            if (!p.ran) {
                r.message = QStringLiteral("Windows PowerShell would not start, so the repair could "
                                           "not run. Security software may be blocking it.");
                r.detail  = p.out;
            } else if (verdict == QStringLiteral("NOGPU")) {
                r.message = QStringLiteral("No NVIDIA graphics card was found on this PC.");
            } else if (verdict == QStringLiteral("NOPKG")) {
                r.message = QStringLiteral(
                    "The NVIDIA Control Panel could not be installed from the Microsoft "
                    "Store. If the Store is missing or will not install anything, run "
                    "\"Repair the Microsoft Store\" first, then run this repair again.");
            } else if (verdict == QStringLiteral("NOAPP")) {
                r.message = QStringLiteral(
                    "The NVIDIA Control Panel is installed, but Windows did not report how "
                    "to open it, so the right-click entry was not added.");
            } else if (verdict == QStringLiteral("NATIVE")) {
                r.ok = true;
                r.message = QStringLiteral(
                    "The NVIDIA Control Panel is installed and its own right-click entry is "
                    "in place. On Windows 11 it is under \"Show more options\".");
            } else if (verdict == QStringLiteral("ADDED")) {
                r.ok = true;
                r.message = QStringLiteral(
                    "The NVIDIA Control Panel is installed and back on the desktop's "
                    "right-click menu. On Windows 11 it is under \"Show more options\".");
            } else {
                r.message = QStringLiteral("The repair did not report a result.");
                r.detail  = p.out;
            }
            return r;
        }

        FixResult clearTemporaryFiles() {
            const QStringList targets = {
                expandEnv(QStringLiteral("%TEMP%")),
                systemRoot() + QStringLiteral("/Temp"),
                expandEnv(QStringLiteral("%ProgramData%"))
                    + QStringLiteral("/Microsoft/Windows/WER/ReportQueue"),
                expandEnv(QStringLiteral("%ProgramData%"))
                    + QStringLiteral("/Microsoft/Windows/WER/ReportArchive"),
            };

            quint64 freed = 0;
            for (const QString& t : targets)
                freed += clearDirectory(t);

            FixResult r;
            r.ok = true;
            r.message = freed > 0
                ? QStringLiteral("Temporary files cleared, freeing %1.").arg(humanBytes(freed))
                : QStringLiteral("There were no temporary files worth clearing.");
            r.detail = QStringLiteral(
                "Files a running program still had open are left alone; run this again "
                "after a restart to catch them.");
            return r;
        }

        // Without /ResetBase, which would make installed updates impossible to remove.
        FixResult compactComponentStore() {
            const ProcOut r0 = runProcess(system32(L"Dism.exe"),
                { QStringLiteral("/Online"), QStringLiteral("/Cleanup-Image"),
                  QStringLiteral("/StartComponentCleanup") }, kLong);

            FixResult r;
            r.ok = r0.ran && r0.code == 0;
            r.message = r.ok
                ? QStringLiteral("Superseded component versions removed from the component store.")
                : QStringLiteral("Windows would not finish the cleanup, usually because an update is "
                                 "waiting for a restart. Restart the PC and try again.");
            r.detail = r0.out;
            return r;
        }

        // Also removes custom plans, including Vethida's.
        FixResult restoreDefaultPowerPlans() {
            const ProcOut r0 = runProcess(system32(L"powercfg.exe"),
                { QStringLiteral("-restoredefaultschemes") }, kShort);

            FixResult r;
            r.ok = r0.ran && r0.code == 0;
            r.message = r.ok
                ? QStringLiteral("Default power plans restored. Any custom plan, including "
                                 "Vethida's, has to be installed again.")
                : QStringLiteral("Windows would not restore the default power plans. If no power "
                                 "plan will change at all, run \"Restore the Power service\" first.");
            r.detail = r0.out;
            return r;
        }

        // Power plans that won't change and sleep settings that do nothing.
        FixResult restorePowerService() {
            QStringList changed;
            reenableIfDisabled(QStringLiteral("Power"), svc::StartType::Auto, changed);
            const bool running = serviceStart(QStringLiteral("Power"));
            FixResult r;
            r.ok = running || !changed.isEmpty();
            r.needsReboot = !running;
            r.message = running
                ? (changed.isEmpty() ? QStringLiteral("The Power service is running normally.")
                                     : QStringLiteral("The Power service is back on and running."))
                : QStringLiteral("The Power service is set to start with Windows again. Restart the "
                                 "PC to bring it up.");
            r.detail = turnedBackOn(changed);
            return r;
        }

        // "The Windows Installer service could not be accessed."
        FixResult restoreWindowsInstaller() {
            QStringList changed;
            reenableIfDisabled(QStringLiteral("msiserver"), svc::StartType::Manual, changed);
            const ProcOut reg = runProcess(system32(L"msiexec.exe"), { QStringLiteral("/regserver") }, kShort);
            const bool started = serviceStart(QStringLiteral("msiserver"));
            FixResult r;
            r.ok = started;
            r.message = started
                ? QStringLiteral("Windows Installer is working again. Run the installer that failed "
                                 "once more.")
                : QStringLiteral("Windows Installer is turned back on, but would not start yet. "
                                 "Restart the PC, then try the installer again.");
            r.needsReboot = !started;
            QStringList notes;
            if (!changed.isEmpty())
                notes << turnedBackOn(changed);
            if (!reg.ran || reg.code != 0)
                notes << reg.out;
            r.detail = notes.join(QLatin1Char(' ')).trimmed();
            return r;
        }

        // The Store broken by disabled services, or removed entirely.
        FixResult repairMicrosoftStore() {
            if (const QString owner = appliedOwner({ "op.svc.disable-store-appx", "op.svc.disable-msaccount" }); !owner.isEmpty())
                return deferToTweak(owner);
            QStringList changed;
            reenableIfDisabled(QStringLiteral("AppXSvc"),        svc::StartType::Manual, changed);
            reenableIfDisabled(QStringLiteral("ClipSVC"),        svc::StartType::Manual, changed);
            reenableIfDisabled(QStringLiteral("InstallService"), svc::StartType::Manual, changed);
            reenableIfDisabled(QStringLiteral("LicenseManager"), svc::StartType::Manual, changed);
            reenableIfDisabled(QStringLiteral("wlidsvc"),        svc::StartType::Manual, changed);
            reenableIfDisabled(QStringLiteral("TokenBroker"),    svc::StartType::Manual, changed);
            reenableIfDisabled(QStringLiteral("DoSvc"),          svc::StartType::AutoDelayed, changed);

            // Re-registered if it's on disk, otherwise reinstalled by wsreset.
            static const QString kScript = QStringLiteral(
                "$p = Get-AppxPackage -AllUsers -Name Microsoft.WindowsStore | Select-Object -First 1; "
                "if ($p) { Add-AppxPackage -DisableDevelopmentMode -Register "
                "(Join-Path $p.InstallLocation 'AppXManifest.xml') -ErrorAction SilentlyContinue; 'REGISTERED' } "
                "else { 'MISSING' }");
            const ProcOut ps = runPowerShell(kScript, kMedium);
            const bool missing = ps.out.contains(QStringLiteral("MISSING"));
            bool reinstallStarted = false;
            if (missing)
                reinstallStarted = QProcess::startDetached(system32(L"wsreset.exe"), { QStringLiteral("-i") });

            const QString cache = localAppData()
                + QStringLiteral("/Packages/Microsoft.WindowsStore_8wekyb3d8bbwe/LocalCache");
            clearDirectory(cache);

            FixResult r;
            r.ok = ps.ran && (!missing || reinstallStarted);
            r.needsReboot = !changed.isEmpty();
            if (!ps.ran)
                r.message = QStringLiteral("The Store repair could not run. Restart the PC and try again.");
            else if (missing)
                r.message = reinstallStarted
                    ? QStringLiteral("The Microsoft Store had been removed. Windows is reinstalling it in "
                                     "the background; it appears in the Start menu within a few minutes.")
                    : QStringLiteral("The Microsoft Store has been removed from this PC, and Windows would "
                                     "not start reinstalling it. Run \"Reset Windows Update\" above, then "
                                     "try again.");
            else
                r.message = QStringLiteral("The Microsoft Store and the services it installs apps with are "
                                           "repaired.");
            if (r.needsReboot && r.ok)
                r.message += QStringLiteral(" Restart the PC to finish.");
            r.detail = turnedBackOn(changed);
            return r;
        }

        // Settings that crashes on open, or opens to a blank window.
        FixResult repairSettingsApp() {
            static const QString kScript = QStringLiteral(
                "$n = 'windows.immersivecontrolpanel'; "
                "$p = Get-AppxPackage -AllUsers -Name $n | Select-Object -First 1; "
                "if ($p) { Add-AppxPackage -DisableDevelopmentMode -Register "
                "(Join-Path $p.InstallLocation 'AppXManifest.xml') -ErrorAction SilentlyContinue; 'OK' } "
                "else { 'MISSING' }");
            const ProcOut ps = runPowerShell(kScript, kMedium);
            FixResult r;
            r.ok = ps.ran && ps.out.contains(QStringLiteral("OK"));
            r.message = r.ok
                ? QStringLiteral("The Settings app is registered again. Open it from the Start menu.")
                : ps.out.contains(QStringLiteral("MISSING"))
                    ? QStringLiteral("The Settings app's files are missing. Run system file check below "
                                     "to restore them.")
                    : QStringLiteral("The Settings app could not be repaired. Restart the PC and try again.");
            return r;
        }

        // Apps that can't hear you or see the camera, after a privacy tool blocked them.
        FixResult restoreMicCameraAccess() {
            const QString store = QStringLiteral(
                "Software\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\");
            int changed = 0;
            const QString cameraOwner = appliedOwner({ "priv.app-sensor-consent-deny" });
            QStringList caps = { QStringLiteral("microphone") };
            if (cameraOwner.isEmpty())
                caps << QStringLiteral("webcam");
            for (const QString& cap : caps) {
                for (const auto& [root, sub] : {
                         std::pair<HKEY, QString>{ HKEY_LOCAL_MACHINE, store + cap },
                         std::pair<HKEY, QString>{ HKEY_CURRENT_USER,  store + cap },
                         std::pair<HKEY, QString>{ HKEY_CURRENT_USER,  store + cap + QStringLiteral("\\NonPackaged") } }) {
                    HKEY key = nullptr;
                    if (RegCreateKeyExW(root, reinterpret_cast<LPCWSTR>(sub.utf16()), 0, nullptr, 0,
                                        KEY_QUERY_VALUE | KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS)
                        continue;
                    if (ensureSz(key, L"Value", QStringLiteral("Allow")))
                        ++changed;
                    RegCloseKey(key);
                }
            }
            HKEY pol = nullptr;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\AppPrivacy",
                              0, KEY_SET_VALUE, &pol) == ERROR_SUCCESS) {
                if (RegDeleteValueW(pol, L"LetAppsAccessMicrophone") == ERROR_SUCCESS)
                    ++changed;
                if (cameraOwner.isEmpty() && RegDeleteValueW(pol, L"LetAppsAccessCamera") == ERROR_SUCCESS)
                    ++changed;
                RegCloseKey(pol);
            }
            if (cameraOwner.isEmpty()
                && RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Camera",
                                 0, KEY_SET_VALUE, &pol) == ERROR_SUCCESS) {
                if (RegDeleteValueW(pol, L"AllowCamera") == ERROR_SUCCESS)
                    ++changed;
                RegCloseKey(pol);
            }

            FixResult r;
            r.ok = true;
            r.message = changed > 0
                ? (cameraOwner.isEmpty()
                       ? QStringLiteral("Apps can use the microphone and camera again. Restart any app "
                                        "that was already open.")
                       : QStringLiteral("Apps can use the microphone again. The camera stays blocked by "
                                        "Vethida's \"%1\" tweak; turn that off on the Tweaks page to "
                                        "allow it too.").arg(cameraOwner))
                : QStringLiteral("Microphone and camera access were already allowed. If an app still "
                                 "hears nothing, check the input device in Windows Sound settings.");
            return r;
        }

        // Game Pass games that will not install or launch, and Xbox sign-in that
        // loops: a debloat disabled the Xbox services.
        FixResult restoreGameServices() {
            if (const QString owner = appliedOwner({ "op.svc.xbox" }); !owner.isEmpty())
                return deferToTweak(owner);
            QStringList changed;
            for (const QString& s : { QStringLiteral("XblAuthManager"), QStringLiteral("XblGameSave"),
                                      QStringLiteral("XboxNetApiSvc"), QStringLiteral("XboxGipSvc") })
                reenableIfDisabled(s, svc::StartType::Manual, changed);
            for (const QString& s : { QStringLiteral("GamingServices"), QStringLiteral("GamingServicesNet") })
                reenableIfDisabled(s, svc::StartType::Auto, changed);
            FixResult r;
            r.ok = true;
            r.needsReboot = !changed.isEmpty();
            r.message = changed.isEmpty()
                ? QStringLiteral("The Xbox and gaming services were already set up normally.")
                : QStringLiteral("The Xbox and gaming services are turned back on. Restart the PC, "
                                 "then open the game again.");
            r.detail = turnedBackOn(changed);
            return r;
        }

        // Keys that drop presses and a mouse that skips, after a tool shrank the
        // input buffers.
        constexpr DWORD kMinInputQueue = 16;

        int shrunkenInputBuffers(bool fix) {
            struct Q { const wchar_t* key; const wchar_t* value; };
            static const Q kQueues[] = {
                { L"SYSTEM\\CurrentControlSet\\Services\\kbdclass\\Parameters", L"KeyboardDataQueueSize" },
                { L"SYSTEM\\CurrentControlSet\\Services\\mouclass\\Parameters", L"MouseDataQueueSize" },
            };
            int count = 0;
            for (const Q& q : kQueues) {
                HKEY key = nullptr;
                if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, q.key, 0,
                                  KEY_QUERY_VALUE | (fix ? KEY_SET_VALUE : 0), &key) != ERROR_SUCCESS)
                    continue;
                DWORD v = 0;
                if (regReadDword(key, q.value, v) && v < kMinInputQueue) {
                    ++count;
                    if (fix)
                        RegDeleteValueW(key, q.value);
                }
                RegCloseKey(key);
            }
            return count;
        }

        FixResult resetInputBuffers() {
            const int reset = shrunkenInputBuffers(true);
            FixResult r;
            r.ok = true;
            r.needsReboot = reset > 0;
            r.message = reset > 0
                ? QStringLiteral("The keyboard and mouse buffers are back to Windows' defaults. Restart "
                                 "the PC to finish.")
                : QStringLiteral("The keyboard and mouse buffers were already a safe size.");
            return r;
        }

        // From the adapter list, because the WLAN service may be switched off.
        bool hasWirelessAdapter() {
            constexpr ULONG kAdapterListOnly = GAA_FLAG_SKIP_UNICAST | GAA_FLAG_SKIP_ANYCAST
                                             | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
            ULONG size = 0;
            if (GetAdaptersAddresses(AF_UNSPEC, kAdapterListOnly, nullptr, nullptr, &size)
                != ERROR_BUFFER_OVERFLOW || size == 0)
                return false;
            QByteArray buf(static_cast<int>(size), '\0');
            auto* list = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buf.data());
            if (GetAdaptersAddresses(AF_UNSPEC, kAdapterListOnly, nullptr, list, &size) != NO_ERROR)
                return false;
            for (auto* a = list; a; a = a->Next)
                if (a->IfType == IF_TYPE_IEEE80211)
                    return true;
            return false;
        }

        struct ServiceDefault {
            const char*    name;
            svc::StartType start;
        };

        // The startup types Windows ships with.
        const ServiceDefault kNetworkServices[] = {
            { "Dhcp",     svc::StartType::Auto },
            { "Dnscache", svc::StartType::Auto },
            { "nsi",      svc::StartType::Auto },
            { "NlaSvc",   svc::StartType::Auto },
            { "netprofm", svc::StartType::Manual },
            { "WlanSvc",  svc::StartType::Auto },
        };

        FixResult restoreNetworkServices() {
            QStringList changed;
            for (const ServiceDefault& d : kNetworkServices)
                reenableIfDisabled(QString::fromLatin1(d.name), d.start, changed);
            QStringList stillDown;
            for (const char* n : { "Dhcp", "Dnscache", "NlaSvc", "WlanSvc" }) {
                const QString name = QString::fromLatin1(n);
                if (svc::getStartType(name) == svc::StartType::Unknown)
                    continue;
                if (!serviceStart(name))
                    stillDown << name;
            }
            FixResult r;
            r.ok = true;
            r.needsReboot = !stillDown.isEmpty();
            if (changed.isEmpty() && stillDown.isEmpty())
                r.message = QStringLiteral("The network services were already set up normally. If "
                                           "Wi-Fi is still missing, try \"Repair network stack\".");
            else if (stillDown.isEmpty())
                r.message = QStringLiteral("The network services are back on. Wi-Fi networks appear "
                                           "again within a minute.");
            else
                r.message = QStringLiteral("The network services are set to start with Windows again. "
                                           "Restart the PC to bring them up.");
            r.detail = turnedBackOn(changed);
            return r;
        }

        // Wi-Fi profiles, Task Scheduler and many installers stop working without it.
        FixResult restoreEventLog() {
            QStringList changed;
            reenableIfDisabled(QStringLiteral("EventLog"), svc::StartType::Auto, changed);
            const bool running = serviceStart(QStringLiteral("EventLog"));
            FixResult r;
            r.ok = running || !changed.isEmpty();
            r.needsReboot = !running;
            r.message = running
                ? (changed.isEmpty() ? QStringLiteral("The Windows Event Log is running normally.")
                                     : QStringLiteral("The Windows Event Log is back on and running."))
                : QStringLiteral("The Windows Event Log is set to start with Windows again. Restart "
                                 "the PC to bring it up.");
            r.detail = turnedBackOn(changed);
            return r;
        }

        const wchar_t* const kMemoryManagement =
            L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management";

        QStringList pagingFiles() {
            QStringList out;
            HKEY key = nullptr;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, kMemoryManagement, 0, KEY_READ, &key) != ERROR_SUCCESS)
                return out;
            DWORD type = 0, bytes = 0;
            if (RegQueryValueExW(key, L"PagingFiles", nullptr, &type, nullptr, &bytes) == ERROR_SUCCESS
                && type == REG_MULTI_SZ && bytes > 0) {
                QVector<wchar_t> buf(static_cast<int>(bytes / sizeof(wchar_t)) + 2, L'\0');
                if (RegQueryValueExW(key, L"PagingFiles", nullptr, nullptr,
                                     reinterpret_cast<LPBYTE>(buf.data()), &bytes) == ERROR_SUCCESS) {
                    for (const wchar_t* p = buf.constData(); *p; p += wcslen(p) + 1) {
                        const QString entry = QString::fromWCharArray(p).trimmed();
                        if (!entry.isEmpty())
                            out << entry;
                    }
                }
            }
            RegCloseKey(key);
            return out;
        }

        // Games that crash with "out of memory" on a PC with plenty of RAM.
        FixResult restorePageFile() {
            if (const QString owner = appliedOwner({ "op.performance.disable-pagefile" }); !owner.isEmpty())
                return deferToTweak(owner);
            FixResult r;
            if (!pagingFiles().isEmpty()) {
                r.ok = true;
                r.message = QStringLiteral("This PC already has a page file. Nothing was changed.");
                return r;
            }
            HKEY key = nullptr;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, kMemoryManagement, 0, KEY_SET_VALUE, &key) != ERROR_SUCCESS) {
                r.message = QStringLiteral("Windows would not open its memory settings. Security "
                                           "software may be protecting them.");
                return r;
            }
            // System-managed size on the system drive.
            static const wchar_t kAuto[] = L"?:\\pagefile.sys\0";
            const LSTATUS set = RegSetValueExW(key, L"PagingFiles", 0, REG_MULTI_SZ,
                                               reinterpret_cast<const BYTE*>(kAuto), sizeof(kAuto));
            RegCloseKey(key);
            r.ok = set == ERROR_SUCCESS;
            r.needsReboot = r.ok;
            r.message = r.ok
                ? QStringLiteral("Windows manages the page file again. Restart the PC to finish.")
                : QStringLiteral("Windows refused to change the page file setting (error %1).").arg(set);
            return r;
        }

        const wchar_t* const kTeredoPolicy =
            L"SOFTWARE\\Policies\\Microsoft\\Windows\\TCPIP\\v6Transition";
        const wchar_t* const kTcpip6 =
            L"SYSTEM\\CurrentControlSet\\Services\\Tcpip6\\Parameters";

        bool teredoPolicyDisabled() {
            HKEY key = nullptr;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, kTeredoPolicy, 0, KEY_READ, &key) != ERROR_SUCCESS)
                return false;
            const bool off = regReadSz(key, L"Teredo_State").compare(QLatin1String("Disabled"),
                                                                   Qt::CaseInsensitive) == 0;
            RegCloseKey(key);
            return off;
        }

        // Bit 0 of DisabledComponents switches off every IPv6 tunnel, including Teredo.
        bool tunnelsDisabled(DWORD* valueOut = nullptr) {
            HKEY key = nullptr;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, kTcpip6, 0, KEY_READ, &key) != ERROR_SUCCESS)
                return false;
            DWORD v = 0;
            const bool has = regReadDword(key, L"DisabledComponents", v);
            RegCloseKey(key);
            if (valueOut)
                *valueOut = v;
            return has && (v & 0x01);
        }

        // Xbox party chat and Game Pass multiplayer use Teredo.
        FixResult fixXboxNetworking() {
            if (const QString owner = appliedOwner({ "op.drv.legacy-network" }); !owner.isEmpty())
                return deferToTweak(owner);
            QStringList changed;
            QStringList notes;
            reenableIfDisabled(QStringLiteral("iphlpsvc"), svc::StartType::Auto, changed);
            reenableIfDisabled(QStringLiteral("XboxNetApiSvc"), svc::StartType::Manual, changed);

            bool reboot = !changed.isEmpty();
            if (teredoPolicyDisabled()) {
                HKEY key = nullptr;
                if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, kTeredoPolicy, 0, KEY_SET_VALUE, &key) == ERROR_SUCCESS) {
                    RegDeleteValueW(key, L"Teredo_State");
                    RegCloseKey(key);
                    notes << QStringLiteral("Removed the policy that turned Teredo off.");
                }
            }
            DWORD dc = 0;
            if (tunnelsDisabled(&dc)) {
                HKEY key = nullptr;
                if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, kTcpip6, 0, KEY_SET_VALUE, &key) == ERROR_SUCCESS) {
                    const DWORD cleared = dc & ~static_cast<DWORD>(0x01);
                    RegSetValueExW(key, L"DisabledComponents", 0, REG_DWORD,
                                   reinterpret_cast<const BYTE*>(&cleared), sizeof(cleared));
                    RegCloseKey(key);
                    notes << QStringLiteral("Turned IPv6 tunnels back on (DisabledComponents 0x%1 to 0x%2).")
                                 .arg(dc, 0, 16).arg(cleared, 0, 16);
                    reboot = true;
                }
            }
            const ProcOut teredo = runProcess(system32(L"netsh.exe"),
                { QStringLiteral("interface"), QStringLiteral("teredo"), QStringLiteral("set"),
                  QStringLiteral("state"), QStringLiteral("type=default") }, kShort);
            if (!teredo.ran || teredo.code != 0)
                notes << teredo.out;
            serviceStart(QStringLiteral("iphlpsvc"));

            FixResult r;
            r.ok = teredo.ran && teredo.code == 0;
            r.needsReboot = reboot;
            r.message = !r.ok
                ? QStringLiteral("Windows would not reset Teredo. Run \"Repair network stack\", "
                                 "restart, then try this again.")
                : reboot
                    ? QStringLiteral("Xbox networking is set back to Windows' defaults. Restart the "
                                     "PC, then check again in the Xbox app under Settings > Network.")
                    : QStringLiteral("Teredo is set back to Windows' defaults. Check again in the "
                                     "Xbox app under Settings > Network.");
            QStringList detail;
            if (!changed.isEmpty())
                detail << turnedBackOn(changed);
            detail += notes;
            r.detail = detail.join(QLatin1Char(' ')).trimmed();
            return r;
        }

        bool gameBarRegistered() {
            HKEY key = nullptr;
            if (RegOpenKeyExW(HKEY_CURRENT_USER,
                    L"Software\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\CurrentVersion\\"
                    L"AppModel\\Repository\\Packages", 0, KEY_READ, &key) != ERROR_SUCCESS)
                return true;
            bool found = false;
            wchar_t name[256];
            for (DWORD i = 0;; ++i) {
                DWORD len = 256;
                if (RegEnumKeyExW(key, i, name, &len, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS)
                    break;
                if (QString::fromWCharArray(name, static_cast<int>(len))
                        .startsWith(QLatin1String("Microsoft.XboxGamingOverlay_"), Qt::CaseInsensitive)) {
                    found = true;
                    break;
                }
            }
            RegCloseKey(key);
            return found;
        }

        // Absent counts as on, which is Windows' default.
        bool gameCaptureOn() {
            DWORD v = 1;
            bool on = false;
            HKEY key = nullptr;
            if (RegOpenKeyExW(HKEY_CURRENT_USER, L"System\\GameConfigStore", 0, KEY_READ, &key) == ERROR_SUCCESS) {
                if (!regReadDword(key, L"GameDVR_Enabled", v) || v != 0)
                    on = true;
                RegCloseKey(key);
            } else {
                on = true;
            }
            return on;
        }

        // "You'll need a new app to open this ms-gamingoverlay link", after Game Bar
        // was removed.
        FixResult fixGamingOverlayPopup() {
            FixResult r;
            HKEY dvr = nullptr;
            HKEY store = nullptr;
            const bool a = RegCreateKeyExW(HKEY_CURRENT_USER,
                L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\GameDVR", 0, nullptr, 0,
                KEY_SET_VALUE | KEY_QUERY_VALUE, nullptr, &dvr, nullptr) == ERROR_SUCCESS;
            const bool b = RegCreateKeyExW(HKEY_CURRENT_USER, L"System\\GameConfigStore", 0, nullptr, 0,
                KEY_SET_VALUE | KEY_QUERY_VALUE, nullptr, &store, nullptr) == ERROR_SUCCESS;
            bool changed = false;
            if (a) { changed |= ensureDword(dvr, L"AppCaptureEnabled", 0); RegCloseKey(dvr); }
            if (b) { changed |= ensureDword(store, L"GameDVR_Enabled", 0); RegCloseKey(store); }
            r.ok = a && b;
            r.message = !r.ok
                ? QStringLiteral("Windows would not save the Game Bar setting for this account.")
                : changed
                    ? QStringLiteral("Windows no longer opens Game Bar when a game starts, so the "
                                     "pop-up is gone. Sign out and back in if it shows once more.")
                    : QStringLiteral("Game capture was already off for this account. If the pop-up "
                                     "still appears, run \"Repair the Microsoft Store\".");
            return r;
        }

        // Typing does nothing in Start menu search, the emoji panel or the touch
        // keyboard.
        FixResult fixTypingInSearch() {
            QStringList changed;
            reenableIfDisabled(QStringLiteral("TextInputManagementService"), svc::StartType::Auto, changed);
            reenableIfDisabled(QStringLiteral("TabletInputService"), svc::StartType::Manual, changed);
            if (svc::getStartType(QStringLiteral("TextInputManagementService")) != svc::StartType::Unknown)
                serviceStart(QStringLiteral("TextInputManagementService"));
            const QString task = QStringLiteral("\\Microsoft\\Windows\\TextServicesFramework\\MsCtfMonitor");
            const ProcOut enable = runProcess(system32(L"schtasks.exe"),
                { QStringLiteral("/change"), QStringLiteral("/tn"), task, QStringLiteral("/enable") }, kShort);
            runProcess(system32(L"schtasks.exe"),
                { QStringLiteral("/run"), QStringLiteral("/tn"), task }, kShort);

            FixResult r;
            r.ok = true;
            r.needsReboot = !changed.isEmpty();
            r.message = r.needsReboot
                ? QStringLiteral("Text input is turned back on. Restart the PC to finish.")
                : QStringLiteral("Text input is running. Click into the search box and type again.");
            QStringList detail;
            if (!changed.isEmpty())
                detail << turnedBackOn(changed);
            if (!enable.ran || enable.code != 0)
                detail << enable.out;
            r.detail = detail.join(QLatin1Char(' ')).trimmed();
            return r;
        }

        // Settings only shows the first of these, so the others can stay off even
        // with "Snap windows" on.
        struct SnapSetting {
            const wchar_t* value;      // under HKCU\Control Panel\Desktop
            UINT           get;
            UINT           set;
            const char*    evidence;   // what is wrong while this one is off
        };
        const SnapSetting kSnapSettings[] = {
            { L"WindowArrangementActive", SPI_GETWINARRANGING, SPI_SETWINARRANGING,
              "Snap windows is turned off." },
            { L"DockMoving", SPI_GETDOCKMOVING, SPI_SETDOCKMOVING,
              "Windows does not snap a window dragged to a screen edge." },
            { L"SnapSizing", SPI_GETSNAPSIZING, SPI_SETSNAPSIZING,
              "Windows does not stretch a window sized to the top or bottom of the screen." },
            { L"DragFromMaximize", SPI_GETDRAGFROMMAXIMIZE, SPI_SETDRAGFROMMAXIMIZE,
              "Dragging a maximized window does not restore it." },
        };

        // The current session's value. The registry is only read at sign-in.
        bool snapSettingOn(UINT get) {
            BOOL on = FALSE;
            return SystemParametersInfoW(get, 0, &on, 0) && on;
        }

        FixResult fixWindowSnapping() {
            HKEY key = nullptr;
            const bool opened = RegOpenKeyExW(HKEY_CURRENT_USER, L"Control Panel\\Desktop", 0,
                                              KEY_QUERY_VALUE | KEY_SET_VALUE, &key) == ERROR_SUCCESS;
            int changed = 0;
            bool live = true;
            for (const SnapSetting& s : kSnapSettings) {
                // As a string, because a DWORD here reads as off at every sign-in.
                const bool stored = opened && ensureSz(key, s.value, QStringLiteral("1"));
                const bool wasOn = snapSettingOn(s.get);
                // TRUE in both parameters, because the docs disagree about which is read.
                SystemParametersInfoW(s.set, TRUE, reinterpret_cast<PVOID>(static_cast<UINT_PTR>(TRUE)),
                                      SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
                if (stored || !wasOn)
                    ++changed;
                live = snapSettingOn(s.get) && live;
            }
            if (opened)
                RegCloseKey(key);

            FixResult r;
            r.ok = live || opened;
            if (!r.ok)
                r.message = QStringLiteral("Windows would not save the snapping settings for this account.");
            else if (!live)
                r.message = QStringLiteral("Window snapping is saved for this account. Sign out and back "
                                           "in to finish.");
            else if (changed > 0)
                r.message = QStringLiteral("Window snapping is back on. Drag a window to a screen edge "
                                           "to snap it.");
            else
                r.message = QStringLiteral("Window snapping was already fully on for this account.");
            return r;
        }

        Symptom present(const QString& evidence) {
            Symptom s;
            s.present  = true;
            s.evidence = evidence;
            return s;
        }

        // Not "anything but running", because a service may be mid-start.
        bool serviceIsStopped(const QString& name) {
            return vth::win::services::getState(name)
                   == vth::win::services::State::Stopped;
        }

        bool serviceIsDisabled(const QString& name) {
            return vth::win::services::getStartType(name)
                   == vth::win::services::StartType::Disabled;
        }

        // An NVIDIA adapter, with neither the driver's menu handler nor the repair's verb.
        Symptom detectNvidiaControlPanelMissing() {
            bool nvidia = false;
            HKEY cls = nullptr;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                    L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}",
                    0, KEY_READ, &cls) == ERROR_SUCCESS) {
                for (DWORD i = 0; !nvidia; ++i) {
                    wchar_t sub[64] = {};
                    DWORD len = DWORD(std::size(sub));
                    if (RegEnumKeyExW(cls, i, sub, &len, nullptr, nullptr, nullptr, nullptr)
                            != ERROR_SUCCESS)
                        break;
                    HKEY a = nullptr;
                    if (RegOpenKeyExW(cls, sub, 0, KEY_READ, &a) != ERROR_SUCCESS)
                        continue;
                    nvidia = regReadSz(a, L"MatchingDeviceId")
                                 .contains(QStringLiteral("VEN_10DE"), Qt::CaseInsensitive);
                    RegCloseKey(a);
                }
                RegCloseKey(cls);
            }
            if (!nvidia)
                return {};

            HKEY k = nullptr;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                    L"SOFTWARE\\Classes\\Directory\\Background\\shell\\NvidiaControlPanel\\command",
                    0, KEY_READ, &k) == ERROR_SUCCESS) {
                RegCloseKey(k);
                return {};
            }
            if (RegOpenKeyExW(HKEY_CLASSES_ROOT,
                    L"Directory\\Background\\shellex\\ContextMenuHandlers\\NvCplDesktopContext",
                    0, KEY_READ, &k) == ERROR_SUCCESS) {
                const QString clsid = regReadSz(k, nullptr);
                RegCloseKey(k);
                if (!clsid.isEmpty()) {
                    const std::wstring path = QString(QStringLiteral("CLSID\\") + clsid
                                               + QStringLiteral("\\InprocServer32")).toStdWString();
                    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, path.c_str(), 0, KEY_READ, &k)
                            == ERROR_SUCCESS) {
                        const QString dll = expandEnv(regReadSz(k, nullptr));
                        RegCloseKey(k);
                        if (!dll.isEmpty() && QFileInfo::exists(dll))
                            return {};
                    }
                }
            }
            return present(QStringLiteral(
                "The NVIDIA Control Panel is missing from the desktop's right-click menu"));
        }

        Symptom detectAudioStopped() {
            if (serviceIsStopped(QStringLiteral("Audiosrv"))
                || serviceIsStopped(QStringLiteral("AudioEndpointBuilder"))) {
                return present(QStringLiteral(
                    "The Windows audio service is not running."));
            }
            return {};
        }

        Symptom detectBluetoothDisabled() {
            if (!appliedOwner({ "op.svc.disable-bluetooth", "op.svc.disable-radio" }).isEmpty())
                return {};
            if (serviceIsDisabled(QStringLiteral("BTHPORT")))
                return present(QStringLiteral(
                    "The Bluetooth driver is disabled."));
            if (serviceIsDisabled(QStringLiteral("bthserv")))
                return present(QStringLiteral(
                    "The Bluetooth support service is disabled."));
            if (serviceIsDisabled(QStringLiteral("RmSvc")))
                return present(QStringLiteral(
                    "The radio management service is disabled."));
            return {};
        }

        // A job that won't delete leaves a .spl file behind.
        Symptom detectStuckPrintJobs() {
            QDir dir(systemRoot() + QStringLiteral("/System32/spool/PRINTERS"));
            if (!dir.exists())
                return {};
            const int jobs = dir.entryList(QStringList{ QStringLiteral("*.spl") },
                                           QDir::Files | QDir::Hidden).size();
            if (jobs <= 0)
                return {};
            return present(jobs == 1
                ? QStringLiteral("1 job is waiting in the print queue.")
                : QStringLiteral("%1 jobs are waiting in the print queue.").arg(jobs));
        }

        Symptom detectProxySet() {
            const auto enabled = vth::win::reg::readDword(
                vth::win::reg::Hive::CurrentUser, kInternetSettingsKey,
                QStringLiteral("ProxyEnable"));
            if (!enabled.has_value() || *enabled == 0)
                return {};

            const auto server = vth::win::reg::readAsText(
                vth::win::reg::Hive::CurrentUser, kInternetSettingsKey,
                QStringLiteral("ProxyServer"));
            return present(server.has_value() && !server->isEmpty()
                ? QStringLiteral("Traffic is going through the proxy %1.").arg(*server)
                : QStringLiteral("A proxy server is configured for this account."));
        }

        Symptom detectWindowsUpdateDisabled() {
            if (serviceIsDisabled(QStringLiteral("wuauserv")))
                return present(QStringLiteral(
                    "The Windows Update service is disabled."));
            return {};
        }

        Symptom detectSearchIndexerStopped() {
            if (serviceIsStopped(QStringLiteral("WSearch")))
                return present(QStringLiteral("The search indexer is not running."));
            return {};
        }

        // It only starts when triggered on a home PC, so being stopped is normal.
        Symptom detectTimeServiceDisabled() {
            if (!appliedOwner({ "op.svc.time-sync-off" }).isEmpty())
                return {};
            if (serviceIsDisabled(QStringLiteral("W32Time")))
                return present(QStringLiteral(
                    "The Windows time service is disabled."));
            return {};
        }

        Symptom detectPowerServiceDown() {
            if (serviceIsDisabled(QStringLiteral("Power")))
                return present(QStringLiteral("The Windows Power service is disabled."));
            if (serviceIsStopped(QStringLiteral("Power")))
                return present(QStringLiteral("The Windows Power service is not running."));
            return {};
        }

        Symptom detectInstallerDisabled() {
            if (serviceIsDisabled(QStringLiteral("msiserver")))
                return present(QStringLiteral("The Windows Installer service is disabled."));
            return {};
        }

        Symptom detectStoreServicesDisabled() {
            if (!appliedOwner({ "op.svc.disable-store-appx", "op.svc.disable-msaccount" }).isEmpty())
                return {};
            for (const QString& s : { QStringLiteral("AppXSvc"), QStringLiteral("ClipSVC"),
                                      QStringLiteral("InstallService"), QStringLiteral("LicenseManager"),
                                      QStringLiteral("DoSvc") })
                if (serviceIsDisabled(s))
                    return present(QStringLiteral("A service the Microsoft Store installs apps with is "
                                                  "disabled (%1).").arg(s));
            return {};
        }

        Symptom detectMicCameraBlocked() {
            const QString store = QStringLiteral(
                "Software\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\");
            const bool cameraOwned = !appliedOwner({ "priv.app-sensor-consent-deny" }).isEmpty();
            for (const QString& cap : { QStringLiteral("microphone"), QStringLiteral("webcam") }) {
                if (cap == QLatin1String("webcam") && cameraOwned)
                    continue;
                for (HKEY root : { HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER }) {
                    HKEY key = nullptr;
                    const QString path = store + cap;
                    if (RegOpenKeyExW(root, reinterpret_cast<LPCWSTR>(path.utf16()), 0,
                                      KEY_QUERY_VALUE, &key) != ERROR_SUCCESS)
                        continue;
                    const QString v = regReadSz(key, L"Value");
                    RegCloseKey(key);
                    if (v.compare(QStringLiteral("Deny"), Qt::CaseInsensitive) == 0)
                        return present(cap == QLatin1String("microphone")
                            ? QStringLiteral("Apps are blocked from using the microphone.")
                            : QStringLiteral("Apps are blocked from using the camera."));
                }
            }
            HKEY pol = nullptr;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\AppPrivacy",
                              0, KEY_QUERY_VALUE, &pol) == ERROR_SUCCESS) {
                DWORD v = 0;
                const bool micDenied = regReadDword(pol, L"LetAppsAccessMicrophone", v) && v == 2;
                const bool camDenied = regReadDword(pol, L"LetAppsAccessCamera", v) && v == 2;
                RegCloseKey(pol);
                if (micDenied || camDenied)
                    return present(QStringLiteral("A policy blocks apps from the microphone or camera."));
            }
            return {};
        }

        Symptom detectGameServicesDisabled() {
            if (!appliedOwner({ "op.svc.xbox" }).isEmpty())
                return {};
            for (const QString& s : { QStringLiteral("XblAuthManager"), QStringLiteral("XblGameSave"),
                                      QStringLiteral("XboxNetApiSvc"), QStringLiteral("GamingServices") })
                if (serviceIsDisabled(s))
                    return present(QStringLiteral("An Xbox or gaming service is disabled (%1).").arg(s));
            return {};
        }

        Symptom detectInputBuffersShrunk() {
            if (shrunkenInputBuffers(false) > 0)
                return present(QStringLiteral("The keyboard or mouse input buffer is set very small."));
            return {};
        }

        Symptom detectNoInternet() {
            const DWORD c = currentConnectivity();
            if (c == 0 || saysInternet(c))
                return {};
            return present(QStringLiteral(
                "Windows reports that this PC has no internet connection."));
        }

        Symptom detectNetworkServicesDisabled() {
            for (const ServiceDefault& d : kNetworkServices) {
                const QString name = QString::fromLatin1(d.name);
                if (name == QLatin1String("WlanSvc") && !hasWirelessAdapter())
                    continue;
                if (serviceIsDisabled(name))
                    return present(QStringLiteral("A network service is disabled (%1).").arg(name));
            }
            return {};
        }

        Symptom detectEventLogDisabled() {
            if (serviceIsDisabled(QStringLiteral("EventLog")))
                return present(QStringLiteral("The Windows Event Log service is disabled."));
            return {};
        }

        Symptom detectNoPageFile() {
            if (!appliedOwner({ "op.performance.disable-pagefile" }).isEmpty())
                return {};
            if (pagingFiles().isEmpty())
                return present(QStringLiteral("This PC has no page file."));
            return {};
        }

        Symptom detectXboxNetworkingOff() {
            // Only a fault where the Xbox gaming stack is installed.
            if (svc::getStartType(QStringLiteral("GamingServices")) == svc::StartType::Unknown)
                return {};
            if (!appliedOwner({ "op.drv.legacy-network" }).isEmpty())
                return {};
            if (serviceIsDisabled(QStringLiteral("iphlpsvc")))
                return present(QStringLiteral("The IP Helper service Xbox networking runs on is disabled."));
            if (teredoPolicyDisabled())
                return present(QStringLiteral("A policy turns off Teredo, which Xbox multiplayer uses."));
            if (tunnelsDisabled())
                return present(QStringLiteral("IPv6 tunnels are turned off, so Teredo cannot run."));
            return {};
        }

        Symptom detectGamingOverlayPopup() {
            if (!gameBarRegistered() && gameCaptureOn())
                return present(QStringLiteral("Game Bar is removed, but game capture still tries to open it."));
            return {};
        }

        Symptom detectTextInputDisabled() {
            for (const QString& s : { QStringLiteral("TextInputManagementService"),
                                      QStringLiteral("TabletInputService") })
                if (serviceIsDisabled(s))
                    return present(QStringLiteral("The text input service is disabled (%1).").arg(s));
            return {};
        }

        Symptom detectSnappingOff() {
            for (const SnapSetting& s : kSnapSettings)
                if (!snapSettingOn(s.get))
                    return present(QString::fromLatin1(s.evidence));
            return {};
        }
    }

    namespace {
        FixResult resetTimerSettings();
        FixResult fixAntiCheatBlockers();
        Symptom detectAntiCheatBlockers();
        Symptom detectTimerChanged();
    }

    FixResult run(FixId id) {
        if (!vth::win::isElevated()) {
            FixResult r;
            r.message = QStringLiteral(
                "This repair needs administrator rights, and Vethida is not running with them. "
                "Close Vethida, open it again, and allow it when Windows asks.");
            return r;
        }

        switch (id) {
        case FixId::RestartAudioServices:     return restartAudioServices();
        case FixId::RestoreBluetoothServices: return restoreBluetoothServices();
        case FixId::ClearPrintQueue:          return clearPrintQueue();

        case FixId::RenewNetworkAddress:      return renewNetworkAddress();
        case FixId::RepairConnectivityDetection: return repairConnectivityDetection();
        case FixId::ClearProxySettings:       return clearProxySettings();
        case FixId::RepairNetworkStack:       return repairNetworkStack();
        case FixId::ResetFirewallRules:       return resetFirewallRules();

        case FixId::ResetWindowsUpdate:       return resetWindowsUpdate();
        case FixId::ResetStoreCache:          return resetStoreCache();
        case FixId::RepairStartMenuAndSearch: return repairStartMenuAndSearch();
        case FixId::SyncSystemClock:          return syncSystemClock();
        case FixId::ClearOrganizationPolicies: return managed::restoreWindowsDefaults();

        case FixId::RebuildIconCache:         return rebuildIconCache();
        case FixId::ResetFolderViews:         return resetFolderViews();
        case FixId::RebuildSearchIndex:       return rebuildSearchIndex();
        case FixId::ClearFontCache:           return clearFontCache();
        case FixId::RepairRecycleBin:         return repairRecycleBin();

        case FixId::ClearShaderCaches:        return clearShaderCaches();
        case FixId::ClearStandbyMemory:       return clearStandbyMemory();
        case FixId::RestoreNvidiaControlPanel: return restoreNvidiaControlPanel();

        case FixId::ClearTemporaryFiles:      return clearTemporaryFiles();
        case FixId::CompactComponentStore:    return compactComponentStore();
        case FixId::RestoreDefaultPowerPlans: return restoreDefaultPowerPlans();

        case FixId::RunSystemFileCheck:       return runSystemFileCheck();
        case FixId::ScheduleDiskCheck:        return scheduleDiskCheck();
        case FixId::SecureWipeFreeSpace:      return secureWipeFreeSpace();

        case FixId::RestorePowerService:      return restorePowerService();
        case FixId::RestoreWindowsInstaller:  return restoreWindowsInstaller();
        case FixId::RepairMicrosoftStore:     return repairMicrosoftStore();
        case FixId::RepairSettingsApp:        return repairSettingsApp();
        case FixId::RestoreMicCameraAccess:   return restoreMicCameraAccess();
        case FixId::RestoreGameServices:      return restoreGameServices();
        case FixId::ResetTimerSettings:       return resetTimerSettings();
        case FixId::ResetInputBuffers:        return resetInputBuffers();
        case FixId::FixAntiCheatBlockers:     return fixAntiCheatBlockers();
        case FixId::RestoreNetworkServices:   return restoreNetworkServices();
        case FixId::RestoreEventLog:          return restoreEventLog();
        case FixId::RestorePageFile:          return restorePageFile();
        case FixId::FixXboxNetworking:        return fixXboxNetworking();
        case FixId::FixGamingOverlayPopup:    return fixGamingOverlayPopup();
        case FixId::FixTypingInSearch:        return fixTypingInSearch();
        case FixId::FixWindowSnapping:        return fixWindowSnapping();
        }
        return {};
    }

    Symptom detect(FixId id) {
        switch (id) {
        case FixId::RestartAudioServices:        return detectAudioStopped();
        case FixId::RestoreBluetoothServices:    return detectBluetoothDisabled();
        case FixId::ClearPrintQueue:             return detectStuckPrintJobs();

        case FixId::RepairConnectivityDetection: return detectNoInternet();
        case FixId::ClearProxySettings:          return detectProxySet();

        case FixId::ResetWindowsUpdate:          return detectWindowsUpdateDisabled();
        case FixId::SyncSystemClock:             return detectTimeServiceDisabled();

        case FixId::RebuildSearchIndex:          return detectSearchIndexerStopped();

        case FixId::RestoreNvidiaControlPanel:   return detectNvidiaControlPanelMissing();

        case FixId::RestorePowerService:         return detectPowerServiceDown();
        case FixId::RestoreWindowsInstaller:     return detectInstallerDisabled();
        case FixId::RepairMicrosoftStore:        return detectStoreServicesDisabled();
        case FixId::RestoreMicCameraAccess:      return detectMicCameraBlocked();
        case FixId::RestoreGameServices:         return detectGameServicesDisabled();
        case FixId::ResetInputBuffers:           return detectInputBuffersShrunk();
        case FixId::FixAntiCheatBlockers:        return detectAntiCheatBlockers();
        case FixId::ResetTimerSettings:          return detectTimerChanged();
        case FixId::RestoreNetworkServices:      return detectNetworkServicesDisabled();
        case FixId::RestoreEventLog:             return detectEventLogDisabled();
        case FixId::RestorePageFile:             return detectNoPageFile();
        case FixId::FixXboxNetworking:           return detectXboxNetworkingOff();
        case FixId::FixGamingOverlayPopup:       return detectGamingOverlayPopup();
        case FixId::FixTypingInSearch:           return detectTextInputDisabled();
        case FixId::FixWindowSnapping:           return detectSnappingOff();

        default:
            return {};
        }
    }
}

namespace vth::win::fixes
{
namespace
{
        QString vethidaTimerTweakOn() {
            vth::services::RegistryEditor reg;
            for (const QString& id : { QStringLiteral("op.performance.disable-hpet"),
                                       QStringLiteral("bcd.constant-boot-tick") }) {
                if (const auto* spec = vth::tweaks::find(id))
                    if (vth::tweaks::detect(*spec, reg) == vth::tweaks::DetectState::On)
                        return spec->name;
            }
            return {};
        }
        FixResult resetTimerSettings() {
            FixResult r;
            if (const QString owner = vethidaTimerTweakOn(); !owner.isEmpty()) {
                r.message = QStringLiteral("These timer settings come from Vethida's \"%1\" tweak. Turn "
                                           "it off on the Tweaks page instead; that puts back exactly "
                                           "what was there before.").arg(owner);
                return r;
            }
            if (!vth::win::bcd::ensureBackupBeforeBootWrite()) {
                r.message = QStringLiteral("Vethida could not back up the boot configuration, so it "
                                           "did not change it.");
                return r;
            }
            int removed = 0;
            for (const QString& element : { QStringLiteral("useplatformclock"), QStringLiteral("useplatformtick"),
                                            QStringLiteral("disabledynamictick"), QStringLiteral("tscsyncpolicy") }) {
                const ProcOut p = runProcess(system32(L"bcdedit.exe"),
                    { QStringLiteral("/deletevalue"), QStringLiteral("{current}"), element }, kShort);
                if (p.ran && p.code == 0)
                    ++removed;
            }
            r.ok = true;
            r.needsReboot = removed > 0;
            r.message = removed > 0
                ? QStringLiteral("The system timer is back to Windows' defaults. Restart the PC to "
                                 "finish.")
                : QStringLiteral("The system timer was already on Windows' defaults.");
            return r;
        }
        struct BootFlags {
            bool testSigning = false;
            bool noIntegrity = false;
            bool debug = false;
            bool timerChanged = false;
        };
        bool readBootFlagsFromStore(bool& testSigning, bool& noIntegrity, bool& debug) {
            const std::wstring objects = L"BCD00000000\\Objects\\";
            const std::wstring bootmgr = objects + L"{9dea862c-5cdd-4e70-acc1-f32b344d4795}"
                                                   L"\\Elements\\23000003";
            wchar_t guid[64] = {};
            DWORD size = sizeof(guid);
            if (RegGetValueW(HKEY_LOCAL_MACHINE, bootmgr.c_str(), L"Element", RRF_RT_REG_SZ,
                             nullptr, guid, &size) != ERROR_SUCCESS)
                return false;
            const auto flag = [&](const wchar_t* element) {
                const std::wstring path = objects + guid + L"\\Elements\\" + element;
                BYTE value[8] = {};
                DWORD n = sizeof(value);
                if (RegGetValueW(HKEY_LOCAL_MACHINE, path.c_str(), L"Element", RRF_RT_REG_BINARY,
                                 nullptr, value, &n) != ERROR_SUCCESS || n == 0)
                    return false;
                return value[0] != 0;
            };
            testSigning = flag(L"16000049");
            noIntegrity = flag(L"16000048");
            debug       = flag(L"260000a0");
            return true;
        }
        BootFlags readBootFlags() {
            BootFlags f;
            const ProcOut p = runProcess(system32(L"bcdedit.exe"),
                                         { QStringLiteral("/enum"), QStringLiteral("{current}") }, kShort);
            if (!p.ran)
                return f;
            for (const QString& line : p.out.split(QLatin1Char('\n'))) {
                const QString l = line.simplified().toLower();
                const bool yes = l.endsWith(QLatin1String(" yes"));
                if (l.startsWith(QLatin1String("testsigning")) && yes)       f.testSigning = true;
                if (l.startsWith(QLatin1String("nointegritychecks")) && yes) f.noIntegrity = true;
                if (l.startsWith(QLatin1String("debug ")) && yes)            f.debug = true;
                if (l.startsWith(QLatin1String("useplatformclock"))
                    || l.startsWith(QLatin1String("useplatformtick"))
                    || l.startsWith(QLatin1String("disabledynamictick"))
                    || l.startsWith(QLatin1String("tscsyncpolicy")))
                    f.timerChanged = true;
            }
            bool ts = false, ni = false, dbg = false;
            if (readBootFlagsFromStore(ts, ni, dbg)) {
                f.testSigning = ts;
                f.noIntegrity = ni;
                f.debug       = dbg;
            }
            return f;
        }
        FixResult fixAntiCheatBlockers() {
            FixResult r;
            if (!vth::win::bcd::ensureBackupBeforeBootWrite()) {
                r.message = QStringLiteral("Vethida could not back up the boot configuration, so it "
                                           "did not change it.");
                return r;
            }
            const BootFlags before = readBootFlags();
            const QString bcd = system32(L"bcdedit.exe");
            runProcess(bcd, { QStringLiteral("/set"), QStringLiteral("{current}"),
                              QStringLiteral("testsigning"), QStringLiteral("off") }, kShort);
            runProcess(bcd, { QStringLiteral("/set"), QStringLiteral("{current}"),
                              QStringLiteral("nointegritychecks"), QStringLiteral("off") }, kShort);
            runProcess(bcd, { QStringLiteral("/debug"), QStringLiteral("{current}"),
                              QStringLiteral("off") }, kShort);
            QStringList changed;
            reenableIfDisabled(QStringLiteral("Winmgmt"), svc::StartType::Auto, changed);
            reenableIfDisabled(QStringLiteral("CryptSvc"), svc::StartType::Auto, changed);
            QStringList fixed;
            if (before.testSigning) fixed << QStringLiteral("test signing");
            if (before.noIntegrity) fixed << QStringLiteral("disabled driver checks");
            if (before.debug)       fixed << QStringLiteral("kernel debugging");
            r.ok = true;
            r.needsReboot = !fixed.isEmpty() || !changed.isEmpty();
            r.message = fixed.isEmpty() && changed.isEmpty()
                ? QStringLiteral("Nothing on this PC was blocking anti-cheat. If a game still refuses "
                                 "to start, reinstall its anti-cheat from the game's own folder or "
                                 "launcher.")
                : QStringLiteral("Turned off %1. Restart the PC, then start the game again.")
                      .arg(fixed.isEmpty() ? QStringLiteral("the settings that block anti-cheat")
                                           : fixed.join(QStringLiteral(", ")));
            r.detail = turnedBackOn(changed);
            return r;
        }
        Symptom detectTimeServiceStopped() {
            if (!appliedOwner({ "op.svc.time-sync-off" }).isEmpty())
                return {};
            if (serviceIsStopped(QStringLiteral("W32Time")))
                return present(QStringLiteral(
                    "The Windows time service is not running."));
            return {};
        }
        Symptom detectAntiCheatBlockers() {
            const BootFlags f = readBootFlags();
            if (f.testSigning)
                return present(QStringLiteral("Test signing is turned on, which anti-cheat refuses to run with."));
            if (f.noIntegrity)
                return present(QStringLiteral("Driver signature checks are turned off."));
            if (f.debug)
                return present(QStringLiteral("Kernel debugging is turned on."));
            return {};
        }
        Symptom detectTimerChanged() {
            if (readBootFlags().timerChanged && vethidaTimerTweakOn().isEmpty())
                return present(QStringLiteral("The system timer has been changed from Windows' defaults."));
            return {};
        }
}
}

