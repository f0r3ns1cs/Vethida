#include "pch.h"
#include "MainWindow.h"

#include "../Pages/BiosPage.h"
#include "../Pages/DashboardPage.h"
#include "../Pages/DebloatPage.h"
#include "../Pages/GpuPage.h"
#include "../Pages/NetworkPage.h"
#include "../Pages/SystemPage.h"
#include "../Pages/TweakController.h"
#include "../Pages/TweaksPage.h"
#include "../Views/SystemRestoreView.h"
#include "../Settings/Preferences.h"
#include "../Settings/SettingsStore.h"
#include "../Theme/DesignTokens.h"
#include "../Theme/Theme.h"
#include "../Utils/TablerIcon.h"
#include "../Widgets/CaptionButtons.h"
#include "../Widgets/NavigationSidebar.h"
#include "../Widgets/CommandButton.h"
#include "../Widgets/Wordmark.h"
#include "../Widgets/Dropdown.h"
#include "../Widgets/Callout.h"
#include "../Widgets/PageHeader.h"
#include "../Widgets/PageScaffold.h"
#include "../Widgets/GlyphLabel.h"
#include "../Widgets/LicenseBadge.h"
#include "../Widgets/ListGroup.h"
#include "../Widgets/SettingsList.h"
#include "../Widgets/ScrimDialog.h"
#include "../Widgets/SwitchButton.h"
#include "../Widgets/ToastManager.h"
#include "../../vth_core/Models/LicenseModels.h"
#include "../../vth_core/Models/ProductTier.h"
#include "../../vth_core/SiteUrls.h"
#include "../../vth_core/Services/SessionProjection.h"
#include "../../vth_core/Services/LicenseService.h"
#include "../Dialogs/Activation.h"
#include "../Dialogs/Confirm.h"
#include "../Dialogs/ToolModal.h"

#include <QFileDialog>
#include <QFontMetrics>
#include <QMenu>
#include "../Dialogs/LicensesDialog.h"
#include "ui_MainWindow.h"

#include "../../vth_core/Runtime/Fetch.h"
#include "../../vth_core/Runtime/Sentry.h"
#include "../../vth_core/Runtime/ServiceRegistry.h"
#include "../../vth_core/Runtime/SupportBundle.h"
#include "../../vth_core/Runtime/Settings.h"
#include "../../vth_core/Runtime/Tier.h"
#include "../../vth_core/Tweaks/TweakEngine.h"

#include <iterator>  // std::size, over the toast-duration table
#include <tuple>     // std::tie, binding the account page's fact rows
#include <utility>   // std::make_pair
#include "../../vth_core/Windows/NvidiaReinstall.h"
#include "../../vth_core/Windows/Privileges.h"
#include "../../vth_core/Windows/Startup.h"
#include "../../vth_core/Runtime/AutoUpdate.h"
#include "../../vth_core/Runtime/WithdrawnTweaks.h"

#include <QAbstractButton>
#include <QAction>
#include <QClipboard>
#include <QDateTime>
#include <QDesktopServices>
#include <QEvent>
#include <QFutureWatcher>
#include <QScopeGuard>
#include <QStackedLayout>

#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLocale>
#include <QCoreApplication>
#include <QDir>
#include <QLineEdit>
#include <QScrollArea>
#include <QSettings>
#include <QShowEvent>
#include <QSysInfo>
#include <QTimer>
#include <QUrl>
#include <QWindow>
#include <QPainter>
#include <QRectF>
#include <QSize>
#include <QStackedWidget>
#include <QStyle>
#include <QSvgRenderer>
#include <QToolButton>
#include <QVBoxLayout>

#include <QGraphicsOpacityEffect>
#include <QPointer>
#include <QPropertyAnimation>

#include <functional>

namespace
{
    // A page arriving fades in. Skipped when Windows' animation effects are off, and the
    // effect comes off when it ends.
    void fadeInPage(QWidget* page)
    {
        if (!page || !vth::theme::animationsEnabled())
            return;
        auto* fx = new QGraphicsOpacityEffect(page);
        fx->setOpacity(0.0);
        page->setGraphicsEffect(fx);
        auto* anim = new QPropertyAnimation(fx, "opacity", page);
        anim->setDuration(vth::theme::animationDuration(170));
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        QObject::connect(anim, &QPropertyAnimation::finished, page,
                         [page, guard = QPointer<QGraphicsOpacityEffect>(fx)]() {
                             if (guard && page->graphicsEffect() == guard)
                                 page->setGraphicsEffect(nullptr);
                         });
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }

    // Builds a tab's view on first show, so its detection sweeps and WMI queries
    // are not paid at startup.
    class LazyPage final : public QWidget
    {
    public:
        explicit LazyPage(std::function<QWidget*(QWidget*)> factory,
                          QWidget* parent = nullptr)
            : QWidget(parent)
            , m_factory(std::move(factory))
        {
            auto* lay = new QVBoxLayout(this);
            lay->setContentsMargins(0, 0, 0, 0);
            lay->setSpacing(0);
        }

    protected:
        void showEvent(QShowEvent* e) override
        {
            if (m_factory) {
                layout()->addWidget(m_factory(this));
                m_factory = nullptr;
            }
            QWidget::showEvent(e);
        }

    private:
        std::function<QWidget*(QWidget*)> m_factory;
    };
}

MainWindow::MainWindow(vth::services::LicenseService& licenseService, QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_licenseService(licenseService)
{
    setAttribute(Qt::WA_NativeWindow, true);
    ui->setupUi(this);
    setWindowTitle(QStringLiteral(VETHIDA_PRODUCT_NAME " " VETHIDA_VERSION_STRING));
    ui->contentLayout->setContentsMargins(0, 0, 0, 0);
    ui->contentLayout->setSpacing(0);

    configureTopBar();
    configureUpdateServices();   // before any page (Settings, Dashboard) looks them up
    configureNavigation();
    configureNotifications();
    restoreUiState();
    applyNativeWindowChrome();

    m_sidebar->focusCurrentItem();
}

MainWindow::~MainWindow()
{
    vth::update::stop();
    m_sidebar.reset();
    delete ui;
}

// WinSparkle and the withdrawn-tweaks feed. Started here rather than by a page,
// so both run whichever page is open.
void MainWindow::configureUpdateServices()
{
    bool automatic = true;
    if (auto* settings = vth::ServiceRegistry::instance().get<vth::Settings>())
        automatic = settings->value(QStringLiteral("updater/autoCheck"), true).toBool();
    vth::update::start(automatic);

    auto* withdrawn = new vth::WithdrawnTweaks(this);
    withdrawn->start();
}

void MainWindow::configureNotifications()
{
    m_toasts = std::make_unique<vth::widgets::ToastManager>(this);
    // MainWindow keeps ownership. The registry only exposes a lookup.
    vth::ServiceRegistry::instance().addAlias<vth::widgets::ToastManager,
                                              vth::widgets::ToastManager>(m_toasts.get());
    if (auto* settings = vth::ServiceRegistry::instance().get<vth::Settings>()) {
        m_toasts->setInformationalEnabled(
            settings->value(QStringLiteral("app/notifications"), true).toBool());
        // The stored dwell time, applied before the first toast can appear.
        applyToastDuration(settings->value(QStringLiteral("notif/closeAfter"), 0).toInt());
    }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    // don't close during a driver install
    if (vth::win::nvidia::reinstall::isRunning()) {
        event->ignore();
        vth::dialogs::inform(this, tr("Driver install in progress"),
            tr("Vethida is replacing the NVIDIA driver. Closing now would leave the "
               "graphics card without a driver, so Vethida stays open until the "
               "install finishes. It takes a few minutes."));
        return;
    }
    saveUiState();
    QMainWindow::closeEvent(event);
}

bool MainWindow::nativeEvent(const QByteArray& eventType, void* message, qintptr* result)
{
    // Windows asks first at sign-out or shutdown, and is told no during a driver install.
    // The reason string is what its "apps are preventing shutdown" screen shows.
    if (eventType == "windows_generic_MSG") {
        const MSG* msg = static_cast<const MSG*>(message);
        if (msg->message == WM_QUERYENDSESSION) {
            if (vth::win::nvidia::reinstall::isRunning()) {
                ShutdownBlockReasonCreate(msg->hwnd,
                    L"Vethida is installing the graphics driver. Wait for it to finish.");
                *result = FALSE;
                return true;
            }
            ShutdownBlockReasonDestroy(msg->hwnd);
        }
    }
    if (m_chrome && m_chrome->handleNativeEvent(eventType, message, result))
        return true;
    return QMainWindow::nativeEvent(eventType, message, result);
}

void MainWindow::changeEvent(QEvent* event)
{
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::WindowStateChange || event->type() == QEvent::ActivationChange)
        refreshCaptionIcons();
    // re-check the license when the window comes back
    if (event->type() == QEvent::ActivationChange && isActiveWindow())
        m_licenseService.validateIfStale();
}

