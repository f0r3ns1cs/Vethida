#pragma once

#include "../../vth_core/Windows/Fixes.h"

#include <QString>
#include <QVector>

namespace vth::ui::models
{
    struct FixDef
    {
        QString group;         // empty means the same group as the entry above
        QString name;
        QString description;   // what's wrong, in the reader's words
        vth::win::fixes::FixId id;
        QString confirm;       // empty means it runs on one press
        bool    alwaysAsk = false;   // asks even with confirmations turned off
    };

    // In page order, and cheapest first inside each group, so working down a group
    // is the troubleshooting order.
    const QVector<FixDef>& fixCatalog();

    const FixDef* findFix(vth::win::fixes::FixId id);
}
