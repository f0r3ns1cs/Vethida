#include "Speedtest.h"

#include "Http.h"
#include "Windows/Network.h"

#include <QByteArray>
#include <QElapsedTimer>
#include <QRandomGenerator>
#include <QStringLiteral>
#include <QThread>

#include <algorithm>
#include <chrono>
#include <iterator>
#include <thread>
#include <vector>
#include <windows.h>

namespace vth::win::speedtest
{
    namespace
    {
        // By duration, because a fixed size reads low on a fast line and times out
        // on a slow one.
        constexpr double kDownloadSeconds = 8.0;
        constexpr double kUploadSeconds   = 5.0;
        constexpr int    kDownloadMaxChunks = 24;
        constexpr int    kUploadMaxChunks   = 12;

        constexpr qint64 kChunkBytesMin = 25'000'000;
        constexpr qint64 kChunkBytesMax = 200'000'000;
        constexpr int kUploadSizeBytes = 10'000'000;
        constexpr int kLatencySamples = 5;
        constexpr int kLatencyTimeoutMs = 2000;
        constexpr int kHttpTimeoutMs = 45'000;

        const QString kDownloadUrlBase =
            QStringLiteral("https://speed.cloudflare.com/__down?bytes=");
        const QString kUploadUrl =
            QStringLiteral("https://speed.cloudflare.com/__up");

        const QString kLatencyHost = QStringLiteral("1.1.1.1");

        double measureLatencyMs(const CancelToken& cancel) {
            std::vector<double> samples;
            samples.reserve(kLatencySamples);
            for (int i = 0; i < kLatencySamples; ++i) {
                if (cancel.isCancelled()) break;
                const double rtt = vth::win::network::probeLatencyMs(kLatencyHost,
                                                                     kLatencyTimeoutMs);
                if (rtt >= 0) samples.push_back(rtt);
                QThread::msleep(150);
            }
            if (samples.empty()) return -1;
            double sum = 0;
            for (double v : samples) sum += v;
            return sum / samples.size();
        }

        bool runDownload(qint64 chunkBytes, qint64* bytesOut, double* secondsOut,
            const CancelToken& cancel) {
            QElapsedTimer t;
            t.start();
            const vth::http::Download d = vth::http::download(
                kDownloadUrlBase + QString::number(chunkBytes),
                [](const char*, qint64) { return true; },
                [&cancel]() { return cancel.isCancelled(); },
                {},
                std::chrono::milliseconds(kHttpTimeoutMs));

            if (bytesOut)   *bytesOut = d.bytes;
            if (secondsOut) *secondsOut = t.nsecsElapsed() / 1'000'000'000.0;
            return d.ok && d.bytes > 0;
        }

        bool runUpload(const QByteArray& payload, double* secondsOut) {
            QElapsedTimer t;
            t.start();
            const vth::http::Response r = vth::http::post(
                kUploadUrl, payload,
                { { QStringLiteral("Content-Type"),
                    QStringLiteral("application/octet-stream") } },
                std::chrono::milliseconds(kHttpTimeoutMs));

            if (secondsOut) *secondsOut = t.nsecsElapsed() / 1'000'000'000.0;
            return r.ok;
        }

        // Random, so nothing on the path can compress it.
        QByteArray randomPayload() {
            static_assert(kUploadSizeBytes % sizeof(quint32) == 0,
                "payload is filled a word at a time");
            QByteArray p(kUploadSizeBytes, Qt::Uninitialized);
            QRandomGenerator::global()->fillRange(
                reinterpret_cast<quint32*>(p.data()),
                kUploadSizeBytes / int(sizeof(quint32)));
            return p;
        }

        double mbps(qint64 bytes, double seconds) {
            if (seconds <= 0.0) return 0.0;
            return (bytes * 8.0 / seconds) / 1'000'000.0;
        }

        // Drops the first sample once there are three, because it includes
        // connection setup and slow-start.
        double averageMbps(const QVector<double>& samples) {
            if (samples.isEmpty()) return 0.0;
            const int skip = samples.size() >= 3 ? 1 : 0;
            double sum = 0;
            for (int i = skip; i < samples.size(); ++i)
                sum += samples[i];
            return sum / (samples.size() - skip);
        }
    }

    QString statusLabel(Status s) {
        switch (s) {
        case Status::Ready:     return QStringLiteral("Ready");
        case Status::Running:   return QStringLiteral("Running");
        case Status::Complete:  return QStringLiteral("Complete");
        case Status::Canceled:  return QStringLiteral("Canceled");
        case Status::Error:     return QStringLiteral("Error");
        }
        return {};
    }

