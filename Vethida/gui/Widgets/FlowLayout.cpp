#include "pch.h"
#include "FlowLayout.h"

#include <QWidget>

namespace vth::widgets
{
    FlowLayout::FlowLayout(QWidget* parent, int hSpacing, int vSpacing)
        : QLayout(parent), hSpace_(hSpacing), vSpace_(vSpacing)
    {
        setContentsMargins(0, 0, 0, 0);
    }

    FlowLayout::~FlowLayout()
    {
        while (QLayoutItem* item = takeAt(0))
            delete item;
    }

    void FlowLayout::addItem(QLayoutItem* item) { items_.append(item); }
    int FlowLayout::count() const { return items_.size(); }
    QLayoutItem* FlowLayout::itemAt(int index) const { return items_.value(index); }

    QLayoutItem* FlowLayout::takeAt(int index)
    {
        if (index < 0 || index >= items_.size())
            return nullptr;
        return items_.takeAt(index);
    }

    int FlowLayout::heightForWidth(int width) const
    {
        return doLayout(QRect(0, 0, width, 0), true);
    }

    QSize FlowLayout::minimumSize() const
    {
        QSize size;
        for (const QLayoutItem* item : items_)
            size = size.expandedTo(item->minimumSize());
        const QMargins m = contentsMargins();
        return size + QSize(m.left() + m.right(), m.top() + m.bottom());
    }

    QSize FlowLayout::sizeHint() const { return minimumSize(); }

    void FlowLayout::setGeometry(const QRect& rect)
    {
        QLayout::setGeometry(rect);
        doLayout(rect, false);
    }

    int FlowLayout::doLayout(const QRect& rect, bool testOnly) const
    {
        const QMargins m = contentsMargins();
        const QRect area = rect.adjusted(m.left(), m.top(), -m.right(), -m.bottom());
        int x = area.x();
        int y = area.y();
        int lineHeight = 0;

        for (QLayoutItem* item : items_) {
            // Only a widget hidden on purpose is skipped. One added to a shown host
            // isn't visible until the event loop shows it.
            if (QWidget* w = item->widget();
                w && w->isHidden() && w->testAttribute(Qt::WA_WState_ExplicitShowHide))
                continue;
            const QSize hint = item->sizeHint();
            int nextX = x + hint.width() + hSpace_;
            if (nextX - hSpace_ > area.right() + 1 && lineHeight > 0) {
                x = area.x();
                y += lineHeight + vSpace_;
                nextX = x + hint.width() + hSpace_;
                lineHeight = 0;
            }
            if (!testOnly)
                item->setGeometry(QRect(QPoint(x, y), hint));
            x = nextX;
            lineHeight = qMax(lineHeight, hint.height());
        }
        return y + lineHeight - rect.y() + m.bottom();
    }
}
