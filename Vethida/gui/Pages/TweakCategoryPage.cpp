#include "pch.h"
#include "TweakCategoryPage.h"

#include "../Dialogs/Confirm.h"
#include "../Dialogs/TweakDetailsDialog.h"
#include "../Theme/DesignTokens.h"
#include "../Utils/TablerIcon.h"
#include "../Widgets/CommandButton.h"
#include "../Widgets/SettingsList.h"
#include "../Widgets/Spinner.h"
#include "../Widgets/SwitchButton.h"

#include "../../vth_core/Models/ProductTier.h"
#include "../../vth_core/Tweaks/TweakEngine.h"

#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QShortcut>
#include <QSignalBlocker>
#include <QStyle>
#include <QElapsedTimer>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QVariant>

#include <utility>

namespace vth::ui
{
    using vth::tweaks::DetectState;

    namespace
    {
        constexpr int kInfoIcon = 18;

        // Section a tweak belongs to inside its category pane.
        QString sectionKeyFor(const vth::tweaks::TweakSpec* s)
        {
            return s->subCategory;
        }

        // Which vendor's hardware a tweak is confined to, as the QSS property
        // value, or empty for the great majority that work anywhere.
        QString vendorKeyFor(const vth::tweaks::TweakSpec* s)
        {
            return vth::tweaks::hardwareVendorKey(*s);
        }
    }

