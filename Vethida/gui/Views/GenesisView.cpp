#include "pch.h"
#include "GenesisView.h"

#include "../Theme/DesignTokens.h"
#include "../Widgets/GlyphLabel.h"
#include "../Widgets/NameListEditor.h"
#include "../Widgets/SettingsList.h"
#include "../Widgets/SwitchButton.h"
#include "../Widgets/TierLockOverlay.h"

#include "../../vth_core/ProtectedProcesses.h"
#include "../../vth_core/Runtime/ServiceRegistry.h"
#include "../../vth_core/Runtime/Settings.h"
#include "../../vth_core/Runtime/Threading.h"
#include "../../vth_core/Models/ProductTier.h"
#include "../../vth_core/Runtime/Tier.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

#include <functional>
#include <utility>

namespace vth::ui
{
    namespace
    {
        namespace gm     = vth::win::genesis;
        namespace tokens = vth::theme::tokens;
        namespace pal    = vth::theme::palette;

        void repolish(QWidget* w)
        {
            w->style()->unpolish(w);
            w->style()->polish(w);
            w->update();
        }

        QLabel* makeChip(const QString& text, const char* tone, QWidget* parent)
        {
            auto* chip = new QLabel(text, parent);
            chip->setProperty("uiRole", QStringLiteral("chip"));
            if (tone)
                chip->setProperty("tone", QString::fromLatin1(tone));
            chip->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
            return chip;
        }
    }

    // One of the three starting points. A whole-tile click target with a glyph,
    // a name and one line; the selected tile carries the accent edge.
    class GenesisPresetTile final : public QFrame
    {
    public:
        GenesisPresetTile(const QString& icon, const QString& title,
                          const QString& line, std::function<void()> onClick,
                          QWidget* parent)
            : QFrame(parent), onClick_(std::move(onClick))
        {
            setObjectName(QStringLiteral("genesisPresetTile"));
            setAttribute(Qt::WA_StyledBackground, true);
            setAttribute(Qt::WA_Hover, true);
            setFocusPolicy(Qt::TabFocus);
            setCursor(Qt::PointingHandCursor);
            setAccessibleName(title);
            setAccessibleDescription(line);

            auto* row = new QHBoxLayout(this);
            row->setContentsMargins(tokens::Space12, tokens::Space8 + tokens::Space2,
                                    tokens::Space12, tokens::Space8 + tokens::Space2);
            row->setSpacing(tokens::Space12);

            glyph_ = new vth::widgets::GlyphLabel(
                icon, [this] { return selected_ ? pal::accentOnSurface() : pal::iconNeutral(); },
                tokens::GlyphLg, this);
            row->addWidget(glyph_, 0, Qt::AlignVCenter);

            auto* text = new QVBoxLayout;
            text->setContentsMargins(0, 0, 0, 0);
            text->setSpacing(tokens::Space2);
            auto* name = new QLabel(title, this);
            name->setObjectName(QStringLiteral("genesisPresetTitle"));
            text->addWidget(name);
            auto* desc = new QLabel(line, this);
            desc->setObjectName(QStringLiteral("settingsRowDesc"));
            desc->setWordWrap(true);
            text->addWidget(desc);
            row->addLayout(text, 1);
        }

        void setSelected(bool on)
        {
            if (selected_ == on)
                return;
            selected_ = on;
            setProperty("selected", on);
            repolish(this);
            if (auto* t = findChild<QLabel*>(QStringLiteral("genesisPresetTitle"))) {
                t->setProperty("selected", on);
                repolish(t);
            }
            glyph_->refresh();
        }

    protected:
        void mouseReleaseEvent(QMouseEvent* e) override
        {
            if (e->button() == Qt::LeftButton && rect().contains(e->position().toPoint()) && onClick_)
                onClick_();
            QFrame::mouseReleaseEvent(e);
        }

        void keyPressEvent(QKeyEvent* e) override
        {
            if ((e->key() == Qt::Key_Space || e->key() == Qt::Key_Return
                 || e->key() == Qt::Key_Enter) && onClick_) {
                onClick_();
                return;
            }
            QFrame::keyPressEvent(e);
        }

