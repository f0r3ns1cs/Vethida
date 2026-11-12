#pragma once

#include <QAbstractButton>

class QVariantAnimation;

namespace vth::widgets
{
    class SwitchButton final : public QAbstractButton
    {
    public:
        explicit SwitchButton(QWidget* parent = nullptr);

        // Large pill variant that renders the ON/OFF state text inside the track,
        // for prominent feature-card toggles. Default is the compact switch.
        void setLabeled(bool labeled);
        bool isLabeled() const { return m_labeled; }

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    protected:
        void changeEvent(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
        // Programmatic setChecked() reaches the thumb through here, because callers
        // block its toggled signal.
        void checkStateSet() override;

    private:
        void updateCursor();
        // Slides the thumb toward the current checked state. Snaps instantly while
        // hidden (e.g. initial state restore) so pages don't animate on first show.
        void animateToState();

        bool m_labeled = false;
        // 0.0 = fully OFF (thumb left), 1.0 = fully ON (thumb right). Drives the
        // knob position and track color so toggling slides instead of snapping.
        qreal m_thumbPos = 0.0;
        QVariantAnimation* m_anim = nullptr;
    };
}
