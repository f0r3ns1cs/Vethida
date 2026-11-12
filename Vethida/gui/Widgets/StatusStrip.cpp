#include "pch.h"
#include "StatusStrip.h"

#include "CommandButton.h"
#include "GlyphLabel.h"

#include "../Theme/DesignTokens.h"

#include "../../vth_core/Models/ProductTier.h"

#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QResizeEvent>
#include <QToolButton>
#include <QSizePolicy>
#include <QStyle>

namespace vth::widgets
{
    StatusStrip::StatusStrip(QWidget* parent)
        : QWidget(parent)
    {
        namespace tokens = vth::theme::tokens;

        setObjectName(QStringLiteral("statusStrip"));
        // A bare QWidget draws no stylesheet background of its own.
        setAttribute(Qt::WA_StyledBackground, true);
        setFixedHeight(tokens::StatusStripHeight);
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

        auto* row = new QHBoxLayout(this);
        row->setContentsMargins(tokens::Space20, 0, tokens::Space20, 0);
        row->setSpacing(0);

        m_glyph = new GlyphLabel(QStringLiteral("check"),
                                 &vth::theme::palette::accentOnSurface,
                                 tokens::GlyphXl, this);
        row->addWidget(m_glyph, 0, Qt::AlignVCenter);

        row->addSpacing(tokens::Space16);

        auto* textCol = new QVBoxLayout;
        textCol->setContentsMargins(0, 0, 0, 0);
        textCol->setSpacing(tokens::Space2);
        // Stretches above and below, so the labels stay a block on the center line.
        textCol->addStretch(1);

        m_message = new QLabel(this);
        m_message->setObjectName(QStringLiteral("statusStripMessage"));
        m_message->setWordWrap(false);
        textCol->addWidget(m_message);

        m_detail = new QLabel(this);
        m_detail->setObjectName(QStringLiteral("statusStripDetail"));
        m_detail->setWordWrap(false);
        textCol->addWidget(m_detail);

        textCol->addStretch(1);
        row->addLayout(textCol, 0);

        row->addStretch(1);

        m_action = new CommandButton(CommandButton::Variant::Primary, QString(), this);
        m_action->setCursor(Qt::PointingHandCursor);
        connect(m_action, &QPushButton::clicked, this, [this]() {
            if (m_onAction)
                m_onAction();
        });
        row->addWidget(m_action, 0, Qt::AlignVCenter);

        row->addSpacing(tokens::CompactGap);

        m_close = new QToolButton(this);
        m_close->setObjectName(QStringLiteral("statusStripClose"));
        m_close->setCursor(Qt::PointingHandCursor);
        m_close->setAccessibleName(tr("Close"));
        m_close->setToolTip(tr("Close"));
        vth::icons::applyIconButton(m_close, QStringLiteral("x"),
                                    &vth::icons::iconColor, tokens::GlyphLg);
        connect(m_close, &QToolButton::clicked, this, &StatusStrip::dismissed);
        row->addWidget(m_close, 0, Qt::AlignVCenter);

        // The page shows this once it has a state to show.
        hide();
    }

    void StatusStrip::setState(State state, const Payload& payload)
    {
        m_onAction = payload.onAction;

        QString glyph;
        QString label;

        // A state with nothing to add leaves the second line empty.
        switch (state) {
        case State::Tip:
            glyph        = QStringLiteral("sparkles");
            m_text       = payload.message;
            m_detailText = payload.detail;
            label        = payload.buttonLabel;
            break;
        case State::FreeTier:
            glyph = QStringLiteral("lock");
            m_text = tr("%1 and %2 add the rest of Vethida")
                         .arg(vth::models::tierLabel(vth::models::ProductTier::Pro),
                              vth::models::tierLabel(vth::models::ProductTier::Ultimate));
            m_detailText = tr("From the processor and memory settings up to "
                              "Genesis and the driver-level graphics settings.");
            label        = tr("See plans");
            break;
        }

        m_glyph->setIconName(glyph);
        m_glyph->refresh();

        m_action->setVisible(!label.isEmpty());
        if (!label.isEmpty()) {
            m_action->setText(label);
            m_action->setAccessibleName(label);
        }

        applyMessage();
    }

    void StatusStrip::resizeEvent(QResizeEvent* event)
    {
        QWidget::resizeEvent(event);
        applyMessage();
    }

    void StatusStrip::applyMessage()
    {
        namespace tokens = vth::theme::tokens;

        if (!m_message)
            return;

        // What the row spends before the message, by hints because this runs before the
        // layout has.
        const int spent = 2 * tokens::Space20          // the strip's inset
                        + tokens::GlyphXl
                        + tokens::Space16              // glyph -> text
                        // isVisibleTo, because setState runs while the strip is hidden.
                        + (m_action->isVisibleTo(this) ? m_action->sizeHint().width() : 0)
                        + tokens::CompactGap           // action -> close
                        // The token, because a hint read before the sheet resolves is wrong.
                        + tokens::IconButtonSize;

        const int budget = qMax(0, width() - spent);

        const QFontMetrics fm(m_message->font());
        m_message->setText(fm.elidedText(m_text, Qt::ElideRight, budget));
        m_message->setToolTip(fm.horizontalAdvance(m_text) > budget ? m_text
                                                                   : QString());

        if (m_detail) {
            m_detail->setVisible(!m_detailText.isEmpty());
            const QFontMetrics dfm(m_detail->font());
            m_detail->setText(dfm.elidedText(m_detailText, Qt::ElideRight, budget));
            m_detail->setToolTip(
                dfm.horizontalAdvance(m_detailText) > budget ? m_detailText
                                                             : QString());
        }
    }
}