void MainWindow::configureTopBar()
{
    // An explicit floor, because otherwise the layout's is an accident of the rail's length.
    setMinimumHeight(vth::theme::tokens::ShellMinHeight);

    m_topBar = new QFrame(this);
    m_topBar->setObjectName(QStringLiteral("accountRibbon"));
    m_topBar->setFixedHeight(vth::theme::tokens::TitleBarHeight);
    setMenuWidget(m_topBar);

    auto* layout = new QHBoxLayout(m_topBar);
    layout->setContentsMargins(vth::theme::tokens::CardMargin, 0, 0, 0);
    layout->setSpacing(vth::theme::tokens::CardMargin);
    layout->addWidget(new vth::widgets::Wordmark(m_topBar));

    layout->addStretch(1);

#ifdef VETHIDA_LICENSING
    // The license badge, left of Help.
    m_titleLicenseBadge = new vth::widgets::LicenseBadge(m_topBar);
    m_titleLicenseBadge->setObjectName(QStringLiteral("titleLicenseBadge"));
    connect(m_titleLicenseBadge, &QAbstractButton::clicked, this,
            [this]() { navigateTo(QStringLiteral("account")); });

    layout->addWidget(m_titleLicenseBadge);
    layout->addSpacing(vth::theme::tokens::TightGap);
    refreshLicenseBadge();
#endif

    configureHelpMenu();
    layout->addWidget(m_titleHelpButton);
    layout->addSpacing(vth::theme::tokens::TightGap);

    m_captionButtons = new vth::widgets::CaptionButtons(true, m_topBar);
    layout->addWidget(m_captionButtons);
}

void MainWindow::refreshLicenseBadge()
{
    if (!m_titleLicenseBadge)
        return;

    vth::services::SessionSnapshot snapshot =
        vth::services::sessionSnapshotFor(m_licenseService.currentInfo(),
                                          QDateTime::currentMSecsSinceEpoch());
    // The entitlement gate wins over the stored license, because it's what actually
    // unlocks the app.
    snapshot.tier = vth::currentTier();
    snapshot.offline = m_licenseService.isOffline();
    const vth::services::RibbonBadge badge =
        vth::services::describeRibbonBadge(snapshot);

    m_titleLicenseBadge->setVisible(badge.visible);
    if (!badge.visible)
        return;

    using Kind = vth::widgets::LicenseBadge::Kind;
    const Kind kind = badge.urgent ? Kind::Warn
                    : snapshot.tier == vth::models::ProductTier::Ultimate ? Kind::Ultimate
                    : snapshot.tier == vth::models::ProductTier::Pro      ? Kind::Pro
                                                                          : Kind::Plain;
    m_titleLicenseBadge->setBadge(badge.text, badge.iconName, kind);
    m_titleLicenseBadge->setToolTip(tr("Open your account"));
}

void MainWindow::configureHelpMenu()
{
    m_titleHelpButton = new QToolButton(m_topBar);
    m_titleHelpButton->setObjectName(QStringLiteral("titleHelpButton"));
    m_titleHelpButton->setAccessibleName(tr("Help"));
    m_titleHelpButton->setToolTip(tr("Help"));
    m_titleHelpButton->setCursor(Qt::PointingHandCursor);
    m_titleHelpButton->setPopupMode(QToolButton::InstantPopup);
    vth::icons::applyIconButton(m_titleHelpButton, QStringLiteral("circle-help"),
                                &vth::icons::iconColor, 22);

    auto* menu = new QMenu(m_titleHelpButton);
    vth::theme::applyMenuChrome(menu);

    const auto addLink = [this, menu](const QString& text, QLatin1StringView url) {
        connect(menu->addAction(text), &QAction::triggered, this, [url]() {
            QDesktopServices::openUrl(QUrl(url));
        });
    };
    addLink(tr("Common questions"), vth::site::kFaq);
    addLink(tr("Support"), vth::site::kSupport);
    addLink(tr("Release notes"), vth::site::kChangelog);
    addLink(tr("Give feedback"), vth::site::kDiscord);
    menu->addSeparator();
    connect(menu->addAction(tr("About Vethida")), &QAction::triggered,
            this, [this]() { showAboutModal(); });
    m_titleHelpButton->setMenu(menu);
}

void MainWindow::showAboutModal()
{
    namespace pal    = vth::theme::palette;
    namespace tokens = vth::theme::tokens;

    // A text hyperlink. Qt paints anchors with QPalette::Link, so they follow the palette.
    // Qt paints anchors with QPalette::Link, which follows the palette on its own.
    const auto makeLink = [](const QString& text, const QString& href,
                             bool external) {
        auto* l = new QLabel(QStringLiteral(
            "<a href=\"%1\" style=\"text-decoration:none;\">%2</a>")
            .arg(href.toHtmlEscaped(), text.toHtmlEscaped()));
        l->setObjectName(QStringLiteral("settingsRowDesc"));
        l->setTextInteractionFlags(Qt::TextBrowserInteraction);
        l->setOpenExternalLinks(external);
        l->setCursor(Qt::PointingHandCursor);
        return l;
    };

    // The Cover modal fills the whole content region, so the content rides a
    // fixed-width column centered in it rather than stretching edge to edge.
    auto* content = new QWidget;
    auto* outer = new QVBoxLayout(content);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    auto* centerRow = new QHBoxLayout;
    centerRow->setContentsMargins(0, 0, 0, 0);
    centerRow->addStretch(1);

    auto* column = new QWidget(content);
    column->setMaximumWidth(tokens::ModalWidthMd);
    auto* col = new QVBoxLayout(column);
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(0);

    col->addWidget(new vth::widgets::Wordmark(column, QSize(161, 48)), 0, Qt::AlignHCenter);

    col->addSpacing(tokens::CardMargin);
    auto* version = new QLabel(
        tr("Version %1 (64-bit)").arg(QString::fromLatin1(VETHIDA_VERSION_STRING)),
        column);
    version->setProperty("uiRole", QStringLiteral("fieldValue"));
    version->setAlignment(Qt::AlignHCenter);
    col->addWidget(version, 0, Qt::AlignHCenter);

    col->addSpacing(tokens::RegionMargin);
    auto* checkBtn = new vth::widgets::CommandButton(vth::widgets::CommandButton::Variant::Secondary,
                                       tr("Check for updates"), column);
    checkBtn->setCursor(Qt::PointingHandCursor);
    connect(checkBtn, &QPushButton::clicked, this, [this]() { checkForUpdates(); });
    col->addWidget(checkBtn, 0, Qt::AlignHCenter);

    col->addSpacing(tokens::Space24);
    auto* rule = new QFrame(column);
    rule->setObjectName(QStringLiteral("settingsRule"));
    rule->setAttribute(Qt::WA_StyledBackground, true);
    col->addWidget(rule);
    col->addSpacing(tokens::Space24);

    auto* cols = new QHBoxLayout;
    cols->setContentsMargins(0, 0, 0, 0);
    cols->setSpacing(tokens::Space24);

    auto* supCol = new QVBoxLayout;
    supCol->setContentsMargins(0, 0, 0, 0);
    supCol->setSpacing(tokens::CardMargin);
    supCol->addWidget(makeLink(tr("Support"), vth::site::kSupport, true));
    supCol->addWidget(makeLink(tr("Release notes"), vth::site::kChangelog, true));
    supCol->addStretch(1);
    cols->addLayout(supCol, 1);

    auto* legCol = new QVBoxLayout;
    legCol->setContentsMargins(0, 0, 0, 0);
    legCol->setSpacing(tokens::CardMargin);
    legCol->addWidget(makeLink(tr("End user license agreement"),
                               vth::site::kEula, true));
    legCol->addWidget(makeLink(tr("Privacy policy"), vth::site::kPrivacy, true));
    // Opens the in-app licenses dialog rather than a URL: the full texts ship
    // with the binary, which is what the LGPL obligation below is satisfied by.
    auto* notices = makeLink(tr("Open-source licenses"),
                             QStringLiteral("vethida:licenses"), false);
    connect(notices, &QLabel::linkActivated, this, [this](const QString&) {
        vth::dialogs::showLicensesModal(this);
    });
    legCol->addWidget(notices);
    legCol->addStretch(1);
    cols->addLayout(legCol, 1);
    col->addLayout(cols);

    col->addSpacing(tokens::Space24);
    auto* copyLine = new QLabel(
        tr("© 2026 %1").arg(QString::fromLatin1(VETHIDA_COMPANY_NAME)), column);
    copyLine->setObjectName(QStringLiteral("settingsRowDesc"));
    copyLine->setAlignment(Qt::AlignHCenter);
    col->addWidget(copyLine, 0, Qt::AlignHCenter);

    // LGPLv3 4(c): a combined work that displays copyright notices while running must
    // include the Library's notice, with a reference to the license copies.
    auto* qtLine = new QLabel(column);
    qtLine->setText(tr("Qt © The Qt Company Ltd and contributors, "
                       "<a href=\"vethida:licenses\" style=\"text-decoration:none;\">"
                       "used under the LGPL v3</a>."));
    qtLine->setObjectName(QStringLiteral("settingsRowDesc"));
    qtLine->setAlignment(Qt::AlignHCenter);
    qtLine->setWordWrap(true);
    qtLine->setTextInteractionFlags(Qt::TextBrowserInteraction);
    qtLine->setCursor(Qt::PointingHandCursor);
    connect(qtLine, &QLabel::linkActivated, this, [this](const QString&) {
        vth::dialogs::showLicensesModal(this);
    });
    col->addWidget(qtLine, 0, Qt::AlignHCenter);

    // Microsoft's trademark guidance asks for the attribution and the non-affiliation
    // statement.
    auto* trademarkLine = new QLabel(
        tr("Windows and Microsoft are trademarks of Microsoft Corporation. "
           "Vethida is not affiliated with or endorsed by Microsoft."),
        column);
    trademarkLine->setObjectName(QStringLiteral("settingsRowDesc"));
    trademarkLine->setAlignment(Qt::AlignHCenter);
    trademarkLine->setWordWrap(true);
    col->addWidget(trademarkLine, 0, Qt::AlignHCenter);

    centerRow->addWidget(column, 0);
    centerRow->addStretch(1);
    outer->addLayout(centerRow, 0);
    outer->addStretch(1);

    vth::dialogs::showContentModal(this, tr("About Vethida"), content,
                                   vth::widgets::PanelDialog::Mode::Cover);
}

