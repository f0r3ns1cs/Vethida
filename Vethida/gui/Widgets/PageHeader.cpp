#include "pch.h"
#include "PageHeader.h"

#include "../Theme/DesignTokens.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace vth::widgets
{
    namespace tokens = vth::theme::tokens;

    PageHeader::PageHeader(const QString& title, const QString& subtitle, QWidget* parent,
                           Inset inset)
        : QWidget(parent)
    {
        column_ = new QVBoxLayout(this);
        if (inset == Inset::Page)
            column_->setContentsMargins(tokens::PageMarginH, tokens::PageMarginTop,
                                        tokens::PageMarginH, tokens::CardMargin);
        else
            column_->setContentsMargins(0, 0, 0, tokens::CardMargin);
        column_->setSpacing(tokens::TightGap);

        auto* titleRow = new QHBoxLayout;
        titleRow->setContentsMargins(0, 0, 0, 0);
        titleRow->setSpacing(tokens::CompactGap);

        auto* titleLabel = new QLabel(title, this);
        titleLabel->setProperty("uiRole", QStringLiteral("pageTitle"));
        titleRow->addWidget(titleLabel, 0, Qt::AlignVCenter);
        titleRow->addStretch(1);

        actions_ = new QHBoxLayout;
        actions_->setContentsMargins(0, 0, 0, 0);
        actions_->setSpacing(tokens::CompactGap);
        titleRow->addLayout(actions_, 0);

        column_->addLayout(titleRow);

        subtitle_ = new QLabel(subtitle, this);
        subtitle_->setProperty("uiRole", QStringLiteral("bodyText"));
        subtitle_->setWordWrap(true);
        subtitle_->setVisible(!subtitle.isEmpty());
        column_->addWidget(subtitle_);
    }

    void PageHeader::addAction(QWidget* action)
    {
        if (!action)
            return;
        action->setParent(this);
        actions_->addWidget(action, 0, Qt::AlignVCenter);
    }

    void PageHeader::addNotice(QWidget* notice)
    {
        if (!notice)
            return;
        notice->setParent(this);
        column_->addWidget(notice);
    }

    void PageHeader::setSubtitle(const QString& text)
    {
        subtitle_->setText(text);
        subtitle_->setVisible(!text.isEmpty());
    }
}
