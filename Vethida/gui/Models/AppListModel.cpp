#include "pch.h"
#include "AppListModel.h"

#include <QDir>
#include <QIcon>
#include <QPixmap>
#include <QRegularExpression>
#include <QStringLiteral>
#include <QVariant>

#include <utility>

namespace vth::ui::models
{
    namespace
    {
        namespace apps = vth::win::apps;

        // A manifest name that doesn't resolve falls back to the package name, which
        // for some Windows components is a bare GUID.
        bool isGuidName(const QString& name)
        {
            static const QRegularExpression kGuidName(
                QStringLiteral("^\\{?[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}"
                               "-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}\\}?$"));
            return kGuidName.match(name).hasMatch();
        }

        // Windows' generic application icon if the app has none.
        QIcon rowIcon(const apps::AppInfo& app)
        {
            if (!app.iconImage.isNull())
                return QIcon(QPixmap::fromImage(app.iconImage));
            static const QIcon kDefault = [] {
                const QImage img = apps::defaultAppIcon();
                return img.isNull() ? QIcon() : QIcon(QPixmap::fromImage(img));
            }();
            return kDefault;
        }
    }

    AppListModel::AppListModel(QObject* parent)
        : QAbstractTableModel(parent)
    {
    }

    AppListModel::Presentation AppListModel::presentationFor(const apps::AppInfo& app) const
    {
        Presentation p;
        const bool unnamed = isGuidName(app.displayName);
        p.shownName = unnamed ? tr("Unnamed package") : app.displayName;
        // Unnamed packages sort together at the end. "~" sorts after letters and digits.
        p.sortName = unnamed ? QStringLiteral("~") + app.displayName.toLower()
                             : app.displayName.toLower();

        // Everything the columns don't carry, on hover.
        QStringList tip;
        tip << app.displayName;
        QStringList meta;
        if (!app.publisher.isEmpty()) meta << app.publisher;
        if (!app.version.isEmpty())   meta << app.version;
        if (!meta.isEmpty())
            tip << meta.join(QStringLiteral("  "));
        tip << tr("%1 - %2").arg(apps::architectureLabel(app.architecture),
                                 apps::sourceLabel(app.source));
        if (!app.installLocation.isEmpty())
            tip << QDir::toNativeSeparators(app.installLocation);
        p.tooltip = tip.join(QStringLiteral("\n"));
        return p;
    }

        // Like Windows' own Installed apps page, system components aren't listed.
    bool AppListModel::listed(const apps::AppInfo& app)
    {
        return !app.systemComponent;
    }

    void AppListModel::setApps(QVector<apps::AppInfo> list)
    {
        beginResetModel();
        apps_.clear();
        shown_.clear();
        apps_.reserve(list.size());
        shown_.reserve(list.size());
        for (apps::AppInfo& app : list) {
            if (!listed(app))
                continue;
            shown_.append(presentationFor(app));
            apps_.append(std::move(app));
        }
        endResetModel();
    }

    void AppListModel::clear()
    {
        setApps({});
    }

    void AppListModel::reload()
    {
        loadToken_.cancel();
        storeToken_.cancel();
        loading_ = true;
        Q_EMIT loadingChanged();

        // Two passes, so the desktop apps are on screen before the Store query starts.
        loadToken_ = vth::fetch<QVector<apps::AppInfo>>(
            this,
            []() { return apps::enumerateDesktop(); },
            [this](QVector<apps::AppInfo> desktop) {
                setApps(std::move(desktop));
                storeToken_ = vth::fetch<QVector<apps::AppInfo>>(
                    this,
                    []() { return apps::enumerateStore(); },
                    [this](QVector<apps::AppInfo> store) {
                        appendApps(std::move(store));
                        loading_ = false;
                        Q_EMIT loadingChanged();
                    });
            });
    }

    void AppListModel::appendApps(QVector<apps::AppInfo> more)
    {
        if (more.isEmpty())
            return;

        // Appended, because a reset would lose the scroll position and selection.
        QVector<apps::AppInfo> add;
        add.reserve(more.size());
        for (apps::AppInfo& app : more) {
            if (listed(app))
                add.append(std::move(app));
        }
        if (add.isEmpty())
            return;

        const int first = static_cast<int>(apps_.size());
        beginInsertRows({}, first, first + static_cast<int>(add.size()) - 1);
        for (apps::AppInfo& app : add) {
            shown_.append(presentationFor(app));
            apps_.append(std::move(app));
        }
        endInsertRows();
    }

    apps::AppInfo AppListModel::appAt(int row) const
    {
        if (row < 0 || row >= apps_.size())
            return {};
        return apps_.at(row);
    }

    const AppListModel::Presentation& AppListModel::presentationAt(int row) const
    {
        static const Presentation kEmpty;
        if (row < 0 || row >= shown_.size())
            return kEmpty;
        return shown_.at(row);
    }

    int AppListModel::rowCount(const QModelIndex& parent) const
    {
        // Flat. Proxies walk this, so a valid parent must have no children.
        return parent.isValid() ? 0 : static_cast<int>(apps_.size());
    }

    int AppListModel::columnCount(const QModelIndex& parent) const
    {
        return parent.isValid() ? 0 : ColumnCount;
    }

    QVariant AppListModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid() || index.row() < 0 || index.row() >= apps_.size())
            return {};

        const apps::AppInfo& app = apps_.at(index.row());
        const Presentation&  p   = presentationAt(index.row());

        switch (role) {
            case Qt::DisplayRole:
                switch (index.column()) {
                    case ColumnName:      return p.shownName;
                    case ColumnPublisher: return app.publisher;
                    case ColumnVersion:   return app.version;
                    case ColumnType:      return apps::typeLabel(app.type);
                    default:              return {};
                }
            case Qt::ToolTipRole:
                return p.tooltip;
            case SortRole:
                switch (index.column()) {
                    case ColumnName:      return p.sortName;
                    case ColumnPublisher: return app.publisher.toLower();
                    case ColumnVersion:   return app.version.toLower();
                    case ColumnType:      return apps::typeLabel(app.type);
                    default:              return {};
                }
            case Qt::DecorationRole:
                // On the name column only, or the tree repeats it in every cell.
                return index.column() == ColumnName ? QVariant::fromValue(rowIcon(app))
                                                    : QVariant{};
            case AppRole:
                return QVariant::fromValue(app);
            default:
                return {};
        }
    }

    QVariant AppListModel::headerData(int section, Qt::Orientation orientation,
                                      int role) const
    {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
            return {};
        switch (section) {
            // No size or install date columns, because Windows reports neither for
            // Store packages. The details dialog shows them per app.
            case ColumnName:      return tr("Name");
            case ColumnPublisher: return tr("Publisher");
            case ColumnVersion:   return tr("Version");
            case ColumnType:      return tr("Type");
            default:              return {};
        }
    }

    Qt::ItemFlags AppListModel::flags(const QModelIndex& index) const
    {
        if (!index.isValid())
            return Qt::NoItemFlags;
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }

    QHash<int, QByteArray> AppListModel::roleNames() const
    {
        QHash<int, QByteArray> names = QAbstractTableModel::roleNames();
        names.insert(SortRole, QByteArrayLiteral("sortKey"));
        names.insert(AppRole,  QByteArrayLiteral("app"));
        return names;
    }
}
