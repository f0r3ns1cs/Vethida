#include "pch.h"
#include "HealthScoreGauge.h"

#include "../Theme/DesignTokens.h"
#include "../Theme/Theme.h"
#include "Spinner.h"

#include "../../vth_core/Windows/HealthScore.h"

#include <QBrush>
#include <QConicalGradient>
#include <QFont>
#include <QFontMetricsF>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QRectF>
#include <QSizePolicy>
#include <QTimer>
#include <QVariantAnimation>

#include <cmath>

namespace vth::widgets
{
    namespace
    {
        constexpr int kScoreMax = 100;

        constexpr int kCountUpMs = 900;
    }

    HealthScoreGauge::HealthScoreGauge(QWidget* parent)
        : QWidget(parent)
    {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    }

    void HealthScoreGauge::setScore(int score)
    {
        if (m_spinTimer)
            m_spinTimer->stop();
        const bool wasPending = m_pending;
        m_pending = false;
        const int target = qBound(0, score, kScoreMax);
        // The first reading counts up from zero. An unchanged score doesn't replay it.
        if (target == m_score && !wasPending) {
            update();
            return;
        }
        m_score = target;
        const qreal from = wasPending ? 0.0 : m_shown;
        if (!m_countUp) {
            m_countUp = new QVariantAnimation(this);
            m_countUp->setEasingCurve(QEasingCurve::OutCubic);
            connect(m_countUp, &QVariantAnimation::valueChanged, this,
                    [this](const QVariant& v) { m_shown = v.toReal(); update(); });
        }
        m_countUp->stop();
        m_countUp->setDuration(vth::theme::animationDuration(kCountUpMs));
        if (m_countUp->duration() == 0 || !isVisible()) {
            m_shown = m_score;
            update();
            return;
        }
        m_countUp->setStartValue(from);
        m_countUp->setEndValue(static_cast<qreal>(m_score));
        m_countUp->start();
    }

    void HealthScoreGauge::setPending()
    {
        m_pending = true;
        if (!m_spinTimer) {
            m_spinTimer = new QTimer(this);
            m_spinTimer->setInterval(Spinner::FrameMs);
            connect(m_spinTimer, &QTimer::timeout, this, [this]() {
                m_spinAngle = (m_spinAngle + Spinner::StepDegrees) % 360;
                update();
            });
        }
        if (isVisible())
            m_spinTimer->start();
        update();
    }

    void HealthScoreGauge::showEvent(QShowEvent* event)
    {
        QWidget::showEvent(event);
        if (m_pending && m_spinTimer)
            m_spinTimer->start();
    }

    void HealthScoreGauge::hideEvent(QHideEvent* event)
    {
        QWidget::hideEvent(event);
        if (m_spinTimer)
            m_spinTimer->stop();
    }

    QSize HealthScoreGauge::sizeHint() const
    {
        return { 212, 176 };
    }

    QSize HealthScoreGauge::minimumSizeHint() const
    {
        return { 120, 108 };
    }

    void HealthScoreGauge::paintEvent(QPaintEvent*)
    {
        namespace palette = vth::theme::palette;

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::TextAntialiasing, true);

        // the arc is ~0.871 * side tall (depends on the end angle below)
        const qreal side = qMin<qreal>(width() - 2.0, height() / 0.871);
        const qreal arcStroke = side * 0.12;
        const qreal cap = arcStroke / 2.0;
        const QRectF arcRect((width() - side) / 2.0 + cap, cap, side - arcStroke, side - arcStroke);

        const QPointF center = arcRect.center();
        const qreal w = arcRect.width();

        // A 270-degree sweep with its gap centered on the bottom. Qt measures
        // counter-clockwise from three o'clock, so 225 is the lower-left end.
        constexpr qreal kStartDeg = 225.0;
        constexpr qreal kSweepDeg = 270.0;
        constexpr int startAngle = static_cast<int>(kStartDeg) * 16;
        constexpr int sweepAngle = -static_cast<int>(kSweepDeg) * 16;

