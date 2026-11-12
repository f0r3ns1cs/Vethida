#pragma once

#include <QString>
#include <QStringList>

// What an NVIDIA display driver install consists of. The Gfe lists are added when
// GeForce Experience and the NVIDIA App are being removed too.
namespace vth::win::nvidia::tables
{
    // HKCR ProgID prefixes. Each match's CLSID, AppID and TypeLib go with it.
    const QStringList& classRoots();
    const QStringList& classRootsGfe();

    // Matched against the text of each HKCR\CLSID registration.
    const QStringList& clsidLeftovers();
    const QStringList& clsidLeftoversGfe();
    const QStringList& clsidLeftoversBroadcast();

    // Matched against the default value of each HKCR\Interface\{guid}.
    const QStringList& interfaces();
    const QStringList& interfacesGfe();

    // Windows Installer product names, uninstalled through msiexec.
    const QStringList& installerPackages();

    // Relative to System32, SysWOW64 and Windows. A leading backslash means
    // directly in that directory.
    const QStringList& driverFiles();
    const QStringList& driverFilesGfe();
    const QStringList& driverFilesBroadcast();

    const QStringList& services();
    const QStringList& servicesGfe();
    const QStringList& servicesBroadcast();

    const QStringList& scheduledTasks();
    const QStringList& scheduledTasksGfe();
}

namespace vth::win::nvidia::tables
{
    const QStringList& servicesAudio();
}
