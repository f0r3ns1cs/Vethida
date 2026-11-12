#include "pch.h"
#include "TablerIcon.h"

#include "../Theme/DesignTokens.h"
#include "../Theme/Theme.h"

#include <QCoreApplication>
#include <QEvent>
#include <QFile>
#include <QGuiApplication>
#include <QHash>
#include <QList>
#include <QPainter>
#include <QPixmap>
#include <QPointF>
#include <QScreen>
#include <QSet>
#include <QRectF>
#include <QSize>
#include <QSvgRenderer>
#include <QToolButton>
#include <QWidget>

#include <utility>

namespace
{
    // by name, IconHoverFilter has no Q_OBJECT so findChild<> by type would match anything
    inline QString iconHoverFilterName() { return QStringLiteral("vthIconHoverFilter"); }

    class IconHoverFilter : public QObject
    {
    public:
        IconHoverFilter(QToolButton* button, const QString& name,
                        vth::icons::ColorFn base, int size)
            : QObject(button)
            , m_button(button)
        {
            setObjectName(iconHoverFilterName());
            reconfigure(name, std::move(base), size);
            m_button->installEventFilter(this);
        }

        // kept so a palette change can re-render the icons
        void reconfigure(QString name, vth::icons::ColorFn base, int size)
        {
            m_name = std::move(name);
            if (base)
                m_base = std::move(base);
            m_size = size;
            rebake();
        }

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override
        {
            switch (event->type()) {
            case QEvent::Enter:
                m_button->setIcon(m_hover);
                break;
            case QEvent::Leave:
            case QEvent::EnabledChange:
                m_button->setIcon(m_normal);
                break;
            case QEvent::ApplicationPaletteChange:
                if (m_button->isVisible())
                    rebake();
                else
                    m_stale = true;
                break;
            case QEvent::Show:
                if (m_stale) {
                    m_stale = false;
                    rebake();
                }
                break;
            default:
                break;
            }
            return QObject::eventFilter(watched, event);
        }

    private:
        void rebake()
        {
            if (!m_base)
                return;
            m_normal = vth::icons::icon(m_name, m_base(), m_size);
            m_hover  = vth::icons::icon(m_name, vth::theme::palette::accentOnSurface(), m_size);
            m_button->setIcon(m_button->underMouse() ? m_hover : m_normal);
        }

        QToolButton*        m_button;
        QString             m_name;
        vth::icons::ColorFn m_base;
        int                 m_size = vth::icons::DefaultIconSize;
        // The palette changed while this button was off screen; both icons are
        // the previous mode's until it is shown again.
        bool                m_stale = false;
        QIcon               m_normal;
        QIcon               m_hover;
    };

    QByteArray loadIconSvg(const QString& name)
    {
        QFile f(QStringLiteral(":/icons/tabler/%1.svg").arg(name));
        if (f.open(QIODevice::ReadOnly))
            return f.readAll();

        // A name missing from vethida.qrc would otherwise look like a real glyph.
        static QSet<QString> warned;
        if (!warned.contains(name)) {
            warned.insert(name);
            qWarning("vth::icons: no glyph named \"%s\" in vethida.qrc; drawing "
                     "the fallback square", qUtf8Printable(name));
        }

        // Unknown name falls back to a neutral rounded square.
        return QByteArrayLiteral(
            "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" "
            "viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" "
            "stroke-linecap=\"round\" stroke-linejoin=\"round\">"
            "<rect x=\"3\" y=\"3\" width=\"18\" height=\"18\" rx=\"3\"/></svg>");
    }

    QByteArray colorizeSvg(const QByteArray& svg, const QColor& c)
    {
        QByteArray out = svg;
        out.replace("currentColor", c.name(QColor::HexRgb).toUtf8());
        return out;
    }

    QHash<QString, QIcon> g_iconCache;
    QHash<QString, QIcon> g_badgeCache;

    // The device pixel ratios worth rasterizing, taken from the displays actually
    // attached.
    QList<qreal> bakeRatios()
    {
        QList<qreal> ratios;
        const auto screens = QGuiApplication::screens();
        ratios.reserve(screens.size());
        for (const QScreen* screen : screens) {
            const qreal r = screen->devicePixelRatio();
            if (r > 0.0 && !ratios.contains(r))
                ratios.append(r);
        }
        if (ratios.isEmpty())
            ratios.append(1.0);
        return ratios;
    }

    // A QPixmap must not outlive QApplication, so both caches empty at aboutToQuit.
    void ensureCacheTeardown()
    {
        static bool hooked = false;
        if (hooked)
            return;
        QCoreApplication* app = QCoreApplication::instance();
        if (!app)
            return;
        hooked = true;
        QObject::connect(app, &QCoreApplication::aboutToQuit, app,
                         []() { vth::icons::clearCache(); });
    }

    // Width of the surface-colored knockout ring separating a lock badge from
    // whatever it overlaps.
    constexpr qreal kLockRing = 1.6;

