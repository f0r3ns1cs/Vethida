#pragma once

#include <QString>
#include <QWidget>

class QHBoxLayout;
class QLabel;
class QVBoxLayout;

namespace vth::widgets
{
    // page title, subtitle, optional buttons and notices
    class PageHeader final : public QWidget
    {
    public:
        // Who supplies the page inset.
        enum class Inset { Page, Bare };

        PageHeader(const QString& title, const QString& subtitle,
                   QWidget* parent = nullptr, Inset inset = Inset::Page);

        // Page-level commands, right-aligned on the title row.
        void addAction(QWidget* action);

        // A line under the subtitle, for state that belongs to the page. Takes
        // ownership, and the caller keeps the pointer to show and hide it.
        void addNotice(QWidget* notice);

        void setSubtitle(const QString& text);

    private:
        QHBoxLayout* actions_  = nullptr;
        QVBoxLayout* column_   = nullptr;
        QLabel*      subtitle_ = nullptr;
    };
}
