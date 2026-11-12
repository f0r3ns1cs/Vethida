#pragma once

#include <QFrame>
#include <QString>

class QLabel;
class QScrollArea;
class QVBoxLayout;
class QHBoxLayout;
class QMouseEvent;

namespace vth::widgets
{
    class SwitchButton;

    // One settings row: title over a wrapping description, controls on the right. The
    // row is as tall as its text needs, so nothing here may set a fixed height.
    class SettingsRow final : public QFrame
    {
        Q_OBJECT

    public:
        SettingsRow(const QString& title, const QString& description,
                    QWidget* parent = nullptr);

        SwitchButton* toggle() const { return toggle_; }

        // Marks the row as a click target, with a hover cue, a pointer cursor and
        // bodyClicked.
        void setInteractive(bool on);

        void setTitle(const QString& text);
        void setDescription(const QString& text);
        // Control placed immediately left of the switch (e.g. an info button).
        void addTrailing(QWidget* w);
        // Caption strip under the description, for badges and state text.
        void setMeta(QWidget* w);

    Q_SIGNALS:
        // Left-click anywhere that is not one of the controls.
        void bodyClicked();

    protected:
        void mousePressEvent(QMouseEvent* event) override;

    private:
        // A SwitchButton has no text of its own, so it takes the row's.
        void applyAccessibleLabels();

        bool          interactive_ = false;
        QLabel*       title_ = nullptr;
        QLabel*       desc_  = nullptr;
        QVBoxLayout*  textCol_ = nullptr;
        QHBoxLayout*  row_ = nullptr;
        QWidget*      meta_ = nullptr;
        SwitchButton* toggle_ = nullptr;
    };

    // A scrolling column of section headings, rules and SettingsRows.
    class SettingsList final : public QWidget
    {
        Q_OBJECT

    public:
        explicit SettingsList(QWidget* parent = nullptr);

        // Returns the heading block, so a caller that filters rows can hide the
        // whole group - caption, rule and spacing - in one call.
        QWidget* addHeading(const QString& text);
        void addRule();
        void addSpace(int px);
        SettingsRow* addRow(const QString& title, const QString& description);
        void addWidget(QWidget* w);

        // Shown in place of the rows while the list is empty.
        void setPlaceholder(const QString& text);
        // Drops every item so the pane can be rebuilt from scratch.
        void clear();
        bool isEmpty() const { return itemCount_ == 0; }

    private:
        void refreshPlaceholder();

        QScrollArea* scroll_ = nullptr;
        QWidget*     content_ = nullptr;
        QVBoxLayout* lay_ = nullptr;
        QLabel*      placeholder_ = nullptr;
        QString      placeholderText_;
        int          itemCount_ = 0;
    };
}