// WinSparkle shows its own window with the answer: up to date, or the new
// version with its notes and an Install button.
void MainWindow::checkForUpdates()
{
    if (!vth::update::available()) {
        if (m_updateStatus)
            m_updateStatus->setText(tr("Updates are off in this build."));
        return;
    }
    vth::update::checkNow();
}

void MainWindow::refreshUpdateUi()
{
    if (!m_updateStatus)
        return;
    m_updateStatus->setText(tr("Version %1").arg(QString::fromLatin1(VETHIDA_VERSION_STRING)));
}

// Index maps to the Notifications dropdown entries; anything else falls back to
// the first.
void MainWindow::applyToastDuration(int index)
{
    static constexpr int kMs[] = { 5000, 10000, 30000, 60000 };
    const int ms = (index >= 0 && index < static_cast<int>(std::size(kMs))) ? kMs[index] : kMs[0];
    if (m_toasts)
        m_toasts->setDefaultDurationMs(ms);
}

void MainWindow::configureNavigation()
{
    m_sidebar = std::make_unique<vth::widgets::NavigationSidebar>(ui->sidebarFrame);
    if (auto* frameLayout = qobject_cast<QVBoxLayout*>(ui->sidebarFrame->layout()))
        frameLayout->addWidget(m_sidebar.get());

    // One controller for every tweak surface. Created before the dashboard, which reads
    // its cached detection.
    m_tweakController = new vth::ui::TweakController({}, this);
    m_tweakController->setElevated(vth::win::isElevated());
    applyEntitlement();
    // Both follow the signal, or a key activated mid-session unlocks nothing until restart.
    connect(&m_licenseService, &vth::services::LicenseService::licenseStateChanged,
            this, &MainWindow::applyEntitlement);
    connect(&m_licenseService, &vth::services::LicenseService::licenseStateChanged,
            this, &MainWindow::refreshLicenseBadge);
    // Said once when the connection to the licensing API goes, and once when it is
    // back, rather than on every failed retry.
    connect(&m_licenseService, &vth::services::LicenseService::licenseStateChanged,
            this, [this]() {
        const bool offline = m_licenseService.isOffline();
        if (offline == m_wasOffline)
            return;
        m_wasOffline = offline;
        if (!m_toasts)
            return;
        if (offline) {
            m_toasts->warning(tr("No internet connection"),
                tr("Vethida cannot check your license, so it is running as Free. "
                   "Your plan returns when the connection does."));
        } else {
            m_toasts->success(tr("License confirmed"), tr("Your plan is active again."));
        }
    });
    m_tweakController->setConfirmHook([this](const vth::tweaks::TweakSpec& spec) {
        // permanent changes always ask, the rest unless turned off in Settings
        if (!vth::tweaks::isOneWay(spec) && !vth::prefs::confirmRisky())
            return true;
        QString body = spec.description;
        if (!spec.warnings.isEmpty())
            body += QStringLiteral("\n\n") + spec.warnings.join(QStringLiteral("\n"));
        if (!spec.reversible)
            body += QStringLiteral("\n\n") + tr("This cannot be undone.");
        return vth::dialogs::confirm(this, tr("Apply %1").arg(spec.name), body,
                                     tr("Apply"), vth::dialogs::PromptTone::Warning);
    });
    // one toast when a tweak didn't stick
    connect(m_tweakController, &vth::ui::TweakController::operationFinished, this,
            [this](const QString& id, const vth::tweaks::ApplyOutcome& outcome) {
                if (outcome.ok || !m_toasts)
                    return;
                const vth::tweaks::TweakSpec* spec = vth::tweaks::find(id);
                m_toasts->error(spec ? spec->name : id,
                                m_tweakController->failureReason(id));
            });

    // only if they turned on restore points and Windows wouldn't make one
    m_tweakController->setRestorePointFailedHook([this](const QString& reason) {
        return vth::dialogs::confirm(this, tr("No restore point was made"),
            tr("%1\n\nUndo still appears in the section afterwards, and reverses "
               "this run.").arg(reason),
            tr("Apply anyway"), vth::dialogs::PromptTone::Warning);
    });

    m_tweakController->setConflictHook(
        [this](const vth::tweaks::TweakSpec& requested, const vth::tweaks::TweakSpec& applied) {
            return vth::dialogs::confirm(this, tr("Conflicting tweaks"),
                tr("\"%1\" changes the same Windows settings as \"%2\", which is "
                   "currently on. Only one of them can be applied.")
                    .arg(requested.name, applied.name),
                tr("Turn off %1").arg(applied.name),
                vth::dialogs::PromptTone::Warning);
        });
    m_tweakController->setLockedHook(
        [this](const vth::tweaks::TweakSpec& spec, vth::tweaks::TweakLock lock) {
            if (lock == vth::tweaks::TweakLock::DisabledRemotely) {
                // Never an upgrade prompt: a withdrawn tweak is not a paywall.
                const QString why = vth::tweaks::remoteDisableReason(spec.id);
                vth::dialogs::inform(this, tr("Tweak withdrawn"),
                    why.isEmpty()
                        ? tr("\"%1\" has been turned off in a Vethida update because "
                             "it did not behave correctly. It will come back if that "
                             "is fixed. Nothing already applied has changed.")
                              .arg(spec.name)
                        : tr("\"%1\" has been turned off in a Vethida update: %2\n\n"
                             "Nothing already applied has changed.")
                              .arg(spec.name, why));
            } else if (lock == vth::tweaks::TweakLock::LockedEarlyAccess) {
                vth::dialogs::inform(this, tr("Early access required"),
                    tr("\"%1\" is an early-access tweak. Enable the Early access "
                       "program (%2) in Settings to apply it.")
                        .arg(spec.name,
                             vth::models::tierProductName(
                                 vth::models::ProductTier::Ultimate)));
            } else {
                const QString product = vth::models::tierProductName(spec.minTier);
                vth::dialogs::inform(this,
                    tr("%1 required").arg(product),
                    tr("\"%1\" is a %2 tweak. Upgrade to apply it.")
                        .arg(spec.name, product));
            }
        });

    m_routeIndex.insert(QStringLiteral("dashboard"), ui->contentStack->indexOf(ui->overviewPage));
    m_sidebar->addPrimaryItem(QStringLiteral("dashboard"), tr("Dashboard"),
                              QStringLiteral("layout-dashboard"));
    if (auto* overviewLayout = qobject_cast<QVBoxLayout*>(ui->overviewPage->layout())) {
        auto* dashboard = new vth::ui::DashboardPage(ui->overviewPage);
        connect(dashboard, &vth::ui::DashboardPage::navigateRequested,
                this, &MainWindow::navigateTo);
        overviewLayout->addWidget(dashboard);
    }

    auto* tweaksPage = new vth::ui::TweaksPage(m_tweakController, ui->contentStack);
    m_routeIndex.insert(QStringLiteral("tweaks"),
                        ui->contentStack->addWidget(tweaksPage));
    m_sidebar->addPrimaryItem(QStringLiteral("tweaks"), tr("Tweaks"),
                              QStringLiteral("sliders-horizontal"));

    auto* gpuPage = new vth::ui::GpuPage(m_tweakController, ui->contentStack);
    m_routeIndex.insert(QStringLiteral("gpu"), ui->contentStack->addWidget(gpuPage));
    m_sidebar->addPrimaryItem(QStringLiteral("gpu"), tr("GPU"), QStringLiteral("gpu"));

    const auto addRuntimePage = [this](const QString& id, const QString& label,
                                       const QString& icon, QWidget* page,
                                       vth::models::ProductTier minTier
                                           = vth::models::ProductTier::Free) {
        m_routeIndex.insert(id, ui->contentStack->addWidget(page));
        m_sidebar->addPrimaryItem(id, label, icon, minTier);
    };

    auto* networkPage = new vth::ui::NetworkPage(m_tweakController, ui->contentStack);
    m_routeIndex.insert(QStringLiteral("network"),
                        ui->contentStack->addWidget(networkPage));
    m_sidebar->addPrimaryItem(QStringLiteral("network"), tr("Network"),
                              QStringLiteral("network"));

    // Debloat gathers everything that removes rather than changes.
    auto* debloatPage = new vth::ui::DebloatPage(m_tweakController, ui->contentStack);
    m_routeIndex.insert(QStringLiteral("debloat"),
                        ui->contentStack->addWidget(debloatPage));
    // Pro at the rail, because everything on the tab removes preinstalled software.
    m_sidebar->addPrimaryItem(QStringLiteral("debloat"), tr("Debloat"),
                              QStringLiteral("package-off"),
                              vth::models::ProductTier::Pro);

    // System is the maintenance hub.
    auto* systemPage = new vth::ui::SystemPage(m_tweakController, ui->contentStack);
    m_routeIndex.insert(QStringLiteral("system"),
                        ui->contentStack->addWidget(systemPage));
    m_sidebar->addPrimaryItem(QStringLiteral("system"), tr("System"),
                              QStringLiteral("wrench"));

    // Genesis has no tab: it lives on the Dashboard card.

    // Built lazily: the firmware pane does IO on first show.
    addRuntimePage(QStringLiteral("bios"), tr("BIOS"), QStringLiteral("cpu"),
                   new LazyPage([](QWidget* p) -> QWidget* {
                       return new vth::ui::BiosPage(p);
                   }, ui->contentStack),
                   vth::models::ProductTier::Ultimate);

    addRuntimePage(QStringLiteral("backup"), tr("Restore"), QStringLiteral("history"),
                   new LazyPage([](QWidget* p) -> QWidget* {
                       return new vth::ui::SystemRestoreView(p);
                   }, ui->contentStack));

#ifdef VETHIDA_LICENSING
    configureAccountPage();
#endif

    m_routeIndex.insert(QStringLiteral("settings"),
                        ui->contentStack->indexOf(ui->settingsPage));
    m_sidebar->addUtilityItem(QStringLiteral("settings"), tr("Settings"),
                              QStringLiteral("settings"), true);
    configureSettingsPage();

    connect(m_sidebar.get(), &vth::widgets::NavigationSidebar::navigationRequested,
            this, &MainWindow::navigateTo);

    m_sidebar->setCurrentItem(QStringLiteral("dashboard"));
}

