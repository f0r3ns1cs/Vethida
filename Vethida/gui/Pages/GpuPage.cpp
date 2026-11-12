#include "pch.h"
#include "GpuPage.h"

#include "SystemTools.h"
#include "TweakController.h"

#include "../Theme/DesignTokens.h"
#include "../Utils/TablerIcon.h"
#include "../Views/GpuDriverView.h"
#include "../Views/TweakSectionView.h"
#include "../Dialogs/ToolModal.h"
#include "../Widgets/Callout.h"
#include "../Widgets/CommandButton.h"
#include "../Widgets/EmptyState.h"
#include "../Widgets/DisabledReason.h"
#include "../Widgets/FeatureCard.h"
#include "../Widgets/GlyphLabel.h"
#include "../Widgets/PageHeader.h"
#include "../Widgets/SectionNav.h"

#include "../../vth_core/Runtime/Fetch.h"
#include "../../vth_core/SystemInfo.h"
#include "../../vth_core/Windows/NvApi.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QShowEvent>
#include <QStackedWidget>
#include <QToolButton>
#include <QVBoxLayout>

namespace vth::ui
{
    namespace tokens  = vth::theme::tokens;
    namespace machine = vth::win::machine;

    namespace
    {
        using vth::widgets::makeFeatureCard;

        // The tool, or a centered message when this PC can't use it. A Self pane so the
        // message can be centered.
        QStackedWidget* toolOrMessage(QWidget* tool, EmptyState** message)
        {
            auto* stack = new QStackedWidget;

            auto* scroll = new QScrollArea(stack);
            scroll->setObjectName(QStringLiteral("settingsScroll"));
            scroll->setWidgetResizable(true);
            scroll->setFrameShape(QFrame::NoFrame);
            scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            auto* host = new QWidget(scroll);
            host->setObjectName(QStringLiteral("scrollContentHost"));
            auto* lay = new QVBoxLayout(host);
            lay->setContentsMargins(tokens::PageMarginH, tokens::PageMarginTop,
                                    tokens::PageMarginH, tokens::PageMarginBottom);
            lay->setSpacing(0);
            lay->addWidget(tool);
            lay->addStretch(1);
            scroll->setWidget(host);
            stack->addWidget(scroll);

            *message = new EmptyState(stack);
            stack->addWidget(*message);
            return stack;
        }
    }

    GpuPage::GpuPage(TweakController* controller, QWidget* parent)
        : QWidget(parent)
        , controller_(controller)
    {
        setObjectName(QStringLiteral("GpuPage"));

        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(0, 0, 0, 0);
        root->setSpacing(0);
    }

    void GpuPage::showEvent(QShowEvent* e)
    {
        QWidget::showEvent(e);
        if (!built_) {
            built_ = true;
            buildContent();
        }
    }

    void GpuPage::prewarm()
    {
        if (!built_) {
            built_ = true;
            buildContent();
        }
        ensurePolished();
    }

    void GpuPage::buildContent()
    {
        auto* root = static_cast<QVBoxLayout*>(layout());

        header_ = new vth::widgets::PageHeader(
            tr("GPU"),
            tr("Driver settings, latency and power behavior for your graphics "
               "card."),
            this);
        root->addWidget(header_);

        auto* nav = new vth::widgets::SectionNav(this);
        nav_ = nav;

        // Both Pro at the rail, so a locked section can't be opened.
        nav->addSection(QStringLiteral("gpu"), tr("NVIDIA profile"),
                        tr("Latency, power, and image quality in the driver"),
                        toolOrMessage(buildProfilePane(), &profileMessage_),
                        vth::widgets::SectionNav::PaneScroll::Self,
                        vth::models::ProductTier::Pro);

        nav->addSection(QStringLiteral("refresh-cw"), tr("Driver reinstall"),
                        tr("Clean removal and reinstall, without safe mode"),
                        toolOrMessage(buildDriverPane(), &driverMessage_),
                        vth::widgets::SectionNav::PaneScroll::Self,
                        vth::models::ProductTier::Pro);

        // Chrome::None, because this pane is already inside the rail's card.
        auto* tweaks = new TweakSectionView(controller_, QStringLiteral("gpu"),
                                            QString(), nullptr, QString(),
                                            TweakSectionView::Chrome::None);
        tweaks->setBulkCategory(QStringLiteral("gpu"));
        tweaks->setPlaceholder(
            tr("Vethida has no graphics settings for this adapter yet."));
        nav->addSection(QStringLiteral("adjustments"),
                        tr("Driver and power settings"),
                        tr("Latency, power, and driver behavior"),
                        tweaks);

        root->addWidget(vth::widgets::sectionNavHost(nav, this), 1);

        // Loads the NVIDIA library on a worker now, so the first Apply doesn't stall the
        // window while the driver initializes.
        vth::fetch<bool>(this, []() { return vth::win::nvapi::available(); },
                         [](bool) {});

        // Once with what the controller already has, and once when detection lands.
        applyHardware(controller_->machineProfile());
        connect(controller_, &TweakController::machineProfileReady, this,
                [this]() { applyHardware(controller_->machineProfile()); });
    }

