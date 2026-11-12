#pragma once

#include "../Theme/DesignTokens.h"
#include "../Utils/TablerIcon.h"
#include "../Widgets/PanelDialog.h"

#include <QAbstractButton>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QSize>
#include <QString>
#include <QToolButton>
#include <QVBoxLayout>

namespace vth::dialogs
{
    // Whether the content scrolls itself or the modal scrolls it.
    enum class ModalScroll { Self, Managed };

    // A title row with a close button over the content, confined to the panel beside
    // the sidebar and below the titlebar. Takes ownership of content.
    inline void showContentModal(QWidget* anchor, const QString& title, QWidget* content,
                                 vth::widgets::PanelDialog::Mode mode,
                                 ModalScroll scroll = ModalScroll::Self)
    {
        auto* card = new QFrame;
        card->setObjectName(QStringLiteral("toolModalCard"));
        card->setAttribute(Qt::WA_StyledBackground, true);

        auto* col = new QVBoxLayout(card);
        col->setContentsMargins(vth::theme::tokens::DialogInsetWideH, vth::theme::tokens::DialogInsetTop,
                                vth::theme::tokens::DialogInsetWideH, vth::theme::tokens::DialogInsetBottom);
        col->setSpacing(vth::theme::tokens::RegionMargin);

        auto* head = new QHBoxLayout;
        head->setContentsMargins(0, 0, 0, 0);
        head->setSpacing(vth::theme::tokens::CardMargin);
        auto* titleLabel = new QLabel(title, card);
        titleLabel->setObjectName(QStringLiteral("toolModalTitle"));
        head->addWidget(titleLabel, 1, Qt::AlignVCenter);

        // A Cover modal collapses back into the page, so it gets a return arrow.
        // A scrim panel floats over the page and gets an X.
        auto* closeBtn = new QToolButton(card);
        closeBtn->setAutoRaise(true);
        closeBtn->setCursor(Qt::PointingHandCursor);
        closeBtn->setFixedSize(vth::theme::tokens::ModalCloseSize,
                               vth::theme::tokens::ModalCloseSize);
        vth::icons::applyIconButton(
            closeBtn,
            mode == vth::widgets::PanelDialog::Mode::Cover
                ? QStringLiteral("arrow-down-left")
                : QStringLiteral("x"),
            &vth::icons::iconColor,
            vth::theme::tokens::ModalCloseGlyph);
        closeBtn->setAccessibleName(QObject::tr("Close"));
        closeBtn->setToolTip(QObject::tr("Close"));
        head->addWidget(closeBtn, 0, Qt::AlignTop);
        col->addLayout(head);

        if (scroll == ModalScroll::Managed) {
            // Bounds the height only, because the card's inset is already the body's.
            auto* scroller = new QScrollArea(card);
            scroller->setObjectName(QStringLiteral("settingsScroll"));
            scroller->setWidgetResizable(true);
            scroller->setFrameShape(QFrame::NoFrame);
            scroller->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

            auto* host = new QWidget(scroller);
            host->setObjectName(QStringLiteral("scrollContentHost"));
            auto* hostLay = new QVBoxLayout(host);
            hostLay->setContentsMargins(0, 0, 0, 0);
            hostLay->setSpacing(0);
            content->setParent(host);
            // A short body sits at the top.
            hostLay->addWidget(content);
            hostLay->addStretch(1);

            scroller->setWidget(host);
            col->addWidget(scroller, 1);
        } else {
            content->setParent(card);
            col->addWidget(content, 1);
        }

        vth::widgets::PanelDialog dlg(card, anchor, mode);
        QObject::connect(closeBtn, &QAbstractButton::clicked, &dlg, &QDialog::reject);
        dlg.exec();
    }
}
