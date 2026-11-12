#pragma once

#include "Registry.h"

#include <QImage>
#include <QMetaType>
#include <QString>
#include <QVector>

// Run keys and Startup folders. Disabling is reversible: a Run value moves to a
// vethida_disabled subkey, and a file moves to a Vethida_Disabled folder.
namespace vth::win::startup
{
    enum class Location {
        RunKey,
        StartupFolder,
        ScheduledTask,
        DelayedStartService,
    };

    using Hive = reg::Hive;

    enum class SignatureStatus {
        Unknown,
        Verified,
        Unverified,
    };

    QString locationLabel(Location l);
    QString signatureStatusLabel(SignatureStatus s);
    QString staleMirrorNotice();

    struct StartupEntry {
        QString id;             // stable across refreshes

        QString displayName;
        QString publisher;
        QString description;
        QString command;        // as registered
        QString resolvedPath;   // best effort: variables expanded, arguments removed
        QString fileName;

        QImage iconImage;

        SignatureStatus signature = SignatureStatus::Unknown;
        QString         signerName;

        Location location = Location::RunKey;
        bool     enabled  = true;

        // In both the live Run key and the mirror, so Windows still starts it.
        bool staleDisableMirror = false;

        Hive    runKeyHive = Hive::CurrentUser;
        QString runKeySubkey;
        QString runKeyValue;
        bool    runKeyWow64 = false;

        QString startupFolderPath;
        bool    startupFolderCommon = false;
        QString taskPath;
        QString serviceName;
    };

    QVector<StartupEntry> enumerate();

    enum class ActionResult {
        Ok,
        AccessDenied,
        NotFound,
        AlreadyExists,
        OtherError,
    };
    QString actionResultLabel(ActionResult r);

    ActionResult setEnabled(const StartupEntry& entry, bool enabled);

    // From the latest Event 100 in the Diagnostics-Performance log.
    struct BootMetrics {
        qint64 bootTimeMs     = 0;
        qint64 mainPathBootMs = 0;
        qint64 postBootMs     = 0;
        bool   valid          = false;
    };

    BootMetrics bootMetrics();

    // Vethida's own start at sign-in.
    namespace self
    {
        bool launchesAtLogon();

        // Returns whether the task ended up in the requested state.
        bool setLaunchAtLogon(bool enabled);
    }
}

Q_DECLARE_METATYPE(vth::win::startup::StartupEntry)
