#include "Logger.h"
#include "Runtime/Paths.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QTextStream>
#include <QThread>

#include <cstdio>

namespace vth::log
{
    namespace
    {
        constexpr qint64 kRotateBytes = 5 * 1024 * 1024;   // 5 MB
        constexpr int    kKeepRotated = 3;                 // .log + .1 + .2 + .3

        QMutex          gLogMutex;
        QFile           gLogFile;
        QTextStream     gLogStream;
        QtMessageHandler gPrevHandler = nullptr;
        bool            gInitialized = false;

        QString gLastCategory;
        QtMsgType gLastLevel = QtDebugMsg;
        QString gLastMessage;
        int     gLastRepeatCount = 0;

        const char* levelTag(QtMsgType t) {
            switch (t) {
            case QtDebugMsg:    return "DBG";
            case QtInfoMsg:     return "INF";
            case QtWarningMsg:  return "WRN";
            case QtCriticalMsg: return "ERR";
            case QtFatalMsg:    return "FAT";
            }
            return "???";
        }

        void rotateIfNeededLocked() {
            if (!gLogFile.isOpen()) return;
            if (gLogFile.size() < kRotateBytes) return;

            gLogStream.flush();
            gLogFile.close();

            const QString base = gLogFile.fileName();
            for (int i = kKeepRotated; i >= 1; --i) {
                QFile::remove(QStringLiteral("%1.%2").arg(base).arg(i));
                QFile::rename(
                    i == 1 ? base : QStringLiteral("%1.%2").arg(base).arg(i - 1),
                    QStringLiteral("%1.%2").arg(base).arg(i));
            }

            gLogFile.setFileName(base);
            if (gLogFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
                gLogStream.setDevice(&gLogFile);
            }
        }

        QString formatLine(qint64 nowMs, QtMsgType type, const QString& cat, const QString& msg) {
            return QStringLiteral("%1 [%2] %3 %4 %5\n")
                .arg(QDateTime::fromMSecsSinceEpoch(nowMs).toUTC()
                    .toString(Qt::ISODateWithMs),
                    QString::fromLatin1(levelTag(type)),
                    QString::asprintf("t=%p", static_cast<void*>(QThread::currentThread())),
                    cat,
                    msg);
        }

        void writeLineLocked(const QString& line) {
            std::fputs(line.toLocal8Bit().constData(), stderr);
            if (gLogFile.isOpen()) {
                gLogStream << line;
                gLogStream.flush();
                gLogFile.flush();
                rotateIfNeededLocked();
            }
        }

        // Caller holds gLogMutex.
        void flushRepetitionLocked() {
            if (gLastRepeatCount <= 0) return;
            const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
            const QString suffix = gLastRepeatCount == 1
                ? QStringLiteral("(previous message repeated 1 more time)")
                : QStringLiteral("(previous message repeated %1 more times)").arg(gLastRepeatCount);
            const QString line = formatLine(nowMs, gLastLevel, gLastCategory, suffix);
            writeLineLocked(line);
            Sink::instance().post(nowMs, gLastLevel, gLastCategory, suffix);
            gLastRepeatCount = 0;
        }

        void messageHandler(QtMsgType type, const QMessageLogContext& ctx, const QString& msg) {
            const qint64  nowMs = QDateTime::currentMSecsSinceEpoch();
            const QString cat = ctx.category && *ctx.category
                ? QString::fromLatin1(ctx.category)
                : QStringLiteral("default");

            // Fatal messages bypass dedup.
            if (type != QtFatalMsg) {
                QMutexLocker lock(&gLogMutex);
                if (cat == gLastCategory && type == gLastLevel && msg == gLastMessage) {
                    ++gLastRepeatCount;
                    return;
                }
                flushRepetitionLocked();
                gLastCategory = cat;
                gLastLevel = type;
                gLastMessage = msg;
            }

            const QString line = formatLine(nowMs, type, cat, msg);

            {
                QMutexLocker lock(&gLogMutex);
                writeLineLocked(line);
            }

            Sink::instance().post(nowMs, type, cat, msg);

            if (type == QtFatalMsg) {
                std::fflush(stderr);
                std::abort();
            }
        }

    }

    void initialize() {
        QMutexLocker lock(&gLogMutex);
        if (gInitialized) return;

        Sink::instance();

        const QString logPath = vth::paths::userLogs() + QStringLiteral("/vethida.log");
        gLogFile.setFileName(logPath);
        if (gLogFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            gLogStream.setDevice(&gLogFile);
            gLogStream << QStringLiteral("--- vethida session start %1 ---\n")
                .arg(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
            gLogStream.flush();
        }

        gPrevHandler = qInstallMessageHandler(&messageHandler);
        gInitialized = true;
    }

    void shutdown() {
        QMutexLocker lock(&gLogMutex);
        if (!gInitialized) return;

        flushRepetitionLocked();

        qInstallMessageHandler(gPrevHandler);
        gPrevHandler = nullptr;

        if (gLogFile.isOpen()) {
            gLogStream << QStringLiteral("--- vethida session end %1 ---\n")
                .arg(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
            gLogStream.flush();
            gLogStream.setDevice(nullptr);
            gLogFile.close();
        }

        gInitialized = false;
    }

    void flush() {
        QMutexLocker lock(&gLogMutex);
        flushRepetitionLocked();
        if (gLogFile.isOpen()) {
            gLogStream.flush();
            gLogFile.flush();
        }
        std::fflush(stderr);
    }

    QString currentLogFile() {
        QMutexLocker lock(&gLogMutex);
        return gLogFile.fileName();
    }


    Sink& Sink::instance() {
        static Sink s;
        return s;
    }

    Sink::Sink() = default;
    Sink::~Sink() = default;

    QVector<Record> Sink::history() const {
        QMutexLocker lock(&mu_);
        QVector<Record> out;
        out.reserve(static_cast<int>(buffer_.size()));
        for (const auto& r : buffer_) {
            out.append(r);
        }
        return out;
    }

    void Sink::post(qint64 timestampMs, QtMsgType type,
        const QString& category, const QString& message) {
            {
                QMutexLocker lock(&mu_);
                buffer_.push_back(Record{ timestampMs, type, category, message });
                while (buffer_.size() > static_cast<size_t>(kSinkBuffer)) {
                    buffer_.pop_front();
                }
            }
    }

}
