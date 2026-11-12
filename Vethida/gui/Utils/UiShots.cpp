#include "pch.h"
#include "UiShots.h"

#ifdef VETHIDA_DEV_BUILD

#include <QAbstractButton>
#include <QApplication>
#include <QDialog>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QScrollBar>
#include <QMouseEvent>
#include <QPointer>
#include <QTextStream>
#include <QTimer>
#include <QWidget>

#include "../Widgets/HealthScoreGauge.h"

#include "../../vth_core/Runtime/ServiceRegistry.h"
#include "../../vth_core/Services/LicenseService.h"

namespace vth::dev
{
    namespace
    {
        QString textOf(QWidget* w)
        {
            if (auto* b = qobject_cast<QAbstractButton*>(w))
                if (!b->text().isEmpty())
                    return b->text();
            if (auto* l = qobject_cast<QLabel*>(w))
                if (!l->text().isEmpty())
                    return l->text();
            if (auto* e = qobject_cast<QLineEdit*>(w))
                return e->placeholderText();
            return QString();
        }

        bool matches(QWidget* w, const QString& want)
        {
            const auto eq = [&](const QString& s) {
                return !s.isEmpty() && s.trimmed().compare(want, Qt::CaseInsensitive) == 0;
            };
            return eq(textOf(w)) || eq(w->accessibleName()) || eq(w->toolTip());
        }

        QWidget* find(const QString& want)
        {
            // Last top-level first: a modal is shown after the window it covers.
            const QWidgetList tops = QApplication::topLevelWidgets();
            for (int i = tops.size() - 1; i >= 0; --i) {
                QWidget* top = tops.at(i);
                if (!top->isVisible())
                    continue;
                const QList<QWidget*> all = top->findChildren<QWidget*>();
                // Reverse: an overlay built later sits above the page it covers.
                for (int j = all.size() - 1; j >= 0; --j) {
                    QWidget* w = all.at(j);
                    if (w->isVisible() && matches(w, want))
                        return w;
                }
            }
            return nullptr;
        }

        void clickWidget(QWidget* w)
        {
            // A label is clicked through its parent, which is what owns the
            // mouse handling on a composed row.
            QWidget* target = w;
            if (qobject_cast<QLabel*>(w) && w->parentWidget())
                target = w->parentWidget();
            if (auto* b = qobject_cast<QAbstractButton*>(target)) {
                b->click();
                return;
            }
            const QPointF c = QRectF(target->rect()).center();
            const QPointF g = target->mapToGlobal(c);
            QMouseEvent press(QEvent::MouseButtonPress, c, g, Qt::LeftButton,
                              Qt::LeftButton, Qt::NoModifier);
            QMouseEvent release(QEvent::MouseButtonRelease, c, g, Qt::LeftButton,
                                Qt::NoButton, Qt::NoModifier);
            QApplication::sendEvent(target, &press);
            QApplication::sendEvent(target, &release);
        }

        class Runner : public QObject
        {
        public:
            Runner(QWidget* window, QStringList steps, QString out)
                : QObject(window), window_(window), steps_(std::move(steps)), out_(std::move(out))
            {
                QDir().mkpath(out_);
                // log UI stalls over 100ms with the current step
                beat_.start();
                auto* hb = new QTimer(this);
                hb->setInterval(16);
                connect(hb, &QTimer::timeout, this, [this] {
                    const qint64 gap = beat_.restart();
                    if (gap > 100)
                        log(QStringLiteral("STALL %1 ms during: %2").arg(gap).arg(current_));
                });
                hb->start();
                FILETIME c{}, e{}, k{}, u{};
                if (GetProcessTimes(GetCurrentProcess(), &c, &e, &k, &u)) {
                    ULARGE_INTEGER t; t.LowPart = c.dwLowDateTime; t.HighPart = c.dwHighDateTime;
                    const qint64 startMs = static_cast<qint64>(t.QuadPart / 10000ULL) - 11644473600000LL;
                    log(QStringLiteral("window shown %1 ms after process start")
                            .arg(QDateTime::currentMSecsSinceEpoch() - startMs));
                }
            }

            void next(int delayMs = 50)
            {
                QTimer::singleShot(delayMs, this, [this] { step(); });
            }

