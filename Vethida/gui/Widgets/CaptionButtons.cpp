#include "pch.h"
#include "CaptionButtons.h"

#include "../Theme/DesignTokens.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QObject>
#include <QSize>
#include <QStyle>
#include <QToolButton>

namespace vth::widgets
{
    using vth::platform::CaptionButton;

    CaptionButtons::CaptionButtons(bool withMaximize, QWidget* parent)
        : QWidget(parent)
    {
        namespace tokens = vth::theme::tokens;

        const auto make = [this](const QString& objectName, int glyphSize,
                                 const QString& label) {
            auto* button = new QToolButton(this);
            button->setObjectName(objectName);
            button->setAutoRaise(true);
            button->setIconSize(QSize(glyphSize, glyphSize));
            button->setAccessibleName(label);
            button->setToolTip(label);
            // The pointer states come from the OS hit test.
            button->setFocusPolicy(Qt::NoFocus);
            button->setCursor(Qt::ArrowCursor);
            button->setProperty("captionHover", false);
            button->setProperty("captionPress", false);
            return button;
        };

        m_minimize = make(QStringLiteral("captionMinimizeButton"),
                          tokens::CaptionGlyph, QObject::tr("Minimize"));
        if (withMaximize) {
            m_maximize = make(QStringLiteral("captionMaximizeButton"),
                              tokens::CaptionGlyph, QObject::tr("Maximize"));
        }
        m_close = make(QStringLiteral("captionCloseButton"),
                       tokens::CaptionGlyphClose, QObject::tr("Close"));

        auto* row = new QHBoxLayout(this);
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(0);
        row->addWidget(m_minimize);
        if (m_maximize)
            row->addWidget(m_maximize);
        row->addWidget(m_close);
    }

    CaptionButton CaptionButtons::buttonAt(const QPoint& pos) const
    {
        if (!rect().contains(pos))
            return CaptionButton::None;

        for (QWidget* w = childAt(pos); w && w != this; w = w->parentWidget()) {
            if (w == m_close)
                return CaptionButton::Close;
            if (w == m_maximize)
                return CaptionButton::MaximizeRestore;
            if (w == m_minimize)
                return CaptionButton::Minimize;
        }
        return CaptionButton::None;
    }

    void CaptionButtons::applyState(CaptionButton hovered, CaptionButton pressed)
    {
        const auto update = [](QToolButton* button, bool isHovered, bool isPressed) {
            if (!button)
                return;
            if (button->property("captionHover").toBool() == isHovered
                && button->property("captionPress").toBool() == isPressed) {
                return;
            }
            button->setProperty("captionHover", isHovered);
            button->setProperty("captionPress", isPressed);
            button->style()->unpolish(button);
            button->style()->polish(button);
            button->update();
        };

        update(m_minimize, hovered == CaptionButton::Minimize,
               pressed == CaptionButton::Minimize);
        update(m_maximize, hovered == CaptionButton::MaximizeRestore,
               pressed == CaptionButton::MaximizeRestore);
        update(m_close, hovered == CaptionButton::Close,
               pressed == CaptionButton::Close);
    }

    void CaptionButtons::changeEvent(QEvent* event)
    {
        QWidget::changeEvent(event);
        if (event->type() == QEvent::ApplicationPaletteChange)
            refreshIcons(m_wasActive, m_wasMaximized);
    }

    void CaptionButtons::refreshIcons(bool windowActive, bool maximized)
    {
        m_wasActive    = windowActive;
        m_wasMaximized = maximized;

        const QColor glyph = windowActive ? vth::theme::palette::textPrimary()
                                          : vth::theme::palette::textMuted();

        if (m_minimize)
            m_minimize->setIcon(vth::platform::captionGlyphIcon(CaptionButton::Minimize, glyph));
        if (m_close)
            m_close->setIcon(vth::platform::captionGlyphIcon(CaptionButton::Close, glyph));
        if (m_maximize) {
            m_maximize->setIcon(vth::platform::captionGlyphIcon(
                CaptionButton::MaximizeRestore, glyph, maximized));
            const QString label = maximized ? QObject::tr("Restore") : QObject::tr("Maximize");
            m_maximize->setAccessibleName(label);
            m_maximize->setToolTip(label);
        }
    }
}
