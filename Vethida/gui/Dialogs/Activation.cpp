#include "pch.h"
#include "Activation.h"

#include "Confirm.h"
#include "../Theme/DesignTokens.h"
#include "../Utils/TablerIcon.h"
#include "../Widgets/CommandButton.h"
#include "../Widgets/ScrimDialog.h"

#include "../../vth_core/Models/LicenseModels.h"
#include "../../vth_core/Models/ProductTier.h"
#include "../../vth_core/Services/LicenseService.h"
#include "../../vth_core/SiteUrls.h"

#include <QDesktopServices>
#include <QDialog>
#include <QFutureWatcher>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedLayout>
#include <QStyle>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QVariant>

namespace vth::dialogs
{
    // The key form, then the receipt, at one height so the card doesn't jump.
    bool runActivation(QWidget* host, vth::services::LicenseService& svc,
                       const QString& prefillKey)
    {
        namespace tokens = vth::theme::tokens;
        using vth::models::LicenseValidation;

        QFrame* card = makePromptCard(
            QObject::tr("Activate Vethida"),
            QObject::tr("Paste the license key from the email we sent after checkout."),
            PromptTone::Question);
        card->setFixedWidth(tokens::ModalWidthMd);
        auto* col = static_cast<QVBoxLayout*>(card->layout());

        auto* stack = new QStackedLayout;
        stack->setContentsMargins(0, 0, 0, 0);
        col->addLayout(stack);

        // The form.
        auto* form = new QWidget(card);
        auto* formCol = new QVBoxLayout(form);
        formCol->setContentsMargins(0, 0, 0, 0);
        formCol->setSpacing(tokens::SmallMargin);

        auto* keyEdit = new QLineEdit(form);
        keyEdit->setObjectName(QStringLiteral("activateKeyField"));
        keyEdit->setPlaceholderText(QStringLiteral("xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"));
        keyEdit->setAccessibleName(QObject::tr("License key"));
        keyEdit->setMaxLength(64);
        formCol->addWidget(keyEdit);

        auto* status = new QLabel(form);
        status->setProperty("uiRole", QStringLiteral("formError"));
        status->setWordWrap(true);
        status->setVisible(false);
        formCol->addWidget(status);

        formCol->addSpacing(tokens::CompactGap);
        auto* findKey = new QToolButton(form);
        findKey->setProperty("uiRole", QStringLiteral("link"));
        findKey->setAutoRaise(true);
        findKey->setText(QObject::tr("Find your key"));
        findKey->setCursor(Qt::PointingHandCursor);
        findKey->setFocusPolicy(Qt::TabFocus);
        QObject::connect(findKey, &QToolButton::clicked, host, []() {
            QDesktopServices::openUrl(QUrl(vth::site::kPortalLicenses));
        });
        auto* cancelBtn = new vth::widgets::CommandButton(
            vth::widgets::CommandButton::Variant::Secondary, QObject::tr("Cancel"), form);
        auto* activateBtn = new vth::widgets::CommandButton(
            vth::widgets::CommandButton::Variant::Primary, QObject::tr("Activate"), form);
        activateBtn->setEnabled(false);
        auto* actions = buttonRow({ cancelBtn, activateBtn });
        actions->insertWidget(0, findKey, 0, Qt::AlignVCenter);
        formCol->addLayout(actions);
        stack->addWidget(form);

        // The receipt.
        auto* done = new QWidget(card);
        auto* doneCol = new QVBoxLayout(done);
        doneCol->setContentsMargins(0, 0, 0, 0);
        doneCol->setSpacing(tokens::CardMargin);
        auto* doneBody = new QLabel(done);
        doneBody->setObjectName(QStringLiteral("srPromptBody"));
        doneBody->setWordWrap(true);
        doneCol->addWidget(doneBody);
        doneCol->addStretch(1);
        auto* closeDone = new vth::widgets::CommandButton(
            vth::widgets::CommandButton::Variant::Primary, QObject::tr("Close"), done);
        doneCol->addLayout(buttonRow({ closeDone }));
        stack->addWidget(done);

        // A key from an activation link fills the field; the customer still
        // presses Activate.
        const auto submittable = [keyEdit]() {
            return LicenseValidation::isValidKeyFormat(
                LicenseValidation::formatKey(keyEdit->text()));
        };
        if (!prefillKey.isEmpty()) {
            keyEdit->setText(prefillKey);
            activateBtn->setEnabled(submittable());
        }

        vth::widgets::ScrimDialog dlg(card, host);
        dlg.setInitialFocus(keyEdit);
        QObject::connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
        QObject::connect(closeDone, &QPushButton::clicked, &dlg, &QDialog::accept);

        const auto setError = [status, keyEdit](const QString& text) {
            status->setText(text);
            status->setVisible(!text.isEmpty());
            keyEdit->setProperty("state", text.isEmpty() ? QVariant()
                                                         : QVariant(QStringLiteral("error")));
            keyEdit->style()->unpolish(keyEdit);
            keyEdit->style()->polish(keyEdit);
        };

        QObject::connect(keyEdit, &QLineEdit::textChanged, &dlg,
                         [activateBtn, submittable, setError]() {
            activateBtn->setEnabled(submittable());
            setError(QString());
        });

        bool activated = false;
        bool busy = false;
        const auto submit = [&]() {
            if (busy || !submittable())
                return;
            busy = true;
            activateBtn->setLoading(true);
            keyEdit->setEnabled(false);
            cancelBtn->setEnabled(false);

            using vth::models::LicenseOpResult;
            auto* watcher = new QFutureWatcher<LicenseOpResult>(&dlg);
            QObject::connect(watcher, &QFutureWatcher<LicenseOpResult>::finished, &dlg,
                             [&, watcher]() {
                const LicenseOpResult result = watcher->result();
                watcher->deleteLater();
                busy = false;
                activateBtn->setLoading(false);
                keyEdit->setEnabled(true);
                cancelBtn->setEnabled(true);

                if (!result.ok) {
                    setError(vth::models::licenseErrorLabel(result.error));
                    keyEdit->setFocus(Qt::OtherFocusReason);
                    keyEdit->selectAll();
                    return;
                }
                activated = true;
                const QString product = vth::models::tierProductName(
                    vth::models::tierFromString(svc.currentInfo().tier));
                doneBody->setText(QObject::tr("%1 is active on this PC. Everything in "
                                              "your plan is unlocked.").arg(product));
                stack->setCurrentWidget(done);
                closeDone->setFocus(Qt::OtherFocusReason);
            });
            watcher->setFuture(svc.activateAsync(keyEdit->text()));
        };
        QObject::connect(activateBtn, &QPushButton::clicked, &dlg, submit);
        QObject::connect(keyEdit, &QLineEdit::returnPressed, &dlg, submit);

        dlg.exec();
        return activated;
    }
}
