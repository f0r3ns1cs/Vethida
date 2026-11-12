#include "pch.h"
#include "SystemRestoreView.h"

#include "../../vth_core/Runtime/Threading.h"
#include "TweakLedgerView.h"
#include "../Dialogs/Confirm.h"
#include "../Theme/DesignTokens.h"
#include "../Utils/TablerIcon.h"
#include "../Widgets/CommandButton.h"
#include "../Widgets/EmptyState.h"
#include "../Widgets/PageScaffold.h"
#include "../Widgets/ToastManager.h"

#include "../../vth_core/Runtime/ServiceRegistry.h"

#include <QDateTime>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QShowEvent>
#include <QStringLiteral>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>

namespace vth::ui
{
    namespace
    {
        namespace sr     = vth::win::sysrestore;
        namespace pal    = vth::theme::palette;
        namespace tokens = vth::theme::tokens;

        // Floor for the panel that carries the empty state, a first-run machine's state.
        constexpr int kEmptyPanelHeight = 200;

        QString relativeFrom(const QDateTime& utc)
        {
            if (!utc.isValid()) return {};
            const qint64 secs = utc.toLocalTime().secsTo(QDateTime::currentDateTime());
            if (secs < 60)         return QObject::tr("just now");
            if (secs < 3600)       return QObject::tr("%1 min ago").arg(secs / 60);
            if (secs < 86400)      return QObject::tr("%1 h ago").arg(secs / 3600);
            if (secs < 86400 * 30) return QObject::tr("%1 d ago").arg(secs / 86400);
            if (secs < 86400 * 365)return QObject::tr("%1 mo ago").arg(secs / (86400 * 30));
            return QObject::tr("%1 y ago").arg(secs / (86400 * 365));
        }

        // "Vethida: <name> - <desc>" in the one Description field
        const QString kPointSeparator = QStringLiteral(" - ");

        struct PointInfo
        {
            QString name;
            QString desc;
        };

        PointInfo parsePoint(const sr::RestorePoint& p)
        {
            QString s = p.description;
            if (s.startsWith(QStringLiteral("Vethida: "), Qt::CaseInsensitive))
                s = s.mid(9).trimmed();

            PointInfo info;
            const int at = s.indexOf(kPointSeparator);
            if (at >= 0) {
                info.name = s.left(at).trimmed();
                info.desc = s.mid(at + kPointSeparator.size()).trimmed();
            } else {
                info.name = s;
            }
            if (info.name.compare(QStringLiteral("Vethida restore point"), Qt::CaseInsensitive) == 0)
                info.name.clear();
            if (info.name.isEmpty())
                info.name = QObject::tr("Restore point");
            return info;
        }
    }

    SystemRestoreView::SystemRestoreView(QWidget* parent) : QWidget(parent)
    {
        VTH_ASSERT_UI_THREAD();
        setObjectName(QStringLiteral("SystemRestoreView"));
        buildUi();
    }

    void SystemRestoreView::showEvent(QShowEvent* event)
    {
        QWidget::showEvent(event);
        // Enumerating restore points spawns a PowerShell process; defer it until
        // the tab is first shown rather than paying it at app startup.
        if (!firstShown_) {
            firstShown_ = true;
            onRefresh();
        }
    }

    SystemRestoreView::~SystemRestoreView()
    {
        refreshToken_.cancel();
    }

    void SystemRestoreView::buildUi()
    {
        auto* outer = new QVBoxLayout(this);
        outer->setContentsMargins(0, 0, 0, 0);
        outer->setSpacing(0);
        // Vethida's own ledger goes first, because it undoes what this application did.
        auto* scaffold = new vth::widgets::PageScaffold(
            tr("Restore"),
            tr("Put back anything Vethida changed, or return Windows to a restore "
               "point."),
            this);
        outer->addWidget(scaffold);
        scaffold->addBodyWidget(new TweakLedgerView(this));
        scaffold->addBodyWidget(buildPointsCard());
    }

