#pragma once

#include <QElapsedTimer>
#include <QWidget>

class QColor;
class QPainter;
class QTimer;

namespace vth::widgets
{
    // The one indeterminate loading mark: a rotating 270-degree arc. Widgets that paint
    // the arc themselves share it through paintArc.
    class Spinner final : public QWidget
    {
    public:
        explicit Spinner(QWidget* parent = nullptr, int diameter = 36);

        static void paintArc(QPainter& painter, const QRectF& box, int angleDeg,
                             const QColor& color, qreal penWidth);

        // ~440 deg/s. For widgets that step their own angle. Spinner itself takes its
        // angle from elapsed time, so a busy UI thread makes it catch up.
        static constexpr int StepDegrees = 7;
        static constexpr int FrameMs = 16;   // ~60 fps, only while visible

    protected:
        void showEvent(QShowEvent* event) override;
        void hideEvent(QHideEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        QTimer*       timer_ = nullptr;
        QElapsedTimer clock_;
    };
}
