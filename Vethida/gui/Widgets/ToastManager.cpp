#include "pch.h"
#include "ToastManager.h"

#include "../Settings/Preferences.h"
#include "../Theme/DesignTokens.h"
#include "../Theme/Theme.h"
#include "../Utils/TablerIcon.h"

#include <QAbstractAnimation>
#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPointer>
#include <QPropertyAnimation>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

namespace
{
    constexpr int ToastWidth = 348;
    constexpr int ToastMargin = vth::theme::tokens::RegionMargin;
    constexpr int ToastSpacing = vth::theme::tokens::CompactGap;
    constexpr int MaxVisibleToasts = 4;

    constexpr int ToastGlyphSize = vth::theme::tokens::GlyphLg;

    constexpr int kFadeInMs  = 140;
    constexpr int kFadeOutMs = 120;

    struct ToastColors
    {
        QColor icon;
    };

    ToastColors colorsFor(vth::widgets::ToastSeverity severity)
    {
        using Severity = vth::widgets::ToastSeverity;
        namespace palette = vth::theme::palette;

        switch (severity) {
        case Severity::Warning:
            return { palette::warningText() };
        case Severity::Success:
            return { palette::scoreGoodText() };
        case Severity::Error:
            return { palette::criticalText() };
        case Severity::Info:
        default:
            return { palette::iconNeutral() };
        }
    }

    QString iconNameFor(vth::widgets::ToastSeverity severity)
    {
        using Severity = vth::widgets::ToastSeverity;

        switch (severity) {
        case Severity::Success:
            return QStringLiteral("circle-check-filled");
        case Severity::Warning:
            return QStringLiteral("triangle-alert-filled");
        case Severity::Error:
            return QStringLiteral("circle-x-filled");
        case Severity::Info:
        default:
            return QStringLiteral("info-filled");
        }
    }
}

namespace vth::widgets
{
    class ToastManager::ToastCard final : public QWidget
    {
    public:
        ToastCard(const ToastMessage& message, QWidget* parent)
            : QWidget(parent)
            , m_message(message)
            , m_colors(colorsFor(message.severity))
        {
            setAttribute(Qt::WA_DeleteOnClose);
            setAttribute(Qt::WA_StyledBackground, true);
            setObjectName(QStringLiteral("toastCard"));
            setMouseTracking(true);
            setFixedWidth(ToastWidth);

            auto* root = new QHBoxLayout(this);
            root->setContentsMargins(vth::theme::tokens::CardInsetH, vth::theme::tokens::Space8,
                                     vth::theme::tokens::Space8, vth::theme::tokens::Space8);
            root->setSpacing(vth::theme::tokens::CardMargin);

            m_iconLabel = new QLabel(this);
            m_iconLabel->setObjectName(QStringLiteral("toastIconLabel"));
            m_iconLabel->setFixedSize(ToastGlyphSize, ToastGlyphSize);
            m_iconLabel->setAlignment(Qt::AlignCenter);
            root->addWidget(m_iconLabel, 0, Qt::AlignVCenter);

            auto* textLayout = new QVBoxLayout;
            textLayout->setContentsMargins(0, 0, 0, 0);
            textLayout->setSpacing(vth::theme::tokens::TightGap);

            m_titleLabel = new QLabel(message.title, this);
            m_titleLabel->setObjectName(QStringLiteral("toastTitleLabel"));
            // Wraps, because a toast title is often a sentence.
            m_titleLabel->setWordWrap(true);
            m_titleLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
            textLayout->addWidget(m_titleLabel);

            if (!message.body.trimmed().isEmpty()) {
                m_bodyLabel = new QLabel(message.body, this);
                m_bodyLabel->setObjectName(QStringLiteral("toastBodyLabel"));
                m_bodyLabel->setWordWrap(true);
                m_bodyLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
                textLayout->addWidget(m_bodyLabel);
            }

            root->addLayout(textLayout, 1);

            m_closeButton = new QToolButton(this);
            m_closeButton->setObjectName(QStringLiteral("toastCloseButton"));
            m_closeButton->setAutoRaise(true);
            m_closeButton->setFixedSize(vth::theme::tokens::Space24,
                                        vth::theme::tokens::Space24);
            // ToastCard has no Q_OBJECT, so tr() would file these under QWidget.
            m_closeButton->setAccessibleName(
                QCoreApplication::translate("ToastCard", "Close notification"));
            m_closeButton->setToolTip(
                QCoreApplication::translate("ToastCard", "Close"));
            root->addWidget(m_closeButton, 0, Qt::AlignTop);

            m_opacityEffect = new QGraphicsOpacityEffect(this);
            m_opacityEffect->setOpacity(0.0);
            setGraphicsEffect(m_opacityEffect);

            connect(m_closeButton, &QToolButton::clicked, this, [this] { dismiss(); });

            refreshTheme();
            adjustSize();

            if (message.durationMs > 0) {
                m_dismissTimer = new QTimer(this);
                m_dismissTimer->setSingleShot(true);
                m_dismissTimer->setInterval(message.durationMs);
                connect(m_dismissTimer, &QTimer::timeout, this, [this] { dismiss(); });
                m_dismissTimer->start();
            }
        }

        void refreshTheme()
        {
            namespace tokens = vth::theme::tokens;
            m_iconLabel->setPixmap(vth::icons::glyphPixmap(
                iconNameFor(m_message.severity), m_colors.icon,
                ToastGlyphSize, m_iconLabel));
            m_closeButton->setIcon(
                vth::icons::icon(QStringLiteral("x"), vth::icons::iconColor(), tokens::GlyphSm));
            m_closeButton->setIconSize(QSize(tokens::GlyphSm, tokens::GlyphSm));
            style()->unpolish(this);
            style()->polish(this);
        }

