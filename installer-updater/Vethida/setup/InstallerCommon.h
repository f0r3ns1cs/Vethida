#pragma once

#include <windows.h>

#include <shlobj.h>

#include <string>
#include <vector>

#include "../version.h"

#ifndef VTH_WIDEN
#define VTH_WIDEN2(x) L##x
#define VTH_WIDEN(x) VTH_WIDEN2(x)
#endif

namespace vth::setup
{
    constexpr wchar_t kProductName[]   = VTH_WIDEN(VETHIDA_PRODUCT_NAME);
    constexpr wchar_t kPublisherName[] = L"Rens Engineering";

    // %LOCALAPPDATA%\<company>\<product>
    constexpr wchar_t kDataFolderCompany[] = VTH_WIDEN(VETHIDA_COMPANY_NAME);
    constexpr wchar_t kUninstallKey[]  =
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Vethida";

    std::wstring modulePath();
    std::wstring moduleDirectory();
    std::wstring moduleFileVersion();

    // %ProgramFiles%\Vethida (native 64-bit view).
    std::wstring installDirectory();
    std::wstring knownFolder(REFKNOWNFOLDERID id);

    std::wstring joinPath(const std::wstring& a, const std::wstring& b);
    bool ensureDirectory(const std::wstring& path);

    // Force kills the process after the wait. Not the default since killing it mid
    // write can corrupt the tweak ledger.
    enum class CloseMode { Graceful, Force };

    // sends WM_CLOSE and waits, true when nothing is left running
    bool closeRunningApp(const std::wstring& installDir, DWORD gracefulWaitMs,
                         CloseMode mode = CloseMode::Graceful);
    bool isAppRunning(const std::wstring& installDir);

    bool createShortcut(const std::wstring& linkPath,
                        const std::wstring& targetPath,
                        const std::wstring& workingDir,
                        const std::wstring& description);

    bool writeUninstallRegistry(const std::wstring& installDir,
                                const std::wstring& displayVersion,
                                DWORD estimatedSizeKb);
    void removeUninstallRegistry();

    std::wstring startMenuShortcutPath();
    std::wstring desktopShortcutPath();

    // keepFile is skipped (the uninstaller that's running)
    bool deleteTree(const std::wstring& dir, const std::wstring& keepFile);

    void launchUnelevated(const std::wstring& exePath);
}
