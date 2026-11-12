#include "NvidiaAnsel.h"

#include <QProcess>
#include <QString>

#include <windows.h>

namespace vth::win::nvidia
{
    namespace
    {
        // Absolute path, because this runs elevated.
        QString system32(const wchar_t* relative) {
            wchar_t dir[MAX_PATH] = {};
            const UINT n = GetSystemDirectoryW(dir, MAX_PATH);
            if (n == 0 || n >= MAX_PATH) return QString::fromWCharArray(relative);
            return QString::fromWCharArray(dir) + QLatin1Char('\\')
                 + QString::fromWCharArray(relative);
        }

        void runPowerShell(const QString& script) {
            QProcess p;
            p.setProgram(system32(L"WindowsPowerShell\\v1.0\\powershell.exe"));
            p.setArguments({
                QStringLiteral("-NoProfile"),
                QStringLiteral("-NonInteractive"),
                QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"),
                QStringLiteral("-Command"), script,
            });
            p.start();
            if (!p.waitForStarted(5000)) return;
            p.waitForFinished(15000);
        }
    }

    void toggleAnsel(bool disable) {
        const QString mode = disable ? QStringLiteral("off") : QStringLiteral("on");
        runPowerShell(QStringLiteral(
            "Get-ChildItem -Path \"$env:SystemRoot\\System32\\DriverStore\\FileRepository\" "
            "-Recurse -Filter 'NvCameraEnable.exe' -ErrorAction SilentlyContinue | "
            "ForEach-Object { & $_.FullName %1 }").arg(mode));
    }
}
