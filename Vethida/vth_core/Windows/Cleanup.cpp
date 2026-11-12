#include "Cleanup.h"

#include "Storage.h"
#include "../Logger.h"

#include <QProcess>
#include <QStringList>
#include <QStringLiteral>

#include <chrono>
#include <filesystem>
#include <system_error>

#include <windows.h>
#include <shellapi.h>
#include <QFile>
#include <QFileInfo>

namespace vth::win::cleanup
{
    namespace
    {
        namespace fs = std::filesystem;

        VTH_DEFINE_LOG_CATEGORY(catCleanup, "vth.cleanup")

        const QString kHiberFile = QStringLiteral("%SystemDrive%\\hiberfil.sys");

        // Absolute, because this runs elevated and PATH could be hijacked.
        QString system32Path(const QString& relative) {
            wchar_t dir[MAX_PATH] = {};
            const UINT n = GetSystemDirectoryW(dir, MAX_PATH);
            const QString system32 = (n > 0 && n < MAX_PATH)
                ? QString::fromWCharArray(dir, static_cast<int>(n))
                : QStringLiteral("C:\\Windows\\System32");
            return system32 + QLatin1Char('\\') + relative;
        }

        QString expandEnv(const QString& s) {
            if (s.isEmpty()) return s;
            const std::wstring in = s.toStdWString();
            DWORD needed = ExpandEnvironmentStringsW(in.c_str(), nullptr, 0);
            if (needed == 0) return s;
            std::wstring out(needed, L'\0');
            DWORD wrote = ExpandEnvironmentStringsW(in.c_str(), out.data(), needed);
            if (wrote == 0 || wrote > needed) return s;
            out.resize(wrote - 1);
            return QString::fromStdWString(out);
        }

        fs::path toFsPath(const QString& s) {
            return fs::path(s.toStdWString());
        }

        struct WalkStats {
            quint64 bytes         = 0;
            int     filesAffected = 0;
            int     errors        = 0;
        };

        WalkStats walkPath(const Path& p, bool doDelete) {
            WalkStats st;

            const QString expanded = expandEnv(p.path);
            if (expanded.isEmpty()) return st;

            const fs::path root = toFsPath(expanded);
            std::error_code ec;
            if (!fs::exists(root, ec) || !fs::is_directory(root, ec)) return st;

            const bool useAge = (p.minAgeDays > 0);
            const auto cutoff = useAge
                ? fs::file_time_type::clock::now()
                      - std::chrono::hours(24 * p.minAgeDays)
                : fs::file_time_type{};

            auto handleFile = [&](const fs::directory_entry& entry) {
                std::error_code fec;
                if (!entry.is_regular_file(fec)) return;

                if (useAge) {
                    const auto t = entry.last_write_time(fec);
                    if (fec) return;
                    if (t > cutoff) return;
                }

                const auto sz = entry.file_size(fec);
                if (fec) { ++st.errors; return; }

                if (doDelete) {
                    std::error_code dec;
                    if (fs::remove(entry.path(), dec)) {
                        st.bytes += sz;
                        ++st.filesAffected;
                    } else {
                        ++st.errors;
                    }
                } else {
                    st.bytes += sz;
                    ++st.filesAffected;
                }
            };

            if (p.recursive) {
                fs::recursive_directory_iterator it(
                    root,
                    fs::directory_options::skip_permission_denied,
                    ec);
                if (ec) return st;
                for (; it != fs::recursive_directory_iterator{}; it.increment(ec)) {
                    if (ec) { ec.clear(); continue; }
                    handleFile(*it);
                }
            } else {
                fs::directory_iterator it(root, ec);
                if (ec) return st;
                for (; it != fs::directory_iterator{}; it.increment(ec)) {
                    if (ec) { ec.clear(); continue; }
                    handleFile(*it);
                }
            }
            return st;
        }

        // A null root means every drive.
        quint64 recycleBinBytes(const wchar_t* root) {
            SHQUERYRBINFO info{};
            info.cbSize = sizeof(info);
            if (FAILED(SHQueryRecycleBinW(root, &info))) return 0;
            return static_cast<quint64>(info.i64Size);
        }

        WalkStats emptyRecycleBinRoot(const wchar_t* root) {
            WalkStats st;
            SHQUERYRBINFO before{};
            before.cbSize = sizeof(before);
            // Emptying an empty bin can return an error, so don't count that as one.
            if (SUCCEEDED(SHQueryRecycleBinW(root, &before)) && before.i64NumItems == 0)
                return st;

            const HRESULT hr = SHEmptyRecycleBinW(
                nullptr, root,
                SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND);

            SHQUERYRBINFO after{};
            after.cbSize = sizeof(after);
            SHQueryRecycleBinW(root, &after);

            if (before.i64Size > after.i64Size)
                st.bytes = static_cast<quint64>(before.i64Size - after.i64Size);
            if (before.i64NumItems > after.i64NumItems)
                st.filesAffected =
                    static_cast<int>(before.i64NumItems - after.i64NumItems);
            if (FAILED(hr)) ++st.errors;
            return st;
        }

