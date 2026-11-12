#include <windows.h>

#include <commctrl.h>
#include <objbase.h>
#include <shellapi.h>

#include <string>

#include "../version.h"
#include "InstallEngine.h"
#include "InstallerCommon.h"
#include "Wizard.h"

#pragma comment(lib, "comctl32.lib")

namespace
{
    using namespace vth::setup;

    struct Options {
        bool silent  = false;
        bool update  = false;
        bool desktop = false;
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
            else if (_wcsicmp(a.c_str(), L"/update") == 0)
                opt.update = opt.silent = true;
            else if (_wcsicmp(a.c_str(), L"/desktopicon") == 0)
                opt.desktop = true;
            // /norestart is ignored, we never restart anyway
        }
        LocalFree(argv);
        return opt;
    }

    void showError(const std::wstring& message)
    {
        TaskDialog(nullptr, nullptr, L"Vethida Setup",
                   L"Setup could not complete", message.c_str(),
                   TDCBF_CLOSE_BUTTON, TD_ERROR_ICON, nullptr);
    }

    int runSilent(const Options& opt)
    {
        InstallRequest req;
        req.installDir = installDirectory();
        req.desktopShortcut = opt.desktop;

        const InstallOutcome result = performInstall(req, {});
        if (!result.ok) {
            // still show errors for /update since the app already closed
            if (!opt.silent || opt.update) showError(result.error);
            return 1;
        }
        if (opt.update && result.wasRunning)
            launchUnelevated(result.exePath);
        return 0;
    }

    int runInteractive(const Options& opt)
    {
        const std::wstring installDir = installDirectory();
        const std::wstring appExe = joinPath(installDir, L"Vethida.exe");

        WizardParams params;
        params.defaultInstallDir = installDir;
        params.version = VTH_WIDEN(VETHIDA_VERSION_STRING);
        params.upgrade =
            GetFileAttributesW(appExe.c_str()) != INVALID_FILE_ATTRIBUTES;
        params.desktopDefault = opt.desktop;

        const WizardOutcome outcome = runWizard(GetModuleHandleW(nullptr),
                                                params);
        if (outcome.installed && outcome.launch)
            launchUnelevated(outcome.exePath);
        return 0;
    }
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    // manifest sets this too
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    if (FAILED(CoInitializeEx(nullptr,
                              COINIT_APARTMENTTHREADED
                                  | COINIT_DISABLE_OLE1DDE)))
        return 1;

    INITCOMMONCONTROLSEX icc{ sizeof(icc),
                              ICC_STANDARD_CLASSES | ICC_PROGRESS_CLASS };
    InitCommonControlsEx(&icc);

    const Options opt = parseCommandLine();
    const int rc = opt.silent ? runSilent(opt) : runInteractive(opt);

    CoUninitialize();
    return rc;
}