    QWidget* SystemRestoreView::buildPointsCard()
    {
        auto* card = new QFrame(this);
        card->setObjectName(QStringLiteral("srPointsCard"));
        card->setAttribute(Qt::WA_StyledBackground, true);

        auto* col = new QVBoxLayout(card);
        col->setContentsMargins(0, 0, 0, 0);
        col->setSpacing(0);

        auto* header = new QHBoxLayout;
        header->setContentsMargins(tokens::CardInsetH, tokens::CardInsetV,
                                   tokens::CardInsetV, tokens::CardInsetV);
        header->setSpacing(tokens::CompactGap);

        auto* sectionTitle = new QLabel(tr("Restore points"), card);
        sectionTitle->setProperty("uiRole", QStringLiteral("sectionTitle"));
        header->addWidget(sectionTitle, 0, Qt::AlignVCenter);

        header->addStretch(1);

        deleteAllBtn_ = new QToolButton(card);
        deleteAllBtn_->setAutoRaise(true);
        deleteAllBtn_->setCursor(Qt::PointingHandCursor);
        deleteAllBtn_->setAccessibleName(tr("Delete all restore points"));
        deleteAllBtn_->setToolTip(tr("Delete all restore points"));
        vth::icons::applyIconButton(deleteAllBtn_, QStringLiteral("trash-2"), &pal::textSecondary, 16);
        deleteAllBtn_->setEnabled(false);
        connect(deleteAllBtn_, &QToolButton::clicked, this, &SystemRestoreView::promptDeleteAll);
        header->addWidget(deleteAllBtn_, 0, Qt::AlignVCenter);

        refreshBtn_ = new QToolButton(card);
        refreshBtn_->setAutoRaise(true);
        refreshBtn_->setCursor(Qt::PointingHandCursor);
        refreshBtn_->setAccessibleName(tr("Refresh the list"));
        refreshBtn_->setToolTip(tr("Refresh the list"));
        vth::icons::applyIconButton(refreshBtn_, QStringLiteral("refresh-cw"), &pal::textSecondary, 16);
        connect(refreshBtn_, &QToolButton::clicked, this, &SystemRestoreView::onRefresh);
        header->addWidget(refreshBtn_, 0, Qt::AlignVCenter);
        col->addLayout(header);

        auto* topRule = new QFrame(card);
        topRule->setObjectName(QStringLiteral("srRowDivider"));
        topRule->setAttribute(Qt::WA_StyledBackground, true);
        col->addWidget(topRule);

        auto* lane = new QHBoxLayout;
        lane->setContentsMargins(tokens::RowInsetH, tokens::RowInsetV,
                                 tokens::RowInsetH, tokens::RowInsetV);
        lane->setSpacing(tokens::CompactGap);
        descEdit_ = new QLineEdit(card);
        descEdit_->setPlaceholderText(tr("Description (optional)"));
        descEdit_->setAccessibleName(tr("Restore point description (optional)"));
        descEdit_->setClearButtonEnabled(true);
        connect(descEdit_, &QLineEdit::returnPressed, this, &SystemRestoreView::onCreate);
        lane->addWidget(descEdit_, 1);

        createBtn_ = new vth::widgets::CommandButton(vth::widgets::CommandButton::Variant::Primary,
                                       tr("Create restore point"), card);
        // pre-rendered at the right dpr (styled buttons draw icons at 1.0)
        // glyph-static: accentText is white in both modes
        createBtn_->setIcon(QIcon(vth::icons::glyphPixmap(
            QStringLiteral("plus"), pal::accentText(), tokens::GlyphMd, createBtn_)));
        createBtn_->setIconSize(QSize(tokens::GlyphMd, tokens::GlyphMd));
        connect(createBtn_, &QPushButton::clicked, this, &SystemRestoreView::onCreate);
        lane->addWidget(createBtn_, 0);
        col->addLayout(lane);

        auto* laneRule = new QFrame(card);
        laneRule->setObjectName(QStringLiteral("srRowDivider"));
        laneRule->setAttribute(Qt::WA_StyledBackground, true);
        col->addWidget(laneRule);

        pointsHost_ = new QWidget(card);
        pointsLayout_ = new QVBoxLayout(pointsHost_);
        pointsLayout_->setContentsMargins(tokens::CompactGap, tokens::TightGap,
                                          tokens::CompactGap, tokens::CompactGap);
        pointsLayout_->setSpacing(0);
        col->addWidget(pointsHost_);

        emptyWrap_ = new QWidget(card);
        emptyWrap_->setMinimumHeight(kEmptyPanelHeight);
        auto* emptyLay = new QVBoxLayout(emptyWrap_);
        emptyLay->setContentsMargins(tokens::CardMargin, tokens::CardMargin,
                                     tokens::CardMargin, tokens::CardMargin);
        emptyLay->setSpacing(0);
        emptyState_ = new EmptyState(emptyWrap_);
        emptyState_->setTitle(tr("No restore points yet"));
        emptyLay->addWidget(emptyState_, 1);
        emptyWrap_->setVisible(false);
        col->addWidget(emptyWrap_);

        return card;
    }

