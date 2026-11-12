#pragma once

#include <QString>

namespace vth::win
{
    // Writes a minidump and leaves a breadcrumb for the next launch on any abnormal
    // exit, then ends the process.
    void installCrashHandler();

    void uninstallCrashHandler();

    // The dump left by the last session, empty if it shut down cleanly.
    bool    hadPreviousCrash();
    QString previousCrashDumpPath();

    void acknowledgePreviousCrash();

    // Deletes all but the newest `keep` dumps.
    void pruneOldDumps(int keep);
}
