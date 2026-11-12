#pragma once

#include <QWidget>

#include "../../vth_core/Models/ProductTier.h"

class QFrame;
class QLabel;

namespace vth::widgets
{
    class GlyphLabel;

    // Why a destination is grayed, for the rails that draw one inert.
    QString lockedDestinationTip(vth::models::ProductTier minTier);

    // "Locked" overlay on a gated view. Blocks input while shown.
    class TierLockOverlay final : public QWidget
    {
    public:
        // What is being covered. A card has room for the heading and the action only.
        enum class Presentation
        {
            Pane,
            Card,
        };

        TierLockOverlay(QWidget* target, vth::models::ProductTier requiredTier,
                        Presentation presentation = Presentation::Pane);

        void setRequiredTier(vth::models::ProductTier tier);

        // Names the thing being gated in the sentence. Ignored by the Card form.
        void setFeatureName(const QString& name);

    protected:
        void paintEvent(QPaintEvent*) override;
        bool eventFilter(QObject* obj, QEvent* event) override;

    private:
        void cover();
        void refreshText();

        QWidget*                 target_ = nullptr;
        vth::models::ProductTier tier_;
        Presentation             presentation_ = Presentation::Pane;
        QString                  feature_;
        QFrame*                  panel_ = nullptr;
        // A GlyphLabel, because the premium gold differs per theme.
        GlyphLabel*              iconLabel_ = nullptr;
        QLabel*                  title_ = nullptr;
        // Null in the Card form. refreshText writes it only when it exists.
        QLabel*                  body_  = nullptr;
    };
}
