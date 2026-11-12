#pragma once

#include <QObject>
#include <QStringList>

#include <atomic>
#include <memory>

class QTimer;

namespace vth::win::genesis
{
    // Closes a suppressed program that starts again during a session.
    class GenesisMonitor : public QObject {
        Q_OBJECT

    public:
        explicit GenesisMonitor(QObject* parent = nullptr);
        ~GenesisMonitor() override;

        void start(const QStringList& targets, const QStringList& exceptions);
        void stop();

    private:
        void sweep();

        QTimer*     timer_ = nullptr;
        QStringList targets_;
        bool        inFlight_ = false;

        // Read by the worker right before it closes anything, so a sweep can't close
        // a program the user just got back by turning Genesis off.
        std::shared_ptr<std::atomic<bool>> live_;
    };
}
