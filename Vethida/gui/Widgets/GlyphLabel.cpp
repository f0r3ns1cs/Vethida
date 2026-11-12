#include "pch.h"
#include "GlyphLabel.h"

#include "../Utils/TablerIcon.h"

#include <QEvent>
#include <QShowEvent>

#include <utility>

namespace vth::widgets
{
    GlyphLabel::GlyphLabel(QString iconName, ColorFn color, int size, QWidget* parent)
        : QLabel(parent)
        , m_iconName(std::move(iconName))
        , m_color(std::move(color))
        , m_size(size)
    {
        setFixedSize(m_size, m_size);
        setAlignment(Qt::AlignCenter);
        rebake();
    }

    GlyphLabel::GlyphLabel(IconFn icon, int size, QWidget* parent)
        : QLabel(parent)
        , m_icon(std::move(icon))
        , m_size(size)
    {
        setFixedSize(m_size, m_size);
        setAlignment(Qt::AlignCenter);
        rebake();
    }

    void GlyphLabel::setIconName(const QString& iconName)
    {
        if (m_iconName == iconName)
            return;
        m_iconName = iconName;
        rebake();
    }

    void GlyphLabel::setColor(ColorFn color)
    {
        m_color = std::move(color);
        rebake();
    }

    void GlyphLabel::refresh()
    {
        rebake();
    }

    void GlyphLabel::changeEvent(QEvent* event)
    {
        QLabel::changeEvent(event);
        if (event->type() == QEvent::ApplicationPaletteChange) {
            if (isVisible())
                rebake();
            else
                m_stale = true;
        }
    }

    void GlyphLabel::showEvent(QShowEvent* event)
    {
        QLabel::showEvent(event);
        if (m_stale) {
            m_stale = false;
            rebake();
        }
    }

    void GlyphLabel::rebake()
    {
        if (m_icon) {
            setPixmap(vth::icons::pixmapFor(m_icon(), m_size, this));
            return;
        }
        if (!m_color)
            return;
        setPixmap(vth::icons::glyphPixmap(m_iconName, m_color(), m_size, this));
    }
}
