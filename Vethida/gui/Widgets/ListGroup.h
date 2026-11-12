#pragma once

#include <QFrame>
#include <QString>

class QHBoxLayout;
class QLabel;
class QProgressBar;
class QVBoxLayout;

namespace vth::widgets
{
    class StatusMark;

    // One entry in a ListGroup: a name, one line saying what it's for, and the action.
    class ListRow final : public QFrame
    {
    public:
        enum class Tone { Neutral, Good, Bad };

        ListRow(const QString& title, const QString& description,
                QWidget* parent = nullptr);

        // The action on the right edge. Several are laid out left to right.
        void addTrailing(QWidget* w);

        // A line of live text under the title, in place of a fixed description.
        void addDetail(QWidget* w);

        // empty text hides it. Good/Bad get an animated mark.
        void setStatus(const QString& text, Tone tone = Tone::Neutral, bool animate = true);

        // A thin progress bar under the text, for work that reports how far it
        // has got. Negative hides it; 0-100 shows that much.
        void setProgress(int percent);

        // Left and right inset of the row's content. Rows in a card are inset
        // from its border; rows in a plain group sit flush with the heading.
        void setInset(int horizontal, int vertical);

    private:
        QVBoxLayout*  text_     = nullptr;
        QHBoxLayout*  row_      = nullptr;
        QWidget*      statusRow_ = nullptr;
        StatusMark*   mark_     = nullptr;
        QLabel*       status_   = nullptr;
        QProgressBar* progress_ = nullptr;
    };

    // A titled card holding a list of rows divided by hairlines.
    class ListGroup final : public QFrame
    {
    public:
        // Card draws one bordered card with its title inside. Grouped puts a small
        // heading above the card, as in Windows settings.
        enum class Style { Card, Grouped };

        explicit ListGroup(const QString& title = QString(), QWidget* parent = nullptr,
                           Style style = Style::Card);

        ListRow* addRow(const QString& title, const QString& description);

        int rowCount() const { return rows_; }

    private:
        QVBoxLayout* lay_  = nullptr;   // where rows go: the card's layout
        QFrame*      body_ = nullptr;   // the card itself; this widget in Card
        int          rows_ = 0;
        Style        style_ = Style::Card;
    };
}
