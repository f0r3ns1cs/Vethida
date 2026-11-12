#include "pch.h"
#include "StorageHeatmap.h"

#include "../Theme/DesignTokens.h"

#include <QEnterEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QSizePolicy>
#include <QStringList>
#include <QToolTip>

#include <algorithm>

namespace
{
    QString formatBytes(quint64 bytes)
    {
        constexpr quint64 GiB = 1024ULL * 1024ULL * 1024ULL;
        constexpr quint64 MiB = 1024ULL * 1024ULL;

        if (bytes >= GiB) {
            return QObject::tr("%1 GB")
                .arg(static_cast<double>(bytes) / static_cast<double>(GiB), 0, 'f', 1);
        }

        if (bytes >= MiB) {
            return QObject::tr("%1 MB")
                .arg(static_cast<double>(bytes) / static_cast<double>(MiB), 0, 'f', 1);
        }

        return QObject::tr("%1 B").arg(bytes);
    }
}

namespace vth::widgets
{
    // A segment's color comes from Segment::color, because matching the display name
    // would break on one tr(). An unstated color cycles the ramp.
    QColor storageSegmentColor(const QString&, int index, const QColor& preferred)
    {
        namespace palette = vth::theme::palette;

        if (preferred.isValid())
            return preferred;

        const QVector<QColor> ramp = {
            palette::storageWindows(),
            palette::storageApps(),
            palette::storageUsers(),
            palette::storageSystemData()
        };
        return ramp.at(index % ramp.size());
    }

    StorageHeatmap::StorageHeatmap(QWidget* parent)
        : QWidget(parent)
    {
        setMouseTracking(true);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

    void StorageHeatmap::setSegments(QVector<Segment> segments, quint64 totalBytes)
    {
        m_segments = std::move(segments);
        m_totalBytes = totalBytes;
        setAccessibleDescription(summaryText());
        setToolTip(QString());
        update();
    }

    void StorageHeatmap::clear()
    {
        m_segments.clear();
        m_totalBytes = 0;
        setAccessibleDescription(QString());
        setToolTip(QString());
        update();
    }

    QSize StorageHeatmap::minimumSizeHint() const
    {
        return { 120, 18 };
    }

    QSize StorageHeatmap::sizeHint() const
    {
        return { 320, 18 };
    }

    void StorageHeatmap::enterEvent(QEnterEvent* event)
    {
        QWidget::enterEvent(event);
        showToolTipAt(event->position().toPoint());
    }

    void StorageHeatmap::leaveEvent(QEvent* event)
    {
        QToolTip::hideText();
        QWidget::leaveEvent(event);
    }

    void StorageHeatmap::mouseMoveEvent(QMouseEvent* event)
    {
        QWidget::mouseMoveEvent(event);
        showToolTipAt(event->position().toPoint());
    }

    void StorageHeatmap::paintEvent(QPaintEvent*)
    {
        namespace palette = vth::theme::palette;

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const QRectF bounds = rect().adjusted(0, 1, 0, -1);
        if (bounds.width() <= 0 || bounds.height() <= 0)
            return;

        QPainterPath clipPath;
        clipPath.addRoundedRect(bounds, 4.0, 4.0);

        painter.setPen(Qt::NoPen);
        painter.setBrush(palette::divider());
        painter.drawPath(clipPath);

        if (m_totalBytes == 0 || m_segments.isEmpty())
            return;

        painter.save();
        painter.setClipPath(clipPath);

        const int left = qCeil(bounds.left());
        const int right = qFloor(bounds.right()) + 1;
        int x = left;
        quint64 cumulativeBytes = 0;
        for (int i = 0; i < m_segments.size(); ++i) {
            const Segment& segment = m_segments.at(i);
            if (segment.bytes == 0)
                continue;

            cumulativeBytes = std::min(m_totalBytes, cumulativeBytes + segment.bytes);
            const int segmentRight = cumulativeBytes >= m_totalBytes
                ? right
                : left + qRound((right - left)
                    * (static_cast<qreal>(cumulativeBytes) / static_cast<qreal>(m_totalBytes)));
            if (segmentRight <= x)
                continue;

            const QRect segmentRect(x, qCeil(bounds.top()), segmentRight - x,
                                    qFloor(bounds.bottom()) - qCeil(bounds.top()) + 1);
            painter.fillRect(segmentRect, storageSegmentColor(segment.name, i, segment.color));
            x = segmentRight;
            if (x >= right)
                break;
        }

        painter.restore();

        painter.setPen(QPen(palette::cardBorder(), 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(clipPath);
    }

    QString StorageHeatmap::summaryText() const
    {
        QStringList lines;
        quint64 usedBytes = 0;
        for (const Segment& segment : m_segments) {
            if (segment.bytes == 0)
                continue;
            usedBytes += segment.bytes;
            lines << QStringLiteral("%1: %2").arg(segment.name, formatBytes(segment.bytes));
        }

        if (m_totalBytes > usedBytes)
            lines << tr("Free: %1").arg(formatBytes(m_totalBytes - usedBytes));

        return lines.join(QLatin1Char('\n'));
    }

    // Mirrors the paint loop, which skips zero-byte segments.
    int StorageHeatmap::segmentAt(const QPoint& position) const
    {
        if (m_totalBytes == 0 || width() <= 0 || !rect().contains(position)) return -1;

        const qreal positionRatio = static_cast<qreal>(position.x()) / static_cast<qreal>(width());
        quint64 cumulative = 0;
        int last = -1;
        for (int index = 0; index < m_segments.size(); ++index) {
            if (m_segments.at(index).bytes == 0)
                continue;
            last = index;
            cumulative += m_segments.at(index).bytes;
            const qreal endRatio = static_cast<qreal>(cumulative)
                / static_cast<qreal>(m_totalBytes);
            if (positionRatio <= endRatio) return index;
        }
        // Past the last drawn band: the paint fills that tail with the final
        // segment's color, so the hit test has to name the same one.
        return last;
    }

    QString StorageHeatmap::toolTipTextAt(const QPoint& position) const
    {
        const int index = segmentAt(position);
        if (index < 0) return {};

        const Segment& segment = m_segments.at(index);
        const double percent = m_totalBytes > 0
            ? static_cast<double>(segment.bytes) * 100.0 / static_cast<double>(m_totalBytes)
            : 0.0;
        return QStringLiteral("%1\n%2 (%3%)")
            .arg(segment.name, formatBytes(segment.bytes))
            .arg(percent, 0, 'f', 1);
    }

    void StorageHeatmap::showToolTipAt(const QPoint& position)
    {
        const QString text = toolTipTextAt(position);
        if (text.isEmpty()) {
            QToolTip::hideText();
            return;
        }
        QToolTip::showText(mapToGlobal(position), text, this);
    }
}
