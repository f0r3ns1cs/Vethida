#pragma once

#include <QString>
#include <QStringView>

// Services Vethida will not silently disable, and why.

namespace vth::win::services
{
    enum class Protection {
        None,       // not on the list
        Risky,      // breaks a feature; warn, do not refuse
        Critical,   // breaks the system; refuse
    };

    Protection protectionOf(QStringView serviceName);

    // True only for Critical. This is what the tweak engine gates on.
    bool isProtected(QStringView serviceName);

    // Empty when the service is not listed at all.
    QString protectedReason(QStringView serviceName);
}