        quint64 freeBytesOf(const std::wstring& root) {
            ULARGE_INTEGER freeToCaller{}, total{}, totalFree{};
            if (!GetDiskFreeSpaceExW(root.c_str(), &freeToCaller, &total, &totalFree))
                return 0;
            return static_cast<quint64>(freeToCaller.QuadPart);
        }

        // Reads the directory entry, because the kernel holds hiberfil.sys open and
        // std::filesystem::file_size would fail on it.
        quint64 fileSizeOf(const QString& path) {
            const std::wstring w = expandEnv(path).toStdWString();
            WIN32_FIND_DATAW fd{};
            const HANDLE h = FindFirstFileW(w.c_str(), &fd);
            if (h == INVALID_HANDLE_VALUE)
                return 0;
            FindClose(h);
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                return 0;
            return (static_cast<quint64>(fd.nFileSizeHigh) << 32)
                 | static_cast<quint64>(fd.nFileSizeLow);
        }

        // Returns false if the file exists but couldn't be removed.
        bool deleteFileIfPresent(const QString& path, quint64& freed) {
            freed = 0;
            const quint64 sz = fileSizeOf(path);
            if (sz == 0) return true;
            std::error_code ec;
            if (!fs::remove(toFsPath(expandEnv(path)), ec)) return false;
            freed = sz;
            return true;
        }

        // Returns the exit code, or -1 if it didn't start, -2 if it timed out and
        // -3 if it crashed.
        int runProcessBlocking(const QString& program, const QStringList& args,
                               int timeoutMs) {
            QProcess proc;
            proc.setProcessChannelMode(QProcess::MergedChannels);
            proc.start(program, args);
            if (!proc.waitForStarted(15000)) return -1;
            if (!proc.waitForFinished(timeoutMs)) {
                proc.kill();
                proc.waitForFinished(2000);
                return -2;
            }
            return proc.exitStatus() == QProcess::NormalExit ? proc.exitCode() : -3;
        }

        // Runs every cleanmgr handler except the four that delete things people
        // want to keep. Returns whether it started.
        bool runWindowsDiskCleanup() {
            const QString script = QStringLiteral(
                "$base='HKLM:\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VolumeCaches';"
                "$skip=@('DownloadsFolder','Recycle Bin','Previous Installations','Windows ESD installation files');"
                "if(Test-Path $base){Get-ChildItem $base | ForEach-Object {"
                "$v=if($skip -contains $_.PSChildName){0}else{2};"
                "New-ItemProperty -Path $_.PSPath -Name 'StateFlags0099' -Value $v "
                "-PropertyType DWord -Force -ErrorAction SilentlyContinue | Out-Null}}"
                "Start-Process -FilePath (Join-Path $env:SystemRoot 'System32\\cleanmgr.exe') "
                "-ArgumentList '/sagerun:99'");

            return QProcess::startDetached(
                system32Path(QStringLiteral("WindowsPowerShell\\v1.0\\powershell.exe")), {
                QStringLiteral("-NoProfile"),
                QStringLiteral("-NonInteractive"),
                QStringLiteral("-WindowStyle"), QStringLiteral("Hidden"),
                QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"),
                QStringLiteral("-Command"), script,
            });
        }

        struct CleanResult {
            quint64 bytesFreed   = 0;
            int     filesDeleted = 0;
            int     errors       = 0;
        };
    }

    QString categoryLabel(Category c) {
        switch (c) {
            case Category::TempAndCache:  return QStringLiteral("Temporary files");
            case Category::AppCaches:     return QStringLiteral("Application caches");
            case Category::ShaderCaches:  return QStringLiteral("Shader caches");
            case Category::WindowsUpdate: return QStringLiteral("Windows Update");
            case Category::Logs:          return QStringLiteral("Logs");
            case Category::Dumps:         return QStringLiteral("Crash dumps");
            case Category::Misc:          return QStringLiteral("Other");
            case Category::Deep:          return QStringLiteral("Deep clean");
        }
        return {};
    }

    ScanResult scan(const Cleaner& c, const QString& driveRoot) {
        switch (c.mechanism) {
        case Mechanism::RecycleBin: {
            const std::wstring root = driveRoot.toStdWString();
            return { recycleBinBytes(root.empty() ? nullptr : root.c_str()), true };
        }
        case Mechanism::Hibernation:
            return { fileSizeOf(kHiberFile), true };
        case Mechanism::ComponentStore:
        case Mechanism::WindowsDiskCleanup:
            return { 0, false };
        case Mechanism::FileDelete: {
            ScanResult r;
            for (const auto& p : c.paths)
                r.bytes += fileSizeOf(p.path);
            return r;
        }
        case Mechanism::DirectoryWalk:
            break;
        }

        ScanResult r;
        for (const auto& p : c.paths)
            r.bytes += walkPath(p, /*doDelete=*/false).bytes;
        return r;
    }

