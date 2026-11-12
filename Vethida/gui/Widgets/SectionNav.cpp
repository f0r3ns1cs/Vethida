#include "pch.h"
#include "SectionNav.h"

#include "../Theme/DesignTokens.h"
#include "../Theme/Theme.h"
#include "../Utils/TablerIcon.h"
#include "TierLockOverlay.h"

#include "../../vth_core/Runtime/Tier.h"

#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMetaObject>
#include <QScrollArea>
#include <QShowEvent>
#include <QStackedWidget>
#include <QStyle>
#include <QVBoxLayout>

#include <utility>

namespace vth::widgets
{
    namespace pal    = vth::theme::palette;
    namespace tokens = vth::theme::tokens;

    void SectionNav::applySelfPaneInset(QLayout* layout)
    {
        if (layout)
            layout->setContentsMargins(tokens::PageMarginH, tokens::PageMarginTop,
                                       tokens::PageMarginH, tokens::PageMarginBottom);
    }

    // One rail entry. Selection is a dynamic "selected" property so the shared
    // stylesheet paints the active state; a plain QFrame keeps this out of moc.
    class SectionNav::RailItem : public QFrame
    {
    public:
        bool isLocked() const   { return locked_; }
        RailItem(SectionNav* owner, int index, QString iconName,
                 const QString& title, const QString& subtitle,
                 vth::models::ProductTier minTier, QWidget* parent)
            : QFrame(parent), owner_(owner), index_(index),
              iconName_(std::move(iconName)),
              locked_(!vth::meetsTier(minTier))
        {
            setObjectName(QStringLiteral("sectionRailItem"));
            setAttribute(Qt::WA_StyledBackground, true);
            // An arrow on a section that doesn't open, and the reason on hover.
            setCursor(locked_ ? Qt::ArrowCursor : Qt::PointingHandCursor);
            if (locked_) {
                setProperty("locked", true);
                setToolTip(lockedDestinationTip(minTier));
            }
            setFocusPolicy(Qt::StrongFocus);

            auto* lay = new QHBoxLayout(this);
            lay->setContentsMargins(tokens::RowInsetH, tokens::RowInsetV,
                                    tokens::RowInsetH, tokens::RowInsetV);
            lay->setSpacing(tokens::CardMargin);

            icon_ = new QLabel(this);
            icon_->setFixedSize(tokens::GlyphLg, tokens::GlyphLg);
            lay->addWidget(icon_, 0, Qt::AlignVCenter);

            auto* col = new QVBoxLayout;
            col->setContentsMargins(0, 0, 0, 0);
            col->setSpacing(tokens::Space2);
            title_ = new QLabel(title, this);
            title_->setObjectName(QStringLiteral("sectionRailTitle"));
            if (locked_)
                title_->setProperty("locked", true);
            // Wraps, or the title sets a hard minimum width on the whole rail.
            title_->setWordWrap(true);
            subtitle_ = new QLabel(subtitle, this);
            subtitle_->setObjectName(QStringLiteral("sectionRailSubtitle"));
            subtitle_->setWordWrap(true);
            col->addWidget(title_);
            col->addWidget(subtitle_);
            lay->addLayout(col, 1);

            refreshIcon();
        }

        void setSubtitle(const QString& text) { subtitle_->setText(text); }

        void setSelected(bool on)
        {
            if (selected_ == on)
                return;
            selected_ = on;
            setProperty("selected", on);
            title_->setProperty("selected", on);
            style()->unpolish(this);
            style()->polish(this);
            title_->style()->unpolish(title_);
            title_->style()->polish(title_);
            refreshIcon();
        }

        bool isSelected() const { return selected_; }
        // What setCurrentIndex will accept: reachable on this license.
        bool isSelectable() const { return !locked_; }

