#pragma once

#include <windows.h>

#include <string>

namespace vth::setup
{
    struct WizardParams {
        std::wstring defaultInstallDir;  // %ProgramFiles%\Vethida
        std::wstring version;            // display version, e.g. "0.1.260616"
        bool         upgrade = false;    // an existing install was detected
        bool         desktopDefault = false;  // /desktopicon on the command line
    };

    struct WizardOutcome {
        bool         installed = false;  // install completed and user finished
        bool         launch = false;     // user asked to launch on finish
        bool         wasRunning = false; // app was open when setup started
        std::wstring exePath;
    };

    // blocks until finished or canceled
    WizardOutcome runWizard(HINSTANCE instance, const WizardParams& params);
}
