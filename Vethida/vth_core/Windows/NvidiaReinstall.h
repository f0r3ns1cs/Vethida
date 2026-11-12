#pragma once

#include <QDateTime>
#include <QJsonObject>
#include <QString>
#include <QStringList>

#include <functional>

// The safety net around the NVIDIA clean reinstall.
namespace vth::win::nvidia::reinstall
{
    // In the user data directory, because cleanup tools empty %TEMP%.
    QString workspace();

    // Run before anything is removed. A blocker means don't start.
    struct Preflight {
        QString     blocker;
        QStringList warnings;
    };
    Preflight preflight(qint64 downloadedBytes);

    // Checks that setup.exe is signed by NVIDIA and a display driver INF is present.
    // Returns the reason if not, otherwise empty.
    QString verifyPackage(const QString& extractDir);

    // "32.0.15.6094" -> "560.94". Empty if it's not a driver version.
    QString nvidiaVersionFromWindows(const QString& windowsVersion);

    // How far a run got, saved so an interrupted one can be finished.
    struct Marker {
        bool        present = false;
        QString     version;          // NVIDIA's name for the driver being installed
        QString     extractDir;
        QString     stage;            // "removing", "installing", "failed"
        QDateTime   startedUtc;
        QJsonObject driverDelivery;   // Windows Update values to put back
        QString     previousDir;      // the exported previous driver, may be empty
        QString     previousVersion;
    };
    Marker readMarker();
    void   writeMarker(const Marker& m);
    void   clearWorkspace();

    // Stops Windows Update installing its own display driver mid-run. Returns the
    // earlier values for restoreDriverDelivery.
    QJsonObject holdDriverDelivery();
    void        restoreDriverDelivery(const QJsonObject& prior);

    // Empty if no NVIDIA driver is bound to the card.
    QString boundDriverVersion();

    // Exports the bound driver into the workspace and returns the folder. Empty if
    // there isn't one or Windows wouldn't export it.
    QString backupBoundDriver(QString* versionOut);

    // True while the old driver is being replaced. Closing the app or signing out
    // would cut the install off, so the shell refuses.
    bool isRunning();
    void setRunning(bool running);

    struct InstallOutcome {
        bool    ok = false;
        bool    restartRequired = false;
        bool    restoredPrevious = false;   // the new driver failed, the old one is back
        QString message;
        QString detail;                     // for the log
    };
    using StageFn = std::function<void(const QString& stage)>;

    // NVIDIA installer (twice), then pnputil. If none works, restores the saved driver.
    InstallOutcome installAndVerify(const QString& extractDir, const QString& expectedVersion,
                                    const QString& previousDir, const QString& previousVersion,
                                    const StageFn& stage);

    // Gives driver delivery back to Windows Update if a run never finished. Returns
    // true if that run can still be finished.
    bool recoverAtStartup();
}

namespace vth::win::nvidia::reinstall
{
    void   clearMarker();
}