    void SystemRestoreView::onRefresh()
    {
        VTH_ASSERT_UI_THREAD();
        refreshToken_.cancel();
        refreshBtn_->setEnabled(false);
        showLoadingPoints();

        refreshToken_ = vth::fetch<sr::EnumerateResult>(
            this,
            []() { return sr::enumerateChecked(); },
            [this](sr::EnumerateResult result) {
                refreshBtn_->setEnabled(true);
                if (!result.ok) {
                    // The list couldn't be read, which says nothing about what's on disk.
                    clearPoints();
                    pointsHost_->setVisible(false);
                    deleteAllBtn_->setEnabled(false);
                    emptyState_->setBusy(false);
                    emptyState_->setTitle(tr("Could not read the restore points"));
                    emptyState_->setSubtitle(result.error + QLatin1Char(' ')
                        + tr("Use the refresh button to try again."));
                    emptyWrap_->setVisible(true);
                    return;
                }
                emptyState_->setSubtitle(QString());
                applyPoints(std::move(result.points));
            });
    }

    void SystemRestoreView::showLoadingPoints()
    {
        clearPoints();
        pointsHost_->setVisible(false);
        // Loader-only while fetching (EmptyState hides text when busy); the
        // "no points / N points" message is set once the load resolves.
        emptyState_->setBusy(true);
        emptyWrap_->setVisible(true);
        deleteAllBtn_->setEnabled(false);
    }

    void SystemRestoreView::applyPoints(QVector<sr::RestorePoint> points)
    {
        clearPoints();
        // Delete-all acts on the list, so an empty list leaves it dead.
        deleteAllBtn_->setEnabled(!points.isEmpty());

        if (points.isEmpty()) {
            pointsHost_->setVisible(false);
            emptyState_->setBusy(false);
            emptyState_->setTitle(tr("No restore points yet"));
            emptyWrap_->setVisible(true);
            return;
        }

        emptyWrap_->setVisible(false);
        pointsHost_->setVisible(true);

        std::sort(points.begin(), points.end(),
                  [](const sr::RestorePoint& a, const sr::RestorePoint& b) {
                      return a.createdUtc > b.createdUtc;
                  });

        for (int i = 0; i < points.size(); ++i)
            addPointRow(points[i], i == points.size() - 1);
    }

    void SystemRestoreView::addPointRow(const sr::RestorePoint& p, bool isLast)
    {
        auto* row = new QFrame(pointsHost_);
        row->setObjectName(QStringLiteral("settingsRow"));
        row->setProperty("interactive", true);
        row->setAttribute(Qt::WA_StyledBackground, true);

        auto* lay = new QHBoxLayout(row);
        lay->setContentsMargins(tokens::CompactGap, tokens::CompactGap,
                                tokens::CompactGap, tokens::CompactGap);
        lay->setSpacing(tokens::CardMargin);

        const PointInfo info = parsePoint(p);

        auto* text = new QVBoxLayout;
        text->setSpacing(tokens::Space2);
        text->setContentsMargins(0, 0, 0, 0);

        // The name, and beside it the mark saying who made the point.
        auto* nameRow = new QHBoxLayout;
        nameRow->setContentsMargins(0, 0, 0, 0);
        nameRow->setSpacing(tokens::CompactGap);
        auto* name = new QLabel(info.name, row);
        name->setObjectName(QStringLiteral("settingsRowTitle"));
        nameRow->addWidget(name, 0, Qt::AlignVCenter);
        if (p.isVethida) {
            auto* tag = new QLabel(tr("Vethida"), row);
            tag->setProperty("uiRole", QStringLiteral("chip"));
            tag->setAttribute(Qt::WA_StyledBackground, true);
            nameRow->addWidget(tag, 0, Qt::AlignVCenter);
        }
        nameRow->addStretch(1);
        text->addLayout(nameRow);

        if (!info.desc.isEmpty()) {
            auto* desc = new QLabel(info.desc, row);
            desc->setObjectName(QStringLiteral("settingsRowDesc"));
            desc->setWordWrap(true);
            text->addWidget(desc);
        }

        auto* meta = new QLabel(row);
        meta->setObjectName(QStringLiteral("settingsRowDesc"));
        meta->setText(QStringLiteral("%1  -  %2")
                          .arg(QLocale().toString(p.createdUtc.toLocalTime(), QLocale::ShortFormat),
                               relativeFrom(p.createdUtc)));
        text->addWidget(meta);
        lay->addLayout(text, 1);

        auto* restoreBtn = new vth::widgets::CommandButton(vth::widgets::CommandButton::Variant::Secondary,
                                             tr("Restore"), row);
        restoreBtn->setToolTip(tr("Restore Windows to this point"));
        connect(restoreBtn, &QPushButton::clicked, this,
                [this, p, restoreBtn]() { promptRestore(p, restoreBtn); });
        lay->addWidget(restoreBtn, 0, Qt::AlignVCenter);

        auto* deleteBtn = new QToolButton(row);
        deleteBtn->setAutoRaise(true);
        deleteBtn->setCursor(Qt::PointingHandCursor);
        deleteBtn->setAccessibleName(tr("Delete restore point"));
        deleteBtn->setToolTip(tr("Delete this restore point"));
        vth::icons::applyIconButton(deleteBtn, QStringLiteral("trash-2"), &pal::textSecondary, 16);
        connect(deleteBtn, &QToolButton::clicked, this,
                [this, p]() { promptDelete(p); });
        lay->addWidget(deleteBtn, 0, Qt::AlignVCenter);

        pointsLayout_->addWidget(row);

        if (!isLast) {
            auto* div = new QFrame(pointsHost_);
            div->setObjectName(QStringLiteral("srRowDivider"));
            div->setAttribute(Qt::WA_StyledBackground, true);
            pointsLayout_->addWidget(div);
        }
    }