    protected:
        // A rail icon is baked into a QLabel, so a theme change leaves it in the old ink.
        void changeEvent(QEvent* event) override
        {
            QFrame::changeEvent(event);
            // Deferred off screen, because this rasterizes an SVG.
            if (event->type() == QEvent::ApplicationPaletteChange) {
                if (isVisible())
                    refreshIcon();
                else
                    stale_ = true;
            }
        }

        void showEvent(QShowEvent* event) override
        {
            QFrame::showEvent(event);
            if (stale_) {
                stale_ = false;
                refreshIcon();
            }
        }

        void mousePressEvent(QMouseEvent*) override
        {
            owner_->setCurrentIndex(index_);   // refused there when locked
        }

        void keyPressEvent(QKeyEvent* e) override
        {
            switch (e->key()) {
            case Qt::Key_Space:
            case Qt::Key_Return:
            case Qt::Key_Enter:  owner_->setCurrentIndex(index_);             return;
            case Qt::Key_Up:
            case Qt::Key_Left:   focusSibling(index_ - 1);                return;
            case Qt::Key_Down:
            case Qt::Key_Right:  focusSibling(index_ + 1);                return;
            case Qt::Key_Home:   focusSibling(0);                         return;
            case Qt::Key_End:    focusSibling(owner_->items_.size() - 1); return;
            default: break;
            }
            QFrame::keyPressEvent(e);
        }

    private:
        // Moves keyboard focus to the rail item at index, when there is one.
        void focusSibling(int index)
        {
            const auto& items = owner_->items_;
            if (index >= 0 && index < items.size())
                items[index]->setFocus(Qt::TabFocusReason);
        }

        void refreshIcon()
        {
            const QColor c = locked_   ? vth::icons::disabledIconColor()
                           : selected_ ? pal::accentOnSurface()
                                       : pal::textMuted();
            icon_->setPixmap(vth::icons::glyphPixmap(
                iconName_, c, vth::icons::DefaultIconSize, icon_));
        }

        SectionNav* owner_ = nullptr;
        int         index_ = 0;
        QString     iconName_;
        bool        locked_ = false;
        // The palette changed while this item was off screen.
        bool        stale_  = false;
        QLabel*     icon_  = nullptr;
        QLabel*     title_ = nullptr;
        QLabel*     subtitle_ = nullptr;
        bool        selected_ = false;
    };

    SectionNav::SectionNav(QWidget* parent) : QWidget(parent)
    {
        auto* lay = new QHBoxLayout(this);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(0);

        // No scroll area, because the rail is a handful of destinations.
        railHost_ = new QWidget(this);
        railHost_->setObjectName(QStringLiteral("sectionRailHost"));
        railHost_->setFixedWidth(tokens::NavRailWidth);
        railLay_ = new QVBoxLayout(railHost_);
        railLay_->setContentsMargins(tokens::CardMargin, tokens::RegionMargin,
                                     tokens::CardMargin, tokens::RegionMargin);
        railLay_->setSpacing(tokens::CardMargin);
        railLay_->addStretch(1);
        railHost_->installEventFilter(this);

        auto* railCol = new QVBoxLayout;
        railCol->setContentsMargins(0, 0, 0, 0);
        railCol->setSpacing(0);
        railCol->addWidget(railHost_, 0);
        railCol->addStretch(1);
        lay->addLayout(railCol);

        auto* divider = new QWidget(this);
        divider->setObjectName(QStringLiteral("sectionRailDivider"));
        divider->setFixedWidth(tokens::RuleThickness);
        lay->addWidget(divider);

        stack_ = new QStackedWidget(this);
        lay->addWidget(stack_, 1);
    }

