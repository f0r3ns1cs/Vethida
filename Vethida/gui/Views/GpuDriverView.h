#pragma once

#include "../../vth_core/Runtime/Fetch.h"

#include <QWidget>

#include <atomic>
#include <memory>

namespace vth::widgets { class CommandButton; }

class QLabel;
class QProgressBar;
class QStackedWidget;

namespace vth::ui
{
    // The NVIDIA clean-and-reinstall pipeline, as the body of the GPU page's
    // "Driver reinstall" section.
    class GpuDriverView : public QWidget
    {
        Q_OBJECT

    public:
        explicit GpuDriverView(QWidget* parent = nullptr);
        ~GpuDriverView() override;

    private Q_SLOTS:
        void onBegin();

    private:
        void buildUi();
        QWidget* buildCheckRow(const QString& text);
        bool confirmReinstall();

        void startReinstall();
        // Finishes a run that removed the old driver but did not get the new one
        // in, from the package it kept. No download.
        void finishInterrupted();
        void restartNow();
        void showProgress(const QString& label, int percent, qint64 received, qint64 total);
        void finish(bool ok, const QString& message);

        QStackedWidget*          actionStack_  = nullptr;  // 0 = button, 1 = progress
        vth::widgets::CommandButton* beginBtn_ = nullptr;
        vth::widgets::CommandButton* cancelBtn_ = nullptr;
        QLabel*                  stageLabel_   = nullptr;
        QLabel*                  bytesLabel_   = nullptr;
        QLabel*                  percentLabel_ = nullptr;
        QLabel*                  resultLabel_  = nullptr;
        QProgressBar*            progress_     = nullptr;
        QWidget*                 recovery_     = nullptr;  // unfinished-run callout
        vth::widgets::CommandButton* restartBtn_ = nullptr;

        // Shared with the worker, so canceling from the UI thread is visible
        // inside a download loop that may run for minutes.
        std::shared_ptr<std::atomic<bool>> cancel_;
        FetchToken runToken_;
    };
}