    static CleanResult clean(const Cleaner& c, const QString& driveRoot) {
        switch (c.mechanism) {
        case Mechanism::RecycleBin: {
            const std::wstring root = driveRoot.toStdWString();
            const auto st = emptyRecycleBinRoot(root.empty() ? nullptr : root.c_str());
            return { st.bytes, st.filesAffected, st.errors };
        }
        case Mechanism::ComponentStore: {
            // Can take this long on an older install. 3010 means it worked and
            // needs a restart.
            const int rc = runProcessBlocking(system32Path(QStringLiteral("Dism.exe")), {
                QStringLiteral("/online"), QStringLiteral("/Cleanup-Image"),
                QStringLiteral("/StartComponentCleanup"), QStringLiteral("/ResetBase") },
                20 * 60 * 1000);
            return { 0, 0, (rc == 0 || rc == ERROR_SUCCESS_REBOOT_REQUIRED) ? 0 : 1 };
        }
        case Mechanism::Hibernation: {
            const quint64 was = fileSizeOf(kHiberFile);
            const int rc = runProcessBlocking(system32Path(QStringLiteral("powercfg.exe")), {
                QStringLiteral("/hibernate"), QStringLiteral("off") }, 60 * 1000);
            return { rc == 0 ? was : 0, 0, rc == 0 ? 0 : 1 };
        }
        case Mechanism::WindowsDiskCleanup:
            // Detached, so there is nothing to report but whether it started.
            return { 0, 0, runWindowsDiskCleanup() ? 0 : 1 };
        case Mechanism::FileDelete: {
            CleanResult r;
            for (const auto& p : c.paths) {
                quint64 freed = 0;
                if (!deleteFileIfPresent(p.path, freed))
                    ++r.errors;
                else if (freed > 0) {
                    r.bytesFreed += freed;
                    ++r.filesDeleted;
                }
            }
            return r;
        }
        case Mechanism::DirectoryWalk:
            break;
        }

        WalkStats agg;
        for (const auto& p : c.paths) {
            const auto st = walkPath(p, /*doDelete=*/true);
            agg.bytes         += st.bytes;
            agg.filesAffected += st.filesAffected;
            agg.errors        += st.errors;
        }
        if (agg.errors > 0) {
            qCWarning(catCleanup).noquote()
                << "clean[" << c.id << "]:" << agg.errors
                << "files could not be removed (locked or denied)";
        }
        return { agg.bytes, agg.filesAffected, agg.errors };
    }

    // Safest first, with the off-by-default deep group last.
    QVector<Category> orderedCategories() {
        return {
            Category::TempAndCache,
            Category::AppCaches,
            Category::ShaderCaches,
            Category::WindowsUpdate,
            Category::Dumps,
            Category::Logs,
            Category::Misc,
            Category::Deep,
        };
    }

