#include "pch.h"
#include "SettingsList.h"

#include "SwitchButton.h"

#include "../Theme/DesignTokens.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QScrollArea>
#include <QStyle>
#include <QVBoxLayout>

namespace vth::widgets
{
    namespace
    {
        // The inset SectionNav gives a Managed pane, so content doesn't shift between
        // sections of the same rail.
        constexpr int kRowPadH        = vth::theme::tokens::RowInsetH;
        constexpr int kPaneMarginH   = vth::theme::tokens::PageMarginH - kRowPadH;
        constexpr int kPaneMarginTop = vth::theme::tokens::PageMarginTop;
        constexpr int kPaneMarginBot = vth::theme::tokens::PageMarginBottom;
        constexpr int kRowPadV        = vth::theme::tokens::Space8;
        constexpr int kRowGap         = vth::theme::tokens::Space12;
        constexpr int kTextGap        = vth::theme::tokens::Space2;
        constexpr int kHeadingAbove   = vth::theme::tokens::Space16;
        constexpr int kHeadingRuleGap = vth::theme::tokens::Space4;
        constexpr int kHeadingBelow   = vth::theme::tokens::Space4;
    }

    SettingsRow::SettingsRow(const QString& title, const QString& description,
                             QWidget* parent)
        : QFrame(parent)
    {
        setObjectName(QStringLiteral("settingsRow"));
        setAttribute(Qt::WA_StyledBackground, true);

        row_ = new QHBoxLayout(this);
        row_->setContentsMargins(kRowPadH, kRowPadV, kRowPadH, kRowPadV);
        row_->setSpacing(kRowGap);

        textCol_ = new QVBoxLayout;
        textCol_->setContentsMargins(0, 0, 0, 0);
        textCol_->setSpacing(kTextGap);

        title_ = new QLabel(title, this);
        title_->setObjectName(QStringLiteral("settingsRowTitle"));
        title_->setWordWrap(true);
        textCol_->addWidget(title_);

        desc_ = new QLabel(description, this);
        desc_->setObjectName(QStringLiteral("settingsRowDesc"));
        desc_->setWordWrap(true);
        desc_->setVisible(!description.isEmpty());
        textCol_->addWidget(desc_);

        row_->addLayout(textCol_, 1);

        toggle_ = new SwitchButton(this);
        applyAccessibleLabels();
        row_->addWidget(toggle_, 0, Qt::AlignVCenter);
    }

    void SettingsRow::applyAccessibleLabels()
    {
        if (!toggle_)
            return;
        toggle_->setAccessibleName(title_->text());
        toggle_->setAccessibleDescription(desc_->text());
    }

    void SettingsRow::setInteractive(bool on)
    {
        interactive_ = on;
        setProperty("interactive", on);
        setCursor(on ? Qt::PointingHandCursor : Qt::ArrowCursor);
        style()->unpolish(this);
        style()->polish(this);
    }

    void SettingsRow::setTitle(const QString& text)
    {
        title_->setText(text);
        applyAccessibleLabels();
    }

    void SettingsRow::setDescription(const QString& text)
    {
        desc_->setText(text);
        desc_->setVisible(!text.isEmpty());
        applyAccessibleLabels();
    }

    void SettingsRow::addTrailing(QWidget* w)
    {
        if (!w)
            return;
        w->setParent(this);
        // Before the switch, which is always the rightmost control.
        row_->insertWidget(row_->count() - 1, w, 0, Qt::AlignVCenter);
    }

    void SettingsRow::setMeta(QWidget* w)
    {
        if (meta_) {
            textCol_->removeWidget(meta_);
            meta_->deleteLater();
        }
        meta_ = w;
        if (meta_) {
            meta_->setParent(this);
            textCol_->addWidget(meta_);
        }
    }

    void SettingsRow::mousePressEvent(QMouseEvent* event)
    {
        // Child controls consume their own clicks; anything reaching the frame
        // is on the body.
        if (interactive_ && event->button() == Qt::LeftButton) {
            Q_EMIT bodyClicked();
            event->accept();
            return;
        }
        QFrame::mousePressEvent(event);
    }