    private:
        std::function<void()>      onClick_;
        vth::widgets::GlyphLabel*  glyph_ = nullptr;
        bool                       selected_ = false;
    };

    GenesisView::GenesisView(QWidget* parent) : QWidget(parent) {
        VTH_ASSERT_UI_THREAD();
        setObjectName(QStringLiteral("GenesisView"));
        buildUi();
        applyOptions(gm::ActivateOptions{});
        renderState();

        // The session record is a file and the picks and both lists are
        // QSettings, so all of it is read on a worker and applied when it lands.
        struct Snapshot {
            gm::State           state;
            gm::ActivateOptions options;
            QStringList         closeList;
            QStringList         exceptions;
        };
        loadToken_ = vth::fetch<Snapshot>(
            this,
            []() {
                return Snapshot{ gm::loadPersistedState(), gm::optionsFromSettings(),
                                 gm::closeListFromSettings(), gm::exceptionsFromSettings() };
            },
            [this](Snapshot s) {
                state_ = std::move(s.state);
                applyOptions(s.options);
                if (closeEditor_)     closeEditor_->setNames(s.closeList);
                if (exceptionEditor_) exceptionEditor_->setNames(s.exceptions);
                renderState();
            });

        // Completions for both add fields, from what is running now. A Toolhelp
        // walk is not UI-thread work.
        runningToken_ = vth::fetch<QStringList>(
            this,
            []() {
                QStringList names;
                for (const auto& p : gm::enumerateProcesses()) {
                    if (!names.contains(p.name, Qt::CaseInsensitive))
                        names << p.name;
                }
                names.sort(Qt::CaseInsensitive);
                return names;
            },
            [this](QStringList names) {
                if (closeEditor_)     closeEditor_->setSuggestions(names);
                if (exceptionEditor_) exceptionEditor_->setSuggestions(names);
            });

        if (!vth::isUltimate()) {
            auto* lock = new vth::widgets::TierLockOverlay(
                this, vth::models::ProductTier::Ultimate);
            lock->show();
            lock->raise();
        }
    }

    GenesisView::~GenesisView() {
        loadToken_.cancel();
        runningToken_.cancel();
    }

    int GenesisView::addOption(const char* key, bool gm::ActivateOptions::* field,
                               bool light, bool balanced, bool full,
                               const QString& title, const QString& description,
                               const QString& caveat) {
        Option o;
        o.key = key;
        o.field = field;
        o.inPreset[0] = light;
        o.inPreset[1] = balanced;
        o.inPreset[2] = full;

        auto* row = new vth::widgets::SettingsRow(title, description);
        row->setInteractive(true);
        if (!caveat.isEmpty())
            row->addTrailing(makeChip(caveat, "warn", row));
        o.toggle = row->toggle();
        connect(row, &vth::widgets::SettingsRow::bodyClicked, o.toggle,
                [t = o.toggle] { t->toggle(); });
        connect(o.toggle, &QAbstractButton::toggled, this, [this](bool) {
            persistOptions();
            refreshSummary();
        });

        options_.push_back(o);
        optionRows_.push_back(row);
        return options_.size() - 1;
    }

