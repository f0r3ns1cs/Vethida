#pragma once

#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QWidget>

#include "../../vth_core/Models/ProductTier.h"
// For BulkAction. TweakController.h only forward-declares this class, so no cycle.
#include "TweakController.h"

class QEvent;
class QLabel;
class QLineEdit;
class QShortcut;
class QToolButton;
class QResizeEvent;

namespace vth::tweaks { struct TweakSpec; }
namespace vth::widgets { class CommandButton; class SettingsList; class SettingsRow; class Spinner; class SwitchButton; }

namespace vth::ui
{
    // One category's tweaks as a settings pane: section headings, and a title over a
    // wrapping description with the switch on the right.
    class TweakCategoryPage final : public QWidget
    {
        Q_OBJECT

    public:
        explicit TweakCategoryPage(QWidget* parent = nullptr);

        void setTitle(const QString& title);
        void setTweaks(const QVector<const vth::tweaks::TweakSpec*>& specs);

        // Turns on the bulk band for this category. Off by default, because a pane
        // assembled from several categories has no single category to act on.
        void setBulkCategory(const QString& category);
        QString bulkCategory() const { return m_bulkCategory; }
        // The band is inert until the machine has been read.
        void setBulkReady(bool ready);
        void setBulkBusy(bool busy);
        // What the run in flight is writing, on the band's second line.
        void setBulkProgress(const QString& label);
        // enabled is false while any bulk run is in flight, anywhere in the app.
        // undoable is whether the last run was this category's.
        void setBulkAvailability(bool enabled, bool undoable);

        // Message shown when the list holds no tweaks at all.
        void setPlaceholder(const QString& text);

        // While set, the pane shows a centered spinner instead of rows. Driven by
        // TweakController while a detection sweep is pending.
        void setStatesLoading(bool loading);

        // Bulk initial detection: id -> DetectState int.
        void setDetected(const QHash<QString, int>& states);
        // Single authoritative update after an apply or revert.
        void setTweakState(const QString& id, int state);
        // Toggles are live only once detection has run and writes are permitted.
        void setToggleInteractive(bool interactive);
        // Current entitlement rank (Free=0/Pro=1/Ultimate=2); rows above it lock.
        void setCurrentTierRank(int rank);
        // Whether the user has opted into the early-access program. Combined with
        // Ultimate, this unlocks early-access tweaks (shown locked otherwise).
        void setEarlyAccessOptIn(bool optedIn);

    Q_SIGNALS:
        void tweakToggled(const QString& id, bool on);
        // ids = what's currently shown (after filtering), in order
        void bulkRequested(const QString& category, vth::ui::BulkAction action,
                           const QStringList& ids);
        // The category this pane acts on was set or changed.
        void bulkCategoryChanged();
        void detailsRequested(const QString& id);

    protected:
        void resizeEvent(QResizeEvent* event) override;

    private:
        // A pane is one widget per tweak, and widgets can't be built off the UI thread,
        // so rebuild() queues the work and buildChunk realizes a slice per event-loop turn.
        void rebuild();
        void buildChunk();
        void finishBuild();
        void setRowsBuilt(bool built);
        // Everything above the list.
        void applyHeaderVisibility();
        void layoutSpinner();
        void applyLoadingState();
        void applyRowState(const QString& id);
        vth::models::ProductTier tierValue() const;
        void applyFilter(const QString& query);
        // What detection last said. Unknown until a sweep has spoken for it.
        vth::tweaks::DetectState stateFor(const QString& id) const;
        // What this pane is showing right now, in display order, so the bulk band can't
        // act on a row the search has hidden.
        QStringList visibleIds() const;
        // The one place the band's three buttons are enabled or disabled.
        void applyBandState();
        void requestBulk(BulkAction action);

        struct RowEntry
        {
            const vth::tweaks::TweakSpec* spec = nullptr;
            vth::widgets::SettingsRow*    row  = nullptr;
            bool locked = false;
            // permanent tweaks get a button + applied mark instead of a switch
            vth::widgets::CommandButton* applyBtn   = nullptr;
            QLabel*                      appliedTag = nullptr;
            // metaStatic = the strip has something besides the live chip
            QWidget*                     meta       = nullptr;
            QLabel*                      stateChip  = nullptr;
            bool                         metaStatic = false;
        };

        // The caption chip for the detected state the switch can't show.
        void applyRowStateChip(const RowEntry& entry, vth::tweaks::DetectState state);

        // after RowEntry, the parameter type has to be declared first
        bool toggleEnabledFor(const RowEntry& entry) const;

        // Flattened build plan: headings and rows in display order, consumed a
        // slice at a time by buildChunk.
        struct BuildItem
        {
            const vth::tweaks::TweakSpec* spec = nullptr;   // null = section heading
            QString heading;
        };

        QVector<const vth::tweaks::TweakSpec*> m_specs;
        // id -> DetectState, as an int for the same reason the controller's is:
        // it crosses a queued connection.
        QHash<QString, int>  m_state;
        QHash<QString, int>  m_rowByIndex;   // id -> index into m_rows
        QVector<RowEntry>    m_rows;
        // Heading blocks, so a search can hide them; owned by the list.
        QVector<QWidget*>    m_headings;

        QString m_placeholder;

        bool    m_interactive = false;
        bool    m_statesLoading = false;
        // No entitlement until told otherwise, so this fails closed.
        int     m_tierRank = 0;
        bool    m_earlyAccessOptIn = false;

        QVector<BuildItem> m_plan;
        int                m_planPos = 0;
        // Bumped by every rebuild so a chunk already queued for the previous
        // plan drops itself instead of appending rows to the new one.
        quint64            m_buildToken = 0;

        // bulk action bar under the search
        QWidget*                     m_bulkRow = nullptr;
        vth::widgets::CommandButton* m_recommendBtn = nullptr;
        vth::widgets::CommandButton* m_applyAllBtn  = nullptr;
        vth::widgets::CommandButton* m_undoBtn      = nullptr;
        // The band's second line, and what it says when no run is in flight.
        QLabel*                      m_bulkNote     = nullptr;
        QString                      m_bulkNoteText;
        QString                    m_bulkCategory;
        bool                       m_bulkReady = false;
        // false while any pane has a run going (not this pane's own busy state)
        bool                       m_bulkEnabled = true;
        bool                       m_undoable    = false;
        // Rows have finished building. Not the same as "header is visible" -
        // applyHeaderVisibility() combines this with m_statesLoading.
        bool                       m_rowsBuilt      = true;
        // The first slice is on screen; the rest may still be arriving.
        bool                       m_rowsShown      = true;

        QLabel*                    m_titleLabel = nullptr;
        QLineEdit*                 m_search  = nullptr;
        // Ctrl+F / Esc (window shortcuts only fire for the visible pane)
        QShortcut*                 m_findShortcut  = nullptr;
        QShortcut*                 m_clearShortcut = nullptr;
        QLabel*                    m_noMatch = nullptr;
        vth::widgets::SettingsList* m_list = nullptr;
        vth::widgets::Spinner*      m_spinner = nullptr;
    };
}
