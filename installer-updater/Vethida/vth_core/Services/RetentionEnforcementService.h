#pragma once

#include <QObject>
#include <QTimer>

namespace vth::services
{
    // Deletes old logs, crash dumps and cache files. Runs at startup and once a day.
    class RetentionEnforcementService : public QObject
    {
        Q_OBJECT

    public:
        static constexpr int LogRetentionDays = 90;
        static constexpr int CrashRetentionDays = 90;
        static constexpr int CacheRetentionDays = 30;

        explicit RetentionEnforcementService(QObject* parent = nullptr);

        void runOnce();

    private:
        QTimer m_timer;
    };
}