    void SystemRestoreView::clearPoints()
    {
        while (QLayoutItem* item = pointsLayout_->takeAt(0)) {
            if (QWidget* w = item->widget())
                w->deleteLater();
            delete item;
        }
    }

    void SystemRestoreView::onCreate()
    {
        VTH_ASSERT_UI_THREAD();
        if (createBtn_->isLoading())
            return;

        const QString desc = descEdit_->text().trimmed();
        const QString description = desc.isEmpty()
            ? QStringLiteral("Vethida restore point")
            : QStringLiteral("Vethida: ") + desc;

        startCreate(description, false, false);
    }

    void SystemRestoreView::startCreate(const QString& description, bool bypassThrottle, bool afterEnable)
    {
        createBtn_->setLoading(true);

        vth::fetch<sr::CreateOutcome>(
            this,
            [description, bypassThrottle]() {
                return sr::create(description, bypassThrottle);
            },
            [this, description, bypassThrottle, afterEnable](sr::CreateOutcome outcome) {
                createBtn_->setLoading(false);

                const QString why = outcome.error.isEmpty()
                    ? sr::createResultLabel(outcome.result) : outcome.error;
                switch (outcome.result) {
                case sr::CreateResult::Ok: {
                    descEdit_->clear();
                    if (auto* toasts = vth::ServiceRegistry::instance().get<vth::widgets::ToastManager>())
                        toasts->success(tr("Restore point created"),
                                        tr("Windows can return to this moment from this page."));
                    onRefresh();
                    return;
                }
                case sr::CreateResult::ProtectionDisabled:
                case sr::CreateResult::ServicesDisabled:
                    if (afterEnable) {
                        vth::dialogs::inform(
                            this, tr("Restart required"),
                            tr("Vethida turned System Protection and its services back on, but "
                               "Windows still reports them as off. Restart this PC, then "
                               "create the restore point again."));
                    } else {
                        promptEnableThenCreate(description, why);
                    }
                    return;
                case sr::CreateResult::Throttled:
                    if (!bypassThrottle) {
                        if (vth::dialogs::confirm(this,
                                tr("Recent restore point"),
                                why + QLatin1Char(' ')
                                    + tr("Vethida can make another one now anyway."),
                                tr("Create anyway")))
                            startCreate(description, true, afterEnable);
                        return;
                    }
                    [[fallthrough]];
                default:
                    vth::dialogs::warn(this, tr("Restore point not created"), why);
                    return;
                }
            });
    }

    void SystemRestoreView::promptEnableThenCreate(const QString& description,
                                                   const QString& why)
    {
        VTH_ASSERT_UI_THREAD();

        if (!vth::dialogs::confirm(
                this,
                tr("Turn System Restore back on"),
                why + QStringLiteral("\n\n")
                    + tr("Vethida can turn System Protection and the Volume Shadow Copy "
                         "services back on for the system drive, then make the restore point."),
                tr("Turn it on"), vth::dialogs::PromptTone::Question,
                tr("Not now")))
            return;

        createBtn_->setLoading(true);
        vth::fetch<QString>(
            this,
            []() { return sr::enableSystemProtection(); },
            [this, description](QString error) {
                if (error.isEmpty()) {
                    startCreate(description, false, true);
                    return;
                }
                createBtn_->setLoading(false);
                vth::dialogs::warn(
                    this, tr("Protection not turned on"),
                    tr("Vethida could not turn on System Protection.\n\n%1").arg(error));
            });
    }

