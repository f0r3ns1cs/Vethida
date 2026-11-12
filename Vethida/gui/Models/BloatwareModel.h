#pragma once

#include "../../vth_core/Runtime/Fetch.h"
#include "../../vth_core/Windows/Apps.h"
#include "../../vth_core/Windows/Bloatware.h"

#include <QAbstractItemModel>
#include <QHash>
#include <QString>
#include <QVector>

namespace vth::ui::models
{
    // The preinstalled apps this PC actually has, grouped by severity.
    class BloatwareModel final : public QAbstractItemModel
    {
        Q_OBJECT

        Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
        Q_PROPERTY(bool anyChecked READ anyChecked NOTIFY checkedChanged)

    public:
        enum Column {
            ColumnApp = 0,
            ColumnSystemName,
            ColumnDescription,
            ColumnCount,
        };

        enum Role {
            IsAppRole = Qt::UserRole + 1,
            PackageNameRole,
            PackageFullNameRole,
            RecommendedRole,
        };

        // a checked app and what's needed to remove it
        struct Selected
        {
            QString packageName;
            QString packageFullName;
            QString displayName;
        };

        explicit BloatwareModel(QObject* parent = nullptr);

        // Matches the catalog against what's installed. Empty groups don't appear.
        void setInstalled(const QVector<vth::win::apps::AppInfo>& installed);
        void clear();

        // Enumerate and rebuild, off the UI thread.
        Q_INVOKABLE void reload();
        bool loading() const { return loading_; }

        int  appCount() const;
        int  recommendedCount() const;
        bool anyChecked() const;
        QVector<Selected> checkedApps() const;

        // Ticks every app under Recommended and returns how many that was.
        int checkRecommended();

        QModelIndex index(int row, int column,
                                        const QModelIndex& parent = {}) const override;
        QModelIndex parent(const QModelIndex& child) const override;
        int rowCount(const QModelIndex& parent = {}) const override;
        int columnCount(const QModelIndex& parent = {}) const override;
        QVariant data(const QModelIndex& index, int role) const override;
        bool setData(const QModelIndex& index, const QVariant& value, int role) override;
        QVariant headerData(int section, Qt::Orientation orientation,
                                          int role) const override;
        Qt::ItemFlags flags(const QModelIndex& index) const override;
        QHash<int, QByteArray> roleNames() const override;

    Q_SIGNALS:
        // A tick moved, or the tree was rebuilt.
        void checkedChanged();
        void loadingChanged();

    private:
        struct App
        {
            QString packageName;       // the catalog's name, and Windows'
            QString packageFullName;   // what Remove-AppxPackage takes
            QString displayName;
            QString description;       // with the impact appended where it is Risky
            QString impact;
            bool    recommended = false;
            bool    checked     = false;
        };

        struct Group
        {
            vth::win::bloatware::Severity severity =
                vth::win::bloatware::Severity::Recommended;
            QString      label;
            QVector<App> apps;
        };

        // A child index carries its group's position in internalId(), and a group
        // index carries this. A pointer would go stale when the vectors reallocate.
        static constexpr quintptr kRootId = ~quintptr(0);

        bool isApp(const QModelIndex& index) const;
        const App* appAt(const QModelIndex& index) const;
        App*       appAt(const QModelIndex& index);

        QVector<Group> groups_;
        bool           loading_ = false;
        FetchToken     loadToken_;
    };
}
