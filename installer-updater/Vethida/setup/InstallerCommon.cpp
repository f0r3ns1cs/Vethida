#include "InstallerCommon.h"

#include <knownfolders.h>
#include <objbase.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <tlhelp32.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "version.lib")
#pragma comment(lib, "user32.lib")

namespace vth::setup
{
    std::wstring modulePath()
    {
        wchar_t buf[MAX_PATH];
        const DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
        return std::wstring(buf, n);
    }

    std::wstring moduleDirectory()
    {
        std::wstring p = modulePath();
        const size_t slash = p.find_last_of(L'\\');
        return slash == std::wstring::npos ? p : p.substr(0, slash);
    }

    std::wstring moduleFileVersion()
    {
        const std::wstring path = modulePath();
        const DWORD size = GetFileVersionInfoSizeW(path.c_str(), nullptr);
        if (size == 0) return L"0.0.0";
        std::vector<unsigned char> data(size);
        if (!GetFileVersionInfoW(path.c_str(), 0, size, data.data()))
            return L"0.0.0";
        VS_FIXEDFILEINFO* info = nullptr;
        UINT len = 0;
        if (!VerQueryValueW(data.data(), L"\\",
                            reinterpret_cast<void**>(&info), &len) || !info)
            return L"0.0.0";
        wchar_t out[64];
        swprintf_s(out, L"%u.%u.%u",
                   HIWORD(info->dwFileVersionMS), LOWORD(info->dwFileVersionMS),
                   HIWORD(info->dwFileVersionLS));
        return out;
    }

    std::wstring knownFolder(REFKNOWNFOLDERID id)
    {
        PWSTR raw = nullptr;
        std::wstring result;
        if (SUCCEEDED(SHGetKnownFolderPath(id, KF_FLAG_DEFAULT, nullptr, &raw))) {
            result = raw;
        }
        if (raw) CoTaskMemFree(raw);
        return result;
    }

    std::wstring installDirectory()
    {
        return joinPath(knownFolder(FOLDERID_ProgramFiles), kProductName);
    }

    std::wstring joinPath(const std::wstring& a, const std::wstring& b)
    {
        if (a.empty()) return b;
        if (!a.empty() && a.back() == L'\\') return a + b;
        return a + L"\\" + b;
    }

    bool ensureDirectory(const std::wstring& path)
    {
        const int rc = SHCreateDirectoryExW(nullptr, path.c_str(), nullptr);
        return rc == ERROR_SUCCESS || rc == ERROR_ALREADY_EXISTS;
    }

    namespace
    {
        std::vector<DWORD> findAppProcesses(const std::wstring& installDir)
        {
            std::vector<DWORD> pids;
            HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (snap == INVALID_HANDLE_VALUE) return pids;

            PROCESSENTRY32W entry{};
            entry.dwSize = sizeof(entry);
            if (Process32FirstW(snap, &entry)) {
                do {
                    if (_wcsicmp(entry.szExeFile, L"Vethida.exe") != 0)
                        continue;
                    HANDLE proc = OpenProcess(
                        PROCESS_QUERY_LIMITED_INFORMATION, FALSE,
                        entry.th32ProcessID);
                    if (!proc) continue;
                    wchar_t path[MAX_PATH];
                    DWORD len = MAX_PATH;
                    const bool inDir =
                        QueryFullProcessImageNameW(proc, 0, path, &len)
                        && _wcsnicmp(path, installDir.c_str(),
                                     installDir.size()) == 0;
                    CloseHandle(proc);
                    if (inDir) pids.push_back(entry.th32ProcessID);
                } while (Process32NextW(snap, &entry));
            }
            CloseHandle(snap);
            return pids;
        }

        struct CloseWindowsContext {
            const std::vector<DWORD>* pids;
        };

        BOOL CALLBACK closeWindowsProc(HWND hwnd, LPARAM lp)
        {
            const auto* ctx = reinterpret_cast<CloseWindowsContext*>(lp);
            DWORD pid = 0;
            GetWindowThreadProcessId(hwnd, &pid);
            for (DWORD candidate : *ctx->pids) {
                if (candidate == pid) {
                    PostMessageW(hwnd, WM_CLOSE, 0, 0);
                    break;
                }
            }
            return TRUE;
        }
    }

    bool isAppRunning(const std::wstring& installDir)
    {
        return !findAppProcesses(installDir).empty();
    }

    bool closeRunningApp(const std::wstring& installDir, DWORD gracefulWaitMs,
                         CloseMode mode)
    {
        std::vector<DWORD> pids = findAppProcesses(installDir);
        if (pids.empty()) return true;

        CloseWindowsContext ctx{ &pids };
        EnumWindows(closeWindowsProc, reinterpret_cast<LPARAM>(&ctx));

        const ULONGLONG deadline = GetTickCount64() + gracefulWaitMs;
        while (GetTickCount64() < deadline) {
            if (findAppProcesses(installDir).empty()) return true;
            Sleep(250);
        }

        if (mode == CloseMode::Graceful)
            return false;

        for (DWORD pid : findAppProcesses(installDir)) {
            HANDLE proc = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
            if (proc) {
                TerminateProcess(proc, 1);
                CloseHandle(proc);
            }
        }
        Sleep(500);
        return findAppProcesses(installDir).empty();
    }

