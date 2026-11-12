#include "pch.h"
#include "PageScaffold.h"

#include "../Theme/DesignTokens.h"
#include "PageHeader.h"

#include <QFrame>
#include <QScrollArea>
#include <QVBoxLayout>

namespace vth::widgets
{
    namespace tokens = vth::theme::tokens;

    PageScaffold::PageScaffold(const QString& title, const QString& subtitle, QWidget* parent)
        : QWidget(parent)
    {
        auto* outer = new QVBoxLayout(this);
        outer->setContentsMargins(0, 0, 0, 0);
        outer->setSpacing(0);

        auto* scroll = new QScrollArea(this);
        scroll->setObjectName(QStringLiteral("settingsScroll"));
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        outer->addWidget(scroll);

        auto* body = new QWidget;
        body->setObjectName(QStringLiteral("scrollContentHost"));
        bodyLay_ = new QVBoxLayout(body);
        bodyLay_->setContentsMargins(tokens::PageMarginH, tokens::PageMarginTop,
                                     tokens::PageMarginH, tokens::PageMarginBottom);
        bodyLay_->setSpacing(tokens::CardMargin);

        if (!title.isEmpty()) {
            header_ = new PageHeader(title, subtitle, body, PageHeader::Inset::Bare);
            bodyLay_->addWidget(header_);
        }

        bodyLay_->addStretch(1);

        scroll->setWidget(body);
    }

    void PageScaffold::addHeaderAction(QWidget* action)
    {
        if (action && header_)
            header_->addAction(action);
    }

    void PageScaffold::setBodySpacing(int spacing)
    {
        bodyLay_->setSpacing(spacing);
    }

    void PageScaffold::addBodyWidget(QWidget* widget, int stretch)
    {
        if (widget)
            bodyLay_->insertWidget(bodyLay_->count() - 1, widget, stretch);
    }

    void PageScaffold::addBodyLayout(QLayout* layout, int stretch)
    {
        if (layout)
            bodyLay_->insertLayout(bodyLay_->count() - 1, layout, stretch);
    }
}
