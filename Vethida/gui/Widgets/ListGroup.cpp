#include "pch.h"
#include "ListGroup.h"

#include "StatusMark.h"

#include "../Theme/DesignTokens.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QStyle>
#include <QVBoxLayout>

namespace vth::widgets
{
    namespace
    {
        namespace tokens = vth::theme::tokens;
    }

    ListRow::ListRow(const QString& title, const QString& description,
                     QWidget* parent)
        : QFrame(parent)
    {
        setObjectName(QStringLiteral("listRow"));
        setAttribute(Qt::WA_StyledBackground, true);

        row_ = new QHBoxLayout(this);
        row_->setContentsMargins(tokens::RowInsetH, tokens::Space12,
                                 tokens::RowInsetH, tokens::Space12);
        row_->setSpacing(tokens::CardMargin);

        text_ = new QVBoxLayout;
        text_->setContentsMargins(0, 0, 0, 0);
        text_->setSpacing(tokens::Space2);

        auto* name = new QLabel(title, this);
        name->setObjectName(QStringLiteral("listRowTitle"));
        name->setWordWrap(true);
        text_->addWidget(name);

        if (!description.isEmpty()) {
            auto* desc = new QLabel(description, this);
            desc->setObjectName(QStringLiteral("settingsRowDesc"));
            desc->setWordWrap(true);
            text_->addWidget(desc);
        }

        statusRow_ = new QWidget(this);
        auto* statusLay = new QHBoxLayout(statusRow_);
        statusLay->setContentsMargins(0, tokens::TightGap, 0, 0);
        statusLay->setSpacing(tokens::TightGap + tokens::Space2);
        mark_ = new StatusMark(statusRow_);
        statusLay->addWidget(mark_, 0, Qt::AlignTop);
        status_ = new QLabel(statusRow_);
        status_->setObjectName(QStringLiteral("listRowStatus"));
        status_->setWordWrap(true);
        status_->setTextInteractionFlags(Qt::TextSelectableByMouse);
        statusLay->addWidget(status_, 1);
        statusRow_->hide();
        text_->addWidget(statusRow_);

        progress_ = new QProgressBar(this);
        progress_->setObjectName(QStringLiteral("listRowProgress"));
        progress_->setRange(0, 100);
        progress_->setTextVisible(false);
        progress_->hide();
        text_->addWidget(progress_);

        row_->addLayout(text_, 1);
    }

    void ListRow::addTrailing(QWidget* w)
    {
        if (!w)
            return;
        w->setParent(this);
        row_->addWidget(w, 0, Qt::AlignVCenter);
    }

    void ListRow::addDetail(QWidget* w)
    {
        if (!w)
            return;
        w->setParent(this);
        text_->insertWidget(1, w);
    }

    void ListRow::setStatus(const QString& text, Tone tone, bool animate)
    {
        if (text.isEmpty()) {
            statusRow_->hide();
            mark_->clear();
            status_->clear();
            return;
        }
        status_->setText(text);
        const char* toneName = tone == Tone::Good ? "good" : tone == Tone::Bad ? "bad" : "";
        if (status_->property("tone").toString() != QLatin1String(toneName)) {
            status_->setProperty("tone", QString::fromLatin1(toneName));
            status_->style()->unpolish(status_);
            status_->style()->polish(status_);
        }
        if (tone == Tone::Neutral)
            mark_->clear();
        else
            mark_->present(tone == Tone::Good ? StatusMark::Kind::Good : StatusMark::Kind::Bad,
                           animate);
        statusRow_->show();
    }

    void ListRow::setProgress(int percent)
    {
        if (percent < 0) {
            progress_->hide();
            return;
        }
        progress_->setValue(qBound(0, percent, 100));
        progress_->show();
    }

    void ListRow::setInset(int horizontal, int vertical)
    {
        row_->setContentsMargins(horizontal, vertical, horizontal, vertical);
    }

    ListGroup::ListGroup(const QString& title, QWidget* parent, Style style)
        : QFrame(parent)
        , style_(style)
    {
        setAttribute(Qt::WA_StyledBackground, true);

        if (style_ == Style::Grouped) {
            setObjectName(QStringLiteral("listGroupOuter"));
            auto* outer = new QVBoxLayout(this);
            outer->setContentsMargins(0, 0, 0, 0);
            outer->setSpacing(tokens::Space8);
            if (!title.isEmpty()) {
                auto* head = new QLabel(title, this);
                head->setObjectName(QStringLiteral("listGroupHeading"));
                head->setContentsMargins(tokens::Space4, 0, 0, 0);
                outer->addWidget(head);
            }
            body_ = new QFrame(this);
            body_->setObjectName(QStringLiteral("listGroup"));
            body_->setAttribute(Qt::WA_StyledBackground, true);
            outer->addWidget(body_);
            lay_ = new QVBoxLayout(body_);
            lay_->setContentsMargins(0, tokens::Space4, 0, tokens::Space4);
            lay_->setSpacing(0);
            return;
        }

        body_ = this;
        lay_ = new QVBoxLayout(this);
        setObjectName(QStringLiteral("listGroup"));
        // Rows carry their own horizontal inset; the card adds only enough that
        // a row's hairline does not touch the border.
        lay_->setContentsMargins(tokens::Space4, tokens::Space4, tokens::Space4, tokens::Space4);
        lay_->setSpacing(0);

        if (!title.isEmpty()) {
            auto* head = new QLabel(title, this);
            head->setObjectName(QStringLiteral("listGroupTitle"));
            head->setContentsMargins(tokens::RowInsetH, tokens::Space8,
                                     tokens::RowInsetH, tokens::Space4);
            lay_->addWidget(head);
        }
    }

    ListRow* ListGroup::addRow(const QString& title, const QString& description)
    {
        if (rows_ > 0) {
            auto* rule = new QFrame(body_);
            rule->setObjectName(QStringLiteral("srRowDivider"));
            rule->setAttribute(Qt::WA_StyledBackground, true);
            auto* holder = new QHBoxLayout;
            const int inset = style_ == Style::Grouped ? tokens::Space16 : tokens::RowInsetH;
            holder->setContentsMargins(inset, 0, inset, 0);
            holder->addWidget(rule);
            lay_->addLayout(holder);
        }
        auto* row = new ListRow(title, description, body_);
        if (style_ == Style::Grouped)
            row->setInset(tokens::Space16, tokens::Space12);
        lay_->addWidget(row);
        ++rows_;
        return row;
    }
}
