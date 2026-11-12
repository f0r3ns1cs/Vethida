#pragma once

#include "../Theme/DesignTokens.h"
#include "../Utils/TablerIcon.h"
#include "../Widgets/CommandButton.h"
#include "../Widgets/ScrimDialog.h"
#include "../Settings/Preferences.h"

#include "../../vth_core/Runtime/ServiceRegistry.h"
#include "../../vth_core/Runtime/Settings.h"
#include "../../vth_core/SiteUrls.h"

#include <QCheckBox>
#include <QDesktopServices>
#include <QDialog>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QObject>
#include <QPushButton>
#include <QSize>
#include <QString>
#include <QUrl>
#include <QVBoxLayout>

namespace vth::dialogs
{
    enum class PromptTone { Question, Warning };

    // The footer every dialog uses: secondary actions first, the primary one last.
    inline QHBoxLayout* buttonRow(std::initializer_list<vth::widgets::CommandButton*> buttons)
    {
        auto* row = new QHBoxLayout;
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(vth::theme::tokens::CompactGap);
        row->addStretch(1);
        for (vth::widgets::CommandButton* b : buttons) {
            b->setMinimumWidth(vth::theme::tokens::ButtonMinWidth);
            b->setCursor(Qt::PointingHandCursor);
            row->addWidget(b, 0, Qt::AlignVCenter);
        }
        return row;
    }

    // The soft shadow under every modal card.
    inline void liftCard(QWidget* card)
    {
        namespace tokens = vth::theme::tokens;
        auto* shadow = new QGraphicsDropShadowEffect(card);
        shadow->setBlurRadius(tokens::Space40);
        shadow->setOffset(0, tokens::Space8);
        shadow->setColor(vth::theme::palette::modalShadow());
        card->setGraphicsEffect(shadow);
    }

    // A tinted disc carrying the tone, a title and a paragraph, on one left edge.
    // Callers add rows to card->layout() below them.
    inline QFrame* makePromptCard(const QString& title, const QString& body, PromptTone tone)
    {
        namespace tokens = vth::theme::tokens;
        namespace pal    = vth::theme::palette;

        auto* card = new QFrame;
        card->setObjectName(QStringLiteral("srPromptCard"));
        card->setAttribute(Qt::WA_StyledBackground, true);
        card->setFixedWidth(tokens::ModalWidthSm);

        liftCard(card);

        auto* col = new QVBoxLayout(card);
        col->setContentsMargins(tokens::DialogInsetH, tokens::DialogInsetH,
                                tokens::DialogInsetH, tokens::DialogInsetBottom);
        col->setSpacing(tokens::CardMargin);

        const bool warning = tone == PromptTone::Warning;
        auto* badge = new QLabel(card);
        badge->setObjectName(QStringLiteral("srPromptBadge"));
        badge->setProperty("tone", warning ? QStringLiteral("warning") : QStringLiteral("question"));
        badge->setFixedSize(tokens::PromptBadgeSize, tokens::PromptBadgeSize);
        badge->setAlignment(Qt::AlignCenter);
        badge->setPixmap(vth::icons::glyphPixmap(
            warning ? QStringLiteral("triangle-alert") : QStringLiteral("info"),
            warning ? pal::warningText() : pal::tagPillText(),
            tokens::GlyphLg, badge));
        col->addWidget(badge, 0, Qt::AlignLeft);
        col->addSpacing(tokens::TightGap);

        // Tighter than the card's gap, because they're one statement.
        auto* text = new QVBoxLayout;
        text->setContentsMargins(0, 0, 0, 0);
        text->setSpacing(tokens::Space4 + tokens::Space2);

        auto* titleLabel = new QLabel(title, card);
        titleLabel->setObjectName(QStringLiteral("srPromptTitle"));
        titleLabel->setWordWrap(true);
        text->addWidget(titleLabel);

        if (!body.isEmpty()) {
            auto* bodyLabel = new QLabel(body, card);
            bodyLabel->setObjectName(QStringLiteral("srPromptBody"));
            bodyLabel->setWordWrap(true);
            bodyLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
            text->addWidget(bodyLabel);
        }
        col->addLayout(text);
        return card;
    }

