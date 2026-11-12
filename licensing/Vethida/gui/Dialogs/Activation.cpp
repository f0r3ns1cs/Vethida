#include "pch.h"
#include "Activation.h"

#include "ActivationController.h"
#include "Confirm.h"
#include "../Theme/DesignTokens.h"
#include "../Utils/TablerIcon.h"
#include "../Widgets/CommandButton.h"
#include "../Widgets/ScrimDialog.h"

#include "../../vth_core/Services/LicenseService.h"
#include "../../vth_core/SiteUrls.h"

#include <QAbstractButton>
#include <QDesktopServices>
#include <QDialog>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
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
    namespace
    {
        // same height for all three states so the card doesn't resize
        constexpr int kActivateModalHeight = 340;

        // title + close button over a content column, caller wires closeBtn
        struct ScrimCard
        {
            QFrame*      card;
            QVBoxLayout* col;
            QToolButton* closeBtn;
        };

        ScrimCard makeScrimCard(int width, const QString& title)
        {
            auto* card = new QFrame;
            card->setObjectName(QStringLiteral("toolModalCard"));
            card->setAttribute(Qt::WA_StyledBackground, true);
            card->setFixedWidth(width);
            auto* col = new QVBoxLayout(card);
            col->setContentsMargins(vth::theme::tokens::DialogInsetWideH, vth::theme::tokens::DialogInsetTop,
                                    vth::theme::tokens::DialogInsetWideH, vth::theme::tokens::DialogInsetBottom);
            col->setSpacing(vth::theme::tokens::CardMargin);

            auto* headRow = new QHBoxLayout;
            headRow->setContentsMargins(0, 0, 0, 0);
            headRow->setSpacing(vth::theme::tokens::CardMargin);
            auto* titleLabel = new QLabel(title, card);
            titleLabel->setObjectName(QStringLiteral("toolModalTitle"));
            titleLabel->setWordWrap(true);
            headRow->addWidget(titleLabel, 1, Qt::AlignVCenter);

            auto* closeBtn = new QToolButton(card);
            closeBtn->setAutoRaise(true);
            closeBtn->setCursor(Qt::PointingHandCursor);
            closeBtn->setFixedSize(vth::theme::tokens::ModalCloseSize,
                                   vth::theme::tokens::ModalCloseSize);
            closeBtn->setAccessibleName(QObject::tr("Close"));
            closeBtn->setToolTip(QObject::tr("Close"));
            vth::icons::applyIconButton(closeBtn, QStringLiteral("x"),
                                        &vth::theme::palette::textMuted,
                                        vth::theme::tokens::ModalCloseGlyph);
            headRow->addWidget(closeBtn, 0, Qt::AlignTop);
            col->addLayout(headRow);

            return { card, col, closeBtn };
        }
    }

    bool runActivation(QWidget* host, vth::services::LicenseService& svc,
                       const QString& prefillKey)
    {
        namespace tokens = vth::theme::tokens;

        auto [card, col, closeBtn] = makeScrimCard(tokens::ModalWidthMd,
                                                   QObject::tr("Activate Vethida"));

        auto* stack = new QStackedLayout;
        stack->setContentsMargins(0, 0, 0, 0);
        col->addLayout(stack, 1);

        auto* form = new QWidget(card);
        auto* formCol = new QVBoxLayout(form);
        formCol->setContentsMargins(0, 0, 0, 0);
        formCol->setSpacing(tokens::CardMargin);

        auto* body = new QLabel(
            QObject::tr("Sign in to your Vethida account and this PC is activated for you. "
               "Activating uses this license's one machine slot."), form);
        body->setObjectName(QStringLiteral("overlayBody"));
        body->setWordWrap(true);
        formCol->addWidget(body);

        auto* signInBtn = new vth::widgets::CommandButton(
            vth::widgets::CommandButton::Variant::Primary, QObject::tr("Sign in"), form);
        signInBtn->setCursor(Qt::PointingHandCursor);
        formCol->addWidget(signInBtn);

        auto* orRow = new QHBoxLayout;
        orRow->setContentsMargins(0, tokens::Space2, 0, tokens::Space2);
        orRow->setSpacing(tokens::CardMargin);
        auto* orLeft = new QFrame(form);
        orLeft->setObjectName(QStringLiteral("settingsRule"));
        orLeft->setAttribute(Qt::WA_StyledBackground, true);
        auto* orLabel = new QLabel(QObject::tr("or"), form);
        orLabel->setProperty("uiRole", QStringLiteral("metaText"));
        auto* orRight = new QFrame(form);
        orRight->setObjectName(QStringLiteral("settingsRule"));
        orRight->setAttribute(Qt::WA_StyledBackground, true);
        orRow->addWidget(orLeft, 1, Qt::AlignVCenter);
        orRow->addWidget(orLabel, 0, Qt::AlignVCenter);
        orRow->addWidget(orRight, 1, Qt::AlignVCenter);
        formCol->addLayout(orRow);

        auto* fieldCol = new QVBoxLayout;
        fieldCol->setContentsMargins(0, 0, 0, 0);
        fieldCol->setSpacing(tokens::SmallMargin);

        auto* fieldLabel = new QLabel(QObject::tr("License key"), form);
        fieldLabel->setProperty("uiRole", QStringLiteral("fieldLabel"));
        fieldCol->addWidget(fieldLabel);

        auto* keyEdit = new QLineEdit(form);
        keyEdit->setObjectName(QStringLiteral("activateKeyField"));
        keyEdit->setPlaceholderText(QStringLiteral("XXXXX-XXXXX-XXXXX-XXXXX-XXXXX"));
        keyEdit->setAccessibleName(QObject::tr("License key"));
        vth::ui::ActivationController::wireKeyField(keyEdit);
        fieldLabel->setBuddy(keyEdit);
        fieldCol->addWidget(keyEdit);

        // hidden instead of removed so the buttons don't move
        auto* status = new QLabel(form);
        status->setProperty("uiRole", QStringLiteral("formError"));
        status->setWordWrap(true);
        status->setVisible(false);
        fieldCol->addWidget(status);
        formCol->addLayout(fieldCol);

        formCol->addStretch(1);

        auto* rule = new QFrame(form);
        rule->setObjectName(QStringLiteral("settingsRule"));
        rule->setAttribute(Qt::WA_StyledBackground, true);
        formCol->addWidget(rule);

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
        // Off until there is a whole key to submit.
        activateBtn->setEnabled(false);

        // The help link goes ahead of the row's own stretch, at the far left.
        auto* actions = vth::dialogs::buttonRow({ cancelBtn, activateBtn });
        actions->insertWidget(0, findKey, 0, Qt::AlignVCenter);
        formCol->addLayout(actions);

        stack->addWidget(form);

        auto* waiting = new QWidget(card);
        auto* waitCol = new QVBoxLayout(waiting);
        waitCol->setContentsMargins(0, 0, 0, 0);
        waitCol->setSpacing(tokens::CardMargin);
        waitCol->addStretch(1);

        auto* waitTitle = new QLabel(QObject::tr("Continue in your browser"), waiting);
        waitTitle->setObjectName(QStringLiteral("activateDoneTitle"));
        waitTitle->setAlignment(Qt::AlignHCenter);
        waitTitle->setWordWrap(true);
        waitCol->addWidget(waitTitle);

        auto* waitBody = new QLabel(
            QObject::tr("my.vethida.com is open in your browser. Approve the request there "
               "and this PC activates itself."), waiting);
        waitBody->setObjectName(QStringLiteral("overlayBody"));
        waitBody->setAlignment(Qt::AlignHCenter);
        waitBody->setWordWrap(true);
        waitCol->addWidget(waitBody);

        // anti-phishing: the user compares this with the code in the browser
        auto* codeCaption = new QLabel(QObject::tr("Check the browser shows this code"), waiting);
        codeCaption->setProperty("uiRole", QStringLiteral("metaText"));
        codeCaption->setAlignment(Qt::AlignHCenter);
        waitCol->addWidget(codeCaption);

        auto* codeLabel = new QLabel(waiting);
        codeLabel->setObjectName(QStringLiteral("activateUserCode"));
        codeLabel->setAlignment(Qt::AlignHCenter);
        codeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        {
            QFont codeFont = codeLabel->font();
            codeFont.setLetterSpacing(QFont::AbsoluteSpacing, 3.0);
            codeLabel->setFont(codeFont);
        }
        waitCol->addWidget(codeLabel);

        auto* waitStatus = new QLabel(waiting);
        waitStatus->setProperty("uiRole", QStringLiteral("formError"));
        waitStatus->setAlignment(Qt::AlignHCenter);
        waitStatus->setWordWrap(true);
        waitStatus->setVisible(false);
        waitCol->addWidget(waitStatus);

        waitCol->addStretch(1);

        // Bottom right in all three states, so the button being aimed at doesn't move.
        auto* waitCancelBtn = new vth::widgets::CommandButton(
            vth::widgets::CommandButton::Variant::Secondary, QObject::tr("Back"), waiting);
        auto* reopenBtn = new vth::widgets::CommandButton(
            vth::widgets::CommandButton::Variant::Primary, QObject::tr("Open the page again"), waiting);
        waitCol->addLayout(vth::dialogs::buttonRow({ waitCancelBtn, reopenBtn }));

        stack->addWidget(waiting);

        auto* done = new QWidget(card);
        auto* doneCol = new QVBoxLayout(done);
        doneCol->setContentsMargins(0, 0, 0, 0);
        doneCol->setSpacing(tokens::CardMargin);
        doneCol->addStretch(1);

        auto* mark = new QLabel(done);
        mark->setFixedSize(tokens::StateMarkSize, tokens::StateMarkSize);
        mark->setAlignment(Qt::AlignCenter);
        mark->setPixmap(vth::icons::glyphPixmap(QStringLiteral("circle-check-filled"),
                                                vth::theme::palette::accentFill(),
                                                tokens::StateMarkSize, mark));
        doneCol->addWidget(mark, 0, Qt::AlignHCenter);

        auto* doneTitle = new QLabel(done);
        doneTitle->setObjectName(QStringLiteral("activateDoneTitle"));
        doneTitle->setAlignment(Qt::AlignHCenter);
        doneTitle->setWordWrap(true);
        doneCol->addWidget(doneTitle);

        auto* doneBody = new QLabel(done);
        doneBody->setObjectName(QStringLiteral("overlayBody"));
        doneBody->setAlignment(Qt::AlignHCenter);
        doneBody->setWordWrap(true);
        doneCol->addWidget(doneBody);

        doneCol->addStretch(1);

        auto* closeDone = new vth::widgets::CommandButton(
            vth::widgets::CommandButton::Variant::Primary, QObject::tr("Done"), done);
        doneCol->addLayout(vth::dialogs::buttonRow({ closeDone }));

        stack->addWidget(done);

        card->setMinimumHeight(kActivateModalHeight);

        // a key from an activation link is only filled in, never auto-submitted
        if (!prefillKey.isEmpty()) {
            keyEdit->setText(prefillKey);
            activateBtn->setEnabled(
                vth::ui::ActivationController::keyIsSubmittable(prefillKey));
        }

        vth::widgets::ScrimDialog dlg(card, host);
        dlg.setInitialFocus(prefillKey.isEmpty()
                                ? static_cast<QWidget*>(signInBtn)
                                : static_cast<QWidget*>(activateBtn));

        QObject::connect(closeBtn, &QAbstractButton::clicked, &dlg, &QDialog::reject);
        QObject::connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
        QObject::connect(closeDone, &QPushButton::clicked, &dlg, &QDialog::accept);

        // owned by the dialog. An activation still finishes if the card is closed.
        auto* control = new vth::ui::ActivationController(svc, &dlg);

        const auto clearError = [status, keyEdit]() {
            if (!status->isVisible())
                return;
            status->setVisible(false);
            keyEdit->setProperty("state", QVariant());
            keyEdit->style()->unpolish(keyEdit);
            keyEdit->style()->polish(keyEdit);
        };

        // errors go on the form
        QObject::connect(control, &vth::ui::ActivationController::failed, &dlg,
                [stack, form, status, keyEdit](const QString& text) {
            stack->setCurrentWidget(form);
            status->setText(text);
            status->setVisible(true);
            keyEdit->setProperty("state", QStringLiteral("error"));
            keyEdit->style()->unpolish(keyEdit);
            keyEdit->style()->polish(keyEdit);
            keyEdit->setFocus(Qt::OtherFocusReason);
            keyEdit->selectAll();
        });

        QObject::connect(control, &vth::ui::ActivationController::notice, &dlg,
                [waitStatus](const QString& text) {
            waitStatus->setText(text);
            waitStatus->setVisible(true);
        });
        QObject::connect(control, &vth::ui::ActivationController::noticeCleared, &dlg,
                [waitStatus]() { waitStatus->setVisible(false); });

        QObject::connect(control, &vth::ui::ActivationController::signInStarted, &dlg,
                [stack, waiting, codeLabel, waitStatus](const QString& userCode) {
            codeLabel->setText(userCode);
            waitStatus->setVisible(false);
            stack->setCurrentWidget(waiting);
        });
        QObject::connect(control, &vth::ui::ActivationController::signInEnded, &dlg,
                [stack, form, signInBtn]() {
            stack->setCurrentWidget(form);
            signInBtn->setFocus(Qt::OtherFocusReason);
        });

        QObject::connect(control, &vth::ui::ActivationController::busyChanged, &dlg,
                [keyEdit, cancelBtn, signInBtn, activateBtn](bool busy) {
            keyEdit->setEnabled(!busy);
            cancelBtn->setEnabled(!busy);
            if (busy)
                return;
            signInBtn->setLoading(false);
            activateBtn->setLoading(false);
            signInBtn->setEnabled(true);
            activateBtn->setEnabled(
                vth::ui::ActivationController::keyIsSubmittable(keyEdit->text()));
        });

        QObject::connect(control, &vth::ui::ActivationController::activated, &dlg,
                [control, stack, done, doneTitle, doneBody, closeDone]() {
            doneTitle->setText(control->successTitle());
            doneBody->setText(control->successBody());

            // Stays until the user closes it.
            stack->setCurrentWidget(done);
            closeDone->setFocus(Qt::OtherFocusReason);
        });

        QObject::connect(reopenBtn, &QPushButton::clicked, control,
                         &vth::ui::ActivationController::reopenSignInPage);
        QObject::connect(waitCancelBtn, &QPushButton::clicked, control,
                         &vth::ui::ActivationController::cancelSignIn);
        // ask the controller before showing the spinner, it can refuse
        QObject::connect(signInBtn, &QPushButton::clicked, &dlg,
                [control, signInBtn, activateBtn, clearError]() {
            if (control->isBusy())
                return;
            clearError();
            signInBtn->setLoading(true);
            activateBtn->setEnabled(false);
            control->beginSignIn();
        });

        QObject::connect(keyEdit, &QLineEdit::textChanged, activateBtn,
                [activateBtn, keyEdit, clearError]() {
            activateBtn->setEnabled(
                vth::ui::ActivationController::keyIsSubmittable(keyEdit->text()));
            clearError();
        });

        const auto submit = [control, keyEdit, activateBtn, signInBtn]() {
            if (control->isBusy()
                || !vth::ui::ActivationController::keyIsSubmittable(keyEdit->text())) {
                return;
            }
            activateBtn->setLoading(true);
            signInBtn->setEnabled(false);
            control->submitKey(keyEdit->text());
        };
        QObject::connect(activateBtn, &QPushButton::clicked, &dlg, submit);
        // Enter submits (ScrimDialog isn't a window, so no default button)
        QObject::connect(keyEdit, &QLineEdit::returnPressed, &dlg, submit);

        dlg.exec();
        return control->isActivated();
    }
}
