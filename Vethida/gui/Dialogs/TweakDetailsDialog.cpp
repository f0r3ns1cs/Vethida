#include "pch.h"
#include "TweakDetailsDialog.h"

#include "Confirm.h"
#include "../Theme/DesignTokens.h"
#include "../Utils/TablerIcon.h"
#include "../Widgets/Callout.h"
#include "../Widgets/CommandButton.h"
#include "../Widgets/ScrimDialog.h"

#include "../../vth_core/Models/ProductTier.h"
#include "../../vth_core/Tweaks/TweakEngine.h"

#include <QDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QStringList>
#include <QVBoxLayout>

namespace vth::dialogs
{
    namespace tokens = vth::theme::tokens;
    namespace pal    = vth::theme::palette;

    namespace
    {
        constexpr int kBodyMaxHeight = 400;
        constexpr int kHeaderGlyph   = tokens::CardBadgeSize;

        // A muted label over the answer.
        QWidget* factTile(QWidget* parent, const QString& label, const QString& value,
                          const QColor& valueColor = QColor())
        {
            auto* tile = new QWidget(parent);
            auto* col = new QVBoxLayout(tile);
            col->setContentsMargins(0, 0, 0, 0);
            col->setSpacing(tokens::Space2);

            auto* l = new QLabel(label, tile);
            l->setProperty("uiRole", QStringLiteral("metaText"));
            col->addWidget(l);

            auto* v = new QLabel(value, tile);
            v->setProperty("uiRole", QStringLiteral("fieldValue"));
            v->setWordWrap(true);
            // The word says it too, so color is never the only signal.
            if (valueColor.isValid()) {
                QPalette p = v->palette();
                p.setColor(QPalette::WindowText, valueColor);
                v->setPalette(p);
            }
            col->addWidget(v);
            return tile;
        }

        QString effectTiming(const vth::tweaks::TweakSpec& t)
        {
            return t.requiresReboot ? QObject::tr("After a restart")
                                    : QObject::tr("Immediately");
        }
    }

