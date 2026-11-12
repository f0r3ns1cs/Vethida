#include "pch.h"
#include "CrashRecoveryDialog.h"

#include "Confirm.h"
#include "../Theme/DesignTokens.h"
#include "../Widgets/CommandButton.h"
#include "../Widgets/ScrimDialog.h"

#include "../../vth_core/Runtime/CrashRecoveryService.h"
#include "../../vth_core/Runtime/Dispatcher.h"
#include "../../vth_core/Runtime/ServiceRegistry.h"
#include "../../vth_core/Runtime/Threading.h"
#include "../../vth_core/Tweaks/TweakEngine.h"

#include <QDialog>
#include <QFrame>
#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

namespace vth::dialogs
{
    namespace
    {
        namespace tokens = vth::theme::tokens;
    }

    void showCrashRecovery(QWidget* host, const QStringList& opIds,
                           const QDateTime& startedAt)
    {
        VTH_ASSERT_UI_THREAD();

        // An id no longer in the catalog still shows, so the report stays complete.
        QStringList displayNames;
        int reversibleCount = 0;
        for (const QString& id : opIds) {
            if (const vth::tweaks::TweakSpec* spec = vth::tweaks::find(id)) {
                displayNames << spec->name;
                if (spec->reversible)
                    ++reversibleCount;
            } else {
                displayNames << id;
            }
        }

        const QString started = startedAt.isValid()
            ? startedAt.toLocalTime().toString(Qt::ISODate)
            : QObject::tr("(time unknown)");

        QFrame* card = makePromptCard(
            QObject::tr("Unfinished tweaks"),
            QObject::tr("Vethida was interrupted at %1 while applying the tweaks below. "
                        "Which of them reached this PC is not recorded.")
                .arg(started),
            PromptTone::Warning);
        card->setFixedWidth(tokens::ModalWidthMd);
        auto* col = static_cast<QVBoxLayout*>(card->layout());

        if (!displayNames.isEmpty()) {
            auto* names = new QLabel(displayNames.join(QLatin1Char('\n')), card);
            names->setObjectName(QStringLiteral("settingsRowDesc"));
            names->setTextInteractionFlags(Qt::TextSelectableByMouse);

            auto* scroll = new QScrollArea(card);
            scroll->setObjectName(QStringLiteral("settingsScroll"));
            scroll->setWidgetResizable(true);
            scroll->setFrameShape(QFrame::NoFrame);
            scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            scroll->setMaximumHeight(tokens::ModalScrollMaxHeight);
            scroll->setWidget(names);
            col->addWidget(scroll);
        }

        auto* status = new QLabel(card);
        status->setObjectName(QStringLiteral("settingsRowDesc"));
        status->setWordWrap(true);
        status->setVisible(false);
        col->addWidget(status);

        col->addSpacing(tokens::CompactGap);
        auto* dismiss = new vth::widgets::CommandButton(
            vth::widgets::CommandButton::Variant::Secondary,
            QObject::tr("Not now"), card);
        auto* revert = new vth::widgets::CommandButton(
            vth::widgets::CommandButton::Variant::Primary,
            QObject::tr("Revert these tweaks"), card);
        col->addLayout(buttonRow({ dismiss, revert }));

        // Nothing reversible to act on leaves a plain dismiss.
        if (reversibleCount == 0) {
            revert->setEnabled(false);
            revert->setToolTip(QObject::tr("Nothing here can be reverted."));
        }

        vth::widgets::ScrimDialog dlg(card, host);
        dlg.setInitialFocus(dismiss);
        QObject::connect(dismiss, &QPushButton::clicked, &dlg, &QDialog::reject);
        QObject::connect(revert, &QPushButton::clicked, card,
                         [card, status, revert, dismiss]() {
            auto* recovery = vth::ServiceRegistry::instance().get<vth::CrashRecoveryService>();
            if (!recovery)
                return;

            revert->setEnabled(false);
            dismiss->setEnabled(false);
            status->setVisible(true);
            status->setText(QObject::tr("Reverting..."));

            // Off the UI thread: it writes the registry once per tweak.
            QPointer<QWidget> guard(card);
            vth::Dispatcher::instance().dispatch([guard, status, revert, dismiss, recovery]() {
                const auto outcome = recovery->revertPriorBatch();
                vth::Dispatcher::instance().dispatchUi(
                    [guard, status, revert, dismiss, outcome]() {
                    if (!guard)
                        return;
                    status->setText(outcome.message);

                    revert->setVisible(false);
                    dismiss->setText(QObject::tr("Close"));
                    dismiss->setEnabled(true);
                });
            });
        });

        dlg.exec();
    }
}
