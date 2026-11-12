#pragma once

#include <QMetaType>
#include <QString>
#include <QVector>

#include <atomic>
#include <functional>
#include <memory>

namespace vth::win::speedtest
{
    enum class Status {
        Ready,
        Running,
        Complete,
        Canceled,
        Error,
    };

    struct Result {
        Status  status = Status::Ready;
        QString statusMessage;

        double  downloadMbps = 0.0;
        double  uploadMbps = 0.0;

        double  idleLatencyMs = -1.0;
        double  loadedDownloadLatencyMs = -1.0;
        double  loadedUploadLatencyMs = -1.0;

        double  bufferbloatIncreaseMs = 0.0;
        QString bufferbloatGrade = QStringLiteral("-");

        double  progress = 0.0;   // 0..1

        QVector<double> downloadHistory;
        QVector<double> uploadHistory;
    };

    QString statusLabel(Status s);

    // kGradeCeilingsMs[i] is the exclusive upper bound of kGradeLetters[i]. The
    // last letter has no ceiling.
    inline constexpr double      kGradeCeilingsMs[] = { 30.0, 60.0, 100.0, 200.0 };
    inline constexpr const char* kGradeLetters[]    = { "A", "B", "C", "D", "F" };

    QString gradeForBufferbloat(double increaseMs);

    // -1 for an unmeasured result.
    int     gradeIndex(const QString& grade);

    class CancelToken {
    public:
        void cancel() noexcept { flag_.store(true); }
        bool isCancelled() const noexcept { return flag_.load(); }
    private:
        std::atomic<bool> flag_{ false };
    };

    using ProgressCallback = std::function<void(const Result&)>;

    // Blocks, so run it on a worker.
    Result run(const ProgressCallback& progress, CancelToken& cancel);
}

Q_DECLARE_METATYPE(vth::win::speedtest::Result)
