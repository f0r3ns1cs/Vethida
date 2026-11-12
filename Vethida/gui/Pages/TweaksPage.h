#pragma once

#include <QHash>
#include <QString>
#include <QVector>
#include <QWidget>

class QLabel;
class QShowEvent;

namespace vth::tweaks { struct TweakSpec; struct ApplyOutcome; }

namespace vth::ui
{
    class HubPage;
    class TweakController;

    // Tweaks tab, one section per category. Also picks up any category no other tab
    // claims, so new ones always show up somewhere.
    class TweaksPage final : public QWidget
    {
        Q_OBJECT

    public:
        explicit TweaksPage(TweakController* controller, QWidget* parent = nullptr);

    protected:
        void showEvent(QShowEvent* e) override;

    private:
        struct Cat
        {
            QString key;
            QString label;
            QString icon;
            QString blurb;   // rail caption: what the category covers
        };

        void buildUi();
        // Factory for a category pane, run by the shell on first selection.
        QWidget* buildCategoryPane(int cat, QWidget* parent);
        void onOperationFinished(const QString& id, const vth::tweaks::ApplyOutcome& outcome);

        QVector<Cat>                                   m_cats;
        // Parallel to m_cats. Kept only to drop categories that hold no tweaks.
        QVector<QVector<const vth::tweaks::TweakSpec*>> m_catSpecs;

        HubPage* m_hub        = nullptr;
        QLabel*  m_rebootNote = nullptr;

        TweakController* m_controller = nullptr;   // shared; owned by MainWindow
        bool m_shownOnce = false;
    };
}