    // Insets by the ring, so its outer edge lands on the canvas edge.
    void drawLockBadge(QPainter& p, qreal canvasPx, qreal discRatio,
                       qreal glyphRatio, qreal dpr)
    {
        namespace pal = vth::theme::palette;

        const qreal d = canvasPx * discRatio;
        const QRectF lockRect(canvasPx - d - kLockRing, canvasPx - d - kLockRing, d, d);
        p.setBrush(pal::cardSurface());
        p.drawEllipse(lockRect.adjusted(-kLockRing, -kLockRing, kLockRing, kLockRing));

        p.setBrush(pal::premiumFill());
        p.drawEllipse(lockRect);

        const int lockGlyph = qRound(d * glyphRatio);
        const QPixmap lg =
            vth::icons::icon(QStringLiteral("lock-filled"), pal::premiumText(), lockGlyph)
                .pixmap(QSize(lockGlyph, lockGlyph), dpr);
        p.drawPixmap(QPointF(lockRect.center().x() - lockGlyph / 2.0,
                             lockRect.center().y() - lockGlyph / 2.0),
                     lg);
    }
}

namespace vth::icons
{
    QIcon icon(const QString& name, const QColor& color, int size)
    {
        // Memoized; mode is part of the key because the disabled tint tracks the theme.
        ensureCacheTeardown();
        QHash<QString, QIcon>& cache = g_iconCache;
        const QString key = QStringLiteral("%1|%2|%3|%4")
                                .arg(name,
                                     color.name(QColor::HexArgb),
                                     QString::number(size),
                                     QString::number(static_cast<int>(vth::theme::currentMode())));
        if (const auto it = cache.constFind(key); it != cache.constEnd())
            return it.value();

        const QByteArray svg = loadIconSvg(name);
        auto colored = [&](const QColor& c) { return colorizeSvg(svg, c); };

        QSvgRenderer normalRenderer(colored(color));
        if (!normalRenderer.isValid())
            return {};
        QSvgRenderer disabledRenderer(colored(disabledIconColor()));

        auto makePixmap = [&](QSvgRenderer& renderer, qreal dpr) -> QPixmap {
            const int physicalSize = qRound(size * dpr);
            QPixmap pixmap(physicalSize, physicalSize);
            pixmap.setDevicePixelRatio(dpr);
            pixmap.fill(Qt::transparent);
            QPainter p(&pixmap);
            p.setRenderHint(QPainter::Antialiasing);
            renderer.render(&p, QRectF(0.0, 0.0, size, size));
            return pixmap;
        };

        QIcon result;
        const QList<qreal> ratios = bakeRatios();
        for (qreal dpr : ratios)
            result.addPixmap(makePixmap(normalRenderer, dpr), QIcon::Normal);
        for (qreal dpr : ratios)
            result.addPixmap(makePixmap(disabledRenderer, dpr), QIcon::Disabled);
        cache.insert(key, result);
        return result;
    }

    QPixmap pixmapFor(const QIcon& glyph, int size, const QWidget* on)
    {
        const qreal dpr = on ? on->devicePixelRatioF() : 1.0;
        return glyph.pixmap(QSize(size, size), dpr);
    }

    QPixmap glyphPixmap(const QString& name, const QColor& color, int size,
                        const QWidget* on)
    {
        return pixmapFor(icon(name, color, size), size, on);
    }

    QIcon iconBadge(const QString& name, int badgePx, bool locked)
    {
        namespace pal = vth::theme::palette;

        ensureCacheTeardown();
        QHash<QString, QIcon>& cache = g_badgeCache;
        const QString key = QStringLiteral("%1|%2|%3|%4")
                                .arg(name,
                                     QString::number(badgePx),
                                     locked ? QStringLiteral("1") : QStringLiteral("0"),
                                     QString::number(static_cast<int>(vth::theme::currentMode())));
        if (const auto it = cache.constFind(key); it != cache.constEnd())
            return it.value();

        auto render = [&](qreal dpr) -> QPixmap {
            QPixmap pm(qRound(badgePx * dpr), qRound(badgePx * dpr));
            pm.setDevicePixelRatio(dpr);
            pm.fill(Qt::transparent);
            QPainter p(&pm);
            p.setRenderHint(QPainter::Antialiasing, true);
            p.setPen(Qt::NoPen);

            const int glyphPx = qRound(badgePx * 0.82);
            const QPixmap glyph =
                icon(name, pal::textSecondary(), glyphPx).pixmap(QSize(glyphPx, glyphPx), dpr);
            p.drawPixmap(QPointF((badgePx - glyphPx) / 2.0, (badgePx - glyphPx) / 2.0), glyph);

            if (locked)
                drawLockBadge(p, badgePx, RailLockRatio, RailLockGlyphRatio, dpr);
            return pm;
        };

        QIcon result;
        for (qreal dpr : bakeRatios())
            result.addPixmap(render(dpr));
        cache.insert(key, result);
        return result;
    }

    void applyIconButton(QToolButton* button, const QString& name,
                         ColorFn base, int size)
    {
        if (!button)
            return;
        button->setProperty("uiRole", QStringLiteral("iconButton"));
        button->setIconSize(QSize(size, size));
        if (QObject* existing = button->findChild<QObject*>(iconHoverFilterName(),
                                                           Qt::FindDirectChildrenOnly))
            static_cast<IconHoverFilter*>(existing)->reconfigure(name, std::move(base), size);
        else
            new IconHoverFilter(button, name, std::move(base), size);
    }

    void clearCache()
    {
        g_iconCache.clear();
        g_badgeCache.clear();
    }

    QColor iconColor()
    {
        return vth::theme::palette::iconNeutral();
    }

    QColor disabledIconColor()
    {
        return vth::theme::palette::iconDisabled();
    }
}
