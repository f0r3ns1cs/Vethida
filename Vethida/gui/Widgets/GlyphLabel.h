#pragma once

#include "../Utils/TablerIcon.h"

#include <QColor>
#include <QIcon>
#include <QLabel>
#include <QString>

#include <functional>

class QEvent;
class QShowEvent;

namespace vth::widgets
{
    // A label whose whole content is one Tabler glyph, which re-bakes itself
    // when the theme changes.
    class GlyphLabel final : public QLabel
    {
        Q_OBJECT

    public:
        using ColorFn = vth::icons::ColorFn;
        // For a composed badge, which rebuilds a whole icon from several palette reads.
        using IconFn = std::function<QIcon()>;

        GlyphLabel(QString iconName, ColorFn color, int size, QWidget* parent = nullptr);
        GlyphLabel(IconFn icon, int size, QWidget* parent = nullptr);

        // Swap the glyph without changing its color role or size.
        void setIconName(const QString& iconName);

        // For a glyph whose color depends on the widget's own state.
        void setColor(ColorFn color);

        // Re-evaluates the color, for a state change the palette doesn't signal.
        void refresh();

    protected:
        void changeEvent(QEvent* event) override;
        void showEvent(QShowEvent* event) override;

    private:
        void rebake();

        // Set when the palette changed while this glyph was off screen.
        bool m_stale = false;

        QString m_iconName;
        ColorFn m_color;
        // Set instead of m_color by the icon constructor.
        IconFn  m_icon;
        int     m_size;
    };
}