    static const QVector<Cleaner>& cleaners() {
        static const QVector<Cleaner> kList = {
            {
                QStringLiteral("temp.user"),
                QStringLiteral("User Temp folder"),
                Category::TempAndCache, reg::Severity::Low,
                QStringLiteral(
                    "Files Windows applications dump into %TEMP% but never "
                    "clean up. Often the largest single source of recoverable "
                    "space."),
                QStringLiteral(
                    "Files modified in the last 24 hours are skipped to avoid "
                    "touching anything an open application is still using."),
                Mechanism::DirectoryWalk,
                {{ QStringLiteral("%TEMP%"), /*recursive=*/true, /*minAgeDays=*/1 }},
            },
            {
                QStringLiteral("temp.system"),
                QStringLiteral("System Temp folder"),
                Category::TempAndCache, reg::Severity::Low,
                QStringLiteral(
                    "C:\\Windows\\Temp - system-level scratch directory. "
                    "Mostly leftover installer / update payloads."),
                QStringLiteral(
                    "Same 24-hour age filter as the User Temp cleaner. Some "
                    "system-owned files need administrator rights to delete."),
                Mechanism::DirectoryWalk,
                {{ QStringLiteral("%SystemRoot%\\Temp"),
                   true, 1 }},
            },
            {
                QStringLiteral("temp.thumbnail_cache"),
                QStringLiteral("Thumbnail cache"),
                Category::TempAndCache, reg::Severity::Low,
                QStringLiteral(
                    "Explorer's pre-rendered thumbnail bitmaps for image and "
                    "video folders."),
                QStringLiteral(
                    "Thumbnails regenerate the next time you open a folder. "
                    "No data loss, brief Explorer pause on first folder "
                    "view."),
                Mechanism::DirectoryWalk,
                {{ QStringLiteral("%LOCALAPPDATA%\\Microsoft\\Windows\\Explorer"),
                   false, 0 }},
            },
            {
                QStringLiteral("temp.recycle_bin"),
                QStringLiteral("Recycle Bin"),
                Category::TempAndCache, reg::Severity::Low,
                QStringLiteral(
                    "Everything currently in the Recycle Bin on this drive."),
                QStringLiteral(
                    "Files cannot be restored after this. Anything you "
                    "intentionally deleted is gone for good."),
                Mechanism::RecycleBin,
                {},
                /*recommended=*/false, /*measurable=*/true, /*perVolume=*/true,
            },
            {
                QStringLiteral("temp.inet_cache"),
                QStringLiteral("Windows internet cache"),
                Category::TempAndCache, reg::Severity::Low,
                QStringLiteral(
                    "The WinINet cache under %LOCALAPPDATA%\\Microsoft\\Windows"
                    "\\INetCache, which Windows and any application using its "
                    "networking stack writes into."),
                QStringLiteral(
                    "Pages and images are fetched again on first use. Nothing "
                    "signed in is signed out."),
                Mechanism::DirectoryWalk,
                {{ QStringLiteral("%LOCALAPPDATA%\\Microsoft\\Windows\\INetCache"),
                   true, 0 }},
            },
            {
                QStringLiteral("temp.font_cache"),
                QStringLiteral("Font cache"),
                Category::TempAndCache, reg::Severity::Low,
                QStringLiteral(
                    "Windows' pre-rasterized font atlas, which grows with every "
                    "font ever installed and is never pruned."),
                QStringLiteral(
                    "Rebuilt on the next sign-in. Text may take a moment longer "
                    "to draw the first time each font is used."),
                Mechanism::DirectoryWalk,
                {{ QStringLiteral("%SystemRoot%\\ServiceProfiles\\LocalService"
                                  "\\AppData\\Local\\FontCache"),
                   true, 0 }},
            },
            {
                QStringLiteral("temp.downloaded_program_files"),
                QStringLiteral("Downloaded program files"),
                Category::TempAndCache, reg::Severity::Low,
                QStringLiteral(
                    "ActiveX controls and Java applets Internet Explorer "
                    "downloaded, in C:\\Windows\\Downloaded Program Files."),
                QStringLiteral("Empty on most installs."),
                Mechanism::DirectoryWalk,
                {{ QStringLiteral("%SystemRoot%\\Downloaded Program Files"),
                   true, 0 }},
            },

            {
                QStringLiteral("appcache.discord"),
                QStringLiteral("Discord cache"),
                Category::AppCaches, reg::Severity::Low,
                QStringLiteral(
                    "Discord's downloaded media cache (avatars, embeds, "
                    "attachments) under %APPDATA%\\discord\\Cache."),
                QStringLiteral(
                    "Repopulates as you re-open conversations. May briefly "
                    "hit the network on first scroll."),
                Mechanism::DirectoryWalk,
                {{ QStringLiteral("%APPDATA%\\discord\\Cache\\Cache_Data"),
                   true, 0 }},
            },
            {
                QStringLiteral("appcache.spotify"),
                QStringLiteral("Spotify cache"),
                Category::AppCaches, reg::Severity::Low,
                QStringLiteral(
                    "Spotify's offline-track and image cache under "
                    "%LOCALAPPDATA%\\Spotify\\Storage."),
                QStringLiteral(
                    "Songs marked Available Offline are also stored here. "
                    "Clearing forces them to re-download from Spotify."),
                Mechanism::DirectoryWalk,
                {{ QStringLiteral("%LOCALAPPDATA%\\Spotify\\Storage"),
                   true, 0 }},
                /*recommended=*/false,
            },
            {
                QStringLiteral("appcache.browsers"),
                QStringLiteral("Browser cache"),
                Category::AppCaches, reg::Severity::Low,
                QStringLiteral(
                    "The disk caches of the browsers on this PC: Chrome, Edge, "
                    "Firefox, Brave and Opera."),
                QStringLiteral(
                    "Cookies, history, and saved passwords are not touched. "
                    "Pages will load from the network on first visit."),
                Mechanism::DirectoryWalk,
                {
                    { QStringLiteral("%LOCALAPPDATA%\\Google\\Chrome\\User Data\\Default\\Cache"), true, 0 },
                    { QStringLiteral("%LOCALAPPDATA%\\Microsoft\\Edge\\User Data\\Default\\Cache"), true, 0 },
                    { QStringLiteral("%LOCALAPPDATA%\\Mozilla\\Firefox\\Profiles"), true, 0 },
                    { QStringLiteral("%LOCALAPPDATA%\\BraveSoftware\\Brave-Browser"
                                     "\\User Data\\Default\\Cache"), true, 0 },
                    { QStringLiteral("%LOCALAPPDATA%\\Opera Software\\Opera Stable\\Cache"), true, 0 },
                },
            },
            {
                QStringLiteral("appcache.game_launchers"),
                QStringLiteral("Game launcher caches"),
                Category::AppCaches, reg::Severity::Low,
                QStringLiteral(
                    "Steam's HTTP cache, logs and interrupted-download staging "
                    "folder, and the Epic Games Launcher's web cache and logs."),
                QStringLiteral(
                    "An interrupted download restarts rather than resuming. "
                    "Installed games are not touched."),
                Mechanism::DirectoryWalk,
                {
                    { QStringLiteral("%ProgramFiles(x86)%\\Steam\\appcache\\httpcache"), true, 0 },
                    { QStringLiteral("%ProgramFiles(x86)%\\Steam\\logs"), true, 0 },
                    { QStringLiteral("%ProgramFiles(x86)%\\Steam\\steamapps\\downloading"), true, 0 },
                    { QStringLiteral("%LOCALAPPDATA%\\EpicGamesLauncher\\Saved\\webcache"), true, 0 },
                    { QStringLiteral("%LOCALAPPDATA%\\EpicGamesLauncher\\Saved\\Logs"), true, 0 },
                },
            },
            {
                QStringLiteral("appcache.vscode"),
                QStringLiteral("Visual Studio Code cache"),
                Category::AppCaches, reg::Severity::Low,
                QStringLiteral(
                    "Code's disk cache, its compiled-script cache and its logs."),
                QStringLiteral(
                    "Settings, extensions and open folders are not touched. The "
                    "first launch afterwards is slightly slower."),
                Mechanism::DirectoryWalk,
                {
                    { QStringLiteral("%APPDATA%\\Code\\Cache"), true, 0 },
                    { QStringLiteral("%APPDATA%\\Code\\CachedData"), true, 0 },
                    { QStringLiteral("%APPDATA%\\Code\\logs"), true, 0 },
                },
            },
            {
                QStringLiteral("appcache.adobe_media"),
                QStringLiteral("Adobe media cache"),
                Category::AppCaches, reg::Severity::Low,
                QStringLiteral(
                    "The conformed audio and peak files Premiere Pro and After "
                    "Effects write beside every clip they open. Routinely tens "
                    "of gigabytes on a machine that edits video."),
                QStringLiteral(
                    "Rebuilt the next time a project is opened, which takes as "
                    "long as it took the first time. Projects and footage are "
                    "not touched."),
                Mechanism::DirectoryWalk,
                {{ QStringLiteral("%LOCALAPPDATA%\\Adobe\\Common\\Media Cache Files"),
                   true, 0 }},
            },
            {
                QStringLiteral("appcache.package_managers"),
                QStringLiteral("Developer package caches"),
                Category::AppCaches, reg::Severity::Medium,
                QStringLiteral(
                    "The download caches npm and pip keep so a second install "
                    "of the same package does not hit the network."),
                QStringLiteral(
                    "Nothing already installed breaks. The next install of each "
                    "package downloads again, which on a metered connection is "
                    "the cost of this one."),
                Mechanism::DirectoryWalk,
                {
                    { QStringLiteral("%LOCALAPPDATA%\\npm-cache"), true, 0 },
                    { QStringLiteral("%LOCALAPPDATA%\\pip\\Cache"), true, 0 },
                },
                /*recommended=*/false,
            },

            {
                QStringLiteral("shader.directx"),
                QStringLiteral("DirectX shader cache"),
                Category::ShaderCaches, reg::Severity::Low,
                QStringLiteral(
                    "Windows' own compiled-shader store, shared by every Direct3D "
                    "application on the machine."),
                QStringLiteral(
                    "Rebuilt as each game runs. Expect brief stutter the first "
                    "few minutes of a session afterwards."),
                Mechanism::DirectoryWalk,
                {{ QStringLiteral("%LOCALAPPDATA%\\D3DSCache"), true, 0 }},
            },
            {
                QStringLiteral("shader.nvidia"),
                QStringLiteral("NVIDIA shader cache"),
                Category::ShaderCaches, reg::Severity::Low,
                QStringLiteral(
                    "The driver's DirectX, OpenGL and compute shader caches."),
                QStringLiteral(
                    "Rebuilt as each game runs. Also the standard fix for shader "
                    "corruption after a driver upgrade."),
                Mechanism::DirectoryWalk,
                {
                    { QStringLiteral("%LOCALAPPDATA%\\NVIDIA\\DXCache"), true, 0 },
                    { QStringLiteral("%LOCALAPPDATA%\\NVIDIA\\GLCache"), true, 0 },
                    { QStringLiteral("%LOCALAPPDATA%\\NVIDIA Corporation\\NV_Cache"), true, 0 },
                    { QStringLiteral("%APPDATA%\\NVIDIA\\ComputeCache"), true, 0 },
                },
            },
            {
                QStringLiteral("shader.amd"),
                QStringLiteral("AMD shader cache"),
                Category::ShaderCaches, reg::Severity::Low,
                QStringLiteral(
                    "The driver's DirectX and OpenGL shader caches."),
                QStringLiteral(
                    "Rebuilt as each game runs. Also the standard fix for shader "
                    "corruption after a driver upgrade."),
                Mechanism::DirectoryWalk,
                {
                    { QStringLiteral("%LOCALAPPDATA%\\AMD\\DxCache"), true, 0 },
                    { QStringLiteral("%LOCALAPPDATA%\\AMD\\DxcCache"), true, 0 },
                    { QStringLiteral("%LOCALAPPDATA%\\AMD\\GLCache"), true, 0 },
                },
            },
            {
                QStringLiteral("shader.intel"),
                QStringLiteral("Intel shader cache"),
                Category::ShaderCaches, reg::Severity::Low,
                QStringLiteral("The Intel graphics driver's shader cache."),
                QStringLiteral("Rebuilt as each game runs."),
                Mechanism::DirectoryWalk,
                {{ QStringLiteral("%LOCALAPPDATA%\\Intel\\ShaderCache"), true, 0 }},
            },
            {
                QStringLiteral("shader.steam"),
                QStringLiteral("Steam shader cache"),
                Category::ShaderCaches, reg::Severity::Low,
                QStringLiteral(
                    "Pre-compiled shaders Steam downloads for installed games, "
                    "which it keeps for titles that are no longer installed."),
                QStringLiteral(
                    "Steam downloads them again the next time each game launches."),
                Mechanism::DirectoryWalk,
                {{ QStringLiteral("%ProgramFiles(x86)%\\Steam\\steamapps\\shadercache"),
                   true, 0 }},
            },

            {
                QStringLiteral("update.download_cache"),
                QStringLiteral("Windows Update download cache"),
                Category::WindowsUpdate, reg::Severity::Medium,
                QStringLiteral(
                    "Pending and recently-downloaded Windows Update payloads "
                    "in SoftwareDistribution\\Download."),
                QStringLiteral(
                    "Recovers gigabytes after major updates. If a download is "
                    "in progress some files are in use and are skipped; pause "
                    "Windows Update in Settings first to clear all of it."),
                Mechanism::DirectoryWalk,
                {{ QStringLiteral("%SystemRoot%\\SoftwareDistribution\\Download"),
                   true, 0 }},
            },
            {
                QStringLiteral("update.delivery_optimization"),
                QStringLiteral("Delivery Optimization cache"),
                Category::WindowsUpdate, reg::Severity::Medium,
                QStringLiteral(
                    "P2P-shared Windows Update cache, used to redistribute "
                    "update payloads to other PCs on your network."),
                QStringLiteral(
                    "Re-populates if Delivery Optimization is enabled. No "
                    "user-visible behavior change beyond the next download "
                    "going to Microsoft instead of a nearby peer."),
                Mechanism::DirectoryWalk,
                {{ QStringLiteral("%SystemRoot%\\SoftwareDistribution\\DeliveryOptimization\\Cache"),
                   true, 0 }},
            },
            {
                QStringLiteral("update.upgrade_leftovers"),
                QStringLiteral("Feature update staging folders"),
                Category::WindowsUpdate, reg::Severity::Caution,
                QStringLiteral(
                    "$WINDOWS.~BT, $Windows.~WS and $GetCurrent - the working "
                    "folders a Windows feature update leaves at the root of the "
                    "system drive. Frequently several gigabytes."),
                QStringLiteral(
                    "These are half of what a rollback to the previous Windows "
                    "version needs; Windows.old is the other half. Removing them "
                    "ends that option for good. Owned by TrustedInstaller, so "
                    "expect some files to be skipped."),
                Mechanism::DirectoryWalk,
                {
                    { QStringLiteral("%SystemDrive%\\$WINDOWS.~BT"), true, 0 },
                    { QStringLiteral("%SystemDrive%\\$Windows.~WS"), true, 0 },
                    { QStringLiteral("%SystemDrive%\\$GetCurrent"), true, 0 },
                },
                /*recommended=*/false,
            },

            {
                QStringLiteral("dumps.user"),
                QStringLiteral("Per-user crash dumps"),
                Category::Dumps, reg::Severity::Low,
                QStringLiteral(
                    "Process crash dumps written to %LOCALAPPDATA%\\CrashDumps "
                    "by Windows Error Reporting."),
                QStringLiteral(
                    "Useful only when triaging a specific crash. Safe to "
                    "delete if you're not actively debugging."),
                Mechanism::DirectoryWalk,
                {{ QStringLiteral("%LOCALAPPDATA%\\CrashDumps"),
                   false, 0 }},
            },
            {
                QStringLiteral("dumps.kernel"),
                QStringLiteral("Kernel minidumps"),
                Category::Dumps, reg::Severity::Low,
                QStringLiteral(
                    "C:\\Windows\\Minidump - the small dumps Windows writes "
                    "every time it blue-screens."),
                QStringLiteral(
                    "Useful for kernel crash analysis only. If you are not "
                    "debugging a blue screen, safe to delete."),
                Mechanism::DirectoryWalk,
                {
                    { QStringLiteral("%SystemRoot%\\Minidump"),
                      false, 0 },
                },
            },
            {
                QStringLiteral("dumps.memory_dmp"),
                QStringLiteral("Full memory dump"),
                Category::Dumps, reg::Severity::Low,
                QStringLiteral(
                    "C:\\Windows\\MEMORY.DMP - the complete kernel memory image "
                    "from the last blue screen. As large as this PC's RAM."),
                QStringLiteral(
                    "Useful for kernel crash analysis only. If you are not "
                    "debugging a blue screen, safe to delete."),
                Mechanism::FileDelete,
                {{ QStringLiteral("%SystemRoot%\\MEMORY.DMP"), false, 0 }},
            },
            {
                QStringLiteral("dumps.error_reports"),
                QStringLiteral("Error report queues"),
                Category::Dumps, reg::Severity::Low,
                QStringLiteral(
                    "The queues Windows Error Reporting holds crash reports in "
                    "before and after sending them, plus the live kernel reports "
                    "it writes when a driver stops responding without crashing."),
                QStringLiteral(
                    "Nothing here is read by anything but Microsoft's crash "
                    "reporting."),
                Mechanism::DirectoryWalk,
                {
                    { QStringLiteral("%ProgramData%\\Microsoft\\Windows\\WER\\ReportQueue"), true, 0 },
                    { QStringLiteral("%ProgramData%\\Microsoft\\Windows\\WER\\ReportArchive"), true, 0 },
                    { QStringLiteral("%LOCALAPPDATA%\\Microsoft\\Windows\\WER"), true, 0 },
                    { QStringLiteral("%SystemRoot%\\LiveKernelReports"), true, 0 },
                },
            },

            {
                QStringLiteral("logs.cbs"),
                QStringLiteral("Component Servicing logs"),
                Category::Logs, reg::Severity::Medium,
                QStringLiteral(
                    "C:\\Windows\\Logs\\CBS - logs from sfc / dism / Windows "
                    "Update operations."),
                QStringLiteral(
                    "Used for diagnosing servicing failures. Only valuable "
                    "if you're actively debugging an update issue. Some "
                    "files locked by TrustedInstaller; expect a few "
                    "access-denied entries."),
                Mechanism::DirectoryWalk,
                {{ QStringLiteral("%SystemRoot%\\Logs\\CBS"),
                   false, 0 }},
            },
            {
                QStringLiteral("logs.windows"),
                QStringLiteral("Windows servicing logs"),
                Category::Logs, reg::Severity::Medium,
                QStringLiteral(
                    "The DISM, Windows Update, update-medic and setup logs under "
                    "C:\\Windows\\Logs."),
                QStringLiteral(
                    "Read only when diagnosing a failed update. Windows starts "
                    "new ones on the next servicing operation."),
                Mechanism::DirectoryWalk,
                {
                    { QStringLiteral("%SystemRoot%\\Logs\\DISM"), true, 0 },
                    { QStringLiteral("%SystemRoot%\\Logs\\MoSetup"), true, 0 },
                    { QStringLiteral("%SystemRoot%\\Logs\\WindowsUpdate"), true, 0 },
                    { QStringLiteral("%SystemRoot%\\Logs\\waasmedic"), true, 0 },
                    { QStringLiteral("%SystemRoot%\\Logs\\SIH"), true, 0 },
                },
            },
            {
                QStringLiteral("logs.setup"),
                QStringLiteral("Windows setup logs"),
                Category::Logs, reg::Severity::Medium,
                QStringLiteral(
                    "C:\\Windows\\Panther - everything Windows Setup wrote while "
                    "installing or upgrading this machine. Hundreds of megabytes "
                    "on an install that has taken a few feature updates."),
                QStringLiteral(
                    "Read only when diagnosing a failed installation or upgrade."),
                Mechanism::DirectoryWalk,
                {{ QStringLiteral("%SystemRoot%\\Panther"), true, 0 }},
            },
            {
                QStringLiteral("logs.setupapi"),
                QStringLiteral("Device installation log"),
                Category::Logs, reg::Severity::Medium,
                QStringLiteral(
                    "setupapi.dev.log - one running record of every driver and "
                    "device Windows has ever installed on this PC. It is never "
                    "rotated and grows for the life of the install."),
                QStringLiteral(
                    "Windows starts a new one on the next device installation."),
                Mechanism::FileDelete,
                {{ QStringLiteral("%SystemRoot%\\INF\\setupapi.dev.log"), false, 0 }},
            },

            {
                QStringLiteral("misc.recent_files"),
                QStringLiteral("Recent files"),
                Category::Misc, reg::Severity::Low,
                QStringLiteral(
                    "Explorer's Recent / jump-list cache - the list of "
                    "documents that show up when you right-click a taskbar "
                    "icon."),
                QStringLiteral(
                    "Recent-document menus fill back in as you use apps. Items "
                    "you pinned to a taskbar jump list are cleared too and do "
                    "not come back."),
                Mechanism::DirectoryWalk,
                {{ QStringLiteral("%APPDATA%\\Microsoft\\Windows\\Recent"),
                   true, 0 }},
            },
            {
                QStringLiteral("misc.prefetch"),
                QStringLiteral("Prefetch cache"),
                Category::Misc, reg::Severity::Caution,
                QStringLiteral(
                    "C:\\Windows\\Prefetch - the prefetcher cache Windows "
                    "uses to speed up application launches."),
                QStringLiteral(
                    "Clearing forces Windows to rebuild the prefetcher "
                    "tables. Apps may launch slightly slower for a day or "
                    "two until cache is rebuilt. Most users should NOT clear "
                    "this - it costs you perf with negligible space "
                    "savings."),
                Mechanism::DirectoryWalk,
                {{ QStringLiteral("%SystemRoot%\\Prefetch"),
                   false, 0 }},
                /*recommended=*/false,
            },
            {
                QStringLiteral("misc.windows_old"),
                QStringLiteral("Windows.old"),
                Category::Misc, reg::Severity::Caution,
                QStringLiteral(
                    "Backup of the previous Windows installation, kept after "
                    "a feature update or in-place upgrade."),
                QStringLiteral(
                    "Removes the rollback to your previous Windows for good, "
                    "along with any old files in it you have not moved over yet. "
                    "Windows deletes this on its own about 10 days after an "
                    "upgrade. Only delete it once the new Windows is working."),
                Mechanism::DirectoryWalk,
                {{ QStringLiteral("%SystemDrive%\\Windows.old"), true, 0 }},
                /*recommended=*/false,
            },

            {
                QStringLiteral("deep.component_store"),
                QStringLiteral("Compact the Windows component store"),
                Category::Deep, reg::Severity::Caution,
                QStringLiteral(
                    "Runs DISM over WinSxS to drop the superseded copies of "
                    "every component this install has ever updated. Usually the "
                    "largest single reclaim on a machine that is years old."),
                QStringLiteral(
                    "Updates already installed can no longer be uninstalled "
                    "afterwards. Takes several minutes and cannot be stopped "
                    "part way."),
                Mechanism::ComponentStore,
                {},
                /*recommended=*/false, /*measurable=*/false,
            },
            {
                QStringLiteral("deep.hibernation"),
                QStringLiteral("Remove the hibernation file"),
                Category::Deep, reg::Severity::Caution,
                QStringLiteral(
                    "hiberfil.sys, which Windows sizes at a large fraction of "
                    "installed memory and keeps whether or not you hibernate."),
                QStringLiteral(
                    "Turns off hibernate AND Fast Startup, so cold boots take "
                    "longer. Windows can turn both back on from its own power "
                    "settings."),
                Mechanism::Hibernation,
                {},
                /*recommended=*/false, /*measurable=*/true,
            },
            {
                QStringLiteral("deep.windows_disk_cleanup"),
                QStringLiteral("Run Windows Disk Cleanup"),
                Category::Deep, reg::Severity::Medium,
                QStringLiteral(
                    "Hands the drive to Windows' own cleanmgr to catch residual "
                    "files the rows above miss. Your Downloads, the Recycle Bin "
                    "and any rollback image are left alone."),
                QStringLiteral(
                    "Runs in the background on its own schedule and reports "
                    "nothing back, so Vethida cannot say what it freed."),
                Mechanism::WindowsDiskCleanup,
                {},
                /*recommended=*/false, /*measurable=*/false,
            },
        };

        return kList;
    }

