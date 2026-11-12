#include "BcdBackup.h"

#include "Privileges.h"
#include "../Logger.h"
#include "../Runtime/Paths.h"

#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStringLiteral>

#include <atomic>

#include <windows.h>

namespace vth::win::bcd
{
    namespace
    {
        VTH_DEFINE_LOG_CATEGORY(catBcd, "vth.bcd")

        // Absolute path to bcdedit. Boot writes run elevated, so a
        // PATH-resolved bcdedit is a hijack of the boot loader.
        QString bcdEditPath() {
            wchar_t dir[MAX_PATH] = {};
            const UINT n = GetSystemDirectoryW(dir, MAX_PATH);
            if (n == 0 || n >= MAX_PATH)
                return QStringLiteral("bcdedit.exe");
            return QString::fromWCharArray(dir) + QStringLiteral("\\bcdedit.exe");
        }

        struct RunResult {
            int     exitCode = -1;
            bool    timedOut = false;
            QString errorText;
        };

        RunResult runBcdEdit(const QStringList& args) {
            RunResult r;
            QProcess p;
            p.setProgram(bcdEditPath());
            p.setArguments(args);
            p.start();
            if (!p.waitForStarted(5000)) {
                r.errorText = QStringLiteral("bcdedit did not start.");
                return r;
            }
            if (!p.waitForFinished(30'000)) {
                p.kill();
                r.timedOut = true;
                r.errorText = QStringLiteral("bcdedit timed out.");
                return r;
            }
            r.exitCode = p.exitCode();
            // bcdedit writes its failures to stdout, not stderr.
            r.errorText = QString::fromLocal8Bit(p.readAllStandardError()).trimmed();
            if (r.errorText.isEmpty())
                r.errorText = QString::fromLocal8Bit(p.readAllStandardOutput()).trimmed();
            return r;
        }

        // One export per session. Reset only by restarting Vethida, which is
        // also when the store may have changed underneath us.
        std::atomic<bool>& backedUpThisSession() {
            static std::atomic<bool> done{false};
            return done;
        }
    }

    QString backupPath() {
        return vth::paths::protectedData() + QStringLiteral("/bcd-store.bak");
    }

    bool hasBackup() {
        const QFileInfo fi(backupPath());
        return fi.exists() && fi.size() > 0;
    }

    BackupResult backupStore() {
        BackupResult r;
        r.filePath = backupPath();

        if (!vth::win::isElevated()) {
            r.error = QStringLiteral(
                "Backing up the boot configuration needs administrator privileges.");
            return r;
        }
        if (!vth::paths::ensureDir(vth::paths::protectedData())) {
            r.error = QStringLiteral("Could not create Vethida's data directory.");
            return r;
        }

        // export to .tmp first so the old backup is only replaced once this works
        const QString tmp = r.filePath + QStringLiteral(".tmp");
        QFile::remove(tmp);

        const RunResult rr = runBcdEdit({ QStringLiteral("/export"), tmp });
        if (rr.timedOut || rr.exitCode != 0) {
            QFile::remove(tmp);
            r.error = rr.errorText.isEmpty()
                ? QStringLiteral("bcdedit /export failed (exit %1).").arg(rr.exitCode)
                : rr.errorText;
            qCWarning(catBcd) << "boot store backup failed:" << r.error;
            return r;
        }

        if (QFileInfo::exists(r.filePath) && !QFile::remove(r.filePath)) {
            QFile::remove(tmp);
            r.error = QStringLiteral("Could not replace the previous boot backup.");
            return r;
        }
        if (!QFile::rename(tmp, r.filePath)) {
            QFile::remove(tmp);
            r.error = QStringLiteral("Could not move the boot backup into place.");
            return r;
        }

        r.bytesWritten = QFileInfo(r.filePath).size();
        r.ok = r.bytesWritten > 0;
        if (!r.ok)
            r.error = QStringLiteral("bcdedit produced an empty store file.");
        else
            qCInfo(catBcd) << "boot store backed up:" << r.bytesWritten << "bytes";
        return r;
    }

    bool ensureBackupBeforeBootWrite() {
        if (backedUpThisSession().load())
            return true;
        const BackupResult r = backupStore();
        if (r.ok)
            backedUpThisSession().store(true);
        return r.ok;
    }

    BackupResult restoreStore() {
        BackupResult r;
        r.filePath = backupPath();

        if (!vth::win::isElevated()) {
            r.error = QStringLiteral(
                "Restoring the boot configuration needs administrator privileges.");
            return r;
        }
        if (!hasBackup()) {
            r.error = QStringLiteral("Vethida has no boot configuration backup to restore.");
            return r;
        }

        const RunResult rr = runBcdEdit({ QStringLiteral("/import"), r.filePath });
        if (rr.timedOut || rr.exitCode != 0) {
            r.error = rr.errorText.isEmpty()
                ? QStringLiteral("bcdedit /import failed (exit %1).").arg(rr.exitCode)
                : rr.errorText;
            return r;
        }
        r.ok = true;
        qCInfo(catBcd) << "boot store restored from" << r.filePath;
        return r;
    }
}