    // The starting points across the top, then either the options or the two app lists,
    // and one status line at the foot.
    void GenesisView::buildUi() {
        auto* outer = new QVBoxLayout(this);
        outer->setContentsMargins(0, 0, 0, 0);
        outer->setSpacing(tokens::CardMargin);

        // Starting points.
        auto* presets = new QHBoxLayout;
        presets->setContentsMargins(0, 0, 0, 0);
        presets->setSpacing(tokens::CardMargin);
        presetTiles_[0] = new GenesisPresetTile(
            QStringLiteral("sparkles"), tr("Light"),
            tr("Closes apps and frees memory"),
            [this] { applyPreset(Preset::Light); }, this);
        presetTiles_[1] = new GenesisPresetTile(
            QStringLiteral("gauge"), tr("Balanced"),
            tr("Adds pausing, quiet mode and priority"),
            [this] { applyPreset(Preset::Balanced); }, this);
        presetTiles_[2] = new GenesisPresetTile(
            QStringLiteral("rocket"), tr("Full"),
            tr("Everything, CPU and graphics included"),
            [this] { applyPreset(Preset::Full); }, this);
        for (auto* tile : presetTiles_)
            presets->addWidget(tile, 1);
        outer->addLayout(presets);

        // Options. The three flags after each name are Light, Balanced, Full:
        // which starting points switch the option on.
        const int kill = addOption("kill", &gm::ActivateOptions::killProcessList, true, true, true,
            tr("Close background apps"),
            tr("Closes the apps on your list. They start normally afterward."));
        const int rename = addOption("renameExes", &gm::ActivateOptions::renameExecutables,
            false, false, false,
            tr("Keep closed apps from restarting"),
            tr("Renames their program files until Genesis turns off."),
            tr("Renames files"));
        const int freeze = addOption("suspendProcs", &gm::ActivateOptions::suspendProcessList,
            false, true, true,
            tr("Pause background apps"),
            tr("Freezes apps like Spotify instead of closing them."));
        const int services = addOption("stopServices", &gm::ActivateOptions::stopServices,
            false, true, true,
            tr("Pause background services"),
            tr("Stops indexing, diagnostics and Xbox helpers."));
        const int standby = addOption("standby", &gm::ActivateOptions::purgeStandbyList,
            true, true, true,
            tr("Free cached memory"),
            tr("Empties the Windows standby cache."));
        const int trim = addOption("trim", &gm::ActivateOptions::trimWorkingSets,
            true, true, true,
            tr("Trim background memory"),
            tr("Background processes hand back memory they are not using."));

        const int priority = addOption("registry", &gm::ActivateOptions::applyRegistryTweaks,
            false, true, true,
            tr("Prioritize the app in front"),
            tr("Gives the foreground app more CPU time."));
        const int throttle = addOption("powerThrottling", &gm::ActivateOptions::disablePowerThrottling,
            true, true, true,
            tr("Keep the app in front at full speed"),
            tr("Stops Windows slowing it down to save power."));
        const int timer = addOption("timer", &gm::ActivateOptions::requestTimerResolution,
            true, true, true,
            tr("High-resolution timer"),
            tr("A 0.5 ms timer for tighter frame and input timing."));
        const int mmcss = addOption("mmcss", &gm::ActivateOptions::enableMmcss,
            true, true, true,
            tr("Raise compositor priority"),
            tr("The desktop compositor gets scheduled first."));
        const int idle = addOption("processorIdle", &gm::ActivateOptions::disableProcessorIdle,
            false, false, true,
            tr("Keep the CPU awake"),
            tr("Turns off deep CPU sleep states for faster wake-ups."),
            tr("Runs warmer"));
        const int nvidia = addOption("nvidiaPerfMode", &gm::ActivateOptions::applyNvidiaPerfMode,
            false, false, true,
            tr("NVIDIA maximum performance"),
            tr("Holds NVIDIA graphics cards at full clocks."));

        const int notify = addOption("notifications", &gm::ActivateOptions::disableNotifications,
            false, true, true,
            tr("Silence notifications"),
            tr("Holds back Windows notifications and pop-ups."));
        const int privacy = addOption("privacy", &gm::ActivateOptions::applyPrivacyTweaks,
            false, false, true,
            tr("Pause data collection"),
            tr("Turns off diagnostics and suggested content."));

        const int explorer = addOption("suspendExplorer", &gm::ActivateOptions::suspendExplorer,
            false, false, true,
            tr("Close the desktop and taskbar"),
            tr("Gives a fullscreen game the screen to itself."),
            tr("No taskbar"));
        const int ghosts = addOption("ghostDevices", &gm::ActivateOptions::runGhostDeviceCleanup,
            false, false, false,
            tr("Remove disconnected devices"),
            tr("Clears entries for devices no longer plugged in."),
            tr("Permanent"));

        // The two-way switch between what Genesis does and which apps it does
        // it to.
        auto* switchRow = new QHBoxLayout;
        switchRow->setContentsMargins(0, tokens::Space4, 0, 0);
        switchRow->setSpacing(tokens::CardMargin);
        auto* track = new QFrame(this);
        track->setObjectName(QStringLiteral("segmentTrack"));
        track->setAttribute(Qt::WA_StyledBackground, true);
        auto* trackLay = new QHBoxLayout(track);
        trackLay->setContentsMargins(tokens::Space2, tokens::Space2, tokens::Space2, tokens::Space2);
        trackLay->setSpacing(tokens::Space2);
        const auto segment = [track, trackLay](const QString& text) {
            auto* b = new QPushButton(text, track);
            b->setProperty("uiRole", QStringLiteral("segment"));
            b->setCheckable(true);
            b->setAutoExclusive(true);
            b->setCursor(Qt::PointingHandCursor);
            // Keyboard focus only, so a click does not leave a ring behind.
            b->setFocusPolicy(Qt::TabFocus);
            trackLay->addWidget(b);
            return b;
        };
        auto* optionsSeg = segment(tr("Options"));
        appsSeg_ = segment(tr("Apps"));
        optionsSeg->setChecked(true);
        switchRow->addWidget(track, 0, Qt::AlignLeft | Qt::AlignVCenter);
        switchRow->addStretch(1);
        stateChip_ = makeChip(QString(), nullptr, this);
        switchRow->addWidget(stateChip_, 0, Qt::AlignVCenter);
        outer->addLayout(switchRow);

        const auto scrolled = [this](QWidget* body) {
            auto* scroll = new QScrollArea(this);
            scroll->setObjectName(QStringLiteral("settingsScroll"));
            scroll->setWidgetResizable(true);
            scroll->setFrameShape(QFrame::NoFrame);
            scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            body->setObjectName(QStringLiteral("scrollContentHost"));
            scroll->setWidget(body);
            return scroll;
        };

        auto* stack = new QStackedWidget(this);

        // Options page: two columns of equal length.
        auto* optionsPage = new QWidget;
        auto* columns = new QHBoxLayout(optionsPage);
        columns->setContentsMargins(0, 0, 0, 0);
        columns->setSpacing(tokens::CardMargin);
        auto* left = new QVBoxLayout;
        left->setContentsMargins(0, 0, 0, 0);
        left->setSpacing(tokens::CardMargin);
        left->addWidget(buildOptionGroup(QStringLiteral("layers"), tr("Free up resources"),
                                         { kill, rename, freeze, services, standby, trim }));
        left->addWidget(buildOptionGroup(QStringLiteral("bell"), tr("Fewer interruptions"),
                                         { notify, privacy }));
        left->addStretch(1);
        auto* right = new QVBoxLayout;
        right->setContentsMargins(0, 0, 0, 0);
        right->setSpacing(tokens::CardMargin);
        right->addWidget(buildOptionGroup(QStringLiteral("zap"), tr("Performance"),
                                          { priority, throttle, timer, mmcss, idle, nvidia }));
        right->addWidget(buildOptionGroup(QStringLiteral("adjustments"), tr("Advanced"),
                                          { explorer, ghosts }));
        right->addStretch(1);
        columns->addLayout(left, 1);
        columns->addLayout(right, 1);
        stack->addWidget(scrolled(optionsPage));

        // Apps page: the two lists side by side.
        const QStringList floor = vth::win::processes::protectedNames();

        closeEditor_ = new vth::widgets::NameListEditor(
            tr("Add an app, like Discord.exe"),
            tr("Nothing to close yet."));
        closeEditor_->setDefaults(gm::defaultCloseList());
        closeEditor_->setRejected(floor, tr("Windows needs %1, so Genesis leaves it running."));
        connect(closeEditor_, &vth::widgets::NameListEditor::changed, this, [this]() {
            gm::saveCloseList(closeEditor_->names());
            refreshSummary();
        });

        exceptionEditor_ = new vth::widgets::NameListEditor(
            tr("Add an app to keep open"),
            tr("No exceptions yet."));
        exceptionEditor_->setDefaults(gm::defaultExceptions());
        exceptionEditor_->setRejected(floor, tr("%1 is always kept open."));
        connect(exceptionEditor_, &vth::widgets::NameListEditor::changed, this, [this]() {
            gm::saveExceptions(exceptionEditor_->names());
        });

        // The protected floor, collapsed.
        auto* floorBox = new QWidget;
        auto* floorLay = new QVBoxLayout(floorBox);
        floorLay->setContentsMargins(0, 0, 0, 0);
        floorLay->setSpacing(tokens::TightGap);
        auto* floorToggle = new QToolButton(floorBox);
        floorToggle->setProperty("uiRole", QStringLiteral("link"));
        floorToggle->setCursor(Qt::PointingHandCursor);
        floorToggle->setText(tr("Plus %1 Windows processes, always").arg(floor.size()));
        floorLay->addWidget(floorToggle, 0, Qt::AlignLeft);
        auto* floorNames = new QLabel(floor.join(QStringLiteral(", ")), floorBox);
        floorNames->setObjectName(QStringLiteral("settingsRowDesc"));
        floorNames->setWordWrap(true);
        floorNames->setTextInteractionFlags(Qt::TextSelectableByMouse);
        floorNames->hide();
        floorLay->addWidget(floorNames);
        connect(floorToggle, &QToolButton::clicked, floorNames,
                [floorNames] { floorNames->setVisible(!floorNames->isVisible()); });

        auto* appsPage = new QWidget;
        auto* lists = new QHBoxLayout(appsPage);
        lists->setContentsMargins(0, 0, 0, 0);
        lists->setSpacing(tokens::CardMargin);
        lists->addWidget(buildListCard(QStringLiteral("ban"), tr("Close or pause"),
            tr("Out of the way while Genesis is on."), closeEditor_), 1, Qt::AlignTop);
        lists->addWidget(buildListCard(QStringLiteral("shield-check"), tr("Always keep open"),
            tr("Genesis never touches these."), exceptionEditor_, floorBox), 1, Qt::AlignTop);
        stack->addWidget(scrolled(appsPage));

        connect(optionsSeg, &QPushButton::toggled, stack, [stack](bool on) {
            if (on) stack->setCurrentIndex(0);
        });
        connect(appsSeg_, &QPushButton::toggled, stack, [stack](bool on) {
            if (on) stack->setCurrentIndex(1);
        });
        outer->addWidget(stack, 1);

        summaryLabel_ = new QLabel(this);
        summaryLabel_->setProperty("uiRole", QStringLiteral("metaText"));
        summaryLabel_->setWordWrap(true);
        outer->addWidget(summaryLabel_);
    }

