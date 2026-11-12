#pragma once

#include "../Theme/DesignTokens.h"

#include <QEvent>
#include <QPainter>
#include <QRectF>
#include <QSize>
#include <QSvgRenderer>
#include <QWidget>

namespace vth::widgets
{
    // The Vethida wordmark, in whichever of the two SVGs the theme calls for.
    class Wordmark final : public QWidget
    {
    public:
        // The title-bar size, and the ratio every other size is built from.
        static constexpr int Width  = 97;
        static constexpr int Height = 29;

        explicit Wordmark(QWidget* parent, QSize size = QSize(Width, Height))
            : QWidget(parent)
            , m_dark(QStringLiteral(":/branding/wordmark_vethida_white.svg"), this)
            , m_light(QStringLiteral(":/branding/wordmark_vethida_black.svg"), this)
        {
            setFixedSize(size);
            setAccessibleName(QStringLiteral("Vethida"));
            // Left hit-testable, it would swallow title bar drags.
            setAttribute(Qt::WA_TransparentForMouseEvents);
        }

    protected:
        void paintEvent(QPaintEvent*) override
        {
            QPainter painter(this);
            painter.setRenderHint(QPainter::Antialiasing, true);
            QSvgRenderer& renderer = vth::theme::palette::isDark() ? m_dark : m_light;
            renderer.render(&painter, QRectF(rect()));
        }

        void changeEvent(QEvent* event) override
        {
            if (event->type() == QEvent::PaletteChange
                || event->type() == QEvent::ApplicationPaletteChange) {
                update();
            }
            QWidget::changeEvent(event);
        }

    private:
        QSvgRenderer m_dark;
        QSvgRenderer m_light;
    };
}
