#pragma once

#include <QWidget>

class QLayout;
class QVBoxLayout;

namespace vth::widgets
{
    class PageHeader;

    // The standard page shell: a header over a scrollable body with the standard padding.
    class PageScaffold : public QWidget
    {
    public:
        explicit PageScaffold(const QString& title = QString(),
                              const QString& subtitle = QString(),
                              QWidget* parent = nullptr);

        // Right-aligned header controls (buttons, selectors), stacked left-to-right.
        void addHeaderAction(QWidget* action);

        // Body content, inserted above the trailing stretch.
        void addBodyWidget(QWidget* widget, int stretch = 0);
        void addBodyLayout(QLayout* layout, int stretch = 0);

        // Gap between body items. Set to 0 for content that carries its own separators.
        void setBodySpacing(int spacing);

        // Appends go after the trailing stretch, so prefer addBodyWidget/addBodyLayout.
        QVBoxLayout* bodyLayout() const { return bodyLay_; }

    private:
        // Null on a page with no title.
        PageHeader*  header_  = nullptr;
        QVBoxLayout* bodyLay_ = nullptr;
    };
}
