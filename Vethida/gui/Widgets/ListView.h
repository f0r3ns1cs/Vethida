#pragma once

#include "../Theme/DesignTokens.h"

#include <QFrame>
#include <QHeaderView>
#include <QSize>
#include <QTreeView>
#include <QVBoxLayout>

namespace vth::widgets
{
    // The one tabular list. Every list of records is a ListCard holding a ListView.
    class ListView : public QTreeView
    {
    public:
        static constexpr int IconPx = 20;

        explicit ListView(QWidget* parent = nullptr)
            : QTreeView(parent)
        {
            setObjectName(QStringLiteral("listView"));
            setRootIsDecorated(false);
            setUniformRowHeights(true);
            setAlternatingRowColors(false);
            setSelectionBehavior(QAbstractItemView::SelectRows);
            setSelectionMode(QAbstractItemView::SingleSelection);
            setSortingEnabled(true);
            setAllColumnsShowFocus(true);
            setIconSize(QSize(IconPx, IconPx));
            setEditTriggers(QAbstractItemView::NoEditTriggers);
            setFrameShape(QFrame::NoFrame);
            // An item-stepped wheel notch jumps a whole row.
            setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
            setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
            header()->setSectionResizeMode(QHeaderView::Interactive);
            header()->setStretchLastSection(false);
            header()->setHighlightSections(false);
            header()->setCursor(Qt::PointingHandCursor);
        }
    };

    // The surface a ListView sits on. Add THIS to a layout, never the view.
    class ListCard final : public QFrame
    {
    public:
        explicit ListCard(ListView* view, QWidget* parent = nullptr)
            : QFrame(parent)
            , view_(view)
        {
            setObjectName(QStringLiteral("listCard"));
            auto* box = new QVBoxLayout(this);
            const int inset = vth::theme::tokens::ListInset;
            box->setContentsMargins(inset, inset, inset, inset);
            box->setSpacing(0);
            view_->setParent(this);
            box->addWidget(view_, 1);
        }

        ListView* view() const { return view_; }

    private:
        ListView* view_ = nullptr;
    };
}