    TweakCategoryPage::TweakCategoryPage(QWidget* parent)
        : QWidget(parent)
    {
        setObjectName(QStringLiteral("tweakCategoryPage"));

        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(0, 0, 0, 0);
        root->setSpacing(0);

        // Pane header: category name on the left, search on the right.
        auto* headerRow = new QHBoxLayout;
        headerRow->setContentsMargins(vth::theme::tokens::PageMarginH,
                                      vth::theme::tokens::PageMarginTop,
                                      vth::theme::tokens::PageMarginH, 0);
        headerRow->setSpacing(vth::theme::tokens::CompactGap);
        m_titleLabel = new QLabel(this);
        m_titleLabel->setProperty("uiRole", QStringLiteral("sectionTitle"));
        headerRow->addWidget(m_titleLabel, 1, Qt::AlignVCenter);

        m_search = new QLineEdit(this);
        m_search->setPlaceholderText(tr("Search this category"));
        m_search->setAccessibleName(tr("Search this category"));
        m_search->setClearButtonEnabled(true);
        m_search->setMinimumWidth(vth::theme::tokens::FilterControlWidth);
        connect(m_search, &QLineEdit::textChanged, this,
                [this](const QString& text) { applyFilter(text); });
        headerRow->addWidget(m_search, 0, Qt::AlignVCenter);
        root->addLayout(headerRow);

        // The actions that act on the whole list: a sticky band under the
        // search, above the scrolling list.
        {
            m_bulkRow = new QWidget(this);
            // Named so the theme can draw the rule under it.
            m_bulkRow->setObjectName(QStringLiteral("recommendBand"));
            m_bulkRow->setAttribute(Qt::WA_StyledBackground, true);
            auto* band = new QVBoxLayout(m_bulkRow);
            band->setContentsMargins(0, 0, 0, 0);
            band->setSpacing(0);

            auto* rowHost = new QWidget(m_bulkRow);
            auto* rowLay = new QHBoxLayout(rowHost);
            rowLay->setContentsMargins(vth::theme::tokens::PageMarginH,
                                       vth::theme::tokens::Space12,
                                       vth::theme::tokens::PageMarginH,
                                       vth::theme::tokens::Space12);
            rowLay->setSpacing(vth::theme::tokens::CardMargin);

            auto* textCol = new QVBoxLayout;
            textCol->setContentsMargins(0, 0, 0, 0);
            textCol->setSpacing(vth::theme::tokens::Space2);
            auto* title = new QLabel(tr("Recommended for this PC"), rowHost);
            title->setObjectName(QStringLiteral("settingsRowTitle"));
            textCol->addWidget(title);
            auto* note = new QLabel(
                tr("Changes that suit this PC and carry no warnings or trade-offs. "
                   "Apply all adds the rest, except anything that cannot be undone."),
                rowHost);
            note->setObjectName(QStringLiteral("settingsRowDesc"));
            note->setWordWrap(true);
            textCol->addWidget(note);
            // Kept, so a run can name its tweak on this line and the line can be put back.
            m_bulkNote     = note;
            m_bulkNoteText = note->text();
            rowLay->addLayout(textCol, 1);

            m_undoBtn = new vth::widgets::CommandButton(
                vth::widgets::CommandButton::Variant::Secondary,
                tr("Undo"), rowHost);
            m_undoBtn->setCursor(Qt::PointingHandCursor);
            m_undoBtn->setToolTip(tr("Turns off what the last apply here turned on."));
            m_undoBtn->hide();
            connect(m_undoBtn, &QPushButton::clicked, this,
                    [this]() { requestBulk(BulkAction::Undo); });
            rowLay->addWidget(m_undoBtn, 0, Qt::AlignVCenter);

            m_applyAllBtn = new vth::widgets::CommandButton(
                vth::widgets::CommandButton::Variant::Secondary,
                tr("Apply all"), rowHost);
            m_applyAllBtn->setCursor(Qt::PointingHandCursor);
            m_applyAllBtn->setEnabled(false);
            connect(m_applyAllBtn, &QPushButton::clicked, this,
                    [this]() { requestBulk(BulkAction::Everything); });
            rowLay->addWidget(m_applyAllBtn, 0, Qt::AlignVCenter);

            m_recommendBtn = new vth::widgets::CommandButton(
                vth::widgets::CommandButton::Variant::Primary,
                tr("Apply recommended"), rowHost);
            m_recommendBtn->setCursor(Qt::PointingHandCursor);
            m_recommendBtn->setEnabled(false);
            connect(m_recommendBtn, &QPushButton::clicked, this,
                    [this]() { requestBulk(BulkAction::Recommended); });
            rowLay->addWidget(m_recommendBtn, 0, Qt::AlignVCenter);
            band->addWidget(rowHost);

            // Hidden until a category is set: a pane showing search results
            // across categories has no single set to act on.
            m_bulkRow->hide();
            root->addWidget(m_bulkRow);
        }

        m_list = new vth::widgets::SettingsList(this);
        root->addWidget(m_list, 1);

        // Shown only when a search matches nothing, so an empty pane always says
        // why it is empty.
        m_noMatch = new QLabel(tr("No tweaks match that search."), this);
        m_noMatch->setObjectName(QStringLiteral("settingsRowDesc"));
        m_noMatch->setContentsMargins(vth::theme::tokens::PageMarginH, 0,
                                      vth::theme::tokens::PageMarginH,
                                      vth::theme::tokens::PageMarginBottom);
        m_noMatch->hide();
        root->addWidget(m_noMatch);

        // Overlay, not a layout item: it must sit over the content region
        // without the list reflowing when it appears and disappears.
        m_spinner = new vth::widgets::Spinner(this);
        m_spinner->hide();

        // Ctrl+F and Escape, on a pane that can hold several hundred rows.
        const auto focusSearch = [this]() {
            if (!m_search || !m_search->isVisible())
                return;
            m_search->setFocus(Qt::ShortcutFocusReason);
            m_search->selectAll();
        };
        m_findShortcut = new QShortcut(QKeySequence::Find, this);
        m_findShortcut->setContext(Qt::WindowShortcut);
        connect(m_findShortcut, &QShortcut::activated, this, focusSearch);
        connect(m_findShortcut, &QShortcut::activatedAmbiguously, this, focusSearch);

        // Escape clears the search rather than closing anything, and only while
        // there is something to clear.
        m_clearShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), m_search);
        m_clearShortcut->setContext(Qt::WidgetWithChildrenShortcut);
        m_clearShortcut->setEnabled(false);
        connect(m_clearShortcut, &QShortcut::activated, m_search, &QLineEdit::clear);
    }

    // An empty title hides the label.
    void TweakCategoryPage::setTitle(const QString& title)
    {
        if (!m_titleLabel)
            return;
        m_titleLabel->setText(title);
        m_titleLabel->setVisible(!title.isEmpty());
    }

    void TweakCategoryPage::setPlaceholder(const QString& text)
    {
        m_placeholder = text;
        if (m_list)
            m_list->setPlaceholder(text);
    }

    void TweakCategoryPage::setBulkCategory(const QString& category)
    {
        if (m_bulkCategory == category)
            return;
        m_bulkCategory = category;
        applyHeaderVisibility();
        Q_EMIT bulkCategoryChanged();
    }

    void TweakCategoryPage::setBulkReady(bool ready)
    {
        m_bulkReady = ready;
        applyBandState();
        applyHeaderVisibility();
    }

    void TweakCategoryPage::setBulkBusy(bool busy)
    {
        // Only the primary button spins, whichever was pressed.
        if (m_recommendBtn)
            m_recommendBtn->setLoading(busy);
        // The name belongs to a run in flight.
        if (!busy && m_bulkNote)
            m_bulkNote->setText(m_bulkNoteText);
        applyBandState();
    }

    void TweakCategoryPage::setBulkProgress(const QString& label)
    {
        if (!m_bulkNote || label.isEmpty())
            return;
        // Only the pane that pressed, whose spinner says so.
        if (!m_recommendBtn || !m_recommendBtn->isLoading())
            return;
        m_bulkNote->setText(tr("Applying %1").arg(label));
    }

    void TweakCategoryPage::setBulkAvailability(bool enabled, bool undoable)
    {
        m_bulkEnabled = enabled;
        m_undoable    = undoable;
        applyBandState();
    }

    // One answer for all three buttons, from all four facts.
    void TweakCategoryPage::applyBandState()
    {
        const bool spinning = m_recommendBtn && m_recommendBtn->isLoading();
        const bool live = m_bulkReady && m_bulkEnabled && !spinning;
        if (m_recommendBtn && !spinning)
            m_recommendBtn->setEnabled(live);
        if (m_applyAllBtn)
            m_applyAllBtn->setEnabled(live);
        if (m_undoBtn) {
            m_undoBtn->setVisible(m_undoable);
            m_undoBtn->setEnabled(live && m_undoable);
        }
    }

    // The one place a bulk action leaves this pane, so the list it acts on and
    // the prompt in front of it are decided once.
    void TweakCategoryPage::requestBulk(BulkAction action)
    {
        if (m_bulkCategory.isEmpty())
            return;

        if (action == BulkAction::Everything
            && !vth::dialogs::cautionGate(
                   this, QStringLiteral("applyAll"),
                   tr("Apply everything in this section?"),
                   tr("Turns on every change here that your plan allows, including "
                      "ones with a warning. Left out: anything that can't be undone, "
                      "is rated Critical, switches off a Windows protection, or is for "
                      "hardware this PC doesn't have.\n\n"
                      "Undo appears here afterwards and reverses this run."))) {
            return;
        }

        Q_EMIT bulkRequested(m_bulkCategory, action,
                             action == BulkAction::Undo ? QStringList() : visibleIds());
    }

    void TweakCategoryPage::setTweaks(const QVector<const vth::tweaks::TweakSpec*>& specs)
    {
        m_specs = specs;
        rebuild();
    }

    void TweakCategoryPage::setStatesLoading(bool loading)
    {
        if (m_statesLoading == loading)
            return;
        m_statesLoading = loading;
        applyLoadingState();
    }

    // The loader rather than a grayed list, which would draw every switch off.
    void TweakCategoryPage::applyLoadingState()
    {
        const bool busy = m_statesLoading || !m_rowsShown;
        if (m_list) {
            m_list->setEnabled(!m_statesLoading);
            m_list->setVisible(!busy);
        }
        if (m_spinner) {
            m_spinner->setVisible(busy);
            if (busy) {
                m_spinner->raise();
                layoutSpinner();
            }
        }
        applyHeaderVisibility();
    }

    DetectState TweakCategoryPage::stateFor(const QString& id) const
    {
        return static_cast<DetectState>(
            m_state.value(id, static_cast<int>(DetectState::Unknown)));
    }

    vth::models::ProductTier TweakCategoryPage::tierValue() const
    {
        return vth::models::tierFromRank(m_tierRank);
    }

    void TweakCategoryPage::setDetected(const QHash<QString, int>& states)
    {
        for (auto it = states.constBegin(); it != states.constEnd(); ++it)
            m_state.insert(it.key(), it.value());
        for (const RowEntry& e : m_rows)
            applyRowState(e.spec->id);
    }

    void TweakCategoryPage::setTweakState(const QString& id, int state)
    {
        m_state.insert(id, state);
        applyRowState(id);
    }

    bool TweakCategoryPage::toggleEnabledFor(const RowEntry& entry) const
    {
        if (!m_interactive || !entry.spec)
            return false;
        const DetectState state = stateFor(entry.spec->id);
        // Applied and not undoable: the switch would only ever produce an error.
        if (state == DetectState::On && !entry.spec->reversible)
            return false;
        // A tier lock stops it being turned on. Turning off is always allowed, and only a
        // tweak detected as fully On counts as something to undo.
        if (entry.locked && state != DetectState::On)
            return false;
        return true;
    }

    void TweakCategoryPage::setToggleInteractive(bool interactive)
    {
        if (m_interactive == interactive)
            return;
        m_interactive = interactive;
        for (const RowEntry& e : m_rows) {
            if (!e.row)
                continue;
            const bool live = toggleEnabledFor(e);
            if (e.row->toggle())
                e.row->toggle()->setEnabled(live);
            e.row->setInteractive(live);
        }
    }

    void TweakCategoryPage::setCurrentTierRank(int rank)
    {
        if (m_tierRank == rank)
            return;
        m_tierRank = rank;
        rebuild();   // lock state changes which rows show a tier pill
    }

    void TweakCategoryPage::setEarlyAccessOptIn(bool optedIn)
    {
        if (m_earlyAccessOptIn == optedIn)
            return;
        m_earlyAccessOptIn = optedIn;
        rebuild();
    }

    // Unknown looks like Off, so add a chip saying so
    void TweakCategoryPage::applyRowStateChip(const RowEntry& entry, DetectState state)
    {
        if (!entry.stateChip)
            return;

        const bool show = state == DetectState::Unknown;
        if (show) {
            entry.stateChip->setText(tr("State unknown"));
            entry.stateChip->setToolTip(
                tr("Windows did not report this setting's current value, so "
                   "the switch may not match it. Applying it still works."));
            // The muted treatment, because unreadable is a caveat.
            entry.stateChip->setProperty("tone", QVariant());
            entry.stateChip->setProperty("muted", QStringLiteral("true"));
            // Both properties feed selectors, so the chip has to be re-polished
            // for the change to reach the paint.
            entry.stateChip->style()->unpolish(entry.stateChip);
            entry.stateChip->style()->polish(entry.stateChip);
        }
        entry.stateChip->setVisible(show);

        // The strip goes with it when it has nothing else on it, so a row that
        // becomes ordinary again returns to the height it had.
        if (entry.meta)
            entry.meta->setVisible(entry.metaStatic || show);
    }

    void TweakCategoryPage::applyRowState(const QString& id)
    {
        const int idx = m_rowByIndex.value(id, -1);
        if (idx < 0 || idx >= m_rows.size())
            return;
        const RowEntry& entry = m_rows.at(idx);
        // Re-evaluated here, because whether the switch accepts input depends on whether
        // the tweak is applied.
        const bool live = toggleEnabledFor(entry);
        const DetectState state = stateFor(id);
        const bool isOn = state == DetectState::On;
        if (vth::widgets::SwitchButton* sw = entry.row->toggle()) {
            const QSignalBlocker blocker(sw);
            sw->setChecked(isOn);
            sw->setEnabled(live);
        }

        applyRowStateChip(entry, state);

        // A one-way row swaps its control for its outcome.
        if (entry.applyBtn) {
            entry.applyBtn->setVisible(!isOn);
            // A tier lock still gates applying it, exactly as it gates a switch.
            entry.applyBtn->setEnabled(live);
        }
        if (entry.appliedTag)
            entry.appliedTag->setVisible(isOn);

        // The button is the only way into a one-way row.
        if (entry.row)
            entry.row->setInteractive(live && !entry.applyBtn);
    }

    // visible rows, in display order
    QStringList TweakCategoryPage::visibleIds() const
    {
        QStringList ids;
        ids.reserve(m_rows.size());
        for (const RowEntry& entry : m_rows) {
            if (entry.spec && entry.row && !entry.row->isHidden())
                ids << entry.spec->id;
        }
        return ids;
    }

    void TweakCategoryPage::applyFilter(const QString& query)
    {
        const QString needle = query.trimmed();
        const bool filtering = !needle.isEmpty();
        if (m_clearShortcut)
            m_clearShortcut->setEnabled(filtering);

        int visible = 0;
        for (const RowEntry& entry : m_rows) {
            if (!entry.row || !entry.spec)
                continue;
            const bool match = !filtering
                || entry.spec->name.contains(needle, Qt::CaseInsensitive)
                || entry.spec->description.contains(needle, Qt::CaseInsensitive);
            entry.row->setVisible(match);
            if (match)
                ++visible;
        }
        for (QWidget* heading : m_headings) {
            if (heading)
                heading->setVisible(!filtering);
        }
        if (m_noMatch)
            m_noMatch->setVisible(filtering && visible == 0);
    }

    void TweakCategoryPage::rebuild()
    {
        if (!m_list)
            return;

        // Invalidate any chunk still queued from a previous plan.
        ++m_buildToken;

        m_list->clear();
        m_rows.clear();
        m_rowByIndex.clear();
        m_headings.clear();   // owned by the list, which just deleted them
        m_list->setPlaceholder(m_placeholder);

        // Stable, alphabetical section order; the unlabelled group sorts first so
        // a category with no subCategories reads as one plain list.
        QMap<QString, QVector<const vth::tweaks::TweakSpec*>> sections;
        for (const vth::tweaks::TweakSpec* s : m_specs) {
            const QString key = sectionKeyFor(s);
            sections[key].push_back(s);
        }

        m_plan.clear();
        m_plan.reserve(m_specs.size() + sections.size());
        for (auto it = sections.begin(); it != sections.end(); ++it) {
            if (!it.key().isEmpty())
                m_plan.push_back(BuildItem{ nullptr, it.key() });
            for (const vth::tweaks::TweakSpec* s : it.value())
                m_plan.push_back(BuildItem{ s, QString() });
        }
        m_planPos = 0;

        if (m_plan.isEmpty()) {
            finishBuild();
            return;
        }

        // Loader first, rows after, so the pane appears immediately.
        m_rowsShown = false;
        setRowsBuilt(false);
        applyLoadingState();
        buildChunk();
    }

    void TweakCategoryPage::buildChunk()
    {
        // One slice per event-loop turn, bounded by time rather than a row
        // count so a slow PC gets smaller slices instead of a stall.
        constexpr qint64 kSliceMs = 10;
        const quint64 token = m_buildToken;
        QElapsedTimer slice;
        slice.start();

        while (m_planPos < m_plan.size()
               && (slice.elapsed() < kSliceMs || m_planPos == 0)) {
            const BuildItem& item = m_plan.at(m_planPos++);

            if (!item.spec) {
                m_headings.push_back(m_list->addHeading(item.heading));
                continue;
            }
            {
                const vth::tweaks::TweakSpec* s = item.spec;
                const vth::tweaks::TweakLock lock =
                    vth::tweaks::lockStateFor(*s, tierValue(), m_earlyAccessOptIn);
                const bool locked = lock != vth::tweaks::TweakLock::Available;

                vth::widgets::SettingsRow* row =
                    m_list->addRow(s->name, s->description);

                // Details live behind an info button, so the row body is free to toggle.
                auto* info = new QToolButton(row);
                info->setCursor(Qt::PointingHandCursor);
                info->setFocusPolicy(Qt::TabFocus);
                info->setAccessibleName(tr("About this tweak"));
                info->setToolTip(tr("About this tweak"));
                vth::icons::applyIconButton(info, QStringLiteral("info"),
                                            &vth::theme::palette::textMuted, kInfoIcon);
                const QString id = s->id;
                connect(info, &QToolButton::clicked, this,
                        [this, id] { Q_EMIT detailsRequested(id); });
                row->addTrailing(info);

                // Warnings, irreversibility and the tier pill go on a caption strip under
                // the description, where they don't squeeze the text.
                const bool showPill = locked || s->earlyAccess;
                const bool hasWarnings = !s->warnings.isEmpty();
                QString stateText;
                if (s->impact != vth::tweaks::Impact::Low)
                    stateText = tr("%1 impact").arg(vth::tweaks::impactLabel(s->impact));

                // The vendor a tweak is confined to, derived from what it writes.
                const QString vendor = vendorKeyFor(s);

                // Built on every row, because any row can turn out to be unreadable. Hidden
                // while it holds nothing visible.
                const bool metaStatic = showPill || hasWarnings || !s->reversible
                                     || !stateText.isEmpty() || !vendor.isEmpty();
                QLabel*  stateChip = nullptr;
                QWidget* metaHost  = nullptr;
                {
                    auto* meta = new QWidget(row);
                    auto* metaLay = new QHBoxLayout(meta);
                    metaLay->setContentsMargins(0, vth::theme::tokens::CompactGap, 0, 0);
                    metaLay->setSpacing(vth::theme::tokens::CompactGap);
                    if (!vendor.isEmpty()) {
                        const QString vendorName =
                            vendor == QLatin1String("nvidia") ? QStringLiteral("NVIDIA")
                                                              : QStringLiteral("AMD");
                        auto* mark = new QLabel(vendorName, meta);
                        mark->setProperty("uiRole", QStringLiteral("chip"));
                        mark->setProperty("vendor", vendor);
                        mark->setAttribute(Qt::WA_StyledBackground, true);
                        mark->setToolTip(tr("Applies only to %1 hardware.")
                                             .arg(vendorName));
                        metaLay->addWidget(mark, 0, Qt::AlignVCenter);
                    }
                    if (showPill) {
                        auto* pill = new QLabel(meta);
                        pill->setObjectName(QStringLiteral("tweakCardTierPill"));
                        pill->setAttribute(Qt::WA_StyledBackground, true);
                        const bool withdrawn =
                            lock == vth::tweaks::TweakLock::DisabledRemotely;
                        const bool early = !withdrawn
                                        && (lock == vth::tweaks::TweakLock::LockedEarlyAccess
                                            || (!locked && s->earlyAccess));
                        // A withdrawn tweak never wears a tier pill, which is a price tag.
                        pill->setProperty("kind", withdrawn ? QStringLiteral("early")
                                                : early     ? QStringLiteral("early")
                                                            : QStringLiteral("tier"));
                        // Only a real tier pill takes a tier's color.
                        if (!withdrawn && !early
                            && s->minTier == vth::models::ProductTier::Ultimate) {
                            pill->setProperty("tier", QStringLiteral("ultimate"));
                        }
                        pill->setText(withdrawn ? tr("Withdrawn")
                                    : early     ? tr("Early access")
                                                : vth::models::tierLabel(s->minTier));
                        if (withdrawn) {
                            const QString why = vth::tweaks::remoteDisableReason(s->id);
                            pill->setToolTip(why.isEmpty()
                                ? tr("Turned off in a Vethida update because it did "
                                     "not behave correctly.")
                                : why);
                        }
                        metaLay->addWidget(pill, 0, Qt::AlignVCenter);
                    }
                    if (hasWarnings) {
                        const int warnCount = static_cast<int>(s->warnings.size());
                        auto* warn = new QToolButton(meta);
                        warn->setProperty("uiRole", QStringLiteral("chip"));
                        warn->setProperty("tone", QStringLiteral("warn"));
                        warn->setAutoRaise(true);
                        warn->setCursor(Qt::PointingHandCursor);
                        warn->setFocusPolicy(Qt::TabFocus);
                        warn->setText(warnCount == 1
                            ? tr("1 warning")
                            : tr("%1 warnings").arg(warnCount));
                        warn->setToolTip(s->warnings.join(QLatin1Char('\n')));
                        warn->setAccessibleName(
                            tr("Warnings for %1. Opens the full text.").arg(s->name));
                        const vth::tweaks::TweakSpec* warnSpec = s;
                        connect(warn, &QToolButton::clicked, this,
                                [this, warnSpec] {
                                    vth::dialogs::showTweakWarnings(this, *warnSpec);
                                });
                        metaLay->addWidget(warn, 0, Qt::AlignVCenter);
                    }
                    if (!s->reversible) {
                        auto* perm = new QLabel(tr("Permanent"), meta);
                        perm->setProperty("uiRole", QStringLiteral("chip"));
                        perm->setProperty("tone", QStringLiteral("critical"));
                        perm->setToolTip(tr("This cannot be undone."));
                        metaLay->addWidget(perm, 0, Qt::AlignVCenter);
                    }
                    if (!stateText.isEmpty()) {
                        auto* state = new QLabel(stateText, meta);
                        state->setObjectName(QStringLiteral("tweakRowState"));
                        // The level as a property, so the stylesheet grades the ink.
                        state->setProperty(
                            "impact",
                            vth::tweaks::impactLabel(s->impact).toLower());
                        metaLay->addWidget(state, 0, Qt::AlignVCenter);
                    }
                    // What the machine says right now. Filled in by applyRowState.
                    stateChip = new QLabel(meta);
                    stateChip->setProperty("uiRole", QStringLiteral("chip"));
                    stateChip->setAttribute(Qt::WA_StyledBackground, true);
                    stateChip->hide();
                    metaLay->addWidget(stateChip, 0, Qt::AlignVCenter);

                    metaLay->addStretch(1);
                    row->setMeta(meta);
                    meta->setVisible(metaStatic);
                    metaHost = meta;
                }

                vth::widgets::SwitchButton* sw = row->toggle();
                const DetectState detected = stateFor(s->id);
                const bool isOn = detected == DetectState::On;

                // An irreversible tweak that's already applied can't be switched back,
                // because the engine refuses the revert.
                const bool stuckOn = isOn && !s->reversible;
                // A tier lock gates applying, never undoing.
                const bool live = m_interactive
                               && !(locked && detected != DetectState::On)
                               && !stuckOn;
                sw->setEnabled(live);
                row->setInteractive(live);
                if (stuckOn) {
                    sw->setToolTip(tr("Applied, and this cannot be undone. Return Windows "
                                      "to a restore point from before you applied it, or "
                                      "change the setting back in Windows."));
                }
                {
                    const QSignalBlocker blocker(sw);
                    sw->setChecked(isOn);
                }
                // Put the switch back before anyone sees it move, and let the
                // controller be the only thing that ever turns it on.
                connect(sw, &QAbstractButton::toggled, this,
                        [this, id, sw](bool on) {
                            {
                                const QSignalBlocker blocker(sw);
                                sw->setChecked(!on);
                            }
                            Q_EMIT tweakToggled(id, on);
                        });

                // A one-way change is a button, not a switch.
                vth::widgets::CommandButton* applyBtn   = nullptr;
                QLabel*                      appliedTag = nullptr;
                if (!s->reversible) {
                    sw->hide();

                    applyBtn = new vth::widgets::CommandButton(
                        vth::widgets::CommandButton::Variant::Secondary, tr("Apply"), row);
                    applyBtn->setCursor(Qt::PointingHandCursor);
                    applyBtn->setAccessibleName(tr("Apply %1").arg(s->name));
                    applyBtn->setAccessibleDescription(
                        tr("This cannot be undone."));
                    connect(applyBtn, &QPushButton::clicked, this,
                            [this, id]() { Q_EMIT tweakToggled(id, true); });
                    row->addTrailing(applyBtn);

                    appliedTag = new QLabel(tr("Applied"), row);
                    appliedTag->setProperty("uiRole", QStringLiteral("chip"));
                    appliedTag->setProperty("tone", QStringLiteral("good"));
                    appliedTag->setAttribute(Qt::WA_StyledBackground, true);
                    appliedTag->setToolTip(
                        tr("Applied, and this cannot be undone. Return Windows to a "
                           "restore point from before you applied it, or change the "
                           "setting back in Windows."));
                    row->addTrailing(appliedTag);

                    // The same lines applyRowState runs, so the row is right from the start.
                    applyBtn->setVisible(!isOn);
                    applyBtn->setEnabled(live);
                    appliedTag->setVisible(isOn);
                    row->setInteractive(false);
                }

                // The switch has to carry the tweak's name itself.
                sw->setAccessibleName(s->name);
                QStringList spoken;
                if (!s->description.isEmpty())
                    spoken << s->description;
                if (!s->warnings.isEmpty())
                    spoken << s->warnings;
                // The state the switch's own position can't carry.
                if (detected == DetectState::Unknown)
                    spoken << tr("Vethida could not read this setting back.");
                if (stuckOn)
                    spoken << tr("Applied. This cannot be undone.");
                else if (!s->reversible)
                    spoken << tr("This cannot be undone.");
                if (locked && isOn) {
                    // Applied on a tier that no longer includes it.
                    spoken << tr("Applied. Your plan no longer includes this, but you "
                                 "can still switch it off.");
                } else if (locked) {
                    spoken << tr("Included with %1.")
                                  .arg(vth::models::tierProductName(s->minTier));
                }
                sw->setAccessibleDescription(spoken.join(QLatin1Char(' ')));
                // The switch is hidden on a one-way row, so the button carries the sentence.
                if (applyBtn)
                    applyBtn->setAccessibleDescription(sw->accessibleDescription());

                // Clicking the row toggles. A locked row isn't interactive, so this can't
                // reach one.
                connect(row, &vth::widgets::SettingsRow::bodyClicked, this,
                        [sw] {
                            if (!sw->isHidden() && sw->isEnabled())
                                sw->toggle();
                        });

                m_rowByIndex.insert(s->id, m_rows.size());
                m_rows.push_back(RowEntry{ s, row, locked, applyBtn, appliedTag,
                                           metaHost, stateChip, metaStatic });
                applyRowStateChip(m_rows.back(), detected);
            }
        }

        if (m_planPos < m_plan.size()) {
            // The first slice fills the screen, so show it now and let the
            // rest arrive below it. The page's actions wait for finishBuild.
            if (!m_rowsShown) {
                m_rowsShown = true;
                applyLoadingState();
            }
            // A short delay rather than 0, so paint and input get a turn
            // between slices. The token drops a slice from an older rebuild.
            constexpr int kYieldMs = 4;
            QTimer::singleShot(kYieldMs, this, [this, token] {
                if (token == m_buildToken)
                    buildChunk();
            });
            return;
        }
        finishBuild();
    }

    void TweakCategoryPage::finishBuild()
    {
        m_plan.clear();
        m_planPos = 0;
        setRowsBuilt(true);
        applyLoadingState();
        // Rows are new objects, so a filter typed before the rebuild has to be
        // re-applied or it silently stops matching anything.
        if (m_search)
            applyFilter(m_search->text());
    }

    void TweakCategoryPage::setRowsBuilt(bool built)
    {
        m_rowsBuilt = built;
        if (built)
            m_rowsShown = true;
        applyHeaderVisibility();
    }

    // The ONE place that decides what is on screen besides the rows.
    void TweakCategoryPage::applyHeaderVisibility()
    {
        const bool header = m_rowsBuilt && !m_statesLoading;
        const bool shown  = m_rowsShown && !m_statesLoading;
        if (m_titleLabel)
            m_titleLabel->setVisible(shown && !m_titleLabel->text().isEmpty());
        if (m_search)
            m_search->setVisible(shown);
        if (m_bulkRow)
            m_bulkRow->setVisible(header && m_bulkReady
                                  && !m_bulkCategory.isEmpty());
    }

    void TweakCategoryPage::layoutSpinner()
    {
        if (!m_spinner)
            return;
        m_spinner->move((width() - m_spinner->width()) / 2,
                        (height() - m_spinner->height()) / 2);
    }

    void TweakCategoryPage::resizeEvent(QResizeEvent* event)
    {
        QWidget::resizeEvent(event);
        layoutSpinner();
    }
}
