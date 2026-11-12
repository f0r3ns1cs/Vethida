#pragma once

#include <functional>
#include <string>

namespace vth::setup
{
    struct InstallRequest {
        std::wstring installDir;
        bool         desktopShortcut = false;
    };

    struct InstallOutcome {
        bool         ok = false;
        std::wstring error;
        std::wstring exePath;      // <installDir>\Vethida.exe
        bool         wasRunning = false;  // app was open when setup started
    };

    // (filesDone, filesTotal, currentName). Runs on the calling thread.
    using InstallProgressFn =
        std::function<void(int filesDone, int filesTotal,
                           const std::wstring& currentName)>;

    // close running app, extract payload, uninstall entry, shortcuts.
    InstallOutcome performInstall(const InstallRequest& request,
                                  const InstallProgressFn& progress);
}