        void showAnimated()
        {
            show();
            raise();

            auto* animation = new QPropertyAnimation(m_opacityEffect, "opacity", this);
            animation->setDuration(vth::theme::animationDuration(kFadeInMs));
            animation->setStartValue(0.0);
            animation->setEndValue(1.0);
            animation->start(QAbstractAnimation::DeleteWhenStopped);
        }

        void dismiss()
        {
            if (m_dismissing)
                return;

            m_dismissing = true;
            auto* animation = new QPropertyAnimation(m_opacityEffect, "opacity", this);
            animation->setDuration(vth::theme::animationDuration(kFadeOutMs));
            animation->setStartValue(m_opacityEffect->opacity());
            animation->setEndValue(0.0);
            connect(animation, &QPropertyAnimation::finished, this, &QWidget::close);
            animation->start(QAbstractAnimation::DeleteWhenStopped);
        }

    protected:
        // Reading beats the timer: while the pointer is over the toast the
        // auto-dismiss clock pauses, and restarts in full on leave.
        void enterEvent(QEnterEvent* event) override
        {
            QWidget::enterEvent(event);
            if (m_dismissTimer)
                m_dismissTimer->stop();
        }

        void leaveEvent(QEvent* event) override
        {
            QWidget::leaveEvent(event);
            if (m_dismissTimer && !m_dismissing)
                m_dismissTimer->start();
        }

    private:
        ToastMessage m_message;
        ToastColors m_colors;
        QLabel* m_iconLabel = nullptr;
        QLabel* m_titleLabel = nullptr;
        QLabel* m_bodyLabel = nullptr;
        QToolButton* m_closeButton = nullptr;
        QGraphicsOpacityEffect* m_opacityEffect = nullptr;
        QTimer* m_dismissTimer = nullptr;
        bool m_dismissing = false;
    };

    ToastManager::ToastManager(QWidget* host, QObject* parent)
        : QObject(parent)
        , m_host(host)
    {
        if (m_host)
            m_host->installEventFilter(this);
    }

    void ToastManager::setInformationalEnabled(bool enabled)
    {
        m_informationalEnabled = enabled;
    }

    void ToastManager::show(const ToastMessage& message)
    {
        if (!m_host || message.title.trimmed().isEmpty())
            return;

        if (!m_informationalEnabled
            && (message.severity == ToastSeverity::Info
                || message.severity == ToastSeverity::Success))
            return;

        // A result arriving while Vethida sits behind another window flashes its
        // taskbar button, so a long repair can be left to run.
        if (vth::prefs::alertOnFinish() && m_host->window()
            && !m_host->window()->isActiveWindow())
            QApplication::alert(m_host->window());

        while (m_cards.size() >= MaxVisibleToasts) {
            if (ToastCard* oldest = m_cards.takeFirst())
                oldest->dismiss();
        }

        // Resolve the duration here rather than in the card, so a toast already
        // on screen keeps the interval it was shown with when the setting moves.
        ToastMessage resolved = message;
        if (resolved.severity == ToastSeverity::Error) {
            // Errors stay until dismissed, whatever the interval. Zero means no timer.
            resolved.durationMs = 0;
        } else if (resolved.durationMs < 0) {
            resolved.durationMs = m_defaultDurationMs;
        }

        auto* card = new ToastCard(resolved, m_host);
        connect(card, &QObject::destroyed, this, [this, card] { removeCard(card); });

        m_cards.append(card);
        restack();
        card->showAnimated();
    }

    void ToastManager::info(const QString& title, const QString& body)
    {
        show({ ToastSeverity::Info, title, body });
    }

    void ToastManager::success(const QString& title, const QString& body)
    {
        show({ ToastSeverity::Success, title, body });
    }

    void ToastManager::warning(const QString& title, const QString& body)
    {
        show({ ToastSeverity::Warning, title, body });
    }

    void ToastManager::error(const QString& title, const QString& body)
    {
        show({ ToastSeverity::Error, title, body });
    }

    void ToastManager::setDefaultDurationMs(int ms)
    {
        if (ms > 0)
            m_defaultDurationMs = ms;
    }

    void ToastManager::clear()
    {
        const QList<ToastCard*> cards = m_cards;
        for (ToastCard* card : cards) {
            if (card)
                card->dismiss();
        }
    }

    bool ToastManager::eventFilter(QObject* watched, QEvent* event)
    {
        if (watched == m_host && (event->type() == QEvent::Resize || event->type() == QEvent::Show))
            restack();

        return QObject::eventFilter(watched, event);
    }

    void ToastManager::removeCard(ToastCard* card)
    {
        m_cards.removeAll(card);
        restack();
    }

    void ToastManager::restack()
    {
        if (!m_host)
            return;

        int bottom = m_host->height() - ToastMargin;
        for (auto it = m_cards.rbegin(); it != m_cards.rend(); ++it) {
            ToastCard* card = *it;
            if (!card)
                continue;

            // heightForWidth, because a wrapping label's sizeHint is its one-line hint.
            // -1 when the layout doesn't support it, which qMax discards.
            const int height = qMax(card->sizeHint().height(),
                                    card->heightForWidth(ToastWidth));
            bottom -= height;
            card->setGeometry(m_host->width() - ToastWidth - ToastMargin, bottom, ToastWidth, height);
            card->raise();
            bottom -= ToastSpacing;
        }
    }
}
