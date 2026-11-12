#include "pch.h"
#include "NameListEditor.h"

#include "CommandButton.h"
#include "FlowLayout.h"
#include "../Theme/DesignTokens.h"
#include "../Utils/TablerIcon.h"

#include <QCompleter>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QToolButton>
#include <QVBoxLayout>

namespace vth::widgets
{
    namespace
    {
        namespace tokens = vth::theme::tokens;
        namespace pal    = vth::theme::palette;
    }

    NameListEditor::NameListEditor(const QString& addPlaceholder,
                                   const QString& emptyText, QWidget* parent)
        : QWidget(parent)
        , emptyText_(emptyText)
    {
        auto* outer = new QVBoxLayout(this);
        outer->setContentsMargins(0, 0, 0, 0);
        outer->setSpacing(tokens::TightGap);

        emptyLabel_ = new QLabel(emptyText_, this);
        emptyLabel_->setObjectName(QStringLiteral("settingsRowDesc"));
        emptyLabel_->setWordWrap(true);
        outer->addWidget(emptyLabel_);

        // No scroll of its own, because the host pane owns the one scroll.
        auto* host = new QWidget(this);
        rowsLay_ = new FlowLayout(host, tokens::Space8, tokens::Space8);
        outer->addWidget(host);

        auto* addRow = new QHBoxLayout;
        addRow->setContentsMargins(0, 0, 0, 0);
        addRow->setSpacing(tokens::CompactGap);

        addField_ = new QLineEdit(this);
        addField_->setPlaceholderText(addPlaceholder);
        addField_->setAccessibleName(addPlaceholder);
        addField_->setClearButtonEnabled(true);
        connect(addField_, &QLineEdit::returnPressed, this, &NameListEditor::addTyped);
        connect(addField_, &QLineEdit::textEdited, this, [this](const QString&) {
            clearError();
        });
        addRow->addWidget(addField_, 1);

        addButton_ = new CommandButton(CommandButton::Variant::Secondary, tr("Add"), this);
        addButton_->setCursor(Qt::PointingHandCursor);
        connect(addButton_, &QPushButton::clicked, this, &NameListEditor::addTyped);
        addRow->addWidget(addButton_, 0);

        resetButton_ = new CommandButton(CommandButton::Variant::Secondary,
                                         tr("Reset to defaults"), this);
        resetButton_->setCursor(Qt::PointingHandCursor);
        connect(resetButton_, &QPushButton::clicked, this, [this]() {
            setNames(defaults_);
            Q_EMIT changed();
        });
        addRow->addWidget(resetButton_, 0);
        outer->addLayout(addRow);

        errorLabel_ = new QLabel(this);
        errorLabel_->setProperty("uiRole", QStringLiteral("formError"));
        errorLabel_->setWordWrap(true);
        errorLabel_->setVisible(false);
        outer->addWidget(errorLabel_);

        rebuildRows();
    }

    // A rejected entry is dropped, so a hand-edited list converges on what actually runs.
    void NameListEditor::setNames(const QStringList& names) {
        names_.clear();
        for (const QString& raw : names) {
            const QString name = normalize(raw);
            if (name.isEmpty() || contains(name)) continue;
            if (isRejected(raw, name)) continue;
            names_ << name;
        }
        rebuildRows();
    }

    void NameListEditor::setDefaults(const QStringList& names) {
        defaults_ = names;
        if (resetButton_)
            resetButton_->setVisible(!defaults_.isEmpty());
    }

    void NameListEditor::setRejected(const QStringList& names, const QString& reason) {
        rejected_.clear();
        for (const QString& n : names) rejected_ << n.trimmed().toLower();
        rejectReason_ = reason;
    }

    void NameListEditor::setSuggestions(const QStringList& names) {
        if (!addField_) return;
        auto* completer = new QCompleter(names, addField_);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        completer->setFilterMode(Qt::MatchContains);
        completer->setCompletionMode(QCompleter::PopupCompletion);
        addField_->setCompleter(completer);
    }

