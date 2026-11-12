#pragma once

#include <QString>

// A safety net under the boot-configuration tweaks, and nothing else.

namespace vth::win::bcd
{
    struct BackupResult {
        bool    ok = false;
        QString error;
        QString filePath;
        qint64  bytesWritten = 0;
    };

    // Absolute path of the store backup under the Vethida user-data directory.
    QString backupPath();
    bool    hasBackup();

    // exports the current store, replacing the old backup
    BackupResult backupStore();

    // once per session before the first boot change. false = don't write.
    bool ensureBackupBeforeBootWrite();

    // bcdedit /import. The user-facing route home from a boot setting that did
    // not agree with the machine.
    BackupResult restoreStore();
}
