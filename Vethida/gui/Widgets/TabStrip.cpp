#include "pch.h"
#include "TabStrip.h"

#include "AccentPill.h"

#include "../Theme/DesignTokens.h"
#include "../Theme/Theme.h"
#include "../Utils/TablerIcon.h"

#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPropertyAnimation>
#include <QScrollArea>
#include <QTimer>
#include <QShowEvent>
#include <QStackedWidget>
#include <QStyle>
#include <QVBoxLayout>

#include <utility>

namespace vth::widgets
{
    namespace pal    = vth::theme::palette;
    namespace tokens = vth::theme::tokens;

    namespace
    {
        constexpr int kTabGlyph = 18;
    }

    // One tab. The underline is the strip's sliding AccentPill.
    class TabStrip::Tab : public QFrame
    {
    public:
        Tab(TabStrip* owner, int index, QString iconName, const QString& title,
            QWidget* parent)
            : QFrame(parent), owner_(owner), index_(index),
              iconName_(std::move(iconName))
        {
            setObjectName(QStringLiteral("tabStripItem"));
            setAttribute(Qt::WA_StyledBackground, true);
            setAttribute(Qt::WA_Hover, true);
            setCursor(Qt::PointingHandCursor);
            setFocusPolicy(Qt::StrongFocus);

            auto* lay = new QHBoxLayout(this);
            lay->setContentsMargins(tokens::RowInsetH, tokens::RowInsetV,
                                    tokens::RowInsetH, tokens::RowInsetV);
            lay->setSpacing(tokens::CompactGap);

            // Centered, because the tabs divide the bar between them.
            lay->addStretch(1);

            icon_ = new QLabel(this);
            icon_->setFixedSize(kTabGlyph, kTabGlyph);
            lay->addWidget(icon_, 0, Qt::AlignVCenter);

            title_ = new QLabel(title, this);
            title_->setObjectName(QStringLiteral("tabStripTitle"));
            lay->addWidget(title_, 0, Qt::AlignVCenter);

            lay->addStretch(1);

            refreshIcon();
        }

        // The glyph and the title together, in the tab's coordinates: the ink
        // spans what the tab says, not the empty width it was stretched to.
        QRect contentRect() const
        {
            return icon_->geometry().united(title_->geometry());
        }

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

    protected:
        // The tab glyph is a baked pixmap, so it doesn't follow the stylesheet.
        void changeEvent(QEvent* event) override
        {
            QFrame::changeEvent(event);
            if (event->type() == QEvent::ApplicationPaletteChange)
                refreshIcon();
        }

        void mousePressEvent(QMouseEvent*) override
        {
            owner_->setCurrentIndex(index_);
        }

        void keyPressEvent(QKeyEvent* e) override
        {
            switch (e->key()) {
            case Qt::Key_Space:
            case Qt::Key_Return:
            case Qt::Key_Enter:  owner_->setCurrentIndex(index_);        return;
            case Qt::Key_Left:   focusSibling(index_ - 1);               return;
            case Qt::Key_Right:  focusSibling(index_ + 1);               return;
            case Qt::Key_Home:   focusSibling(0);                        return;
            case Qt::Key_End:    focusSibling(owner_->tabs_.size() - 1); return;
            default: break;
            }
            QFrame::keyPressEvent(e);
        }

    private:
        void focusSibling(int i)
        {
            const auto& tabs = owner_->tabs_;
            if (i >= 0 && i < tabs.size())
                tabs[i]->setFocus(Qt::TabFocusReason);
        }

        void refreshIcon()
        {
            const QColor c = selected_ ? pal::accentOnSurface() : pal::textMuted();
            icon_->setPixmap(vth::icons::glyphPixmap(iconName_, c, kTabGlyph, icon_));
        }

        TabStrip* owner_ = nullptr;
        int       index_ = 0;
        QString   iconName_;
        QLabel*   icon_  = nullptr;
        QLabel*   title_ = nullptr;
        bool      selected_ = false;
    };

    TabStrip::TabStrip(QWidget* parent) : QWidget(parent)
    {
        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(0, 0, 0, 0);
        root->setSpacing(0);

        // The bar spans the page and carries the rule under the tabs. The inset is
        // reduced by the tab's own padding, so the text lines up with the page title.
        auto* bar = new QWidget(this);
        bar->setObjectName(QStringLiteral("tabStripBar"));
        bar->setAttribute(Qt::WA_StyledBackground, true);
        tabLay_ = new QHBoxLayout(bar);
        tabLay_->setContentsMargins(tokens::PageMarginH - tokens::RowInsetH, 0,
                                    tokens::PageMarginH - tokens::RowInsetH, 0);
        tabLay_->setSpacing(0);
        root->addWidget(bar);
        bar_ = bar;
        bar_->installEventFilter(this);

        ink_ = new AccentPill(bar_);
        ink_->hide();
        inkAnim_ = new QPropertyAnimation(ink_, "geometry", this);
        inkAnim_->setEasingCurve(QEasingCurve::OutCubic);

        stack_ = new QStackedWidget(this);
        root->addWidget(stack_, 1);
    }