void MainWindow::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);
    if (m_prewarmStarted)
        return;
    m_prewarmStarted = true;
    // After the dashboard's own first work has settled.
    QTimer::singleShot(2000, this, &MainWindow::prewarmPages);
}

void MainWindow::prewarmPages()
{
    if (m_prewarmQueue.isEmpty()) {
        for (const char* id : { "gpu", "network", "system", "debloat" }) {
            const int index = m_routeIndex.value(QString::fromLatin1(id), -1);
            if (index >= 0)
                m_prewarmQueue.append(ui->contentStack->widget(index));
        }
    }
    // Only while the user is not touching anything: a page takes a moment to
    // build, and that moment must not land on a click.
    LASTINPUTINFO last{ sizeof(LASTINPUTINFO), 0 };
    if (GetLastInputInfo(&last) && GetTickCount() - last.dwTime < 800) {
        QTimer::singleShot(400, this, &MainWindow::prewarmPages);
        return;
    }
    // One page per tick, so input that arrives in between is handled at once.
    while (!m_prewarmQueue.isEmpty()) {
        const QPointer<QWidget> page = m_prewarmQueue.takeFirst();
        if (!page)
            continue;
        QMetaObject::invokeMethod(page.data(), "prewarm", Qt::DirectConnection);
        if (!m_prewarmQueue.isEmpty())
            QTimer::singleShot(150, this, &MainWindow::prewarmPages);
        return;
    }
}

void MainWindow::navigateTo(const QString& routeId)
{
    const int page = m_routeIndex.value(routeId, -1);
    if (page < 0)
        return;
    // Every way into a page comes through here, so a gated destination is gated on all
    // of them.
    if (m_sidebar && m_sidebar->isItemTierLocked(routeId))
        return;
    const bool moving = ui->contentStack->currentIndex() != page;
    ui->contentStack->setCurrentIndex(page);
    if (moving)
        fadeInPage(ui->contentStack->widget(page));
    m_settings.setSelectedRoute(routeId);
    m_sidebar->setCurrentItem(routeId);
}