    QWidget* GpuPage::buildProfilePane()
    {
        auto* body = new QWidget;
        auto* bodyCol = new QVBoxLayout(body);
        bodyCol->setContentsMargins(0, 0, 0, 0);
        bodyCol->setSpacing(tokens::CardMargin);

        auto* blurb = new QLabel(
            tr("Writes Vethida's settings to your global NVIDIA profile, the one every "
               "game uses unless it has its own."), body);
        blurb->setProperty("uiRole", QStringLiteral("bodyText"));
        blurb->setWordWrap(true);
        bodyCol->addWidget(blurb);

        // every group of settings the profile changes
        auto* checks = new QVBoxLayout;
        checks->setContentsMargins(0, 0, 0, 0);
        checks->setSpacing(tokens::TightGap);
        const auto check = [body, checks](const QString& text) {
            auto* row = new QHBoxLayout;
            row->setContentsMargins(0, 0, 0, 0);
            row->setSpacing(tokens::CompactGap);
            row->addWidget(new vth::widgets::GlyphLabel(QStringLiteral("check"),
                                                        &vth::theme::palette::scoreGoodText,
                                                        tokens::GlyphMd, body),
                           0, Qt::AlignTop);
            auto* label = new QLabel(text, body);
            label->setProperty("uiRole", QStringLiteral("bodyText"));
            label->setWordWrap(true);
            row->addWidget(label, 1);
            checks->addLayout(row);
        };
        check(tr("Low latency mode on, with one frame queued ahead"));
        check(tr("Power management set to prefer maximum performance"));
        check(tr("Texture filtering set for speed, shader cache with no size limit"));
        check(tr("Driver-forced anti-aliasing, sharpening, ambient occlusion and Ansel off"));
        check(tr("Resizable BAR turned off in the driver profile"));
        bodyCol->addLayout(checks);

        bodyCol->addWidget(vth::widgets::makeCallout(
            body, QStringLiteral("triangle-alert"), tr("Turns G-SYNC off"),
            { tr("Vertical sync and G-SYNC are switched off for every game. On a G-SYNC "
                 "monitor, switch G-SYNC on again in the NVIDIA Control Panel afterward.") },
            QStringLiteral("warn"), &vth::theme::palette::warningText));

        nvidiaNote_ = vth::widgets::makeDisabledReason(body, QString());
        nvidiaNote_->setVisible(false);
        bodyCol->addWidget(nvidiaNote_);

        auto* all = new QToolButton(body);
        all->setProperty("uiRole", QStringLiteral("link"));
        all->setCursor(Qt::PointingHandCursor);
        all->setText(tr("See all %1 settings").arg(vth::win::nvapi::optimalProfile().size()));
        connect(all, &QToolButton::clicked, this, [this]() {
            // Two columns in a scrolling panel: forty-odd names in one column
            // ran past the bottom of the window.
            const auto& profile = vth::win::nvapi::optimalProfile();
            auto* content = new QWidget;
            auto* grid = new QGridLayout(content);
            grid->setContentsMargins(0, 0, 0, 0);
            grid->setHorizontalSpacing(tokens::Space32);
            grid->setVerticalSpacing(tokens::TightGap);
            const int perColumn = (static_cast<int>(profile.size()) + 1) / 2;
            for (int i = 0; i < profile.size(); ++i) {
                auto* name = new QLabel(QString::fromLatin1(profile.at(i).label), content);
                name->setObjectName(QStringLiteral("settingsRowDesc"));
                grid->addWidget(name, i % perColumn, i / perColumn);
            }
            vth::dialogs::showContentModal(this, tr("Settings in the NVIDIA profile"), content,
                                           vth::widgets::PanelDialog::Mode::ScrimPanel,
                                           vth::dialogs::ModalScroll::Managed);
        });

        nvidiaUndo_ = new vth::widgets::CommandButton(
            vth::widgets::CommandButton::Variant::Secondary, tr("Undo"), body);
        nvidiaUndo_->setToolTip(tr("Puts back the NVIDIA settings you had before."));
        connect(nvidiaUndo_, &QPushButton::clicked, this, [this]() {
            vth::ui::tools::revertNvidiaProfile(this, [this]() { refreshProfileUndo(); });
        });

        nvidiaBtn_ = new vth::widgets::CommandButton(
            vth::widgets::CommandButton::Variant::Primary,
            tr("Apply NVIDIA profile"), body);
        connect(nvidiaBtn_, &QPushButton::clicked, this, [this]() {
            vth::ui::tools::applyOptimalNvidiaProfile(this, [this]() { refreshProfileUndo(); });
        });

        // One line: the link on the left, the actions on the right.
        auto* actionRow = new QHBoxLayout;
        actionRow->setContentsMargins(0, 0, 0, 0);
        actionRow->setSpacing(tokens::CompactGap);
        actionRow->addWidget(all, 0, Qt::AlignVCenter);
        actionRow->addStretch(1);
        actionRow->addWidget(nvidiaUndo_, 0, Qt::AlignVCenter);
        actionRow->addWidget(nvidiaBtn_, 0, Qt::AlignVCenter);
        bodyCol->addLayout(actionRow);
        refreshProfileUndo();

        return makeFeatureCard(nullptr, QStringLiteral("gpu"),
                               tr("NVIDIA profile settings"), body,
                               vth::widgets::Accent::Nvidia);
    }

