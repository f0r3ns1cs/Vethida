#include "pch.h"
#include "CleanupView.h"

#include "../Dialogs/Confirm.h"
#include "../Theme/DesignTokens.h"
#include "../Widgets/CommandButton.h"
#include "../Widgets/SettingsList.h"
#include "../Widgets/SwitchButton.h"

#include "../../vth_core/Runtime/Dispatcher.h"

#include <QAbstractButton>
#include <QHBoxLayout>
#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStringList>
#include <QVBoxLayout>

namespace vth::ui
{
    namespace tokens  = vth::theme::tokens;
    namespace cleanup = vth::win::cleanup;

    CleanupView::CleanupView(const QString& driveRoot, QWidget* parent)
        : QWidget(parent)
        , driveRoot_(driveRoot)
    {
        setObjectName(QStringLiteral("cleanupView"));
        buildUi();
        rescan();
    }

    CleanupView::~CleanupView()
    {
        scanToken_.cancel();
        runToken_.cancel();
    }

    void CleanupView::buildUi()
    {
        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(0, 0, 0, 0);
        root->setSpacing(0);

        list_ = new vth::widgets::SettingsList(this);
        root->addWidget(list_, 1);

        // One row per cleaner that applies to this volume, grouped by category.
        const QVector<const cleanup::Cleaner*> applicable =
            cleanup::cleanersFor(driveRoot_);

        for (const cleanup::Category cat : cleanup::orderedCategories()) {
            bool headed = false;
            for (const cleanup::Cleaner* c : applicable) {
                if (c->category != cat)
                    continue;
                if (!headed) {
                    list_->addHeading(cleanup::categoryLabel(cat));
                    headed = true;
                }

                auto* row = list_->addRow(
                    c->displayName,
                    QStringLiteral("%1 %2").arg(c->summary, c->impact));

                // Empty until the scan lands, because a zero would be a claim.
                auto* size = new QLabel(this);
                size->setObjectName(QStringLiteral("dashboardReadoutValue"));
                row->addTrailing(size);

                // Before the scan, because a cleaner's recommendation doesn't depend on
                // how much it holds.
                {
                    const QSignalBlocker block(row->toggle());
                    row->toggle()->setChecked(c->recommended);
                }
                connect(row->toggle(), &QAbstractButton::toggled, this,
                        [this]() { refreshSelection(); });

                rows_.push_back({ c->id, row, size, c->measurable });
            }
        }

        // Outside the list, so it stays put while the rows scroll.
        auto* footer = new QWidget(this);
        auto* foot = new QHBoxLayout(footer);
        foot->setContentsMargins(tokens::PageMarginH, tokens::CardMargin,
                                 tokens::PageMarginH, tokens::CardMargin);
        foot->setSpacing(tokens::CompactGap);

        total_ = new QLabel(footer);
        total_->setObjectName(QStringLiteral("settingsRowTitle"));
        foot->addWidget(total_, 0, Qt::AlignVCenter);
        foot->addStretch(1);

        cleanBtn_ = new vth::widgets::CommandButton(
            vth::widgets::CommandButton::Variant::Primary, tr("Clean now"), footer);
        cleanBtn_->setCursor(Qt::PointingHandCursor);
        cleanBtn_->setEnabled(false);
        connect(cleanBtn_, &QPushButton::clicked, this, &CleanupView::onCleanClicked);
        foot->addWidget(cleanBtn_, 0, Qt::AlignVCenter);

        root->addWidget(footer, 0);

        refreshSelection();
    }

    void CleanupView::rescan()
    {
        // Cancel first, because a run finishes by rescanning and the first scan's old
        // sizes would otherwise land on top of the second's.
        scanToken_.cancel();
        scanned_ = false;
        sizes_.clear();
        for (const Row& r : rows_) {
            if (r.size)
                r.size->clear();
        }
        refreshSelection();

        // On a worker, because every scan is a directory walk. The sizes land together so
        // the list doesn't relayout under the pointer.
        const QString root = driveRoot_;
        scanToken_ = vth::fetch<QHash<QString, cleanup::ScanResult>>(
            this,
            [root]() {
                QHash<QString, cleanup::ScanResult> out;
                for (const cleanup::Cleaner* c : cleanup::cleanersFor(root))
                    out.insert(c->id, cleanup::scan(*c, root));
                return out;
            },
            [this](QHash<QString, cleanup::ScanResult> sizes) {
                applyScan(sizes);
            });
    }

