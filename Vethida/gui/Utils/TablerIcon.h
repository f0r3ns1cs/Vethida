#pragma once

#include <QColor>
#include <QIcon>
#include <QPixmap>
#include <QString>

#include <functional>

class QToolButton;
class QWidget;

namespace vth::icons
{
    constexpr int DefaultIconSize = 18;
    constexpr int SidebarIconSize = 21;

    // A color asked for at bake time, so a baked glyph survives a theme switch.
    using ColorFn = std::function<QColor()>;

    QIcon icon(const QString& name, const QColor& color, int size = DefaultIconSize);

    QIcon iconBadge(const QString& name, int badgePx, bool locked = false);

    // The one way to put a glyph into a QLabel.
    QPixmap pixmapFor(const QIcon& glyph, int size, const QWidget* on);

    // The common case: build the glyph and ask for it in one call.
    QPixmap glyphPixmap(const QString& name, const QColor& color, int size,
                        const QWidget* on);

    // A rail badge is a disc with a centered glyph and, when locked, a corner lock.
    constexpr int   RailDiscSize        = 40;
    constexpr qreal RailGlyphRatio      = 0.65;
    constexpr qreal RailLockRatio       = 0.48;
    constexpr qreal RailLockGlyphRatio  = 0.64;
    constexpr int   RailGlyphSize       = int(RailDiscSize * RailGlyphRatio + 0.5);

    // Takes a ColorFn so it can re-bake when the application palette changes.
    void applyIconButton(QToolButton* button, const QString& name,
                         ColorFn base, int size = DefaultIconSize);

    // Drops every baked icon, so no QPixmap outlives QApplication.
    void clearCache();

    QColor iconColor();
    QColor disabledIconColor();
}
