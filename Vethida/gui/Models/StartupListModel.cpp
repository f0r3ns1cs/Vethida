#include "pch.h"
#include "StartupListModel.h"

#include "../../vth_core/Windows/Apps.h"

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
        namespace su = vth::win::startup;

        // A name containing a braced GUID is an installer's bookkeeping key.
        bool nameIsMachineGenerated(const QString& s)
        {
            static const QRegularExpression guid(
                QStringLiteral("\\{[0-9A-Fa-f]{8}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-"
                               "[0-9A-Fa-f]{4}-[0-9A-Fa-f]{12}\\}"));
            return guid.match(s).hasMatch();
        }

        QString appNameFor(const su::StartupEntry& e)
        {
            QString name = e.description.isEmpty() ? e.displayName : e.description;
            if (!e.fileName.isEmpty() && nameIsMachineGenerated(name))
                name = e.fileName;
            if (name.endsWith(QStringLiteral(".lnk"), Qt::CaseInsensitive))
                name.chop(4);
            return name;
        }
    }

    StartupListModel::StartupListModel(QObject* parent)
        : QAbstractTableModel(parent)
    {
    }

    void StartupListModel::setEntries(QVector<su::StartupEntry> list)
    {
        beginResetModel();
        entries_ = std::move(list);

        shown_.clear();
        shown_.reserve(entries_.size());
        for (const su::StartupEntry& e : entries_) {
            Presentation p;
            p.appName = appNameFor(e);

            if (e.location != su::Location::RunKey) {
                p.typeLabel = su::locationLabel(e.location);
            } else {
                const bool once = e.runKeySubkey.endsWith(QStringLiteral("RunOnce"),
                                                          Qt::CaseInsensitive);
                const bool allUsers = (e.runKeyHive == su::Hive::LocalMachine);
                const QString kind  = once ? tr("Run once") : tr("Run key");
                const QString scope = allUsers ? tr("all users") : tr("this user");
                p.typeLabel = e.runKeyWow64 ? tr("%1, %2 (32-bit)").arg(kind, scope)
                                            : tr("%1, %2").arg(kind, scope);
            }

            shown_.append(std::move(p));
        }

        endResetModel();
    }

    void StartupListModel::clear()
    {
        setEntries({});
    }

    void StartupListModel::reload()
    {
        loadToken_.cancel();
        loading_ = true;
        Q_EMIT loadingChanged();

        loadToken_ = vth::fetch<QVector<su::StartupEntry>>(
            this,
            []() { return su::enumerate(); },
            [this](QVector<su::StartupEntry> entries) {
                setEntries(std::move(entries));
                loading_ = false;
                Q_EMIT loadingChanged();
            });
    }

    su::StartupEntry StartupListModel::entryAt(int row) const
    {
        if (row < 0 || row >= entries_.size())
            return {};
        return entries_.at(row);
    }

    void StartupListModel::setEnabled(int row, bool enabled)
    {
        if (row < 0 || row >= entries_.size())
            return;
        if (entries_[row].enabled == enabled)
            return;
        entries_[row].enabled = enabled;
        // The whole row, because every cell's tooltip derives from the entry.
        const QModelIndex first = index(row, 0);
        const QModelIndex last  = index(row, ColumnCount - 1);
        Q_EMIT dataChanged(first, last,
                           { Qt::DisplayRole, Qt::ToolTipRole, SortRole,
                             EntryRole, EnabledRole });
    }

    const StartupListModel::Presentation& StartupListModel::presentationAt(int row) const
    {
        static const Presentation kEmpty;
        if (row < 0 || row >= shown_.size())
            return kEmpty;
        return shown_.at(row);
    }

    int StartupListModel::rowCount(const QModelIndex& parent) const
    {
        return parent.isValid() ? 0 : static_cast<int>(entries_.size());
    }

    int StartupListModel::columnCount(const QModelIndex& parent) const
    {
        return parent.isValid() ? 0 : ColumnCount;
    }

    QVariant StartupListModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid() || index.row() < 0 || index.row() >= entries_.size())
            return {};

        const su::StartupEntry& e = entries_.at(index.row());
        const Presentation&     p = presentationAt(index.row());

        switch (role) {
            case Qt::DisplayRole:
                switch (index.column()) {
                    case ColumnApp:     return p.appName;
                    case ColumnCompany: return e.publisher;
                    case ColumnFile:    return e.fileName;
                    case ColumnType:    return p.typeLabel;
                    case ColumnStatus:  return {};
                    default:            return {};
                }
            case Qt::ToolTipRole:
                // An unfinished disable shows on every cell.
                if (e.staleDisableMirror)
                    return su::staleMirrorNotice();
                return index.column() == ColumnApp && !e.resolvedPath.isEmpty()
                    ? QVariant(e.resolvedPath)
                    : QVariant();
            case Qt::AccessibleDescriptionRole:
                if (e.staleDisableMirror && index.column() == ColumnApp)
                    return su::staleMirrorNotice();
                return {};
            case SortRole:
                switch (index.column()) {
                    case ColumnApp:     return p.appName.toLower();
                    case ColumnCompany: return e.publisher.toLower();
                    case ColumnFile:    return e.fileName.toLower();
                    case ColumnType:    return p.typeLabel.toLower();
                    case ColumnStatus:  return e.enabled ? 1 : 0;
                    default:            return {};
                }
            case Qt::DecorationRole:
                // On the application column only, or the tree repeats it in every cell.
                if (index.column() != ColumnApp)
                    return {};
                if (e.iconImage.isNull()) {
                    static const QIcon kDefault = [] {
                        const QImage img = vth::win::apps::defaultAppIcon();
                        return img.isNull() ? QIcon() : QIcon(QPixmap::fromImage(img));
                    }();
                    return QVariant::fromValue(kDefault);
                }
                return QVariant::fromValue(QIcon(QPixmap::fromImage(e.iconImage)));
            case EntryRole:       return QVariant::fromValue(e);
            case EnabledRole:     return e.enabled;
            case StaleMirrorRole: return e.staleDisableMirror;
            default:
                return {};
        }
    }

    QVariant StartupListModel::headerData(int section, Qt::Orientation orientation,
                                          int role) const
    {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
            return {};
        switch (section) {
            case ColumnApp:     return tr("Application");
            case ColumnCompany: return tr("Company");
            case ColumnFile:    return tr("File name");
            case ColumnType:    return tr("Type");
            case ColumnStatus:  return tr("Status");
            default:            return {};
        }
    }

    Qt::ItemFlags StartupListModel::flags(const QModelIndex& index) const
    {
        if (!index.isValid())
            return Qt::NoItemFlags;
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }

    QHash<int, QByteArray> StartupListModel::roleNames() const
    {
        QHash<int, QByteArray> names = QAbstractTableModel::roleNames();
        names.insert(SortRole,        QByteArrayLiteral("sortKey"));
        names.insert(EntryRole,       QByteArrayLiteral("entry"));
        names.insert(EnabledRole,     QByteArrayLiteral("startsWithWindows"));
        names.insert(StaleMirrorRole, QByteArrayLiteral("staleDisableMirror"));
        return names;
    }
}
