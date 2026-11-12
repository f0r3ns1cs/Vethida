#pragma once

#include <QLoggingCategory>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QVector>
#include <QtCore/qlogging.h>

#include <deque>

namespace vth::log
{

    void initialize();
    void shutdown();

    void flush();

    QString currentLogFile();


    struct Record {
        qint64    timestampMs;
        QtMsgType type;
        QString   category;
        QString   message;
    };

    class Sink : public QObject {
        Q_OBJECT

    public:
        static Sink& instance();

        QVector<Record> history() const;

        void post(qint64 timestampMs, QtMsgType type,
            const QString& category, const QString& message);

    private:
        Sink();
        ~Sink() override;

        mutable QMutex      mu_;
        std::deque<Record>  buffer_;

        static constexpr int kSinkBuffer = 5000;
    };

}

#define VTH_DECLARE_LOG_CATEGORY(name)  Q_DECLARE_LOGGING_CATEGORY(name)
#define VTH_DEFINE_LOG_CATEGORY(name, str) \
    Q_LOGGING_CATEGORY(name, str, QtInfoMsg)