    SettingsList::SettingsList(QWidget* parent)
        : QWidget(parent)
    {
        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(0, 0, 0, 0);
        root->setSpacing(0);

        scroll_ = new QScrollArea(this);
        scroll_->setObjectName(QStringLiteral("settingsScroll"));
        scroll_->setWidgetResizable(true);
        scroll_->setFrameShape(QFrame::NoFrame);
        scroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        root->addWidget(scroll_);

        content_ = new QWidget(scroll_);
        content_->setObjectName(QStringLiteral("scrollContentHost"));
        lay_ = new QVBoxLayout(content_);
        lay_->setContentsMargins(kPaneMarginH, kPaneMarginTop, kPaneMarginH, kPaneMarginBot);
        lay_->setSpacing(0);

        placeholder_ = new QLabel(content_);
        placeholder_->setObjectName(QStringLiteral("settingsRowDesc"));
        placeholder_->setWordWrap(true);
        placeholder_->setVisible(false);
        lay_->addWidget(placeholder_);

        // Everything is inserted before this, so the column stays top-aligned
        // however few rows it holds.
        lay_->addStretch(1);

        scroll_->setWidget(content_);
    }

    void SettingsList::addWidget(QWidget* w)
    {
        w->setParent(content_);
        lay_->insertWidget(lay_->count() - 1, w);
        ++itemCount_;
        refreshPlaceholder();
    }

    void SettingsList::addSpace(int px)
    {
        lay_->insertSpacing(lay_->count() - 1, px);
    }

    QWidget* SettingsList::addHeading(const QString& text)
    {
        // A muted caption over a hairline rule.
        auto* group = new QWidget(content_);
        auto* groupLay = new QVBoxLayout(group);
        // Same horizontal inset as a row, so a heading and the titles under it
        // sit on one left edge.
        groupLay->setContentsMargins(kRowPadH, itemCount_ > 0 ? kHeadingAbove : 0,
                                     kRowPadH, kHeadingBelow);
        groupLay->setSpacing(kHeadingRuleGap);

        auto* label = new QLabel(text, group);
        label->setObjectName(QStringLiteral("settingsSectionLabel"));
        groupLay->addWidget(label);

        auto* rule = new QFrame(group);
        rule->setObjectName(QStringLiteral("settingsRule"));
        rule->setAttribute(Qt::WA_StyledBackground, true);
        rule->setFixedHeight(vth::theme::tokens::RuleThickness);
        groupLay->addWidget(rule);

        lay_->insertWidget(lay_->count() - 1, group);
        ++itemCount_;
        refreshPlaceholder();
        return group;
    }

    void SettingsList::addRule()
    {
        auto* line = new QFrame(content_);
        line->setObjectName(QStringLiteral("settingsRule"));
        line->setAttribute(Qt::WA_StyledBackground, true);
        line->setFixedHeight(vth::theme::tokens::RuleThickness);
        lay_->insertWidget(lay_->count() - 1, line);
    }

    SettingsRow* SettingsList::addRow(const QString& title, const QString& description)
    {
        auto* row = new SettingsRow(title, description, content_);
        lay_->insertWidget(lay_->count() - 1, row);
        ++itemCount_;
        refreshPlaceholder();
        return row;
    }

    void SettingsList::setPlaceholder(const QString& text)
    {
        placeholderText_ = text;
        placeholder_->setText(text);
        refreshPlaceholder();
    }

    void SettingsList::refreshPlaceholder()
    {
        placeholder_->setVisible(itemCount_ == 0 && !placeholderText_.isEmpty());
    }

    void SettingsList::clear()
    {
        // Walk backwards over everything except the placeholder and the trailing
        // stretch, deleting widgets and dropping spacers.
        for (int i = lay_->count() - 2; i >= 1; --i) {
            QLayoutItem* item = lay_->takeAt(i);
            if (QWidget* w = item->widget())
                w->deleteLater();
            delete item;
        }
        itemCount_ = 0;
        refreshPlaceholder();
    }
}
