#include "pch.h"
#include "Dropdown.h"

#include "../Theme/DesignTokens.h"
#include "../Utils/TablerIcon.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QEvent>
#include <QMouseEvent>
#include <QPushButton>
#include <QScreen>
#include <QStyle>
#include <QVBoxLayout>

namespace vth::ui
{
    namespace
    {
        namespace tokens = vth::theme::tokens;

        // A press on the trigger that dismissed an open popup arrives afterwards.
        constexpr int kReopenGuardMs = 200;
    }

    Dropdown::Dropdown(QWidget* parent) : QWidget(parent)
    {
        setObjectName(QStringLiteral("vethDropdown"));
        setAttribute(Qt::WA_StyledBackground, true);
        setCursor(Qt::PointingHandCursor);
        setFocusPolicy(Qt::StrongFocus);

        auto* lay = new QHBoxLayout(this);
        lay->setContentsMargins(tokens::RowInsetH, 0, tokens::CompactGap, 0);
        lay->setSpacing(tokens::CompactGap);

        valueLabel_ = new QLabel(this);
        valueLabel_->setObjectName(QStringLiteral("vethDropdownValue"));
        lay->addWidget(valueLabel_, 1);

        chevron_ = new QLabel(this);
        chevron_->setFixedSize(tokens::GlyphMd, tokens::GlyphMd);
        refreshChevron();
        lay->addWidget(chevron_, 0);

        setMinimumHeight(tokens::ControlHeight);
    }

    void Dropdown::addItem(const QString& text, const QVariant& data)
    {
        items_.push_back({text, data});
        invalidatePopup();
        if (current_ < 0) setCurrentIndex(0);
    }

    void Dropdown::setItems(const QStringList& texts)
    {
        items_.clear();
        for (const QString& t : texts) items_.push_back({t, QVariant()});
        invalidatePopup();
        current_ = -1;
        if (!items_.isEmpty()) setCurrentIndex(0);
    }

    void Dropdown::invalidatePopup()
    {
        if (!popup_) return;
        popup_->deleteLater();
        popup_ = nullptr;
        rows_.clear();
    }

    QString Dropdown::currentText() const
    {
        return (current_ >= 0 && current_ < items_.size()) ? items_.at(current_).text : QString();
    }

    QVariant Dropdown::currentData() const
    {
        return (current_ >= 0 && current_ < items_.size()) ? items_.at(current_).data : QVariant();
    }

    void Dropdown::setCurrentIndex(int index)
    {
        if (index < 0 || index >= items_.size()) return;
        current_ = index;
        updateValueText();
    }

    // Elides long adapter and device names, with the full text in the tooltip.
    void Dropdown::updateValueText()
    {
        if (!valueLabel_) return;
        const QString full = currentText();
        const QString elided = valueLabel_->fontMetrics().elidedText(
            full, Qt::ElideRight, qMax(0, valueLabel_->width() - 2));
        valueLabel_->setText(elided);
        setToolTip(elided == full ? QString() : full);

        setAccessibleDescription(full);
    }

    void Dropdown::resizeEvent(QResizeEvent* e)
    {
        QWidget::resizeEvent(e);
        updateValueText();
    }

    void Dropdown::choose(int index)
    {
        if (popup_) popup_->hide();
        if (index == current_ || index < 0 || index >= items_.size()) return;
        setCurrentIndex(index);
        Q_EMIT currentIndexChanged(index);
    }

    void Dropdown::mousePressEvent(QMouseEvent* e)
    {
        if (e->button() != Qt::LeftButton) {
            QWidget::mousePressEvent(e);
            return;
        }
        // the click that closed the popup also lands here, don't reopen it
        if (popupClosedAt_.isValid() && popupClosedAt_.elapsed() < kReopenGuardMs) {
            popupClosedAt_.invalidate();
            return;
        }
        showPopup();
    }

    void Dropdown::keyPressEvent(QKeyEvent* e)
    {
        switch (e->key()) {
        case Qt::Key_Space:
        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_Down:
            showPopup();
            break;
        default:
            QWidget::keyPressEvent(e);
        }
    }

    QSize Dropdown::sizeHint() const
    {
        return { 220, 40 };
    }

    void Dropdown::changeEvent(QEvent* e)
    {
        QWidget::changeEvent(e);
        if (e->type() == QEvent::EnabledChange)
            setCursor(isEnabled() ? Qt::PointingHandCursor : Qt::ArrowCursor);

        if (e->type() == QEvent::ApplicationPaletteChange) {
            if (isVisible())
                refreshChevron();
            else
                chevronStale_ = true;
        }
    }

