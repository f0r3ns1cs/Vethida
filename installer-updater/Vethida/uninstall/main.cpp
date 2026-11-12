#include <windows.h>

#include <commctrl.h>
#include <knownfolders.h>
#include <objbase.h>
#include <shellapi.h>

#include <string>

#include "../setup/InstallerCommon.h"

#pragma comment(lib, "comctl32.lib")

namespace
{
    using namespace vth::setup;

    struct Options {
        bool silent     = false;
        bool removeData = false;
        bool phase2     = false; // running as the temp copy
        DWORD parentPid = 0;
        std::wstring installDir;
    };

    Options parseCommandLine()
    {
        Options opt;
        int argc = 0;
        wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
        if (!argv) return opt;
        for (int i = 1; i < argc; ++i) {
            const std::wstring a = argv[i];
            if (_wcsicmp(a.c_str(), L"/silent") == 0
                || _wcsicmp(a.c_str(), L"/S") == 0)
                opt.silent = true;
            else if (_wcsicmp(a.c_str(), L"/removedata") == 0)
                opt.removeData = true;
            else if (_wcsicmp(a.c_str(), L"--phase2") == 0 && i + 2 < argc) {
                opt.phase2 = true;
                opt.parentPid = static_cast<DWORD>(_wtoi(argv[++i]));
                opt.installDir = argv[++i];
            }
        }
        LocalFree(argv);
        return opt;
    }

    bool confirmUninstall(bool& removeData)
    {
        TASKDIALOGCONFIG cfg{};
        cfg.cbSize = sizeof(cfg);
        cfg.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION;
        cfg.pszWindowTitle = L"Uninstall Vethida";
        cfg.pszMainInstruction = L"Remove Vethida from this computer?";

        TASKDIALOG_BUTTON buttons[2] = {
            { IDOK, L"Uninstall" },
            { IDCANCEL, L"Cancel" },
        };
        cfg.pButtons = buttons;
        cfg.cButtons = 2;
        cfg.nDefaultButton = IDCANCEL;
        cfg.pszVerificationText = L"Also remove my settings and logs";

        int pressed = 0;
        BOOL verified = FALSE;
        if (FAILED(TaskDialogIndirect(&cfg, &pressed, nullptr, &verified)))
            return false;
        removeData = verified == TRUE;
        return pressed == IDOK;
    }

    // copy ourselves to temp and run from there, can't delete the folder we're in
    int spawnPhase2(const Options& opt)
    {
        wchar_t tempDir[MAX_PATH];
        GetTempPathW(MAX_PATH, tempDir);
        wchar_t tempExe[MAX_PATH];
        swprintf_s(tempExe, L"%svth-uninstall-%08lx.exe", tempDir,
                   GetCurrentProcessId());

        if (!CopyFileW(modulePath().c_str(), tempExe, FALSE)) return 1;

        wchar_t cmd[1024];
        swprintf_s(cmd, L"\"%s\" --phase2 %lu \"%s\"%s%s", tempExe,
                   GetCurrentProcessId(), moduleDirectory().c_str(),
                   opt.removeData ? L" /removedata" : L"",
                   opt.silent ? L" /silent" : L"");

        STARTUPINFOW si{};
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi{};
        if (!CreateProcessW(tempExe, cmd, nullptr, nullptr, FALSE, 0, nullptr,
                            tempDir, &si, &pi))
            return 1;
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return 0;
    }