    const Cleaner* find(const QString& id) {
        for (const Cleaner& c : cleaners()) {
            if (c.id == id)
                return &c;
        }
        return nullptr;
    }

    QVector<const Cleaner*> cleanersFor(const QString& driveRoot) {
        const QString sys = systemDriveRoot();
        const bool isSystem = !sys.isEmpty() && !driveRoot.isEmpty()
            && driveRoot.left(1).compare(sys.left(1), Qt::CaseInsensitive) == 0;

        QVector<const Cleaner*> out;
        out.reserve(cleaners().size());
        for (const Cleaner& c : cleaners()) {
            if (isSystem || c.perVolume)
                out.push_back(&c);
        }
        return out;
    }

    QString formatBytes(quint64 bytes) {
        constexpr quint64 kKB = 1024ULL;
        constexpr quint64 kMB = kKB * 1024ULL;
        constexpr quint64 kGB = kMB * 1024ULL;
        constexpr quint64 kTB = kGB * 1024ULL;

        if (bytes >= kTB) {
            return QStringLiteral("%1 TB")
                .arg(static_cast<double>(bytes) / kTB, 0, 'f', 2);
        }
        if (bytes >= kGB) {
            return QStringLiteral("%1 GB")
                .arg(static_cast<double>(bytes) / kGB, 0, 'f', 2);
        }
        if (bytes >= kMB) {
            return QStringLiteral("%1 MB")
                .arg(static_cast<double>(bytes) / kMB, 0, 'f', 1);
        }
        if (bytes >= kKB) {
            return QStringLiteral("%1 KB")
                .arg(static_cast<double>(bytes) / kKB, 0, 'f', 0);
        }
        return QStringLiteral("%1 B").arg(bytes);
    }

