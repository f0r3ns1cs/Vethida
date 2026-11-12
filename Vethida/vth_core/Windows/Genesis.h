#pragma once

#include <QDateTime>
#include <QString>
#include <QStringList>
#include <QVector>

// A set of independently reversible changes for a gaming session. The rollback data
// is saved as it goes, so a session cut off by a crash is undone on the next launch.
namespace vth::win::genesis
{
    struct ManagerResult {
        bool        ok = true;
        QStringList success;
        QStringList failure;
        QString     error;
    };

    struct ProcessEntry {
        QString name;         // e.g. "Discord.exe"
        QString imagePath;    // empty if it couldn't be read
        quint32 pid = 0;
    };

    struct ServiceState {
        QString name;
        quint32 startType = 0;
        bool    running   = false;
    };

    // The PC at activation, captured before anything changes.
    struct Baseline {
        QDateTime             capturedAt;
        QVector<ProcessEntry> processes;
        QVector<ServiceState> services;
    };

    struct RegTweak {
        QString hive;          // "HKLM" or "HKCU"
        QString subkey;
        QString name;
        bool    isDword = true;
        quint32 dwordValue = 0;
        QString stringValue;
    };

    struct RegSnapshot {
        QString hive;
        QString subkey;
        QString name;
        bool    existedBefore = false;
        bool    isDword = true;
        quint32 prevDword = 0;
        QString prevString;
    };

    struct ServiceSnapshot {
        QString name;
        quint32 prevStartType = 0;
        bool    wasRunning    = false;
    };

    // Saved before the rename is attempted, so a crash during it can be resolved.
    enum class RenameStage { Pending, Renamed };

    struct RenameRecord {
        QString     imageName;
        QString     originalPath;
        QString     renamedPath;
        RenameStage stage = RenameStage::Pending;
    };

    struct State {
        bool        active = false;
        QDateTime   startedAt;
        QString     previousPowerPlanGuid;
        int         processesKilled = 0;

        Baseline              baseline;
        QVector<RenameRecord> renames;

        // By name, because the PIDs mean nothing to the next launch.
        QStringList suspendedProcessNames;
        QStringList powerThrottlingDisabledFor;

        bool        processorIdleDisabled    = false;

        QVector<ServiceSnapshot> servicesStopped;
        QVector<RegSnapshot>     registrySnapshots;

        bool        explorerSuspended        = false;
        bool        notificationsDisabled    = false;
        quint32     timerResolutionRequested = 0;    // 100-ns units, 0 if not requested
        bool        mmcssEnabled             = false;
        bool        drsBasePerfApplied       = false;
        QStringList lastKilledNames;
    };

    const QStringList& defaultCloseList();

    // Every entry can be removed. The ones that can't are in ProtectedProcesses.
    const QStringList& defaultExceptions();

    // An empty saved list stays empty, and nothing saved means the defaults.
    QStringList closeListFromSettings();
    QStringList exceptionsFromSettings();
    void        saveCloseList(const QStringList& names);
    void        saveExceptions(const QStringList& names);

    // The protected images plus the user's exceptions.
    QStringList effectiveExceptions(const QStringList& userExceptions);

    // Removes protected processes and exceptions. Always go through this before
    // acting on a process name.
    QStringList allowedTargets(const QStringList& requested,
                               const QStringList& exceptions);

    QVector<ProcessEntry> enumerateProcesses();

    struct ActivateOptions {
        bool killProcessList         = true;
        bool suspendProcessList      = false;
        bool stopServices            = false;
        bool applyRegistryTweaks     = false;
        bool applyPrivacyTweaks      = false;
        bool purgeStandbyList        = true;
        bool disableNotifications    = false;
        bool suspendExplorer         = false;
        bool requestTimerResolution  = true;
        bool enableMmcss             = true;
        bool trimWorkingSets         = true;
        bool runGhostDeviceCleanup   = false;   // can't be undone
        bool applyNvidiaPerfMode     = false;
        bool disableProcessorIdle    = false;
        bool disablePowerThrottling  = true;
        // Renames each closed app's program file so it can't restart. If a session
        // is interrupted, the files stay renamed until Vethida runs again.
        bool renameExecutables       = false;
        QString targetExe;                      // empty means the foreground window
    };

    struct ActivateResult {
        bool    ok = false;
        QString error;
        int         registryValuesApplied = 0;
        int         privacyValuesApplied  = 0;
        bool        standbyListPurged     = false;
        quint32     timerResolutionAchieved = 0;
        int         ghostDevicesRemoved   = 0;
        int         ghostDevicesFailed    = 0;
        int         nvidiaPerfValuesApplied = 0;
        int         nvidiaSubkeysTouched    = 0;
        int         processesSuspended      = 0;
        int         powerThrottlingProcesses = 0;
        int         executablesRenamed       = 0;
        QStringList killedNames;
        QStringList suspendedNames;
        QStringList renameFailures;
    };

    ActivateResult activate(const ActivateOptions& opts,
                            const QStringList&     closeList,
                            const QStringList&     exceptions,
                            State&                 stateOut);

    bool deactivate(State& state);

    // Whether a session is on, or part of an earlier one still has to be undone.
    bool hasUnfinishedSession(const State& state);

    // success is what was closed, failure is what couldn't be.
    ManagerResult closeProcesses(const QStringList& names, const QStringList& exceptions);

    ManagerResult purgeStandbyList();

    ActivateOptions optionsFromSettings();

    State loadPersistedState();
    bool  savePersistedState(const State& s);

    // Undoes a session that didn't end. Runs at startup before the window exists, so
    // renamed program files are back before anything is on screen.
    struct RecoveryReport {
        QStringList unrestored;
        bool        ran              = false;
        int         namesRestored    = 0;
        int         servicesRestored = 0;
        QDateTime   sessionStarted;
    };
    RecoveryReport recoverInterruptedSession();
}

namespace vth::win::genesis
{
    struct GhostCleanupResult {
        int removed = 0;
        int failed  = 0;
        int skipped = 0;
    };
    struct NvidiaPerfModeResult {
        bool        ok = false;
        int         valuesApplied = 0;
        int         subkeysTouched = 0;
        QStringList failures;
    };
}
