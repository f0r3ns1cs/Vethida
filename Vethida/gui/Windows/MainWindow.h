#pragma once

#include "../Settings/SettingsStore.h"
#include "NativeWindowChrome.h"

#include <QHash>
#include <QList>
#include <QMainWindow>
#include <QPoint>
#include <QPointer>
#include <QString>

#include <memory>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

class QFrame;
class QHBoxLayout;
class QLabel;
class QVBoxLayout;
class QAction;
class QToolButton;
namespace vth::widgets { class CommandButton; }

namespace vth::widgets
{
    class CaptionButtons;
    class GlyphLabel;
    class LicenseBadge;
    class NavigationSidebar;
    class SettingsList;
    class ToastManager;
}

namespace vth::ui
{
    class TweakController;
}

namespace vth::services
{
    class LicenseService;
}

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    bool m_activateModalOpen = false;
    // Required, so it's a reference rather than a ServiceRegistry lookup.
    explicit MainWindow(vth::services::LicenseService& licenseService,
                        QWidget* parent = nullptr);
    ~MainWindow() override;

    // Brings this window forward; a second launch of Vethida asks for it.
    void bringToFront();

private:
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;
    void changeEvent(QEvent* event) override;
    void showEvent(QShowEvent* event) override;

    // Builds the pages that construct themselves on first visit, one per idle
    // moment after startup, so no first click pays for it.
    void prewarmPages();
    bool m_prewarmStarted = false;
    QList<QPointer<QWidget>> m_prewarmQueue;
    void closeEvent(QCloseEvent* event) override;

    void configureTopBar();
    void configureHelpMenu();
    // Rebuilds the title-bar license badge. Called on every licenseStateChanged and once
    // at startup.
    void refreshLicenseBadge();
    void showAboutModal();
    // Opens WinSparkle's update window. Settings and the About modal share it.
    void checkForUpdates();
    void configureNavigation();
    void navigateTo(const QString& routeId);
    void configureSettingsPage();
    // WinSparkle and the withdrawn-tweaks feed.
    void configureUpdateServices();
    void configureAccountPage();
    // Rebuilds the account list from the current license.
    void refreshAccountPage();
    // call on every license change
    void applyEntitlement();
    void refreshUpdateUi();
    // Settings -> Notifications: how long a toast stays up.
    void applyToastDuration(int index);
    // The activation card: a license key field. A prefilled key only fills the
    // field; the customer still presses Activate.
    void showActivateKeyModal(const QString& prefillKey = QString());
    void configureNotifications();
    vth::platform::NativeWindowChrome::HitArea titleBarHitTest(const QPoint& clientPos) const;
    void applyCaptionButtonState(vth::platform::CaptionButton hovered,
                                 vth::platform::CaptionButton pressed);
    void applyNativeWindowChrome();
    void refreshCaptionIcons();
    void restoreUiState();
    void saveUiState();

    bool m_wasOffline = false;

    Ui::MainWindow* ui = nullptr;
    QFrame* m_topBar = nullptr;
    QToolButton* m_titleHelpButton = nullptr;
    // The license state, in the title bar. A button, because every state has the same
    // next step: the account page.
    vth::widgets::LicenseBadge* m_titleLicenseBadge = nullptr;
    // Minimize, maximize and close.
    vth::widgets::CaptionButtons* m_captionButtons = nullptr;
    vth::ui::TweakController* m_tweakController = nullptr;   // shared by all tweak surfaces
    vth::services::LicenseService& m_licenseService;
    vth::settings::SettingsStore m_settings;
    std::unique_ptr<vth::platform::NativeWindowChrome> m_chrome;
    std::unique_ptr<vth::widgets::NavigationSidebar> m_sidebar;
    std::unique_ptr<vth::widgets::ToastManager> m_toasts;
    QHash<QString, int> m_routeIndex;

    vth::widgets::CommandButton* m_updateBtn         = nullptr;
    QLabel*        m_updateStatus      = nullptr;

    // Rebuilt by refreshAccountPage rather than updated in place.
    QWidget*                    m_accountPage = nullptr;
    vth::widgets::SettingsList* m_accountList = nullptr;
};