    void GpuPage::refreshProfileUndo()
    {
        if (nvidiaUndo_)
            nvidiaUndo_->setVisible(vth::win::nvapi::hasProfileBackup());
    }

    QWidget* GpuPage::buildDriverPane()
    {
        return makeFeatureCard(nullptr, QStringLiteral("refresh-cw"),
                               tr("NVIDIA driver reinstall"),
                               new GpuDriverView(nullptr),
                               vth::widgets::Accent::Nvidia);
    }

    // Everything on this page that depends on which adapter is installed.
    void GpuPage::applyHardware(const machine::Profile& profile)
    {
        const bool usable = profile.hasNvidia();
        const bool elsewhere = !usable && profile.gpuIdentified();
        const QString gpuName = vth::win::tidyHardwareName(profile.gpuName);

        // The card Vethida found, in the subtitle.
        if (header_ && !gpuName.isEmpty()) {
            header_->setSubtitle(
                profile.gpuDriverVersion.isEmpty()
                    ? gpuName
                    : tr("%1, driver %2").arg(gpuName, profile.gpuDriverVersion));
        }

        // no NVIDIA card: open on the settings section instead (once)
        if (elsewhere && nav_ && !openedOnSettings_ && nav_->currentIndex() == 0) {
            openedOnSettings_ = true;
            nav_->setCurrentIndex(2);
        }

        // A card from another maker: both NVIDIA panes say so once, in their
        // middle, instead of showing controls that cannot act.
        for (EmptyState* message : { profileMessage_, driverMessage_ }) {
            if (!message)
                continue;
            message->setTitle(tr("These tools need an NVIDIA graphics card"));
            message->setSubtitle(gpuName.isEmpty()
                ? tr("They work through NVIDIA's driver, and this PC has a card "
                     "from another maker.")
                : tr("They work through NVIDIA's driver, and this PC has %1.")
                      .arg(gpuName));
            if (auto* stack = qobject_cast<QStackedWidget*>(message->parentWidget()))
                stack->setCurrentIndex(elsewhere ? 1 : 0);
        }

        if (!nvidiaBtn_)
            return;

        nvidiaBtn_->setEnabled(usable);
        nvidiaBtn_->setToolTip(
            usable ? tr("Writes the profile to the NVIDIA driver.")
                   : tr("This profile is written through the NVIDIA driver, "
                        "so it needs an NVIDIA card."));

        if (nvidiaNote_) {
            nvidiaNote_->setVisible(!usable);
            vth::widgets::setDisabledReason(nvidiaNote_,
                elsewhere
                    ? tr("This profile is written through the NVIDIA driver, "
                         "so it needs an NVIDIA card.")
                    : tr("Vethida could not read this PC's graphics card yet."));
        }
    }
}