void MainWindow::configureSettingsPage()
{
    auto* layout = qobject_cast<QVBoxLayout*>(ui->settingsPage->layout());
    if (!layout)
        return;
    ui->settingsPage->setAttribute(Qt::WA_StyledBackground, true);

    while (layout->count()) {
        QLayoutItem* item = layout->takeAt(0);
        if (QWidget* w = item->widget())
            w->deleteLater();
        delete item;
    }
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    namespace tokens = vth::theme::tokens;
    using vth::widgets::ListGroup;
    using vth::widgets::ListRow;
    using vth::widgets::SwitchButton;

    // Named sections, each a small heading over one card of rows.
    auto* page = new vth::widgets::PageScaffold(tr("Settings"));
    page->setBodySpacing(tokens::Space24);
    layout->addWidget(page);
    auto section = [page](const QString& title) {
        auto* g = new ListGroup(title, nullptr, ListGroup::Style::Grouped);
        page->addBodyWidget(g);
        return g;
    };

    auto* settingsSvc = vth::ServiceRegistry::instance().get<vth::Settings>();

    // A switch stored under `key`. `inverted` shows the opposite of the stored
    // value, for a key named for the opposite of what the switch says.
    auto boundSwitch = [settingsSvc](const QString& key, bool defaultOn, bool inverted = false) {
        auto* sw = new SwitchButton;
        sw->setCheckable(true);
        const bool stored = settingsSvc ? settingsSvc->value(key, defaultOn).toBool() : defaultOn;
        sw->setChecked(stored != inverted);
        connect(sw, &QAbstractButton::toggled, sw, [settingsSvc, key, inverted](bool on) {
            if (!settingsSvc)
                return;
            settingsSvc->setValue(key, on != inverted);
            settingsSvc->sync();
        });
        return sw;
    };
    auto row = [](ListGroup* group, const QString& title, const QString& desc,
                  QWidget* control) {
        ListRow* r = group->addRow(title, desc);
        if (control) {
            if (control->accessibleName().isEmpty())
                control->setAccessibleName(title);
            r->addTrailing(control);
        }
        return r;
    };

    // askLess is stored as "don't ask"; the switch reads as "confirm".
    auto* confirmReversible = boundSwitch(vth::prefs::key::askLess, false, true);
    auto* confirmRisky      = boundSwitch(vth::prefs::key::confirmRisky, true);
    auto* showTips          = boundSwitch(vth::prefs::key::showTips, true);
    auto* reduceMotion      = boundSwitch(vth::prefs::key::reduceMotion, false);
    auto* lastPage          = boundSwitch(vth::prefs::key::restoreLastPage, true);
    // Read by TweakController::runBulk.
    auto* restorePointToggle = boundSwitch(QStringLiteral("app/restorePointBeforeBulk"), true);

    auto* notifToggle = boundSwitch(QStringLiteral("app/notifications"), true);
    connect(notifToggle, &QAbstractButton::toggled, this, [this](bool on) {
        if (m_toasts)
            m_toasts->setInformationalEnabled(on);
    });
    auto* closeAfterDrop = new vth::ui::Dropdown;
    closeAfterDrop->setItems({ tr("5 seconds"), tr("10 seconds"), tr("30 seconds"), tr("1 minute") });
    closeAfterDrop->setMinimumWidth(tokens::FieldWidthCompact + tokens::Space32);
    {
        const int idx = settingsSvc ? settingsSvc->value(QStringLiteral("notif/closeAfter"), 0).toInt() : 0;
        closeAfterDrop->setCurrentIndex(qBound(0, idx, 3));
    }
    connect(closeAfterDrop, &vth::ui::Dropdown::currentIndexChanged, this,
            [this, settingsSvc](int index) {
                if (settingsSvc) {
                    settingsSvc->setValue(QStringLiteral("notif/closeAfter"), index);
                    settingsSvc->sync();
                }
                applyToastDuration(index);
            });
    auto* alertToggle = boundSwitch(vth::prefs::key::alertOnFinish, true);

    auto* autoCheckToggle = boundSwitch(QStringLiteral("updater/autoCheck"), true);
    connect(autoCheckToggle, &QAbstractButton::toggled, this,
            [](bool on) { vth::update::setAutomatic(on); });
    m_updateBtn = new vth::widgets::CommandButton(vth::widgets::CommandButton::Variant::Secondary,
                                                  tr("Check for updates"), nullptr);
    connect(m_updateBtn, &QPushButton::clicked, this, [this]() { checkForUpdates(); });
    m_updateStatus = new QLabel;
    m_updateStatus->setObjectName(QStringLiteral("settingsRowDesc"));
    m_updateStatus->setWordWrap(true);
    refreshUpdateUi();

    auto* startupToggle = new SwitchButton;
    startupToggle->setCheckable(true);
    // Read and written on a worker, and disabled until the current value lands.
    startupToggle->setEnabled(false);
    {
        QPointer<QAbstractButton> guard(startupToggle);
        vth::fetch<bool>(
            this,
            []() { return vth::win::startup::self::launchesAtLogon(); },
            [guard](bool on) {
                if (!guard)
                    return;
                QSignalBlocker block(guard);
                guard->setChecked(on);
                guard->setEnabled(true);
            });
    }
    connect(startupToggle, &QAbstractButton::toggled, this,
            [this, startupToggle](bool on) {
                QPointer<QAbstractButton> guard(startupToggle);
                startupToggle->setEnabled(false);
                vth::fetch<bool>(
                    this,
                    [on]() { return vth::win::startup::self::setLaunchAtLogon(on); },
                    [this, guard, on](bool done) {
                        if (!guard)
                            return;
                        guard->setEnabled(true);
                        if (done)
                            return;
                        QSignalBlocker block(guard);
                        guard->setChecked(!on);
                        if (m_toasts)
                            m_toasts->error(tr("Startup setting not changed"),
                                            tr("Windows refused the change. A policy on a managed PC or "
                                               "a security product can guard the startup list."));
                    });
            });

    auto* general = section(tr("General"));
    row(general, tr("Start with Windows"),
        tr("Open Vethida when you sign in."), startupToggle);
    row(general, tr("Open to the last page"),
        tr("Return to the page you had open when Vethida was closed."), lastPage);
    row(general, tr("Reduce motion"),
        tr("Turn off animations and transitions."), reduceMotion);
    row(general, tr("Show tips"),
        tr("Explain a page briefly the first time you open it."), showTips);

    auto* safety = section(tr("Safety"));
    row(safety, tr("Create a restore point before batch changes"),
        tr("Runs before Apply recommended and Apply all. Windows allows one restore "
           "point a day, so it is skipped when today already has one."),
        restorePointToggle);
    row(safety, tr("Confirm high-risk changes"),
        tr("Ask before tweaks that can stop Windows from starting or lock you out "
           "of your account."),
        confirmRisky);
    row(safety, tr("Confirm reversible changes"),
        tr("Ask before changes that can be undone. Permanent changes, file "
           "deletions and restarts always ask."),
        confirmReversible);

    auto* notices = section(tr("Notifications"));
    row(notices, tr("Notify when a task finishes"),
        tr("Errors are always shown."), notifToggle);
    row(notices, tr("Notification duration"),
        tr("How long a notification stays on screen."), closeAfterDrop);
    row(notices, tr("Flash the taskbar when a task finishes"),
        tr("Only while Vethida is in the background."), alertToggle);

    auto* updates = section(tr("Updates"));
    ListRow* versionRow = row(updates, tr("Vethida"), QString(), m_updateBtn);
    // The version line sits under the row's title.
    versionRow->addDetail(m_updateStatus);
    row(updates, tr("Check for updates automatically"),
        tr("Checks once a day. Nothing is installed without your approval."),
        autoCheckToggle);

    // Only while the catalog holds an early-access tweak.
    bool anyEarlyAccess = false;
    for (const vth::tweaks::TweakSpec& spec : vth::tweaks::catalog())
        if (spec.earlyAccess) {
            anyEarlyAccess = true;
            break;
        }
    if (anyEarlyAccess) {
        auto* earlyToggle = new SwitchButton;
        earlyToggle->setCheckable(true);
        auto* early = section(tr("Early access"));
        if (vth::isUltimate()) {
            earlyToggle->setChecked(settingsSvc
                && settingsSvc->value(QStringLiteral("app/earlyAccessOptIn"), false).toBool());
            connect(earlyToggle, &QAbstractButton::toggled, this,
                    [this, earlyToggle, settingsSvc](bool on) {
                // confirm joining, leaving doesn't need it
                if (on && !vth::dialogs::confirm(this,
                        tr("Join early access"),
                        tr("Early-access tweaks are still being tested. They have "
                           "been tried on fewer PCs than the rest of the catalog, can "
                           "change or be removed in any update, and are provided on "
                           "the same terms as everything else in Vethida, without any "
                           "warranty."),
                        tr("Join"), vth::dialogs::PromptTone::Warning)) {
                    QSignalBlocker block(earlyToggle);
                    earlyToggle->setChecked(false);
                    return;
                }
                if (settingsSvc) {
                    settingsSvc->setValue(QStringLiteral("app/earlyAccessOptIn"), on);
                    settingsSvc->sync();
                }
                // right away, not at next launch
                applyEntitlement();
            });
            row(early, tr("Early access features"),
                tr("Try tweaks that are still in development. They are labeled "
                   "Early access and may change or be removed."),
                earlyToggle);
        } else {
            earlyToggle->setEnabled(false);
            // Opens the upgrade.
            const QString ultimate = vth::models::tierProductName(vth::models::ProductTier::Ultimate);
            auto* lock = new QToolButton;
            lock->setProperty("uiRole", QStringLiteral("chip"));
            lock->setProperty("tone", QStringLiteral("premium"));
            lock->setText(vth::models::tierLabel(vth::models::ProductTier::Ultimate));
            lock->setCursor(Qt::PointingHandCursor);
            lock->setToolTip(tr("Included with %1").arg(ultimate));
            lock->setAccessibleName(tr("Included with %1").arg(ultimate));
            connect(lock, &QToolButton::clicked, this, [this]() {
                vth::dialogs::showUpgrade(this, vth::models::tierLabel(vth::models::ProductTier::Ultimate));
            });
            ListRow* r = row(early, tr("Early access features"),
                             tr("Included with %1.").arg(ultimate), lock);
            r->addTrailing(earlyToggle);
        }
    }

    auto& registry = vth::ServiceRegistry::instance();
    auto* sentry = registry.get<vth::sentry::Sentry>();
    auto* crashSwitch = new SwitchButton;
    crashSwitch->setCheckable(true);
    crashSwitch->setChecked(sentry && sentry->isCrashEnabled());
    connect(crashSwitch, &QAbstractButton::toggled, this, [sentry](bool on) {
        if (sentry)
            sentry->setCrashEnabled(on);
    });

    // only saves a file where the user picks, doesn't upload anything
    auto* supportBtn = new vth::widgets::CommandButton(
        vth::widgets::CommandButton::Variant::Secondary, tr("Export"), nullptr);
    connect(supportBtn, &QPushButton::clicked, this, [this, supportBtn]() {
        const QString suggested =
            QDir(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation))
                .filePath(vth::support::suggestedFileName());
        const QString target = QFileDialog::getSaveFileName(
            this, tr("Save support bundle"), suggested, tr("Zip archive (*.zip)"));
        if (target.isEmpty())
            return;
        supportBtn->setLoading(true);
        vth::fetch<vth::support::BundleResult>(
            this,
            [target]() { return vth::support::writeBundle(target); },
            [this, supportBtn](vth::support::BundleResult result) {
                supportBtn->setLoading(false);
                if (!m_toasts)
                    return;
                if (result.ok)
                    m_toasts->success(tr("Support bundle saved"), QDir::toNativeSeparators(result.path));
                else
                    m_toasts->error(tr("Support bundle not saved"), result.error);
            });
    });

    auto* privacy = section(tr("Privacy"));
    row(privacy, tr("Send crash reports"),
        tr("If Vethida crashes, send the error and a snapshot of its memory to "
           "Sentry so the fault can be fixed. No license key and none of your files."),
        crashSwitch);
    auto* policyBtn = new vth::widgets::CommandButton(
        vth::widgets::CommandButton::Variant::Secondary, tr("Open"), nullptr);
    connect(policyBtn, &QPushButton::clicked, this,
            []() { QDesktopServices::openUrl(QUrl(vth::site::kPrivacy)); });
    row(privacy, tr("Privacy policy"),
        tr("What Vethida collects, and what it never does."), policyBtn);

    auto* support = section(tr("Support"));
    row(support, tr("Support bundle"),
        tr("Save system details, Vethida's change history and logs to a zip file "
           "to send to support. License keys are removed."),
        supportBtn);

    // reset preferences (not startup or consent)
    auto* restoreBtn = new vth::widgets::CommandButton(
        vth::widgets::CommandButton::Variant::Secondary, tr("Reset"), nullptr);
    connect(restoreBtn, &QPushButton::clicked, this,
            [this, confirmReversible, confirmRisky, showTips, notifToggle, closeAfterDrop, alertToggle,
             reduceMotion, lastPage, restorePointToggle, autoCheckToggle, settingsSvc]() {
                confirmReversible->setChecked(true);
                confirmRisky->setChecked(true);
                showTips->setChecked(true);
                notifToggle->setChecked(true);
                closeAfterDrop->setCurrentIndex(0);
                alertToggle->setChecked(true);
                reduceMotion->setChecked(false);
                lastPage->setChecked(true);
                restorePointToggle->setChecked(true);
                autoCheckToggle->setChecked(true);
                // The one-time tips come back too.
                if (settingsSvc) {
                    for (const QString& k : settingsSvc->allKeys())
                        if (k.startsWith(QLatin1String("cautionAck/")))
                            settingsSvc->remove(k);
                    settingsSvc->sync();
                }
                if (m_toasts)
                    m_toasts->success(tr("Settings"), tr("Defaults restored."));
            });
    auto* reset = section(tr("Reset"));
    row(reset, tr("Restore default settings"),
        tr("Puts every setting on this page back to how Vethida ships. Start with "
           "Windows and the privacy choices are kept."),
        restoreBtn);

    // Seed the update line so it says which version is running before any
    // check has been made.
    refreshUpdateUi();
}

