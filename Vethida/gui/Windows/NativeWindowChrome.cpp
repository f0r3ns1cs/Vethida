#include "pch.h"
#include "NativeWindowChrome.h"

#include "../Theme/DesignTokens.h"
#include "../Utils/TablerIcon.h"

#include <QWindow>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#include <windowsx.h>
#include <shellapi.h>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif
#ifndef DWMWCP_ROUND
#define DWMWCP_ROUND 2
#endif
#endif

namespace vth::platform
{
    QIcon captionGlyphIcon(CaptionButton button, const QColor& color, bool maximized)
    {
        namespace tokens = vth::theme::tokens;
        switch (button) {
            case CaptionButton::Minimize:
                return vth::icons::icon(QStringLiteral("minus"), color, tokens::CaptionGlyph);
            case CaptionButton::MaximizeRestore:
                // The overlap when the window is already maximized, which is
                // what Windows draws for restore: two rectangles, one behind.
                return vth::icons::icon(maximized ? QStringLiteral("copy")
                                                  : QStringLiteral("square"),
                                        color, tokens::CaptionGlyph);
            case CaptionButton::Close:
                return vth::icons::icon(QStringLiteral("x"), color, tokens::CaptionGlyphClose);
            case CaptionButton::None:
                break;
        }
        return {};
    }

#ifdef Q_OS_WIN
    namespace
    {
        bool isMaximized(HWND hwnd)
        {
            WINDOWPLACEMENT placement{};
            placement.length = sizeof(placement);
            if (GetWindowPlacement(hwnd, &placement))
                return placement.showCmd == SW_SHOWMAXIMIZED;
            return false;
        }

        UINT dpiForWindow(HWND hwnd)
        {
            const UINT dpi = GetDpiForWindow(hwnd);
            return dpi != 0 ? dpi : 96;
        }

        int resizeFrameThickness(HWND hwnd)
        {
            const UINT dpi = dpiForWindow(hwnd);
            return GetSystemMetricsForDpi(SM_CXFRAME, dpi) + GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
        }

        CaptionButton captionFromHitTest(WPARAM hitTest)
        {
            switch (hitTest) {
            case HTMINBUTTON:
                return CaptionButton::Minimize;
            case HTMAXBUTTON:
                return CaptionButton::MaximizeRestore;
            case HTCLOSE:
                return CaptionButton::Close;
            default:
                return CaptionButton::None;
            }
        }

        // leave 1px for an auto-hide taskbar when maximized
        void reserveAutoHideTaskbar(HWND hwnd, RECT* clientRect)
        {
            APPBARDATA appBar{};
            appBar.cbSize = sizeof(appBar);
            if ((SHAppBarMessage(ABM_GETSTATE, &appBar) & ABS_AUTOHIDE) == 0)
                return;

            const HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
            MONITORINFO monitorInfo{};
            monitorInfo.cbSize = sizeof(monitorInfo);
            if (!GetMonitorInfoW(monitor, &monitorInfo))
                return;

            const RECT mon = monitorInfo.rcMonitor;
            auto edgeHasBar = [&](UINT edge) {
                APPBARDATA query{};
                query.cbSize = sizeof(query);
                query.uEdge = edge;
                query.rc = mon;
                return SHAppBarMessage(ABM_GETAUTOHIDEBAREX, &query) != 0;
            };

            if (edgeHasBar(ABE_BOTTOM))
                clientRect->bottom -= 1;
            else if (edgeHasBar(ABE_TOP))
                clientRect->top += 1;
            else if (edgeHasBar(ABE_LEFT))
                clientRect->left += 1;
            else if (edgeHasBar(ABE_RIGHT))
                clientRect->right -= 1;
        }

