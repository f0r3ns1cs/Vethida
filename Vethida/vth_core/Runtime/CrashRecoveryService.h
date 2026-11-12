#pragma once

#include <QtCore/QDateTime>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>

// Marks a batch of tweaks as in progress. If the marker is still there on the
// next launch, the batch didn't finish and can be reverted.
namespace vth
{
    class CrashRecoveryService : public QObject {
        Q_OBJECT

    public:
        QString     priorBatchId()      const noexcept { return priorBatchId_; }
        CrashRecoveryService();
        ~CrashRecoveryService() override;

        // Reads any existing marker. Call before the UI shows.
        void initialize();

        bool detectUnclean() const noexcept { return uncleanDetected_; }

        QStringList priorBatchOps()     const noexcept { return priorBatchOps_; }
        QDateTime   priorBatchStarted() const noexcept { return priorBatchStarted_; }

        void beginBatch(const QString& batchId, const QStringList& opIds);
        void endBatch();

        // The user has reverted or dismissed the unfinished batch.
        void acknowledge();

        struct RevertOutcome {
            int reverted = 0;
            int failed   = 0;
            QString message;
        };

        RevertOutcome revertPriorBatch() const;

    private:
        QString markerPath() const;
        void    writeMarker(const QString& batchId,
                            const QStringList& opIds,
                            const QDateTime& startedAt);
        void    clearMarker();

        bool        uncleanDetected_ = false;
        QString     priorBatchId_;
        QStringList priorBatchOps_;
        QDateTime   priorBatchStarted_;
    };
}
