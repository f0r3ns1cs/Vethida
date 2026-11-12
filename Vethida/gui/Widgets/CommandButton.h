#pragma once

#include <QPushButton>
#include <QString>

class QTimer;

namespace vth::widgets
{
    class CommandButton : public QPushButton
    {
        Q_OBJECT

    public:
        enum class Variant { Secondary, Primary };

        explicit CommandButton(QWidget* parent = nullptr);
        explicit CommandButton(const QString& text, QWidget* parent = nullptr);
        explicit CommandButton(Variant variant, const QString& text, QWidget* parent = nullptr);

        void setLoading(bool loading);
        bool isLoading() const { return m_loading; }

    protected:
        void changeEvent(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        void applyVariant();
        void updateCursor();

        Variant  m_variant       = Variant::Secondary;
        bool     m_loading       = false;
        bool     m_savedEnabled  = true;   // enabled state to restore when loading ends
        int      m_savedMinWidth = 0;
        int      m_spinAngle     = 0;
        QTimer*  m_spinTimer = nullptr;
        QString  m_savedText;
    };
}
