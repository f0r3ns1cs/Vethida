#pragma once

#include <QAbstractButton>
#include <QString>

class QVariantAnimation;

namespace vth::widgets
{
    // The plan pill in the title bar. Painted, so the mark and the word share one
    // measured center line.
    class LicenseBadge final : public QAbstractButton
    {
        Q_OBJECT

    public:
        enum class Kind { Plain, Pro, Ultimate, Warn };

        explicit LicenseBadge(QWidget* parent = nullptr);

        void setBadge(const QString& text, const QString& iconName, Kind kind);

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override { return sizeHint(); }

    protected:
        void paintEvent(QPaintEvent* event) override;
        void hideEvent(QHideEvent* event) override;
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;

    private:
        void animateHover(bool hovered);
        QFont badgeFont() const;

        QString            m_iconName;
        Kind               m_kind = Kind::Plain;
        qreal              m_hover = 0.0;    // 0..1, rest to hover
        QVariantAnimation* m_hoverAnim = nullptr;
    };
}
