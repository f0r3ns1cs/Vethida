#pragma once

#include <QColor>
#include <QPoint>
#include <QString>
#include <QWidget>
#include <QVector>

class QEnterEvent;
class QEvent;
class QMouseEvent;

namespace vth::widgets
{
    QColor storageSegmentColor(const QString& name, int index, const QColor& preferred = {});

    class StorageHeatmap final : public QWidget
    {
        Q_OBJECT

    public:
        struct Segment
        {
            QString name;
            quint64 bytes = 0;
            QColor color;
        };

        explicit StorageHeatmap(QWidget* parent = nullptr);

        void setSegments(QVector<Segment> segments, quint64 totalBytes);
        void clear();

        QSize minimumSizeHint() const override;
        QSize sizeHint() const override;

    protected:
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        void showToolTipAt(const QPoint& position);
        int segmentAt(const QPoint& position) const;
        QString toolTipTextAt(const QPoint& position) const;
        QString summaryText() const;

        QVector<Segment> m_segments;
        quint64 m_totalBytes = 0;
    };
}
