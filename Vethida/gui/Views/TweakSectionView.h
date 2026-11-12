#pragma once

// For BulkReport, which is passed by const reference through a signal connected here.
#include "../Pages/TweakController.h"

#include <QString>
#include <QWidget>

class QShowEvent;

namespace vth::tweaks { struct TweakSpec; }

namespace vth::ui
{
    class TweakCategoryPage;

    // A single tweak category as an embeddable view: the virtualized row list bound to
    // the shared TweakController. Shows a category outside the main Tweaks page.
    class TweakSectionView final : public QWidget
    {
        Q_OBJECT

    public:
        // Whether this view draws its own card.
        enum class Chrome { Card, None };

        // When idPrefix is set, tweaks are selected by id prefix (e.g. "bo.") instead of
        // category, for a curated group whose members share a broad category.
        explicit TweakSectionView(TweakController* controller,
                                  const QString& category, const QString& title,
                                  QWidget* parent = nullptr,
                                  const QString& idPrefix = QString(),
                                  Chrome chrome = Chrome::Card);

        // Text shown when the category holds no tweaks at all. Without it an
        // empty category renders as a blank panel with no explanation.
        void setPlaceholder(const QString& text);

        // Offers the bulk band for this category. Passed through rather than derived,
        // because a view built from an id prefix shows a curated slice.
        void setBulkCategory(const QString& category);

    protected:
        void showEvent(QShowEvent* e) override;

    private:
        void showDetails(const QString& id);
        void report(const BulkReport& r);

        TweakController*   controller_ = nullptr;   // shared; owned by MainWindow
        TweakCategoryPage* page_ = nullptr;
        // The category the bulk band acts on, so the shared controller's one
        // signal can be filtered down to the view that asked for it.
        QString            bulkCategory_;
        bool               shownOnce_ = false;
    };
}