        // system menu on right-click of our caption
        void showSystemMenu(HWND hwnd, int screenX, int screenY)
        {
            HMENU menu = GetSystemMenu(hwnd, FALSE);
            if (!menu)
                return;

            const bool maximized = isMaximized(hwnd);
            const bool minimized = IsIconic(hwnd) != FALSE;

            const auto enable = [&](UINT item, bool on) {
                EnableMenuItem(menu, item,
                    MF_BYCOMMAND | (on ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
            };
            enable(SC_RESTORE,  maximized || minimized);
            enable(SC_MOVE,     !maximized && !minimized);
            enable(SC_SIZE,     !maximized && !minimized);
            enable(SC_MINIMIZE, !minimized);
            enable(SC_MAXIMIZE, !maximized);
            enable(SC_CLOSE,    true);
            SetMenuDefaultItem(menu, SC_CLOSE, FALSE);

            const int cmd = TrackPopupMenu(
                menu, TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN,
                screenX, screenY, 0, hwnd, nullptr);
            if (cmd != 0)
                PostMessageW(hwnd, WM_SYSCOMMAND, static_cast<WPARAM>(cmd), 0);
        }
    }
#endif

    NativeWindowChrome::NativeWindowChrome(QWindow* window)
        : m_window(window)
    {
    }

#ifdef Q_OS_WIN
    qreal NativeWindowChrome::devicePixelRatioForWindow() const
    {
        return m_window ? m_window->devicePixelRatio() : 1.0;
    }

    void NativeWindowChrome::notifyButtons()
    {
        if (m_buttonSink)
            m_buttonSink(m_hovered, m_pressed);
    }

    void NativeWindowChrome::invokeCaption(CaptionButton button)
    {
        if (!m_window)
            return;
        const HWND hwnd = reinterpret_cast<HWND>(m_window->winId());
        if (!hwnd)
            return;

        switch (button) {
        case CaptionButton::Minimize:
            SendMessageW(hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
            break;
        case CaptionButton::MaximizeRestore:
            SendMessageW(hwnd, WM_SYSCOMMAND, IsZoomed(hwnd) ? SC_RESTORE : SC_MAXIMIZE, 0);
            break;
        case CaptionButton::Close:
            SendMessageW(hwnd, WM_SYSCOMMAND, SC_CLOSE, 0);
            break;
        case CaptionButton::None:
            break;
        }
    }
#endif

    void NativeWindowChrome::applyTheme(bool dark)
    {
#ifdef Q_OS_WIN
        if (!m_window)
            return;
        const HWND hwnd = reinterpret_cast<HWND>(m_window->winId());
        if (!hwnd)
            return;

        m_dark = dark;

        // Windows 11 and late Windows 10 features. Older builds return a failure HRESULT
        // and change nothing.
        const BOOL useDark = dark ? TRUE : FALSE;
        DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDark, sizeof(useDark));

        const DWORD cornerPreference = DWMWCP_ROUND;
        DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &cornerPreference, sizeof(cornerPreference));
#else
        Q_UNUSED(dark);
#endif
    }

    void NativeWindowChrome::install(bool dark, bool resizable)
    {
        m_resizable = resizable;
#ifdef Q_OS_WIN
        if (!m_window)
            return;
        const HWND hwnd = reinterpret_cast<HWND>(m_window->winId());
        if (!hwnd)
            return;

        LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
        if (resizable) {
            style |= WS_OVERLAPPEDWINDOW;
        } else {
            style |= WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
            style &= ~static_cast<LONG_PTR>(WS_THICKFRAME | WS_MAXIMIZEBOX);
        }
        SetWindowLongPtrW(hwnd, GWL_STYLE, style);

        applyTheme(dark);

        SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                     SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
#else
        Q_UNUSED(dark);
#endif
    }

