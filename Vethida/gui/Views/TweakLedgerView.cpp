#include "pch.h"
#include "TweakLedgerView.h"

#include "../Settings/Preferences.h"

#include "../../vth_core/Runtime/ServiceRegistry.h"
#include "../../vth_core/Runtime/Threading.h"
#include "../../vth_core/Tweaks/TweakEngine.h"
#include "../Dialogs/Confirm.h"
#include "../Models/TweakCategories.h"
#include "../Theme/DesignTokens.h"
#include "../Utils/TablerIcon.h"
#include "../Widgets/CommandButton.h"
#include "../Widgets/EmptyState.h"
#include "../Widgets/ToastManager.h"

#include <QDate>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QPushButton>
#include <QShowEvent>
#include <QStringLiteral>
#include <QToolButton>
#include <QVBoxLayout>

namespace vth::ui
{
    namespace
    {
        namespace pal    = vth::theme::palette;
        namespace tokens = vth::theme::tokens;

        // Sessions shown per page. Enough to cover a normal week of use without
        // building hundreds of rows on open.
        constexpr int kPageSize = 15;
        // Search results are capped; a query that matches more than this is
        // too broad to act on row by row anyway.
        constexpr int kSearchCap = 60;
        constexpr int kEmptyPanelHeight = 160;

        vth::TweakStateTracker* tracker()
        {
            return vth::ServiceRegistry::instance().get<vth::TweakStateTracker>();
        }

        // A readable name for a change whose tweak the catalog no longer
        // carries: the leaf of its id, spaced and sentence-cased.
        QString humanizeDomain(const QString& domain)
        {
            QString leaf = domain.section(QLatin1Char('.'), -1);
            leaf.replace(QLatin1Char('-'), QLatin1Char(' '));
            leaf.replace(QLatin1Char('_'), QLatin1Char(' '));
            leaf = leaf.trimmed();
            if (leaf.isEmpty())
                return domain;
            leaf[0] = leaf[0].toUpper();
            return leaf;
        }

        // The name a change wears: the tweak's own name, or the feature that made
        // it. Never what it wrote.
        QString domainDisplayName(const QString& domain)
        {
            if (const vth::tweaks::TweakSpec* spec = vth::tweaks::find(domain))
                return spec->name;
            if (domain == QLatin1String("genesis"))
                return QObject::tr("Genesis");
            if (domain.startsWith(QLatin1String("nvidia")))
                return QObject::tr("NVIDIA driver settings");
            return humanizeDomain(domain);
        }

        QString changesText(int n)
        {
            return n == 1 ? QObject::tr("1 change") : QObject::tr("%1 changes").arg(n);
        }

        QString sessionsText(int n)
        {
            return n == 1 ? QObject::tr("1 session") : QObject::tr("%1 sessions").arg(n);
        }

        QString categoryName(const QString& key)
        {
            for (const auto& c : vth::ui::models::tweakCategories())
                if (c.key == key)
                    return c.label;
            if (key == QLatin1String("privacy+security")) return QObject::tr("Privacy & security");
            if (key == QLatin1String("networking"))       return QObject::tr("Network");
            if (key == QLatin1String("gpu"))              return QObject::tr("GPU");
            if (key == QLatin1String("debloating"))       return QObject::tr("Windows features");
            if (key == QLatin1String("browseroffice"))    return QObject::tr("Browsers and apps");
            return humanizeDomain(key);
        }

        // batch id formats: recommended:<cat>:<id>, all:<cat>:<id>, <tweak>:<id>, genesis:<id>, nvidia.*:<id>
        QString sessionLabel(const vth::TweakBatchGroup& g, const QStringList& domains)
        {
            const QStringList parts = g.batchId.split(QLatin1Char(':'));
            if (g.batchId.isEmpty())
                return QObject::tr("Earlier changes");
            if (parts.size() >= 3 && parts.at(0) == QLatin1String("recommended"))
                return QObject::tr("Recommended tweaks in %1").arg(categoryName(parts.at(1)));
            if (parts.size() >= 3 && parts.at(0) == QLatin1String("all"))
                return QObject::tr("All tweaks in %1").arg(categoryName(parts.at(1)));
            if (parts.size() >= 2 && parts.at(0) == QLatin1String("genesis"))
                return QObject::tr("Genesis session");
            if (parts.size() >= 2 && parts.at(0).startsWith(QLatin1String("nvidia")))
                return QObject::tr("NVIDIA driver settings");
            if (domains.size() == 1)
                return domainDisplayName(domains.first());
            return changesText(domains.size());
        }

