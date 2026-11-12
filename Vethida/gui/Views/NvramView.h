#pragma once

#include "../../vth_core/Runtime/Fetch.h"
#include "../../vth_core/Windows/Nvram.h"
#include "../../vth_core/Windows/NvramPresets.h"

#include <QVector>
#include <QWidget>

namespace vth::widgets { class CommandButton; class RainbowButton; }

class QPushButton;
class QVBoxLayout;

namespace vth::ui
{
    class EmptyState;

    // BIOS settings through AMISCE: two preset levels and a manual editor. Every write
    // shows the changes first, and the original copy is kept.
    class NvramView final : public QWidget
    {
        Q_OBJECT

    public:
        explicit NvramView(QWidget* parent = nullptr);
        ~NvramView() override;

    private:
        enum class State {
            Unsupported,   // not AMI firmware; nothing here can ever work
            NoTool,        // AMI, but AMISCE was not found
            Ready,         // the two apply buttons
        };

        void buildUi();
        void evaluateEnvironment();
        void showCentered(bool on);
        void showUnavailable(const QString& title, const QString& reason);
        void setState(State s, const QString& detail = QString());

        void onChooseToolFolder();
        void onApply(vth::win::nvram::presets::Level level);
        void onEditByHand();
        void onRestoreBaseline();

        // Every action button off while one runs, and the one that is running
        // shows it.
        void setBusy(bool busy, QPushButton* source);
        // review before writing (requireTypedWord = type a word to confirm)
        bool confirmChanges(const QString& title,
                            const QVector<vth::win::nvram::presets::Change>& changes,
                            int skipped, bool requireTypedWord);
        // true = write the changes (left in doc)
        bool editDocument(vth::win::nvram::Document& doc);
        void writePlan(const QString& path);

        State state_ = State::Ready;

        QVBoxLayout*  root_    = nullptr;
        // The unbootable-PC hazard notice. A required disclosure wherever a
        // firmware write is reachable, and hidden only where it is not.
        QWidget*      hazard_  = nullptr;
        QWidget*      notice_  = nullptr;   // the current panel
        EmptyState*   busy_    = nullptr;

        vth::widgets::CommandButton* optimalBtn_  = nullptr;
        vth::widgets::RainbowButton* allBtn_      = nullptr;
        vth::widgets::CommandButton* editBtn_     = nullptr;
        vth::widgets::CommandButton* restoreBtn_  = nullptr;

        vth::win::nvram::ScewinPaths         tool_;
        vth::win::nvram::presets::Platform   platform_ =
            vth::win::nvram::presets::Platform::Unknown;

        FetchToken token_;
    };
}