    bool NativeWindowChrome::handleNativeEvent(const QByteArray& eventType, void* message, qintptr* result)
    {
#ifdef Q_OS_WIN
        if (eventType != "windows_generic_MSG" && eventType != "windows_dispatcher_MSG")
            return false;

        MSG* msg = static_cast<MSG*>(message);
        const HWND hwnd = msg->hwnd;
        if (!hwnd)
            return false;

        switch (msg->message) {
        case WM_NCCALCSIZE: {
            if (msg->wParam == FALSE)
                return false;

            auto* params = reinterpret_cast<NCCALCSIZE_PARAMS*>(msg->lParam);
            RECT* clientRect = &params->rgrc[0];
            const LONG originalTop = clientRect->top;

            DefWindowProcW(hwnd, WM_NCCALCSIZE, msg->wParam, msg->lParam);

            if (isMaximized(hwnd)) {
                // maximized windows hang off the screen by the frame thickness
                const int frame = resizeFrameThickness(hwnd);
                clientRect->top = originalTop + frame;
                reserveAutoHideTaskbar(hwnd, clientRect);
            } else {
                clientRect->top = originalTop;
            }

            *result = 0;
            return true;
        }

        case WM_NCHITTEST: {
            LRESULT dwmResult = 0;
            if (DwmDefWindowProc(hwnd, msg->message, msg->wParam, msg->lParam, &dwmResult)) {
                *result = dwmResult;
                return true;
            }

            POINT clientPoint{ GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam) };
            ScreenToClient(hwnd, &clientPoint);

            const qreal dpr = devicePixelRatioForWindow();
            const QPoint logical(qRound(clientPoint.x / dpr), qRound(clientPoint.y / dpr));
            const HitArea area = m_hitTest ? m_hitTest(logical) : HitArea::Client;

            switch (area) {
            case HitArea::MinimizeButton:
                *result = HTMINBUTTON;
                return true;
            case HitArea::MaximizeButton:
                *result = HTMAXBUTTON; // required for Windows 11 Snap Layouts
                return true;
            case HitArea::CloseButton:
                *result = HTCLOSE;
                return true;
            default:
                break;
            }

            const bool maximized = isMaximized(hwnd);
            // 0 for fixed-size windows (no top resize)
            const int topBorder = m_resizable ? resizeFrameThickness(hwnd) : 0;

            // no top frame, so turn the top corners into diagonal hits ourselves
            const LRESULT frameHit = DefWindowProcW(hwnd, WM_NCHITTEST, msg->wParam, msg->lParam);
            const bool inTopBand = !maximized && clientPoint.y >= 0 && clientPoint.y < topBorder;
            switch (frameHit) {
            case HTLEFT:
                *result = inTopBand ? HTTOPLEFT : HTLEFT;
                return true;
            case HTRIGHT:
                *result = inTopBand ? HTTOPRIGHT : HTRIGHT;
                return true;
            case HTTOP:
            case HTBOTTOM:
            case HTTOPLEFT:
            case HTTOPRIGHT:
            case HTBOTTOMLEFT:
            case HTBOTTOMRIGHT:
                *result = frameHit;
                return true;
            default:
                break;
            }

            // Top-center resize strip, synthesized because the top frame was removed. Restricted to
            // draggable caption background so it never steals clicks from the controls.
            if (inTopBand && area == HitArea::Caption) {
                *result = HTTOP;
                return true;
            }

            *result = area == HitArea::Caption ? HTCAPTION : HTCLIENT;
            return true;
        }

        case WM_NCMOUSEMOVE: {
            const CaptionButton hovered = captionFromHitTest(msg->wParam);
            if (hovered != m_hovered) {
                m_hovered = hovered;
                notifyButtons();
            }
            if (hovered != CaptionButton::None) {
                TRACKMOUSEEVENT track{};
                track.cbSize = sizeof(track);
                track.dwFlags = TME_LEAVE | TME_NONCLIENT;
                track.hwndTrack = hwnd;
                TrackMouseEvent(&track);
            }
            LRESULT dwmResult = 0;
            if (DwmDefWindowProc(hwnd, msg->message, msg->wParam, msg->lParam, &dwmResult)) {
                *result = dwmResult;
                return true;
            }
            return false;
        }

        case WM_MOUSEMOVE: {
            if (m_hovered != CaptionButton::None) {
                m_hovered = CaptionButton::None;
                notifyButtons();
            }
            return false;
        }

        case WM_NCMOUSELEAVE: {
            if (m_hovered != CaptionButton::None) {
                m_hovered = CaptionButton::None;
                notifyButtons();
            }
            LRESULT dwmResult = 0;
            DwmDefWindowProc(hwnd, msg->message, msg->wParam, msg->lParam, &dwmResult);
            return false;
        }

        case WM_NCLBUTTONDOWN: {
            const CaptionButton button = captionFromHitTest(msg->wParam);
            if (button != CaptionButton::None) {
                m_pressed = button;
                notifyButtons();
                *result = 0;
                return true; // suppress default drag/maximize so the click registers on release
            }
            return false;
        }

        case WM_NCLBUTTONUP: {
            const CaptionButton button = captionFromHitTest(msg->wParam);
            if (button != CaptionButton::None) {
                const bool commit = button == m_pressed;
                m_pressed = CaptionButton::None;
                notifyButtons();
                if (commit)
                    invokeCaption(button);
                *result = 0;
                return true;
            }
            if (m_pressed != CaptionButton::None) {
                m_pressed = CaptionButton::None;
                notifyButtons();
            }
            return false;
        }

        case WM_NCRBUTTONUP: {
            // Right-click on the synthetic caption opens the native system menu,
            // exactly like the standard Windows title bar.
            if (msg->wParam == HTCAPTION || msg->wParam == HTSYSMENU) {
                showSystemMenu(hwnd, GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam));
                *result = 0;
                return true;
            }
            return false;
        }

        case WM_DPICHANGED:
            if (m_relayout)
                m_relayout();
            return false;

        case WM_THEMECHANGED:
        case WM_DWMCOMPOSITIONCHANGED:
            applyTheme(m_dark);
            return false;

        default:
            break;
        }

        return false;
#else
        Q_UNUSED(eventType);
        Q_UNUSED(message);
        Q_UNUSED(result);
        return false;
#endif
    }
}
