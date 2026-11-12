#include "Wizard.h"

#include <windows.h>

#include <commctrl.h>
#include <objbase.h>
#include <shobjidl.h>

#include <cwchar>
#include <string>

#include "InstallEngine.h"

#pragma comment(lib, "comctl32.lib")

namespace vth::setup
{
    namespace
    {
        // Layout constants (logical pixels at 96 DPI)
        constexpr int kClientW = 560;
        constexpr int kClientH = 420;
        constexpr int kBarH    = 50;   // bottom button strip
        constexpr int kBrandW  = 180;  // welcome / finish left panel
        constexpr int kHeaderH = 62;   // interior page header band
        constexpr int kPad     = 24;
        constexpr int kBtnW    = 90;
        constexpr int kBtnH    = 26;

        // Colors: system where possible, so it matches the shell
        constexpr COLORREF kWhite      = RGB(0xFF, 0xFF, 0xFF);
        constexpr COLORREF kTextTitle  = RGB(0x00, 0x33, 0x99);  // wizard title
        constexpr COLORREF kTextBody   = RGB(0x00, 0x00, 0x00);
        constexpr COLORREF kTextSubtle = RGB(0x40, 0x40, 0x40);

        // Control ids
        enum {
            IDC_BACK = 101,
            IDC_NEXT,
            IDC_BROWSE,
            IDC_RADIO_ACCEPT,
            IDC_RADIO_DECLINE,
            IDC_CHK_DESKTOP,
            IDC_CHK_LAUNCH,
            IDC_EDIT_DIR,
            IDC_EDIT_LICENSE,
            IDC_PROGRESS,
            IDC_STATUS,
        };

        constexpr UINT WM_APP_PROGRESS = WM_APP + 1;
        constexpr UINT WM_APP_DONE     = WM_APP + 2;

        enum Page {
            PgWelcome = 0,
            PgLicense,
            PgDirectory,
            PgReady,
            PgInstalling,
            PgNotice,
            PgFinish,
        };

        const wchar_t kWindowClass[] = L"VethidaSetupWizard";