        QString whenText(const QDateTime& utc)
        {
            if (!utc.isValid())
                return QObject::tr("Date not recorded");
            const QDateTime local = utc.toLocalTime();
            const QString time = QLocale().toString(local.time(), QLocale::ShortFormat);
            const QDate today = QDate::currentDate();
            if (local.date() == today)
                return QObject::tr("Today, %1").arg(time);
            if (local.date() == today.addDays(-1))
                return QObject::tr("Yesterday, %1").arg(time);
            // "Aug 29, 12:42", in the same shape as the two above; the year
            // only when it is not this one.
            const QString day = QLocale().toString(
                local.date(), local.date().year() == today.year()
                                  ? QStringLiteral("MMM d")
                                  : QStringLiteral("MMM d, yyyy"));
            return QObject::tr("%1, %2").arg(day, time);
        }

        QStringList domainsOf(const vth::TweakBatchGroup& group)
        {
            QStringList out;
            for (const vth::TweakLedgerEntry& e : group.entries)
                if (!out.contains(e.snapshot.domain))
                    out.append(e.snapshot.domain);
            return out;
        }

        QFrame* divider(QWidget* parent)
        {
            auto* d = new QFrame(parent);
            d->setObjectName(QStringLiteral("srRowDivider"));
            d->setAttribute(Qt::WA_StyledBackground, true);
            return d;
        }
    }

    TweakLedgerView::TweakLedgerView(QWidget* parent) : QWidget(parent)
    {
        VTH_ASSERT_UI_THREAD();
        setObjectName(QStringLiteral("TweakLedgerView"));
        buildUi();
        // A change applied anywhere else while this page is open shows up when
        // the person comes back to it; an undo made here re-reads at once.
        if (auto* t = tracker())
            connect(t, &vth::TweakStateTracker::reverted, this,
                    [this]() { if (!busy_) refresh(); }, Qt::QueuedConnection);
    }

    TweakLedgerView::~TweakLedgerView()
    {
        undoToken_.cancel();
        readToken_.cancel();
    }

    void TweakLedgerView::showEvent(QShowEvent* event)
    {
        QWidget::showEvent(event);
        // Re-read on every visit: changes are recorded from other pages.
        if (!busy_)
            refresh();
    }

    void TweakLedgerView::buildUi()
    {
        auto* outer = new QVBoxLayout(this);
        outer->setContentsMargins(0, 0, 0, 0);
        outer->setSpacing(0);

        auto* card = new QFrame(this);
        card->setObjectName(QStringLiteral("srPointsCard"));
        card->setAttribute(Qt::WA_StyledBackground, true);
        auto* col = new QVBoxLayout(card);
        col->setContentsMargins(0, 0, 0, 0);
        col->setSpacing(0);

        auto* header = new QHBoxLayout;
        header->setContentsMargins(tokens::CardInsetH, tokens::CardInsetV,
                                   tokens::CardInsetH, tokens::CardInsetV);
        header->setSpacing(tokens::CardMargin);

        auto* titles = new QVBoxLayout;
        titles->setContentsMargins(0, 0, 0, 0);
        titles->setSpacing(tokens::Space2);
        auto* title = new QLabel(tr("Changes Vethida made"), card);
        title->setProperty("uiRole", QStringLiteral("sectionTitle"));
        titles->addWidget(title);
        summary_ = new QLabel(card);
        summary_->setProperty("uiRole", QStringLiteral("metaText"));
        titles->addWidget(summary_);
        header->addLayout(titles, 1);

        search_ = new QLineEdit(card);
        search_->setPlaceholderText(tr("Find a change"));
        search_->setAccessibleName(tr("Find a change"));
        search_->setClearButtonEnabled(true);
        search_->setFixedWidth(tokens::FilterControlWidth);
        connect(search_, &QLineEdit::textChanged, this, [this](const QString&) { render(); });
        header->addWidget(search_, 0, Qt::AlignVCenter);

        undoAllBtn_ = new vth::widgets::CommandButton(
            vth::widgets::CommandButton::Variant::Secondary, tr("Undo everything"), card);
        undoAllBtn_->setEnabled(false);
        connect(undoAllBtn_, &QPushButton::clicked, this, [this]() {
            requestUndo(Scope::All, QString(), tr("everything Vethida changed"));
        });
        header->addWidget(undoAllBtn_, 0, Qt::AlignVCenter);
        col->addLayout(header);
        col->addWidget(divider(card));

        listHost_ = new QWidget(card);
        listLay_ = new QVBoxLayout(listHost_);
        listLay_->setContentsMargins(tokens::Space8, tokens::Space4, tokens::Space8, tokens::Space8);
        listLay_->setSpacing(0);
        col->addWidget(listHost_);

        moreBtn_ = new vth::widgets::CommandButton(
            vth::widgets::CommandButton::Variant::Secondary, tr("Show more"), card);
        moreBtn_->setVisible(false);
        connect(moreBtn_, &QPushButton::clicked, this, [this]() {
            shown_ += kPageSize;
            render();
        });
        auto* moreRow = new QHBoxLayout;
        moreRow->setContentsMargins(tokens::CardInsetH, 0, tokens::CardInsetH, tokens::CardInsetV);
        moreRow->addWidget(moreBtn_, 0, Qt::AlignLeft);
        moreRow->addStretch(1);
        col->addLayout(moreRow);

        emptyWrap_ = new QWidget(card);
        emptyWrap_->setMinimumHeight(kEmptyPanelHeight);
        auto* emptyLay = new QVBoxLayout(emptyWrap_);
        emptyLay->setContentsMargins(tokens::CardMargin, tokens::CardMargin,
                                     tokens::CardMargin, tokens::CardMargin);
        empty_ = new EmptyState(emptyWrap_);
        emptyLay->addWidget(empty_, 1);
        emptyWrap_->setVisible(false);
        col->addWidget(emptyWrap_);

        outer->addWidget(card);
    }

