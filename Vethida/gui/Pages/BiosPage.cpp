#include "pch.h"
#include "BiosPage.h"

#include "../Theme/DesignTokens.h"
#include "../Views/NvramView.h"
#include "../Widgets/PageHeader.h"
#include "../Widgets/TierLockOverlay.h"

#include "../../vth_core/Models/ProductTier.h"
#include "../../vth_core/Runtime/Tier.h"

#include <QShowEvent>
#include <QVBoxLayout>

namespace vth::ui
{
    namespace tokens = vth::theme::tokens;

    BiosPage::BiosPage(QWidget* parent)
        : QWidget(parent)
    {
        setObjectName(QStringLiteral("BiosPage"));

        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(0, 0, 0, 0);
        root->setSpacing(0);
    }

    void BiosPage::showEvent(QShowEvent* e)
    {
        QWidget::showEvent(e);
        if (!built_) {
            built_ = true;
            buildContent();
        }
    }

    void BiosPage::buildContent()
    {
        auto* root = static_cast<QVBoxLayout*>(layout());

        header_ = new vth::widgets::PageHeader(
            tr("BIOS"),
            tr("Set up your motherboard's firmware in one click."),
            this);
        root->addWidget(header_);

        auto* content = new QWidget(this);
        auto* contentCol = new QVBoxLayout(content);
        contentCol->setContentsMargins(tokens::PageMarginH, 0,
                                       tokens::PageMarginH, tokens::PageMarginBottom);
        contentCol->setSpacing(0);
        contentCol->addWidget(new NvramView(content), 1);
        root->addWidget(content, 1);

        // Everything on this tab is Ultimate, so the tab is gated once. NvramView's own
        // ensureTier still refuses the write.
        if (!vth::meetsTier(vth::models::ProductTier::Ultimate)) {
            auto* lock = new vth::widgets::TierLockOverlay(
                content, vth::models::ProductTier::Ultimate);
            lock->setFeatureName(tr("Firmware settings"));
            lock->show();
            lock->raise();
        }
    }
}
