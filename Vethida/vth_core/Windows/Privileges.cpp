#include "Privileges.h"

#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>

namespace vth::win
{

    bool isElevated() noexcept {
        return ::IsUserAnAdmin() != FALSE;
    }

    bool relaunchAsAdmin() noexcept {
        wchar_t path[MAX_PATH] = {};
        if (::GetModuleFileNameW(nullptr, path, MAX_PATH) == 0)
            return false;
        SHELLEXECUTEINFOW sei = { sizeof(sei) };
        sei.lpVerb = L"runas";
        sei.lpFile = path;
        sei.nShow  = SW_SHOWNORMAL;
        return ::ShellExecuteExW(&sei) != FALSE;
    }

}
