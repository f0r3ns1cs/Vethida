#pragma once

#include "../../vth_core/Runtime/Fetch.h"
#include "../../vth_core/Windows/Fixes.h"

#include <QHash>
#include <QSet>
#include <QString>
#include <QWidget>

class QLabel;
class QVBoxLayout;

namespace vth::widgets { class CommandButton; class ListRow; }

namespace vth::ui
{
    class TweakController;

    // One-click repairs for common Windows problems. Risky ones ask first.
    class FixesView final : public QWidget
    {
        Q_OBJECT

    public:
        // The controller lets a repair that puts a Windows setting back force a detection
        // sweep, because the tweak catalog owns some of the same state.
        explicit FixesView(TweakController* controller, QWidget* parent = nullptr);
        ~FixesView() override;

    private:
        // What a run left behind, per catalog index. Kept off the card, because cards are
        // rebuilt whenever a sweep lands.
        struct RunState
        {
            enum class Phase { Idle, Running, Done, Failed };
            Phase   phase = Phase::Idle;
            QString outcome;
        };

        // The two widgets on a card that a run writes to. Repopulated by rebuild(), so a
        // callback that outlived one finds the live pair.
        struct CardRefs
        {
            vth::widgets::ListRow*       row = nullptr;
            vth::widgets::CommandButton* run = nullptr;
        };

        void buildUi();
        // Lays the pane out from the catalog and the last sweep.
        void rebuild();
        // Re-runs detection on a worker. Dropped rather than queued while one
        // is in flight - a second sweep reads the same machine.
        void rescan();
        void runFix(int catalogIndex);
        // Puts the card for one index back in step with state_ and symptoms_,
        // without rebuilding the pane around it.
        void refreshCard(int catalogIndex);

        TweakController* controller_ = nullptr;   // shared; owned by MainWindow

        QVBoxLayout* list_ = nullptr;
        // Present symptoms by catalog index. Absent means the detector said no,
        // or this repair has no detector at all - both mean "does not float".
        QHash<int, vth::win::fixes::Symptom> symptoms_;
        QHash<int, RunState>                 state_;
        QHash<int, CardRefs>                 cards_;
        // Fixes whose result mark has already played; a rebuild shows theirs
        // complete instead of drawing it in a second time.
        QSet<int>                            announced_;
        bool       scanning_ = false;
        FetchToken sweepToken_;
    };
}