        QPen trackPen(palette::cardBorderEmphasis(), arcStroke, Qt::SolidLine, Qt::RoundCap);
        painter.setPen(trackPen);
        painter.drawArc(arcRect, startAngle, sweepAngle);

        // A gradient along the arc's own length, full accent where the score lands.
        const qreal swept =
            m_pending ? 0.0 : kSweepDeg * (m_shown / static_cast<qreal>(kScoreMax));
        if (swept > 0.0) {
            const qreal endDeg = std::fmod(kStartDeg - swept + 360.0, 360.0);
            QConicalGradient value(center, endDeg);
            value.setColorAt(0.0, palette::accentOnSurface());
            value.setColorAt(swept / 360.0, palette::gaugeArcStart());
            value.setColorAt(1.0, palette::accentOnSurface());
            painter.setPen(QPen(QBrush(value), arcStroke, Qt::SolidLine, Qt::RoundCap));
            painter.drawArc(arcRect, startAngle, -static_cast<int>(swept * 16.0));
        }

        // Both on the ring's center, so nothing jumps when the score replaces the spinner.
        if (m_pending) {
            const qreal d = w * 0.28;
            const QRectF spinBox(center.x() - d / 2.0, center.y() - d / 2.0, d, d);
            Spinner::paintArc(painter, spinBox, m_spinAngle,
                              palette::accentOnSurface(), 3.0);
        } else {
            namespace type   = vth::theme::type;
            namespace tokens = vth::theme::tokens;

            QFont scoreFont = font();
            scoreFont.setPixelSize(type::Readout);
            scoreFont.setWeight(QFont::Weight(type::WeightStrong));

            QFont scaleFont = font();
            scaleFont.setPixelSize(type::Title);
            scaleFont.setWeight(QFont::Weight(type::WeightMedium));

            QFont bandFont = font();
            bandFont.setPixelSize(type::Title);
            bandFont.setWeight(QFont::Weight(type::WeightMedium));

            const QFontMetricsF scoreMetrics(scoreFont);
            const QFontMetricsF scaleMetrics(scaleFont);
            const QFontMetricsF bandMetrics(bandFont);

            const QString number = QString::number(qRound(m_shown));
            const QString scale  = tr("of %1").arg(kScoreMax);
            const QString band   = vth::win::health::scoreBand(m_score);

            // Cap height and baselines, because a digit has no descender and a line box
            // would leave the ink sitting high.
            const qreal numberCap = scoreMetrics.capHeight();
            const qreal bandCap   = bandMetrics.capHeight();
            const qreal gap       = tokens::CardMargin;
            const qreal top       = center.y() - (numberCap + gap + bandCap) / 2.0;

            const auto run = [&](const QFont& f, const QColor& color,
                                 const QString& text, qreal atX, qreal baseline) {
                painter.setFont(f);
                painter.setPen(color);
                painter.drawText(QPointF(atX, baseline), text);
            };

            // The value and the scale share a baseline and center as one run.
            const qreal numberWidth   = scoreMetrics.horizontalAdvance(number);
            const qreal spaceWidth    = scaleMetrics.horizontalAdvance(QLatin1Char(' '));
            const qreal scaleWidth    = scaleMetrics.horizontalAdvance(scale);
            const qreal valueBaseline = top + numberCap;
            const qreal valueLeft =
                center.x() - (numberWidth + spaceWidth + scaleWidth) / 2.0;

            run(scoreFont, palette::textPrimary(), number, valueLeft, valueBaseline);
            run(scaleFont, palette::textMuted(), scale,
                valueLeft + numberWidth + spaceWidth, valueBaseline);
            run(bandFont, palette::textSecondary(), band,
                center.x() - bandMetrics.horizontalAdvance(band) / 2.0,
                valueBaseline + gap + bandCap);
        }
    }
}
