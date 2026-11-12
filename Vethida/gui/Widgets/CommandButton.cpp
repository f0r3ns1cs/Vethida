#include "pch.h"
#include "CommandButton.h"

#include "../Theme/DesignTokens.h"
#include "Spinner.h"

#include <QEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QTimer>

namespace vth::widgets
{
    CommandButton::CommandButton(QWidget* parent)
        : CommandButton(Variant::Secondary, QString(), parent)
    {
    }

    CommandButton::CommandButton(const QString& text, QWidget* parent)
        : CommandButton(Variant::Secondary, text, parent)
    {
    }

    CommandButton::CommandButton(Variant variant, const QString& text, QWidget* parent)
        : QPushButton(text, parent)
        , m_variant(variant)
    {
        // TabFocus, because StrongFocus leaves the focus ring lit after a click.
        setFocusPolicy(Qt::TabFocus);
        applyVariant();
        updateCursor();
    }

    void CommandButton::changeEvent(QEvent* event)
    {
        QPushButton::changeEvent(event);
        if (event->type() == QEvent::EnabledChange) {
            updateCursor();
            update();
        }
    }

    void CommandButton::setLoading(bool loading)
    {
        if (m_loading == loading)
            return;
        m_loading = loading;

        if (loading) {
            m_savedEnabled = isEnabled();   // so an already-disabled button stays disabled after
            m_savedText = text();
            m_savedMinWidth = minimumWidth();
            setMinimumWidth(width());       // hold the laid-out width so the row doesn't jump
            QPushButton::setText(QString());
            if (!m_spinTimer) {
                m_spinTimer = new QTimer(this);
                m_spinTimer->setInterval(vth::widgets::Spinner::FrameMs);
                connect(m_spinTimer, &QTimer::timeout, this, [this]() {
                    m_spinAngle = (m_spinAngle + vth::widgets::Spinner::StepDegrees) % 360;
                    update();
                });
            }
            m_spinAngle = 0;
            m_spinTimer->start();
            setEnabled(false);
        } else {
            if (m_spinTimer)
                m_spinTimer->stop();
            setEnabled(m_savedEnabled);
            QPushButton::setText(m_savedText);
            setMinimumWidth(m_savedMinWidth);
        }
        update();
    }

    void CommandButton::paintEvent(QPaintEvent* event)
    {
        // Drawn while loading too, or a disabled primary button has no fill under the arc.
        QPushButton::paintEvent(event);
        if (!m_loading)
            return;

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const qreal d = qMin(qMin(width(), height()) - 12, 20);
        const QRectF box((width() - d) / 2.0, (height() - d) / 2.0, d, d);
        vth::widgets::Spinner::paintArc(painter, box, m_spinAngle,
                                        vth::theme::palette::accentOnSurface(), 2.4);
    }

    void CommandButton::applyVariant()
    {
        QString variantName;
        switch (m_variant) {
        case Variant::Primary:   variantName = QStringLiteral("primary");   break;
        default:                 variantName = QStringLiteral("secondary"); break;
        }
        setProperty("commandVariant", variantName);
        setFlat(false);
    }

    void CommandButton::updateCursor()
    {
        if (isEnabled())
            setCursor(Qt::PointingHandCursor);
        else
            unsetCursor();
    }
}