    void CleanupView::applyScan(const QHash<QString, cleanup::ScanResult>& sizes)
    {
        scanned_ = true;
        sizes_.clear();
        for (const Row& r : rows_) {
            const cleanup::ScanResult s = sizes.value(r.id);
            if (!r.size)
                continue;
            if (!s.known) {
                // The row's impact line already says this one can't be sized in advance.
                r.size->clear();
                continue;
            }
            sizes_.insert(r.id, s.bytes);
            r.size->setText(cleanup::formatBytes(s.bytes));
        }
        refreshSelection();
    }

    QStringList CleanupView::selectedIds() const
    {
        QStringList ids;
        for (const Row& r : rows_) {
            if (r.row && r.row->toggle()->isChecked())
                ids << r.id;
        }
        return ids;
    }

    void CleanupView::refreshSelection()
    {
        quint64 bytes = 0;
        int     picked = 0;
        bool    anyUnsized = false;
        for (const Row& r : rows_) {
            if (!r.row || !r.row->toggle()->isChecked())
                continue;
            ++picked;
            if (sizes_.contains(r.id))
                bytes += sizes_.value(r.id);
            else if (!r.measurable)
                anyUnsized = true;
        }

        if (cleanBtn_)
            cleanBtn_->setEnabled(picked > 0);

        if (!total_)
            return;
        if (picked == 0) {
            total_->setText(tr("Nothing selected"));
            return;
        }
        if (!scanned_) {
            total_->setText(tr("Measuring..."));
            return;
        }
        // "At least", when something in the selection can't be sized in advance.
        total_->setText(anyUnsized
            ? tr("At least %1").arg(cleanup::formatBytes(bytes))
            : cleanup::formatBytes(bytes));
    }

    void CleanupView::onCleanClicked()
    {
        if (!cleanBtn_ || cleanBtn_->isLoading())
            return;

        const QStringList ids = selectedIds();
        if (ids.isEmpty())
            return;

        QStringList named;
        for (const Row& r : rows_) {
            if (!ids.contains(r.id))
                continue;
            if (const cleanup::Cleaner* c = cleanup::find(r.id))
                named << QStringLiteral("- ") + c->displayName;
        }

        if (!vth::dialogs::confirm(
                this, tr("Clean up"),
                tr("Vethida will remove the following from drive %1:\n\n%2\n\n"
                   "This cannot be undone.")
                    .arg(driveRoot_.left(2), named.join(QLatin1Char('\n'))),
                tr("Clean now"))) {
            return;
        }

        cleanBtn_->setLoading(true);
        if (list_)
            list_->setEnabled(false);

        // Which cleaner is running, in the footer where the total was.
        QPointer<CleanupView> guard(this);
        auto progress = [guard](const QString& stage, int step, int steps) {
            vth::Dispatcher::instance().dispatchUi([guard, stage, step, steps]() {
                if (!guard || !guard->total_)
                    return;
                guard->total_->setText(tr("%1 of %2: %3")
                                           .arg(step + 1).arg(steps).arg(stage));
            });
        };

        const QString root = driveRoot_;
        runToken_ = vth::fetch<cleanup::RunResult>(
            this,
            [ids, root, progress]() { return cleanup::run(ids, root, progress); },
            [this](cleanup::RunResult result) {
                if (cleanBtn_)
                    cleanBtn_->setLoading(false);
                if (list_)
                    list_->setEnabled(true);

                QString summary = tr("Freed %1.")
                    .arg(cleanup::formatBytes(result.reclaimedBytes));
                if (result.filesDeleted > 0) {
                    summary += QStringLiteral("\n")
                             + tr("Items removed: %1.").arg(result.filesDeleted);
                }
                if (result.errors > 0) {
                    summary += QStringLiteral("\n\n")
                             + tr("Some files were in use and were skipped.");
                }
                vth::dialogs::inform(this, tr("Cleanup finished"), summary);

                Q_EMIT cleaned(result.reclaimedBytes);
                rescan();
            });
    }
}
