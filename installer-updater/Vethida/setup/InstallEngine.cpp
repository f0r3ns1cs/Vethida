#include "InstallEngine.h"

#include <windows.h>

#include "Extract7z.h"
#include "InstallerCommon.h"

namespace vth::setup
{
    InstallOutcome performInstall(const InstallRequest& request,
                                  const InstallProgressFn& progress)
    {
        InstallOutcome out;
        const std::wstring& installDir = request.installDir;
        out.exePath = joinPath(installDir, L"Vethida.exe");

        out.wasRunning = isAppRunning(installDir);
        if (!closeRunningApp(installDir, 10'000)) {
            out.error = L"Vethida is still running and could not be closed. "
                        L"Close it and run Setup again.";
            return out;
        }

        if (!ensureDirectory(installDir)) {
            out.error = L"Could not create the installation folder:\n"
                        + installDir;
            return out;
        }

        ExtractProgressFn extractProgress;
        if (progress) {
            extractProgress = [&progress](int done, int total,
                                          const std::wstring& name) {
                progress(done, total, name);
            };
        }

        const ExtractResult extracted =
            extractEmbeddedPayload(modulePath(), installDir, extractProgress);
        if (!extracted.ok) {
            out.error = extracted.error;
            return out;
        }

        const std::wstring version = moduleFileVersion();
        const DWORD sizeKb =
            static_cast<DWORD>(extracted.bytesExtracted / 1024ull);
        writeUninstallRegistry(installDir, version, sizeKb);

        createShortcut(startMenuShortcutPath(), out.exePath, installDir,
                       L"Windows optimization suite");
        if (request.desktopShortcut) {
            createShortcut(desktopShortcutPath(), out.exePath, installDir,
                           L"Windows optimization suite");
        }

        out.ok = true;
        return out;
    }
}