    void TweakLedgerView::refresh()
    {
        VTH_ASSERT_UI_THREAD();
        vth::TweakStateTracker* t = tracker();
        if (!t) {
            applyGroups({});
            return;
        }
        readToken_.cancel();
        readToken_ = vth::fetch<QVector<vth::TweakBatchGroup>>(
            this,
            [t]() { return t->batches(); },
            [this](QVector<vth::TweakBatchGroup> groups) { applyGroups(std::move(groups)); });
    }

    void TweakLedgerView::applyGroups(QVector<vth::TweakBatchGroup> groups)
    {
        groups_ = std::move(groups);
        if (shown_ < kPageSize)
            shown_ = kPageSize;

        int changes = 0;
        QSet<QString> distinct;
        for (const auto& g : groups_)
            for (const auto& e : g.entries)
                distinct.insert(g.batchId + QLatin1Char('|') + e.snapshot.domain);
        changes = distinct.size();

        summary_->setText(groups_.isEmpty()
            ? tr("Nothing yet. Every change Vethida makes is listed here.")
            : changesText(changes) + QStringLiteral("  ") + QChar(0x00B7)
                  + QStringLiteral("  ") + sessionsText(groups_.size()));
        undoAllBtn_->setEnabled(!groups_.isEmpty() && !busy_);
        search_->setEnabled(!groups_.isEmpty());
        render();
    }

    void TweakLedgerView::render()
    {
        // Built fresh each time; the widgets are few because sessions are paged
        // and collapsed. Hidden before deletion so nothing old paints for a frame.
        while (QLayoutItem* item = listLay_->takeAt(0)) {
            if (QWidget* w = item->widget()) {
                w->hide();
                w->deleteLater();
            }
            delete item;
        }

        const QString query = search_->text().trimmed();
        if (groups_.isEmpty()) {
            listHost_->setVisible(false);
            moreBtn_->setVisible(false);
            empty_->setBusy(false);
            empty_->setTitle(tr("Vethida has not changed anything yet"));
            emptyWrap_->setVisible(true);
            return;
        }
        emptyWrap_->setVisible(false);
        listHost_->setVisible(true);

        if (query.isEmpty())
            renderSessions();
        else
            renderSearch(query);
    }

    void TweakLedgerView::renderSessions()
    {
        const int n = qMin(shown_, groups_.size());
        for (int i = 0; i < n; ++i) {
            if (i > 0)
                listLay_->addWidget(divider(listHost_));
            listLay_->addWidget(buildSession(groups_.at(i)));
        }
        const int rest = groups_.size() - n;
        moreBtn_->setText(tr("Show %1 more").arg(qMin(rest, kPageSize)));
        moreBtn_->setVisible(rest > 0);
    }

