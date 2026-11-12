#include "pch.h"
#include "GpuDriverView.h"

#include "../Dialogs/Confirm.h"
#include "../Pages/SystemTools.h"
#include "../Theme/DesignTokens.h"
#include "../Utils/TablerIcon.h"
#include "../Widgets/Callout.h"
#include "../Widgets/CommandButton.h"
#include "../Widgets/GlyphLabel.h"
#include "../Widgets/ScrimDialog.h"

#include "../../vth_core/Models/ProductTier.h"
#include "../../vth_core/Runtime/Dispatcher.h"
#include "../../vth_core/Windows/NvidiaAnsel.h"
#include "../../vth_core/Windows/NvidiaDriver.h"
#include "../../vth_core/Windows/NvidiaReinstall.h"

#include <QCoreApplication>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPointer>
#include <QProcess>
#include <QProgressBar>
#include <QScopeGuard>
#include <QPushButton>
#include <QSet>
#include <QSize>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace vth::ui
{
    namespace tokens = vth::theme::tokens;
    namespace pal    = vth::theme::palette;

    namespace
    {
        QString humanBytes(qint64 b)
        {
            if (b >= 1024ll * 1024 * 1024)
                return QStringLiteral("%1 GB").arg(b / double(1024ll * 1024 * 1024), 0, 'f', 1);
            if (b >= 1024 * 1024)
                return QStringLiteral("%1 MB").arg(b / (1024 * 1024));
            if (b >= 1024)
                return QStringLiteral("%1 KB").arg(b / 1024);
            return QStringLiteral("%1 B").arg(b);
        }
    }

    namespace
    {
        namespace nv = vth::win::nvidia;
        namespace ri = vth::win::nvidia::reinstall;

        QString T(const char* s)
        {
            return QCoreApplication::translate("vth::ui::GpuDriverView", s);
        }

        struct RunOutcome { bool ok = false; bool restart = false; QString message; };

        // install, verify, give driver delivery back to Windows Update. Clears setRunning.
        RunOutcome installPhase(const QString& extractDir, const QString& version,
                                const std::function<void(const QString&, int)>& post)
        {
            const auto running = qScopeGuard([] { ri::setRunning(false); });

            ri::Marker m = ri::readMarker();
            m.stage = QStringLiteral("installing");
            ri::writeMarker(m);

            const ri::InstallOutcome inst = ri::installAndVerify(
                extractDir, version, m.previousDir, m.previousVersion,
                [&post](const QString& stage) { post(stage, 92); });

            // always, success or not
            ri::restoreDriverDelivery(m.driverDelivery);
            m.driverDelivery = QJsonObject();

            // The previous driver is back on the card: nothing is left to finish,
            // so the prepared package and the backup go.
            if (inst.restoredPrevious) {
                ri::clearWorkspace();
                return { false, inst.restartRequired, inst.message };
            }

            if (!inst.ok) {
                m.stage = QStringLiteral("failed");
                ri::writeMarker(m);
                return { false, false, inst.message };
            }

            post(T("Turning off driver telemetry..."), 96);
            QSet<QString> settings;
            for (const nv::InstallSetting& s : nv::installSettingsCatalog())
                if (s.defaultEnabled)
                    settings.insert(s.id);
            if (settings.contains(QStringLiteral("drvtlm")))
                nv::applyDrvtlmRuntime();
            if (settings.contains(QStringLiteral("nvidiaansel")))
                nv::toggleAnsel(true);

            post(T("Applying the driver settings..."), 99);
            QSet<QString> tweakIds;
            for (const nv::NvidiaRegTweak& t : nv::registryTweaksCatalog())
                if (t.defaultEnabled)
                    tweakIds.insert(t.id);
            if (!tweakIds.isEmpty())
                nv::applyRegistryTweaks(tweakIds);

            ri::clearWorkspace();
            return { true, inst.restartRequired, inst.message };
        }
    }

    GpuDriverView::GpuDriverView(QWidget* parent)
        : QWidget(parent)
    {
        setObjectName(QStringLiteral("GpuDriverView"));
        buildUi();
    }

    GpuDriverView::~GpuDriverView()
    {
        // Order matters. The flag stops the download loop inside the worker;
        // the token stops the result being delivered into a destroyed view.
        if (cancel_)
            cancel_->store(true);
        runToken_.cancel();
    }

    void GpuDriverView::buildUi()
    {
        auto* col = new QVBoxLayout(this);
        col->setContentsMargins(0, 0, 0, 0);
        col->setSpacing(tokens::CardMargin);

        auto* blurb = new QLabel(
            tr("Replaces the installed NVIDIA driver with a clean copy of the "
               "current one."), this);
        blurb->setProperty("uiRole", QStringLiteral("bodyText"));
        blurb->setWordWrap(true);
        col->addWidget(blurb);

        auto* checks = new QVBoxLayout;
        checks->setContentsMargins(0, 0, 0, 0);
        checks->setSpacing(tokens::TightGap);
        checks->addWidget(buildCheckRow(
            tr("Takes a System Restore point first, if Windows has System "
               "Protection turned on")));
        checks->addWidget(buildCheckRow(
            tr("Downloads the current driver for the card in this PC")));
        checks->addWidget(buildCheckRow(
            tr("Keeps the display driver and leaves the overlay, the capture "
               "tools and the updater out of the package")));
        checks->addWidget(buildCheckRow(
            tr("Saves a copy of your current driver and puts it back if the new "
               "one will not install")));
        checks->addWidget(buildCheckRow(
            tr("Clears out the installed driver and what earlier versions left "
               "behind, including the copies Windows keeps staged")));
        checks->addWidget(buildCheckRow(
            tr("Switches off the telemetry service and the reporting tasks the "
               "new install turns back on")));
        col->addLayout(checks);

        col->addStretch(1);

        actionStack_ = new QStackedWidget(this);

        auto* beginPage = new QWidget(actionStack_);
        auto* beginRow = new QHBoxLayout(beginPage);
        beginRow->setContentsMargins(0, 0, 0, 0);
        beginRow->setSpacing(0);
        beginBtn_ = new vth::widgets::CommandButton(
            vth::widgets::CommandButton::Variant::Primary,
            // Doubled, because a single & is Qt's mnemonic marker.
            tr("Clean && reinstall"), beginPage);
        beginBtn_->setCursor(Qt::PointingHandCursor);
        beginBtn_->setIcon(vth::icons::icon(QStringLiteral("refresh-cw"),
                                            pal::accentText(), 16));
        beginBtn_->setIconSize(QSize(tokens::GlyphMd, tokens::GlyphMd));
        connect(beginBtn_, &QPushButton::clicked, this, &GpuDriverView::onBegin);
        beginRow->addStretch(1);
        beginRow->addWidget(beginBtn_, 0, Qt::AlignVCenter);
        actionStack_->addWidget(beginPage);

        auto* progPage = new QWidget(actionStack_);
        auto* progCol = new QVBoxLayout(progPage);
        progCol->setContentsMargins(0, 0, 0, 0);
        progCol->setSpacing(tokens::TightGap);

        stageLabel_ = new QLabel(progPage);
        stageLabel_->setProperty("uiRole", QStringLiteral("fieldValue"));
        stageLabel_->setWordWrap(true);
        progCol->addWidget(stageLabel_);

        progress_ = new QProgressBar(progPage);
        progress_->setProperty("uiRole", QStringLiteral("progressBar"));
        progress_->setRange(0, 100);
        progress_->setValue(0);
        progress_->setTextVisible(false);
        progCol->addWidget(progress_);

        auto* metaRow = new QHBoxLayout;
        metaRow->setContentsMargins(0, 0, 0, 0);
        metaRow->setSpacing(tokens::CompactGap);
        bytesLabel_ = new QLabel(progPage);
        bytesLabel_->setProperty("uiRole", QStringLiteral("metaText"));
        metaRow->addWidget(bytesLabel_, 0, Qt::AlignLeft);
        percentLabel_ = new QLabel(progPage);
        percentLabel_->setProperty("uiRole", QStringLiteral("metaText"));
        metaRow->addWidget(percentLabel_, 0, Qt::AlignLeft);
        metaRow->addStretch(1);

        // Cancellable until the installer runs. After that the only safe direction is
        // forward, so the button is disabled.
        cancelBtn_ = new vth::widgets::CommandButton(
            vth::widgets::CommandButton::Variant::Secondary, tr("Cancel"), progPage);
        cancelBtn_->setCursor(Qt::PointingHandCursor);
        connect(cancelBtn_, &QPushButton::clicked, this, [this]() {
            if (cancel_)
                cancel_->store(true);
            cancelBtn_->setEnabled(false);
            if (stageLabel_)
                stageLabel_->setText(tr("Stopping after the current step..."));
        });
        metaRow->addWidget(cancelBtn_, 0, Qt::AlignRight);
        progCol->addLayout(metaRow);

        actionStack_->addWidget(progPage);
        actionStack_->setCurrentIndex(0);
        col->addWidget(actionStack_);

        resultLabel_ = new QLabel(this);
        resultLabel_->setProperty("uiRole", QStringLiteral("bodyText"));
        resultLabel_->setWordWrap(true);
        resultLabel_->hide();
        col->addWidget(resultLabel_);

        restartBtn_ = new vth::widgets::CommandButton(
            vth::widgets::CommandButton::Variant::Primary, tr("Restart now"), this);
        restartBtn_->hide();
        connect(restartBtn_, &QPushButton::clicked, this, &GpuDriverView::restartNow);
        auto* restartRow = new QHBoxLayout;
        restartRow->setContentsMargins(0, 0, 0, 0);
        restartRow->addStretch(1);
        restartRow->addWidget(restartBtn_);
        col->addLayout(restartRow);

        // an install that didn't finish, shown first
        const ri::Marker unfinished = ri::readMarker();
        if (unfinished.present
            && QFileInfo::exists(unfinished.extractDir + QStringLiteral("/setup.exe"))) {
            recovery_ = new QWidget(this);
            auto* rcol = new QVBoxLayout(recovery_);
            rcol->setContentsMargins(0, 0, 0, 0);
            rcol->setSpacing(tokens::CompactGap);
            rcol->addWidget(vth::widgets::makeCallout(
                recovery_, QStringLiteral("triangle-alert"),
                tr("The last driver install did not finish"),
                { tr("Vethida kept the prepared driver %1, so finishing needs no download.")
                      .arg(unfinished.version) },
                QStringLiteral("warn"), &pal::warningText));
            auto* finishBtn = new vth::widgets::CommandButton(
                vth::widgets::CommandButton::Variant::Primary, tr("Finish the install"), recovery_);
            connect(finishBtn, &QPushButton::clicked, this, &GpuDriverView::finishInterrupted);
            auto* frow = new QHBoxLayout;
            frow->setContentsMargins(0, 0, 0, 0);
            frow->addStretch(1);
            frow->addWidget(finishBtn);
            rcol->addLayout(frow);
            col->insertWidget(0, recovery_);
        }
    }

    QWidget* GpuDriverView::buildCheckRow(const QString& text)
    {
        auto* row = new QWidget(this);
        auto* lay = new QHBoxLayout(row);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(tokens::CompactGap);

        constexpr int kGlyph = 16;
        auto* glyph = new vth::widgets::GlyphLabel(
            QStringLiteral("check"), &pal::scoreGoodText, kGlyph, row);
        lay->addWidget(glyph, 0, Qt::AlignTop);

        auto* label = new QLabel(text, row);
        label->setProperty("uiRole", QStringLiteral("bodyText"));
        label->setWordWrap(true);
        lay->addWidget(label, 1);
        return row;
    }

    bool GpuDriverView::confirmReinstall()
    {
        QFrame* card = vth::dialogs::makePromptCard(
            tr("Clean and reinstall driver"),
            tr("Vethida makes a restore point, downloads and checks the new driver, "
               "saves a copy of the current one, and only then removes it. While it "
               "runs:"),
            vth::dialogs::PromptTone::Warning);
        auto* col = static_cast<QVBoxLayout*>(card->layout());

        const QStringList notes = {
            tr("The screen will flash and go black more than once."),
            tr("Secondary displays switch off and come back at the end."),
            tr("Leave the mouse and keyboard alone, and keep Vethida open, until "
               "it finishes."),
            tr("Windows needs a restart afterwards to complete the install."),
        };
        for (const QString& n : notes) {
            auto* text = new QLabel(n, card);
            text->setObjectName(QStringLiteral("srPromptBody"));
            text->setWordWrap(true);
            col->addWidget(text);
        }

        col->addSpacing(tokens::CompactGap);
        auto* cancel = new vth::widgets::CommandButton(
            vth::widgets::CommandButton::Variant::Secondary, tr("Cancel"), card);
        auto* start = new vth::widgets::CommandButton(
            vth::widgets::CommandButton::Variant::Primary, tr("Reinstall"), card);
        col->addLayout(vth::dialogs::buttonRow({ cancel, start }));

        vth::widgets::ScrimDialog dlg(card, this);
        // Cancel holds focus, so Enter on a prompt nobody read does not remove
        // the machine's display driver.
        dlg.setInitialFocus(cancel);
        connect(cancel, &QPushButton::clicked, &dlg, &QDialog::reject);
        connect(start, &QPushButton::clicked, &dlg, &QDialog::accept);
        return dlg.exec() == QDialog::Accepted;
    }

    void GpuDriverView::onBegin()
    {
        if (actionStack_ && actionStack_->currentIndex() == 1)
            return;
        if (!vth::ui::tools::ensureTier(this, vth::models::ProductTier::Pro))
            return;
        if (confirmReinstall())
            startReinstall();
    }

    void GpuDriverView::startReinstall()
    {
        actionStack_->setCurrentIndex(1);
        if (resultLabel_)
            resultLabel_->hide();
        if (cancelBtn_)
            cancelBtn_->setEnabled(true);
        showProgress(tr("Starting..."), 0, 0, 0);

        cancel_ = std::make_shared<std::atomic<bool>>(false);
        auto cancel = cancel_;
        QPointer<GpuDriverView> guard(this);

        // Marshals one progress update from the worker to the UI thread, guarded
        // against the pane being destroyed mid-run (the user can leave the tab).
        auto post = [guard](const QString& label, int pct, qint64 rec, qint64 tot) {
            vth::Dispatcher::instance().dispatchUi([guard, label, pct, rec, tot]() {
                if (guard)
                    guard->showProgress(label, pct, rec, tot);
            });
        };
        // Disables Cancel from the worker at the point of no return.
        auto sealCancel = [guard]() {
            vth::Dispatcher::instance().dispatchUi([guard]() {
                if (guard && guard->cancelBtn_)
                    guard->cancelBtn_->setEnabled(false);
            });
        };

        runToken_ = vth::fetch<RunOutcome>(
            this,
            [post, sealCancel, cancel]() -> RunOutcome {
                auto aborted = [&] { return cancel->load(); };
                const RunOutcome canceled{ false, false, T("Canceled. Nothing was changed.") };

                post(T("Looking for your graphics card..."), 2, 0, 0);
                const nv::GpuInfo gpu = nv::detectGpu();
                if (!gpu.detected)
                    return { false, false, T("No NVIDIA graphics card was found on this PC.") };
                if (gpu.name.isEmpty())
                    return { false, false, T("This NVIDIA card has no driver yet, so Vethida cannot "
                                             "tell which model it is. Install any NVIDIA driver from "
                                             "Windows Update or nvidia.com once, then run this again.") };
                if (aborted()) return canceled;

                post(T("Finding the current driver..."), 5, 0, 0);
                const nv::ResolveResult res = nv::resolveLatestDriverUrl(gpu);
                if (!res.ok)
                    return { false, false, res.error + QLatin1Char(' ') + T("Nothing was changed.") };
                if (aborted()) return canceled;

                // A fresh workspace: a failed or canceled run leaves its files
                // here, and unpacking over them mixes two driver versions.
                ri::clearWorkspace();
                const QString workDir    = ri::workspace();
                const QString pkgPath    = workDir + QStringLiteral("/driver.exe");
                const QString extractDir = workDir + QStringLiteral("/extracted");

                const QString dlLabel = T("Downloading driver %1...").arg(res.version);
                post(dlLabel, 6, 0, 0);
                const nv::DownloadResult dl = nv::downloadFromUrl(
                    res.url, pkgPath,
                    [&](nv::DownloadProgress p) {
                        const int pct = (p.bytesTotal > 0)
                            ? 6 + static_cast<int>(48 * p.bytesReceived / p.bytesTotal)
                            : 6;
                        post(dlLabel, pct, p.bytesReceived, p.bytesTotal);
                    },
                    *cancel);
                if (dl == nv::DownloadResult::Canceled) { ri::clearWorkspace(); return canceled; }
                if (dl != nv::DownloadResult::Ok) {
                    ri::clearWorkspace();
                    return { false, false, dl == nv::DownloadResult::WriteError
                        ? T("The driver could not be saved to disk. Check that the system drive has "
                            "space. Nothing was changed.")
                        : T("The driver download did not complete. Check the internet connection "
                            "and try again. Nothing was changed.") };
                }

                post(T("Unpacking the package..."), 56, 0, 0);
                QString extractErr;
                if (nv::extractPackage(pkgPath, extractDir, &extractErr) != nv::ExtractResult::Ok) {
                    ri::clearWorkspace();
                    return { false, false, T("The driver package could not be unpacked (%1). Nothing "
                                             "was changed.").arg(extractErr) };
                }
                if (aborted()) { ri::clearWorkspace(); return canceled; }

                post(T("Checking the package..."), 58, 0, 0);
                if (const QString bad = ri::verifyPackage(extractDir); !bad.isEmpty()) {
                    ri::clearWorkspace();
                    return { false, false, bad };
                }

                // Kept when installed: its program, data and registrations stay,
                // and the package keeps the app component so setup updates it.
                const bool keepApp = nv::companionAppInstalled();

                post(T("Removing the components you don't need..."), 62, 0, 0);
                QStringList recIds = nv::recommendedComponentIds();
                if (keepApp)
                    recIds << QStringLiteral("NvApp");
                const QSet<QString> components =
                    nv::resolveDependencies(QSet<QString>(recIds.begin(), recIds.end()));
                QSet<QString> settings;
                for (const nv::InstallSetting& s : nv::installSettingsCatalog())
                    if (s.defaultEnabled)
                        settings.insert(s.id);
                const nv::PatchResult patch = nv::patchPackage(extractDir, components, settings);
                if (!patch.ok) {
                    ri::clearWorkspace();
                    return { false, false, T("The package could not be prepared (%1). Nothing was "
                                             "changed.").arg(patch.error) };
                }
                if (aborted()) { ri::clearWorkspace(); return canceled; }

                const ri::Preflight pre = ri::preflight(QFileInfo(pkgPath).size());
                if (!pre.blocker.isEmpty()) {
                    ri::clearWorkspace();
                    return { false, false, pre.blocker + QLatin1Char(' ') + T("Nothing was changed.") };
                }
                QFile::remove(pkgPath);

                // back up the current driver in case the new one doesn't bind
                post(T("Saving a copy of your current driver..."), 63, 0, 0);
                QString previousVersion;
                const QString previousDir = ri::backupBoundDriver(&previousVersion);
                if (aborted()) { ri::clearWorkspace(); return canceled; }

                // Point of no return. Recorded first, so a crash or power loss
                // from here on is offered for completion at the next start.
                sealCancel();
                ri::setRunning(true);
                ri::Marker m;
                m.present    = true;
                m.version    = res.version;
                m.extractDir = extractDir;
                m.stage      = QStringLiteral("removing");
                m.startedUtc = QDateTime::currentDateTimeUtc();
                m.previousDir     = previousDir;
                m.previousVersion = previousVersion;
                m.driverDelivery = ri::holdDriverDelivery();
                ri::writeMarker(m);

                nv::CleanOptions cleanOpts;
                cleanOpts.removeGfe = !keepApp;
                const nv::CleanResult clean = nv::runCleanRemoval(
                    cleanOpts,
                    [&](const QString& stage, int pct) {
                        post(stage, 66 + (22 * pct) / 100, 0, 0);
                    });

                RunOutcome out = installPhase(extractDir, res.version,
                    [&post](const QString& stage, int pct) { post(stage, pct, 0, 0); });
                if (out.ok && !clean.ok && !clean.error.isEmpty())
                    out.message += QLatin1Char(' ') + clean.error;
                if (out.ok && !pre.warnings.isEmpty())
                    out.message += QLatin1Char(' ') + pre.warnings.join(QLatin1Char(' '));
                return out;
            },
            [this](RunOutcome o) { finish(o.ok, o.message); if (o.restart) restartBtn_->show(); });
    }

    void GpuDriverView::finishInterrupted()
    {
        if (actionStack_->currentIndex() == 1 || ri::isRunning())
            return;
        const ri::Marker m = ri::readMarker();
        if (!m.present)
            return;
        ri::setRunning(true);
        if (recovery_)
            recovery_->hide();
        actionStack_->setCurrentIndex(1);
        cancelBtn_->setEnabled(false);
        resultLabel_->hide();
        showProgress(tr("Installing the driver..."), 88, 0, 0);

        QPointer<GpuDriverView> guard(this);
        auto post = [guard](const QString& label, int pct) {
            vth::Dispatcher::instance().dispatchUi([guard, label, pct]() {
                if (guard)
                    guard->showProgress(label, pct, 0, 0);
            });
        };
        const QString extractDir = m.extractDir;
        const QString version    = m.version;
        runToken_ = vth::fetch<RunOutcome>(
            this,
            [post, extractDir, version]() { return installPhase(extractDir, version, post); },
            [this](RunOutcome o) {
                finish(o.ok, o.message);
                if (o.restart)
                    restartBtn_->show();
                // Offered again only while there is still something to finish.
                if (!o.ok && recovery_ && ri::readMarker().present)
                    recovery_->show();
            });
    }

    void GpuDriverView::restartNow()
    {
        if (!vth::dialogs::confirm(this, tr("Restart your PC"),
                tr("Save your work in other apps first. Windows restarts in a few seconds."),
                tr("Restart now"), vth::dialogs::PromptTone::Warning))
            return;
        wchar_t dir[MAX_PATH] = {};
        const UINT n = GetSystemDirectoryW(dir, MAX_PATH);
        const QString shutdownExe = (n > 0 && n < MAX_PATH)
            ? QString::fromWCharArray(dir, static_cast<int>(n)) + QStringLiteral("\\shutdown.exe")
            : QStringLiteral("shutdown.exe");
        QProcess::startDetached(shutdownExe, { QStringLiteral("/r"), QStringLiteral("/t"),
                                               QStringLiteral("5") });
    }

    void GpuDriverView::showProgress(const QString& label, int percent,
                                     qint64 received, qint64 total)
    {
        stageLabel_->setText(label);
        const int pct = qBound(0, percent, 100);
        progress_->setValue(pct);
        percentLabel_->setText(QStringLiteral("%1%").arg(pct));
        if (total > 0)
            bytesLabel_->setText(QStringLiteral("%1 / %2")
                                     .arg(humanBytes(received), humanBytes(total)));
        else
            bytesLabel_->clear();
    }

    void GpuDriverView::finish(bool ok, const QString& message)
    {
        // The outcome stays on the pane and the button comes back, so a failed
        // or canceled run is retryable without restarting Vethida.
        actionStack_->setCurrentIndex(0);
        if (!resultLabel_)
            return;
        // message is only set when something was left behind
        const QString text = !message.isEmpty() ? message
                           : ok ? tr("Restart Windows to finish the driver install.")
                                : tr("The reinstall did not finish.");
        resultLabel_->setText(text);
        resultLabel_->show();
    }
}
