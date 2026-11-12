#include "pch.h"
#include "NvramView.h"

#include "../Dialogs/Confirm.h"
#include "../Dialogs/ToolModal.h"
#include "../Models/NvramEntryModel.h"
#include "../Pages/SystemTools.h"
#include "../Theme/DesignTokens.h"
#include "../Utils/TablerIcon.h"
#include "../Widgets/Callout.h"
#include "../Widgets/CommandButton.h"
#include "../Widgets/DisabledReason.h"
#include "../Widgets/Dropdown.h"
#include "../Widgets/EmptyState.h"
#include "../Widgets/ListContextMenu.h"
#include "../Widgets/ListView.h"
#include "../Widgets/PanelDialog.h"
#include "../Widgets/RainbowButton.h"
#include "../Widgets/ScrimDialog.h"

#include "../../vth_core/Models/ProductTier.h"
#include "../../vth_core/Runtime/Paths.h"
#include "../../vth_core/Runtime/ServiceRegistry.h"
#include "../../vth_core/Runtime/Settings.h"

#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QSaveFile>
#include <QSignalBlocker>
#include <QSortFilterProxyModel>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace vth::ui
{
    namespace tokens  = vth::theme::tokens;
    namespace pal     = vth::theme::palette;
    namespace nv      = vth::win::nvram;
    namespace presets = vth::win::nvram::presets;

    namespace
    {
        // Typed into the confirmation. Not through tr(), because a translator rendering
        // it differently in the prompt than the comparison would lock the feature out.
        const QLatin1String kWriteWord("WRITE");

        // Tall enough for about ten changed settings before the list scrolls.
        constexpr int kChangeListHeight = 180;

        QString exportPath()
        {
            return vth::paths::userCache() + QStringLiteral("/nvram-current.txt");
        }

        QString applyPath()
        {
            return vth::paths::userCache() + QStringLiteral("/nvram-apply.txt");
        }

        struct ReadResult { bool ok = false; QString error; nv::Document doc; };

        // worker: read the firmware and save the first copy before anything can change
        ReadResult readFirmware(const nv::ScewinPaths& tool, const QString& in)
        {
            ReadResult r;
            vth::paths::userCache();          // created on first call
            QFile::remove(in);
            const nv::RunResult run = nv::exportSnapshot(tool, in);
            if (!run.success) {
                r.error = run.message.isEmpty()
                    ? QObject::tr("AMISCE did not report why the read failed.")
                    : run.message;
                return r;
            }
            r.doc = nv::parseFile(in);
            if (r.doc.entries.isEmpty()) {
                r.error = QObject::tr(
                    "AMISCE produced a file with no settings in it. On most "
                    "boards this means HII data is not published; on ASUS "
                    "boards there is a \"Publish HII Resources\" switch in "
                    "the BIOS that has to be on.");
                return r;
            }
            // Never overwriting an existing one: this copy has to predate every
            // write Vethida makes.
            nv::saveBaselineIfAbsent(in);
            r.ok = true;
            return r;
        }

        // Worker side. Writes the file AMISCE imports. Empty on success.
        QString stage(const nv::Document& doc, const QString& out)
        {
            QFile::remove(out);
            QSaveFile f(out);
            if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
                return QObject::tr("Vethida could not write the file AMISCE reads. "
                                   "Nothing was changed.");
            f.write(nv::serialize(doc).toUtf8());
            if (!f.commit())
                return QObject::tr("Vethida could not finish writing the file "
                                   "AMISCE reads. Nothing was changed.");
            return {};
        }

        // What an entry held when it was read, as the editor shows it.
        QString originalDisplay(const nv::Entry& e)
        {
            if (e.valueKind != nv::ValueKind::Options)
                return e.originalValue;
            for (const nv::Option& o : e.options)
                if (o.code == e.originalValue)
                    return o.label.isEmpty() ? o.code : o.label;
            return e.originalValue;
        }

        // Only the kinds whose format the writer knows. A Raw value is round-
        // tripped as literal text, and a hand-typed one could be anything.
        bool editableByHand(const nv::Entry& e)
        {
            return !e.isCommented && e.valueKind != nv::ValueKind::Raw;
        }
    }

    NvramView::NvramView(QWidget* parent) : QWidget(parent)
    {
        setObjectName(QStringLiteral("NvramView"));
        buildUi();
        evaluateEnvironment();
    }

    NvramView::~NvramView()
    {
        token_.cancel();
    }

    void NvramView::buildUi()
    {
        root_ = new QVBoxLayout(this);
        root_->setContentsMargins(0, 0, 0, 0);
        root_->setSpacing(tokens::CardMargin);

        // Hidden only on firmware this tool can't read at all.
        hazard_ = vth::widgets::makeCallout(
            this, QStringLiteral("triangle-alert"),
            tr("Risk of an unbootable PC"),
            {
                tr("These buttons change your motherboard's firmware settings, "
                   "including ones the BIOS setup screen does not show. A value "
                   "your board does not tolerate can leave it unable to start."),
                tr("Recovery is \"Restore first-seen settings\" below while the PC "
                   "still boots, or clearing CMOS on the board (or a BIOS "
                   "flashback) when it does not. Know which your motherboard "
                   "supports before you apply anything."),
            },
            QStringLiteral("critical"), &pal::criticalText);
        root_->addWidget(hazard_);

        busy_ = new EmptyState(this);
        busy_->setBusy(true);
        busy_->hide();
        root_->addWidget(busy_);

        root_->addStretch(1);
    }

    // The pane holds either the controls at the top or a single centered message.
    void NvramView::showCentered(bool on)
    {
        busy_->setVisible(on);
        root_->setStretchFactor(busy_, on ? 1 : 0);
        root_->setStretch(root_->count() - 1, on ? 0 : 1);
    }

    void NvramView::showUnavailable(const QString& title, const QString& reason)
    {
        busy_->setBusy(false);
        busy_->setTitle(title);
        busy_->setSubtitle(reason);
        showCentered(true);
    }

    void NvramView::evaluateEnvironment()
    {
        if (hazard_)
            hazard_->hide();
        busy_->setBusy(true);
        showCentered(true);

        struct Env { bool ami = false; nv::ScewinPaths tool; presets::Platform cpu{}; };
        token_ = vth::fetch<Env>(
            this,
            []() {
                Env e;
                // Firmware vendor first. Loading AMI's kernel drivers on a board
                // they cannot serve is not something to find out by trying.
                e.ami = nv::isAmiFirmware();
                if (e.ami)
                    e.tool = nv::locateScewin();
                e.cpu = presets::detectPlatform();
                return e;
            },
            [this](Env e) {
                tool_     = e.tool;
                platform_ = e.cpu;
                if (!e.ami)
                    setState(State::Unsupported);
                else if (!e.tool.available)
                    setState(State::NoTool);
                else
                    setState(State::Ready);
            });
    }

    void NvramView::setState(State s, const QString& detail)
    {
        state_ = s;

        if (notice_) {
            root_->removeWidget(notice_);
            notice_->deleteLater();
            notice_ = nullptr;
        }

        // Owned by the panel just deleted.
        optimalBtn_ = nullptr;
        allBtn_     = nullptr;
        editBtn_    = nullptr;
        restoreBtn_ = nullptr;

        // Nothing this PC can use. A processor the presets don't cover isn't this case,
        // because the hand editor works on any AMI board.
        if (s == State::Unsupported) {
            if (hazard_)
                hazard_->hide();
            showUnavailable(
                tr("BIOS settings are not available on this PC"),
                tr("Vethida's firmware settings are for AMI firmware, which most "
                   "desktop motherboards use. This PC's firmware is from another "
                   "maker."));
            return;
        }

        showCentered(false);
        if (hazard_)
            hazard_->show();

        auto* panel = new QFrame(this);
        panel->setProperty("uiRole", QStringLiteral("card"));
        panel->setAttribute(Qt::WA_StyledBackground, true);
        auto* col = new QVBoxLayout(panel);
        col->setContentsMargins(tokens::CardInsetH, tokens::CardInsetV,
                                tokens::CardInsetH, tokens::CardInsetV);
        col->setSpacing(tokens::CardMargin);

        const auto addText = [&](const QString& text, const char* role = "bodyText") {
            auto* l = new QLabel(text, panel);
            l->setProperty("uiRole", QString::fromLatin1(role));
            l->setWordWrap(true);
            col->addWidget(l);
        };

        auto* actions = new QHBoxLayout;
        actions->setContentsMargins(0, 0, 0, 0);
        actions->setSpacing(tokens::CompactGap);

        switch (s) {
        case State::Unsupported:
            break;

        case State::NoTool: {
            addText(tr("Vethida could not find the firmware tool it ships with. "
                       "A repair install puts it back."));
            addText(tr("You can also point Vethida at another copy: the folder "
                       "holding SCEWIN_64.exe, amifldrv64.sys and "
                       "amigendrv64.sys together."));
            if (!detail.isEmpty())
                addText(detail);
            auto* choose = new vth::widgets::CommandButton(
                vth::widgets::CommandButton::Variant::Primary,
                tr("Choose another folder"), panel);
            connect(choose, &QPushButton::clicked, this, &NvramView::onChooseToolFolder);
            actions->addWidget(choose);
            break;
        }

        case State::Ready: {
            if (platform_ == presets::Platform::Unknown) {
                col->addWidget(vth::widgets::makeDisabledReason(
                    panel, tr("Vethida's firmware tweaks cover Intel and AMD "
                              "processors, and this one is neither.")));
            } else {
                const QString cpu = presets::platformName(platform_);
                addText(tr("Vethida reads your firmware settings, sets every one "
                           "on its %1 list that your board has, and writes them "
                           "back. You see exactly what changes before anything is "
                           "written.").arg(cpu));
                addText(tr("Optimal turns off power saving: C-states, link power "
                           "management, clock gating, spread spectrum and wake "
                           "sources (%1 settings).")
                            .arg(presets::countFor(platform_, presets::Level::Optimal)),
                        "metaText");
                addText(tr("Everything adds the ones that remove a safety net: "
                           "Spectre and memory encryption, IOMMU, virtualization "
                           "and sleep states (%1 settings in all). Thermal "
                           "protection and memory error correction always stay "
                           "on. Only for a machine you can recover by hand.")
                            .arg(presets::countFor(platform_, presets::Level::Everything)),
                        "metaText");

                optimalBtn_ = new vth::widgets::CommandButton(
                    vth::widgets::CommandButton::Variant::Primary,
                    tr("Apply optimal settings"), panel);
                connect(optimalBtn_, &QPushButton::clicked, this,
                        [this] { onApply(presets::Level::Optimal); });
                actions->addWidget(optimalBtn_, 0, Qt::AlignVCenter);

                allBtn_ = new vth::widgets::RainbowButton(
                    tr("Apply everything (incl. dangerous)"), panel);
                connect(allBtn_, &QPushButton::clicked, this,
                        [this] { onApply(presets::Level::Everything); });
                actions->addWidget(allBtn_, 0, Qt::AlignVCenter);
            }

            editBtn_ = new vth::widgets::CommandButton(
                platform_ == presets::Platform::Unknown
                    ? vth::widgets::CommandButton::Variant::Primary
                    : vth::widgets::CommandButton::Variant::Secondary,
                tr("Edit settings by hand"), panel);
            connect(editBtn_, &QPushButton::clicked, this, &NvramView::onEditByHand);
            actions->addWidget(editBtn_, 0, Qt::AlignVCenter);

            addText(nv::hasBaseline()
                ? tr("Vethida holds a copy of the settings it first saw on this PC.")
                : tr("The first read is saved as a reference copy you can restore."),
                "metaText");

            actions->addStretch(1);
            if (nv::hasBaseline()) {
                restoreBtn_ = new vth::widgets::CommandButton(
                    vth::widgets::CommandButton::Variant::Secondary,
                    tr("Restore first-seen settings"), panel);
                connect(restoreBtn_, &QPushButton::clicked,
                        this, &NvramView::onRestoreBaseline);
                actions->addWidget(restoreBtn_, 0, Qt::AlignVCenter);
            }
            break;
        }
        }

        if (actions->count() > 0) {
            if (s != State::Ready)
                actions->addStretch(1);
            col->addLayout(actions);
        } else {
            delete actions;
        }

        notice_ = panel;
        root_->insertWidget(root_->count() - 1, notice_);
    }

    void NvramView::onChooseToolFolder()
    {
        const QString dir = QFileDialog::getExistingDirectory(
            this, tr("Select the folder containing SCEWIN_64.exe"), QDir::homePath());
        if (dir.isEmpty())
            return;

        auto* settings = vth::ServiceRegistry::instance().get<vth::Settings>();
        if (!settings) {
            setState(State::NoTool, tr("Vethida could not save that location."));
            return;
        }
        settings->setValue(QStringLiteral("nvram/toolDirectory"), dir);

        const nv::ScewinPaths found = nv::locateScewin();
        if (!found.available) {
            setState(State::NoTool,
                tr("That folder is missing at least one of SCEWIN_64.exe, "
                   "amifldrv64.sys or amigendrv64.sys."));
            return;
        }
        tool_ = found;
        setState(State::Ready);
    }

    void NvramView::setBusy(bool busy, QPushButton* source)
    {
        // Each by its own type: setLoading is not virtual, and the two button
        // classes each declare their own.
        const auto mark = [busy, source](auto* button) {
            if (!button)
                return;
            if (button == source)
                button->setLoading(busy);
            else
                button->setEnabled(!busy);
        };
        mark(optimalBtn_);
        mark(allBtn_);
        mark(editBtn_);
        mark(restoreBtn_);
    }

    void NvramView::onApply(presets::Level level)
    {
        if (!vth::ui::tools::ensureTier(this, vth::models::ProductTier::Ultimate))
            return;

        // Guarded: the panel that owns the button is rebuilt by setState.
        const QPointer<QPushButton> source = level == presets::Level::Optimal
            ? static_cast<QPushButton*>(optimalBtn_)
            : static_cast<QPushButton*>(allBtn_);
        setBusy(true, source);

        const nv::ScewinPaths   tool = tool_;
        const presets::Platform cpu  = platform_;
        const QString in  = exportPath();
        const QString out = applyPath();

        struct Result { bool ok = false; QString error; presets::Plan plan; };

        // Read, plan and stage in one pass. Nothing reaches the chip here; the
        // staged file is only imported after the user has seen the plan.
        token_ = vth::fetch<Result>(
            this,
            [tool, cpu, level, in, out]() {
                Result r;
                ReadResult read = readFirmware(tool, in);
                if (!read.ok) {
                    r.error = read.error;
                    return r;
                }
                QFile::remove(out);
                r.plan = presets::apply(read.doc, cpu, level);
                if (!r.plan.changes.isEmpty()) {
                    r.error = stage(read.doc, out);
                    if (!r.error.isEmpty())
                        return r;
                }
                r.ok = true;
                return r;
            },
            [this, level, out, source](Result r) {
                if (!r.ok) {
                    setBusy(false, source);
                    vth::dialogs::warn(this, tr("Firmware settings not read"), r.error);
                    return;
                }
                if (r.plan.changes.isEmpty()) {
                    setBusy(false, source);
                    vth::dialogs::inform(this, tr("Nothing to change"),
                        tr("Every setting on the list that this board has is already "
                           "at the value Vethida recommends (%1 already set, %2 not on this "
                           "board).").arg(r.plan.alreadySet).arg(r.plan.notOnBoard));
                    setState(State::Ready);
                    return;
                }
                // The baseline is written above, so this only fires if it couldn't be.
                if (!nv::hasBaseline()) {
                    setBusy(false, source);
                    vth::dialogs::warn(this, tr("No reference copy"),
                        tr("Vethida could not save a copy of this PC's original "
                           "firmware settings, so there would be nothing to go back "
                           "to. Nothing was changed."));
                    return;
                }
                const bool everything = level == presets::Level::Everything;
                if (!confirmChanges(everything ? tr("Apply every firmware tweak")
                                               : tr("Apply optimal firmware settings"),
                                    r.plan.changes,
                                    r.plan.notOnBoard + r.plan.noSuchValue,
                                    everything)) {
                    setBusy(false, source);
                    return;
                }
                writePlan(out);
            });
    }

    void NvramView::onEditByHand()
    {
        if (!vth::ui::tools::ensureTier(this, vth::models::ProductTier::Ultimate))
            return;

        const QPointer<QPushButton> source = editBtn_;
        setBusy(true, source);

        const nv::ScewinPaths tool = tool_;
        const QString in  = exportPath();
        const QString out = applyPath();

        token_ = vth::fetch<ReadResult>(
            this,
            [tool, in]() { return readFirmware(tool, in); },
            [this, out, source](ReadResult r) {
                if (!r.ok) {
                    setBusy(false, source);
                    vth::dialogs::warn(this, tr("Firmware settings not read"), r.error);
                    return;
                }
                // The same refusal as onApply: no write without a way back.
                if (!nv::hasBaseline()) {
                    setBusy(false, source);
                    vth::dialogs::warn(this, tr("No reference copy"),
                        tr("Vethida could not save a copy of this PC's original "
                           "firmware settings, so there would be nothing to go back "
                           "to. Nothing was changed."));
                    return;
                }
                if (!editDocument(r.doc)) {
                    setBusy(false, source);
                    return;
                }

                // Only what serialize() writes: a commented entry never reaches
                // the file, so it is never listed as a change either.
                QVector<presets::Change> changes;
                for (const nv::Entry& e : r.doc.entries)
                    if (!e.isCommented && e.isModified())
                        changes.push_back({ e.setupQuestion, originalDisplay(e),
                                            e.currentDisplayValue(), false });
                if (changes.isEmpty()) {
                    setBusy(false, source);
                    return;
                }
                // A typed word every time: a value set by hand is one no list
                // has vouched for.
                if (!confirmChanges(tr("Write your changes"), changes, 0, true)) {
                    setBusy(false, source);
                    return;
                }

                token_ = vth::fetch<QString>(
                    this,
                    [doc = std::move(r.doc), out]() { return stage(doc, out); },
                    [this, out, source](QString error) {
                        if (!error.isEmpty()) {
                            setBusy(false, source);
                            vth::dialogs::warn(this, tr("Firmware settings not written"),
                                               error);
                            return;
                        }
                        writePlan(out);
                    });
            });
    }

    bool NvramView::editDocument(nv::Document& doc)
    {
        namespace m = vth::ui::models;

        auto* content = new QWidget;
        auto* col = new QVBoxLayout(content);
        col->setContentsMargins(0, 0, 0, 0);
        col->setSpacing(tokens::CardMargin);

        auto* search = new QLineEdit(content);
        search->setPlaceholderText(tr("Search settings"));
        search->setAccessibleName(tr("Search settings"));
        search->setClearButtonEnabled(true);
        search->setMinimumWidth(tokens::FilterControlWidth);
        col->addWidget(search, 0, Qt::AlignLeft);

        auto* model = new m::NvramEntryModel(&doc, content);
        auto* proxy = new QSortFilterProxyModel(content);
        proxy->setSourceModel(model);
        proxy->setFilterRole(m::NvramEntryModel::SearchRole);
        proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
        connect(search, &QLineEdit::textChanged,
                proxy, &QSortFilterProxyModel::setFilterFixedString);

        auto* view = new vth::widgets::ListView(content);
        // The firmware's order is the order.
        view->setSortingEnabled(false);
        view->setModel(proxy);
        view->header()->setSectionResizeMode(m::NvramEntryModel::ColSetting,
                                             QHeaderView::Stretch);
        view->header()->setSectionResizeMode(m::NvramEntryModel::ColValue,
                                             QHeaderView::ResizeToContents);
        listmenu::installShortcuts(view, listmenu::Hooks{
            .onQuickFilter = [search] { search->setFocus(Qt::ShortcutFocusReason); },
        });
        col->addWidget(new vth::widgets::ListCard(view, content), 1);

        // The selected setting, and the one control that sets it.
        auto* editor = new QFrame(content);
        editor->setProperty("uiRole", QStringLiteral("card"));
        editor->setAttribute(Qt::WA_StyledBackground, true);
        auto* ed = new QVBoxLayout(editor);
        ed->setContentsMargins(tokens::CardInsetH, tokens::CardInsetV,
                               tokens::CardInsetH, tokens::CardInsetV);
        ed->setSpacing(tokens::CompactGap);

        auto* name = new QLabel(editor);
        name->setProperty("uiRole", QStringLiteral("sectionTitle"));
        name->setWordWrap(true);
        ed->addWidget(name);

        auto* help = new QLabel(editor);
        help->setProperty("uiRole", QStringLiteral("bodyText"));
        help->setWordWrap(true);
        ed->addWidget(help);

        auto* original = new QLabel(editor);
        original->setProperty("uiRole", QStringLiteral("metaText"));
        original->setWordWrap(true);
        ed->addWidget(original);

        // The same patterns the parser reads back, so a value typed here is
        // one the file it is written to still parses as.
        auto* numeric = new QRegularExpressionValidator(
            QRegularExpression(QStringLiteral(R"(\d+)")), editor);
        auto* text = new QRegularExpressionValidator(
            QRegularExpression(QStringLiteral(R"([^"]*)")), editor);

        auto* stack  = new QStackedWidget(editor);
        auto* choice = new Dropdown(stack);
        auto* field  = new QLineEdit(stack);
        auto* locked = vth::widgets::makeDisabledReason(stack, QString());
        stack->addWidget(choice);
        stack->addWidget(field);
        stack->addWidget(locked);
        ed->addWidget(stack);
        col->addWidget(editor);
        editor->hide();

        auto* footer = new QHBoxLayout;
        footer->setContentsMargins(0, 0, 0, 0);
        footer->setSpacing(tokens::CompactGap);
        auto* count = new QLabel(content);
        count->setProperty("uiRole", QStringLiteral("metaText"));
        footer->addWidget(count, 1, Qt::AlignVCenter);
        auto* review = new vth::widgets::CommandButton(
            vth::widgets::CommandButton::Variant::Primary, tr("Review changes"), content);
        review->setMinimumWidth(tokens::ButtonMinWidth);
        footer->addWidget(review, 0, Qt::AlignVCenter);
        col->addLayout(footer);

        const auto current = [view, proxy] {
            return proxy->mapToSource(view->currentIndex())
                .siblingAtColumn(m::NvramEntryModel::ColValue);
        };

        const auto showCurrent = [=] {
            const QModelIndex idx = current();
            const nv::Entry* e = model->entryAt(idx);
            editor->setVisible(e != nullptr);
            if (!e)
                return;
            name->setText(e->setupQuestion);
            help->setText(e->helpString);
            help->setVisible(!e->helpString.isEmpty());
            original->setText(tr("Read from the firmware: %1").arg(originalDisplay(*e)));

            if (!editableByHand(*e)) {
                vth::widgets::setDisabledReason(locked, e->isCommented
                    ? tr("Your firmware does not accept a write for this setting.")
                    : tr("Vethida cannot tell what format this value takes, so it "
                         "does not change it."));
                stack->setCurrentWidget(locked);
            } else if (e->valueKind == nv::ValueKind::Options) {
                choice->setItems(model->data(idx, m::NvramEntryModel::OptionsRole)
                                     .toStringList());
                choice->setCurrentIndex(model->data(idx, m::NvramEntryModel::SelectedRole)
                                            .toInt());
                choice->setAccessibleName(e->setupQuestion);
                stack->setCurrentWidget(choice);
            } else {
                const QSignalBlocker block(field);
                field->setValidator(e->valueKind == nv::ValueKind::Numeric
                                        ? static_cast<const QValidator*>(numeric)
                                        : text);
                field->setText(e->value);
                field->setAccessibleName(e->setupQuestion);
                stack->setCurrentWidget(field);
            }
        };

        const auto refreshCount = [&doc, count, review] {
            int n = 0;
            for (const nv::Entry& e : doc.entries)
                if (!e.isCommented && e.isModified())
                    ++n;
            count->setText(n == 0 ? tr("No settings changed.")
                         : n == 1 ? tr("One setting changed.")
                                  : tr("%1 settings changed.").arg(n));
            review->setEnabled(n > 0);
        };
        refreshCount();

        connect(view->selectionModel(), &QItemSelectionModel::currentChanged,
                content, showCurrent);
        connect(model, &QAbstractItemModel::dataChanged, content, refreshCount);
        connect(choice, &Dropdown::currentIndexChanged, content, [=](int i) {
            if (i >= 0)
                model->setData(current(), i, Qt::EditRole);
        });
        connect(field, &QLineEdit::textEdited, content, [=] {
            // An empty number is a field half typed, not a value to write.
            if (field->hasAcceptableInput())
                model->setData(current(), field->text(), Qt::EditRole);
        });

        if (proxy->rowCount() > 0)
            view->setCurrentIndex(proxy->index(0, m::NvramEntryModel::ColSetting));

        // showContentModal owns the dialog and offers only a close, so the
        // write path accepts the panel it sits in, found as its ancestor.
        bool write = false;
        connect(review, &QPushButton::clicked, content, [content, &write] {
            for (QWidget* w = content->parentWidget(); w; w = w->parentWidget()) {
                if (auto* dlg = qobject_cast<QDialog*>(w)) {
                    write = true;
                    dlg->accept();
                    return;
                }
            }
        });

        // ScrimPanel: a surface with controls on it. Self: the list scrolls.
        vth::dialogs::showContentModal(this, tr("Edit firmware settings"), content,
                                       vth::widgets::PanelDialog::Mode::ScrimPanel,
                                       vth::dialogs::ModalScroll::Self);
        return write;
    }

    bool NvramView::confirmChanges(const QString& title,
                                   const QVector<presets::Change>& changes,
                                   int skipped, bool requireTypedWord)
    {
        const int n = changes.size();

        QString body = (n == 1
                ? tr("One setting is written to the firmware chip on your motherboard.")
                : tr("%1 settings are written to the firmware chip on your "
                     "motherboard.").arg(n))
            + QStringLiteral(" ")
            + tr("If your board does not accept a value the PC will not start, and "
                 "recovery is clearing CMOS or a BIOS flashback.");
        if (skipped > 0)
            body += QStringLiteral("\n\n")
                 + tr("%1 settings on the list are not on this board and are "
                      "skipped.").arg(skipped);

        QFrame* card = vth::dialogs::makePromptCard(
            title, body, vth::dialogs::PromptTone::Warning);
        auto* col = static_cast<QVBoxLayout*>(card->layout());

        // old -> new, dangerous ones marked with a word
        QStringList lines;
        for (const presets::Change& c : changes)
            lines << QStringLiteral("%1%2: %3 -> %4")
                         .arg(c.dangerous ? tr("[Risky] ") : QString(),
                              c.setting, c.from, c.to);
        auto* list = new QPlainTextEdit(lines.join(QLatin1Char('\n')), card);
        list->setReadOnly(true);
        list->setLineWrapMode(QPlainTextEdit::NoWrap);
        list->setAccessibleName(tr("Settings that will change"));
        list->setFixedHeight(kChangeListHeight);
        col->addWidget(list);

        QLineEdit* field = nullptr;
        if (requireTypedWord) {
            auto* prompt = new QLabel(
                tr("Type %1 to confirm you can recover this PC if it does not start.")
                    .arg(kWriteWord), card);
            prompt->setObjectName(QStringLiteral("srPromptBody"));
            prompt->setWordWrap(true);
            col->addWidget(prompt);

            field = new QLineEdit(card);
            field->setPlaceholderText(kWriteWord);
            field->setAccessibleName(prompt->text());
            col->addWidget(field);
        }

        col->addSpacing(tokens::CompactGap);
        auto* cancel = new vth::widgets::CommandButton(
            vth::widgets::CommandButton::Variant::Secondary, tr("Cancel"), card);
        auto* go = new vth::widgets::CommandButton(
            vth::widgets::CommandButton::Variant::Primary, tr("Write to firmware"), card);
        col->addLayout(vth::dialogs::buttonRow({ cancel, go }));

        if (field) {
            go->setEnabled(false);
            connect(field, &QLineEdit::textChanged, go, [go](const QString& t) {
                go->setEnabled(t.trimmed().compare(kWriteWord, Qt::CaseSensitive) == 0);
            });
        }

        vth::widgets::ScrimDialog dlg(card, this);
        // The safe side: Enter on a prompt nobody read must not write firmware.
        dlg.setInitialFocus(field ? static_cast<QWidget*>(field) : cancel);
        connect(cancel, &QPushButton::clicked, &dlg, &QDialog::reject);
        connect(go, &QPushButton::clicked, &dlg, &QDialog::accept);
        return dlg.exec() == QDialog::Accepted;
    }

    void NvramView::writePlan(const QString& path)
    {
        const nv::ScewinPaths tool = tool_;
        token_ = vth::fetch<nv::RunResult>(
            this,
            [tool, path]() { return nv::importSnapshot(tool, path); },
            [this](nv::RunResult r) {
                if (r.success) {
                    vth::dialogs::inform(this, tr("Firmware settings written"),
                        tr("Restart the PC for the new values to take effect.\n\n"
                           "%1").arg(r.message.trimmed()));
                } else {
                    vth::dialogs::warn(this, tr("Changes rejected"),
                        r.message.isEmpty()
                            ? tr("AMISCE did not report a reason. Some boards "
                                 "write-protect these variables until the "
                                 "protection is turned off in the BIOS.")
                            : r.message);
                }
                // Rebuilt either way: the restore button appears once a baseline
                // exists, and the loading state goes with the old panel.
                setState(State::Ready);
            });
    }

    void NvramView::onRestoreBaseline()
    {
        // Not tier-gated, so a lapsed license can still undo a write.
        if (!nv::hasBaseline())
            return;

        if (!vth::dialogs::confirm(this,
                tr("Restore first-seen settings"),
                tr("This writes back the firmware settings as they were the "
                   "first time Vethida read this PC. Every setting goes back to "
                   "that state, including ones you changed in the BIOS "
                   "yourself."),
                tr("Restore"), vth::dialogs::PromptTone::Warning))
            return;

        setBusy(true, restoreBtn_);

        const nv::ScewinPaths tool = tool_;
        const QString baseline = nv::baselinePath();
        token_ = vth::fetch<nv::RunResult>(
            this,
            [tool, baseline]() { return nv::importSnapshot(tool, baseline); },
            [this](nv::RunResult r) {
                if (r.success)
                    vth::dialogs::inform(this, tr("Firmware settings restored"),
                        tr("Restart the PC for the restored values to take effect."));
                else
                    vth::dialogs::warn(this, tr("Restore rejected"),
                        r.message.isEmpty()
                            ? tr("AMISCE did not report a reason.") : r.message);
                setState(State::Ready);
            });
    }
}
