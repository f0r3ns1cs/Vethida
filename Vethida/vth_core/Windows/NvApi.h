#pragma once

#include <QString>
#include <QVector>

#include <functional>

// NVIDIA driver profile settings, through nvapi64.dll.
namespace vth::win::nvapi
{
    struct ProfileSetting {
        quint32     id;
        quint32     value;
        const char* label;    // for display and logging
    };

    // Whether an NVIDIA driver is present and NVAPI loaded.
    bool available();

    const QVector<ProfileSetting>& optimalProfile();

    // Writes optimalProfile() to the global profile and returns how many settings
    // were saved. Touches the driver, so call it from a worker.
    int applyOptimalProfile(const std::function<void(int done, int total)>& onProgress = {});

    // The first apply saves the earlier values. True while that copy exists.
    bool hasProfileBackup();

    // ok with valuesWritten below the requested count is a partial success, and
    // error names what failed.
    struct Result {
        bool    ok = false;
        int     valuesWritten = 0;
        QString error;
    };

    // The performance settings Genesis uses for a session, and their driver
    // defaults.
    Result applyBasePerfProfile();
    Result revertBasePerfProfile();

    // Puts back what applyOptimalProfile() saved, then removes the saved copy.
    Result revertOptimalProfile();
}
