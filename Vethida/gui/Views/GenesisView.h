#pragma once

#include "../../vth_core/Runtime/Fetch.h"
#include "../../vth_core/Windows/Genesis.h"

#include <QVector>
#include <QWidget>

class QLabel;
class QPushButton;
class QVBoxLayout;

namespace vth::widgets { class NameListEditor; class SwitchButton; }

namespace vth::ui
{
    class GenesisPresetTile;

    // Genesis settings (the on/off switch is on the dashboard card)
    class GenesisView : public QWidget {
        Q_OBJECT

    public:
        explicit GenesisView(QWidget* parent = nullptr);
        ~GenesisView() override;

    private:
        enum class Preset { Light, Balanced, Full, Custom };

        struct Option {
            const char*                 key = nullptr;    // genesis/opts/<key>
            bool vth::win::genesis::ActivateOptions::* field = nullptr;
            bool                        inPreset[3] = { false, false, false };
            vth::widgets::SwitchButton* toggle = nullptr;
        };

        void buildUi();
        QWidget* buildOptionGroup(const QString& icon, const QString& title,
                                  const QVector<int>& optionIndexes);
        QWidget* buildListCard(const QString& icon, const QString& title,
                               const QString& caption, QWidget* editor,
                               QWidget* footer = nullptr);
        int  addOption(const char* key, bool vth::win::genesis::ActivateOptions::* field,
                       bool light, bool balanced, bool full,
                       const QString& title, const QString& description,
                       const QString& caveat = QString());

        void applyPreset(Preset p);
        Preset currentPreset() const;
        void refreshSummary();
        void renderState();
        void persistOptions() const;
        void applyOptions(const vth::win::genesis::ActivateOptions& o);

        QVector<Option>               options_;
        // Built per option in addOption and placed by buildOptionGroup.
        QVector<QWidget*>             optionRows_;
        GenesisPresetTile*            presetTiles_[3] = { nullptr, nullptr, nullptr };

        vth::widgets::NameListEditor* closeEditor_     = nullptr;
        vth::widgets::NameListEditor* exceptionEditor_ = nullptr;
        QLabel*                       stateChip_       = nullptr;
        QLabel*                       summaryLabel_    = nullptr;
        QPushButton*                  appsSeg_         = nullptr;

        vth::win::genesis::State      state_;
        FetchToken                    loadToken_;
        FetchToken                    runningToken_;
    };
}
