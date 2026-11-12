#pragma once

#include <QVector>
#include <QWidget>

#include "../../vth_core/Models/ProductTier.h"

class QFrame;
class QScrollArea;
class QStackedWidget;
class QLayout;
class QVBoxLayout;

namespace vth::widgets
{
    // vertical section list (icon, title, subtitle) next to a stack, inside a card
    class SectionNav : public QWidget
    {
        Q_OBJECT

    public:
        // Who scrolls a pane. The rail owning the scrolling stops panes nesting scroll
        // areas inside each other.
        enum class PaneScroll { Managed, Self };

        // The inset a Self pane owes itself, the one the rail gives a Managed pane.
        static void applySelfPaneInset(QLayout* layout);

        explicit SectionNav(QWidget* parent = nullptr);

        // minTier gates the section. Below it the item is grayed and can't be selected.
        int  addSection(const QString& iconName, const QString& title,
                        const QString& subtitle, QWidget* pane,
                        PaneScroll scroll = PaneScroll::Self,
                        vth::models::ProductTier minTier
                            = vth::models::ProductTier::Free);

        // Live subtitle text, for a rail whose caption reflects state (e.g. the
        // tweak tab's enabled count) rather than a fixed description.
        void setSubtitle(int index, const QString& text);

        void setCurrentIndex(int index);
        int  currentIndex() const;
        int  count() const;

    Q_SIGNALS:
        void currentChanged(int index);

    protected:
        // The rail's content settles long after the rail is built.
        bool eventFilter(QObject* watched, QEvent* event) override;

    private:
        class RailItem;

        // Give the rail column the height its items actually occupy. Nothing
        // scrolls it, so this height is what is on screen.
        void updateRailCap();

        QVBoxLayout*       railLay_ = nullptr;
        // The rail column, fixed at its own content height so it neither runs
        // the height of the window nor scrolls inside it.
        QWidget*           railHost_ = nullptr;
        QStackedWidget*    stack_   = nullptr;
        QVector<RailItem*> items_;
    };

    // The card frame behind the rail, its divider and the pane stack.
    QFrame* sectionNavCard(SectionNav* nav, QWidget* parent);

    // sectionNavCard plus the page inset, for a page whose header sits outside the
    // margins. Add the result with a stretch of 1.
    QWidget* sectionNavHost(SectionNav* nav, QWidget* parent);
}
