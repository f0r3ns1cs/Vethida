#pragma once

#include "../../vth_core/Windows/Nvram.h"

#include <QAbstractTableModel>
#include <QHash>
#include <QVariant>

namespace vth::ui::models
{
    // One row per setup question, in the firmware's order. Column 1 is read-only for
    // entries AMISCE commented out.
    class NvramEntryModel final : public QAbstractTableModel
    {
        Q_OBJECT

    public:
        // What the search box matches on, in one string.
        static constexpr int SearchRole = Qt::UserRole + 1;

        enum Role {
            SettingRole = Qt::UserRole + 2,
            ValueRole,
            HelpRole,
            // The option labels, or an empty list for a free-text entry.
            OptionsRole,
            SelectedRole,
            EditableRole,
            ModifiedRole,
        };
        Q_ENUM(Role)

        enum Column { ColSetting = 0, ColValue = 1, ColumnCount = 2 };

        NvramEntryModel(vth::win::nvram::Document* doc, QObject* parent);

        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& index, int role) const override;
        QVariant headerData(int section, Qt::Orientation o,
                                          int role) const override;
        Qt::ItemFlags flags(const QModelIndex& index) const override;
        bool setData(const QModelIndex& index, const QVariant& value, int role) override;

        const vth::win::nvram::Entry* entryAt(const QModelIndex& index) const;

        QHash<int, QByteArray> roleNames() const override;

        // Bracket a replacement of the document this model points into.
        void beginReset();
        void endReset();

        // Returns false for a row the firmware won't accept a write for.
        Q_INVOKABLE bool setValueAt(int row, const QVariant& value);

    private:
        vth::win::nvram::Document* doc_ = nullptr;
    };
}
