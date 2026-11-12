#pragma once

#include "Nvram.h"

#include <QString>
#include <QVector>
#include <QStringList>

// The firmware tweak list, applied to an AMISCE export.
namespace vth::win::nvram::presets
{
    enum class Platform { Unknown, Intel, Amd };
    enum class Level    { Optimal, Everything };

    // From CPUID, because the Intel and AMD lists use different question names.
    Platform detectPlatform();
    QString  platformName(Platform p);

    struct Change {
        QString setting;
        QString from;
        QString to;
        bool    dangerous = false;
    };

    struct Plan {
        QVector<Change> changes;
        int             alreadySet = 0;
        int             noSuchValue = 0;   // the board doesn't offer the target value
        int             notOnBoard = 0;    // the board doesn't have the setting
    int             listSize   = 0;
    };

    int countFor(Platform p, Level level);

    // Changes doc only. They reach the firmware through serialize() and
    // importSnapshot().
    Plan apply(Document& doc, Platform p, Level level);
}
