#pragma once

#include <QtCore/QAbstractItemModel>
#include <QtCore/QStringList>
#include <QtGui/QAction>
#include <QtGui/QClipboard>
#include <QtGui/QGuiApplication>
#include <QtGui/QKeySequence>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMenu>
#include <QtWidgets/QTableView>
#include <QtWidgets/QTreeView>

#include <functional>

namespace vth::ui::listmenu
{
    struct Hooks
    {
        std::function<void()> onQuickFilter;
    };

    // Owns the shortcuts. The context menu reuses these actions, because two QActions
    // with one shortcut on a widget make it ambiguous and Qt then fires neither.
    inline void installShortcuts(QAbstractItemView* view, Hooks hooks = {})
    {
        if (view->selectionMode() != QAbstractItemView::NoSelection) {
            auto* copy = new QAction(QObject::tr("Copy"), view);
            copy->setShortcut(QKeySequence::Copy);
            copy->setShortcutContext(Qt::WidgetShortcut);
            QObject::connect(copy, &QAction::triggered, view, [view] {
                if (!view->selectionModel() || !view->model())
                    return;
                const auto rows = view->selectionModel()->selectedRows();
                if (rows.isEmpty())
                    return;
                // Every visible column, tab separated. QAbstractItemView has no header
                // accessor, so the concrete type is recovered here.
                const QHeaderView* header = nullptr;
                if (const auto* tree = qobject_cast<const QTreeView*>(view))
                    header = tree->header();
                else if (const auto* table = qobject_cast<const QTableView*>(view))
                    header = table->horizontalHeader();

                const int columns = view->model()->columnCount();
                QStringList lines;
                lines.reserve(rows.size());
                for (const auto& row : rows) {
                    QStringList cells;
                    cells.reserve(columns);
                    for (int c = 0; c < columns; ++c) {
                        if (header && header->isSectionHidden(c))
                            continue;
                        cells.append(row.sibling(row.row(), c).data(Qt::DisplayRole).toString());
                    }
                    lines.append(cells.join(QLatin1Char('\t')));
                }
                QGuiApplication::clipboard()->setText(lines.join(QLatin1Char('\n')));
            });
            view->addAction(copy);
        }

        if (hooks.onQuickFilter) {
            auto* filter = new QAction(QObject::tr("Quick filter..."), view);
            filter->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_F));
            filter->setShortcutContext(Qt::WidgetShortcut);
            auto fn = hooks.onQuickFilter;
            QObject::connect(filter, &QAction::triggered, view, [fn] { fn(); });
            view->addAction(filter);
        }
    }

    inline void appendStandardActions(QMenu* menu, QAbstractItemView* view,
                                      const QPoint&, Hooks = {})
    {
        const auto actions = view->actions();
        if (actions.isEmpty())
            return;
        for (QAction* action : actions) {
            if (action->shortcut() == QKeySequence(Qt::CTRL | Qt::Key_F))
                menu->addSeparator();
            menu->addAction(action);
        }
    }
}

namespace listmenu = vth::ui::listmenu;
