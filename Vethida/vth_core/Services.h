#pragma once

#include <QString>
#include <QVector>

namespace vth::win::services
{
    enum class State {
        Unknown,
        Stopped,
        StartPending,
        StopPending,
        Running,
        ContinuePending,
        PausePending,
        Paused,
    };

    QString stateLabel(State s);

    enum class StartType {
        Unknown,
        Boot,           // SERVICE_BOOT_START, kernel-mode driver only
        System,         // SERVICE_SYSTEM_START, kernel-mode driver only
        Auto,           // SERVICE_AUTO_START
        AutoDelayed,    // SERVICE_AUTO_START + SERVICE_DELAYED_AUTO_START_INFO
        Manual,         // SERVICE_DEMAND_START
        Disabled,       // SERVICE_DISABLED
    };

    QString startTypeLabel(StartType t);

    struct ServiceInfo {
        QString name;          // SCM service name (e.g. "Spooler")
        QString displayName;   // human-readable name (e.g. "Print Spooler")
        State   state = State::Unknown;
    };

    QVector<ServiceInfo> enumerate();

    // Returns Unknown on access denial or any other failure.
    StartType getStartType(const QString& serviceName);

    // Returns Unknown on failure.
    State getState(const QString& serviceName);

    enum class SetStartTypeResult {
        Ok,
        AccessDenied,      // Not elevated, or service ACL denies CHANGE_CONFIG.
        NotFound,          // Service does not exist on this system.
        InvalidStartType,  // Boot / System / Unknown rejected - user-mode only.
        OtherError,
    };

    QString setStartTypeResultLabel(SetStartTypeResult r);

    SetStartTypeResult setStartType(const QString& serviceName, StartType t);
}

namespace vth::win::services
{
    QStringList getDependencies(const QString& serviceName);
    QStringList getDependents(const QString& serviceName);
}
