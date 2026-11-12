#pragma once

#include <QDateTime>
#include <QString>
#include <QVector>

namespace vth::win::sysrestore
{
    struct RestorePoint {
        int       sequenceNumber = 0;
        QString   description;
        QDateTime createdUtc;
        bool      isVethida = false;   // description contains "Vethida"
    };

    struct EnumerateResult {
        bool                  ok = false;
        QString               error;
        QVector<RestorePoint> points;
    };
    EnumerateResult enumerateChecked();

    enum class CreateResult {
        Ok,
        AccessDenied,
        Throttled,
        ProtectionDisabled,
        ServicesDisabled,
        DiskFull,
        OtherError,
    };
    QString createResultLabel(CreateResult r);

    struct CreateOutcome {
        CreateResult result = CreateResult::OtherError;
        QString      error;
        int          sequenceNumber = 0;
    };

    // bypassThrottle lifts Windows' limit of one restore point per 24 hours for
    // this call. Needs elevation.
    CreateOutcome create(const QString& description, bool bypassThrottle);

    // Returns an empty string on success, otherwise the reason. Needs elevation.
    QString enableSystemProtection();

    enum class RestoreResult {
        Restarting,
        AccessDenied,
        InProgress,
        NotFound,
        OtherError,
    };
    QString restoreResultLabel(RestoreResult r);

    struct RestoreOutcome {
        RestoreResult result = RestoreResult::OtherError;
        QString       error;
    };

    // Windows finishes the restore during a restart. Needs elevation.
    RestoreOutcome restoreToPoint(int sequenceNumber);

    enum class DeleteResult {
        Ok,
        AccessDenied,
        NotFound,
        OtherError,
    };
    QString deleteResultLabel(DeleteResult r);

    struct DeleteOutcome {
        DeleteResult result = DeleteResult::OtherError;
        QString      error;
    };

    // Deletes the shadow copy closest in time to the restore point. Needs elevation.
    DeleteOutcome deleteRestorePoint(int sequenceNumber);

    // Deletes every restore point on the system drive. Needs elevation.
    DeleteOutcome deleteAllRestorePoints();
}

namespace vth::win::sysrestore
{
    struct Readiness {
        bool isElevated = false;
        bool vssServiceInstalled = false;
        bool vssServiceRunning = false;
        bool systemProtectionEnabled = false;
        bool wmiReachable = false;
    };
    Readiness               checkReadiness();
    QVector<RestorePoint>   enumerate();
}
