#pragma once

#include "../Windows/NativeWindowChrome.h"

#include <QPoint>
#include <QWidget>

class QEvent;
class QToolButton;

namespace vth::widgets
{
    // The minimize / maximize / close group at the right end of a title bar.
    class CaptionButtons final : public QWidget
    {
    public:
        // withMaximize is false for a window whose size is fixed.
        explicit CaptionButtons(bool withMaximize, QWidget* parent = nullptr);

        // The button under a point in this widget's coordinates, or None. These are
        // the only title bar children the OS resolves rather than Qt.
        vth::platform::CaptionButton buttonAt(const QPoint& pos) const;

        void applyState(vth::platform::CaptionButton hovered,
                        vth::platform::CaptionButton pressed);

        // An inactive window draws its glyphs muted, like its title text.
        void refreshIcons(bool windowActive, bool maximized);

    protected:
        // The glyphs are baked pixmaps, so a palette change re-applies the last state.
        void changeEvent(QEvent* event) override;

    private:
        QToolButton* m_minimize = nullptr;
        QToolButton* m_maximize = nullptr;   // null on a fixed-size window
        QToolButton* m_close    = nullptr;

        bool m_wasActive    = true;
        bool m_wasMaximized = false;
    };
}
