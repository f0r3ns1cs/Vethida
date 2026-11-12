#include "pch.h"
#include "ScrimDialog.h"

#include "../Theme/DesignTokens.h"
#include "../Theme/Theme.h"

#include <QApplication>
#include <QColor>
#include <QEvent>
#include <QEventLoop>
#include <QGraphicsOpacityEffect>
#include <QKeyEvent>
#include <QPainter>
#include <QPointer>
#include <QPropertyAnimation>
#include <QVBoxLayout>

namespace vth::widgets
{
    ScrimDialog::ScrimDialog(QWidget* card, QWidget* host)
        : QDialog(host ? host->window() : host)
        , host_(host ? host->window() : nullptr)
    {
        // Embed as a child overlay of the top-level window (not a separate
        // window), so its geometry tracks the window atomically.
        setWindowFlags(Qt::Widget);
        setAttribute(Qt::WA_NoSystemBackground, true);

        auto* lay = new QVBoxLayout(this);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->addWidget(card, 0, Qt::AlignCenter);

        if (host_) {
            coverHost();
            host_->installEventFilter(this);
        }
    }

    int ScrimDialog::exec()
    {
        QPointer<QWidget> restoreTo(QApplication::focusWidget());
        show();
        raise();
        fadeIn();

        if (!focusFirstControl())
            setFocus();

        QEventLoop loop;
        connect(this, &QDialog::finished, &loop, [&loop]() { loop.quit(); });
        loop.exec();
        if (restoreTo)
            restoreTo->setFocus(Qt::OtherFocusReason);
        return result();
    }

    // keep Tab inside the card (child dialogs don't get modal focus handling)
    bool ScrimDialog::focusNextPrevChild(bool next)
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

    void ScrimDialog::paintEvent(QPaintEvent*)
    {
        QPainter painter(this);
        painter.fillRect(rect(), vth::theme::palette::scrimOverlay());
    }

    bool ScrimDialog::eventFilter(QObject* obj, QEvent* e)
    {
        if (obj == host_ && e->type() == QEvent::Resize)
            coverHost();
        return QDialog::eventFilter(obj, e);
    }

    void ScrimDialog::keyPressEvent(QKeyEvent* e)
    {
        if (e->key() == Qt::Key_Escape) {
            e->accept();
            reject();
            return;
        }
        QDialog::keyPressEvent(e);
    }

    // The effect is dropped when the fade ends, because it renders the card through an
    // offscreen pixmap for as long as it's set.
    void ScrimDialog::fadeIn()
    {
        if (!vth::theme::animationsEnabled())
            return;
        auto* fade = new QGraphicsOpacityEffect(this);
        fade->setOpacity(0.0);
        setGraphicsEffect(fade);
        auto* anim = new QPropertyAnimation(fade, "opacity", this);
        anim->setDuration(vth::theme::animationDuration(140));
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        connect(anim, &QPropertyAnimation::finished, this, [this]() {
            setGraphicsEffect(nullptr);
        });
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }

    void ScrimDialog::setInitialFocus(QWidget* widget)
    {
        initialFocus_ = widget;
    }

    bool ScrimDialog::focusFirstControl()
    {
        if (initialFocus_ && (initialFocus_->focusPolicy() & Qt::TabFocus)
            && initialFocus_->isEnabled()) {
            initialFocus_->setFocus(Qt::TabFocusReason);
            return true;
        }

        const QList<QWidget*> all = findChildren<QWidget*>();
        for (QWidget* w : all) {
            if ((w->focusPolicy() & Qt::TabFocus) && w->isEnabled() && w->isVisibleTo(this)) {
                w->setFocus(Qt::TabFocusReason);
                return true;
            }
        }
        return false;
    }

    void ScrimDialog::coverHost()
    {
        if (host_) {
            setGeometry(host_->rect());
            raise();
        }
    }
}
