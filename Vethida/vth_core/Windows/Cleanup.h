#pragma once

#include "Registry.h"

#include <QString>
#include <QStringList>
#include <QVector>
#include <QtTypes>

#include <functional>

namespace vth::win::cleanup
{
    enum class Category {
        TempAndCache,
        AppCaches,
        ShaderCaches,
        WindowsUpdate,
        Logs,
        Dumps,
        Misc,
        Deep,
    };

    enum class Mechanism {
        DirectoryWalk,      // delete regular files under paths
        FileDelete,         // delete the named files
        RecycleBin,
        ComponentStore,     // DISM /StartComponentCleanup /ResetBase
        Hibernation,        // powercfg /hibernate off
        WindowsDiskCleanup, // cleanmgr /sagerun
    };

    struct Path {
        QString path;
        bool    recursive   = true;
        int     minAgeDays  = 0;
    };

    struct Cleaner {
        QString         id;
        QString         displayName;
        Category        category;
        reg::Severity   severity;
        QString         summary;
        QString         impact;
        Mechanism       mechanism;
        QVector<Path>   paths;

        bool            recommended = true;   // on by default
        bool            measurable  = true;   // scan() can size it
        bool            perVolume   = false;  // offered for drives other than the system drive
    };

    QString categoryLabel(Category c);

    QVector<Category> orderedCategories();

    // All cleaners on the system drive, only the per-volume ones on any other.
    QVector<const Cleaner*> cleanersFor(const QString& driveRoot);

    const Cleaner* find(const QString& id);

    struct ScanResult {
        quint64 bytes = 0;
        bool    known = true;   // false if the cleaner can't be sized
        int     fileCount = 0;
    };

    ScanResult scan(const Cleaner& cleaner, const QString& driveRoot);

    struct RunResult {
        quint64 reclaimedBytes = 0;
        int     filesDeleted   = 0;
        int     errors         = 0;
        bool        valid          = false;
        QStringList ranIds;
        QStringList failedIds;
    };

    // (stageLabel, stepIndex, stepCount)
    using Progress = std::function<void(const QString&, int, int)>;

    // Runs the named cleaners in catalog order. Ids that aren't in the catalog or
    // don't apply to this drive are skipped.
    RunResult run(const QStringList& cleanerIds, const QString& driveRoot,
                  const Progress& progress = {});

    QString formatBytes(quint64 bytes);
}
