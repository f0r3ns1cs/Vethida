#include "pch.h"
#include "EmptyState.h"

#include "Spinner.h"

#include "../Theme/DesignTokens.h"

#include <QLabel>
#include <QVBoxLayout>

namespace vth::ui
{
    namespace
    {
        namespace tokens = vth::theme::tokens;
        constexpr int kGlyphPx = tokens::StateMarkSize;
    }

    EmptyState::EmptyState(QWidget* parent) : QWidget(parent)
    {
        auto* lay = new QVBoxLayout(this);
        lay->setContentsMargins(tokens::CardInsetH, tokens::CardInsetV,
                                tokens::CardInsetH, tokens::CardInsetV);
        lay->setSpacing(tokens::CompactGap);

        lay->addStretch(1);

        spinner_ = new vth::widgets::Spinner(this, kGlyphPx);
        spinner_->hide();
        lay->addWidget(spinner_, 0, Qt::AlignHCenter);

        title_ = new QLabel(this);
        title_->setObjectName(QStringLiteral("emptyStateTitle"));
        title_->setAlignment(Qt::AlignCenter);
        title_->setWordWrap(true);

        subtitle_ = new QLabel(this);
        subtitle_->setObjectName(QStringLiteral("emptyStateSubtitle"));
        subtitle_->setAlignment(Qt::AlignCenter);
        subtitle_->setWordWrap(true);
        subtitle_->hide();

        lay->addWidget(title_);
        lay->addWidget(subtitle_);

        lay->addStretch(1);
    }

    EmptyState::~EmptyState() = default;

    void EmptyState::setTitle(const QString& text)
    {
        title_->setText(text);
        applyContentVisibility();
    }

    void EmptyState::setSubtitle(const QString& text)
    {
        subtitle_->setText(text);
        applyContentVisibility();
    }

    void EmptyState::setBusy(bool busy)
    {
        busy_ = busy;
        spinner_->setVisible(busy);
        applyContentVisibility();
    }

    void EmptyState::applyContentVisibility()
    {
        title_->setVisible(!busy_ && !title_->text().isEmpty());
        subtitle_->setVisible(!busy_ && !subtitle_->text().isEmpty());
    }
}
