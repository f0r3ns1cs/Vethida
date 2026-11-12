#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>

namespace vth::core
{
    enum class UpdateChannel { Stable, Beta, Off };

    struct UpdateAvailable {
        QString version;
        QString channel;
        QString downloadUrl;
        QString sha256;
        qint64 sizeBytes = 0;
        QString releaseNotesUrl;
        bool securityFix = false;
        QString manifestJws;
    };

    enum class UpdateStage {
        Idle,
        Checking,
        Downloading,
        Verifying,
        Ready,
        Launching,
        Failed,
    };

    struct UpdateState {
        UpdateStage stage = UpdateStage::Idle;
        UpdateAvailable advertised;
        QString localPath;
        qint64 bytesReceived = 0;
        qint64 bytesTotal = 0;
        QString lastError;
    };

    class Updater : public QObject
    {
        Q_OBJECT

    public:
        explicit Updater(QObject* parent = nullptr);
        ~Updater() override;

        void evaluateFromSettings();
        void onAvailableUpdate(const QString& version,
                               const QString& channel,
                               const QString& downloadUrl,
                               const QString& sha256,
                               qint64 sizeBytes,
                               const QString& manifestJws);
        void beginDownload();
        void installAndRestart();

        UpdateState state() const { return state_; }

    Q_SIGNALS:
        void availableChanged(const UpdateAvailable& update);
        void stageChanged(UpdateStage stage);
        void downloadProgress(qint64 received, qint64 total);
        void failed(const QString& message);

    private:
        UpdateState state_;
        UpdateChannel channel_ = UpdateChannel::Stable;

        // result from the worker, copied back to the UI thread
        struct VerifyResult {
            QString actualSha256;
            QString signerCommonName;
            bool    hashMatches   = false;
            bool    authenticodeOk = false;
        };

        void setStage(UpdateStage stage);
        void verifyDownload(); // hashing + signature check on a worker
        void onVerifyFinished(const VerifyResult& result);
        void pruneStaleDownloads(const QString& keepPath);
        bool authenticodeOk(const QString& path) const; // chain trust only, not who signed it
        QString authenticodeSignerCommonName(const QString& path) const;
        static QString sha256OfFile(const QString& path);
    };
}
