#pragma once

#include <QByteArray>
#include <QRect>
#include <QString>

namespace vth::settings
{
    class SettingsStore final
    {
    public:
        QByteArray windowGeometry() const;
        void setWindowGeometry(const QByteArray& geometry);

        // Where the window was, as a rectangle, and whether it was maximized.
        QRect windowRect() const;
        void setWindowRect(const QRect& rect);

        bool windowMaximized() const;
        void setWindowMaximized(bool maximized);

        bool sidebarCollapsed() const;
        void setSidebarCollapsed(bool collapsed);

        QString selectedRoute() const;
        void setSelectedRoute(const QString& routeId);
    };
}
