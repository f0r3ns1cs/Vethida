#pragma once

#include <QAbstractButton>

class QTimer;

namespace vth::widgets
{
    // A round on/off control: a dotted ring when off, a filled disc when on, with
    // a power mark centered in both.
    class PowerToggle final : public QAbstractButton
    {
        Q_OBJECT

    public:
        explicit PowerToggle(QWidget* parent = nullptr);

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    protected:
        void paintEvent(QPaintEvent* event) override;
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void changeEvent(QEvent* event) override;
        // Programmatic setChecked() reaches the paint through here, because callers
        // block its toggled signal.
        void checkStateSet() override;
        void showEvent(QShowEvent* event) override;
        void hideEvent(QHideEvent* event) override;

    private:
        // The live ring turns only while the toggle is on and on screen, and
        // never with Windows' animation effects off.
        void syncSpin();

        bool    m_hovered = false;
        QTimer* m_spin    = nullptr;
        qreal   m_phase   = 0.0;   // ring rotation, degrees
    };
}
