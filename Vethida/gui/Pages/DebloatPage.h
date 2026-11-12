#pragma once

#include <QString>
#include <QWidget>

class QLabel;
class QShowEvent;

namespace vth::tweaks { struct ApplyOutcome; }

namespace vth::ui
{
    class TweakController;

    // The Debloat tab: turning off background features, and removing what came
    // preinstalled.
    class DebloatPage final : public QWidget
    {
        Q_OBJECT

    public:
        // Builds and styles the page while it is hidden, so the first
        // visit does not pay for it. MainWindow calls it when idle.
        Q_INVOKABLE void prewarm();
        explicit DebloatPage(TweakController* controller, QWidget* parent = nullptr);

    protected:
        // Built on first show: a pane is one widget per tweak and this page is
        // constructed during startup, long before anyone looks at it.
        void showEvent(QShowEvent* e) override;

    private:
        void buildContent();

        // Shown once a change lands that only takes effect after a restart.
        QLabel* rebootNote_ = nullptr;
        TweakController* controller_ = nullptr;   // shared; owned by MainWindow
        bool built_ = false;
        bool detected_ = false;
    };
}
