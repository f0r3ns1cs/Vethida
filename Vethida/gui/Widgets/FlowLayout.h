#pragma once

#include <QLayout>
#include <QList>

namespace vth::widgets
{
    // left to right, wraps when full (used for chips)
    class FlowLayout final : public QLayout
    {
    public:
        FlowLayout(QWidget* parent, int hSpacing, int vSpacing);
        ~FlowLayout() override;

        void addItem(QLayoutItem* item) override;
        int count() const override;
        QLayoutItem* itemAt(int index) const override;
        QLayoutItem* takeAt(int index) override;

        Qt::Orientations expandingDirections() const override { return {}; }
        bool hasHeightForWidth() const override { return true; }
        int heightForWidth(int width) const override;
        QSize minimumSize() const override;
        QSize sizeHint() const override;
        void setGeometry(const QRect& rect) override;

    private:
        int doLayout(const QRect& rect, bool testOnly) const;

        QList<QLayoutItem*> items_;
        int hSpace_;
        int vSpace_;
    };
}