    void SystemRestoreView::promptRestore(const sr::RestorePoint& p, vth::widgets::CommandButton* btn)
    {
        VTH_ASSERT_UI_THREAD();
        if (btn->isLoading())
            return;

        const QString when = p.createdUtc.isValid()
            ? QLocale().toString(p.createdUtc.toLocalTime(), QLocale::ShortFormat)
            : QString();

        const QString pointTitle = parsePoint(p).name;
        const QString target = when.isEmpty()
            ? tr("\"%1\"").arg(pointTitle)
            : tr("\"%1\" from %2").arg(pointTitle, when);

        if (!vth::dialogs::confirm(
                this,
                tr("Restore this PC"),
                tr("Windows returns to the restore point %1. Documents and personal "
                   "files are left alone; apps and drivers installed since then are "
                   "removed.\n\nThis PC restarts to finish, and the restore cannot be "
                   "interrupted. Save your work first.")
                    .arg(target),
                tr("Restore and restart"), vth::dialogs::PromptTone::Warning))
            return;

        btn->setLoading(true);
        const int sequence = p.sequenceNumber;
        // vth::fetch guards the context object only, and a refresh rebuilds the rows.
        QPointer<vth::widgets::CommandButton> btnGuard(btn);
        vth::fetch<sr::RestoreOutcome>(
            this,
            [sequence]() { return sr::restoreToPoint(sequence); },
            [this, btnGuard](sr::RestoreOutcome outcome) {
                if (outcome.result == sr::RestoreResult::Restarting) {
                    vth::dialogs::inform(
                        this, tr("Restoring"),
                        tr("The restore is set up. Windows restarts in a few seconds to "
                           "finish it."));
                    return;
                }

                if (btnGuard)
                    btnGuard->setLoading(false);
                QString detail;
                switch (outcome.result) {
                case sr::RestoreResult::AccessDenied:
                    detail = tr("Windows refused to start the restore. Security "
                                "software or a Group Policy that turns off System "
                                "Restore is usually the cause.");
                    break;
                case sr::RestoreResult::InProgress:
                    detail = tr("A system restore is already in progress. Let it "
                                "finish before starting another.");
                    break;
                case sr::RestoreResult::NotFound:
                    detail = tr("This restore point could no longer be found. "
                                "Refresh the list and try again.");
                    break;
                default:
                    detail = outcome.error.isEmpty()
                        ? sr::restoreResultLabel(outcome.result)
                        : outcome.error;
                    break;
                }
                vth::dialogs::warn(this, tr("Restore failed"), detail);
            });
    }

    void SystemRestoreView::promptDelete(const sr::RestorePoint& p)
    {
        VTH_ASSERT_UI_THREAD();

        if (!vth::dialogs::confirm(this,
                tr("Delete restore point"),
                tr("\"%1\" is removed and Windows can no longer return to it. "
                   "Other restore points are kept.").arg(parsePoint(p).name),
                tr("Delete"), vth::dialogs::PromptTone::Warning))
            return;

        const int sequence = p.sequenceNumber;
        vth::fetch<sr::DeleteOutcome>(
            this,
            [sequence]() { return sr::deleteRestorePoint(sequence); },
            [this](sr::DeleteOutcome outcome) {
                if (outcome.result != sr::DeleteResult::Ok) {
                    vth::dialogs::warn(
                        this, tr("Restore point not deleted"),
                        outcome.error.isEmpty()
                            ? sr::deleteResultLabel(outcome.result)
                            : outcome.error);
                }
                onRefresh();
            });
    }

    void SystemRestoreView::promptDeleteAll()
    {
        VTH_ASSERT_UI_THREAD();

        if (!vth::dialogs::confirm(
                this,
                tr("Delete all restore points"),
                tr("Every restore point on the system drive is removed, along with any "
                   "previous versions of files kept with them. Windows cannot return to "
                   "an earlier state until you create a new one. This cannot be undone."),
                tr("Delete all"), vth::dialogs::PromptTone::Warning))
            return;

        deleteAllBtn_->setEnabled(false);
        vth::fetch<sr::DeleteOutcome>(
            this,
            []() { return sr::deleteAllRestorePoints(); },
            [this](sr::DeleteOutcome outcome) {
                if (outcome.result != sr::DeleteResult::Ok) {
                    vth::dialogs::warn(
                        this, tr("Restore points not deleted"),
                        outcome.error.isEmpty()
                            ? sr::deleteResultLabel(outcome.result)
                            : outcome.error);
                }
                onRefresh();
            });
    }
}
