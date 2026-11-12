#include "pch.h"
#include "PanelDialog.h"

#include "../Theme/DesignTokens.h"

#include <QApplication>
#include <QEvent>
#include <QEventLoop>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPointer>
#include <QResizeEvent>

namespace vth::widgets
{
    namespace
    {
        // A named frame of the main window, or the window itself when the
        // frame cannot be found.
        QWidget* named(QWidget* anchor, const char* name)
        {
            QWidget* win = anchor ? anchor->window() : nullptr;
            if (!win)
                return nullptr;
            if (QWidget* frame = win->findChild<QWidget*>(QString::fromLatin1(name)))
                return frame;
            return win;
        }
    }

    PanelDialog::PanelDialog(QWidget* card, QWidget* anchor, Mode mode)
        : QDialog(named(anchor, "mainRegionFrame"))
        , region_(named(anchor, "mainRegionFrame"))
        , content_(named(anchor, "contentFrame"))
        , card_(card)
        , mode_(mode)
    {
        setWindowFlags(Qt::Widget);
        setAttribute(Qt::WA_NoSystemBackground, true);

        // Placed by hand rather than by a layout: the card belongs over the
        // content area, which is only part of what the dialog covers.
        card_->setParent(this);

        if (region_) {
            coverRegion();
            region_->installEventFilter(this);
        }
    }

    int PanelDialog::exec()
    {
        QPointer<QWidget> restoreTo(QApplication::focusWidget());
        show();
        raise();
        setFocus();
        QEventLoop loop;
        connect(this, &QDialog::finished, &loop, [&loop]() { loop.quit(); });
        loop.exec();
        if (restoreTo)
            restoreTo->setFocus(Qt::OtherFocusReason);
        return result();
    }

    // Confine Tab to the card's own focusable descendants (a non-window child
    // dialog gets none of Qt's modal focus containment).
    bool PanelDialog::focusNextPrevChild(bool next)
    {
        QList<QWidget*> chain;
        const QList<QWidget*> all = findChildren<QWidget*>();
        for (QWidget* w : all)
            if ((w->focusPolicy() & Qt::TabFocus) && w->isEnabled() && w->isVisibleTo(this))
                chain.append(w);
        if (chain.isEmpty())
            return true;
        const qsizetype at = chain.indexOf(focusWidget());
        const qsizetype step = next ? 1 : -1;
        const qsizetype idx = at < 0 ? (next ? 0 : chain.size() - 1)
                                     : (at + step + chain.size()) % chain.size();
        chain.at(idx)->setFocus(next ? Qt::TabFocusReason : Qt::BacktabFocusReason);
        return true;
    }

    // Everything the card does not cover is dimmed, the sidebar included, so
    // it reads as out of reach until the panel is closed.
    void PanelDialog::paintEvent(QPaintEvent*)
    {
        QPainter painter(this);
        painter.fillRect(rect(), vth::theme::palette::scrimOverlay());
    }

    // clicking outside closes a floating panel, a full-cover one needs the button or Esc
    void PanelDialog::mousePressEvent(QMouseEvent* e)
    {
        if (card_ && !card_->geometry().contains(e->pos())) {
            if (mode_ == Mode::ScrimPanel)
                reject();
            e->accept();
            return;
        }
        QDialog::mousePressEvent(e);
    }

    void PanelDialog::keyPressEvent(QKeyEvent* e)
    {
        if (e->key() == Qt::Key_Escape) {
            reject();
            return;
        }
        QDialog::keyPressEvent(e);
    }

    bool PanelDialog::eventFilter(QObject* obj, QEvent* e)
    {
        if (obj == region_ && e->type() == QEvent::Resize)
            coverRegion();
        return QDialog::eventFilter(obj, e);
    }

    void PanelDialog::resizeEvent(QResizeEvent* e)
    {
        QDialog::resizeEvent(e);
        placeCard();
    }

    void PanelDialog::coverRegion()
    {
        if (region_) {
            setGeometry(region_->rect());
            raise();
        }
        placeCard();
    }

    // Over the content area, inset by the panel margin on every side.
    void PanelDialog::placeCard()
    {
        if (!card_)
            return;
        QRect area = rect();
        if (content_ && region_ && content_ != region_)
            area = QRect(content_->mapTo(region_, QPoint(0, 0)), content_->size());
        const int m = vth::theme::tokens::PanelModalMargin;
        card_->setGeometry(area.adjusted(m, m, -m, -m));
    }
}
