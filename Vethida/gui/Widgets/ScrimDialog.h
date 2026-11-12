#pragma once

#include <QDialog>
#include <QPointer>

namespace vth::widgets
{
    // Centered card over a dimmed scrim, as a child of the main window. Blocks with
    // a local event loop.
    class ScrimDialog : public QDialog
    {
    public:
        ScrimDialog(QWidget* card, QWidget* host);

        // The control that holds focus when the dialog opens. Set it on any dialog whose
        // buttons aren't equally safe.
        void setInitialFocus(QWidget* widget);

        int exec();

    protected:
        void paintEvent(QPaintEvent*) override;
        bool eventFilter(QObject* obj, QEvent* e) override;
        bool focusNextPrevChild(bool next) override;
        // QDialog's own Escape handling only applies to a dialog that is a window.
        void keyPressEvent(QKeyEvent* e) override;

    private:
        void coverHost();
        void fadeIn();
        // Moves focus into the card. False when the card has nothing focusable.
        bool focusFirstControl();

        QWidget* host_ = nullptr;
        QPointer<QWidget> initialFocus_;
    };
}
