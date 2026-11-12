#pragma once

#include <QStringList>
#include <QWidget>

class QLabel;
class QLineEdit;
class QVBoxLayout;

namespace vth::widgets
{
    class CommandButton;

    // An editable list of program names, with a field to add one and a control that
    // puts the shipped list back.
    class NameListEditor final : public QWidget
    {
        Q_OBJECT

    public:
        NameListEditor(const QString& addPlaceholder, const QString& emptyText,
                       QWidget* parent = nullptr);

        QStringList names() const { return names_; }
        void setNames(const QStringList& names);

        // Offered by the reset control, and never shown as a row.
        void setDefaults(const QStringList& names);

        // reason carries one %1, which is filled with the name that was refused.
        void setRejected(const QStringList& names, const QString& reason);

        // Completions for the add field. The caller supplies what is running now.
        void setSuggestions(const QStringList& names);

    Q_SIGNALS:
        void changed();

    private:
        void rebuildRows();
        void addTyped();
        void removeName(const QString& name);
        void showError(const QString& text);
        void clearError();

        static QString normalize(const QString& raw);
        bool contains(const QString& name) const;
        // checks both forms, some protected names have no .exe ("System")
        bool isRejected(const QString& raw, const QString& normalized) const;

        QLayout*     rowsLay_   = nullptr;
        QLabel*      emptyLabel_ = nullptr;
        QLabel*      errorLabel_ = nullptr;
        QLineEdit*   addField_  = nullptr;
        CommandButton* addButton_ = nullptr;
        CommandButton* resetButton_ = nullptr;

        QStringList names_;
        QStringList defaults_;
        QStringList rejected_;
        QString     rejectReason_;
        QString     emptyText_;
    };
}
