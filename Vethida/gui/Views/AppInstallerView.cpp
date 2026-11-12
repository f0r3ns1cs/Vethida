#include "pch.h"
#include "AppInstallerView.h"

#include "../Theme/DesignTokens.h"
#include "../Widgets/CommandButton.h"
#include "../Widgets/ListGroup.h"
#include "../Widgets/ToastManager.h"

#include "../../vth_core/Runtime/Dispatcher.h"
#include "../../vth_core/Runtime/Fetch.h"
#include "../../vth_core/Runtime/ServiceRegistry.h"
#include "../../vth_core/Windows/AppInstaller.h"

#include <QRegularExpression>
#include <QPointer>
#include <QPushButton>
#include <QSet>
#include <QStringList>
#include <QVBoxLayout>

namespace vth::ui
{
    namespace tokens = vth::theme::tokens;
    namespace inst   = vth::win::installer;

    namespace
    {
        struct InstallScan
        {
            QSet<QString> installed;
        };
    }

    AppInstallerView::AppInstallerView(QWidget* parent) : QWidget(parent)
    {
        setObjectName(QStringLiteral("appInstallerView"));
        buildUi();
        scanInstalledState();
    }

    namespace
    {
        // "Downloading 40%" -> 40; -1 when the line carries no percentage.
        int percentIn(const QString& text)
        {
            static const QRegularExpression re(QStringLiteral(R"((\d{1,3})%)"));
            const auto m = re.match(text);
            return m.hasMatch() ? m.captured(1).toInt() : -1;
        }
    }

    void AppInstallerView::buildUi()
    {
        // A Managed pane: the shell owns the scrolling and the inset.
        auto* lay = new QVBoxLayout(this);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(tokens::RegionMargin);

        vth::widgets::ListGroup* group = nullptr;
        QString currentCategory;
        for (const auto& pkg : inst::catalog()) {
            if (!group || pkg.category != currentCategory) {
                currentCategory = pkg.category;
                group = new vth::widgets::ListGroup(pkg.category, this);
                lay->addWidget(group);
            }

            vth::widgets::ListRow* listRow =
                group->addRow(pkg.name, pkg.description);

            auto* install = new vth::widgets::CommandButton(
                vth::widgets::CommandButton::Variant::Secondary, tr("Install"), listRow);
            install->setAccessibleName(tr("Install %1").arg(pkg.name));
            // Held until the scan says what is already installed, so a click
            // cannot race it into a second install of something present.
            install->setEnabled(false);
            listRow->addTrailing(install);

            const InstallRow row{ pkg.id, install, listRow };
            connect(install, &QPushButton::clicked, this, [this, row]() { startInstall(row); });
            rows_.append(row);
        }
    }

    void AppInstallerView::markInstalled(const InstallRow& row, const QString& text)
    {
        row.button->setEnabled(false);
        row.button->setText(tr("Installed"));
        row.row->setProgress(-1);
        row.row->setStatus(text, vth::widgets::ListRow::Tone::Good);
    }

    void AppInstallerView::startInstall(const InstallRow& row)
    {
        if (row.button->isLoading())
            return;
        row.button->setLoading(true);
        row.row->setStatus(tr("Starting"));

        // progress from the worker, dropped if the row is gone
        QPointer<vth::widgets::ListRow> rowGuard(row.row);
        const inst::StatusFn progress = [rowGuard](const QString& text) {
            vth::Dispatcher::instance().dispatchUi([rowGuard, text]() {
                if (!rowGuard)
                    return;
                const int pct = percentIn(text);
                rowGuard->setProgress(pct);
                rowGuard->setStatus(pct >= 0 ? AppInstallerView::tr("Downloading") : text);
            });
        };

        const QString id = row.id;
        vth::fetch<inst::InstallOutcome>(
            this,
            [id, progress]() { return inst::install(id, progress); },
            [this, row](inst::InstallOutcome outcome) {
                row.button->setLoading(false);
                if (outcome.result == inst::InstallResult::Ok
                    || outcome.result == inst::InstallResult::AlreadyInstalled) {
                    markInstalled(row, outcome.message);
                    return;
                }
                row.row->setProgress(-1);
                if (outcome.message.isEmpty())
                    outcome.message = tr("The install stopped unexpectedly. Try again in a "
                                         "moment.");
                row.row->setStatus(outcome.message, vth::widgets::ListRow::Tone::Bad);
                if (auto* toasts = vth::ServiceRegistry::instance().get<vth::widgets::ToastManager>())
                    toasts->warning(tr("Not installed"), outcome.message);
            });
    }

    void AppInstallerView::scanInstalledState()
    {
        for (const InstallRow& r : rows_)
            r.row->setStatus(tr("Checking"));

        vth::fetch<InstallScan>(
            this,
            []() {
                InstallScan s;
                QStringList ids;
                for (const auto& pkg : inst::catalog())
                    ids << pkg.id;
                s.installed = inst::installedAmong(ids);
                return s;
            },
            [this](InstallScan s) {
                // always usable, there's a fallback install route
                for (const InstallRow& r : rows_) {
                    r.row->setStatus(QString());
                    if (s.installed.contains(r.id)) {
                        // Found, not installed just now: no mark drawn in, and
                        // the line stays muted rather than reporting a success.
                        r.button->setEnabled(false);
                        r.button->setText(tr("Installed"));
                        continue;
                    }
                    r.button->setEnabled(true);
                }
            });
    }
}