    TweakDetailsOutcome showTweakDetails(QWidget* parent,
                                         const vth::tweaks::TweakSpec& t,
                                         const TweakDetailsContext& ctx)
    {
        // revertVerbose refuses an irreversible spec, so the button could only fail.
        const bool offerRevert = ctx.changedByVethida && t.reversible;

        auto* card = new QFrame;
        card->setObjectName(QStringLiteral("srPromptCard"));
        card->setAttribute(Qt::WA_StyledBackground, true);
        card->setFixedWidth(tokens::ModalWidthMd);
        liftCard(card);

        auto* outer = new QVBoxLayout(card);
        outer->setContentsMargins(tokens::DialogInsetH, tokens::DialogInsetTop,
                                  tokens::DialogInsetH, tokens::DialogInsetBottom);
        outer->setSpacing(tokens::CardMargin);

        // Header: the tweak's own glyph, its name, where it lives.
        auto* head = new QHBoxLayout;
        head->setContentsMargins(0, 0, 0, 0);
        head->setSpacing(tokens::CardMargin);

        auto* badge = new QLabel(card);
        badge->setFixedSize(kHeaderGlyph, kHeaderGlyph);
        badge->setAlignment(Qt::AlignCenter);
        badge->setPixmap(vth::icons::pixmapFor(
            vth::icons::iconBadge(
                t.iconName.isEmpty() ? QStringLiteral("adjustments") : t.iconName,
                kHeaderGlyph),
            kHeaderGlyph, badge));
        head->addWidget(badge, 0, Qt::AlignVCenter);

        auto* titleCol = new QVBoxLayout;
        titleCol->setContentsMargins(0, 0, 0, 0);
        titleCol->setSpacing(tokens::Space2);

        auto* title = new QLabel(t.name, card);
        title->setObjectName(QStringLiteral("srPromptTitle"));
        title->setWordWrap(true);
        titleCol->addWidget(title);

        QStringList where;
        if (!t.subCategory.isEmpty())
            where << t.subCategory;
        if (!where.isEmpty()) {
            auto* crumb = new QLabel(where.join(QStringLiteral(" - ")), card);
            crumb->setProperty("uiRole", QStringLiteral("metaText"));
            titleCol->addWidget(crumb);
        }
        head->addLayout(titleCol, 1);

        // On Ultimate and early access only. Pro is the floor, so a Pro pill would sit
        // on nearly every tweak.
#ifdef VETHIDA_LICENSING
        const bool showEntitlementPill = t.earlyAccess
            || t.minTier == vth::models::ProductTier::Ultimate;
#else
        const bool showEntitlementPill = t.earlyAccess;
#endif
        if (showEntitlementPill) {
            auto* pill = new QLabel(t.earlyAccess
                                        ? QObject::tr("Early access")
                                        : vth::models::tierLabel(t.minTier), card);
            pill->setProperty("uiRole", QStringLiteral("chip"));
            pill->setProperty("tone", QStringLiteral("premium"));
            head->addWidget(pill, 0, Qt::AlignTop);
        }
        outer->addLayout(head);

        auto* scroll = new QScrollArea(card);
        scroll->setObjectName(QStringLiteral("settingsScroll"));
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scroll->setMaximumHeight(kBodyMaxHeight);

        auto* body = new QWidget(scroll);
        body->setObjectName(QStringLiteral("scrollContentHost"));
        auto* col = new QVBoxLayout(body);
        col->setContentsMargins(0, 0, 0, 0);
        col->setSpacing(tokens::CardMargin);

        if (!t.description.isEmpty()) {
            auto* desc = new QLabel(t.description, body);
            desc->setProperty("uiRole", QStringLiteral("bodyText"));
            desc->setWordWrap(true);
            col->addWidget(desc);
        }

        {
            auto* facts = new QWidget(body);
            auto* factRow = new QHBoxLayout(facts);
            factRow->setContentsMargins(0, 0, 0, 0);
            factRow->setSpacing(tokens::CardMargin);

            const bool heavy = t.impact == vth::tweaks::Impact::High
                            || t.impact == vth::tweaks::Impact::Critical;
            factRow->addWidget(factTile(facts, QObject::tr("Impact"),
                                        vth::tweaks::impactLabel(t.impact),
                                        heavy ? pal::warningText() : QColor()), 1);

            // Only the warning half, because a change being permanent has to be disclosed.
            if (!t.reversible) {
                factRow->addWidget(factTile(facts, QObject::tr("Permanent"),
                                            QObject::tr("Yes"),
                                            pal::criticalText()), 1);
            }

            factRow->addWidget(factTile(facts, QObject::tr("Takes effect"),
                                        effectTiming(t)), 1);

            col->addWidget(facts);
        }

        if (!t.warnings.isEmpty()) {
            col->addWidget(vth::widgets::makeCallout(body, QStringLiteral("alert-triangle"),
                                   QObject::tr("Warnings"),
                                   t.warnings, QStringLiteral("warn"), &pal::warningText));
        }

        // Not risks, so in neutral ink.
        if (!t.effects.isEmpty()) {
            col->addWidget(vth::widgets::makeCallout(body, QStringLiteral("info"),
                                   QObject::tr("Side effects"),
                                   t.effects, QString(), &pal::textSecondary));
        }

        if (!t.excludes.isEmpty()) {
            QStringList names;
            for (const QString& id : t.excludes) {
                const vth::tweaks::TweakSpec* other = vth::tweaks::find(id);
                names << (other && !other->name.isEmpty() ? other->name : id);
            }
            names.sort(Qt::CaseInsensitive);
            col->addWidget(vth::widgets::makeCallout(
                body, QStringLiteral("info"), QObject::tr("Conflicts"),
                { names.join(QStringLiteral(", ")) }, QString(), &pal::textSecondary));
        }

        if (!ctx.lastFailureReason.isEmpty()) {
            col->addWidget(vth::widgets::makeCallout(body, QStringLiteral("alert-circle"),
                                   QObject::tr("Last attempt"),
                                   { ctx.lastFailureReason }, QStringLiteral("critical"), &pal::criticalText));
        }

        // What Vethida did here, and what putting it back means.
        if (offerRevert) {
            const QString when = ctx.changedAt.isValid()
                ? QObject::tr("Vethida changed this on %1.")
                      .arg(ctx.changedAt.toLocalTime().toString(QStringLiteral("d MMMM yyyy")))
                : QObject::tr("Vethida changed this.");
            col->addWidget(vth::widgets::makeCallout(
                body, QStringLiteral("history"), QObject::tr("Changed by Vethida"),
                { QStringLiteral("%1 %2").arg(
                      when,
                      QObject::tr("Undo restores the settings this PC had "
                                  "before.")) },
                QString(), &pal::textSecondary));
        }

        // No registry paths, command lines or values. Hand edits to them would fight
        // the rollback ledger.

        scroll->setWidget(body);
        // As tall as its content, up to the cap.
        body->ensurePolished();
        const int innerWidth = tokens::ModalWidthMd - 2 * tokens::DialogInsetH;
        const int bodyHeight = body->layout()->hasHeightForWidth()
            ? body->layout()->heightForWidth(innerWidth)
            : body->sizeHint().height();
        scroll->setFixedHeight(qMin(bodyHeight, kBodyMaxHeight));
        outer->addWidget(scroll);
        outer->addSpacing(tokens::CompactGap);

        auto* close = new vth::widgets::CommandButton(
            vth::widgets::CommandButton::Variant::Primary, QObject::tr("Close"), card);

        // Not a danger variant, because putting the machine back is the safe direction.
        vth::widgets::CommandButton* revert = nullptr;
        if (offerRevert) {
            revert = new vth::widgets::CommandButton(
                vth::widgets::CommandButton::Variant::Secondary,
                QObject::tr("Undo Vethida's change"), card);
            revert->setAccessibleDescription(
                QObject::tr("Puts back the settings that were on this PC before "
                            "Vethida changed %1.").arg(t.name));
            outer->addLayout(buttonRow({ revert, close }));
        } else {
            outer->addLayout(buttonRow({ close }));
        }

        // Accepted either way, so this is what tells Undo from Escape.
        auto outcome = TweakDetailsOutcome::Closed;

        vth::widgets::ScrimDialog dlg(card, parent);
        QObject::connect(close, &QPushButton::clicked, &dlg, &QDialog::accept);

        if (revert) {
            QObject::connect(revert, &QPushButton::clicked, &dlg, [&dlg, &outcome]() {
                outcome = TweakDetailsOutcome::RevertRequested;
                dlg.accept();
            });
        }

        dlg.exec();
        return outcome;
    }

