#pragma once

#include "../../vth_core/Runtime/Fetch.h"
#include "../../vth_core/Windows/Apps.h"

#include <QAbstractTableModel>
#include <QHash>
#include <QString>
#include <QVector>

namespace vth::ui::models
{
    // The installed applications, as a model rather than as rows a view builds.
    class AppListModel final : public QAbstractTableModel
    {
        Q_OBJECT

    public:
        enum Column {
            ColumnName = 0,
            ColumnPublisher,
            ColumnVersion,
            ColumnType,
            ColumnCount,
        };

        enum Role {
            // Lower-cased, with unnamed packages past the alphabet.
            SortRole = Qt::UserRole + 1,
            AppRole,
        };

        explicit AppListModel(QObject* parent = nullptr);

        // Replaces every row.
        void setApps(QVector<vth::win::apps::AppInfo> apps);
        // Adds rows without touching the ones already there.
        void appendApps(QVector<vth::win::apps::AppInfo> more);
        void clear();

        // Enumerates off the UI thread, publishing desktop apps before Store apps.
        void reload();
        bool loading() const { return loading_; }

        const QVector<vth::win::apps::AppInfo>& apps() const { return apps_; }
        vth::win::apps::AppInfo appAt(int row) const;

        int rowCount(const QModelIndex& parent = {}) const override;
        int columnCount(const QModelIndex& parent = {}) const override;
        QVariant data(const QModelIndex& index, int role) const override;
        QVariant headerData(int section, Qt::Orientation orientation,
                                          int role) const override;
        Qt::ItemFlags flags(const QModelIndex& index) const override;
        QHash<int, QByteArray> roleNames() const override;

    Q_SIGNALS:
        void loadingChanged();

    private:
        // Precomputed, because data() is called once per cell per paint.
        struct Presentation
        {
            QString shownName;
            QString sortName;
            QString tooltip;
        };

        const Presentation& presentationAt(int row) const;
        Presentation presentationFor(const vth::win::apps::AppInfo& app) const;
        static bool listed(const vth::win::apps::AppInfo& app);

        QVector<vth::win::apps::AppInfo> apps_;
        QVector<Presentation>            shown_;
        bool                             loading_ = false;
        FetchToken                       loadToken_;
        // A token of its own, because canceling a reload has to stop both.
        FetchToken                       storeToken_;
    };
}
