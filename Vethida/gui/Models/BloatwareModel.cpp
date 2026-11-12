#include "pch.h"
#include "BloatwareModel.h"

#include "../Theme/DesignTokens.h"

#include <QFont>
#include <QIcon>
#include <QStringLiteral>
#include <QVariant>

#include <utility>

namespace vth::ui::models
{
    namespace
    {
        namespace apps = vth::win::apps;
        namespace bw   = vth::win::bloatware;

        constexpr bw::Severity kSeverityOrder[] = {
            bw::Severity::Recommended,
            bw::Severity::Optional,
            bw::Severity::Risky,
        };
    }

    BloatwareModel::BloatwareModel(QObject* parent)
        : QAbstractItemModel(parent)
    {
    }

    void BloatwareModel::setInstalled(const QVector<apps::AppInfo>& installed)
    {
        beginResetModel();
        groups_.clear();

        for (const bw::Severity sev : kSeverityOrder) {
            Group group;
            group.severity = sev;
            group.label    = bw::severityLabel(sev);

            for (const bw::Entry& entry : bw::catalog()) {
                if (entry.severity != sev)
                    continue;

                // Get-AppxPackage's Name, which is the form the catalog stores.
                QString matchedFullName;
                for (const apps::AppInfo& app : installed) {
                    if (app.type != apps::Type::UWP) continue;
                    if (app.nonRemovable)            continue;
                    if (app.packageName == entry.name) {
                        matchedFullName = app.packageFullName;
                        break;
                    }
                }
                if (matchedFullName.isEmpty())
                    continue;

                App app;
                app.packageName     = entry.name;
                app.packageFullName = matchedFullName;
                app.displayName     = entry.displayName;
                app.impact          = entry.impact;
                app.recommended     = (sev == bw::Severity::Recommended);
                // A risky row's impact note is in the cell, so keyboard users see it.
                app.description     = entry.description;
                if (sev == bw::Severity::Risky && !entry.impact.isEmpty())
                    app.description += QLatin1Char(' ') + entry.impact;
                // Nothing is ticked by default. Ticking is the consent.
                app.checked = false;

                group.apps.append(std::move(app));
            }

            if (!group.apps.isEmpty())
                groups_.append(std::move(group));
        }

        endResetModel();
        Q_EMIT checkedChanged();
    }

    void BloatwareModel::clear()
    {
        setInstalled({});
    }

    void BloatwareModel::reload()
    {
        loadToken_.cancel();
        loading_ = true;
        Q_EMIT loadingChanged();

        loadToken_ = vth::fetch<QVector<apps::AppInfo>>(
            this,
            []() { return apps::enumerate(); },
            [this](QVector<apps::AppInfo> found) {
                setInstalled(found);
                loading_ = false;
                Q_EMIT loadingChanged();
            });
    }

    int BloatwareModel::appCount() const
    {
        int n = 0;
        for (const Group& g : groups_)
            n += static_cast<int>(g.apps.size());
        return n;
    }

    int BloatwareModel::recommendedCount() const
    {
        int n = 0;
        for (const Group& g : groups_)
            for (const App& a : g.apps)
                if (a.recommended)
                    ++n;
        return n;
    }

    bool BloatwareModel::anyChecked() const
    {
        for (const Group& g : groups_)
            for (const App& a : g.apps)
                if (a.checked)
                    return true;
        return false;
    }

    QVector<BloatwareModel::Selected> BloatwareModel::checkedApps() const
    {
        QVector<Selected> out;
        for (const Group& g : groups_)
            for (const App& a : g.apps)
                if (a.checked)
                    out.append(Selected{ a.packageName, a.packageFullName, a.displayName });
        return out;
    }

    int BloatwareModel::checkRecommended()
    {
        int ticked = 0;
        for (int gi = 0; gi < groups_.size(); ++gi) {
            Group& g = groups_[gi];
            for (int ai = 0; ai < g.apps.size(); ++ai) {
                if (!g.apps[ai].recommended)
                    continue;
                ++ticked;
                if (g.apps[ai].checked)
                    continue;
                g.apps[ai].checked = true;
                const QModelIndex idx = index(ai, ColumnApp, index(gi, 0));
                Q_EMIT dataChanged(idx, idx, { Qt::CheckStateRole });
            }
        }
        if (ticked > 0)
            Q_EMIT checkedChanged();
        return ticked;
    }

    bool BloatwareModel::isApp(const QModelIndex& index) const
    {
        return index.isValid() && index.internalId() != kRootId;
    }

    const BloatwareModel::App* BloatwareModel::appAt(const QModelIndex& index) const
    {
        if (!isApp(index))
            return nullptr;
        const auto gi = static_cast<int>(index.internalId());
        if (gi < 0 || gi >= groups_.size())
            return nullptr;
        const Group& g = groups_.at(gi);
        if (index.row() < 0 || index.row() >= g.apps.size())
            return nullptr;
        return &g.apps.at(index.row());
    }