    bool NameListEditor::contains(const QString& name) const {
        return names_.contains(name, Qt::CaseInsensitive);
    }

    bool NameListEditor::isRejected(const QString& raw, const QString& normalized) const {
        return rejected_.contains(raw.trimmed().toLower())
            || rejected_.contains(normalized.toLower());
    }

    // "discord", "C:\...\Discord.exe " etc all become discord.exe
    QString NameListEditor::normalize(const QString& raw) {
        QString text = raw.trimmed();
        if (text.startsWith(QLatin1Char('"')) && text.endsWith(QLatin1Char('"')) && text.size() > 1)
            text = text.mid(1, text.size() - 2).trimmed();
        if (text.isEmpty())
            return {};
        text = QFileInfo(text).fileName();
        if (text.isEmpty())
            return {};
        if (!text.contains(QLatin1Char('.')))
            text += QStringLiteral(".exe");
        return text;
    }

    void NameListEditor::addTyped() {
        if (!addField_) return;
        const QString typed = addField_->text();
        const QString name  = normalize(typed);
        if (name.isEmpty()) {
            showError(tr("Type the name of an app, like Discord.exe."));
            return;
        }
        if (isRejected(typed, name)) {
            showError(rejectReason_.arg(typed.trimmed()));
            return;
        }
        if (contains(name)) {
            showError(tr("%1 is already on this list.").arg(name));
            return;
        }
        names_ << name;
        addField_->clear();
        clearError();
        rebuildRows();
        Q_EMIT changed();
    }

    void NameListEditor::removeName(const QString& name) {
        for (int i = 0; i < names_.size(); ++i) {
            if (names_.at(i).compare(name, Qt::CaseInsensitive) != 0) continue;
            names_.removeAt(i);
            clearError();
            rebuildRows();
            Q_EMIT changed();
            return;
        }
    }

    void NameListEditor::showError(const QString& text) {
        if (!errorLabel_) return;
        errorLabel_->setText(text);
        errorLabel_->setVisible(true);
    }

    void NameListEditor::clearError() {
        if (errorLabel_ && errorLabel_->isVisible())
            errorLabel_->setVisible(false);
    }

    void NameListEditor::rebuildRows() {
        if (!rowsLay_) return;

        while (QLayoutItem* item = rowsLay_->takeAt(0)) {
            if (QWidget* w = item->widget()) {
                // Hidden now: deleteLater leaves it painted over the new
                // chip in its slot until the event loop gets to it.
                w->hide();
                w->deleteLater();
            }
            delete item;
        }

        QWidget* host = rowsLay_->parentWidget();
        for (const QString& name : names_) {
            auto* chip = new QFrame(host);
            chip->setObjectName(QStringLiteral("nameChip"));
            chip->setAttribute(Qt::WA_StyledBackground, true);
            auto* lay = new QHBoxLayout(chip);
            lay->setContentsMargins(tokens::Space12, tokens::Space2, tokens::Space2, tokens::Space2);
            lay->setSpacing(tokens::Space4);

            auto* label = new QLabel(name, chip);
            label->setObjectName(QStringLiteral("nameChipText"));
            lay->addWidget(label, 0, Qt::AlignVCenter);

            auto* remove = new QToolButton(chip);
            remove->setObjectName(QStringLiteral("nameChipRemove"));
            remove->setCursor(Qt::PointingHandCursor);
            remove->setAccessibleName(tr("Remove %1").arg(name));
            remove->setToolTip(tr("Remove %1").arg(name));
            vth::icons::applyIconButton(remove, QStringLiteral("x"),
                                        &pal::textSecondary, tokens::GlyphSm);
            connect(remove, &QToolButton::clicked, this,
                    [this, name]() { removeName(name); });
            lay->addWidget(remove, 0, Qt::AlignVCenter);

            // polish first or it measures with the wrong font
            chip->ensurePolished();
            rowsLay_->addWidget(chip);
        }

        if (emptyLabel_)
            emptyLabel_->setVisible(names_.isEmpty());
    }
}
