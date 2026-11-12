#pragma once

#include "../Theme/DesignTokens.h"
#include "../Utils/TablerIcon.h"

#include "GlyphLabel.h"

#include <QColor>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>
#include <QWidget>

#include <utility>

namespace vth::widgets
{
    // tinted notice panel: icon, heading, body lines
    inline QFrame* makeCallout(QWidget* parent, const QString& iconName,
                               const QString& heading, const QStringList& lines,
                               const QString& tone,
                               vth::widgets::GlyphLabel::ColorFn glyphColor)
    {
        namespace tokens = vth::theme::tokens;

        constexpr int kGlyph = 18;

        auto* box = new QFrame(parent);
        box->setProperty("uiRole", QStringLiteral("callout"));
        if (!tone.isEmpty())
            box->setProperty("tone", tone);
        box->setAttribute(Qt::WA_StyledBackground, true);

        auto* row = new QHBoxLayout(box);
        row->setContentsMargins(tokens::CardInsetH, tokens::CardInsetV,
                                tokens::CardInsetH, tokens::CardInsetV);
        row->setSpacing(tokens::CardMargin);

        auto* glyph = new vth::widgets::GlyphLabel(iconName, std::move(glyphColor),
                                                   kGlyph, box);
        // Top-aligned, so the mark stays beside the heading when the body wraps.
        row->addWidget(glyph, 0, Qt::AlignTop);

        auto* col = new QVBoxLayout;
        col->setContentsMargins(0, 0, 0, 0);
        col->setSpacing(tokens::Space4);

        auto* title = new QLabel(heading, box);
        title->setProperty("uiRole", QStringLiteral("calloutTitle"));
        if (!tone.isEmpty())
            title->setProperty("tone", tone);
        title->setWordWrap(true);
        col->addWidget(title);

        for (const QString& line : lines) {
            auto* l = new QLabel(line, box);
            l->setProperty("uiRole", QStringLiteral("calloutBody"));
            l->setWordWrap(true);
            col->addWidget(l);
        }
        row->addLayout(col, 1);
        return box;
    }
}
