#pragma once

#include <QByteArray>
#include <QString>
#include <QVector>
#include <optional>

namespace vth::win::power
{
    struct Plan {
        QString guid;   // no braces
        QString name;
        bool    active = false;
    };

    struct WellKnownPlans {
        static QString balanced();
        static QString highPerformance();
        static QString powerSaver();
        static QString ultimatePerformance();
    };

    QVector<Plan> enumeratePlans();

    bool setActivePlan(const QString& guid);

    struct ProcessorPolicy {
        bool valid            = false;
        int  minStatePercent  = 0;
        int  maxStatePercent  = 0;
        int  minCoresPercent  = 0;
    };

    // AC values only.
    ProcessorPolicy readProcessorPolicy(const QString& guid);

    // Empty if the power service doesn't answer.
    struct ActiveScheme {
        QString guid;   // no braces, lower case
        QString name;
    };
    ActiveScheme readActiveScheme();

    // Empty if the scheme isn't installed.
    QString schemeName(const QString& guid);

    struct PlanOutcome {
        bool    ok = false;
        QString guid;
        QString message;
        QString detail;
    };

    // Installs a plan file under a fixed GUID and makes it active.
    PlanOutcome installAndActivate(const QByteArray& planFile, const QString& schemeGuid,
                                   const QString& wantedName, const QString& displayName);

    // Makes an installed scheme active and checks that it stayed active.
    PlanOutcome activate(const QString& guid, const QString& displayName);
}

namespace vth::win::power
{
    struct Timeouts {
        int monitorAc = 0;
        int monitorDc = 0;
        int sleepAc   = 0;
        int sleepDc   = 0;
    };
    enum class Result {
        Ok,
        AccessDenied,
        NotFound,
        OtherError,
    };
    QString resultLabel(Result r);
    QString activePlanGuid();
    Timeouts readTimeouts();
    Result writeTimeouts(const Timeouts& t);
    std::optional<QString> duplicatePlan(const QString& sourceGuid,
                                         const QString& newName = QString{});
    std::optional<QString> importPlan(const QString& filePath);
    Result exportPlan(const QString& guid, const QString& filePath);
    bool hasBattery();
}