        // short version of legal/eula.md, shown in a multiline edit
        const wchar_t* const kEulaText =
            L"Vethida End User License Agreement\r\n"
            L"Effective September 30, 2026\r\n\r\n"
            L"This End User License Agreement (\"EULA\") is a legal agreement "
            L"between you and Rens Engineering for "
            L"the Vethida desktop software, including the installer, updates, "
            L"and accompanying files (the \"Software\"). By installing or using "
            L"the Software, you accept this EULA. If you do not agree, do not "
            L"install or use the Software.\r\n\r\n"
            L"1. License grant\r\n"
            L"Rens Engineering grants you a royalty-free, personal, "
            L"non-exclusive, non-transferable, revocable license to install and "
            L"use the Software at the Free tier on a machine you own or "
            L"control, subject to your compliance with this EULA. This grant "
            L"requires no license key and no payment, and it does "
            L"not expire. It is subject to the Restrictions, the Disclaimer of "
            L"warranties and the Limitation of liability below in exactly the "
            L"way a paid grant is.\r\n\r\n"
            L"Subject to your compliance with this EULA and payment of any "
            L"applicable fees, Rens Engineering grants you a personal, "
            L"non-exclusive, non-transferable, revocable license to install and "
            L"use the Software on a machine you own or control, at the tier you "
            L"have activated.\r\n\r\n"
            L"2. Activation and device limits\r\n"
            L"Paid tiers require online activation with a valid license key. "
            L"A license is for your own use on one PC. It carries a small "
            L"number of activations, so that a reinstall or a new PC does not "
            L"lock you out; deactivating a PC from the Software frees its "
            L"activation. Activation is checked by Lemon Squeezy, and the "
            L"Privacy Policy at vethida.com/legal/privacy lists exactly what "
            L"the Software sends.\r\n\r\n"
            L"3. License checks and reduced mode\r\n"
            L"On Pro and Ultimate the Software checks the license with Lemon "
            L"Squeezy when it starts and once a day, and works offline for up "
            L"to fourteen days from the last successful check. If entitlement "
            L"cannot be confirmed - for example, if a subscription ends, a "
            L"license is disabled, or no check has succeeded for fourteen days "
            L"- the Software returns to the Free tier and continues to operate "
            L"there, leaving any changes already applied in place. This "
            L"behavior is a normal part of the licensing model.\r\n\r\n"
            L"A Free install makes no licensing call, so nothing in this "
            L"section applies to it.\r\n\r\n"
            L"4. System modification and no guarantee\r\n"
            L"The Software modifies Windows configuration only as you direct, "
            L"and is designed so that changes can be reviewed, snapshotted, and "
            L"reverted. Some changes are marked Permanent in the Software and "
            L"cannot be undone by it at all. The Software does not warrant that "
            L"any change is reversible.\r\n\r\n"
            L"Rens Engineering does not guarantee any specific performance "
            L"result and is not responsible for system instability, data loss, "
            L"or hardware issues arising from configuration changes you choose "
            L"to apply. Maintain backups and restore points before applying "
            L"high-risk changes.\r\n\r\n"
            // 5 and 6 are word for word from eula.md
            L"5. Disclaimer of warranties\r\n"
            L"The Software is provided \"as is\" and \"as available\", without "
            L"warranty of any kind, express or implied, including any implied "
            L"warranty of merchantability, fitness for a particular purpose, "
            L"title, or non-infringement, to the maximum extent permitted by "
            L"law.\r\n\r\n"
            L"Rens Engineering does not warrant that the Software is free of "
            L"defects, that any configuration change is appropriate for your "
            L"hardware or your Windows build, that a change will behave "
            L"identically across machines, that a change will produce any "
            L"particular performance result, or that any change can be "
            L"reversed.\r\n\r\n"
            L"6. Limitation of liability\r\n"
            L"To the maximum extent permitted by law, Rens Engineering will not "
            L"be liable for any indirect, incidental, special, consequential, "
            L"exemplary or punitive damages, or for lost profits, lost or "
            L"corrupted data, loss of use, business interruption, or goodwill, "
            L"arising from or relating to the Software.\r\n\r\n"
            L"This exclusion applies expressly to damage to or failure of an "
            L"operating system, an installation of Windows, stored data, "
            L"drivers, firmware, or hardware resulting from configuration "
            L"changes you chose to apply, and to the cost of reinstalling, "
            L"repairing or replacing any of them. It applies regardless of the "
            L"theory of liability and even if Rens Engineering has been advised "
            L"of the possibility of those damages.\r\n\r\n"
            L"To the maximum extent permitted by law, total aggregate liability "
            L"for all claims relating to the Software will not exceed the "
            L"greater of the amount you paid for the Software in the twelve "
            L"months before the claim, or fifty United States dollars. Where "
            L"you paid nothing because you use the Software at the Free tier, "
            L"that cap is fifty United States dollars.\r\n\r\n"
            L"Some jurisdictions do not allow the exclusion of implied "
            L"warranties or the limitation of certain damages, so parts of "
            L"these two sections may not apply to you. Nothing here excludes or "
            L"limits liability that cannot lawfully be excluded, including "
            L"liability for death or personal injury caused by negligence, for "
            L"fraud, or under mandatory consumer-protection law in your country "
            L"of residence.\r\n\r\n"
            L"7. Restrictions\r\n"
            L"You may not: copy, redistribute, resell, rent, or sublicense the "
            L"Software except as expressly permitted; circumvent, disable, or "
            L"tamper with license validation, activation, or integrity checks; "
            L"reverse engineer, decompile, or disassemble the Software, except "
            L"to the extent applicable law expressly permits despite this "
            L"restriction, and except for the third-party components licensed "
            L"to allow it; or use the Software for any unlawful purpose.\r\n\r\n"
            L"8. Updates\r\n"
            L"The Software never installs an update on its own; installation "
            L"always follows an action you take. If you turn on automatic "
            L"downloads - off by default - the installer for an available "
            L"update is fetched in advance so it is ready when you choose to "
            L"install it. Updates are delivered as signed installers whose "
            L"Authenticode signature, publisher, and SHA-256 are verified "
            L"before installation. This EULA governs updates unless an update "
            L"is accompanied by different terms.\r\n\r\n"
            L"9. Third-party components and Qt (LGPLv3)\r\n"
            L"The Software includes third-party open-source components. The "
            L"notices and the full license texts ship inside the Software: open "
            L"\"Open-source licenses\" in the app to read them, and \"Save "
            L"license texts\" to write them to a folder of your choice, with no "
            L"internet connection required. The same notices are published at "
            L"https://vethida.com/legal/third-party. In particular, the Software "
            L"uses the "
            L"Qt framework under the GNU Lesser General Public License v3 "
            L"(LGPLv3) via dynamic linking. Nothing in this EULA restricts your "
            L"rights under LGPLv3 with respect to those Qt components.\r\n\r\n"
            L"10. Ownership\r\n"
            L"The Software is licensed, not sold. Rens Engineering and its "
            L"licensors retain all rights, title, and interest in the Software "
            L"except for the open-source components under their own "
            L"licenses.\r\n\r\n"
            L"11. Termination\r\n"
            L"This EULA terminates automatically if you breach it. On "
            L"termination you must stop using and uninstall the Software.\r\n\r\n"
            L"12. Contact\r\n"
            L"Rens Engineering, Pennsylvania, United States. "
            L"legal@vethida.com\r\n\r\n"
            L"The complete End User License Agreement, including sections not "
            L"reproduced above, is published at https://vethida.com/legal/eula "
            L"and governs your use of the Software.\r\n";

        std::wstring baseName(const std::wstring& path)
        {
            const size_t slash = path.find_last_of(L"\\/");
            return slash == std::wstring::npos ? path : path.substr(slash + 1);
        }

        std::wstring trim(const std::wstring& s)
        {
            const size_t a = s.find_first_not_of(L" \t\r\n");
            if (a == std::wstring::npos) return std::wstring();
            const size_t b = s.find_last_not_of(L" \t\r\n");
            return s.substr(a, b - a + 1);
        }

        COLORREF sysFace()   { return GetSysColor(COLOR_3DFACE); }
        COLORREF sysShadow() { return GetSysColor(COLOR_3DSHADOW); }
        COLORREF sysHilite() { return GetSysColor(COLOR_3DHIGHLIGHT); }
    }

    class Wizard
    {
    public:
        Wizard(HINSTANCE instance, const WizardParams& params)
            : instance_(instance), params_(params)
        {
            InitializeCriticalSection(&lock_);
        }

        ~Wizard()
        {
            destroyFonts();
            if (iconLogo_)  DestroyIcon(iconLogo_);
            if (iconGlyph_) DestroyIcon(iconGlyph_);
            if (whiteBrush_) DeleteObject(whiteBrush_);
            DeleteCriticalSection(&lock_);
        }

