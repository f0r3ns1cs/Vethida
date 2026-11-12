#include "pch.h"
#include "SpeedTestView.h"

#include "../Theme/DesignTokens.h"
#include "CommandButton.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMetaObject>
#include <QProgressBar>
#include <QPushButton>
#include <QStringList>
#include <QStyle>
#include <QThread>
#include <QVBoxLayout>
#include <QVariant>

#include <iterator>

namespace vth::widgets
{
    namespace
    {
        namespace st     = vth::win::speedtest;
        namespace tokens = vth::theme::tokens;

        QString formatMeasure(double value)
        {
            if (value < 0.0)
                return QStringLiteral("-");
            return QString::number(value, 'f', value >= 100.0 ? 0 : 1);
        }

        QString formatMbps(double mbps)
        {
            return mbps <= 0.0
                ? QStringLiteral("-")
                : SpeedTestView::tr("%1 Mbps").arg(formatMeasure(mbps));
        }

        QString formatMs(double ms)
        {
            return ms < 0.0
                ? QStringLiteral("-")
                : SpeedTestView::tr("%1 ms").arg(formatMeasure(ms));
        }

        // The rise from rest to under-load. A loaded sample faster than the idle one is
        // noise, so it floors at zero.
        QString formatDelta(double idleMs, double loadedMs)
        {
            if (idleMs < 0.0 || loadedMs < 0.0)
                return QStringLiteral("-");
            return SpeedTestView::tr("+%1 ms").arg(formatMeasure(qMax(0.0, loadedMs - idleMs)));
        }

        QString formatStatus(const st::Result& result)
        {
            if (!result.statusMessage.isEmpty())
                return result.statusMessage;
            return st::statusLabel(result.status);
        }

        // What each grade means for the things people actually notice. Ordered
        // with kGradeLetters, so a change to the scale moves both together.
        QString gradeMeaning(int index)
        {
            switch (index) {
            case 0:  return SpeedTestView::tr(
                "Latency holds steady while the line is busy. Calls and games hold up "
                "during a large transfer.");
            case 1:  return SpeedTestView::tr(
                "Latency rises a little while the line is busy. Calls and games stay "
                "usable during a large transfer.");
            case 2:  return SpeedTestView::tr(
                "Latency rises noticeably while the line is busy. Calls break up and "
                "games stutter during a large transfer.");
            case 3:  return SpeedTestView::tr(
                "Latency rises sharply while the line is busy. Calls drop and games are "
                "unplayable during a large transfer.");
            case 4:  return SpeedTestView::tr(
                "Latency collapses while the line is busy. Anything interactive stops "
                "working during a large transfer.");
            default: break;
            }
            return QString();
        }

        QString gradeTone(int index)
        {
            if (index <= 1) return QStringLiteral("good");
            if (index == 2) return QStringLiteral("warn");
            return QStringLiteral("critical");
        }

        // A muted caption over a group inside the host card.
        QLabel* groupCaption(const QString& text, QWidget* parent)
        {
            auto* label = new QLabel(text, parent);
            label->setProperty("uiRole", QStringLiteral("metaText"));
            return label;
        }

        void restyle(QWidget* w)
        {
            w->style()->unpolish(w);
            w->style()->polish(w);
        }
    }

    SpeedTestView::SpeedTestView(QWidget* parent)
        : QWidget(parent)
    {
        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(0, 0, 0, 0);
        root->setSpacing(tokens::CardMargin);

        // What the test does, above the button that runs it, until it has been
        // run once; the results then take its place under the button.
        m_intro = new QLabel(
            tr("Measures throughput in both directions, then how much latency the "
               "connection adds while it is carrying that traffic."), this);
        m_intro->setProperty("uiRole", QStringLiteral("bodyText"));
        m_intro->setWordWrap(true);
        root->addWidget(m_intro);

        buildActions(root);

        m_resultBlock = new QWidget(this);
        auto* resultCol = new QVBoxLayout(m_resultBlock);
        resultCol->setContentsMargins(0, 0, 0, 0);
        resultCol->setSpacing(tokens::CardMargin);
        buildMeasurements(resultCol);
        root->addWidget(m_resultBlock);

        // Everything is constructed visible, so the idle state has to be applied.
        resetValues();
    }