    void TweakLedgerView::renderSearch(const QString& query)
    {
        moreBtn_->setVisible(false);
        QStringList hits;
        QSet<QString> seen;
        for (const auto& g : groups_) {
            for (const auto& e : g.entries) {
                const QString& d = e.snapshot.domain;
                if (seen.contains(d))
                    continue;
                seen.insert(d);
                if (domainDisplayName(d).contains(query, Qt::CaseInsensitive))
                    hits << d;
            }
        }
        if (hits.isEmpty()) {
            auto* none = new QLabel(tr("No change matches \"%1\".").arg(query), listHost_);
            none->setObjectName(QStringLiteral("settingsRowDesc"));
            none->setContentsMargins(tokens::Space8, tokens::Space12, tokens::Space8, tokens::Space12);
            listLay_->addWidget(none);
            return;
        }
        const int n = qMin(hits.size(), kSearchCap);
        for (int i = 0; i < n; ++i)
            listLay_->addWidget(buildChangeRow(hits.at(i), listHost_));
        if (hits.size() > n) {
            const int extra = hits.size() - n;
            auto* more = new QLabel(extra == 1
                ? tr("1 more match. Narrow the search to see it.")
                : tr("%1 more matches. Narrow the search to see them.").arg(extra), listHost_);
            more->setObjectName(QStringLiteral("settingsRowDesc"));
            more->setContentsMargins(tokens::Space8, tokens::Space8, tokens::Space8, tokens::Space8);
            listLay_->addWidget(more);
        }
    }

    QWidget* TweakLedgerView::buildSession(const vth::TweakBatchGroup& group)
    {
        const QStringList domains = domainsOf(group);
        const bool single = domains.size() == 1;
        const bool open = !single && expanded_.contains(group.batchId);

        auto* host = new QWidget(listHost_);
        auto* col = new QVBoxLayout(host);
        col->setContentsMargins(0, 0, 0, 0);
        col->setSpacing(0);

        auto* head = new QFrame(host);
        head->setObjectName(QStringLiteral("settingsRow"));
        head->setAttribute(Qt::WA_StyledBackground, true);
        auto* row = new QHBoxLayout(head);
        row->setContentsMargins(tokens::Space8, tokens::Space8, tokens::Space8, tokens::Space8);
        row->setSpacing(tokens::CardMargin);

        auto* expand = new QToolButton(head);
        expand->setAutoRaise(true);
        expand->setCursor(Qt::PointingHandCursor);
        // A single change has nothing to expand, and its text starts on the
        // card's edge rather than after an empty arrow-sized gap.
        expand->setVisible(!single);
        expand->setAccessibleName(open ? tr("Hide the changes") : tr("Show the changes"));
        vth::icons::applyIconButton(expand, open ? QStringLiteral("chevron-down")
                                                 : QStringLiteral("chevron-right"),
                                    &pal::textSecondary, tokens::GlyphMd);
        const QString id = group.batchId;
        connect(expand, &QToolButton::clicked, this, [this, id]() {
            if (expanded_.contains(id)) expanded_.remove(id);
            else                        expanded_.insert(id);
            render();
        });
        row->addWidget(expand, 0, Qt::AlignVCenter);

        auto* text = new QVBoxLayout;
        text->setContentsMargins(0, 0, 0, 0);
        text->setSpacing(tokens::Space2);
        auto* name = new QLabel(sessionLabel(group, domains), head);
        name->setObjectName(QStringLiteral("settingsRowTitle"));
        name->setWordWrap(true);
        text->addWidget(name);
        auto* meta = new QLabel(single
            ? whenText(group.appliedAt)
            : whenText(group.appliedAt) + QStringLiteral("  ") + QChar(0x00B7) + QStringLiteral("  ")
                  + changesText(domains.size()), head);
        meta->setObjectName(QStringLiteral("settingsRowDesc"));
        text->addWidget(meta);
        row->addLayout(text, 1);

        auto* undo = new vth::widgets::CommandButton(
            vth::widgets::CommandButton::Variant::Secondary, tr("Undo"), head);
        undo->setEnabled(!busy_);
        const QString what = single ? domainDisplayName(domains.first())
                                    : sessionLabel(group, domains);
        connect(undo, &QPushButton::clicked, this, [this, id, what]() {
            requestUndo(Scope::Batch, id, what);
        });
        row->addWidget(undo, 0, Qt::AlignVCenter);
        col->addWidget(head);

        if (open) {
            auto* children = new QWidget(host);
            auto* childLay = new QVBoxLayout(children);
            childLay->setContentsMargins(tokens::MinTargetSize + tokens::CardMargin, 0, 0, tokens::Space4);
            childLay->setSpacing(0);
            for (const QString& d : domains)
                childLay->addWidget(buildChangeRow(d, children));
            col->addWidget(children);
        }
        return host;
    }