    QWidget* GenesisView::buildOptionGroup(const QString& icon, const QString& title,
                                           const QVector<int>& optionIndexes) {
        auto* card = new QFrame;
        card->setProperty("uiRole", QStringLiteral("card"));
        card->setAttribute(Qt::WA_StyledBackground, true);
        auto* col = new QVBoxLayout(card);
        col->setContentsMargins(tokens::Space4, tokens::Space12, tokens::Space4, tokens::Space4);
        col->setSpacing(0);

        auto* head = new QHBoxLayout;
        head->setContentsMargins(tokens::RowInsetH, 0, tokens::RowInsetH, tokens::Space4);
        head->setSpacing(tokens::Space8);
        head->addWidget(new vth::widgets::GlyphLabel(icon, &pal::accentOnSurface,
                                                     tokens::GlyphMd, card), 0, Qt::AlignVCenter);
        auto* name = new QLabel(title, card);
        name->setProperty("uiRole", QStringLiteral("sectionTitle"));
        head->addWidget(name, 1, Qt::AlignVCenter);
        col->addLayout(head);

        bool first = true;
        for (int i : optionIndexes) {
            if (!first) {
                auto* rule = new QFrame(card);
                rule->setObjectName(QStringLiteral("srRowDivider"));
                auto* ruleHolder = new QHBoxLayout;
                ruleHolder->setContentsMargins(tokens::RowInsetH, 0, tokens::RowInsetH, 0);
                ruleHolder->addWidget(rule);
                col->addLayout(ruleHolder);
            }
            first = false;
            optionRows_.at(i)->setParent(card);
            col->addWidget(optionRows_.at(i));
        }
        return card;
    }