    void Dropdown::showEvent(QShowEvent* e)
    {
        QWidget::showEvent(e);
        if (chevronStale_) {
            chevronStale_ = false;
            refreshChevron();
        }
    }

    void Dropdown::refreshChevron()
    {
        if (!chevron_)
            return;
        chevron_->setPixmap(vth::icons::glyphPixmap(
            QStringLiteral("chevron-down"),
            vth::icons::iconColor(), tokens::GlyphMd, chevron_));
    }

    void Dropdown::showPopup()
    {
        if (items_.isEmpty()) return;

        if (!popup_) {
            // translucent for the rounded corners, an inner frame paints the background
            popup_ = new QFrame(this, Qt::Popup | Qt::FramelessWindowHint
                                          | Qt::NoDropShadowWindowHint);
            popup_->setObjectName(QStringLiteral("vethDropdownContainer"));
            popup_->setAttribute(Qt::WA_TranslucentBackground, true);
            popup_->setFrameShape(QFrame::NoFrame);
            popup_->installEventFilter(this);

            auto* outer = new QVBoxLayout(popup_);
            outer->setContentsMargins(0, 0, 0, 0);

            auto* surface = new QFrame(popup_);
            surface->setObjectName(QStringLiteral("vethDropdownPopup"));
            outer->addWidget(surface);

            auto* pl = new QVBoxLayout(surface);
            pl->setContentsMargins(tokens::CompactGap, tokens::CompactGap,
                                   tokens::CompactGap, tokens::CompactGap);
            pl->setSpacing(tokens::Space2);
            rows_.clear();
            for (int i = 0; i < items_.size(); ++i) {
                auto* item = new QPushButton(items_.at(i).text, surface);
                item->setObjectName(QStringLiteral("vethDropdownItem"));
                item->setCursor(Qt::PointingHandCursor);
                item->setFlat(true);
                item->setFocusPolicy(Qt::StrongFocus);
                item->installEventFilter(this);
                connect(item, &QPushButton::clicked, this, [this, i] { choose(i); });
                pl->addWidget(item);
                rows_.push_back(item);
            }
        }

        for (int i = 0; i < rows_.size(); ++i) {
            rows_.at(i)->setProperty("selected", i == current_);
            rows_.at(i)->style()->unpolish(rows_.at(i));
            rows_.at(i)->style()->polish(rows_.at(i));
        }

        popup_->setFixedWidth(qMax(width(), 200));
        popup_->adjustSize();
        QPoint pos = mapToGlobal(QPoint(0, height() + 4));
        if (const QScreen* scr = screen();
            scr && pos.y() + popup_->height() > scr->availableGeometry().bottom())
            pos = mapToGlobal(QPoint(0, -popup_->height() - 4));
        popup_->move(pos);
        popup_->show();

        // Focus the current row so arrows/Enter work without a mouse; Qt::Popup self-closes on Escape.
        focusRow(current_ >= 0 ? current_ : 0);
    }

    void Dropdown::focusRow(int index)
    {
        if (index < 0 || index >= rows_.size())
            return;
        rows_.at(index)->setFocus(Qt::TabFocusReason);
    }

    bool Dropdown::eventFilter(QObject* obj, QEvent* e)
    {
        if (obj == popup_ && e->type() == QEvent::Hide) {
            popupClosedAt_.start();
            return false;
        }
        if (e->type() == QEvent::KeyPress && !rows_.isEmpty()) {
            const int here = rows_.indexOf(qobject_cast<QPushButton*>(obj));
            if (here >= 0) {
                auto* ke = static_cast<QKeyEvent*>(e);
                switch (ke->key()) {
                case Qt::Key_Down:
                    focusRow((here + 1) % rows_.size());
                    return true;
                case Qt::Key_Up:
                    focusRow((here - 1 + rows_.size()) % rows_.size());
                    return true;
                case Qt::Key_Home:
                    focusRow(0);
                    return true;
                case Qt::Key_End:
                    focusRow(rows_.size() - 1);
                    return true;
                case Qt::Key_Return:
                case Qt::Key_Enter:
                    rows_.at(here)->click();
                    return true;
                default:
                    break;
                }
            }
        }
        return QWidget::eventFilter(obj, e);
    }
}
