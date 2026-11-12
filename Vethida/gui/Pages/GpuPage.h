#pragma once

#include "../../vth_core/Windows/MachineProfile.h"

#include <QString>
#include <QWidget>

class QFrame;
class QShowEvent;

namespace vth::widgets { class CommandButton; class PageHeader; class SectionNav; }

namespace vth::ui
{
    class EmptyState;
    class TweakController;

    // The GPU tab: the NVIDIA driver profile Vethida can write, the driver
    // reinstall workflow, and the graphics settings catalog.
    class GpuPage final : public QWidget
    {
        Q_OBJECT

    public:
        // Builds and styles the page while it is hidden, so the first
        // visit does not pay for it. MainWindow calls it when idle.
        Q_INVOKABLE void prewarm();
        explicit GpuPage(TweakController* controller, QWidget* parent = nullptr);

    protected:
        void showEvent(QShowEvent* e) override;

    private:
        void buildContent();
        // Everything on this page that depends on which adapter is installed.
        void applyHardware(const vth::win::machine::Profile& profile);

        QWidget* buildProfilePane();
        QWidget* buildDriverPane();
        // Undo is offered exactly while there is something to undo.
        void refreshProfileUndo();

        TweakController*             controller_ = nullptr;   // shared; owned by MainWindow
        vth::widgets::PageHeader*    header_     = nullptr;
        vth::widgets::SectionNav*    nav_        = nullptr;
        // only auto-switch to settings once
        bool                         openedOnSettings_ = false;
        vth::widgets::CommandButton* nvidiaBtn_  = nullptr;
        // Shown while Vethida holds a copy of the settings the profile replaced.
        vth::widgets::CommandButton* nvidiaUndo_ = nullptr;
        // explains why the profile button is disabled
        QFrame*                      nvidiaNote_ = nullptr;
        // The centered message each NVIDIA pane shows on a PC with another
        // maker's graphics card.
        EmptyState*                  profileMessage_ = nullptr;
        EmptyState*                  driverMessage_  = nullptr;
        bool                         built_ = false;
    };
}