    SpeedTestView::~SpeedTestView()
    {
        if (m_cancelToken)
            m_cancelToken->cancel();

        if (m_worker) {
            m_worker->wait();
            // The finished lambda that would delete it died with this view's connections.
            delete m_worker;
            m_worker = nullptr;
        }
    }

    void SpeedTestView::buildActions(QVBoxLayout* into)
    {
        auto* row = new QHBoxLayout;
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(tokens::CompactGap);

        m_startButton = new CommandButton(CommandButton::Variant::Primary,
                                          tr("Run test"), this);
        m_startButton->setCursor(Qt::PointingHandCursor);
        connect(m_startButton, &QPushButton::clicked, this, &SpeedTestView::startTest);
        row->addWidget(m_startButton);

        m_cancelButton = new CommandButton(CommandButton::Variant::Secondary,
                                           tr("Cancel"), this);
        m_cancelButton->setCursor(Qt::PointingHandCursor);
        connect(m_cancelButton, &QPushButton::clicked, this, &SpeedTestView::cancelTest);
        row->addWidget(m_cancelButton);
        row->addStretch(1);
        into->addLayout(row);

        m_progressBar = new QProgressBar(this);
        m_progressBar->setProperty("uiRole", QStringLiteral("progressBar"));
        m_progressBar->setRange(0, 1000);
        m_progressBar->setTextVisible(false);
        into->addWidget(m_progressBar);

        // Under the bar, so the bar's width doesn't jump with the status text.
        m_statusLabel = new QLabel(this);
        m_statusLabel->setProperty("uiRole", QStringLiteral("metaText"));
        into->addWidget(m_statusLabel);
    }

    void SpeedTestView::addMeasureRow(QGridLayout* grid, int row, const QString& caption,
                                      QLabel** valueOut, QLabel** extraOut)
    {
        grid->addWidget(groupCaption(caption, m_resultBlock), row, 0);

        auto* value = new QLabel(m_resultBlock);
        value->setProperty("uiRole", QStringLiteral("fieldValue"));
        grid->addWidget(value, row, 1);
        *valueOut = value;

        if (!extraOut)
            return;
        auto* extra = new QLabel(m_resultBlock);
        extra->setProperty("uiRole", QStringLiteral("metaText"));
        grid->addWidget(extra, row, 2);
        *extraOut = extra;
    }

    void SpeedTestView::buildMeasurements(QVBoxLayout* into)
    {
        auto* grid = new QGridLayout;
        grid->setContentsMargins(0, 0, 0, 0);
        grid->setHorizontalSpacing(tokens::CardMargin);
        grid->setVerticalSpacing(tokens::TightGap);
        grid->setColumnStretch(3, 1);

        addMeasureRow(grid, 0, tr("Download"), &m_downloadValue, nullptr);
        addMeasureRow(grid, 1, tr("Upload"), &m_uploadValue, nullptr);
        addMeasureRow(grid, 2, tr("At rest"), &m_idleValue, nullptr);
        addMeasureRow(grid, 3, tr("While downloading"), &m_downLatValue, &m_downLatDelta);
        addMeasureRow(grid, 4, tr("While uploading"), &m_upLatValue, &m_upLatDelta);
        addMeasureRow(grid, 5, tr("Bufferbloat"), &m_gradeHeadline, nullptr);

        m_gradeChip = new QLabel(m_resultBlock);
        m_gradeChip->setProperty("uiRole", QStringLiteral("chip"));
        grid->addWidget(m_gradeChip, 5, 2, Qt::AlignVCenter);

        into->addLayout(grid);

        m_gradeMeaning = new QLabel(m_resultBlock);
        m_gradeMeaning->setProperty("uiRole", QStringLiteral("bodyText"));
        m_gradeMeaning->setWordWrap(true);
        into->addWidget(m_gradeMeaning);
    }

    void SpeedTestView::startTest()
    {
        if (m_worker)
            return;

        resetValues();
        setRunning(true);
        m_cancelToken = std::make_unique<st::CancelToken>();
        auto* token = m_cancelToken.get();

        m_worker = QThread::create([this, token]() {
            const auto progress = [this](const st::Result& result) {
                QMetaObject::invokeMethod(this, [this, result]() {
                    handleProgress(result);
                }, Qt::QueuedConnection);
            };

            const st::Result final = st::run(progress, *token);
            QMetaObject::invokeMethod(this, [this, final]() {
                handleFinished(final);
            }, Qt::QueuedConnection);
        });

        connect(m_worker, &QThread::finished, this, [this]() {
            if (!m_worker)
                return;
            m_worker->deleteLater();
            m_worker = nullptr;
        });

        m_worker->start();
    }

