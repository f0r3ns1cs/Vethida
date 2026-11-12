#pragma once

#include <QString>
#include <QVector>
#include <QWidget>

#include <functional>

class QHBoxLayout;
class QPropertyAnimation;
class QShowEvent;
class QStackedWidget;

namespace vth::widgets
{
    // A horizontal tab strip over a full-width pane.
    class TabStrip : public QWidget
    {
        Q_OBJECT

    public:
        using Factory = std::function<QWidget*(QWidget*)>;
        enum class PaneScroll { Managed, Self };

        explicit TabStrip(QWidget* parent = nullptr);

        // Builds the opening tab before the strip is first shown.
        void prewarm() { ensureBuilt(currentIndex()); }

        int addTab(const QString& iconName, const QString& title, Factory factory,
                   PaneScroll scroll = PaneScroll::Self);

        void setCurrentIndex(int index);
        int  currentIndex() const;
        int  count() const;

    Q_SIGNALS:
        void currentChanged(int index);

    protected:
        // The opening tab builds here, because the strip is assembled during page
        // construction.
        void showEvent(QShowEvent* e) override;
        bool eventFilter(QObject* watched, QEvent* event) override;

    private:
        class Tab;

        struct Pane
        {
            Factory  factory;
            QWidget* host  = nullptr;
            bool     built = false;
        };

        void ensureBuilt(int index);
        // The selected tab's underline, which slides to the tab it moves to.
        void moveInk(bool animate);

        QHBoxLayout*    tabLay_ = nullptr;
        QWidget*        bar_    = nullptr;
        QWidget*        ink_    = nullptr;
        QPropertyAnimation* inkAnim_ = nullptr;
        QStackedWidget* stack_  = nullptr;
        QVector<Tab*>   tabs_;
        QVector<Pane>   panes_;
        bool            firstShown_ = false;
    };
}
