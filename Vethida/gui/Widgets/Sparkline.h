#pragma once

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtWidgets/QWidget>

#include <deque>
#include <functional>

namespace vth::ui
{
    // Rolling-window area chart drawn with QPainter. Holds up to capacity_
    // samples and draws the newest at the right edge.
    class Sparkline : public QWidget
    {
        Q_OBJECT
    public:
        explicit Sparkline(QWidget* parent = nullptr);
        ~Sparkline() override;

        // Append a new sample, clamped to [yMin_, yMax_]. The oldest sample is
        // dropped once the window exceeds capacity.
        void appendSample(double value);

        // Evenly spaced x-axis tick labels, ordered oldest (left) to newest
        // (right). When non-empty these replace the single left/right x labels.
        void setXTicks(const QStringList& labels);

        // Formatter for the value shown in the hover tooltip.
        void setValueFormatter(std::function<QString(double)> fmt);

        // Y-axis value range. Defaults to [0.0, 100.0].
        void setRange(double yMin, double yMax);

        // Maximum samples in the rolling window. Default: 60.
        void setCapacity(int n);

        QSize minimumSizeHint() const override;
        QSize sizeHint()        const override;

    protected:
        void paintEvent(QPaintEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void leaveEvent(QEvent* event) override;

    private:
        std::deque<double>          samples_;
        int                         capacity_    = 60;
        double                      yMin_        = 0.0;
        double                      yMax_        = 100.0;

        QStringList                 xTicks_;

        std::function<QString(double)> formatter_;

        bool                        hovered_     = false;
        int                         hoverX_      = -1;

        QPointF sampleToPoint(int idx, const QRectF& chartRect) const;
        QString defaultFormat(double v) const;
    };

}