    RunResult run(const QStringList& cleanerIds, const QString& driveRoot,
                  const Progress& progress) {
        RunResult r;

        QString target = driveRoot.isEmpty() ? systemDriveRoot() : driveRoot;
        if (target.size() < 2 || target.at(1) != QChar::fromLatin1(':'))
            return r;
        target = target.left(2) + QStringLiteral("\\");

        const QVector<const Cleaner*> applicable = cleanersFor(target);
        QVector<const Cleaner*> queue;
        queue.reserve(applicable.size());
        for (const Cleaner* c : applicable) {
            if (cleanerIds.contains(c->id))
                queue.push_back(c);
        }
        if (queue.isEmpty())
            return r;

        const std::wstring targetW = target.toStdWString();
        const quint64 freeBefore = freeBytesOf(targetW);
        quint64 measuredBytes = 0;

        int step = 0;
        const int steps = static_cast<int>(queue.size());
        for (const Cleaner* c : queue) {
            if (progress)
                progress(c->displayName, step, steps);
            ++step;

            const CleanResult one = clean(*c, target);
            measuredBytes  += one.bytesFreed;
            r.filesDeleted += one.filesDeleted;
            r.errors       += one.errors;
        }

        // Free space also catches what the cleaners can't measure. Something else
        // may have written to the disk meanwhile, so fall back to the measured sum.
        const quint64 freeAfter = freeBytesOf(targetW);
        r.reclaimedBytes = (freeAfter > freeBefore) ? (freeAfter - freeBefore) : measuredBytes;
        return r;
    }
}

namespace vth::win::cleanup
{
namespace
{
        WalkStats queryRecycleBinRoot(const wchar_t* root) {
            WalkStats st;
            SHQUERYRBINFO info{};
            info.cbSize = sizeof(info);
            if (FAILED(SHQueryRecycleBinW(root, &info))) return st;
            st.bytes         = static_cast<quint64>(info.i64Size);
            st.filesAffected = static_cast<int>(info.i64NumItems);
            return st;
        }
}
}