    QWidget* GenesisView::buildListCard(const QString& icon, const QString& title,
                                        const QString& caption, QWidget* editor,
                                        QWidget* footer) {
        auto* card = new QFrame;
        card->setProperty("uiRole", QStringLiteral("card"));
        card->setAttribute(Qt::WA_StyledBackground, true);
        auto* col = new QVBoxLayout(card);
        col->setContentsMargins(tokens::CardInsetH, tokens::Space12,
                                tokens::CardInsetH, tokens::CardInsetH);
        col->setSpacing(tokens::Space8);

        auto* head = new QHBoxLayout;
        head->setContentsMargins(0, 0, 0, 0);
        head->setSpacing(tokens::Space8);
        head->addWidget(new vth::widgets::GlyphLabel(icon, &pal::accentOnSurface,
                                                     tokens::GlyphMd, card), 0, Qt::AlignVCenter);
        auto* name = new QLabel(title, card);
        name->setProperty("uiRole", QStringLiteral("sectionTitle"));
        head->addWidget(name, 1, Qt::AlignVCenter);
        col->addLayout(head);

        auto* cap = new QLabel(caption, card);
        cap->setObjectName(QStringLiteral("settingsRowDesc"));
        cap->setWordWrap(true);
        col->addWidget(cap);

        editor->setParent(card);
        col->addWidget(editor);
        if (footer) {
            footer->setParent(card);
            col->addWidget(footer);
        }
        return card;
    }

