#pragma once

#include <QtWidgets/QWidget>
#include <QtCore/QString>

class QLabel;

namespace vth::widgets { class Spinner; }

namespace vth::ui
{
    class EmptyState : public QWidget
    {
        Q_OBJECT
    public:
        explicit EmptyState(QWidget* parent = nullptr);
        ~EmptyState() override;

        void setTitle(const QString& text);
        void setSubtitle(const QString& text);
        // Busy shows only the spinner. The text returns when busy clears.
        void setBusy(bool busy);

    private:
        void applyContentVisibility();

        QLabel*                title_    = nullptr;
        QLabel*                subtitle_ = nullptr;
        vth::widgets::Spinner* spinner_  = nullptr;
        bool                   busy_     = false;
    };
}
