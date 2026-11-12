#pragma once

#include <QtGlobal>
#include <QByteArray>
#include <QIcon>
#include <QPoint>
#include <functional>

class QColor;
class QWindow;

namespace vth::platform
{
    enum class CaptionButton
    {
        None,
        Minimize,
        MaximizeRestore,
        Close
    };

    // The glyph for one caption button, baked in the color asked for.
    QIcon captionGlyphIcon(CaptionButton button, const QColor& color,
                                         bool maximized = false);

    // Native-frame-preserving custom window chrome for Windows.
    class NativeWindowChrome
    {
    public:
        enum class HitArea
        {
            Client,
            Caption,
            MinimizeButton,
            MaximizeButton,
            CloseButton
        };

        // pt is in logical (device-independent) coordinates relative to the window's client
        // area, which equals the whole window because the standard frame has been removed.
        using HitTestProvider = std::function<HitArea(const QPoint& pt)>;
        using ButtonStateSink = std::function<void(CaptionButton hovered, CaptionButton pressed)>;
        using RelayoutSink = std::function<void()>;

        explicit NativeWindowChrome(QWindow* window);

        void setHitTestProvider(HitTestProvider provider) { m_hitTest = std::move(provider); }
        void setButtonStateSink(ButtonStateSink sink) { m_buttonSink = std::move(sink); }
        void setRelayoutSink(RelayoutSink sink) { m_relayout = std::move(sink); }

        // resizable=false: no sizing frame or maximize
        void install(bool dark, bool resizable = true);
        void applyTheme(bool dark);

        bool handleNativeEvent(const QByteArray& eventType, void* message, qintptr* result);

    private:
        QWindow* m_window = nullptr;
        HitTestProvider m_hitTest;
        ButtonStateSink m_buttonSink;
        RelayoutSink m_relayout;
        CaptionButton m_hovered = CaptionButton::None;
        CaptionButton m_pressed = CaptionButton::None;
        bool m_dark = true;
        bool m_resizable = true;

#ifdef Q_OS_WIN
        void notifyButtons();
        void invokeCaption(CaptionButton button);
        qreal devicePixelRatioForWindow() const;
#endif
    };
}
