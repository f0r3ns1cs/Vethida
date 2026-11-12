#include "pch.h"
#include "AppDetailsDialog.h"

#include "../Settings/Preferences.h"

#include "../../vth_core/Runtime/Fetch.h"
#include "../../vth_core/Runtime/Threading.h"
#include "../Pages/SystemTools.h"
#include "../Theme/DesignTokens.h"
#include "../Widgets/CommandButton.h"
#include "../Widgets/FieldValue.h"
#include "../Widgets/ScrimDialog.h"
#include "Confirm.h"

#include <QDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QLocale>
#include <QPushButton>
#include <QVBoxLayout>

namespace vth::dialogs
{
    namespace
    {
        namespace apps   = vth::win::apps;
        namespace tokens = vth::theme::tokens;

        QString formatSize(quint64 bytes)
        {
            if (bytes == 0) return QStringLiteral("-");
            constexpr quint64 GB = 1024ULL * 1024ULL * 1024ULL;
            constexpr quint64 MB = 1024ULL * 1024ULL;
            constexpr quint64 KB = 1024ULL;
            if (bytes >= GB) {
                return QString::asprintf("%.2f GB",
                    static_cast<double>(bytes) / static_cast<double>(GB));
            }
            if (bytes >= MB) {
                return QString::asprintf("%.1f MB",
                    static_cast<double>(bytes) / static_cast<double>(MB));
            }
            if (bytes >= KB) {
                return QString::asprintf("%.0f KB",
                    static_cast<double>(bytes) / static_cast<double>(KB));
            }
            return QString::number(bytes) + QStringLiteral(" B");
        }

        QString formatDate(const QDate& d)
        {
            if (!d.isValid()) return QStringLiteral("-");
            return QLocale::system().toString(d, QLocale::ShortFormat);
        }
    }

    bool showAppDetails(QWidget* host, const apps::AppInfo& app)
    {
        VTH_ASSERT_UI_THREAD();

        auto* card = new QFrame;
        card->setObjectName(QStringLiteral("srPromptCard"));
        card->setAttribute(Qt::WA_StyledBackground, true);
        card->setFixedWidth(tokens::ModalWidthMd);

        auto* col = new QVBoxLayout(card);
        col->setContentsMargins(tokens::DialogInsetH, tokens::DialogInsetTop,
                                tokens::DialogInsetH, tokens::DialogInsetBottom);
        col->setSpacing(tokens::CardMargin);

        auto* title = new QLabel(app.displayName, card);
        title->setObjectName(QStringLiteral("srPromptTitle"));
        title->setWordWrap(true);
        col->addWidget(title);

        auto* form = new QFormLayout;
        form->setContentsMargins(0, 0, 0, 0);
        form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
        form->setHorizontalSpacing(tokens::CardMargin);
        form->setVerticalSpacing(tokens::TightGap);
        form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

        const auto row = [&](const QString& label, const QString& value) {
            form->addRow(label, vth::widgets::makeFieldValue(value, card));
        };
        row(QObject::tr("Publisher"),    app.publisher);
        row(QObject::tr("Version"),      app.version);
        row(QObject::tr("Installed"),    formatDate(app.installDate));
        row(QObject::tr("Size"),         formatSize(app.estimatedBytes));
        row(QObject::tr("Architecture"), apps::architectureLabel(app.architecture));
        row(QObject::tr("Source"),       apps::sourceLabel(app.source));
        row(QObject::tr("Location"),     app.installLocation);
        col->addLayout(form);

        // Focus opens on Close, so Enter on an unread dialog can't remove an app.
        auto* uninstall = new vth::widgets::CommandButton(
            vth::widgets::CommandButton::Variant::Primary,
            QObject::tr("Uninstall"), card);
        // A Store package has no uninstall string, and is removed by package name.
        uninstall->setEnabled(app.type == apps::Type::UWP
            ? !app.packageFullName.isEmpty()
            : (!app.uninstallString.isEmpty() || !app.quietUninstallString.isEmpty()));

        auto* openFolder = new vth::widgets::CommandButton(
            vth::widgets::CommandButton::Variant::Secondary,
            QObject::tr("Open install folder"), card);
        openFolder->setEnabled(!app.installLocation.isEmpty()
                               && QFileInfo::exists(app.installLocation));

        auto* close = new vth::widgets::CommandButton(
            vth::widgets::CommandButton::Variant::Secondary,
            QObject::tr("Close"), card);

        col->addLayout(vth::dialogs::buttonRow({ openFolder, close, uninstall }));

        vth::widgets::ScrimDialog dlg(card, host);
        dlg.setInitialFocus(close);

        bool launched = false;
        QObject::connect(close, &QPushButton::clicked, &dlg, &QDialog::accept);
        QObject::connect(openFolder, &QPushButton::clicked, card, [host, app]() {
            if (!apps::openInstallLocation(app)) {
                vth::dialogs::warn(host, QObject::tr("Install folder"),
                    QObject::tr("Vethida could not open %1. Check that the folder still "
                                "exists and that you have access to it.").arg(app.installLocation));
            }
        });
        QObject::connect(uninstall, &QPushButton::clicked, card,
                         [&dlg, &launched, card, host, app, uninstall]() {
            // Free lists apps. Removing one is Pro.
            if (!vth::ui::tools::ensureTier(host, vth::models::ProductTier::Pro))
                return;
            const bool ask = app.type == apps::Type::UWP || !vth::prefs::askLess();
            if (ask && !vth::dialogs::confirm(
                    host,
                    QObject::tr("Uninstall"),
                    QObject::tr("Vethida starts the uninstaller for %1. "
                                "Follow its prompts to finish the removal.")
                        .arg(app.displayName),
                    QObject::tr("Uninstall")))
                return;

            // Off the UI thread, because a Store removal waits on PowerShell for up to
            // 70s. Parented to the card, which outlives the dialog's event loop.
            uninstall->setEnabled(false);
            const bool isStore = app.type == apps::Type::UWP;
            vth::fetch<bool>(
                card,
                [app]() { return apps::launchUninstall(app); },
                [&dlg, &launched, host, isStore, uninstall](bool started) {
                    if (started) {
                        launched = true;
                        dlg.accept();
                        return;
                    }
                    uninstall->setEnabled(true);
                    vth::dialogs::warn(host, QObject::tr("Uninstall failed"), isStore
                        ? QObject::tr("Windows refused to remove this app. Apps that "
                                      "are part of Windows itself cannot be uninstalled.")
                        : QObject::tr("Vethida could not start the uninstaller for this app. "
                                      "Remove it from Settings > Apps instead."));
                });
        });

        dlg.exec();
        return launched;
    }
}
