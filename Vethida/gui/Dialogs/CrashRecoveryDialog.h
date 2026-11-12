#pragma once

#include <QDateTime>
#include <QStringList>

class QWidget;

namespace vth::dialogs
{
    // At startup after an interrupted apply-all. Reverting the whole batch is safe,
    // reverting something that never applied does nothing.
    void showCrashRecovery(QWidget* host, const QStringList& opIds,
                           const QDateTime& startedAt);
}
