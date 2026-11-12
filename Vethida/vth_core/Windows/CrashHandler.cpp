#include "CrashHandler.h"

#include "../Runtime/Paths.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QString>

#include <windows.h>
#include <dbghelp.h>

#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cwchar>
#include <exception>
#include <new>

#pragma comment(lib, "dbghelp.lib")

namespace vth::win
{
    namespace
    {
        // Everything in this namespace runs during a crash, so it can't allocate or
        // use Qt, and it stays light on stack because of stack overflows.
        constexpr int kPathMax = 1024;

        wchar_t gDumpDir[kPathMax]    = {};   // "...\crashes"
        wchar_t gBreadcrumb[kPathMax] = {};   // "...\crashes\last-crash.txt"
        wchar_t gDumpPath[kPathMax]   = {};
        LPTOP_LEVEL_EXCEPTION_FILTER gPrevFilter = nullptr;
        _purecall_handler            gPrevPurecall = nullptr;
        _invalid_parameter_handler   gPrevInvalidParam = nullptr;
        std::terminate_handler       gPrevTerminate = nullptr;
        bool  gInstalled = false;
        LONG  gDumping = 0;   // one dump per process

        void appendW(wchar_t* dst, size_t cap, const wchar_t* src)
        {
            const size_t used = wcsnlen(dst, cap);
            if (used + 1 >= cap) return;
            wcsncat_s(dst, cap, src, cap - used - 1);
        }

        // "vethida-20260726-142233-8421.dmp"
        void buildDumpPath(wchar_t* out, size_t cap)
        {
            SYSTEMTIME st{};
            GetSystemTime(&st);
            wchar_t stamp[64] = {};
            swprintf_s(stamp, L"\\vethida-%04u%02u%02u-%02u%02u%02u-%lu.dmp",
                       st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond,
                       GetCurrentProcessId());
            out[0] = L'\0';
            appendW(out, cap, gDumpDir);
            appendW(out, cap, stamp);
        }

        struct DumpArgs
        {
            EXCEPTION_POINTERS* info   = nullptr;
            DWORD               thread = 0;
            wchar_t*            path   = nullptr;
        };

        // On its own thread, because after a stack overflow the faulting thread has
        // no usable stack left.
        DWORD WINAPI dumpThread(LPVOID param)
        {
            auto* args = static_cast<DumpArgs*>(param);

            HANDLE file = CreateFileW(args->path, GENERIC_WRITE, 0, nullptr,
                                      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (file == INVALID_HANDLE_VALUE)
                return 0;

            MINIDUMP_EXCEPTION_INFORMATION mei{};
            mei.ThreadId          = args->thread;
            mei.ExceptionPointers = args->info;
            mei.ClientPointers    = FALSE;

            // Stacks and locals without a full-memory dump.
            const MINIDUMP_TYPE type = static_cast<MINIDUMP_TYPE>(
                MiniDumpWithThreadInfo
                | MiniDumpWithUnloadedModules
                | MiniDumpWithIndirectlyReferencedMemory
                | MiniDumpWithDataSegs
                | MiniDumpWithHandleData);

            MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, type,
                              args->info ? &mei : nullptr, nullptr, nullptr);
            FlushFileBuffers(file);
            CloseHandle(file);
            return 1;
        }

