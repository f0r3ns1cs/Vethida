#pragma once

#include <QByteArray>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>
#include <atomic>
#include <functional>

// Downloads, unpacks, trims and installs NVIDIA drivers. Everything here blocks,
// so call it from a worker.
namespace vth::win::nvidia
{
    struct GpuInfo {
        QString name;
        QString deviceInstanceId;
        QString driverVersion;
        bool    detected = false;
    };

    GpuInfo detectGpu();

    struct ResolveResult {
        bool    ok = false;
        QString url;
        QString version;
        QString error;
    };

    // The newest Game Ready driver, from the undocumented service behind NVIDIA's
    // driver finder. Fails if the adapter name can't be matched to a product.
    ResolveResult resolveLatestDriverUrl(const GpuInfo& gpu);

    struct DownloadProgress {
        qint64 bytesReceived = 0;
        qint64 bytesTotal    = -1;
    };

    using DownloadProgressFn = std::function<void(DownloadProgress)>;

    enum class DownloadResult {
        Ok,
        InvalidUrl,
        NetworkError,
        WriteError,
        Canceled,
    };

    DownloadResult downloadFromUrl(const QString& url,
                                   const QString& outputPath,
                                   const DownloadProgressFn& progress,
                                   std::atomic<bool>& cancel);

    enum class ExtractResult {
        Ok,
        Failed,
    };

    ExtractResult extractPackage(const QString& packagePath,
                                 const QString& outputDir,
                                 QString* errorOut);

    struct Component {
        QString id;            // section name in setup.cfg
        QString displayName;
        QString description;
        bool    required        = false;
        bool    defaultEnabled  = true;
        QStringList dependsOn;
    };

    const QVector<Component>& componentCatalog();
    QStringList               recommendedComponentIds();
    QSet<QString>             resolveDependencies(const QSet<QString>& chosen);

    struct InstallSetting {
        QString id;
        QString displayName;
        QString description;
        bool    defaultEnabled;
    };

    const QVector<InstallSetting>& installSettingsCatalog();

    struct PatchResult {
        bool    ok = false;
        QString error;
        int         componentsKept    = 0;
        int         componentsRemoved = 0;
        int         settingsApplied   = 0;
        QStringList removedSections;
        QStringList appliedSettings;
    };

    // Rewrites setup.cfg and presentations.cfg to keep only the given components and
    // their dependencies.
    PatchResult patchPackage(const QString&       extractedDir,
                             const QSet<QString>& componentIdsKept,
                             const QSet<QString>& settingIdsEnabled);

    struct InstallResult {
        bool    ok = false;
        int     exitCode = 0;
        QString error;
    };

    // Runs setup.exe silently and doesn't restart.
    InstallResult runSilentInstall(const QString& extractedDir);

    struct CleanOptions {
        bool removePhysX     = true;
        bool removeGfe       = true;   // GeForce Experience and the NVIDIA App
        bool removeBroadcast = true;
        bool removeLegacy    = true;   // the old updater's local account

        // Best effort, because System Protection is off by default on Windows 11.
        bool createRestorePoint = true;
        bool removeAudio    = true;
    };

    struct CleanResult {
        bool        ok = false;
        QString     error;
        bool        rebootRequired = false;
        bool        restorePointCreated = false;

        QStringList servicesStopped;
        QStringList servicesDeleted;
        QStringList infsRemoved;
        QStringList infsFailed;
        QStringList registryKeysRemoved;
        QStringList pathsRemoved;
        QStringList tasksRemoved;
    };

    using CleanProgressFn = std::function<void(const QString& stage, int percent)>;

    // Removes the installed driver and what it left behind: driver store packages,
    // services, registry keys, files, folders and scheduled tasks.
    CleanResult runCleanRemoval(const CleanOptions& options,
                                const CleanProgressFn& progress);

    // Whether the NVIDIA App or GeForce Experience is installed.
    bool companionAppInstalled();

    enum class TweakTarget {
        NvidiaClassSubkey,    // Class\{4d36e968-...}\NNNN, one per NVIDIA adapter
        GraphicsDrivers,      // Control\GraphicsDrivers
        NvDisplayContainer,   // Services\NVDisplay.ContainerLocalSystem\...\NvcDispCorePlugin
        WindowsUpdatePolicy,  // Policies\Microsoft\Windows\WindowsUpdate
        HdmiAudioClassSubkey, // Class\{4d36e96c-...}\NNNN, NVIDIA HDMI audio
    };

    // A value under the target, at subpath if there is one. isBinary wins over
    // isDword, and neither means REG_SZ.
    struct RegValueSpec {
        QString subpath;
        QString name;
        bool    isDword     = true;
        quint32 dwordValue  = 0;
        QString stringValue;

        bool       isBinary = false;
        QByteArray binaryValue;
    };

    struct NvidiaRegTweak {
        QString id;
        QString name;
        QString description;

        TweakTarget target = TweakTarget::NvidiaClassSubkey;

        // The value at the target itself. Empty skips it.
        QString valueName;
        bool    isDword     = true;
        quint32 dwordValue  = 0;
        QString stringValue;

        bool       isBinary = false;
        QByteArray binaryValue;

        QVector<RegValueSpec> extraValues;

        bool    defaultEnabled = false;
    };

    const QVector<NvidiaRegTweak>& registryTweaksCatalog();

    struct ApplyResult {
        bool        ok = false;
        QString     error;
        QStringList success;
        QStringList failure;
        QStringList classKeysTouched;
    };

    // Writes each selected tweak and records it in TweakStateTracker for undo.
    ApplyResult applyRegistryTweaks(const QSet<QString>& tweakIds);

    // Disables the NvTelemetryContainer service and the NvTm scheduled tasks, and
    // records them in TweakStateTracker. Does nothing for what's already gone.
    void applyDrvtlmRuntime();
}

namespace vth::win::nvidia
{
    QString downloadResultLabel(DownloadResult r);
    QString extractResultLabel(ExtractResult r);
    ApplyResult revertRegistryTweaks();
    struct RegSnapshot {
        TweakTarget target = TweakTarget::NvidiaClassSubkey;
        QString classSubkey;
        QString valueName;
        bool    existedBefore = false;
        bool    isDword = true;
        quint32 prevDword = 0;
        QString prevString;
        bool       isBinary = false;
        QByteArray prevBinary;
    };
    QVector<RegSnapshot> loadPersistedSnapshots();
    void                 savePersistedSnapshots(const QVector<RegSnapshot>& snaps);
    struct DrvtlmRuntimeResult {
        bool ok = false;
        QString error;
        bool serviceFound      = false;
        bool serviceDisabled   = false;
        quint32 prevServiceStart = 0;
        QStringList tasksDisabled;
        QStringList tasksMissing;
        QStringList tasksFailed;
    };
}
