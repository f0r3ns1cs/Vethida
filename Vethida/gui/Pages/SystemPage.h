#pragma once

#include <QString>
#include <QWidget>

class QShowEvent;

namespace vth::widgets { class PageHeader; class TabStrip; }

namespace vth::ui
{
    class TweakController;

    // The System tab: repair this machine, get and remove apps, control what
    // starts with it, and pick a power plan.
    class SystemPage final : public QWidget
    {
        Q_OBJECT

    public:
        int  lastTab_ = 0;
        void onTabChanged(int index);
        // Builds and styles the page while it is hidden, so the first
        // visit does not pay for it. MainWindow calls it when idle.
        Q_INVOKABLE void prewarm();
        // The controller is passed through to the Fixes pane.
        explicit SystemPage(TweakController* controller, QWidget* parent = nullptr);

    protected:
        void showEvent(QShowEvent* e) override;

    private:
        void buildContent();

        vth::widgets::TabStrip*     tabs_     = nullptr;
        vth::widgets::PageHeader*  header_   = nullptr;
        TweakController*           controller_ = nullptr;   // shared; owned by MainWindow
        bool built_ = false;
    };
}

