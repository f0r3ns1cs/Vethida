#include "DarkMode.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")

namespace vth::win {
    void applyImmersiveDarkMode(WId windowId, bool dark) noexcept
    {
        const auto hwnd = reinterpret_cast<HWND>(windowId);
        if (!hwnd) return;

        // DWMWA_USE_IMMERSIVE_DARK_MODE was documented with value 20 from
        // Windows 10 Build 18985 onwards; older insider builds used 19.
        constexpr DWORD kDarkModeAttr  = 20;
        constexpr DWORD kDarkModeAttrOld = 19;

        BOOL value = dark ? TRUE : FALSE;
        HRESULT hr = DwmSetWindowAttribute(hwnd, kDarkModeAttr,
                                            &value, sizeof(value));
        if (FAILED(hr)) {
            DwmSetWindowAttribute(hwnd, kDarkModeAttrOld,
                                   &value, sizeof(value));
        }
    }
}