// Account: the license on this PC. Invoices and the subscription live in Stripe's
// customer portal.
void MainWindow::configureAccountPage()
{
    m_accountPage = new QWidget(ui->contentStack);
    auto* pageLay = new QVBoxLayout(m_accountPage);
    pageLay->setContentsMargins(0, 0, 0, 0);
    pageLay->setSpacing(0);

    auto* header = new vth::widgets::PageHeader(tr("Account"), QString(), m_accountPage);
    pageLay->addWidget(header);

    m_accountList = new vth::widgets::SettingsList(m_accountPage);
    pageLay->addWidget(m_accountList, 1);

    m_routeIndex.insert(QStringLiteral("account"), ui->contentStack->addWidget(m_accountPage));
    m_sidebar->addUtilityItem(QStringLiteral("account"), tr("Account"),
                              QStringLiteral("circle-user"), true);

    // Rebuilt per state rather than built once and mutated, so an unreachable
    // state cannot be left on screen. License changes are rare.
    refreshAccountPage();
    connect(&m_licenseService, &vth::services::LicenseService::licenseStateChanged,
            this, &MainWindow::refreshAccountPage);
}

namespace
{
    // Same vocabulary as the restore-point list (SystemRestoreView), so "last
    // checked" reads the way every other elapsed time in the app reads.
    QString accountElapsed(const QDateTime& when)
    {
        const qint64 secs = when.toLocalTime().secsTo(QDateTime::currentDateTime());
        if (secs < 60)         return MainWindow::tr("just now");
        if (secs < 3600)       return MainWindow::tr("%1 min ago").arg(secs / 60);
        if (secs < 86400)      return MainWindow::tr("%1 h ago").arg(secs / 3600);
        if (secs < 86400 * 30) return MainWindow::tr("%1 d ago").arg(secs / 86400);
        return MainWindow::tr("%1 mo ago").arg(secs / (86400 * 30));
    }
}

