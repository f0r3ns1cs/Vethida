#pragma once

#include <QString>
#include <QtGlobal>

namespace vth::win
{

    struct OsVersion {
        quint32 major = 0;      // 10 for Win10/11
        quint32 minor = 0;
        quint32 build = 0;      // 19045 = 22H2 final, 26100 = 24H2, ...
        quint32 ubr = 0;        // Update Build Revision (the .NNNN part)
        QString productName;    // "Windows 11 Pro"
        QString displayVersion; // "24H2"
        bool    isWin11 = false;
        bool    isServer = false;
    };

    const OsVersion& osVersion();

    bool buildAtLeast(quint32 minBuild);
    bool buildAtMost(quint32 maxBuild);
    bool buildInRange(quint32 minBuild, quint32 maxBuild);

    QString prettyVersion();

}
