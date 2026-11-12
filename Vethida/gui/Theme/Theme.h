#pragma once

#include "../../vth_core/Windows/DarkMode.h"

#include <QHash>
#include <QString>

class QApplication;
class QMenu;

namespace vth::theme
{
    // Only dark is in use. The palette tables keep both columns.
    enum class Mode {
        Light,
        Dark
    };

    // The generated stylesheet, the proxy style and the palette. Call once, before
    // any widget exists.
    void apply(QApplication& app);

    Mode currentMode();

    // A QMenu is a native popup, so the application stylesheet doesn't reach it.
    void applyMenuChrome(QMenu* menu);

    // Every token, by name.
    QHash<QString, QString> tokenMap();

    // False when Windows' animation effects are off.
    bool animationsEnabled();

    // 0 when animations are off.
    int animationDuration(int preferredMs);
}