void MainWindow::refreshAccountPage()
{
    using vth::models::LicenseStatus;
    using vth::models::ProductTier;
    namespace tokens = vth::theme::tokens;
    namespace pal    = vth::theme::palette;

    if (!m_accountList)
        return;
    m_accountList->clear();

    const vth::models::LicenseInfo info = m_licenseService.currentInfo();

    const bool haveKey = !info.key.isEmpty();

    // from currentTier(), not the saved license (they differ when offline)
    const ProductTier granted = vth::currentTier();
    const bool stored   = haveKey && info.status == LicenseStatus::Valid;
    // Holds something paid for. Free is a tier everyone has, so it can't be the test.
    const bool granting =
        vth::models::tierRank(granted) >= vth::models::tierRank(ProductTier::Pro);
    const bool unverified = stored && !granting;
    // The activation on this PC is gone (support reset the key, or it was
    // deactivated from somewhere else); the key itself may still be good.
    const bool notActivatedHere = haveKey && info.instanceId.isEmpty();
    const bool offline = m_licenseService.isOffline() && stored;

    auto* plan = new QFrame;
    plan->setObjectName(QStringLiteral("sectionCard"));
    plan->setAttribute(Qt::WA_StyledBackground, true);
    auto* planCol = new QVBoxLayout(plan);
    planCol->setContentsMargins(tokens::CardInsetH, tokens::CardInsetV,
                                tokens::CardInsetH, tokens::CardInsetV);
    planCol->setSpacing(tokens::TightGap);

    // Named from `granted`, the same entitlement gate the title-bar badge reads,
    // so the two surfaces cannot disagree.
    if (!granting) {
        auto* freeName = new QLabel(vth::models::tierLabel(ProductTier::Free), plan);
        freeName->setObjectName(QStringLiteral("accountTierName"));
        planCol->addWidget(freeName);
        planCol->addSpacing(tokens::CardMargin);
    }

    if (granting) {
        auto* eyebrow = new QLabel(tr("Current plan"), plan);
        eyebrow->setProperty("uiRole", QStringLiteral("metaText"));
        planCol->addWidget(eyebrow);
    }

    auto* tierRow = new QHBoxLayout;
    tierRow->setContentsMargins(0, 0, 0, 0);
    tierRow->setSpacing(tokens::CompactGap);
    if (granting) {
        auto* tierName = new QLabel(vth::models::tierProductName(granted), plan);
        tierName->setObjectName(QStringLiteral("accountTierName"));
        tierRow->addWidget(tierName, 0, Qt::AlignVCenter);
    }

    // The pill carries the state the tier name does not, and the line under
    // the card says what to do about it.
    QString pillText;
    QString stateLine;
    bool    pillGood = false;
    if (offline) {
        pillText  = tr("Offline");
        stateLine = tr("No internet connection. Vethida is running as Free because it "
                       "cannot check your license. It tries again every minute, and "
                       "your plan returns when the connection does.");
    } else if (notActivatedHere) {
        pillText  = tr("Not active on this PC");
        stateLine = tr("This PC's activation was removed. Activate it again to "
                       "use your plan here.");
    } else if (haveKey) {
        switch (info.status) {
        case LicenseStatus::Valid:
            pillText = unverified ? tr("Not verified") : tr("Active");
            pillGood = !unverified;
            if (unverified)
                stateLine = tr("Vethida could not confirm this license. Check again.");
            break;
        case LicenseStatus::Expired:
            pillText  = tr("Expired");
            stateLine = tr("The subscription behind this license has ended. Renew "
                           "it in Billing, then check again.");
            break;
        case LicenseStatus::Revoked:
            pillText  = tr("Disabled");
            stateLine = tr("This license was disabled, usually after a refund. "
                           "Contact support if that is unexpected.");
            break;
        case LicenseStatus::Unverified:
            pillText  = tr("Not verified");
            stateLine = tr("This license has not been confirmed yet. "
                           "Check again.");
            break;
        }
    }
    if (!pillText.isEmpty()) {
        auto* pill = new QLabel(pillText, plan);
        pill->setObjectName(QStringLiteral("accountStatusPill"));
        pill->setAttribute(Qt::WA_StyledBackground, true);
        pill->setProperty("tone", pillGood ? QStringLiteral("good")
                                           : QStringLiteral("warn"));
        tierRow->addWidget(pill, 0, Qt::AlignVCenter);
    }
    tierRow->addStretch(1);
    // The stretch means the row is never empty, so it's counted.
    if (tierRow->count() > 1) {
        planCol->addLayout(tierRow);
        planCol->addSpacing(tokens::CardMargin);
    } else {
        delete tierRow;
    }
    if (!stateLine.isEmpty()) {
        auto* line = new QLabel(stateLine, plan);
        line->setObjectName(QStringLiteral("settingsRowDesc"));
        line->setWordWrap(true);
        planCol->addWidget(line);
        planCol->addSpacing(tokens::CardMargin);
    }

    auto* actions = new QHBoxLayout;
    actions->setContentsMargins(0, 0, 0, 0);
    actions->setSpacing(tokens::CompactGap);

    if (!haveKey) {
        auto* plans = new vth::widgets::CommandButton(
            vth::widgets::CommandButton::Variant::Primary, tr("See plans"), plan);
        plans->setCursor(Qt::PointingHandCursor);
        connect(plans, &QPushButton::clicked, this, []() {
            QDesktopServices::openUrl(QUrl(vth::site::kPricing));
        });
        actions->addWidget(plans, 0, Qt::AlignVCenter);

        auto* activate = new vth::widgets::CommandButton(
            vth::widgets::CommandButton::Variant::Secondary, tr("Activate"), plan);
        activate->setCursor(Qt::PointingHandCursor);
        connect(activate, &QPushButton::clicked, this, [this]() { showActivateKeyModal(); });
        actions->addWidget(activate, 0, Qt::AlignVCenter);
    } else if (notActivatedHere) {
        auto* again = new vth::widgets::CommandButton(
            vth::widgets::CommandButton::Variant::Primary, tr("Activate again"), plan);
        again->setCursor(Qt::PointingHandCursor);
        connect(again, &QPushButton::clicked, this, [this, again]() {
            again->setLoading(true);
            using vth::models::LicenseOpResult;
            auto* watcher = new QFutureWatcher<LicenseOpResult>(this);
            connect(watcher, &QFutureWatcher<LicenseOpResult>::finished, this,
                    [this, watcher]() {
                const LicenseOpResult result = watcher->result();
                watcher->deleteLater();
                if (!result.ok && m_toasts)
                    m_toasts->warning(tr("Not activated"),
                                      vth::models::licenseErrorLabel(result.error));
                refreshAccountPage();
            });
            watcher->setFuture(m_licenseService.reactivateAsync());
        });
        actions->addWidget(again, 0, Qt::AlignVCenter);

        auto* other = new vth::widgets::CommandButton(
            vth::widgets::CommandButton::Variant::Secondary, tr("Use another key"), plan);
        other->setCursor(Qt::PointingHandCursor);
        connect(other, &QPushButton::clicked, this, [this]() { showActivateKeyModal(); });
        actions->addWidget(other, 0, Qt::AlignVCenter);
    } else {
        if (info.status == LicenseStatus::Expired) {
            auto* renew = new vth::widgets::CommandButton(
                vth::widgets::CommandButton::Variant::Primary, tr("Renew"), plan);
            renew->setCursor(Qt::PointingHandCursor);
            connect(renew, &QPushButton::clicked, this, []() {
                QDesktopServices::openUrl(QUrl(vth::site::kPortalBilling));
            });
            actions->addWidget(renew, 0, Qt::AlignVCenter);
        }
        // Primary only when it is the fix for what the card says.
        const bool checkIsTheFix = unverified || offline
            || info.status == LicenseStatus::Unverified;
        auto* check = new vth::widgets::CommandButton(
            checkIsTheFix ? vth::widgets::CommandButton::Variant::Primary
                          : vth::widgets::CommandButton::Variant::Secondary,
            tr("Check now"), plan);
        check->setCursor(Qt::PointingHandCursor);
        connect(check, &QPushButton::clicked, this, [this, check]() {
            // On success licenseStateChanged rebuilds this page and destroys the
            // button, so the watcher is parented to the window, not the button.
            check->setLoading(true);
            using vth::models::LicenseOpResult;
            auto* watcher = new QFutureWatcher<LicenseOpResult>(this);
            connect(watcher, &QFutureWatcher<LicenseOpResult>::finished, this,
                    [this, watcher]() {
                const LicenseOpResult result = watcher->result();
                watcher->deleteLater();
                if (result.ok)
                    return;   // the state change already rebuilt the page

                // The actual reason, not "could not reach the server" for every
                // failure: a revoked key and a skewed clock are not offline.
                if (m_toasts) {
                    m_toasts->warning(tr("License not verified"),
                                      vth::models::licenseErrorLabel(result.error));
                }
                refreshAccountPage();   // clears the loading state
            });
            watcher->setFuture(m_licenseService.validateAsync());
        });
        actions->addWidget(check, 0, Qt::AlignVCenter);

        auto* off = new vth::widgets::CommandButton(
            vth::widgets::CommandButton::Variant::Secondary, tr("Deactivate this PC"), plan);
        off->setCursor(Qt::PointingHandCursor);
        connect(off, &QPushButton::clicked, this, [this]() {
            if (vth::dialogs::confirm(this, tr("Deactivate this PC"),
                    tr("This frees one activation on your license so you can use "
                       "it on another PC. Vethida goes back to Free here until "
                       "you activate it again. Your subscription is not affected."),
                    tr("Deactivate"))) {
                using vth::models::LicenseOpResult;
                auto* watcher = new QFutureWatcher<LicenseOpResult>(this);
                connect(watcher, &QFutureWatcher<LicenseOpResult>::finished, this,
                        [this, watcher]() {
                    const LicenseOpResult result = watcher->result();
                    watcher->deleteLater();
                    if (result.ok || !m_toasts)
                        return;
                    // cleared here, the server may still count the activation
                    m_toasts->warning(
                        tr("Activation may still be counted"),
                        tr("%1 If another PC is refused, contact support and we "
                           "will free it.")
                            .arg(vth::models::licenseErrorLabel(result.error)));
                });
                watcher->setFuture(m_licenseService.deactivateAsync());
            }
        });
        actions->addWidget(off, 0, Qt::AlignVCenter);
    }
    actions->addStretch(1);
    planCol->addLayout(actions);

    // A row paints a backplate wider than its text, so the card pays the row
    // padding itself to line its left edge up with the titles under it.
    constexpr int kPlanCardShare = 3;
    constexpr int kPlanCardGap   = 2;
    // Keeps the two action buttons on one line at the narrowest supported
    // window. Below this the card stops shrinking and the gap absorbs it.
    constexpr int kPlanCardMinWidth = 360;

    plan->setMinimumWidth(kPlanCardMinWidth);
    // Its content height and no more.
    plan->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    auto* planHost = new QWidget;
    auto* planHostLay = new QHBoxLayout(planHost);
    planHostLay->setContentsMargins(tokens::RowInsetH, 0, tokens::RowInsetH, 0);
    planHostLay->setSpacing(0);
    planHostLay->addWidget(plan, kPlanCardShare);
    planHostLay->addStretch(kPlanCardGap);
    m_accountList->addWidget(planHost);

    // read-only row, same objectName/insets as settingsRow so they line up
    const auto addFact = [this](const QString& label, const QString& value,
                                const QString& note, QWidget* trailing) {
        namespace tk = vth::theme::tokens;
        auto* row = new QFrame;
        row->setObjectName(QStringLiteral("settingsRow"));
        row->setAttribute(Qt::WA_StyledBackground, true);
        auto* lay = new QHBoxLayout(row);
        lay->setContentsMargins(tk::RowInsetH, tk::CardMargin,
                                tk::RowInsetH, tk::CardMargin);
        lay->setSpacing(tk::CardMargin);

        auto* col = new QVBoxLayout;
        col->setContentsMargins(0, 0, 0, 0);
        col->setSpacing(tk::Space2);
        auto* title = new QLabel(label, row);
        title->setObjectName(QStringLiteral("settingsRowTitle"));
        col->addWidget(title);
        if (!note.isEmpty()) {
            auto* desc = new QLabel(note, row);
            desc->setObjectName(QStringLiteral("settingsRowDesc"));
            desc->setWordWrap(true);
            col->addWidget(desc);
        }
        lay->addLayout(col, 1);

        auto* val = new QLabel(value, row);
        val->setProperty("uiRole", QStringLiteral("fieldValue"));
        val->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        val->setTextInteractionFlags(Qt::TextSelectableByMouse);
        lay->addWidget(val, 0, Qt::AlignVCenter);

        if (trailing) {
            trailing->setParent(row);
            lay->addWidget(trailing, 0, Qt::AlignVCenter);
        }
        m_accountList->addWidget(row);
        return row;
    };

    if (haveKey) {
        m_accountList->addHeading(tr("License"));

        if (!info.customerEmail.isEmpty())
            addFact(tr("Purchased by"), info.customerEmail, QString(), nullptr);

        // Copy, not reveal: the key stays masked on screen while still being
        // handed over. It confirms in place rather than firing a toast.
        auto* copy = new QToolButton;
        copy->setProperty("uiRole", QStringLiteral("link"));
        copy->setAutoRaise(true);
        copy->setCursor(Qt::PointingHandCursor);
        copy->setFocusPolicy(Qt::TabFocus);
        copy->setText(tr("Copy"));
        copy->setAccessibleName(tr("Copy the full license key to the clipboard"));
        const QString fullKey = info.key;
        connect(copy, &QToolButton::clicked, copy, [copy, fullKey]() {
            QGuiApplication::clipboard()->setText(fullKey);
            copy->setText(tr("Copied"));
            QTimer::singleShot(2000, copy, [copy]() { copy->setText(tr("Copy")); });
        });
        addFact(tr("Key"),
                vth::models::LicenseValidation::maskKey(info.key),
                QString(), copy);

        addFact(tr("Activations"),
                info.activationLimit > 0
                    ? tr("%1 of %2 in use").arg(info.activationUsage).arg(info.activationLimit)
                    : tr("%1 in use").arg(info.activationUsage),
                tr("Deactivate a PC to use the license on another one."),
                nullptr);
    }

    if (haveKey) {
        const QDateTime act = QDateTime::fromString(info.activatedAt, Qt::ISODate);
        const bool haveActivated = act.isValid();
        const bool haveChecked   = info.lastVerifiedUtcMs > 0;

        // Only when the section has something in it.
        if (haveActivated || haveChecked)
            m_accountList->addHeading(tr("This PC"));

        if (haveActivated) {
            addFact(tr("Activated"),
                    QLocale().toString(act.toLocalTime().date(), QLocale::LongFormat),
                    QString(), nullptr);
        }

        if (haveChecked) {
            // fromMSecsSinceEpoch already yields local time in Qt 6.
            const QDateTime seen = QDateTime::fromMSecsSinceEpoch(info.lastVerifiedUtcMs);
            QFrame* row = addFact(tr("Last checked"), accountElapsed(seen),
                                  tr("Vethida checks every hour, and needs an internet "
                                     "connection to keep your plan active."),
                                  nullptr);
            row->setToolTip(QLocale().toString(seen, QLocale::LongFormat));
        }
    }

    m_accountList->addHeading(tr("Manage"));

    {
        // Four destinations across one row, each the width of its own label.
        auto* manage = new QWidget;
        auto* manageRow = new QHBoxLayout(manage);
        manageRow->setContentsMargins(0, 0, 0, 0);
        manageRow->setSpacing(tokens::CompactGap);

        const auto addDestination = [this, manage, manageRow](const QString& label,
                                                              const QString& glyph,
                                                              auto handler) {
            auto* row = new vth::widgets::SettingsRow(label, QString(), manage);
            row->setInteractive(true);
            // These rows go somewhere rather than turning something on.
            if (auto* sw = row->toggle())
                sw->hide();
            // One line each, because a row sized to its label wraps early.
            if (auto* title = row->findChild<QLabel*>(QStringLiteral("settingsRowTitle")))
                title->setWordWrap(false);
            row->addTrailing(new vth::widgets::GlyphLabel(
                glyph, &vth::theme::palette::textMuted,
                vth::theme::tokens::GlyphMd, row));
            connect(row, &vth::widgets::SettingsRow::bodyClicked, this, handler);
            // Its label's width, or the four divide the card between them.
            row->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
            manageRow->addWidget(row, 0, Qt::AlignVCenter);
        };

        // The glyph says where the row goes: out to a browser, or to a panel
        // inside the app.
        const QString kLeaves = QStringLiteral("link");
        const QString kStays  = QStringLiteral("chevron-right");

        // The first changes with the state, because the next step does.
        addDestination(
            haveKey ? tr("Your license keys") : tr("Find your license key"),
            kLeaves,
            []() {
                QDesktopServices::openUrl(QUrl(vth::site::kPortalLicenses));
            });

        addDestination(tr("Billing"), kLeaves, []() {
            QDesktopServices::openUrl(QUrl(vth::site::kPortalBilling));
        });
        addDestination(tr("Support"), kLeaves, []() {
            QDesktopServices::openUrl(QUrl(vth::site::kSupport));
        });
        addDestination(tr("Open-source licenses"), kStays, [this]() {
            vth::dialogs::showLicensesModal(this);
        });

        // The slack goes to the right of the last button rather than into the
        // buttons, which is what keeps each one its label's width.
        manageRow->addStretch(1);
        m_accountList->addWidget(manage);
    }
}