    bool createShortcut(const std::wstring& linkPath,
                        const std::wstring& targetPath,
                        const std::wstring& workingDir,
                        const std::wstring& description)
    {
        IShellLinkW* link = nullptr;
        HRESULT hr = CoCreateInstance(CLSID_ShellLink, nullptr,
                                      CLSCTX_INPROC_SERVER, IID_IShellLinkW,
                                      reinterpret_cast<void**>(&link));
        if (FAILED(hr)) return false;

        link->SetPath(targetPath.c_str());
        link->SetWorkingDirectory(workingDir.c_str());
        link->SetDescription(description.c_str());

        IPersistFile* file = nullptr;
        hr = link->QueryInterface(IID_IPersistFile,
                                  reinterpret_cast<void**>(&file));
        if (SUCCEEDED(hr)) {
            hr = file->Save(linkPath.c_str(), TRUE);
            file->Release();
        }
        link->Release();
        return SUCCEEDED(hr);
    }

    std::wstring startMenuShortcutPath()
    {
        return joinPath(knownFolder(FOLDERID_CommonPrograms),
                        std::wstring(kProductName) + L".lnk");
    }

    std::wstring desktopShortcutPath()
    {
        return joinPath(knownFolder(FOLDERID_PublicDesktop),
                        std::wstring(kProductName) + L".lnk");
    }

    bool writeUninstallRegistry(const std::wstring& installDir,
                                const std::wstring& displayVersion,
                                DWORD estimatedSizeKb)
    {
        HKEY key = nullptr;
        if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, kUninstallKey, 0, nullptr, 0,
                            KEY_WRITE | KEY_WOW64_64KEY, nullptr, &key,
                            nullptr) != ERROR_SUCCESS)
            return false;

        const std::wstring exe = joinPath(installDir, L"Vethida.exe");
        const std::wstring uninstall = joinPath(installDir, L"uninstall.exe");
        const std::wstring quiet = L"\"" + uninstall + L"\" /silent";
        const std::wstring uninstallCmd = L"\"" + uninstall + L"\"";

        auto setStr = [&](const wchar_t* name, const std::wstring& value) {
            RegSetValueExW(key, name, 0, REG_SZ,
                           reinterpret_cast<const BYTE*>(value.c_str()),
                           static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t)));
        };
        auto setDword = [&](const wchar_t* name, DWORD value) {
            RegSetValueExW(key, name, 0, REG_DWORD,
                           reinterpret_cast<const BYTE*>(&value), sizeof(value));
        };

        setStr(L"DisplayName", kProductName);
        setStr(L"DisplayVersion", displayVersion);
        setStr(L"Publisher", kPublisherName);
        setStr(L"InstallLocation", installDir);
        setStr(L"DisplayIcon", exe);
        setStr(L"UninstallString", uninstallCmd);
        setStr(L"QuietUninstallString", quiet);
        setStr(L"URLInfoAbout", L"https://vethida.com");
        setStr(L"HelpLink", L"https://vethida.com/support");
        setDword(L"EstimatedSize", estimatedSizeKb);
        setDword(L"NoModify", 1);
        setDword(L"NoRepair", 1);

        RegCloseKey(key);
        return true;
    }

    void removeUninstallRegistry()
    {
        RegDeleteTreeW(HKEY_LOCAL_MACHINE, kUninstallKey);
    }

    bool deleteTree(const std::wstring& dir, const std::wstring& keepFile)
    {
        const DWORD dirAttrs = GetFileAttributesW(dir.c_str());
        if (dirAttrs == INVALID_FILE_ATTRIBUTES)
            return true;

        WIN32_FIND_DATAW fd{};
        HANDLE find = FindFirstFileW((dir + L"\\*").c_str(), &fd);
        if (find == INVALID_HANDLE_VALUE) return false;

        bool keptSomething = false;
        do {
            const std::wstring name = fd.cFileName;
            if (name == L"." || name == L"..") continue;
            const std::wstring full = joinPath(dir, name);
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                // don't follow junctions, just remove the link
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
                    SetFileAttributesW(full.c_str(), FILE_ATTRIBUTE_NORMAL);
                    if (!RemoveDirectoryW(full.c_str())) keptSomething = true;
                } else if (!deleteTree(full, keepFile)) {
                    keptSomething = true;
                }
            } else if (!keepFile.empty()
                       && _wcsicmp(full.c_str(), keepFile.c_str()) == 0) {
                keptSomething = true;
            } else {
                SetFileAttributesW(full.c_str(), FILE_ATTRIBUTE_NORMAL);
                if (!DeleteFileW(full.c_str())) {
                    MoveFileExW(full.c_str(), nullptr,
                                MOVEFILE_DELAY_UNTIL_REBOOT);
                    keptSomething = true;
                }
            }
        } while (FindNextFileW(find, &fd));
        FindClose(find);

        SetFileAttributesW(dir.c_str(), FILE_ATTRIBUTE_NORMAL);
        if (!RemoveDirectoryW(dir.c_str())) {
            MoveFileExW(dir.c_str(), nullptr, MOVEFILE_DELAY_UNTIL_REBOOT);
            return false;
        }
        return !keptSomething;
    }

    void launchUnelevated(const std::wstring& exePath)
    {
        // going through explorer drops the admin token
        ShellExecuteW(nullptr, L"open", L"explorer.exe", exePath.c_str(),
                      nullptr, SW_SHOWNORMAL);
    }
}
