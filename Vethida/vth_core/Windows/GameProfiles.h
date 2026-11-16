#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

namespace vth::win::gameprofiles
{

    // Locates known games' config files through well-known path templates and
    // applies or reverts a low-overhead settings profile. The existing file is
    // copied to <name>.vth-bak before anything is written, so a restore is a
    // single copy back.

    enum class ConfigFormat {
        Ini,            // sectioned key=value (UE GameUserSettings and friends)
        SourceCfg,      // Source-engine cfg: key value, sometimes quoted
        PlainKeyValue,  // sectionless key/value pairs (Apex videoconfig.txt)
        Directory,      // per-account subdirectories; not editable automatically
    };

    struct Profile {
        QString id;
        QString name;
        QString publisher;
        QString abbreviation;
        QString accentColor;
        QString settingsPath;       // resolved; empty when not found on this machine
        QString settingsTemplate;
        QString notes;
        ConfigFormat format = ConfigFormat::Ini;
    };

    struct Setting {
        QString section;    // empty for sectionless formats
        QString key;
        QString value;
        QString comment;    // user-facing description of the recommendation
    };

    struct ReadResult {
        bool             ok = false;
        QString          error;
        QVector<Setting> settings;
    };

    struct WriteResult {
        bool    ok = false;
        QString error;
        QString backupPath;         // empty if no backup was taken
        int     valuesWritten = 0;
    };

    QVector<Profile> enumerate();

    ReadResult readSettings(const Profile& p, const QStringList& keysOfInterest = {});

    WriteResult writeSettings(const Profile& p, const QVector<Setting>& settings);

    // Copies <name>.vth-bak back over the config. Ok when no backup exists.
    WriteResult restoreBackup(const Profile& p);

    // Recommended low-overhead settings for a known profile id; empty when
    // there is none.
    QVector<Setting> recommendedProfile(const QString& profileId);

}
