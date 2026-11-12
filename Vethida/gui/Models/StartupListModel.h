#pragma once

#include "../../vth_core/Runtime/Fetch.h"
#include "../../vth_core/Windows/Startup.h"

#include <QAbstractTableModel>
#include <QHash>
#include <QString>
#include <QVector>

namespace vth::ui::models
{
    // Everything that launches with Windows, as a model.
    class StartupListModel final : public QAbstractTableModel
    {
        Q_OBJECT

    public:
        enum Column {
            ColumnApp = 0,
            ColumnCompany,
            ColumnFile,
            ColumnType,
            ColumnStatus,
            ColumnCount,
        };

        enum Role {
            SortRole = Qt::UserRole + 1,
            EntryRole,
            EnabledRole,
            // A previous disable didn't finish, so the program still starts.
            StaleMirrorRole,
        };

        explicit StartupListModel(QObject* parent = nullptr);

        void setEntries(QVector<vth::win::startup::StartupEntry> entries);
        void clear();

        // Enumerates off the UI thread, because signature checks are slow.
        void reload();
        bool loading() const { return loading_; }

        const QVector<vth::win::startup::StartupEntry>& entries() const
        {
            return entries_;
        }
        vth::win::startup::StartupEntry entryAt(int row) const;

        void setEnabled(int row, bool enabled);

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
        struct Presentation
        {
            QString appName;
            QString typeLabel;
        };

        const Presentation& presentationAt(int row) const;

        QVector<vth::win::startup::StartupEntry> entries_;
        QVector<Presentation>                    shown_;
        bool                                     loading_ = false;
        FetchToken                               loadToken_;
    };
}
