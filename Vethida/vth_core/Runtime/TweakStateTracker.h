#pragma once

#include <QDateTime>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

#include <QMutex>

#include <functional>

namespace vth
{

    enum class SnapshotKind {
        Registry,
        Service,
        Task,
    };

    QString snapshotKindToString(SnapshotKind k);
    SnapshotKind snapshotKindFromString(const QString& s);

    // One struct for all kinds; kind selects the active field group.
    struct TweakSnapshot {
        QString      domain;
        SnapshotKind kind = SnapshotKind::Registry;
        QString      label;

        // Which apply session wrote this, and when.
        QString   batchId;
        QDateTime appliedAt;   // UTC

        // Registry
        QString hive;               // "HKLM" / "HKCU"
        QString subkey;
        QString name;
        bool    existedBefore = false;
        bool    isDword       = true;
        quint32 prevDword     = 0;
        QString prevString;

        // Service
        QString serviceName;
        quint32 prevStartType     = 0;
        bool    serviceWasRunning = false;

        // Task
        QString taskPath;
        bool    taskWasEnabled = true;

        QJsonObject toJson() const;
        static TweakSnapshot fromJson(const QJsonObject& obj);
    };

    struct TweakLedgerEntry {
        int           index = -1;   // position in the ledger
        TweakSnapshot snapshot;
    };

    // One apply session's entries.
    struct TweakBatchGroup {
        QString   batchId;
        QDateTime appliedAt;   // newest in the group; invalid for older entries
        QVector<TweakLedgerEntry> entries;
    };

    // Rollback ledger for every tweak domain, persisted to tweak-state.json.
    class TweakStateTracker : public QObject {
        Q_OBJECT

    public:
        bool revertOne(int index);
        int  revertAll();
        int  revertBatch(const QString& batchId);
        int  revertDomain(const QString& domain);
        void clearAll();
        void removeAt(int index);
        TweakStateTracker();

        // Saved before returning. Change the system only after this returns, so
        // a crash leaves a recoverable snapshot rather than an untracked change.
        void record(TweakSnapshot snap);

        QVector<TweakSnapshot> entries(const QString& domain) const;
        QVector<TweakSnapshot> entries() const;

        // Newest session first, with each session's entries newest first.
        QVector<TweakBatchGroup> batches() const;

        QStringList domains() const;

        void clearDomain(const QString& domain);

        // Newest first. Failures stay in the ledger for a retry.
        struct RevertReport {
            int         reverted = 0;
            int         failed   = 0;
            QStringList failedDomains;
        };
        RevertReport undoDomain(const QString& domain);
        RevertReport undoBatch(const QString& batchId);
        RevertReport undoAll();

    Q_SIGNALS:
        // May be emitted from a worker thread; connect queued.
        void reverted();

    private:
        void load();
        void saveLocked();
        RevertReport revertMatching(const std::function<bool(const TweakSnapshot&)>& want);
        RevertReport revertMatchingLocked(const std::function<bool(const TweakSnapshot&)>& want);

        QString filePath() const;

        mutable QMutex         mu_;   // guards entries_, shared with worker threads
        QVector<TweakSnapshot> entries_;
    };

    QString newTweakBatchId(const QString& label = QString());

    // Groups every snapshot recorded on this thread, for as long as it is alive,
    // under one batch id.
    class TweakBatch {
    public:
        explicit TweakBatch(QString batchId);
        ~TweakBatch();

        TweakBatch(const TweakBatch&)            = delete;
        TweakBatch& operator=(const TweakBatch&) = delete;

        QString id() const { return id_; }

    private:
        QString id_;
        QString outer_;   // restored on destruction, so batches can nest
    };

    // The batch id in force on this thread, empty outside a TweakBatch.
    QString currentTweakBatch();

    // Go through the registered TweakStateTracker; no-ops when there is none.
    void                   recordTweakSnapshot(TweakSnapshot snap);
    QVector<TweakSnapshot> tweakSnapshots(const QString& domain);

    // Drops a domain's snapshots without restoring them.
    void                   clearTweakDomain(const QString& domain);

}

namespace vth
{
    QString snapshotKindLabel(SnapshotKind k);
    int                    revertTweakDomain(const QString& domain);
}

