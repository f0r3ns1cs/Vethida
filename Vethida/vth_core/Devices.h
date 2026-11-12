#pragma once

#include <QDateTime>
#include <QString>
#include <QVector>

namespace vth::win::devices
{

    struct GhostDevice {
        QString    instanceId;
        QString    deviceName;
        QString    description;
        QString    className;
        QString    classGuid;
        QString    manufacturer;
        QString    service;
        QString    driver;
        QString    hardwareIds;
        QString    enumeratorName;
        QDateTime  lastUsed;        // invalid if unknown

        // Software-pseudo / root-enumerated entries: rarely reclaimable and
        // sometimes still loaded on demand, so bulk removal skips them.
        bool isNonPnp() const noexcept;
    };

    // Devices that are registered but not present (phantom devices).
    QVector<GhostDevice> enumerateGhostDevices();

    struct GhostRemoveResult {
        int removed = 0;
        int failed  = 0;
        int skipped = 0;    // present again at removal time; left untouched
    };

    // Needs admin. Each device is checked again first, anything that came back is
    // skipped and anything unreadable counts as failed.
    GhostRemoveResult removeGhostDevices(const QVector<GhostDevice>& devices);

}

namespace vth::win::devices
{
    struct DriverInfo {
        QString name;
        QString provider;
        QString version;
        QString date;
        QString infPath;
        QString className;
        QString signer;
        bool    isSigned = false;
        int     ageDays = 0;
        bool    flaggedOld = false;
        bool    flaggedUnsigned = false;
    };
    struct DriverDuplicateGroup {
        QString             className;
        QString             provider;
        QVector<DriverInfo> drivers;
    };
    struct DriverAuditSummary {
        int totalDrivers = 0;
        int signedCount = 0;
        int unsignedCount = 0;
        int oldCount = 0;
        int duplicateCount = 0;
    };
    struct DriverAuditResult {
        DriverAuditSummary            summary;
        QVector<DriverInfo>           drivers;
        QVector<DriverDuplicateGroup> duplicates;
    };
    DriverAuditResult auditDrivers();
}
