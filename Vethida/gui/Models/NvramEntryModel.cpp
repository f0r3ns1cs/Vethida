#include "pch.h"
#include "NvramEntryModel.h"

#include <QLatin1Char>
#include <QObject>
#include <QStringList>
#include <QStringLiteral>
#include <QVariant>

namespace vth::ui::models
{
    namespace
    {
        namespace nv = vth::win::nvram;

        QString valueText(const nv::Entry& e)
        {
            if (e.valueKind != nv::ValueKind::Options)
                return e.value;
            for (const nv::Option& o : e.options)
                if (o.selected)
                    return o.label.isEmpty() ? o.code : o.label;
            return {};
        }
    }

    NvramEntryModel::NvramEntryModel(nv::Document* doc, QObject* parent)
        : QAbstractTableModel(parent), doc_(doc)
    {
    }

    int NvramEntryModel::rowCount(const QModelIndex& parent) const
    {
        return parent.isValid() || !doc_ ? 0 : static_cast<int>(doc_->entries.size());
    }

    int NvramEntryModel::columnCount(const QModelIndex& parent) const
    {
        return parent.isValid() ? 0 : ColumnCount;
    }

    const nv::Entry* NvramEntryModel::entryAt(const QModelIndex& index) const
    {
        if (!doc_ || !index.isValid() || index.row() >= doc_->entries.size())
            return nullptr;
        return &doc_->entries[index.row()];
    }

    QVariant NvramEntryModel::data(const QModelIndex& index, int role) const
    {
        const nv::Entry* e = entryAt(index);
        if (!e)
            return {};

        switch (role) {
        case Qt::DisplayRole:
            return index.column() == ColSetting ? e->setupQuestion : valueText(*e);
        case Qt::EditRole:
            if (index.column() != ColValue)
                return {};
            if (e->valueKind != nv::ValueKind::Options)
                return e->value;
            for (int i = 0; i < e->options.size(); ++i)
                if (e->options[i].selected)
                    return i;
            return 0;
        case Qt::ToolTipRole:
            if (e->isCommented) {
                const QString note = QObject::tr("Read-only: your firmware does not "
                                                 "expose this one for editing.");
                return e->helpString.isEmpty()
                    ? note
                    : QString(e->helpString + QStringLiteral("\n\n") + note);
            }
            return e->helpString;
        case Qt::UserRole:
            // The option labels, for the delegate. Absent means a text field.
            if (index.column() != ColValue || e->valueKind != nv::ValueKind::Options)
                return {};
            {
                QStringList labels;
                labels.reserve(e->options.size());
                for (const nv::Option& o : e->options)
                    labels << (o.label.isEmpty() ? o.code : o.label);
                return labels;
            }
        case SearchRole:
            // QString(), because QT_USE_QSTRINGBUILDER's result won't convert to QVariant.
            return QString(e->setupQuestion + QLatin1Char(' ') + e->helpString
                           + QLatin1Char(' ') + e->token);

        case SettingRole:
            return e->setupQuestion;
        case ValueRole:
            return valueText(*e);
        case HelpRole:
            return e->isCommented
                ? (e->helpString.isEmpty()
                       ? QObject::tr("Read-only: your firmware does not expose "
                                     "this one for editing.")
                       : QString(e->helpString + QStringLiteral("\n\n")
                                 + QObject::tr("Read-only: your firmware does not "
                                               "expose this one for editing.")))
                : e->helpString;
        case OptionsRole: {
            if (e->valueKind != nv::ValueKind::Options)
                return QStringList{};
            QStringList labels;
            labels.reserve(e->options.size());
            for (const nv::Option& o : e->options)
                labels << (o.label.isEmpty() ? o.code : o.label);
            return labels;
        }
        case SelectedRole:
            if (e->valueKind != nv::ValueKind::Options)
                return -1;
            for (int i = 0; i < e->options.size(); ++i)
                if (e->options[i].selected)
                    return i;
            return -1;
        case EditableRole:
            return !e->isCommented;
        case ModifiedRole:
            return !e->isCommented && e->isModified();

        default:
            return {};
        }
    }

    QHash<int, QByteArray> NvramEntryModel::roleNames() const
    {
        QHash<int, QByteArray> names = QAbstractTableModel::roleNames();
        names.insert(SettingRole,    QByteArrayLiteral("setting"));
        names.insert(ValueRole,      QByteArrayLiteral("value"));
        names.insert(HelpRole,       QByteArrayLiteral("help"));
        names.insert(OptionsRole,    QByteArrayLiteral("options"));
        names.insert(SelectedRole,   QByteArrayLiteral("selectedOption"));
        names.insert(EditableRole,   QByteArrayLiteral("editable"));
        names.insert(ModifiedRole,   QByteArrayLiteral("modified"));
        names.insert(SearchRole,     QByteArrayLiteral("searchKey"));
        return names;
    }

    void NvramEntryModel::beginReset()
    {
        beginResetModel();
    }

    void NvramEntryModel::endReset()
    {
        endResetModel();
    }

    bool NvramEntryModel::setValueAt(int row, const QVariant& value)
    {
        return setData(index(row, ColValue), value, Qt::EditRole);
    }

    QVariant NvramEntryModel::headerData(int section, Qt::Orientation o, int role) const
    {
        if (o != Qt::Horizontal || role != Qt::DisplayRole)
            return {};
        return section == ColSetting ? QObject::tr("Setting") : QObject::tr("Value");
    }

    Qt::ItemFlags NvramEntryModel::flags(const QModelIndex& index) const
    {
        Qt::ItemFlags f = QAbstractTableModel::flags(index);
        const nv::Entry* e = entryAt(index);
        if (e && index.column() == ColValue && !e->isCommented)
            f |= Qt::ItemIsEditable;
        return f;
    }

    bool NvramEntryModel::setData(const QModelIndex& index, const QVariant& value, int role)
    {
        if (role != Qt::EditRole || index.column() != ColValue || !doc_
            || !index.isValid() || index.row() >= doc_->entries.size())
            return false;

        nv::Entry& e = doc_->entries[index.row()];
        if (e.isCommented)
            return false;

        if (e.valueKind == nv::ValueKind::Options) {
            const int chosen = value.toInt();
            for (int i = 0; i < e.options.size(); ++i)
                e.options[i].selected = (i == chosen);
        } else {
            e.value = value.toString();
        }
        Q_EMIT dataChanged(index, index,
                           { Qt::DisplayRole, Qt::EditRole,
                             ValueRole, SelectedRole, ModifiedRole });
        return true;
    }
}
