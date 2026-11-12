#pragma once

#include <QString>
#include <QWidget>

#include <functional>

class QLabel;
class QToolButton;

namespace vth::widgets
{
    class CommandButton;
    class GlyphLabel;

    // dashboard status line: icon, message, one optional action, close
    class StatusStrip final : public QWidget
    {
        Q_OBJECT

    public:
        enum class State
        {
            // The message, the detail and the button all come from the payload.
            Tip,
            FreeTier,
        };

        // What a state cannot know for itself. Tip supplies all of its strings,
        // and every state supplies what its button opens.
        struct Payload
        {
            QString               message;
            QString               detail;
            QString               buttonLabel;
            std::function<void()> onAction;
        };

        explicit StatusStrip(QWidget* parent = nullptr);

        void setState(State state, const Payload& payload = {});

    Q_SIGNALS:
        // The close control was pressed. The page decides what dismissing means.
        void dismissed();

    protected:
        // re-elide the message
        void resizeEvent(QResizeEvent* event) override;

    private:
        void applyMessage();

        GlyphLabel*  m_glyph   = nullptr;
        QLabel*      m_message = nullptr;
        // The line under the message. Hidden when a state has nothing to add, so
        // the message centers on its own rather than sitting high over a blank row.
        QLabel*      m_detail  = nullptr;
        CommandButton* m_action = nullptr;
        QToolButton* m_close   = nullptr;

        QString               m_text;
        QString               m_detailText;
        std::function<void()> m_onAction;
    };
}
