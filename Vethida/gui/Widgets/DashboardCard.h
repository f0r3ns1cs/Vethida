#pragma once

#include <QEnterEvent>
#include <QEvent>
#include <QFrame>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QStyle>

#include <functional>
#include <utility>

namespace vth::widgets
{
    // Card frame with an accent border on hover, driven by the "hovered" property.
    // The border only repaints, so nothing shifts.
    class DashboardCard final : public QFrame
    {
    public:
        explicit DashboardCard(const QString& objectName, QWidget* parent)
            : QFrame(parent)
        {
            setObjectName(objectName);
            setAttribute(Qt::WA_Hover, true);
        }

        // Makes the whole card act like a button. Give non-interactive children
        // Qt::WA_TransparentForMouseEvents so their clicks fall through to the card.
        void setClickHandler(std::function<void()> handler)
        {
            m_onClick = std::move(handler);
            if (m_onClick) {
                setCursor(Qt::PointingHandCursor);
                setFocusPolicy(Qt::StrongFocus);
            } else {
                unsetCursor();
                setFocusPolicy(Qt::NoFocus);
            }
        }

    protected:
        void enterEvent(QEnterEvent* event) override
        {
            setHovered(true);
            QFrame::enterEvent(event);
        }

        void leaveEvent(QEvent* event) override
        {
            setHovered(false);
            QFrame::leaveEvent(event);
        }

        void mousePressEvent(QMouseEvent* event) override
        {
            if (m_onClick && event->button() == Qt::LeftButton) {
                m_pressed = true;
                event->accept();
                return;
            }
            QFrame::mousePressEvent(event);
        }

        void mouseReleaseEvent(QMouseEvent* event) override
        {
            if (m_onClick && m_pressed && event->button() == Qt::LeftButton) {
                m_pressed = false;
                if (rect().contains(event->position().toPoint()))
                    m_onClick();
                event->accept();
                return;
            }
            QFrame::mouseReleaseEvent(event);
        }

        void keyPressEvent(QKeyEvent* event) override
        {
            if (m_onClick && (event->key() == Qt::Key_Space
                              || event->key() == Qt::Key_Return
                              || event->key() == Qt::Key_Enter)) {
                m_onClick();
                event->accept();
                return;
            }
            QFrame::keyPressEvent(event);
        }

    private:
        void setHovered(bool hovered)
        {
            // Only interactive (clickable) cards get the accent-border highlight;
            // a plain informational card never lights up on hover.
            if (!m_onClick)
                hovered = false;
            if (property("hovered").toBool() == hovered)
                return;
            setProperty("hovered", hovered);
            style()->unpolish(this);
            style()->polish(this);
            update();
        }

        std::function<void()> m_onClick;
        bool m_pressed = false;
    };
}
