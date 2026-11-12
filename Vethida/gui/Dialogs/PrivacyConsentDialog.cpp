#include "pch.h"
#include "PrivacyConsentDialog.h"

#include "../../vth_core/Runtime/Consent.h"
#include "../../vth_core/SiteUrls.h"

#include "Confirm.h"
#include "../Theme/DesignTokens.h"
#include "../Widgets/CommandButton.h"
#include "../Widgets/ScrimDialog.h"

#include <QDesktopServices>
#include <QFrame>
#include <QLabel>
#include <QUrl>
#include <QVBoxLayout>

namespace vth::dialogs
{
    void PrivacyConsentDialog::runFirstRun(QWidget* host)
    {
        if (vth::consent::decided()) return;

        auto* card = new QFrame;
        card->setObjectName(QStringLiteral("srPromptCard"));
        card->setAttribute(Qt::WA_StyledBackground, true);
        card->setFixedWidth(vth::theme::tokens::ModalWidthMd);

        auto* col = new QVBoxLayout(card);
        col->setContentsMargins(vth::theme::tokens::DialogInsetH, vth::theme::tokens::DialogInsetTop,
                                vth::theme::tokens::DialogInsetH, vth::theme::tokens::DialogInsetBottom);
        col->setSpacing(vth::theme::tokens::CardMargin);

        auto* title = new QLabel(QObject::tr("Crash reports"), card);
        title->setObjectName(QStringLiteral("srPromptTitle"));
        title->setWordWrap(true);
        col->addWidget(title);

        auto* body = new QLabel(card);
        body->setObjectName(QStringLiteral("srPromptBody"));
        body->setWordWrap(true);
        body->setText(QObject::tr(
            "If Vethida crashes, it can send a report so the fault can be found and "
            "fixed. A report holds the error, a snapshot of Vethida's memory at the "
            "moment of the crash, the Vethida and Windows versions, and a coarse "
            "hardware class (core count, memory size, graphics vendor). It holds no "
            "license key and none of your files. Reports go to Sentry and are "
            "deleted within 90 days.\n\nYou can change this at any time in Settings."));
        col->addWidget(body);

        auto* policy = new QLabel(card);
        policy->setProperty("uiRole", QStringLiteral("link"));
        policy->setTextFormat(Qt::RichText);
        policy->setText(QStringLiteral("<a href=\"%1\">%2</a>")
                            .arg(vth::site::kPrivacy,
                                 QObject::tr("Privacy policy")));
        policy->setTextInteractionFlags(Qt::LinksAccessibleByMouse |
                                        Qt::LinksAccessibleByKeyboard);
        QObject::connect(policy, &QLabel::linkActivated, card, [](const QString& url) {
            QDesktopServices::openUrl(QUrl(url));
        });
        col->addWidget(policy);

        col->addSpacing(vth::theme::tokens::TightGap);

        auto* decline = new vth::widgets::CommandButton(vth::widgets::CommandButton::Variant::Secondary,
                                                        QObject::tr("Don't send"), card);
        auto* accept = new vth::widgets::CommandButton(vth::widgets::CommandButton::Variant::Primary,
                                                       QObject::tr("Send crash reports"), card);
        col->addLayout(vth::dialogs::buttonRow({ decline, accept }));

        vth::widgets::ScrimDialog dlg(card, host);
        QObject::connect(decline, &QPushButton::clicked, &dlg, &QDialog::reject);
        QObject::connect(accept, &QPushButton::clicked, &dlg, &QDialog::accept);
        // The answer that sends nothing takes initial focus.
        decline->setFocus();

        // Dismissing counts as declining, so the question isn't asked again.
        vth::consent::record(dlg.exec() == QDialog::Accepted);
    }
}