    QWidget* TweakLedgerView::buildChangeRow(const QString& domain, QWidget* parent)
    {
        auto* row = new QFrame(parent);
        row->setObjectName(QStringLiteral("settingsRow"));
        row->setAttribute(Qt::WA_StyledBackground, true);
        auto* lay = new QHBoxLayout(row);
        lay->setContentsMargins(tokens::Space8, tokens::Space4, tokens::Space8, tokens::Space4);
        lay->setSpacing(tokens::CardMargin);

        const QString name = domainDisplayName(domain);
        auto* label = new QLabel(name, row);
        label->setObjectName(QStringLiteral("settingsRowDesc"));
        label->setWordWrap(true);
        lay->addWidget(label, 1, Qt::AlignVCenter);

        auto* undo = new QToolButton(row);
        undo->setProperty("uiRole", QStringLiteral("link"));
        undo->setCursor(Qt::PointingHandCursor);
        undo->setText(tr("Undo"));
        undo->setAccessibleName(tr("Undo %1").arg(name));
        undo->setEnabled(!busy_);
        connect(undo, &QToolButton::clicked, this, [this, domain, name]() {
            requestUndo(Scope::Domain, domain, name);
        });
        lay->addWidget(undo, 0, Qt::AlignVCenter);
        return row;
    }

    void TweakLedgerView::setBusy(bool busy)
    {
        busy_ = busy;
        undoAllBtn_->setLoading(busy);
        for (auto* b : listHost_->findChildren<QAbstractButton*>())
            b->setEnabled(!busy);
    }

    void TweakLedgerView::requestUndo(Scope scope, const QString& key, const QString& what)
    {
        if (busy_)
            return;
        const QString title = scope == Scope::All ? tr("Undo everything?")
                                                  : tr("Undo \"%1\"?").arg(what);
        const QString body = scope == Scope::All
            ? tr("Every change listed here goes back to the value it had before Vethida "
                 "changed it. Anything you changed by hand since is overwritten. Apps you "
                 "installed and restore points are not affected.")
            : tr("These settings go back to the values they had before Vethida changed "
                 "them. Anything you changed by hand since is overwritten.");
        // Undoing one change needs no second thought; undoing everything asks.
        if ((scope == Scope::All || !vth::prefs::askLess())
            && !vth::dialogs::confirm(this, title, body, tr("Undo"),
                                   scope == Scope::All ? vth::dialogs::PromptTone::Warning
                                                       : vth::dialogs::PromptTone::Question))
            return;

        setBusy(true);
        undoToken_ = vth::fetch<vth::TweakStateTracker::RevertReport>(
            this,
            [scope, key]() {
                vth::TweakStateTracker::RevertReport r;
                vth::TweakStateTracker* t = tracker();
                if (!t)
                    return r;
                switch (scope) {
                case Scope::All:    return t->undoAll();
                case Scope::Batch:  return t->undoBatch(key);
                case Scope::Domain: return t->undoDomain(key);
                }
                return r;
            },
            [this](vth::TweakStateTracker::RevertReport r) {
                setBusy(false);
                refresh();
                auto* toasts = vth::ServiceRegistry::instance().get<vth::widgets::ToastManager>();
                if (r.failed == 0) {
                    if (toasts)
                        toasts->success(tr("Changes undone"),
                                        tr("Some changes take full effect after a restart."));
                    return;
                }
                QStringList names;
                for (const QString& d : r.failedDomains)
                    names << QStringLiteral("- ") + domainDisplayName(d);
                vth::dialogs::warn(this, tr("Some changes could not be undone"),
                    tr("Windows refused to put these back:\n\n%1\n\nThey stay in this list so "
                       "you can try again. If it keeps failing, a program that protects these "
                       "settings (security or management software) is the usual cause; a "
                       "restore point from before the change covers them too.")
                        .arg(names.join(QLatin1Char('\n'))));
            });
    }
}

