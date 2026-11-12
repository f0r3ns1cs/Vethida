#pragma once

#include "../Theme/DesignTokens.h"

#include "GlyphLabel.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QString>
#include <QWidget>

namespace vth::widgets
{
    // Why a control on this pane cannot act: the machine does not have the
    // hardware, or the license does not reach it.
    inline QFrame* makeDisabledReason(QWidget* parent, const QString& reason)
    {
        namespace tokens = vth::theme::tokens;

        auto* box = new QFrame(parent);

        auto* row = new QHBoxLayout(box);
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(tokens::CompactGap);

        // Top-aligned, so the mark stays beside the first line when the reason wraps.
        row->addWidget(new vth::widgets::GlyphLabel(
                           QStringLiteral("info"),
                           &vth::theme::palette::textSecondary,
                           tokens::GlyphMd, box),
                       0, Qt::AlignTop);

        auto* text = new QLabel(reason, box);
        text->setObjectName(QStringLiteral("disabledReasonText"));
        text->setWordWrap(true);
        row->addWidget(text, 1);
        return box;
    }

    // Replaces the reason on a panel already built. Found by name, because the glyph
    // beside it is a QLabel too.
    inline void setDisabledReason(QFrame* box, const QString& reason)
    {
        if (!box)
            return;
        if (auto* text = box->findChild<QLabel*>(QStringLiteral("disabledReasonText")))
            text->setText(reason);
    }
}