    BloatwareModel::App* BloatwareModel::appAt(const QModelIndex& index)
    {
        return const_cast<App*>(std::as_const(*this).appAt(index));
    }

    QModelIndex BloatwareModel::index(int row, int column, const QModelIndex& parent) const
    {
        if (row < 0 || column < 0 || column >= ColumnCount)
            return {};

        if (!parent.isValid()) {
            if (row >= groups_.size())
                return {};
            return createIndex(row, column, kRootId);
        }

        if (isApp(parent))
            return {};
        if (parent.row() < 0 || parent.row() >= groups_.size())
            return {};
        if (row >= groups_.at(parent.row()).apps.size())
            return {};
        return createIndex(row, column, static_cast<quintptr>(parent.row()));
    }

    QModelIndex BloatwareModel::parent(const QModelIndex& child) const
    {
        if (!isApp(child))
            return {};
        const auto gi = static_cast<int>(child.internalId());
        if (gi < 0 || gi >= groups_.size())
            return {};
        return createIndex(gi, 0, kRootId);
    }

    int BloatwareModel::rowCount(const QModelIndex& parent) const
    {
        if (!parent.isValid())
            return static_cast<int>(groups_.size());
        if (isApp(parent))
            return 0;
        if (parent.row() < 0 || parent.row() >= groups_.size())
            return 0;
        return static_cast<int>(groups_.at(parent.row()).apps.size());
    }

    int BloatwareModel::columnCount(const QModelIndex&) const
    {
        return ColumnCount;
    }

    QVariant BloatwareModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid())
            return {};

        if (!isApp(index)) {
            if (index.row() < 0 || index.row() >= groups_.size())
                return {};
            const Group& g = groups_.at(index.row());
            switch (role) {
                case Qt::DisplayRole:
                    return index.column() == ColumnApp ? QVariant(g.label) : QVariant();
                case Qt::FontRole: {
                    QFont f;
                    f.setWeight(QFont::Weight(vth::theme::type::WeightMedium));
                    return f;
                }
                case IsAppRole:      return false;
                case RecommendedRole:
                    return g.severity == vth::win::bloatware::Severity::Recommended;
                default:             return {};
            }
        }

        const App* app = appAt(index);
        if (!app)
            return {};

        switch (role) {
            case Qt::DisplayRole:
                switch (index.column()) {
                    case ColumnApp:         return app->displayName;
                    case ColumnSystemName:  return app->packageName;
                    case ColumnDescription: return app->description;
                    default:                return {};
                }
            case Qt::CheckStateRole:
                // Column zero only.
                return index.column() == ColumnApp
                    ? QVariant(app->checked ? Qt::Checked : Qt::Unchecked)
                    : QVariant();
            case Qt::ToolTipRole:
                return index.column() == ColumnDescription ? app->description
                                                           : app->impact;
            case IsAppRole:            return true;
            case PackageNameRole:      return app->packageName;
            case PackageFullNameRole:  return app->packageFullName;
            case RecommendedRole:      return app->recommended;
            default:                   return {};
        }
    }

    bool BloatwareModel::setData(const QModelIndex& index, const QVariant& value, int role)
    {
        if (role != Qt::CheckStateRole || index.column() != ColumnApp)
            return false;
        App* app = appAt(index);
        if (!app)
            return false;

        const bool checked = value.toInt() == Qt::Checked;
        if (app->checked == checked)
            return true;
        app->checked = checked;
        Q_EMIT dataChanged(index, index, { Qt::CheckStateRole });
        Q_EMIT checkedChanged();
        return true;
    }

    QVariant BloatwareModel::headerData(int section, Qt::Orientation orientation,
                                        int role) const
    {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
            return {};
        switch (section) {
            case ColumnApp:         return tr("App");
            case ColumnSystemName:  return tr("System name");
            case ColumnDescription: return tr("Description");
            default:                return {};
        }
    }

    Qt::ItemFlags BloatwareModel::flags(const QModelIndex& index) const
    {
        if (!index.isValid())
            return Qt::NoItemFlags;
                // A heading can't be ticked. Ticking the apps under it is the consent.
        if (!isApp(index))
            return Qt::ItemIsEnabled;
        return index.column() == ColumnApp
            ? (Qt::ItemIsEnabled | Qt::ItemIsUserCheckable)
            : Qt::ItemIsEnabled;
    }

    QHash<int, QByteArray> BloatwareModel::roleNames() const
    {
        QHash<int, QByteArray> names = QAbstractItemModel::roleNames();
        names.insert(Qt::CheckStateRole,  QByteArrayLiteral("checkState"));
        names.insert(IsAppRole,           QByteArrayLiteral("isApp"));
        names.insert(PackageNameRole,     QByteArrayLiteral("packageName"));
        names.insert(PackageFullNameRole, QByteArrayLiteral("packageFullName"));
        names.insert(RecommendedRole,     QByteArrayLiteral("recommended"));
        return names;
    }
}
