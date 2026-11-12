#pragma once

#include <QWidget>

class QTimer;
class QVariantAnimation;

namespace vth::widgets
{
    class HealthScoreGauge final : public QWidget
    {
        Q_OBJECT

    public:
        explicit HealthScoreGauge(QWidget* parent = nullptr);

        void setScore(int score);
        // A spinner replaces the value until the next setScore, so the gauge never
        // shows a score that wasn't measured.
        void setPending();

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    protected:
        void paintEvent(QPaintEvent* event) override;
        void showEvent(QShowEvent* event) override;
        void hideEvent(QHideEvent* event) override;

    private:
        int m_score = 38;
        // What the arc and the numeral show, counting toward m_score when it changes.
        qreal m_shown = 38.0;
        QVariantAnimation* m_countUp = nullptr;
        bool m_pending = false;
        int m_spinAngle = 0;
        QTimer* m_spinTimer = nullptr;
    };
}