        WizardOutcome run()
        {
            WNDCLASSEXW wc{};
            wc.cbSize        = sizeof(wc);
            wc.lpfnWndProc   = &Wizard::wndProc;
            wc.hInstance     = instance_;
            wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
            wc.hbrBackground = nullptr;  // painted in WM_PAINT, no erase flicker
            wc.lpszClassName = kWindowClass;
            wc.hIcon = static_cast<HICON>(LoadImageW(
                instance_, L"IDI_ICON1", IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));
            RegisterClassExW(&wc);

            const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU
                                | WS_MINIMIZEBOX;
            hwnd_ = CreateWindowExW(0, kWindowClass, L"Vethida Setup", style,
                                    CW_USEDEFAULT, CW_USEDEFAULT, 400, 300,
                                    nullptr, nullptr, instance_, this);
            if (!hwnd_) return outcome_;

            // has to happen after CreateWindow returns, sizing in WM_CREATE gets overridden
            dpi_ = GetDpiForWindow(hwnd_);
            rebuildDpiResources();
            sizeAndCenter();
            layoutControls();
            applyFonts();
            showPage(PgWelcome);

            ShowWindow(hwnd_, SW_SHOW);
            UpdateWindow(hwnd_);

            // dpi can change once it's actually shown on a monitor
            const UINT shownDpi = GetDpiForWindow(hwnd_);
            if (shownDpi != 0 && shownDpi != dpi_) {
                dpi_ = shownDpi;
                rebuildDpiResources();
                sizeAndCenter();
                layoutControls();
                applyFonts();
                InvalidateRect(hwnd_, nullptr, TRUE);
            }

            MSG msg;
            while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
                if (!IsDialogMessageW(hwnd_, &msg)) {
                    TranslateMessage(&msg);
                    DispatchMessageW(&msg);
                }
            }
            return outcome_;
        }

    private:
        // DPI helpers
        int S(int v) const { return MulDiv(v, static_cast<int>(dpi_), 96); }

