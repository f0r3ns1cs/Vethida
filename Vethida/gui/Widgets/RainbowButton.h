#pragma once

#include <QPushButton>

class QTimer;

namespace vth::widgets
{
    // A dark pill with an animated rainbow border and a soft glow, for the action that
    // goes further than anything else on its page.
    class RainbowButton final : public QPushButton
    {
    public:
        explicit RainbowButton(const QString& text, QWidget* parent = nullptr);

        void setLoading(bool loading);
        bool isLoading() const { return m_loading; }

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override { return sizeHint(); }

    protected:
        void paintEvent(QPaintEvent* event) override;
        void changeEvent(QEvent* event) override;
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;

    private:
        void syncTimer();

        QTimer* m_timer     = nullptr;
        qreal   m_phase     = 0.0;   // border rotation, degrees
        int     m_spinAngle = 0;
        bool    m_hover     = false;
        bool    m_loading   = false;
        bool    m_savedEnabled = true;
    };
}