    void waitForParent(DWORD pid)
    {
        HANDLE parent = OpenProcess(SYNCHRONIZE, FALSE, pid);
        if (parent) {
            WaitForSingleObject(parent, 30'000);
            CloseHandle(parent);
        }
    }

    bool askToForceClose()
    {
        TASKDIALOGCONFIG cfg{};
        cfg.cbSize = sizeof(cfg);
        cfg.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_USE_COMMAND_LINKS;
        cfg.pszWindowTitle = L"Uninstall Vethida";
        cfg.pszMainIcon = TD_WARNING_ICON;
        cfg.pszMainInstruction = L"Vethida did not close";
        cfg.pszContent =
            L"It was asked to close and has not. Ending it now is safe unless "
            L"it is part-way through applying or undoing a change, in which "
            L"case Vethida may lose track of how to put that change back.";

        const TASKDIALOG_BUTTON buttons[] = {
            { 1001, L"Cancel the uninstall\nClose Vethida yourself, then try again" },
            { 1002, L"End Vethida and continue" },
        };
        cfg.pButtons = buttons;
        cfg.cButtons = ARRAYSIZE(buttons);
        cfg.dwCommonButtons = 0;
        cfg.nDefaultButton = 1001;

        int pressed = 0;
        if (FAILED(TaskDialogIndirect(&cfg, &pressed, nullptr, nullptr)))
            return false;
        return pressed == 1002;
    }

    // phase 2 gets the dir from the command line, so make sure it actually has Vethida.exe in it
    bool looksLikeInstallDir(const std::wstring& dir)
    {
        if (dir.empty())
            return false;
        if (dir.size() < 4) // drive root or similar
            return false;

        const DWORD attrs = GetFileAttributesW(dir.c_str());
        if (attrs == INVALID_FILE_ATTRIBUTES || !(attrs & FILE_ATTRIBUTE_DIRECTORY))
            return false;

        const std::wstring exe = joinPath(dir, std::wstring(kProductName) + L".exe");
        return GetFileAttributesW(exe.c_str()) != INVALID_FILE_ATTRIBUTES;
    }

    int runPhase2(const Options& opt)
    {
        if (!looksLikeInstallDir(opt.installDir)) {
            if (!opt.silent) {
                TaskDialog(nullptr, nullptr, L"Uninstall Vethida",
                           L"Nothing was removed",
                           L"The folder this uninstaller was pointed at does not "
                           L"contain a Vethida installation, so it has been left "
                           L"alone. Run the uninstaller from Windows Settings.",
                           TDCBF_CLOSE_BUTTON, TD_ERROR_ICON, nullptr);
            }
            return 1;
        }

        waitForParent(opt.parentPid);

        // nothing gets deleted until the app is closed
        if (!closeRunningApp(opt.installDir, 10'000, CloseMode::Graceful)) {
            if (opt.silent) return 1;
            if (!askToForceClose()) return 1;
            if (!closeRunningApp(opt.installDir, 5'000, CloseMode::Force)) {
                TaskDialog(nullptr, nullptr, L"Uninstall Vethida",
                           L"Vethida could not be closed",
                           L"Nothing has been removed. Restart this PC and run "
                           L"the uninstaller again.",
                           TDCBF_CLOSE_BUTTON, TD_ERROR_ICON, nullptr);
                return 1;
            }
        }

        // leftovers get deleted on reboot, so remove the registry entry either way
        const bool treeGone = deleteTree(opt.installDir, L"");
        DeleteFileW(startMenuShortcutPath().c_str());
        DeleteFileW(desktopShortcutPath().c_str());
        removeUninstallRegistry();

        bool dataGone = true;
        if (opt.removeData) {
            const std::wstring localAppData = knownFolder(FOLDERID_LocalAppData);
            if (localAppData.empty()) {
                dataGone = false;
            } else {
                // %LOCALAPPDATA%\<company>\<product>, plus the fallback without the company
                const std::wstring companyDir = joinPath(localAppData, kDataFolderCompany);
                const bool primaryGone  = deleteTree(joinPath(companyDir, kProductName), L"");
                const bool fallbackGone = deleteTree(joinPath(localAppData, kProductName), L"");
                dataGone = primaryGone && fallbackGone;

                RemoveDirectoryW(companyDir.c_str()); // only works if it's empty
            }
        }

        if (!opt.silent) {
            if (treeGone && dataGone) {
                TaskDialog(nullptr, nullptr, L"Uninstall Vethida",
                           L"Vethida was removed",
                           opt.removeData
                               ? L"The application and your Vethida settings are "
                                 L"gone.\n\nThanks for trying Vethida. Feedback is "
                                 L"always welcome at support@vethida.com."
                               : L"Your settings and logs were kept.\n\nThanks for "
                                 L"trying Vethida. Feedback is always welcome at "
                                 L"support@vethida.com.",
                           TDCBF_CLOSE_BUTTON, nullptr, nullptr);
            } else {
                TaskDialog(nullptr, nullptr, L"Uninstall Vethida",
                           L"Vethida was removed, but some files are still in use",
                           L"They are scheduled to be deleted the next time this "
                           L"PC restarts. Nothing else is left to do.",
                           TDCBF_CLOSE_BUTTON, TD_WARNING_ICON, nullptr);
            }
        }

        // delete the temp copy on reboot
        MoveFileExW(modulePath().c_str(), nullptr,
                    MOVEFILE_DELAY_UNTIL_REBOOT);
        return (treeGone && dataGone) ? 0 : 2;
    }
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    if (FAILED(CoInitializeEx(nullptr,
                              COINIT_APARTMENTTHREADED
                                  | COINIT_DISABLE_OLE1DDE)))
        return 1;

    INITCOMMONCONTROLSEX icc{ sizeof(icc), ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);

    Options opt = parseCommandLine();

    int rc;
    if (opt.phase2) {
        rc = runPhase2(opt);
    } else if (opt.silent || confirmUninstall(opt.removeData)) {
        rc = spawnPhase2(opt);
    } else {
        rc = 0;
    }

    CoUninitialize();
    return rc;
}