        static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam,
                                        LPARAM lParam)
        {
            if (msg == WM_NCCREATE) {
                auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
                SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                                  reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
            }
            auto* self = reinterpret_cast<Wizard*>(
                GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (!self) return DefWindowProcW(hwnd, msg, wParam, lParam);
            return self->handle(hwnd, msg, wParam, lParam);
        }

        LRESULT handle(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
        {
            hwnd_ = hwnd; // not set yet during WM_CREATE

            switch (msg) {
            case WM_CREATE:
                onCreate();
                return 0;
            case WM_ERASEBKGND:
                return 1;  // fully painted in WM_PAINT
            case WM_PAINT:
                onPaint();
                return 0;
            case WM_COMMAND:
                onCommand(LOWORD(wParam));
                return 0;
            case WM_CTLCOLORSTATIC:
            case WM_CTLCOLOREDIT: {
                auto hdc = reinterpret_cast<HDC>(wParam);
                SetBkColor(hdc, kWhite);
                SetTextColor(hdc, kTextBody);
                return reinterpret_cast<LRESULT>(whiteBrush_);
            }
            case WM_APP_PROGRESS:
                onProgress();
                return 0;
            case WM_APP_DONE:
                onDone();
                return 0;
            case WM_DPICHANGED:
                onDpiChanged(HIWORD(wParam), reinterpret_cast<RECT*>(lParam));
                return 0;
            case WM_CLOSE:
                onCancelRequest();
                return 0;
            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;
            default:
                break;
            }
            return DefWindowProcW(hwnd, msg, wParam, lParam);
        }

        // Construction
        void onCreate()
        {
            whiteBrush_ = CreateSolidBrush(kWhite);
            createControls();
        }

        void createControls()
        {
            auto make = [&](const wchar_t* cls, const wchar_t* text, DWORD extra,
                            int id) -> HWND {
                return CreateWindowExW(
                    0, cls, text, WS_CHILD | extra, 0, 0, 10, 10, hwnd_,
                    reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                    instance_, nullptr);
            };

            // only the nav buttons start visible, showPage() handles the rest
            btnBack_ = make(L"BUTTON", L"< Back",
                            WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, IDC_BACK);
            btnNext_ = make(L"BUTTON", L"Next >",
                            WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                            IDC_NEXT);
            btnCancel_ = make(L"BUTTON", L"Cancel",
                              WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, IDCANCEL);

            edLicense_ = make(L"EDIT", L"",
                              WS_BORDER | WS_VSCROLL | WS_TABSTOP | ES_MULTILINE
                                  | ES_READONLY,
                              IDC_EDIT_LICENSE);
            SetWindowTextW(edLicense_, kEulaText);
            rbAccept_ = make(L"BUTTON", L"I accept the agreement",
                             WS_TABSTOP | WS_GROUP | BS_AUTORADIOBUTTON,
                             IDC_RADIO_ACCEPT);
            rbDecline_ = make(L"BUTTON", L"I do not accept the agreement",
                              WS_TABSTOP | BS_AUTORADIOBUTTON,
                              IDC_RADIO_DECLINE);

            edDir_ = make(L"EDIT", params_.defaultInstallDir.c_str(),
                          WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL,
                          IDC_EDIT_DIR);
            btnBrowse_ = make(L"BUTTON", L"Browse...",
                              WS_TABSTOP | BS_PUSHBUTTON, IDC_BROWSE);
            cbDesktop_ = make(L"BUTTON", L"Create a Desktop shortcut",
                              WS_TABSTOP | BS_AUTOCHECKBOX, IDC_CHK_DESKTOP);
            if (params_.desktopDefault)
                SendMessageW(cbDesktop_, BM_SETCHECK, BST_CHECKED, 0);

            progress_ = make(PROGRESS_CLASSW, L"", 0, IDC_PROGRESS);
            stStatus_ = make(L"STATIC", L"", SS_LEFTNOWORDWRAP | SS_NOPREFIX,
                             IDC_STATUS);

            cbLaunch_ = make(L"BUTTON", L"Launch Vethida now",
                             WS_TABSTOP | BS_AUTOCHECKBOX, IDC_CHK_LAUNCH);
            SendMessageW(cbLaunch_, BM_SETCHECK, BST_CHECKED, 0);
        }

        // DPI-dependent resources
        void rebuildDpiResources()
        {
            destroyFonts();

            // system message font, scaled for the titles
            LOGFONTW base = messageFont();
            fBody_    = CreateFontIndirectW(&base);
            fTitle_   = scaledFont(base, 1.22, FW_BOLD);   // header title
            fHeading_ = scaledFont(base, 1.45, FW_BOLD);   // welcome/finish
            fBrand_   = scaledFont(base, 1.85, FW_BOLD);   // wordmark
            fVersion_ = CreateFontIndirectW(&base);

            if (iconLogo_)  DestroyIcon(iconLogo_);
            if (iconGlyph_) DestroyIcon(iconGlyph_);
            iconLogo_ = static_cast<HICON>(LoadImageW(
                instance_, L"IDI_ICON1", IMAGE_ICON, S(48), S(48),
                LR_DEFAULTCOLOR));
            iconGlyph_ = static_cast<HICON>(LoadImageW(
                instance_, L"IDI_ICON1", IMAGE_ICON, S(32), S(32),
                LR_DEFAULTCOLOR));
        }

        LOGFONTW messageFont() const
        {
            NONCLIENTMETRICSW ncm{};
            ncm.cbSize = sizeof(ncm);
            if (SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS,
                                           sizeof(ncm), &ncm, 0, dpi_))
                return ncm.lfMessageFont;

            LOGFONTW lf{};
            lf.lfHeight  = -MulDiv(9, static_cast<int>(dpi_), 72);
            lf.lfWeight  = FW_NORMAL;
            lf.lfQuality = CLEARTYPE_QUALITY;
            wcscpy_s(lf.lfFaceName, L"Segoe UI");
            return lf;
        }

        HFONT scaledFont(LOGFONTW base, double scale, int weight) const
        {
            base.lfHeight = static_cast<LONG>(base.lfHeight * scale);
            base.lfWeight = weight;
            return CreateFontIndirectW(&base);
        }

        void destroyFonts()
        {
            for (HFONT* f : { &fBody_, &fTitle_, &fHeading_, &fBrand_,
                              &fVersion_ }) {
                if (*f) { DeleteObject(*f); *f = nullptr; }
            }
        }

        void applyFonts()
        {
            for (HWND h : { btnBack_, btnNext_, btnCancel_, rbAccept_,
                            rbDecline_, edDir_, btnBrowse_, cbDesktop_,
                            stStatus_, cbLaunch_ })
                SendMessageW(h, WM_SETFONT,
                             reinterpret_cast<WPARAM>(fBody_), TRUE);
            SendMessageW(edLicense_, WM_SETFONT,
                         reinterpret_cast<WPARAM>(fBody_), TRUE);
        }

        // Sizing / layout
        void sizeAndCenter()
        {
            RECT rc{ 0, 0, S(kClientW), S(kClientH) };
            const DWORD style = static_cast<DWORD>(
                GetWindowLongPtrW(hwnd_, GWL_STYLE));
            AdjustWindowRectExForDpi(&rc, style, FALSE, 0, dpi_);
            const int w = rc.right - rc.left;
            const int h = rc.bottom - rc.top;

            HMONITOR mon = MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST);
            MONITORINFO mi{ sizeof(mi) };
            GetMonitorInfoW(mon, &mi);
            const int x = mi.rcWork.left
                          + (mi.rcWork.right - mi.rcWork.left - w) / 2;
            const int y = mi.rcWork.top
                          + (mi.rcWork.bottom - mi.rcWork.top - h) / 2;
            SetWindowPos(hwnd_, nullptr, x, y, w, h, SWP_NOZORDER);
        }

        void layoutControls()
        {
            const int cw = S(kClientW);
            const int barTop = S(kClientH) - S(kBarH);

            // Bottom buttons, right-aligned: [Back][Next]   [Cancel]
            const int by = barTop + (S(kBarH) - S(kBtnH)) / 2;
            const int cancelX = cw - S(14) - S(kBtnW);
            const int nextX   = cancelX - S(16) - S(kBtnW);
            const int backX   = nextX - S(6) - S(kBtnW);
            MoveWindow(btnBack_,   backX,   by, S(kBtnW), S(kBtnH), TRUE);
            MoveWindow(btnNext_,   nextX,   by, S(kBtnW), S(kBtnH), TRUE);
            MoveWindow(btnCancel_, cancelX, by, S(kBtnW), S(kBtnH), TRUE);

            // License page.
            const int lx = S(kPad);
            const int lw = cw - 2 * S(kPad);
            const int rbH = S(20);
            const int rbDeclY = barTop - S(12) - rbH;
            const int rbAccY  = rbDeclY - rbH;
            const int edTop = S(kHeaderH) + S(14);
            const int edBottom = rbAccY - S(34);  // leaves room for the prompt
            MoveWindow(edLicense_, lx, edTop, lw, edBottom - edTop, TRUE);
            MoveWindow(rbAccept_,  lx + S(2), rbAccY,  lw - S(4), rbH, TRUE);
            MoveWindow(rbDecline_, lx + S(2), rbDeclY, lw - S(4), rbH, TRUE);

            // Directory page.
            const int rowY = S(kHeaderH) + S(96);
            const int browseW = S(94);
            const int dirEditW = lw - browseW - S(8);
            MoveWindow(edDir_, lx, rowY, dirEditW, S(24), TRUE);
            MoveWindow(btnBrowse_, lx + dirEditW + S(8), rowY - S(1),
                       browseW, S(26), TRUE);
            MoveWindow(cbDesktop_, lx, rowY + S(40), lw, S(20), TRUE);

            // Installing page.
            const int stY = S(kHeaderH) + S(40);
            MoveWindow(stStatus_, lx, stY, lw, S(20), TRUE);
            MoveWindow(progress_, lx, stY + S(26), lw, S(16), TRUE);

            // Finish page (right content column, beside the brand panel).
            const int fx = S(kBrandW) + S(28);
            const int fw = cw - fx - S(24);
            MoveWindow(cbLaunch_, fx, barTop - S(52), fw, S(20), TRUE);
        }

        // Painting
        void onPaint()
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd_, &ps);

            RECT client;
            GetClientRect(hwnd_, &client);
            FillRect(hdc, &client, whiteBrush_);

            const int cw = S(kClientW);
            const int barTop = S(kClientH) - S(kBarH);

            RECT bar{ 0, barTop, cw, S(kClientH) };
            HBRUSH barBrush = CreateSolidBrush(sysFace());
            FillRect(hdc, &bar, barBrush);
            DeleteObject(barBrush);
            drawEtchedH(hdc, 0, cw, barTop);

            SetBkMode(hdc, TRANSPARENT);

            if (page_ == PgWelcome || page_ == PgFinish)
                paintBrandPage(hdc);
            else
                paintInteriorPage(hdc);

            EndPaint(hwnd_, &ps);
        }

        void paintBrandPage(HDC hdc)
        {
            const int barTop = S(kClientH) - S(kBarH);
            paintBrandPanel(hdc, barTop);

            const int x = S(kBrandW) + S(28);
            const int w = S(kClientW) - x - S(24);
            int y = S(40);

            const std::wstring heading = page_ == PgWelcome
                ? welcomeHeading()
                : L"Completing the Vethida Setup Wizard";
            RECT hr{ x, y, x + w, y + S(80) };
            drawText(hdc, hr, heading, fHeading_, kTextTitle,
                     DT_WORDBREAK | DT_TOP);
            y += measure(hdc, heading, fHeading_, w) + S(16);

            const std::wstring body = page_ == PgWelcome
                ? welcomeBody()
                : finishBody();
            RECT br{ x, y, x + w, barTop - S(16) };
            drawText(hdc, br, body, fBody_, kTextBody,
                     DT_WORDBREAK | DT_TOP);
        }

        void paintInteriorPage(HDC hdc)
        {
            const int cw = S(kClientW);

            // Header band: title + subtitle on the left, brand glyph at right.
            std::wstring title, subtitle;
            interiorHeader(title, subtitle);
            RECT tr{ S(kPad), S(12), cw - S(kPad) - S(44), S(38) };
            drawText(hdc, tr, title, fTitle_, kTextTitle,
                     DT_SINGLELINE | DT_TOP);
            RECT sr{ S(kPad) + S(12), S(38), cw - S(kPad) - S(44), S(60) };
            drawText(hdc, sr, subtitle, fBody_, kTextSubtle,
                     DT_SINGLELINE | DT_TOP);
            if (iconGlyph_)
                DrawIconEx(hdc, cw - S(kPad) - S(32), S(15), iconGlyph_,
                           S(32), S(32), 0, nullptr, DI_NORMAL);
            drawEtchedH(hdc, 0, cw, S(kHeaderH));

            const int bx = S(kPad);
            const int bw = cw - 2 * S(kPad);

            switch (page_) {
            case PgLicense: {
                RECT pr{ bx + S(2), rbPromptY() - S(2), cw - S(kPad),
                         rbPromptY() + S(20) };
                drawText(hdc, pr, L"Do you accept the terms of the License "
                                  L"Agreement?", fBody_, kTextBody,
                         DT_SINGLELINE | DT_TOP);
                break;
            }
            case PgDirectory: {
                RECT br{ bx, S(kHeaderH) + S(14), bx + bw,
                         S(kHeaderH) + S(76) };
                drawText(hdc, br,
                         L"Setup will install Vethida into the folder below. "
                         L"To install into a different folder, click Browse "
                         L"and choose another folder.", fBody_, kTextBody,
                         DT_WORDBREAK | DT_TOP);
                RECT lr{ bx, S(kHeaderH) + S(76), bx + bw,
                         S(kHeaderH) + S(94) };
                drawText(hdc, lr, L"Installation folder:", fBody_, kTextSubtle,
                         DT_SINGLELINE | DT_TOP);
                break;
            }
            case PgReady: {
                RECT br{ bx, S(kHeaderH) + S(14), bx + bw, S(kClientH) - S(kBarH)
                             - S(16) };
                drawText(hdc, br, readyBody(), fBody_, kTextBody,
                         DT_WORDBREAK | DT_TOP);
                break;
            }
            case PgNotice: {
                RECT br{ bx, S(kHeaderH) + S(16), bx + bw, S(kClientH) - S(kBarH)
                             - S(16) };
                drawText(hdc, br, noticeBody(), fBody_, kTextBody,
                         DT_WORDBREAK | DT_TOP);
                break;
            }
            default:
                break;
            }
        }

        int rbPromptY() const
        {
            const int barTop = S(kClientH) - S(kBarH);
            const int rbH = S(20);
            return barTop - S(12) - rbH - rbH - S(24);
        }

        void paintBrandPanel(HDC hdc, int height)
        {
            const int w = S(kBrandW);

            RECT panel{ 0, 0, w, height };
            HBRUSH panelBrush = CreateSolidBrush(sysFace());
            FillRect(hdc, &panel, panelBrush);
            DeleteObject(panelBrush);
            drawEtchedV(hdc, w, 0, height);

            const int icon = S(48);
            const int iconX = (w - icon) / 2;
            const int iconY = S(56);
            if (iconLogo_)
                DrawIconEx(hdc, iconX, iconY, iconLogo_, icon, icon, 0,
                           nullptr, DI_NORMAL);

            SetBkMode(hdc, TRANSPARENT);
            RECT wr{ 0, iconY + icon + S(18), w, iconY + icon + S(50) };
            drawText(hdc, wr, L"Vethida", fBrand_, kTextTitle,
                     DT_SINGLELINE | DT_CENTER | DT_TOP);
            RECT vr{ 0, iconY + icon + S(48), w, iconY + icon + S(70) };
            drawText(hdc, vr, L"Version " + params_.version, fVersion_,
                     kTextSubtle, DT_SINGLELINE | DT_CENTER | DT_TOP);
        }

        // etched separators
        void drawEtchedH(HDC hdc, int x1, int x2, int y)
        {
            RECT s{ x1, y, x2, y + 1 };
            HBRUSH sh = CreateSolidBrush(sysShadow());
            FillRect(hdc, &s, sh);
            DeleteObject(sh);
            RECT h{ x1, y + 1, x2, y + 2 };
            HBRUSH hl = CreateSolidBrush(sysHilite());
            FillRect(hdc, &h, hl);
            DeleteObject(hl);
        }

        void drawEtchedV(HDC hdc, int x, int y1, int y2)
        {
            RECT s{ x, y1, x + 1, y2 };
            HBRUSH sh = CreateSolidBrush(sysShadow());
            FillRect(hdc, &s, sh);
            DeleteObject(sh);
            RECT h{ x + 1, y1, x + 2, y2 };
            HBRUSH hl = CreateSolidBrush(sysHilite());
            FillRect(hdc, &h, hl);
            DeleteObject(hl);
        }

        void drawText(HDC hdc, RECT r, const std::wstring& text, HFONT font,
                      COLORREF color, UINT format)
        {
            HFONT old = static_cast<HFONT>(SelectObject(hdc, font));
            SetTextColor(hdc, color);
            DrawTextW(hdc, text.c_str(), -1, &r, format);
            SelectObject(hdc, old);
        }

        int measure(HDC hdc, const std::wstring& text, HFONT font, int width)
        {
            HFONT old = static_cast<HFONT>(SelectObject(hdc, font));
            RECT r{ 0, 0, width, 0 };
            DrawTextW(hdc, text.c_str(), -1, &r,
                      DT_WORDBREAK | DT_CALCRECT);
            SelectObject(hdc, old);
            return r.bottom - r.top;
        }

        // Page navigation
        void showPage(int page)
        {
            page_ = page;

            const bool lic = page == PgLicense;
            const bool dir = page == PgDirectory;
            const bool ins = page == PgInstalling;
            const bool fin = page == PgFinish;
            ShowWindow(edLicense_, lic ? SW_SHOW : SW_HIDE);
            ShowWindow(rbAccept_,  lic ? SW_SHOW : SW_HIDE);
            ShowWindow(rbDecline_, lic ? SW_SHOW : SW_HIDE);
            ShowWindow(edDir_,     dir ? SW_SHOW : SW_HIDE);
            ShowWindow(btnBrowse_, dir ? SW_SHOW : SW_HIDE);
            ShowWindow(cbDesktop_, dir ? SW_SHOW : SW_HIDE);
            ShowWindow(progress_,  ins ? SW_SHOW : SW_HIDE);
            ShowWindow(stStatus_,  ins ? SW_SHOW : SW_HIDE);
            ShowWindow(cbLaunch_,  fin ? SW_SHOW : SW_HIDE);

            updateButtons();
            InvalidateRect(hwnd_, nullptr, TRUE);

            if (lic) SetFocus(rbAccept_);
            else if (dir) SetFocus(edDir_);
            else SetFocus(btnNext_);
        }

        void updateButtons()
        {
            bool backEnabled = false, nextEnabled = true, cancelEnabled = true;
            const wchar_t* nextText = L"Next >";

            switch (page_) {
            case PgWelcome:
                backEnabled = false;
                break;
            case PgLicense:
                backEnabled = true;
                nextEnabled = SendMessageW(rbAccept_, BM_GETCHECK, 0, 0)
                              == BST_CHECKED;
                break;
            case PgDirectory:
                backEnabled = true;
                break;
            case PgReady:
                backEnabled = true;
                nextText = L"Install";
                break;
            case PgInstalling:
                backEnabled = false;
                nextEnabled = false;
                cancelEnabled = false;
                nextText = L"Install";
                break;
            case PgNotice:
                backEnabled = false;
                cancelEnabled = false;
                break;
            case PgFinish:
                backEnabled = false;
                cancelEnabled = false;
                nextText = L"Finish";
                break;
            default:
                break;
            }

            SetWindowTextW(btnNext_, nextText);
            EnableWindow(btnBack_, backEnabled);
            EnableWindow(btnNext_, nextEnabled);
            EnableWindow(btnCancel_, cancelEnabled);
        }

        void onCommand(int id)
        {
            switch (id) {
            case IDC_NEXT:
                if (IsWindowEnabled(btnNext_)) onNext();
                break;
            case IDC_BACK:
                if (IsWindowEnabled(btnBack_)) onBack();
                break;
            case IDCANCEL:
                onCancelRequest();
                break;
            case IDC_BROWSE:
                onBrowse();
                break;
            case IDC_RADIO_ACCEPT:
            case IDC_RADIO_DECLINE:
                updateButtons();
                break;
            default:
                break;
            }
        }

        void onNext()
        {
            switch (page_) {
            case PgWelcome:
                showPage(PgLicense);
                break;
            case PgLicense:
                showPage(PgDirectory);
                break;
            case PgDirectory: {
                const std::wstring dir = trim(readText(edDir_));
                if (dir.empty()) {
                    MessageBoxW(hwnd_, L"Please choose an installation folder.",
                                L"Vethida Setup", MB_OK | MB_ICONWARNING);
                    return;
                }
                installDir_ = dir;
                desktopShortcut_ =
                    SendMessageW(cbDesktop_, BM_GETCHECK, 0, 0) == BST_CHECKED;
                showPage(PgReady);
                break;
            }
            case PgReady:
                showPage(PgInstalling);
                startInstall();
                break;
            case PgNotice:
                showPage(PgFinish);
                break;
            case PgFinish:
                finish(true);
                break;
            default:
                break;
            }
        }

        void onBack()
        {
            switch (page_) {
            case PgLicense:   showPage(PgWelcome);   break;
            case PgDirectory: showPage(PgLicense);   break;
            case PgReady:     showPage(PgDirectory); break;
            default: break;
            }
        }

        void onCancelRequest()
        {
            if (page_ >= PgInstalling) {
                if (page_ == PgInstalling) return; // can't cancel mid-install
                if (page_ == PgFinish || page_ == PgNotice) { finish(true); return; }
            }
            TASKDIALOGCONFIG cfg{};
            cfg.cbSize = sizeof(cfg);
            cfg.hwndParent = hwnd_;
            cfg.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION;
            cfg.pszWindowTitle = L"Vethida Setup";
            cfg.pszMainIcon = TD_WARNING_ICON;
            cfg.pszMainInstruction = L"Exit Setup?";
            cfg.pszContent =
                L"Vethida is not yet installed. Are you sure you want to quit?";
            TASKDIALOG_BUTTON btns[2] = {
                { IDYES, L"Exit Setup" },
                { IDNO, L"Continue" },
            };
            cfg.pButtons = btns;
            cfg.cButtons = 2;
            cfg.nDefaultButton = IDNO;
            int pressed = 0;
            if (SUCCEEDED(TaskDialogIndirect(&cfg, &pressed, nullptr, nullptr))
                && pressed == IDYES) {
                finish(false);
            }
        }

        void finish(bool installed)
        {
            outcome_.installed = installed && installDone_;
            outcome_.launch = outcome_.installed
                && SendMessageW(cbLaunch_, BM_GETCHECK, 0, 0) == BST_CHECKED;
            outcome_.wasRunning = wasRunning_;
            outcome_.exePath = exePath_;
            DestroyWindow(hwnd_);
        }

        void onBrowse()
        {
            IFileOpenDialog* dlg = nullptr;
            if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr,
                                        CLSCTX_INPROC_SERVER,
                                        IID_PPV_ARGS(&dlg))))
                return;
            DWORD opts = 0;
            dlg->GetOptions(&opts);
            dlg->SetOptions(opts | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM
                            | FOS_PATHMUSTEXIST);
            dlg->SetTitle(L"Choose the installation folder");
            if (SUCCEEDED(dlg->Show(hwnd_))) {
                IShellItem* item = nullptr;
                if (SUCCEEDED(dlg->GetResult(&item))) {
                    PWSTR path = nullptr;
                    if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH,
                                                       &path))) {
                        SetWindowTextW(edDir_, path);
                        CoTaskMemFree(path);
                    }
                    item->Release();
                }
            }
            dlg->Release();
        }

        std::wstring readText(HWND edit) const
        {
            const int len = GetWindowTextLengthW(edit);
            std::wstring s(static_cast<size_t>(len), L'\0');
            if (len > 0)
                GetWindowTextW(edit, s.data(), len + 1);
            return s;
        }

        // Install worker
        void startInstall()
        {
            SetWindowTextW(stStatus_, L"Preparing files...");
            SendMessageW(progress_, PBM_SETRANGE32, 0, 1);
            SendMessageW(progress_, PBM_SETPOS, 0, 0);
            startMarquee();

            HANDLE thread = CreateThread(nullptr, 0, &Wizard::threadEntry, this,
                                         0, nullptr);
            if (thread) CloseHandle(thread);
        }

        static DWORD WINAPI threadEntry(LPVOID param)
        {
            auto* self = static_cast<Wizard*>(param);
            InstallRequest req;
            req.installDir = self->installDir_;
            req.desktopShortcut = self->desktopShortcut_;

            InstallOutcome result = performInstall(
                req, [self](int done, int total, const std::wstring& name) {
                    self->postProgress(done, total, name);
                });

            EnterCriticalSection(&self->lock_);
            self->workerResult_ = result;
            LeaveCriticalSection(&self->lock_);
            PostMessageW(self->hwnd_, WM_APP_DONE, 0, 0);
            return 0;
        }

        void postProgress(int done, int total, const std::wstring& name)
        {
            EnterCriticalSection(&lock_);
            progDone_ = done;
            progTotal_ = total;
            progName_ = name;
            LeaveCriticalSection(&lock_);
            PostMessageW(hwnd_, WM_APP_PROGRESS, 0, 0);
        }

        void onProgress()
        {
            int done, total;
            std::wstring name;
            EnterCriticalSection(&lock_);
            done = progDone_;
            total = progTotal_;
            name = progName_;
            LeaveCriticalSection(&lock_);

            if (total > 0) {
                if (marquee_) stopMarquee(total);
                SendMessageW(progress_, PBM_SETPOS, done, 0);
            }
            std::wstring status = name.empty()
                ? std::wstring(L"Finishing installation...")
                : (L"Extracting " + baseName(name));
            SetWindowTextW(stStatus_, status.c_str());
        }

        void onDone()
        {
            InstallOutcome result;
            EnterCriticalSection(&lock_);
            result = workerResult_;
            LeaveCriticalSection(&lock_);

            if (result.ok) {
                installDone_ = true;
                exePath_ = result.exePath;
                wasRunning_ = result.wasRunning;
                SendMessageW(progress_, PBM_SETPOS, 1, 0);
                showPage(PgNotice);
            } else {
                stopMarquee(1);
                MessageBoxW(hwnd_,
                            result.error.empty()
                                ? L"Setup could not complete the installation."
                                : result.error.c_str(),
                            L"Vethida Setup", MB_OK | MB_ICONERROR);
                showPage(PgReady);
            }
        }

        void startMarquee()
        {
            const LONG_PTR style = GetWindowLongPtrW(progress_, GWL_STYLE);
            SetWindowLongPtrW(progress_, GWL_STYLE, style | PBS_MARQUEE);
            SendMessageW(progress_, PBM_SETMARQUEE, TRUE, 30);
            marquee_ = true;
        }

        void stopMarquee(int total)
        {
            if (!marquee_) return;
            SendMessageW(progress_, PBM_SETMARQUEE, FALSE, 0);
            const LONG_PTR style = GetWindowLongPtrW(progress_, GWL_STYLE);
            SetWindowLongPtrW(progress_, GWL_STYLE, style & ~PBS_MARQUEE);
            SendMessageW(progress_, PBM_SETRANGE32, 0, total);
            marquee_ = false;
        }

        // DPI change
        void onDpiChanged(UINT newDpi, const RECT* suggested)
        {
            dpi_ = newDpi;
            rebuildDpiResources();
            if (suggested)
                SetWindowPos(hwnd_, nullptr, suggested->left, suggested->top,
                             suggested->right - suggested->left,
                             suggested->bottom - suggested->top,
                             SWP_NOZORDER | SWP_NOACTIVATE);
            layoutControls();
            applyFonts();
            InvalidateRect(hwnd_, nullptr, TRUE);
        }

        // Page copy
        std::wstring welcomeHeading() const
        {
            return params_.upgrade
                ? L"Welcome to the Vethida Update Wizard"
                : L"Welcome to the Vethida Setup Wizard";
        }

        std::wstring welcomeBody() const
        {
            if (params_.upgrade)
                return L"This wizard will update Vethida to version "
                       + params_.version + L" on your computer.\r\n\r\n"
                       L"Your existing settings and license are kept. It is "
                       L"recommended that you close Vethida before "
                       L"continuing.\r\n\r\n"
                       L"Click Next to continue, or Cancel to exit Setup.";
            return L"This wizard will install Vethida " + params_.version
                   + L" on your computer.\r\n\r\n"
                   L"Vethida is a Windows optimization suite from Rens "
                   L"Engineering. It is recommended that you close all other "
                   L"applications before continuing.\r\n\r\n"
                   L"Click Next to continue, or Cancel to exit Setup.";
        }

        std::wstring finishBody() const
        {
            return params_.upgrade
                ? L"Setup has finished updating Vethida on your computer.\r\n\r\n"
                  L"Click Finish to exit Setup."
                : L"Setup has finished installing Vethida on your "
                  L"computer.\r\n\r\nClick Finish to exit Setup.";
        }

        std::wstring readyBody() const
        {
            std::wstring shortcuts = L"Start menu";
            if (SendMessageW(cbDesktop_, BM_GETCHECK, 0, 0) == BST_CHECKED)
                shortcuts += L", Desktop";
            const wchar_t* verb =
                params_.upgrade ? L"updating" : L"installing";
            return std::wstring(L"Setup is now ready to begin ") + verb
                   + L" Vethida on your computer.\r\n\r\n"
                   L"Click Install to continue, or click Back to review or "
                   L"change any settings.\r\n\r\n"
                   L"Destination folder:\r\n    " + installDir_
                   + L"\r\n\r\nShortcuts:\r\n    " + shortcuts;
        }

        std::wstring noticeBody() const
        {
            return L"Vethida changes Windows settings only when you choose to.\r\n\r\n"
                   L"Before applying high-risk changes, we recommend creating a "
                   L"System Restore point - Vethida can create one for you from "
                   L"the System Restore screen.\r\n\r\n"
                   L"Rens Engineering does not guarantee any specific "
                   L"performance result and is not responsible for issues "
                   L"arising from changes you choose to apply.";
        }

        void interiorHeader(std::wstring& title, std::wstring& subtitle) const
        {
            switch (page_) {
            case PgLicense:
                title = L"License Agreement";
                subtitle = L"Please read the following agreement carefully.";
                break;
            case PgDirectory:
                title = L"Installation Folder";
                subtitle = L"Choose where Vethida will be installed.";
                break;
            case PgReady:
                title = L"Ready to Install";
                subtitle = L"Setup is ready to begin installing Vethida.";
                break;
            case PgInstalling:
                title = L"Installing";
                subtitle = L"Please wait while Setup installs Vethida.";
                break;
            case PgNotice:
                title = L"Before you begin";
                subtitle = L"A note about system changes.";
                break;
            default:
                title.clear();
                subtitle.clear();
                break;
            }
        }

        // State
        HINSTANCE instance_;
        WizardParams params_;
        WizardOutcome outcome_;
        HWND hwnd_ = nullptr;
        UINT dpi_ = 96;
        int page_ = PgWelcome;

        HFONT fBody_ = nullptr, fTitle_ = nullptr, fHeading_ = nullptr;
        HFONT fBrand_ = nullptr, fVersion_ = nullptr;
        HICON iconLogo_ = nullptr, iconGlyph_ = nullptr;
        HBRUSH whiteBrush_ = nullptr;

        HWND btnBack_ = nullptr, btnNext_ = nullptr, btnCancel_ = nullptr;
        HWND edLicense_ = nullptr, rbAccept_ = nullptr, rbDecline_ = nullptr;
        HWND edDir_ = nullptr, btnBrowse_ = nullptr, cbDesktop_ = nullptr;
        HWND progress_ = nullptr, stStatus_ = nullptr, cbLaunch_ = nullptr;

        std::wstring installDir_;
        bool desktopShortcut_ = false;
        bool installDone_ = false;
        bool wasRunning_ = false;
        std::wstring exePath_;
        bool marquee_ = false;

        CRITICAL_SECTION lock_;
        int progDone_ = 0, progTotal_ = 0;
        std::wstring progName_;
        InstallOutcome workerResult_;
    };

    WizardOutcome runWizard(HINSTANCE instance, const WizardParams& params)
    {
        Wizard wizard(instance, params);
        return wizard.run();
    }
}