    // A reader only. The row's own switch is how to act.
    void showTweakWarnings(QWidget* parent, const vth::tweaks::TweakSpec& t)
    {
        if (t.warnings.isEmpty())
            return;

        QFrame* card = makePromptCard(t.name,
                                      QObject::tr("Read this before you switch it on."),
                                      PromptTone::Warning);
        card->setFixedWidth(tokens::ModalWidthSm + tokens::Space40);
        auto* col = static_cast<QVBoxLayout*>(card->layout());

        // The facts that change how much the warnings matter, as chips.
        const auto chip = [card](const QString& text, const char* tone) {
            auto* c = new QLabel(text, card);
            c->setProperty("uiRole", QStringLiteral("chip"));
            if (tone)
                c->setProperty("tone", QString::fromLatin1(tone));
            return c;
        };
        const bool heavy = t.impact == vth::tweaks::Impact::High
                        || t.impact == vth::tweaks::Impact::Critical;
        auto* facts = new QHBoxLayout;
        facts->setContentsMargins(0, 0, 0, 0);
        facts->setSpacing(tokens::TightGap + tokens::Space2);
        facts->addWidget(chip(QObject::tr("%1 impact").arg(vth::tweaks::impactLabel(t.impact)),
                              heavy ? "warn" : nullptr));
        facts->addWidget(chip(t.requiresReboot ? QObject::tr("Needs a restart")
                                               : QObject::tr("Works right away"), nullptr));
        // Only the permanent case is stated.
        if (!t.reversible)
            facts->addWidget(chip(QObject::tr("Permanent"), "critical"));
        facts->addStretch(1);
        col->addLayout(facts);

        // The warnings, in one amber panel, each on its own line.
        auto* panel = new QFrame(card);
        panel->setProperty("uiRole", QStringLiteral("callout"));
        panel->setProperty("tone", QStringLiteral("warn"));
        panel->setAttribute(Qt::WA_StyledBackground, true);
        auto* rows = new QVBoxLayout(panel);
        rows->setContentsMargins(tokens::CardMargin, tokens::CardMargin,
                                 tokens::CardMargin, tokens::CardMargin);
        rows->setSpacing(tokens::CompactGap);
        // One line: a glyph and the sentence beside it, top-aligned so a wrapped
        // line keeps its mark beside its first line.
        const auto line = [](QWidget* parent, QVBoxLayout* into, const QString& body,
                             const QString& glyph, const QColor& ink, const char* role) {
            auto* lay = new QHBoxLayout;
            lay->setContentsMargins(0, 0, 0, 0);
            lay->setSpacing(tokens::CompactGap);
            auto* mark = new QLabel(parent);
            mark->setFixedSize(tokens::GlyphMd, tokens::GlyphMd);
            mark->setPixmap(vth::icons::glyphPixmap(glyph, ink, tokens::GlyphMd, mark));
            lay->addWidget(mark, 0, Qt::AlignTop);
            auto* label = new QLabel(body, parent);
            label->setProperty("uiRole", QString::fromLatin1(role));
            label->setWordWrap(true);
            label->setTextInteractionFlags(Qt::TextSelectableByMouse);
            lay->addWidget(label, 1);
            into->addLayout(lay);
        };
        for (const QString& warning : t.warnings)
            line(panel, rows, warning, QStringLiteral("triangle-alert"),
                 pal::warningText(), "warnLine");
        col->addWidget(panel);

        // Side effects that are not risks: quieter, below the risks.
        if (!t.effects.isEmpty()) {
            auto* effects = new QVBoxLayout;
            effects->setContentsMargins(tokens::CardMargin, 0, tokens::CardMargin, 0);
            effects->setSpacing(tokens::TightGap + tokens::Space2);
            for (const QString& effect : t.effects)
                line(card, effects, effect, QStringLiteral("info"), pal::textMuted(), "metaText");
            col->addLayout(effects);
        }

        col->addSpacing(tokens::TightGap);
        auto* close = new vth::widgets::CommandButton(
            vth::widgets::CommandButton::Variant::Primary, QObject::tr("Close"), card);
        col->addLayout(buttonRow({ close }));

        vth::widgets::ScrimDialog dlg(card, parent);
        QObject::connect(close, &QPushButton::clicked, &dlg, &QDialog::accept);
        close->setFocus();
        dlg.exec();
    }
}
