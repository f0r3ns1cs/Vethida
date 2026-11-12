#pragma once

#include <QDialog>

namespace vth::widgets
{
    // Modal panel over the sidebar + content. Only the title bar still works.
    class PanelDialog : public QDialog
    {
    public:
        enum class Mode { ScrimPanel, Cover };

        PanelDialog(QWidget* card, QWidget* anchor, Mode mode);

        int exec();

    protected:
        void paintEvent(QPaintEvent*) override;
        void mousePressEvent(QMouseEvent*) override;
        void keyPressEvent(QKeyEvent*) override;
        bool eventFilter(QObject* obj, QEvent* e) override;
        bool focusNextPrevChild(bool next) override;
        void resizeEvent(QResizeEvent*) override;

    private:
        void coverRegion();
        void placeCard();

        // sidebar + content
        QWidget* region_ = nullptr;
        // Where the card sits: the content area inside that region.
        QWidget* content_ = nullptr;
        QWidget* card_   = nullptr;
        Mode     mode_;
    };
}