    QString gradeForBufferbloat(double inc) {
        for (size_t i = 0; i < std::size(kGradeCeilingsMs); ++i)
            if (inc < kGradeCeilingsMs[i])
                return QString::fromLatin1(kGradeLetters[i]);
        return QString::fromLatin1(kGradeLetters[std::size(kGradeCeilingsMs)]);
    }

    int gradeIndex(const QString& grade) {
        for (size_t i = 0; i < std::size(kGradeLetters); ++i)
            if (grade == QLatin1String(kGradeLetters[i]))
                return static_cast<int>(i);
        return -1;
    }

    Result run(const ProgressCallback& progress, CancelToken& cancel) {
        Result r;
        const auto canceled = [&]() {
            r.status = Status::Canceled;
            r.statusMessage = QStringLiteral("Canceled");
            if (progress) progress(r);
            return r;
        };

        r.status = Status::Running;
        r.statusMessage = QStringLiteral("Measuring idle latency...");
        r.progress = 0.05;
        if (progress) progress(r);

        if (cancel.isCancelled())
            return canceled();

        r.idleLatencyMs = measureLatencyMs(cancel);
        r.statusMessage = QStringLiteral("Testing download speed...");
        r.progress = 0.15;
        if (progress) progress(r);

        if (cancel.isCancelled())
            return canceled();

        // Latency under load, measured while the download runs.
        double loadedDownLatency = -1;
        std::thread loadedDownThread([&]() {
            loadedDownLatency = measureLatencyMs(cancel);
        });

        QVector<double> dlSamples;
        double dlElapsed = 0.0;
        qint64 chunkBytes = kChunkBytesMin;
        for (int i = 0; i < kDownloadMaxChunks && dlElapsed < kDownloadSeconds; ++i) {
            if (cancel.isCancelled()) break;

            qint64 bytes = 0;
            double secs = 0;
            const bool ok = runDownload(chunkBytes, &bytes, &secs, cancel);
            if (ok) {
                const double m = mbps(bytes, secs);
                dlSamples.push_back(m);
                r.downloadHistory.append(m);
                dlElapsed += secs;

                // Aim the next request at roughly one second of transfer.
                if (secs > 0.0) {
                    const double perSecond = static_cast<double>(bytes) / secs;
                    chunkBytes = qBound(kChunkBytesMin,
                                        static_cast<qint64>(perSecond),
                                        kChunkBytesMax);
                }
            }

            r.downloadMbps = averageMbps(dlSamples);
            r.progress = 0.15 + 0.4 * qMin(1.0, dlElapsed / kDownloadSeconds);
            if (progress) progress(r);
        }

        if (loadedDownThread.joinable()) loadedDownThread.join();
        r.loadedDownloadLatencyMs = loadedDownLatency;

        if (cancel.isCancelled())
            return canceled();

        r.statusMessage = QStringLiteral("Testing upload speed...");
        r.progress = 0.6;
        if (progress) progress(r);

        const QByteArray payload = randomPayload();

        double loadedUpLatency = -1;
        std::thread loadedUpThread([&]() {
            loadedUpLatency = measureLatencyMs(cancel);
        });

        QVector<double> ulSamples;
        double ulElapsed = 0.0;
        for (int i = 0; i < kUploadMaxChunks && ulElapsed < kUploadSeconds; ++i) {
            if (cancel.isCancelled()) break;
            double secs = 0;
            if (!runUpload(payload, &secs)) break;
            ulElapsed += secs;
            const double m = mbps(static_cast<qint64>(payload.size()), secs);
            ulSamples.push_back(m);
            r.uploadHistory.append(m);

            r.uploadMbps = averageMbps(ulSamples);
            r.progress = 0.6 + 0.35 * qMin(1.0, ulElapsed / kUploadSeconds);
            if (progress) progress(r);
        }

        if (loadedUpThread.joinable()) loadedUpThread.join();
        r.loadedUploadLatencyMs = loadedUpLatency;

        const double loadedMax = std::max(r.loadedDownloadLatencyMs,
            r.loadedUploadLatencyMs);
        if (loadedMax >= 0 && r.idleLatencyMs >= 0) {
            r.bufferbloatIncreaseMs = std::max(0.0, loadedMax - r.idleLatencyMs);
            r.bufferbloatGrade = gradeForBufferbloat(r.bufferbloatIncreaseMs);
        }

        r.progress = 1.0;
        r.status = cancel.isCancelled() ? Status::Canceled : Status::Complete;
        r.statusMessage = cancel.isCancelled()
            ? QStringLiteral("Canceled")
            : QStringLiteral("Complete");
        if (progress) progress(r);
        return r;
    }
}