        private:
            void log(const QString& line)
            {
                QFile f(out_ + QStringLiteral("/log.txt"));
                if (f.open(QIODevice::Append | QIODevice::Text))
                    QTextStream(&f) << line << '\n';
            }

            void step()
            {
                if (index_ >= steps_.size()) {
                    log(QStringLiteral("done"));
                    QApplication::exit(0);
                    return;
                }
                const QString line = steps_.at(index_++).trimmed();
                current_ = line;
                if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) {
                    next(0);
                    return;
                }
                const QString cmd = line.section(QLatin1Char(' '), 0, 0).toLower();
                const QString arg = line.section(QLatin1Char(' '), 1).trimmed();

                if (cmd == QLatin1String("wait")) {
                    next(arg.toInt());
                    return;
                }
                if (cmd == QLatin1String("clickin")) {
                    // clickin <anchor text> | <button text>, nearest matching button
                    next(400);
                    const QString anchorText = arg.section(QLatin1Char('|'), 0, 0).trimmed();
                    const QString buttonText = arg.section(QLatin1Char('|'), 1).trimmed();
                    QWidget* anchor = find(anchorText);
                    for (QWidget* p = anchor ? anchor->parentWidget() : nullptr; p; p = p->parentWidget()) {
                        for (QAbstractButton* b : p->findChildren<QAbstractButton*>()) {
                            if (b->isVisible() && matches(b, buttonText)) {
                                log(QStringLiteral("clickin ok: ") + arg);
                                b->click();
                                return;
                            }
                        }
                    }
                    log(QStringLiteral("clickin MISSING: ") + arg);
                    return;
                }
                if (cmd == QLatin1String("click")) {
                    // before the click, a modal's exec() blocks until the next step closes it
                    next(400);
                    if (QWidget* w = find(arg)) {
                        log(QStringLiteral("click ok: ") + arg);
                        clickWidget(w);
                    } else {
                        log(QStringLiteral("click MISSING: ") + arg);
                    }
                    return;
                }
                if (cmd == QLatin1String("shot")) {
                    if (window_)
                        window_->grab().save(out_ + QLatin1Char('/') + arg + QStringLiteral(".png"));
                    // A real top-level dialog is not part of the window's grab.
                    int n = 2;
                    for (QWidget* top : QApplication::topLevelWidgets())
                        if (top != window_ && top->isVisible() && top->isWindow()
                            && top->width() > 50)
                            top->grab().save(QStringLiteral("%1/%2_%3.png").arg(out_, arg).arg(n++));
                    log(QStringLiteral("shot ") + arg);
                    next(50);
                    return;
                }
                if (cmd == QLatin1String("key")) {
                    const int key = arg == QLatin1String("enter") ? Qt::Key_Return : Qt::Key_Escape;
                    QWidget* target = QApplication::activeModalWidget();
                    // The in-window panels are child dialogs, which are never
                    // the application's active modal widget.
                    if (!target && window_) {
                        const auto dialogs = window_->findChildren<QDialog*>();
                        for (auto it = dialogs.crbegin(); it != dialogs.crend() && !target; ++it)
                            if ((*it)->isVisible())
                                target = *it;
                    }
                    if (!target)
                        target = QApplication::focusWidget();
                    if (!target)
                        target = window_;
                    QKeyEvent press(QEvent::KeyPress, key, Qt::NoModifier);
                    QKeyEvent release(QEvent::KeyRelease, key, Qt::NoModifier);
                    QApplication::sendEvent(target, &press);
                    QApplication::sendEvent(target, &release);
                    next(400);
                    return;
                }
                if (cmd == QLatin1String("size")) {
                    if (window_)
                        window_->resize(arg.section(QLatin1Char(' '), 0, 0).toInt(),
                                        arg.section(QLatin1Char(' '), 1, 1).toInt());
                    next(300);
                    return;
                }
                if (cmd == QLatin1String("type")) {
                    // type <text>: one key at a time into the focused widget
                    QWidget* target = QApplication::focusWidget();
                    if (target) {
                        for (const QChar ch : arg) {
                            QKeyEvent press(QEvent::KeyPress, 0, Qt::NoModifier, QString(ch));
                            QKeyEvent release(QEvent::KeyRelease, 0, Qt::NoModifier, QString(ch));
                            QApplication::sendEvent(target, &press);
                            QApplication::sendEvent(target, &release);
                            QApplication::processEvents();
                        }
                    }
                    log(QStringLiteral("typed: ") + arg);
                    next(300);
                    return;
                }
                if (cmd == QLatin1String("focus")) {
                    if (QWidget* w = find(arg)) {
                        w->setFocus(Qt::OtherFocusReason);
                        log(QStringLiteral("focus ok: ") + arg);
                    } else {
                        log(QStringLiteral("focus MISSING: ") + arg);
                    }
                    next(200);
                    return;
                }
                if (cmd == QLatin1String("scroll")) {
                    // scroll: every visible scroll area to the bottom, in steps.
                    for (QWidget* top : QApplication::topLevelWidgets())
                        for (auto* a : top->findChildren<QScrollArea*>())
                            if (a->isVisible() && a->verticalScrollBar()->maximum() > 0) {
                                QScrollBar* bar = a->verticalScrollBar();
                                for (int v = bar->value(); v < bar->maximum(); v += 120) {
                                    bar->setValue(v);
                                    QApplication::processEvents();
                                }
                            }
                    next(300);
                    return;
                }
                if (cmd == QLatin1String("license")) {
                    // license <state>: free, pro, ultimate, expired, revoked,
                    // offline, unverified or notactive. Nothing is saved.
                    if (auto* svc = vth::ServiceRegistry::instance()
                                        .get<vth::services::LicenseService>())
                        svc->devSimulate(arg);
                    next(600);
                    return;
                }
                if (cmd == QLatin1String("score")) {
                    // score <n>: a representative reading.
                    for (QWidget* top : QApplication::topLevelWidgets())
                        for (auto* g : top->findChildren<vth::widgets::HealthScoreGauge*>())
                            g->setScore(arg.toInt());
                    next(1200);
                    return;
                }
                if (cmd == QLatin1String("dump")) {
                    // dump <objectName>: every visible widget of that name, with
                    // its geometry and what sits inside it.
                    for (QWidget* top : QApplication::topLevelWidgets())
                        for (QWidget* w : top->findChildren<QWidget*>(arg)) {
                            if (!w->isVisible())
                                continue;
                            QString line = QStringLiteral("%1 %2,%3 %4x%5 hint %6x%7")
                                .arg(arg).arg(w->x()).arg(w->y()).arg(w->width())
                                .arg(w->height()).arg(w->sizeHint().width())
                                .arg(w->sizeHint().height());
                            for (QWidget* c : w->findChildren<QWidget*>(Qt::FindDirectChildrenOnly))
                                line += QStringLiteral(" | %1[%2] %3,%4 %5x%6 vis=%7")
                                    .arg(QString::fromLatin1(c->metaObject()->className()),
                                         textOf(c))
                                    .arg(c->x()).arg(c->y()).arg(c->width()).arg(c->height())
                                    .arg(c->isVisible());
                            log(line);
                        }
                    next(0);
                    return;
                }
                if (cmd == QLatin1String("quit")) {
                    log(QStringLiteral("quit"));
                    QApplication::exit(0);
                    return;
                }
                log(QStringLiteral("unknown step: ") + line);
                next(0);
            }

            QElapsedTimer     beat_;
            QString           current_ = QStringLiteral("startup");
            QPointer<QWidget> window_;
            QStringList       steps_;
            QString           out_;
            int               index_ = 0;
        };
    }

    void startUiShots(QWidget* mainWindow)
    {
        const QString script = qEnvironmentVariable("VETHIDA_UI_SHOT");
        if (script.isEmpty())
            return;
        QFile f(script);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
            return;
        const QStringList steps = QString::fromUtf8(f.readAll()).split(QLatin1Char('\n'));
        QString out = qEnvironmentVariable("VETHIDA_UI_SHOT_OUT");
        if (out.isEmpty())
            out = QDir::tempPath() + QStringLiteral("/vethida-shots");
        auto* runner = new Runner(mainWindow, steps, out);
        runner->next(1500);
    }
}

#else

namespace vth::dev
{
    void startUiShots(QWidget*) {}
}

#endif