void MainWindow::applyEntitlement()
{
    // The rail first, because a gated destination doesn't depend on the controller.
    if (m_sidebar)
        m_sidebar->refreshTierGates();

    if (!m_tweakController)
        return;
    bool earlyOptIn = false;
    if (auto* settings = vth::ServiceRegistry::instance().get<vth::Settings>())
        earlyOptIn = settings->value(QStringLiteral("app/earlyAccessOptIn"), false).toBool();
    m_tweakController->setEntitlement(vth::currentTier(),
                                      earlyOptIn || vth::devUnlockAll());
}

// A second launch of Vethida lands here instead of opening another window.
void MainWindow::bringToFront()
{
    // showNormal only when actually minimized: on a maximized window it
    // restores down.
    if (isMinimized())
        showNormal();
    raise();
    // Windows only honors this from a process owning the foreground or granted it.
    activateWindow();
}

void MainWindow::showActivateKeyModal(const QString& prefillKey)
{
    vth::dialogs::runActivation(this, m_licenseService, prefillKey);
}

vth::platform::NativeWindowChrome::HitArea
MainWindow::titleBarHitTest(const QPoint& clientPos) const
{
    using HitArea = vth::platform::NativeWindowChrome::HitArea;
    if (!m_topBar)
        return HitArea::Client;

    const QPoint barPos = m_topBar->mapFrom(this, clientPos);
    if (!m_topBar->rect().contains(barPos))
        return HitArea::Client;

    // The caption group first, the only children the OS resolves rather than Qt.
    if (m_captionButtons) {
        switch (m_captionButtons->buttonAt(m_captionButtons->mapFrom(m_topBar, barPos))) {
        case vth::platform::CaptionButton::Close:
            return HitArea::CloseButton;
        case vth::platform::CaptionButton::MaximizeRestore:
            return HitArea::MaximizeButton;
        case vth::platform::CaptionButton::Minimize:
            return HitArea::MinimizeButton;
        case vth::platform::CaptionButton::None:
            break;
        }
    }

    for (QWidget* w = m_topBar->childAt(barPos); w && w != m_topBar; w = w->parentWidget()) {
        if (qobject_cast<QAbstractButton*>(w))
            return HitArea::Client;
    }
    return HitArea::Caption;
}

void MainWindow::applyCaptionButtonState(vth::platform::CaptionButton hovered,
                                         vth::platform::CaptionButton pressed)
{
    if (m_captionButtons)
        m_captionButtons->applyState(hovered, pressed);
    refreshCaptionIcons();
}

void MainWindow::applyNativeWindowChrome()
{
    if (!m_chrome) {
        // winId() forces the window handle to exist.
        (void)winId();
        QWindow* handle = windowHandle();
        if (!handle)
            return;
        m_chrome = std::make_unique<vth::platform::NativeWindowChrome>(handle);
        m_chrome->setHitTestProvider([this](const QPoint& pt) { return titleBarHitTest(pt); });
        m_chrome->setButtonStateSink([this](vth::platform::CaptionButton hovered,
                                            vth::platform::CaptionButton pressed) {
            applyCaptionButtonState(hovered, pressed);
        });
        m_chrome->setRelayoutSink([this]() { refreshCaptionIcons(); });
        m_chrome->install(vth::theme::palette::isDark());
    } else {
        m_chrome->applyTheme(vth::theme::palette::isDark());
    }
}

void MainWindow::refreshCaptionIcons()
{
    if (m_captionButtons)
        m_captionButtons->refreshIcons(isActiveWindow(), isMaximized());
}

void MainWindow::restoreUiState()
{
    const QByteArray geometry = m_settings.windowGeometry();
    if (!geometry.isEmpty())
        restoreGeometry(geometry);

    // Through navigateTo, so the sidebar selection follows. An unknown route no-ops.
    navigateTo(vth::prefs::restoreLastPage() ? m_settings.selectedRoute()
                                             : QStringLiteral("dashboard"));
}

void MainWindow::saveUiState()
{
    m_settings.setWindowGeometry(saveGeometry());
}