    void GenesisView::applyPreset(Preset p) {
        if (p == Preset::Custom)
            return;
        const int column = static_cast<int>(p);
        for (const Option& o : options_) {
            const QSignalBlocker block(o.toggle);
            o.toggle->setChecked(o.inPreset[column]);
        }
        persistOptions();
        refreshSummary();
    }

    GenesisView::Preset GenesisView::currentPreset() const {
        for (int column = 0; column < 3; ++column) {
            bool same = true;
            for (const Option& o : options_) {
                if (o.toggle->isChecked() != o.inPreset[column]) {
                    same = false;
                    break;
                }
            }
            if (same)
                return static_cast<Preset>(column);
        }
        return Preset::Custom;
    }

    void GenesisView::refreshSummary() {
        const Preset p = currentPreset();
        for (int i = 0; i < 3; ++i)
            if (presetTiles_[i])
                presetTiles_[i]->setSelected(static_cast<int>(p) == i);

        int on = 0;
        for (const Option& o : options_)
            if (o.toggle->isChecked())
                ++on;

        const QString count = tr("%1 of %2 options on").arg(on).arg(options_.size());
        QString name;
        switch (p) {
        case Preset::Light:    name = tr("Light");    break;
        case Preset::Balanced: name = tr("Balanced"); break;
        case Preset::Full:     name = tr("Full");     break;
        case Preset::Custom:   name = tr("Your own mix"); break;
        }
        const QString session = state_.active
            ? tr("Changes apply the next time you turn Genesis on.")
            : tr("Saved as you go.");
        const QString dot = QStringLiteral("  ") + QChar(0x00B7) + QStringLiteral("  ");
        summaryLabel_->setText(name + dot + count + dot + session);

        if (appsSeg_ && closeEditor_) {
            const int apps = closeEditor_->names().size();
            appsSeg_->setText(apps > 0 ? tr("Apps (%1)").arg(apps) : tr("Apps"));
        }
    }

    void GenesisView::renderState() {
        // Shown only while a session runs.
        stateChip_->setText(tr("Running now"));
        stateChip_->setProperty("tone", QStringLiteral("good"));
        stateChip_->setVisible(state_.active);
        repolish(stateChip_);
        refreshSummary();
    }

    void GenesisView::persistOptions() const {
        auto* settings = ServiceRegistry::instance().get<vth::Settings>();
        if (!settings) return;
        for (const Option& o : options_)
            settings->setValue(QStringLiteral("genesis/opts/") + QString::fromLatin1(o.key),
                               o.toggle->isChecked());
        settings->sync();
    }

    // Signals blocked while these land: every switch persists the whole set on
    // toggled(), so restoring saved values would otherwise write once per switch.
    void GenesisView::applyOptions(const gm::ActivateOptions& opts) {
        for (const Option& o : options_) {
            const QSignalBlocker block(o.toggle);
            o.toggle->setChecked(opts.*(o.field));
        }
        refreshSummary();
    }
}
