#pragma once

#include "../../vth_core/Speedtest.h"

#include <QVector>
#include <QWidget>

#include <memory>

class QGridLayout;
class QLabel;
class QProgressBar;
class QThread;
class QVBoxLayout;

namespace vth::widgets
{
    class CommandButton;

    // The connection measurement and its result, as the body of a host card.
    class SpeedTestView final : public QWidget
    {
        Q_OBJECT

    public:
        explicit SpeedTestView(QWidget* parent = nullptr);
        ~SpeedTestView() override;

    private:
        void buildActions(QVBoxLayout* into);
        void buildMeasurements(QVBoxLayout* into);

        void startTest();
        void cancelTest();
        void handleProgress(const vth::win::speedtest::Result& result);
        void handleFinished(const vth::win::speedtest::Result& result);
        void renderResult(const vth::win::speedtest::Result& result);
        void renderGrade(const vth::win::speedtest::Result& result);
        void setRunning(bool running);
        void resetValues();

        // extraOut = optional third column (latency rise or the bufferbloat grade)
        void addMeasureRow(QGridLayout* grid, int row, const QString& caption,
                           QLabel** valueOut, QLabel** extraOut);

        QLabel*  m_intro       = nullptr;
        QWidget* m_resultBlock = nullptr;

        CommandButton* m_startButton  = nullptr;
        CommandButton* m_cancelButton = nullptr;
        QProgressBar*  m_progressBar  = nullptr;
        QLabel*        m_statusLabel  = nullptr;

        QLabel* m_downloadValue = nullptr;
        QLabel* m_uploadValue   = nullptr;

        QLabel* m_idleValue      = nullptr;
        QLabel* m_downLatValue   = nullptr;
        QLabel* m_downLatDelta   = nullptr;
        QLabel* m_upLatValue     = nullptr;
        QLabel* m_upLatDelta     = nullptr;

        QLabel*          m_gradeChip     = nullptr;
        QLabel*          m_gradeHeadline = nullptr;
        QLabel*          m_gradeMeaning  = nullptr;

        // A test is in flight. Decides whether an unmeasured figure reads as a
        // dash (nothing to show) or as "Measuring" (being measured right now).
        bool     m_running = false;
        QThread* m_worker  = nullptr;
        std::unique_ptr<vth::win::speedtest::CancelToken> m_cancelToken;
    };
}
