#pragma once

#include <QString>
#include <QVector>
#include <QWidget>

#include <functional>

#include "../../vth_core/Models/ProductTier.h"
// Full definition, not a forward declaration: PaneScroll is a nested enum and
// the alias below needs the complete type.
#include "../Widgets/SectionNav.h"

class QShowEvent;
class QVBoxLayout;

namespace vth::ui
{
    // page header + a card with the section rail and the current pane (built lazily)
    class HubPage : public QWidget
    {
    public:
        using Factory = std::function<QWidget*(QWidget*)>;

        HubPage(const QString& title, const QString& subtitle, QWidget* parent = nullptr);

        using PaneScroll = vth::widgets::SectionNav::PaneScroll;

        // scroll says who owns this pane's scrolling. See SectionNav::PaneScroll. Returns
        // the rail section index.
        int  addPageCard(const QString& iconName, const QString& title,
                         const QString& description, Factory factory,
                         vth::models::ProductTier minTier = vth::models::ProductTier::None,
                         PaneScroll scroll = PaneScroll::Self);

        // Content between the header and the rail card: a page-level notice that
        // belongs to no single section (elevation state, restart-pending).
        void addBodyWidget(QWidget* widget);

        int  currentSection() const;
        // Selects a section and builds its pane if this is the first time, so a
        // caller can reach into the pane immediately afterwards.
        void setCurrentSection(int index);

    protected:
        // build the first section when shown, not at construction
        void showEvent(QShowEvent* e) override;

    private:
        struct Entry
        {
            Factory  factory;
            // None means the section has no tier requirement.
            vth::models::ProductTier minTier = vth::models::ProductTier::None;
            QWidget* host  = nullptr;
            bool     built = false;
        };

        // Entry order is rail order: every entry takes exactly one rail slot.
        void ensureBuilt(int index);

        vth::widgets::SectionNav* nav_ = nullptr;
        QVBoxLayout*   bodyLay_        = nullptr;
        QVector<Entry> entries_;
        bool           firstShown_     = false;
    };
}