    void SpeedTestView::cancelTest()
    {
        if (!m_cancelToken)
            return;

        m_cancelToken->cancel();
        m_cancelButton->setEnabled(false);
        m_statusLabel->setText(tr("Canceling..."));
    }

    void SpeedTestView::handleProgress(const st::Result& result)
    {
        m_progressBar->setValue(qBound(0, static_cast<int>(result.progress * 1000.0), 1000));
        m_statusLabel->setText(formatStatus(result));
        renderResult(result);
    }

    void SpeedTestView::handleFinished(const st::Result& result)
    {
        m_progressBar->setValue(qBound(0, static_cast<int>(result.progress * 1000.0), 1000));
        m_statusLabel->setText(formatStatus(result));
        renderResult(result);
        setRunning(false);
        m_cancelToken.reset();

        // A canceled or failed run keeps the line, because it says why the figures are
        // partial.
        m_statusLabel->setVisible(result.status != st::Status::Complete);
    }

    void SpeedTestView::renderResult(const st::Result& result)
    {
        m_intro->setVisible(false);
        m_resultBlock->setVisible(true);

        // A dash means nothing to show, so a figure still being measured says so.
        const auto pending = [this](const QString& text) {
            return (m_running && text == QLatin1String("-")) ? tr("Measuring") : text;
        };

        m_downloadValue->setText(pending(formatMbps(result.downloadMbps)));
        m_uploadValue  ->setText(pending(formatMbps(result.uploadMbps)));

        m_idleValue   ->setText(pending(formatMs(result.idleLatencyMs)));
        m_downLatValue->setText(pending(formatMs(result.loadedDownloadLatencyMs)));
        m_upLatValue  ->setText(pending(formatMs(result.loadedUploadLatencyMs)));
        m_downLatDelta->setText(formatDelta(result.idleLatencyMs,
                                            result.loadedDownloadLatencyMs));
        m_upLatDelta  ->setText(formatDelta(result.idleLatencyMs,
                                            result.loadedUploadLatencyMs));

        renderGrade(result);
    }

    void SpeedTestView::renderGrade(const st::Result& result)
    {
        const int index = st::gradeIndex(result.bufferbloatGrade);

        if (index < 0) {
            m_gradeChip->setVisible(false);
            m_gradeHeadline->setText(m_running ? tr("Measuring")
                                               : tr("Not measured yet"));
            m_gradeHeadline->setAccessibleName(
                m_running ? tr("Measuring bufferbloat")
                          : tr("Bufferbloat not measured yet"));
            m_gradeMeaning->clear();
            return;
        }

        const QString letter = result.bufferbloatGrade;
        const QString added  = formatMs(result.bufferbloatIncreaseMs);

        m_gradeChip->setVisible(true);
        m_gradeChip->setText(letter);
        m_gradeChip->setProperty("tone", gradeTone(index));
        restyle(m_gradeChip);

        m_gradeHeadline->setText(tr("%1 added under load").arg(added));
        // Spoken as a sentence: the chip beside it is a sibling QLabel, so the
        // letter is not otherwise announced with the number it belongs to.
        m_gradeHeadline->setAccessibleName(
            tr("Bufferbloat grade %1. %2 of latency added under load.").arg(letter, added));
        m_gradeMeaning->setText(gradeMeaning(index));
    }

    void SpeedTestView::setRunning(bool running)
    {
        m_startButton->setEnabled(!running);
        // Hidden, because a grayed Cancel on a card that never ran reads as broken.
        m_running = running;
        m_cancelButton->setVisible(running);
        m_cancelButton->setEnabled(running);
        m_progressBar->setVisible(running);
        m_statusLabel->setVisible(running);
    }

    void SpeedTestView::resetValues()
    {
        m_progressBar->setValue(0);
        m_statusLabel->clear();

        m_intro->setVisible(true);
        m_resultBlock->setVisible(false);

        setRunning(false);
    }
}