    int TabStrip::addTab(const QString& iconName, const QString& title, Factory factory,
                         PaneScroll scroll)
    {
        const int index = tabs_.size();
        auto* tab = new Tab(this, index, iconName, title, tabLay_->parentWidget());
        tabLay_->insertWidget(index, tab, 1);
        tabs_.push_back(tab);

        Pane pane;
        pane.factory = std::move(factory);
        pane.host = new QWidget;
        auto* hostLay = new QVBoxLayout(pane.host);
        hostLay->setContentsMargins(0, 0, 0, 0);
        hostLay->setSpacing(0);

        // Managed = we wrap it in a scroll area, Self = it scrolls itself
        if (scroll == PaneScroll::Managed) {
            auto* paneScroll = new QScrollArea(stack_);
            paneScroll->setObjectName(QStringLiteral("settingsScroll"));
            paneScroll->setWidgetResizable(true);
            paneScroll->setFrameShape(QFrame::NoFrame);
            paneScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

            auto* insetHost = new QWidget(paneScroll);
            insetHost->setObjectName(QStringLiteral("scrollContentHost"));
            auto* insetLay = new QVBoxLayout(insetHost);
            insetLay->setContentsMargins(tokens::PageMarginH, tokens::PageMarginTop,
                                         tokens::PageMarginH, tokens::PageMarginBottom);
            insetLay->setSpacing(0);
            // A short pane sits at the top.
            insetLay->addWidget(pane.host);
            insetLay->addStretch(1);

            paneScroll->setWidget(insetHost);
            stack_->addWidget(paneScroll);
        } else {
            stack_->addWidget(pane.host);
        }

        panes_.push_back(pane);

        if (index == 0) {
            tab->setSelected(true);
            stack_->setCurrentIndex(0);
        }
        return index;
    }

    void TabStrip::showEvent(QShowEvent* e)
    {
        QWidget::showEvent(e);
        if (firstShown_)
            return;
        firstShown_ = true;
        // setCurrentIndex does not fire for the opening tab (the stack already
        // sits on index 0), so without this the page opens on an empty pane.
        ensureBuilt(stack_->currentIndex());
        QTimer::singleShot(0, this, [this]() { moveInk(false); });
    }

    bool TabStrip::eventFilter(QObject* watched, QEvent* event)
    {
        // The tabs share the bar's width, so any resize moves them; the ink
        // follows without animating.
        if (watched == bar_ && event->type() == QEvent::Resize)
            QTimer::singleShot(0, this, [this]() { moveInk(false); });
        return QWidget::eventFilter(watched, event);
    }

    void TabStrip::moveInk(bool animate)
    {
        const int index = stack_->currentIndex();
        if (!ink_ || index < 0 || index >= tabs_.size()) {
            if (ink_)
                ink_->hide();
            return;
        }
        const Tab* tab = tabs_.at(index);
        const QRect content = tab->contentRect().translated(tab->pos());
        constexpr int kOverhang = tokens::Space4;
        const int thickness = tokens::TabUnderlineThickness;
        const QRect target(content.left() - kOverhang, bar_->height() - thickness,
                           content.width() + 2 * kOverhang, thickness);
        inkAnim_->stop();
        if (animate && ink_->isVisible() && vth::theme::animationsEnabled()) {
            inkAnim_->setDuration(vth::theme::animationDuration(220));
            inkAnim_->setStartValue(ink_->geometry());
            inkAnim_->setEndValue(target);
            inkAnim_->start();
        } else {
            ink_->setGeometry(target);
        }
        ink_->show();
        ink_->raise();
    }

    void TabStrip::ensureBuilt(int index)
    {
        if (index < 0 || index >= panes_.size())
            return;
        Pane& pane = panes_[index];
        if (pane.built || !pane.factory || !pane.host)
            return;
        pane.built = true;
        pane.host->layout()->addWidget(pane.factory(pane.host));
    }

    void TabStrip::setCurrentIndex(int index)
    {
        if (index < 0 || index >= tabs_.size())
            return;
        for (int i = 0; i < tabs_.size(); ++i)
            tabs_[i]->setSelected(i == index);
        // Built before the switch, so the pane is never shown empty for a frame.
        ensureBuilt(index);
        if (stack_->currentIndex() != index) {
            stack_->setCurrentIndex(index);
            moveInk(true);
            Q_EMIT currentChanged(index);
        }
    }

    int TabStrip::currentIndex() const
    {
        return stack_->currentIndex();
    }

    int TabStrip::count() const
    {
        return tabs_.size();
    }
}
