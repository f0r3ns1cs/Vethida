#pragma once

#include <QString>
#include <QWidget>

class QShowEvent;

namespace vth::ui
{
    class TweakController;

    // The Network tab: the connection this PC is on, what it is capable of, and
    // what can be changed about it.
    class NetworkPage final : public QWidget
    {
        Q_OBJECT

    public:
        // Builds and styles the page while it is hidden, so the first
        // visit does not pay for it. MainWindow calls it when idle.
        Q_INVOKABLE void prewarm();
        explicit NetworkPage(TweakController* tweakController, QWidget* parent = nullptr);

    protected:
        void showEvent(QShowEvent* e) override;

    private:
        void buildContent();

        // Panes, in rail order.
        QWidget* buildConnectionPane();

        TweakController* tweakController_ = nullptr;   // shared; owned by MainWindow
        bool built_ = false;
    };
}
