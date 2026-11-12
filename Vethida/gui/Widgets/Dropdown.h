#pragma once

#include <QElapsedTimer>
#include <QVariant>
#include <QVector>
#include <QWidget>

class QFrame;
class QLabel;
class QPushButton;

namespace vth::ui
{
    class Dropdown : public QWidget
    {
        Q_OBJECT
    public:
        explicit Dropdown(QWidget* parent = nullptr);

        void addItem(const QString& text, const QVariant& data = QVariant());
        void setItems(const QStringList& texts);

        int     currentIndex() const { return current_; }
        QString currentText() const;
        QVariant currentData() const;
        void    setCurrentIndex(int index);

        QSize sizeHint() const override;

    Q_SIGNALS:
        void currentIndexChanged(int index);

    protected:
        void mousePressEvent(QMouseEvent* e) override;
        void keyPressEvent(QKeyEvent* e) override;
        void changeEvent(QEvent* e) override;
        void showEvent(QShowEvent* e) override;
        void resizeEvent(QResizeEvent* e) override;
        bool eventFilter(QObject* obj, QEvent* e) override;

    private:
        void showPopup();
        // Bakes the chevron at the current theme, because a pixmap doesn't follow it.
        void refreshChevron();
        void choose(int index);
        void focusRow(int index);
        void invalidatePopup();
        void updateValueText();

        struct Item { QString text; QVariant data; };
        QVector<Item> items_;
        QVector<QPushButton*> rows_;
        int     current_     = -1;
        QLabel* valueLabel_  = nullptr;
        QLabel* chevron_     = nullptr;
        QFrame* popup_       = nullptr;
        // Marks when the popup last hid, so the same click that dismissed it
        // (Qt::Popup grab) doesn't immediately reopen it.
        QElapsedTimer popupClosedAt_;
        // The theme flipped while this dropdown was off screen; the chevron
        // re-bakes on the next show rather than during the switch.
        bool chevronStale_ = false;
    };
}