    int SectionNav::addSection(const QString& iconName, const QString& title,
                               const QString& subtitle, QWidget* pane,
                               PaneScroll scroll,
                               vth::models::ProductTier minTier)
    {
        const int index = items_.size();
        auto* item = new RailItem(this, index, iconName, title, subtitle, minTier,
                                  railLay_->parentWidget());
        railLay_->insertWidget(railLay_->count() - 1, item);
        items_.push_back(item);
        updateRailCap();

        // Managed panes get the rail's scroll area and inset, and carry neither of their
        // own. The host lets the shell supply the inset.
        if (scroll == PaneScroll::Managed) {
            auto* paneScroll = new QScrollArea(stack_);
            paneScroll->setObjectName(QStringLiteral("settingsScroll"));
            paneScroll->setWidgetResizable(true);
            paneScroll->setFrameShape(QFrame::NoFrame);
            paneScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

            auto* host = new QWidget(paneScroll);
            host->setObjectName(QStringLiteral("scrollContentHost"));
            auto* hostLay = new QVBoxLayout(host);
            hostLay->setContentsMargins(tokens::PageMarginH, tokens::PageMarginTop,
                                        tokens::PageMarginH, tokens::PageMarginBottom);
            hostLay->setSpacing(0);
            // A short pane sits at the top.
            hostLay->addWidget(pane);
            hostLay->addStretch(1);

            paneScroll->setWidget(host);
            stack_->addWidget(paneScroll);
        } else {
            stack_->addWidget(pane);
        }

        // The first section this license can open becomes the selected one, or a locked
        // one at the top would leave the stack on a pane nobody can use.
        bool anySelected = false;
        for (RailItem* it : items_) {
            if (it->isSelected()) {
                anySelected = true;
                break;
            }
        }
        if (!anySelected)
            setCurrentIndex(index);
        return index;
    }

    void SectionNav::updateRailCap()
    {
        if (!railHost_)
            return;

        QLayout* host = railHost_->layout();
        const int content = host && host->hasHeightForWidth()
                                ? host->totalHeightForWidth(tokens::NavRailWidth)
                                : railHost_->sizeHint().height();

        // setFixedHeight posts a layout request, which is what calls this.
        if (railHost_->maximumHeight() != content)
            railHost_->setFixedHeight(content);
    }

    bool SectionNav::eventFilter(QObject* watched, QEvent* event)
    {
        if (watched == railHost_ && event->type() == QEvent::LayoutRequest)
            updateRailCap();
        return QWidget::eventFilter(watched, event);
    }

    void SectionNav::setSubtitle(int index, const QString& text)
    {
        if (index >= 0 && index < items_.size())
            items_.at(index)->setSubtitle(text);
    }

    void SectionNav::setCurrentIndex(int index)
    {
        if (index < 0 || index >= items_.size())
            return;
        // Refused here, so every way in is gated.
        if (!items_.at(index)->isSelectable())
            return;
        for (int i = 0; i < items_.size(); ++i)
            items_[i]->setSelected(i == index);
        if (stack_->currentIndex() != index) {
            stack_->setCurrentIndex(index);
            Q_EMIT currentChanged(index);
        }
    }

    int SectionNav::currentIndex() const
    {
        return stack_->currentIndex();
    }

    int SectionNav::count() const
    {
        return items_.size();
    }

    QFrame* sectionNavCard(SectionNav* nav, QWidget* parent)
    {
        auto* card = new QFrame(parent);
        card->setObjectName(QStringLiteral("sectionCard"));
        card->setAttribute(Qt::WA_StyledBackground, true);
        auto* cardLay = new QHBoxLayout(card);
        cardLay->setContentsMargins(0, 0, 0, 0);
        cardLay->setSpacing(0);
        // A stretch, or the rail keeps its default size in the card's corner.
        cardLay->addWidget(nav, 1);
        return card;
    }

    QWidget* sectionNavHost(SectionNav* nav, QWidget* parent)
    {
        QFrame* card = sectionNavCard(nav, parent);

        auto* host = new QWidget(parent);
        auto* hostLay = new QVBoxLayout(host);
        hostLay->setContentsMargins(tokens::PageMarginH, 0,
                                    tokens::PageMarginH, tokens::PageMarginBottom);
        hostLay->setSpacing(0);
        hostLay->addWidget(card);
        return host;
    }
}

