#pragma once

#include <QObject>
#include <QList>
#include <QString>

class QWidget;

namespace vth::widgets
{
    enum class ToastSeverity
    {
        Info,
        Success,
        Warning,
        Error
    };

    struct ToastMessage
    {
        ToastSeverity severity = ToastSeverity::Info;
        QString title;
        QString body;
        // -1 = use the manager's default (the Notifications setting).
        // 0 = never auto-dismiss. Anything positive is an explicit override.
        int durationMs = -1;
    };

    class ToastManager final : public QObject
    {
        Q_OBJECT

    public:
        explicit ToastManager(QWidget* host, QObject* parent = nullptr);

        void show(const ToastMessage& message);
        void info(const QString& title, const QString& body = QString());
        void success(const QString& title, const QString& body = QString());
        void warning(const QString& title, const QString& body = QString());
        void error(const QString& title, const QString& body = QString());
        void clear();

        void setInformationalEnabled(bool enabled);

        // default duration (from the Notifications setting)
        void setDefaultDurationMs(int ms);

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;

    private:
        class ToastCard;

        void removeCard(ToastCard* card);
        void restack();

        QWidget* m_host = nullptr;
        QList<ToastCard*> m_cards;
        bool m_informationalEnabled = true;
        int  m_defaultDurationMs = 5000;   // matches the "5 seconds (recommended)" default
    };
}
