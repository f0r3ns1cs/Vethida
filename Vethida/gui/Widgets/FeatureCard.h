#pragma once

#include "../Theme/DesignTokens.h"
#include "../Utils/TablerIcon.h"
#include "GlyphLabel.h"

#include <QColor>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QSize>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

namespace vth::widgets
{
    enum class Accent { Neutral, Nvidia, Amd };

    inline QColor featureAccentColor(Accent a)
    {
        namespace pal = vth::theme::palette;
        switch (a) {
        case Accent::Nvidia: return pal::nvidiaMark();
        case Accent::Amd:    return pal::amdMark();
        case Accent::Neutral: break;
        }
        return pal::featureMark();
    }

    inline QString featureAccentName(Accent a)
    {
        switch (a) {
        case Accent::Nvidia: return QStringLiteral("nvidia");
        case Accent::Amd:    return QStringLiteral("amd");
        case Accent::Neutral: break;
        }
        return QString();
    }

    // The headline card on a pane: a colored glyph, a title and the capability
    // underneath. The accent is fixed at construction.
    inline QFrame* makeFeatureCard(QWidget* parent, const QString& iconName,
                                   const QString& title, QWidget* body,
                                   Accent accent = Accent::Neutral)
    {
        namespace tokens = vth::theme::tokens;

        auto* card = new QFrame(parent);
        card->setProperty("uiRole", QStringLiteral("featureCard"));
        card->setProperty("accent", featureAccentName(accent));
        card->setAttribute(Qt::WA_StyledBackground, true);

        auto* col = new QVBoxLayout(card);
        col->setContentsMargins(tokens::CardInsetH, tokens::CardInsetV,
                                tokens::CardInsetH, tokens::CardInsetV);
        col->setSpacing(tokens::TightGap);

        auto* head = new QHBoxLayout;
        head->setContentsMargins(0, 0, 0, 0);
        head->setSpacing(tokens::CompactGap);

        constexpr int kGlyph = 28;
        // Captures the accent, because every mark hue differs per mode.
        auto* glyph = new vth::widgets::GlyphLabel(
            iconName,
            [accent] { return featureAccentColor(accent); },
            kGlyph, card);
        head->addWidget(glyph, 0, Qt::AlignVCenter);

        auto* titleLabel = new QLabel(title, card);
        titleLabel->setProperty("uiRole", QStringLiteral("featureTitle"));
        head->addWidget(titleLabel, 1, Qt::AlignVCenter);
        col->addLayout(head);

        if (body) {
            col->addSpacing(tokens::CardMargin);
            body->setParent(card);
            col->addWidget(body);
        }
        return card;
    }
}
