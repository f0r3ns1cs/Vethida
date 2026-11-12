#pragma once

#include <QWidget>

class QVariantAnimation;

namespace vth::widgets
{
    // The small mark at the start of a result line: a check in a green disc
    // when something worked, a cross in a red one when it did not.
    class StatusMark final : public QWidget
    {
    public:
        enum class Kind { Good, Bad };

        explicit StatusMark(QWidget* parent = nullptr);

        // doesn't replay if already showing the same kind
        void present(Kind kind, bool animate = true);

        // Hides the mark; the next present() plays its entrance again.
        void clear();

        QSize sizeHint() const override;

    protected:
        void paintEvent(QPaintEvent* event) override;
        void changeEvent(QEvent* event) override;

    private:
        Kind               kind_     = Kind::Good;
        bool               shown_    = false;
        qreal              progress_ = 1.0;   // 0 = nothing drawn, 1 = complete
        QVariantAnimation* anim_     = nullptr;
    };
}
