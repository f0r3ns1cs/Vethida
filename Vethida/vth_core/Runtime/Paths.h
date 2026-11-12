#pragma once

#include <QString>

namespace vth::paths
{

    QString installDir();

    // %LOCALAPPDATA%\Vethida
    QString userData();

    // Data an elevated Vethida acts on (the change ledger). ProgramData when elevated,
    // since anything can write to LOCALAPPDATA. userData() otherwise.
    QString protectedData();

    // Under protectedData().
    QString userLogs();
    QString userCrashes();
    // Under userData().
    QString userCache();

    QString shippedCfg();

    // True if the directory exists after the call.
    bool ensureDir(const QString& absolutePath);

}

namespace vth::paths
{
    QString shippedThemes();
    QString shippedLicenses();
    QString lgplLicenseFile();
    QString gplLicenseFile();
    QString thirdPartyNoticesFile();
}