        // UTF-16LE with a BOM.
        void writeBreadcrumb(const wchar_t* dumpPath)
        {
            if (gBreadcrumb[0] == L'\0')
                return;
            HANDLE file = CreateFileW(gBreadcrumb, GENERIC_WRITE, 0, nullptr,
                                      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (file == INVALID_HANDLE_VALUE)
                return;
            const unsigned char bom[2] = { 0xFF, 0xFE };
            DWORD written = 0;
            WriteFile(file, bom, 2, &written, nullptr);
            WriteFile(file, dumpPath,
                      static_cast<DWORD>(wcsnlen(dumpPath, kPathMax) * sizeof(wchar_t)),
                      &written, nullptr);
            FlushFileBuffers(file);
            CloseHandle(file);
        }

        // info is null for abort, terminate and the like, which have no exception record.
        void writeDump(EXCEPTION_POINTERS* info)
        {
            if (InterlockedCompareExchange(&gDumping, 1, 0) != 0)
                return;
            if (gDumpDir[0] == L'\0')
                return;

            buildDumpPath(gDumpPath, kPathMax);

            DumpArgs args{ info, GetCurrentThreadId(), gDumpPath };
            HANDLE thread = CreateThread(nullptr, 64 * 1024, &dumpThread, &args, 0, nullptr);
            if (thread) {
                // Bounded, so a hung dbghelp can't stop the process from dying.
                WaitForSingleObject(thread, 30000);
                CloseHandle(thread);
            } else {
                dumpThread(&args);
            }
            writeBreadcrumb(gDumpPath);
        }

        [[noreturn]] void die()
        {
            TerminateProcess(GetCurrentProcess(), 0xC0000409u);
            for (;;) {}
        }

        LONG WINAPI sehFilter(EXCEPTION_POINTERS* info)
        {
            writeDump(info);
            // Chained so a debugger can still break in.
            if (gPrevFilter)
                return gPrevFilter(info);
            return EXCEPTION_EXECUTE_HANDLER;
        }

        // qFatal and Q_ASSERT end up here.
        void onAbort(int)
        {
            writeDump(nullptr);
            die();
        }

        void onTerminate()
        {
            writeDump(nullptr);
            die();
        }

        void onPurecall()
        {
            writeDump(nullptr);
            die();
        }

        void onInvalidParameter(const wchar_t*, const wchar_t*, const wchar_t*,
                                unsigned int, uintptr_t)
        {
            writeDump(nullptr);
            die();
        }

        void onNewFailed()
        {
            writeDump(nullptr);
            die();
        }
    }

    void installCrashHandler()
    {
        if (gInstalled)
            return;

        // Resolved now, while allocation still works.
        const QString dir = QDir::toNativeSeparators(vth::paths::userCrashes());
        if (!dir.isEmpty() && dir.size() < kPathMax - 64) {
            dir.toWCharArray(gDumpDir);
            gDumpDir[dir.size()] = L'\0';

            const QString crumb =
                QDir::toNativeSeparators(vth::paths::userCrashes()
                                         + QStringLiteral("/last-crash.txt"));
            crumb.toWCharArray(gBreadcrumb);
            gBreadcrumb[crumb.size()] = L'\0';
        }

        gPrevFilter = SetUnhandledExceptionFilter(&sehFilter);

        // Otherwise abort() raises its own WER report and skips the filter above.
        _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
        signal(SIGABRT, &onAbort);

        gPrevTerminate    = std::set_terminate(&onTerminate);
        gPrevPurecall     = _set_purecall_handler(&onPurecall);
        gPrevInvalidParam = _set_invalid_parameter_handler(&onInvalidParameter);
        std::set_new_handler(&onNewFailed);

        SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

        gInstalled = true;
    }

    void uninstallCrashHandler()
    {
        if (!gInstalled)
            return;
        SetUnhandledExceptionFilter(gPrevFilter);
        gPrevFilter = nullptr;
        signal(SIGABRT, SIG_DFL);
        std::set_terminate(gPrevTerminate);
        _set_purecall_handler(gPrevPurecall);
        _set_invalid_parameter_handler(gPrevInvalidParam);
        std::set_new_handler(nullptr);
        gInstalled = false;

        if (gBreadcrumb[0] != L'\0')
            DeleteFileW(gBreadcrumb);
    }

    bool hadPreviousCrash()
    {
        return !previousCrashDumpPath().isEmpty();
    }

    QString previousCrashDumpPath()
    {
        const QString crumb = vth::paths::userCrashes()
                            + QStringLiteral("/last-crash.txt");
        QFile f(crumb);
        if (!f.exists() || !f.open(QIODevice::ReadOnly))
            return {};
        const QByteArray raw = f.readAll();
        f.close();
        if (raw.size() < 2)
            return {};
        const QString path = QString::fromUtf16(
            reinterpret_cast<const char16_t*>(raw.constData() + 2),
            (raw.size() - 2) / 2);
        return QFileInfo::exists(path) ? path : QString();
    }

    void acknowledgePreviousCrash()
    {
        QFile::remove(vth::paths::userCrashes() + QStringLiteral("/last-crash.txt"));
    }

    void pruneOldDumps(int keep)
    {
        QDir dir(vth::paths::userCrashes());
        const QFileInfoList dumps = dir.entryInfoList({ QStringLiteral("*.dmp") },
                                                      QDir::Files, QDir::Time);
        for (int i = keep; i < dumps.size(); ++i)
            QFile::remove(dumps.at(i).absoluteFilePath());
    }
}
