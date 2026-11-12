#pragma once

#include "../Theme/DesignTokens.h"

#include <QEvent>
#include <QPainter>
#include <QWidget>

namespace vth::widgets
{
    // A small rounded bar in the accent color, marking the selected item on a rail or
    // tab strip. Its owner animates the geometry, and the pill only paints.
    class AccentPill final : public QWidget
    {
    public:
        explicit AccentPill(QWidget* parent) : QWidget(parent)
        {
            setAttribute(Qt::WA_TransparentForMouseEvents, true);
        }

    protected:
        void paintEvent(QPaintEvent*) override
        {
            QPainter p(this);
            p.setRenderHint(QPainter::Antialiasing, true);
            p.setPen(Qt::NoPen);
            p.setBrush(vth::theme::palette::accentOnSurface());
            const qreal r = qMin(width(), height()) / 2.0;
            p.drawRoundedRect(QRectF(rect()), r, r);
        }

        void changeEvent(QEvent* e) override
        {
            QWidget::changeEvent(e);
            if (e->type() == QEvent::PaletteChange
                || e->type() == QEvent::ApplicationPaletteChange)
                update();
        }
    };
}
