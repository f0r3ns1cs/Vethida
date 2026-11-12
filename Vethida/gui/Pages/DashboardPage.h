#pragma once

#include "../Widgets/StorageHeatmap.h"

#include "../../vth_core/Runtime/Fetch.h"
#include "../../vth_core/SystemInfo.h"
#include "../../vth_core/Windows/BiosParser.h"
#include "../../vth_core/Windows/HealthScore.h"
#include "../../vth_core/Windows/Genesis.h"
#include "../../vth_core/Windows/Storage.h"

#include <QString>
#include <QStringList>
#include <QVector>
#include <QWidget>

class QGridLayout;
class QLabel;
class QTimer;
class QStackedWidget;
class QVBoxLayout;

namespace vth::widgets
{
    class CommandButton;
    class DashboardCard;
    class GlyphLabel;
    class HealthScoreGauge;
    class PowerToggle;
    class StatusStrip;
    class StorageHeatmap;
}

namespace vth::ui
{
    class Sparkline;
    class Dropdown;
    class TweakController;

    // Landing page. A three-column grid, two rows, every width fixed.
    class DashboardPage final : public QWidget
    {
        Q_OBJECT

    public:
        explicit DashboardPage(QWidget* parent = nullptr);
        ~DashboardPage() override;

    Q_SIGNALS:
        // Emitted by row and button actions; MainWindow routes it to the page.
        void navigateRequested(const QString& routeId);

    protected:
        // The score reads Windows settings this page doesn't own, so it re-evaluates on
        // return.
        void showEvent(QShowEvent* event) override;

    private:
        void buildUi();

        // Live CPU / GPU / memory trend, labeled with the real hardware.
        QWidget* buildMonitorCard();
        void     tickMonitor();
        // Adds GPU to the chart once a worker has found a GPU counter.
        void     enableGpuMetric();
        Sparkline* makeSpark();
        void     refreshHardwareNames();

        // Health: the score for this machine, and the panel explaining it.
        QWidget* buildHealthCard();
        void     refreshHealthScore();
        void     showScoreInfo();
        // The right-hand column of that panel: what this PC is, component by component.
        void     buildMachineReport(QWidget* host, QVBoxLayout* into,
                                    const vth::win::health::Report& report);

        // Drive usage, its breakdown, and the deep cleanup.
        QWidget* buildStorageCard();

        // How finely the category table is cut.
        enum class Detail { Merged, Full };

        // One definition of the category table, for the card's bar and the detail modal's.
        static QVector<vth::widgets::StorageHeatmap::Segment>
            segmentsFor(const vth::win::StorageBreakdown& breakdown, Detail detail);

        // Rebuilds the color key under the bar from the segments it was given.
        void rebuildLegend(const QVector<vth::widgets::StorageHeatmap::Segment>& segments);
        // Empties the key and centers a spinner in it, for the stretch between
        // selecting a drive and the category walk returning.
        void setLegendLoading();
        // Drops every tile without putting anything in their place.
        void clearLegend();
        void     populateDrives();
        QString  selectedDriveRoot() const;
        void     refreshDiskSpace();
        void     onCleanupClicked();
        void     showStorageInfo();

        // The getting-started checklist, and the support card beside it.
        QWidget* buildStartCard();

        // Genesis: the state, and the switch that changes it.
        QWidget* buildGenesisCard();
        // Opens the manager switches as a tool modal.
        void     openGenesisConfig();
        void     refreshGenesis();
        void     updateGenesisStatus();
        void     onGenesisToggled(bool on);

        // Painted from m_genesisState rather than from what was clicked, so a refused or
        // failed operation needs no snap-back.
        vth::widgets::PowerToggle*   m_genesisPower = nullptr;
        // The state, said twice so it never rests on hue alone.
        QLabel*                      m_genesisDot = nullptr;
        QLabel*                      m_genesisStateWord = nullptr;
        // The right-hand value of each stat row. What a running session actually
        // did, and what a stopped one will do.
        QLabel*                      m_genesisApps = nullptr;
        QLabel*                      m_genesisServices = nullptr;
        QLabel*                      m_genesisNotices = nullptr;
        // Read off a file, so refreshGenesis fetches it on a worker.
        vth::win::genesis::State           m_genesisState;
        // How many programs the exception list covers, protected floor included.
        int                                m_genesisKeptOpen = 0;
        vth::FetchToken                    m_genesisToken;
        void     refreshChecklist();

        // The status strip above the grid, and the one function that decides its state.
        vth::widgets::StatusStrip* m_strip = nullptr;
        void refreshStatusStrip();
        // The close control. Advances the informational rotation when what was
        // showing came from it, then puts the strip away for this session.
        void onStripDismissed();
        // Closed by hand, for this session.
        bool m_stripDismissed = false;
        // Whether what's on screen came from the rotation.
        bool m_stripRotating = false;
        // Ticks a step and persists it. A step is done once you have opened it.
        void     markStepVisited(const QString& route);
        static QString stepKey(const QString& route);
        static void setStepDone(vth::widgets::GlyphLabel* mark, bool done);

        // System Monitor.
        Sparkline*      m_cpuSpark   = nullptr;
        Sparkline*      m_gpuSpark   = nullptr;
        Sparkline*      m_memSpark   = nullptr;
        QStackedWidget* m_perfStack  = nullptr;
        Dropdown*       m_metricSel  = nullptr;
        QLabel*         m_perfValue  = nullptr;
        QLabel*         m_perfSubtitle = nullptr;   // the selected part's real name
        QTimer*         m_monitorTimer = nullptr;
        bool            m_gpuAvailable = false;
        // A sample is on a worker; the next tick waits for it.
        bool            m_sampling     = false;
        QString         m_gpuName;
        // Hardware names indexed to match the metric dropdown, so the reading
        // row can say "Ryzen 7 5800X3D" instead of "CPU".
        QVector<QString> m_hardwareNames;
        // The snapshot the names came from, for the Health panel. Empty until m_haveSysInfo.
        vth::win::SystemInfo m_sysInfo;
        bool                 m_haveSysInfo = false;
        // SMBIOS, for the same panel: the board, the chassis and the memory modules.
        vth::win::biosparser::Snapshot m_smbios;

        // Health. The last report is kept so the breakdown modal renders from cache.
        vth::widgets::HealthScoreGauge* m_gauge = nullptr;
        vth::win::health::Report m_lastReport;
        bool                     m_haveReport = false;

        // Storage. The drive list and last breakdown are kept for the detail modal.
        vth::widgets::StorageHeatmap* m_heatmap = nullptr;
        // legend under the bar, rebuilt when the breakdown arrives
        QWidget*         m_legend        = nullptr;
        QGridLayout*     m_legendGrid    = nullptr;
        Dropdown*        m_diskSel       = nullptr;
        vth::widgets::CommandButton*   m_cleanupBtn    = nullptr;
        QLabel*          m_diskFree      = nullptr;   // "X free of Y"
        QVector<QString> m_driveRoots;
        QVector<vth::win::DriveInfo>  m_drives;
        vth::win::StorageBreakdown    m_lastBreakdown;
        bool                          m_haveBreakdown = false;

        // Get started. One tick mark per step, in the order buildStartCard adds them.
        QVector<vth::widgets::GlyphLabel*> m_stepIcons;
        // Parallel to m_stepIcons: the route each step opens, which is also the
        // key its tick persists under.
        QStringList      m_stepRoutes;

        vth::FetchToken m_diskToken;
        vth::FetchToken m_spaceToken;
        vth::FetchToken m_drivesToken;
        vth::FetchToken m_healthToken;
        vth::FetchToken m_hardwareToken;
    };
}
