#pragma once

#include <QString>
#include <QStringList>
#include <QStringView>

// Images Genesis never closes, freezes or renames.

namespace vth::win::processes
{
    bool isProtected(QStringView imageName);

    // Empty when the image is not on the list.
    QString protectedReason(QStringView imageName);

    // Every listed image, sorted.
    const QStringList& protectedNames();
}