    // cancelLabel replaces "Cancel" where declining is a deferral, such as "Not now".
    inline bool confirm(QWidget* host, const QString& title, const QString& body,
                        const QString& confirmLabel, PromptTone tone = PromptTone::Question,
                        const QString& cancelLabel = QString())
    {
        QFrame* card = makePromptCard(title, body, tone);
        auto* col = static_cast<QVBoxLayout*>(card->layout());
        col->addSpacing(vth::theme::tokens::CompactGap);

        auto* cancel = new vth::widgets::CommandButton(vth::widgets::CommandButton::Variant::Secondary,
                                         cancelLabel.isEmpty() ? QObject::tr("Cancel") : cancelLabel,
                                         card);
        auto* ok = new vth::widgets::CommandButton(vth::widgets::CommandButton::Variant::Primary, confirmLabel, card);
        col->addLayout(buttonRow({ cancel, ok }));

        vth::widgets::ScrimDialog dlg(card, host);
        // Enter on a confirm the user hasn't read mustn't carry it out.
        dlg.setInitialFocus(cancel);
        QObject::connect(cancel, &QPushButton::clicked, &dlg, &QDialog::reject);
        QObject::connect(ok, &QPushButton::clicked, &dlg, &QDialog::accept);
        cancel->setFocus();
        return dlg.exec() == QDialog::Accepted;
    }

    inline void notify(QWidget* host, const QString& title, const QString& body, PromptTone tone)
    {
        QFrame* card = makePromptCard(title, body, tone);
        auto* col = static_cast<QVBoxLayout*>(card->layout());
        col->addSpacing(vth::theme::tokens::CompactGap);

        auto* ok = new vth::widgets::CommandButton(vth::widgets::CommandButton::Variant::Primary, QObject::tr("Close"), card);
        col->addLayout(buttonRow({ ok }));

        vth::widgets::ScrimDialog dlg(card, host);
        QObject::connect(ok, &QPushButton::clicked, &dlg, &QDialog::accept);
        ok->setFocus();
        dlg.exec();
    }

    inline void inform(QWidget* host, const QString& title, const QString& body)
    {
        notify(host, title, body, PromptTone::Question);
    }

    inline void warn(QWidget* host, const QString& title, const QString& body)
    {
        notify(host, title, body, PromptTone::Warning);
    }

    // One-time caution before a risky area. "Don't remind me again" persists under
    // cautionAck/<key>. Returns true when the user may proceed.
    inline bool cautionGate(QWidget* host, const QString& key,
                            const QString& title, const QString& body)
    {
        auto* settings = vth::ServiceRegistry::instance().get<vth::Settings>();
        const QString ackKey = QStringLiteral("cautionAck/") + key;
        if (!vth::prefs::showTips()
            || (settings && settings->value(ackKey, false).toBool()))
            return true;

        QFrame* card = makePromptCard(title, body, PromptTone::Question);
        auto* col = static_cast<QVBoxLayout*>(card->layout());

        auto* remind = new QCheckBox(QObject::tr("Don't remind me again"), card);
        remind->setCursor(Qt::PointingHandCursor);
        col->addWidget(remind);
        col->addSpacing(vth::theme::tokens::CompactGap);

        auto* backBtn = new vth::widgets::CommandButton(vth::widgets::CommandButton::Variant::Secondary,
                                          QObject::tr("Cancel"), card);
        auto* continueBtn = new vth::widgets::CommandButton(vth::widgets::CommandButton::Variant::Primary,
                                              QObject::tr("Continue"), card);
        col->addLayout(buttonRow({ backBtn, continueBtn }));

        vth::widgets::ScrimDialog dlg(card, host);
        dlg.setInitialFocus(backBtn);
        QObject::connect(backBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
        QObject::connect(continueBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
        continueBtn->setFocus();
        const bool proceed = dlg.exec() == QDialog::Accepted;
        if (proceed && remind->isChecked() && settings)
            settings->setValue(ackKey, true);
        return proceed;
    }

    // The upgrade prompt, wherever a locked feature is activated. tier is the required
    // tier's label, and empty means Ultimate.
    inline void showUpgrade(QWidget* host, const QString& tier)
    {
        const QString tierName = tier.isEmpty() ? QObject::tr("Ultimate") : tier;

        QFrame* card = makePromptCard(
            QObject::tr("Vethida %1 feature").arg(tierName),
            QObject::tr("Upgrade to unlock this and the rest of Vethida %1.").arg(tierName),
            PromptTone::Question);
        auto* col = static_cast<QVBoxLayout*>(card->layout());

        col->addSpacing(vth::theme::tokens::CompactGap);
        auto* later = new vth::widgets::CommandButton(vth::widgets::CommandButton::Variant::Secondary,
                                        QObject::tr("Not now"), card);
        auto* cta = new vth::widgets::CommandButton(vth::widgets::CommandButton::Variant::Primary,
                                      QObject::tr("Upgrade to %1").arg(tierName), card);
        col->addLayout(buttonRow({ later, cta }));

        vth::widgets::ScrimDialog dlg(card, host);
        QObject::connect(cta, &QPushButton::clicked, &dlg, &QDialog::accept);
        QObject::connect(later, &QPushButton::clicked, &dlg, &QDialog::reject);
        cta->setFocus();
        if (dlg.exec() == QDialog::Accepted)
            QDesktopServices::openUrl(QUrl(vth::site::kPricing));
    }
}
