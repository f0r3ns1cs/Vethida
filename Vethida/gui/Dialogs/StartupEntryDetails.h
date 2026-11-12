#pragma once

// Read-only startup entry inspector, with one button that flips the entry's state.

#include "Confirm.h"
#include "../Theme/DesignTokens.h"
#include "../Widgets/Callout.h"
#include "../Widgets/CommandButton.h"
#include "../Widgets/FieldValue.h"
#include "../Widgets/ScrimDialog.h"

#include "../../vth_core/Windows/Startup.h"

#include <QDialog>
#include <QFont>
#include <QFrame>
#include <QLabel>
#include <QString>
#include <QVBoxLayout>

#include <optional>

namespace vth::dialogs
{
    namespace detail_startup
    {
        inline QString locationDetail(const vth::win::startup::StartupEntry& e)
        {
            namespace su = vth::win::startup;
            switch (e.location) {
                case su::Location::RunKey: {
                    const QString hive = (e.runKeyHive == su::Hive::CurrentUser)
                        ? QStringLiteral("HKCU") : QStringLiteral("HKLM");
                    return hive + QStringLiteral("\\") + e.runKeySubkey
                         + QStringLiteral("\\") + e.runKeyValue;
                }
                case su::Location::StartupFolder: return e.startupFolderPath;
                case su::Location::ScheduledTask: return e.taskPath;
                case su::Location::DelayedStartService: return e.serviceName;
            }
            return {};
        }

        // One field: its name, and its value under it across the whole card.
        inline void addField(QVBoxLayout* into, QWidget* card,
                             const QString& name, const QString& value, int width)
        {
            auto* block = new QVBoxLayout;
            block->setContentsMargins(0, 0, 0, 0);
            block->setSpacing(vth::theme::tokens::Space2);

            auto* label = new QLabel(name, card);
            label->setProperty("uiRole", QStringLiteral("fieldLabel"));
            block->addWidget(label);

            // The bound makes the wrapping report its real height.
            auto* text = vth::widgets::makeFieldValue(value, card);
            text->setProperty("uiRole", QStringLiteral("fieldValue"));
            text->setMaximumWidth(width);
            block->addWidget(text);

            into->addLayout(block);
        }
    }

    // Returns the new enabled state if the user clicked the Enable/Disable
    // action button, nullopt if they just closed the dialog.
    inline std::optional<bool> showStartupEntryDetails(
        QWidget* host, const vth::win::startup::StartupEntry& entry)
    {
        namespace su = vth::win::startup;
        namespace d  = detail_startup;

        auto* card = new QFrame;
        card->setObjectName(QStringLiteral("srPromptCard"));
        card->setAttribute(Qt::WA_StyledBackground, true);
        card->setFixedWidth(vth::theme::tokens::ModalWidthMd);

        auto* col = new QVBoxLayout(card);
        col->setContentsMargins(vth::theme::tokens::DialogInsetH, vth::theme::tokens::DialogInsetTop,
                                vth::theme::tokens::DialogInsetH, vth::theme::tokens::DialogInsetBottom);
        col->setSpacing(vth::theme::tokens::CardMargin);

        auto* title = new QLabel(QObject::tr("Startup entry"), card);
        title->setObjectName(QStringLiteral("srPromptTitle"));
        col->addWidget(title);

        if (entry.staleDisableMirror) {
            col->addWidget(vth::widgets::makeCallout(
                card, QStringLiteral("alert-triangle"),
                QObject::tr("Still starts with Windows"),
                { su::staleMirrorNotice() }, QStringLiteral("warn"),
                &vth::theme::palette::warningText));
        }

        // The width a value has to wrap inside: the card, less its own insets.
        const int fieldWidth = vth::theme::tokens::ModalWidthMd
                             - 2 * vth::theme::tokens::DialogInsetH;

        auto* fields = new QVBoxLayout;
        fields->setContentsMargins(0, 0, 0, 0);
        fields->setSpacing(vth::theme::tokens::CardMargin);

        d::addField(fields, card, QObject::tr("Name"), entry.displayName, fieldWidth);
        d::addField(fields, card, QObject::tr("Publisher"), entry.publisher, fieldWidth);
        d::addField(fields, card, QObject::tr("Signed"),
                    su::signatureStatusLabel(entry.signature)
                        + (entry.signerName.isEmpty()
                               ? QString()
                               : QStringLiteral(" - ") + entry.signerName),
                    fieldWidth);
        d::addField(fields, card, QObject::tr("Description"), entry.description, fieldWidth);
        d::addField(fields, card, QObject::tr("File path"), entry.resolvedPath, fieldWidth);
        d::addField(fields, card, QObject::tr("Command"), entry.command, fieldWidth);
        d::addField(fields, card, QObject::tr("Location"),
                    su::locationLabel(entry.location)
                        + QStringLiteral(" - ") + d::locationDetail(entry),
                    fieldWidth);
        col->addLayout(fields);

        auto* close = new vth::widgets::CommandButton(vth::widgets::CommandButton::Variant::Secondary,
                                        QObject::tr("Close"), card);
        auto* toggle = new vth::widgets::CommandButton(vth::widgets::CommandButton::Variant::Primary,
            entry.enabled ? QObject::tr("Disable") : QObject::tr("Enable"), card);
        col->addLayout(buttonRow({ close, toggle }));

        vth::widgets::ScrimDialog dlg(card, host);
        dlg.setInitialFocus(close);
        QObject::connect(close, &QPushButton::clicked, &dlg, &QDialog::reject);
        QObject::connect(toggle, &QPushButton::clicked, &dlg, &QDialog::accept);

        if (dlg.exec() != QDialog::Accepted)
            return std::nullopt;
        return !entry.enabled;
    }
}
