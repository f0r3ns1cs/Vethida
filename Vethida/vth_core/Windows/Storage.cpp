#include "Storage.h"

#include <filesystem>
#include <system_error>

#include <windows.h>

namespace vth::win
{
    namespace
    {
        namespace fs = std::filesystem;

        quint64 directorySize(const fs::path& dir) {
            std::error_code ec;
            if (!fs::exists(dir, ec) || !fs::is_directory(dir, ec)) {
                return 0;
            }

            quint64 total = 0;
            fs::recursive_directory_iterator it(
                dir,
                fs::directory_options::skip_permission_denied,
                ec);
            fs::recursive_directory_iterator end;

            while (it != end) {
                std::error_code lec;
                if (it->is_regular_file(lec) && !lec) {
                    const auto sz = it->file_size(lec);
                    if (!lec) {
                        total += static_cast<quint64>(sz);
                    }
                }
                it.increment(ec);
                if (ec) ec.clear();
            }
            return total;
        }

        quint64 fileSize(const fs::path& path) {
            WIN32_FILE_ATTRIBUTE_DATA data{};
            if (!GetFileAttributesExW(path.wstring().c_str(),
                                      GetFileExInfoStandard, &data)) {
                return 0;
            }
            if (data.dwFileAttributes == INVALID_FILE_ATTRIBUTES) return 0;
            if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) return 0;
            return (static_cast<quint64>(data.nFileSizeHigh) << 32) | data.nFileSizeLow;
        }

        bool resolveSystemRoot(wchar_t (&root)[4]) {
            wchar_t systemDir[MAX_PATH] = {0};
            if (GetSystemDirectoryW(systemDir, MAX_PATH) == 0) return false;
            if (systemDir[0] == L'\0' || systemDir[1] != L':') return false;
            root[0] = systemDir[0];
            root[1] = L':';
            root[2] = L'\\';
            root[3] = L'\0';
            return true;
        }

        // "X:\" form, empty if it isn't a drive letter
        std::wstring normalizeRoot(const QString& root) {
            const std::wstring in = root.toStdWString();
            if (in.size() < 2 || in[1] != L':') return {};
            wchar_t out[4] = { in[0], L':', L'\\', L'\0' };
            return std::wstring(out);
        }
    }

    QString systemDriveRoot() {
        wchar_t root[4];
        if (!resolveSystemRoot(root)) return {};
        return QString::fromWCharArray(root);
    }

    DriveSpace driveSpace(const QString& root) {
        DriveSpace out;

        const std::wstring r = normalizeRoot(root);
        if (r.empty()) return out;

        ULARGE_INTEGER freeToCaller{}, total{}, totalFree{};
        if (!GetDiskFreeSpaceExW(r.c_str(), &freeToCaller, &total, &totalFree)) {
            return out;
        }

        out.totalBytes = total.QuadPart;
        out.freeBytes  = freeToCaller.QuadPart;
        out.valid      = true;
        return out;
    }

    StorageBreakdown driveBreakdown(const QString& root) {
        StorageBreakdown b;

        const std::wstring r = normalizeRoot(root);
        if (r.empty()) return b;

        ULARGE_INTEGER freeToCaller{}, total{}, totalFree{};
        if (!GetDiskFreeSpaceExW(r.c_str(), &freeToCaller, &total, &totalFree)) {
            return b;
        }

        b.totalBytes = total.QuadPart;
        b.freeBytes  = totalFree.QuadPart;

        const fs::path rootPath(r);
        b.windowsBytes         = directorySize(rootPath / L"Windows");
        b.programFilesBytes    = directorySize(rootPath / L"Program Files");
        b.programFilesX86Bytes = directorySize(rootPath / L"Program Files (x86)");
        b.usersBytes           = directorySize(rootPath / L"Users");
        b.pageFileBytes        = fileSize(rootPath / L"pagefile.sys");
        b.hibernationBytes     = fileSize(rootPath / L"hiberfil.sys");

        const quint64 used = (b.totalBytes > b.freeBytes)
                                ? (b.totalBytes - b.freeBytes) : 0;
        const quint64 measured = b.windowsBytes
                               + b.programFilesBytes
                               + b.programFilesX86Bytes
                               + b.usersBytes
                               + b.pageFileBytes
                               + b.hibernationBytes;
        b.otherBytes = (used > measured) ? (used - measured) : 0;
        b.valid      = true;
        return b;
    }

    std::vector<DriveInfo> fixedDrives() {
        std::vector<DriveInfo> drives;

        wchar_t sysRoot[4];
        const bool haveSys = resolveSystemRoot(sysRoot);
        const wchar_t sysLetter = haveSys ? sysRoot[0] : L'\0';

        const DWORD mask = GetLogicalDrives();
        for (int i = 0; i < 26; ++i) {
            if (!(mask & (1u << i))) continue;

            wchar_t rootW[4] = { static_cast<wchar_t>(L'A' + i), L':', L'\\', L'\0' };
            if (GetDriveTypeW(rootW) != DRIVE_FIXED) continue;

            DriveInfo info;
            info.root     = QString::fromWCharArray(rootW);
            info.isSystem = (rootW[0] == sysLetter);

            ULARGE_INTEGER freeToCaller{}, total{}, totalFree{};
            if (GetDiskFreeSpaceExW(rootW, &freeToCaller, &total, &totalFree)) {
                info.totalBytes = total.QuadPart;
                info.freeBytes  = freeToCaller.QuadPart;
            }

            wchar_t label[MAX_PATH + 1] = {0};
            wchar_t fsName[MAX_PATH + 1] = {0};
            if (GetVolumeInformationW(rootW, label, MAX_PATH + 1, nullptr, nullptr,
                                      nullptr, fsName, MAX_PATH + 1)) {
                info.label      = QString::fromWCharArray(label);
                info.fileSystem = QString::fromWCharArray(fsName);
            }

            drives.push_back(std::move(info));
        }

        return drives;
    }

}

namespace vth::win
{
    DriveSpace systemDriveSpace() {
        return driveSpace(systemDriveRoot());
    }
    StorageBreakdown systemDriveBreakdown() {
        return driveBreakdown(systemDriveRoot());
    }
}
