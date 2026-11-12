#pragma once

#include "../../vth_core/Runtime/ServiceRegistry.h"
#include "../../vth_core/Runtime/Settings.h"

#include <QString>

// UI preferences, read live so changes apply without a restart
namespace vth::prefs
{
    namespace key
    {
        inline const QString askLess        = QStringLiteral("app/askLess");
        inline const QString confirmRisky   = QStringLiteral("app/confirmRisky");
        inline const QString showTips       = QStringLiteral("app/showTips");
        inline const QString alertOnFinish  = QStringLiteral("app/alertOnFinish");
        inline const QString reduceMotion   = QStringLiteral("app/reduceMotion");
        inline const QString restoreLastPage = QStringLiteral("app/restoreLastPage");
    }

    inline bool read(const QString& k, bool fallback)
    {
        auto* s = vth::ServiceRegistry::instance().get<vth::Settings>();
        return s ? s->value(k, fallback).toBool() : fallback;
    }

    // Skip the confirmation on anything that can be undone. Permanent changes,
    // deletions and restarts still ask.
    inline bool askLess()         { return read(key::askLess, false); }
    // Ask before the few tweaks marked askFirst. One-way tweaks ask regardless.
    inline bool confirmRisky()    { return read(key::confirmRisky, true); }
    // The one-time notes some tabs show on first visit.
    inline bool showTips()        { return read(key::showTips, true); }
    // Flash the taskbar button when a result arrives while Vethida is behind
    // another window.
    inline bool alertOnFinish()   { return read(key::alertOnFinish, true); }
    // Vethida's own animations off, whatever Windows is set to.
    inline bool reduceMotion()    { return read(key::reduceMotion, false); }
    // Open on the page that was showing when Vethida last closed.
    inline bool restoreLastPage() { return read(key::restoreLastPage, true); }
}
