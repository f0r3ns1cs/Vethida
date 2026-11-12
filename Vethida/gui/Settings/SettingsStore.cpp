#include "pch.h"
#include "SettingsStore.h"

#include <QSettings>

namespace
{
    constexpr auto UiGroup = "ui";
    constexpr auto WindowGeometryKey = "windowGeometry";
    constexpr auto SidebarCollapsedKey = "sidebarCollapsed";
    constexpr auto SelectedRouteKey = "selectedRoute";
    constexpr auto WindowRectKey = "windowRect";
    constexpr auto WindowMaximizedKey = "windowMaximized";

    QSettings makeSettings()
    {
        return QSettings();
    }
}

namespace vth::settings
{
    QByteArray SettingsStore::windowGeometry() const
    {
        QSettings s = makeSettings();
        s.beginGroup(QString::fromLatin1(UiGroup));
        return s.value(QString::fromLatin1(WindowGeometryKey)).toByteArray();
    }

    void SettingsStore::setWindowGeometry(const QByteArray& geometry)
    {
        QSettings s = makeSettings();
        s.beginGroup(QString::fromLatin1(UiGroup));
        s.setValue(QString::fromLatin1(WindowGeometryKey), geometry);
    }

    QRect SettingsStore::windowRect() const
    {
        QSettings s = makeSettings();
        s.beginGroup(QString::fromLatin1(UiGroup));
        return s.value(QString::fromLatin1(WindowRectKey)).toRect();
    }

    void SettingsStore::setWindowRect(const QRect& rect)
    {
        QSettings s = makeSettings();
        s.beginGroup(QString::fromLatin1(UiGroup));
        s.setValue(QString::fromLatin1(WindowRectKey), rect);
    }

    bool SettingsStore::windowMaximized() const
    {
        QSettings s = makeSettings();
        s.beginGroup(QString::fromLatin1(UiGroup));
        return s.value(QString::fromLatin1(WindowMaximizedKey), false).toBool();
    }

    void SettingsStore::setWindowMaximized(bool maximized)
    {
        QSettings s = makeSettings();
        s.beginGroup(QString::fromLatin1(UiGroup));
        s.setValue(QString::fromLatin1(WindowMaximizedKey), maximized);
    }

    bool SettingsStore::sidebarCollapsed() const
    {
        QSettings s = makeSettings();
        s.beginGroup(QString::fromLatin1(UiGroup));
        return s.value(QString::fromLatin1(SidebarCollapsedKey), false).toBool();
    }

    void SettingsStore::setSidebarCollapsed(bool collapsed)
    {
        QSettings s = makeSettings();
        s.beginGroup(QString::fromLatin1(UiGroup));
        s.setValue(QString::fromLatin1(SidebarCollapsedKey), collapsed);
    }

    QString SettingsStore::selectedRoute() const
    {
        QSettings s = makeSettings();
        s.beginGroup(QString::fromLatin1(UiGroup));
        return s.value(QString::fromLatin1(SelectedRouteKey)).toString();
    }

    void SettingsStore::setSelectedRoute(const QString& routeId)
    {
        QSettings s = makeSettings();
        s.beginGroup(QString::fromLatin1(UiGroup));
        s.setValue(QString::fromLatin1(SelectedRouteKey), routeId);
    }
}
