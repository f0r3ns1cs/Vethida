#include "pch.h"
#include "StatusMark.h"

#include "../Theme/DesignTokens.h"
#include "../Theme/Theme.h"

#include <QEasingCurve>
#include <QEvent>
#include <QPainter>
#include <QPainterPath>
#include <QVariantAnimation>

namespace vth::widgets
{
    namespace
    {
        namespace pal    = vth::theme::palette;
        namespace tokens = vth::theme::tokens;

        constexpr int   kSize       = tokens::GlyphMd;
        constexpr int   kEntranceMs = 420;
        // The share of the entrance spent growing the disc; the stroke draws in
        // the rest.
        constexpr qreal kDiscShare  = 0.4;

        // A point a fraction of the way along a polyline.
        QPointF along(const QVector<QPointF>& pts, qreal t, int* segmentOut)
        {
            qreal total = 0;
            for (int i = 1; i < pts.size(); ++i)
                total += QLineF(pts[i - 1], pts[i]).length();
            qreal want = qBound<qreal>(0, t, 1) * total;
            for (int i = 1; i < pts.size(); ++i) {
                const qreal len = QLineF(pts[i - 1], pts[i]).length();
                if (want <= len || i == pts.size() - 1) {
                    *segmentOut = i;
                    const qreal f = len > 0 ? qMin<qreal>(want / len, 1) : 1;
                    return pts[i - 1] + (pts[i] - pts[i - 1]) * f;
                }
                want -= len;
            }
            *segmentOut = pts.size() - 1;
            return pts.last();
        }
    }

    StatusMark::StatusMark(QWidget* parent) : QWidget(parent)
    {
        setFixedSize(kSize, kSize);
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        hide();
    }

    QSize StatusMark::sizeHint() const
    {
        return { kSize, kSize };
    }

    void StatusMark::present(Kind kind, bool animate)
    {
        const bool replay = !shown_ || kind_ != kind;
        kind_ = kind;
        shown_ = true;
        show();
        if (!replay)
            return;

        if (!animate || !vth::theme::animationsEnabled()) {
            progress_ = 1.0;
            update();
            return;
        }
        if (!anim_) {
            anim_ = new QVariantAnimation(this);
            anim_->setStartValue(0.0);
            anim_->setEndValue(1.0);
            anim_->setEasingCurve(QEasingCurve::OutCubic);
            connect(anim_, &QVariantAnimation::valueChanged, this, [this](const QVariant& v) {
                progress_ = v.toReal();
                update();
            });
        }
        anim_->stop();
        anim_->setDuration(vth::theme::animationDuration(kEntranceMs));
        progress_ = 0.0;
        anim_->start();
    }

    void StatusMark::clear()
    {
        shown_ = false;
        if (anim_)
            anim_->stop();
        hide();
    }

    void StatusMark::changeEvent(QEvent* event)
    {
        QWidget::changeEvent(event);
        if (event->type() == QEvent::PaletteChange
            || event->type() == QEvent::ApplicationPaletteChange)
            update();
    }

    void StatusMark::paintEvent(QPaintEvent*)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);

        const QColor tone = kind_ == Kind::Good ? pal::scoreGoodText() : pal::criticalText();
        const QPointF c = QRectF(rect()).center();
        const qreal r = kSize / 2.0 - 0.5;

        // The disc: a soft wash of the tone, grown from the center.
        const qreal disc = qMin<qreal>(progress_ / kDiscShare, 1.0);
        QColor wash = tone;
        wash.setAlphaF(0.22f);
        p.setPen(Qt::NoPen);
        p.setBrush(wash);
        p.drawEllipse(c, r * disc, r * disc);

        // The stroke, drawn along its own path once the disc is in.
        const qreal draw = progress_ <= kDiscShare
            ? 0.0 : (progress_ - kDiscShare) / (1.0 - kDiscShare);
        if (draw <= 0.0)
            return;

        QPen pen(tone);
        pen.setWidthF(1.8);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);

        const qreal s = kSize;
        if (kind_ == Kind::Good) {
            const QVector<QPointF> check = { { s * 0.29, s * 0.52 },
                                             { s * 0.44, s * 0.67 },
                                             { s * 0.72, s * 0.36 } };
            int seg = 1;
            const QPointF end = along(check, draw, &seg);
            QPainterPath path(check[0]);
            for (int i = 1; i < seg; ++i)
                path.lineTo(check[i]);
            path.lineTo(end);
            p.drawPath(path);
        } else {
            // Two strokes, one after the other.
            const qreal a = qMin<qreal>(draw * 2.0, 1.0);
            const qreal b = qBound<qreal>(0.0, draw * 2.0 - 1.0, 1.0);
            const QPointF a0(s * 0.34, s * 0.34), a1(s * 0.66, s * 0.66);
            const QPointF b0(s * 0.66, s * 0.34), b1(s * 0.34, s * 0.66);
            p.drawLine(a0, a0 + (a1 - a0) * a);
            if (b > 0.0)
                p.drawLine(b0, b0 + (b1 - b0) * b);
        }
    }
}
