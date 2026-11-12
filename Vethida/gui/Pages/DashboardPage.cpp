#include "pch.h"
#include "DashboardPage.h"

#include "../Settings/Preferences.h"

#include "SystemTools.h"

#include "../../vth_core/SiteUrls.h"

#include "../Widgets/CommandButton.h"
#include "../Widgets/DashboardCard.h"
#include "../Widgets/Dropdown.h"
#include "../Widgets/GlyphLabel.h"
#include "../Widgets/HealthScoreGauge.h"
#include "../Widgets/PowerToggle.h"
#include "../Widgets/Sparkline.h"
#include "../Widgets/Spinner.h"
#include "../Widgets/StatusStrip.h"
#include "../Widgets/StorageHeatmap.h"
#include "../Widgets/TierLockOverlay.h"
#include "../Dialogs/Confirm.h"
#include "../Dialogs/ToolModal.h"
#include "../Views/CleanupView.h"
#include "../Views/GenesisView.h"
#include "../Theme/DesignTokens.h"
#include "../Utils/TablerIcon.h"

#include "../../vth_core/Runtime/Fetch.h"
#include "../../vth_core/Runtime/ServiceRegistry.h"
#include "../../vth_core/Models/ProductTier.h"
#include "../../vth_core/Runtime/Tier.h"
#include "../../vth_core/Windows/Genesis.h"
#include "../../vth_core/Windows/GenesisMonitor.h"
#include "../../vth_core/Runtime/Settings.h"
#include "../../vth_core/SystemInfo.h"
#include "../../vth_core/Windows/BiosParser.h"
#include "../../vth_core/Services/CpuSampler.h"
#include "../../vth_core/Services/GpuSampler.h"
#include "../../vth_core/Services/MemorySampler.h"
#include "../../vth_core/Services/LicenseService.h"
#include "../../vth_core/Windows/Cleanup.h"
#include "../../vth_core/Windows/HealthScore.h"
#include "../../vth_core/Windows/Storage.h"

#include <QAbstractButton>
#include <QDesktopServices>
#include <QFontMetrics>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayoutItem>
#include <QStyle>
#include <QPainter>
#include <QPixmap>
#include <QPointer>
#include <QPushButton>
#include <QRectF>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QSize>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QTimer>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>

#include <functional>
#include <iterator>
#include <utility>
#include <tuple>
#include <vector>

namespace vth::ui
{
    namespace tokens = vth::theme::tokens;

    namespace
    {
        // The block is designed at its minimum size. The elide budgets, wrap caps and gauge
        // diameter below are computed at that size.
        constexpr int kWindowMinWidth = 1120;
        // Rail, the page's two margins, and 12px of headroom. The columns are built on 972.
        constexpr int kContentWidth =
            kWindowMinWidth - 88 - 2 * vth::theme::tokens::PageMarginH - 12;   // 972
        constexpr int kContentMaxWidth = 1440;
        constexpr int kGutter     = tokens::Space16;

        // Column width in a two-column modal at the minimum window size. Wrapped
        // QLabels need a max width or they report the wrong height and clip.
        constexpr int kModalRegionWidth = kWindowMinWidth - 88 - 12;      // 1020
        constexpr int kModalCardInner   = kModalRegionWidth
                                        - 2 * tokens::PanelModalMargin
                                        - 2 * tokens::DialogInsetWideH;   // 924
        constexpr int kModalColumnWidth = (kModalCardInner - tokens::Space32) / 2;

        // too narrow is fine, too wide clips
        void wrapAt(QLabel* label, int width)
        {
            label->setWordWrap(true);
            label->setMaximumWidth(width);
        }

        // Wraps only when the text is wider than its space, because a wrapped QLabel
        // reports a sizeHint narrower than its own text.
        void wrapIfLong(QLabel* label, const QString& text, int width)
        {
            const QFontMetrics fm(label->font());
            if (fm.horizontalAdvance(text) > width)
                wrapAt(label, width);
            else
                label->setWordWrap(false);
        }

        // A section heading inside a detail panel, with the gap above it.
        void modalHeading(QWidget* host, QVBoxLayout* into, const QString& text)
        {
            if (into->count() > 0)
                into->addSpacing(tokens::Space20);
            auto* h = new QLabel(text, host);
            h->setObjectName(QStringLiteral("dashboardSectionLabel"));
            into->addWidget(h);
            into->addSpacing(tokens::TightGap);
        }

        // empty glyph still reserves the icon column. tint is a function so it follows the theme.
        void modalItem(QWidget* host, QVBoxLayout* into, const QString& glyph,
                       vth::widgets::GlyphLabel::ColorFn tint, const QString& title,
                       const QString& detail)
        {
            auto* w = new QWidget(host);
            auto* h = new QHBoxLayout(w);
            h->setContentsMargins(0, tokens::TightGap, 0, tokens::TightGap);
            h->setSpacing(tokens::CompactGap);

            // On the center of the whole text block.
            if (glyph.isEmpty()) {
                auto* pad = new QWidget(w);
                pad->setFixedWidth(tokens::GlyphMd);
                h->addWidget(pad, 0, Qt::AlignVCenter);
            } else {
                h->addWidget(new vth::widgets::GlyphLabel(glyph, std::move(tint),
                                                          tokens::GlyphMd, w),
                             0, Qt::AlignVCenter);
            }

            auto* textCol = new QVBoxLayout;
            textCol->setContentsMargins(0, 0, 0, 0);
            textCol->setSpacing(tokens::Space2);
            auto* t = new QLabel(title, w);
            t->setObjectName(QStringLiteral("settingsRowTitle"));
            t->setWordWrap(true);
            textCol->addWidget(t);
            if (!detail.isEmpty()) {
                auto* d = new QLabel(detail, w);
                d->setObjectName(QStringLiteral("settingsRowDesc"));
                // Less the mark column and its gap, which share this row.
                wrapAt(d, kModalColumnWidth - tokens::GlyphMd - tokens::CompactGap);
                textCol->addWidget(d);
            }
            h->addLayout(textCol, 1);

            into->addWidget(w);
        }

        // A = Storage, B = Genesis, C = Health + Get started
        constexpr int kColA = 320;
        constexpr int kColB = 320;
        constexpr int kColC = kContentWidth - 2 * kGutter - kColA - kColB;   // 300

        // The chart spans A and B, so it is as wide as everything beneath it.
        constexpr int kChartWidth = kColA + kGutter + kColB;   // 656

        constexpr int kLeftCardInner  = kChartWidth - 2 * tokens::CardInsetH;   // 624
        // The health gauge, which is sized to the column it sits in.
        constexpr int kRightCardInner = kColC - 2 * tokens::CardInsetH;         // 268

        QString fmtGiB(quint64 bytes)
        {
            return QString::asprintf(
                "%.0f GB", static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0));
        }

        // The title lives outside the card; controls stay inside the card they
        // operate on. An empty title builds a card with no label at all.
        struct Titled
        {
            QWidget*                      wrap;   // add this to the column
            vth::widgets::DashboardCard*  card;
            QVBoxLayout*                  body;
            // The title row, for a pill beside the card's name. Null with no title.
            QHBoxLayout*                  head;
        };

        // bigger than the text, Tabler icons are inset in their viewBox
        constexpr int kHeadingGlyph = 26;

        // wrap width for the Genesis text
        constexpr int kGenesisTextWidth = kColB - 2 * tokens::CardInsetH;   // 288

        constexpr int kCardGaugeHeight = 228;
        // The modal's gauge takes a fixed box rather than its own sizeHint, which
        // is sized for the card above.
        constexpr int kModalGaugeWidth  = 300;
        constexpr int kModalGaugeHeight = 200;

        // Taller on the modal, where every segment has to be a hoverable target
        // rather than a hairline.
        constexpr int kModalBarHeight = 44;

        // Floor for the live chart card, which drives its whole grid row's height.
        constexpr int kMonitorCardHeight = 300;

        // RowList cards have a smaller inset, the rows have their own padding
        enum class CardInset { Standard, RowList };

        Titled makeTitledCard(QWidget* parent, const QString& title,
                              const QString& iconName = QString(),
                              CardInset inset = CardInset::Standard)
        {
            auto* wrap = new QWidget(parent);
            auto* col = new QVBoxLayout(wrap);
            col->setContentsMargins(0, 0, 0, 0);
            col->setSpacing(tokens::CompactGap);

            QHBoxLayout* head = nullptr;
            if (!title.isEmpty()) {
                head = new QHBoxLayout;
                head->setContentsMargins(0, 0, 0, 0);
                head->setSpacing(tokens::CompactGap);

                if (!iconName.isEmpty()) {
                    auto* mark = new vth::widgets::GlyphLabel(
                        iconName, &vth::theme::palette::accentOnSurface,
                        kHeadingGlyph, wrap);
                    head->addWidget(mark, 0, Qt::AlignVCenter);
                }

                auto* t = new QLabel(title, wrap);
                t->setObjectName(QStringLiteral("dashboardCardHeading"));
                head->addWidget(t, 0, Qt::AlignVCenter);
                head->addStretch(1);
                col->addLayout(head);
            }

            const bool rowList = inset == CardInset::RowList;
            const int insetH = rowList ? tokens::Space4 : tokens::CardInsetH;
            const int insetV = rowList ? tokens::Space4 : tokens::CardInsetV;

            auto* card = new vth::widgets::DashboardCard(
                QStringLiteral("dashboardStatusCard"), wrap);
            auto* body = new QVBoxLayout(card);
            body->setContentsMargins(insetH, insetV, insetH, insetV);
            body->setSpacing(tokens::CompactGap);
            col->addWidget(card, 1);

            return { wrap, card, body, head };
        }

        // The card's header line: its first readout on the left, its own controls
        // on the right, sharing one row.
        QHBoxLayout* makeCardHeader(QVBoxLayout* body)
        {
            auto* row = new QHBoxLayout;
            row->setContentsMargins(0, 0, 0, 0);
            row->setSpacing(tokens::CompactGap);
            body->addLayout(row);
            return row;
        }

        // The card's action line, at a stated height, so footers of different cards line
        // up on one grid row.
        QHBoxLayout* makeCardFooter(QWidget* card, QVBoxLayout* body)
        {
            auto* host = new QWidget(card);
            host->setFixedHeight(tokens::CardFooterHeight);
            auto* row = new QHBoxLayout(host);
            row->setContentsMargins(0, 0, 0, 0);
            row->setSpacing(tokens::CompactGap);
            body->addWidget(host);
            return row;
        }

        QToolButton* makeCardActionLink(QWidget* card, const QString& label,
                                        std::function<void()> onClick)
        {
            auto* b = new QToolButton(card);
            b->setText(label);
            b->setToolButtonStyle(Qt::ToolButtonTextOnly);
            b->setProperty("uiRole", QStringLiteral("link"));
            b->setAutoRaise(true);
            b->setCursor(Qt::PointingHandCursor);
            b->setAccessibleName(label);
            QObject::connect(b, &QToolButton::clicked, card,
                             [onClick = std::move(onClick)]() { onClick(); });
            return b;
        }

        // A key on the left, a value on the right. Returns the value label, so the card
        // can keep it current.
        QLabel* addStatRow(QWidget* host, QVBoxLayout* into, const QString& key)
        {
            auto* w = new QWidget(host);
            auto* h = new QHBoxLayout(w);
            h->setContentsMargins(0, tokens::Space2, 0, tokens::Space2);
            h->setSpacing(tokens::CardMargin);

            auto* k = new QLabel(key, w);
            k->setObjectName(QStringLiteral("settingsRowDesc"));
            h->addWidget(k, 0, Qt::AlignVCenter);
            h->addStretch(1);

            auto* v = new QLabel(w);
            v->setObjectName(QStringLiteral("dashboardReadoutValue"));
            v->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            h->addWidget(v, 0, Qt::AlignVCenter);

            into->addWidget(w);
            return v;
        }

        // Elides against a fixed budget so a long name cannot widen its column,
        // and keeps the full string in the tooltip.
        void setElided(QLabel* label, const QString& text, int maxWidth)
        {
            const QFontMetrics fm(label->font());
            label->setText(fm.elidedText(text, Qt::ElideRight, maxWidth));
            label->setToolTip(fm.horizontalAdvance(text) > maxWidth ? text : QString());
        }

        // painted, the colors come from the palette at runtime
        QPixmap colorDot(const QColor& color, int px, qreal dpr)
        {
            QPixmap pm(QSize(px, px) * dpr);
            pm.setDevicePixelRatio(dpr);
            pm.fill(Qt::transparent);
            QPainter p(&pm);
            p.setRenderHint(QPainter::Antialiasing, true);
            p.setPen(Qt::NoPen);
            p.setBrush(color);
            p.drawEllipse(QRectF(0, 0, px, px));
            return pm;
        }

        // clickable row: icon, title, optional reason, chevron. null leading = no icon column
        vth::widgets::DashboardCard* makeActionRow(
            QWidget* parent, QWidget* leading, const QString& title,
            const QString& reason, const QString& trailingGlyph,
            std::function<void()> onClick)
        {
            auto* row = new vth::widgets::DashboardCard(QStringLiteral("settingsRow"), parent);
            row->setProperty("interactive", true);
            row->setCursor(Qt::PointingHandCursor);
            row->setClickHandler(std::move(onClick));

            auto* lay = new QHBoxLayout(row);
            lay->setContentsMargins(tokens::RowInsetH, tokens::RowInsetV,
                                    tokens::RowInsetH, tokens::RowInsetV);
            lay->setSpacing(tokens::CardMargin);

            if (leading) {
                leading->setParent(row);
                leading->setAttribute(Qt::WA_TransparentForMouseEvents, true);
                lay->addWidget(leading, 0, Qt::AlignVCenter);
            }

            // Stretches above and below, so the text stays a block centered in a taller row.
            auto* textCol = new QVBoxLayout;
            textCol->setContentsMargins(0, 0, 0, 0);
            textCol->setSpacing(tokens::Space2);
            textCol->addStretch(1);
            auto* label = new QLabel(title, row);
            label->setObjectName(QStringLiteral("settingsRowTitle"));
            textCol->addWidget(label);
            if (!reason.isEmpty()) {
                auto* desc = new QLabel(reason, row);
                desc->setObjectName(QStringLiteral("settingsRowDesc"));
                desc->setWordWrap(true);
                textCol->addWidget(desc);
            }
            textCol->addStretch(1);
            lay->addLayout(textCol, 1);

            auto* glyph = new vth::widgets::GlyphLabel(
                trailingGlyph, &vth::theme::palette::textMuted, tokens::GlyphMd, row);
            lay->addWidget(glyph, 0, Qt::AlignVCenter);
            return row;
        }
    }

    DashboardPage::DashboardPage(QWidget* parent)
        : QWidget(parent)
    {
        // GPU joins the chart when the first sample finds a counter for it.
        // Asking here opened a PDH query on the UI thread during startup.
        buildUi();
        refreshHardwareNames();
        refreshHealthScore();
        refreshChecklist();
        refreshStatusStrip();

        // The tier decides whether the strip is on screen, so it follows the license.
        if (auto* licenses =
                vth::ServiceRegistry::instance().get<vth::services::LicenseService>()) {
            connect(licenses, &vth::services::LicenseService::licenseStateChanged,
                    this, &DashboardPage::refreshStatusStrip);
        }

        m_monitorTimer = new QTimer(this);
        m_monitorTimer->setInterval(1000);
        connect(m_monitorTimer, &QTimer::timeout, this, &DashboardPage::tickMonitor);
        tickMonitor();
        m_monitorTimer->start();
    }

    DashboardPage::~DashboardPage()
    {
        m_diskToken.cancel();
        m_spaceToken.cancel();
        m_drivesToken.cancel();
        m_healthToken.cancel();
        m_hardwareToken.cancel();
    }

    void DashboardPage::showEvent(QShowEvent* event)
    {
        QWidget::showEvent(event);
        // these change on other pages, refresh when we come back
        refreshHealthScore();
        refreshChecklist();
        refreshGenesis();
        refreshStatusStrip();
    }

    void DashboardPage::buildUi()
    {
        // No scroll area. Everything is pinned, and the window minimum (ShellMinHeight) is
        // derived from this page's arithmetic.
        auto* outer = new QVBoxLayout(this);
        outer->setContentsMargins(tokens::PageMarginH, tokens::PageMarginTop,
                                  tokens::PageMarginH, tokens::PageMarginBottom);
        outer->setSpacing(0);

        // A floor and a ceiling. The floor is the size the block is designed
        // at, which the enforced window minimum always leaves room for.
        auto* content = new QWidget(this);
        content->setMinimumWidth(kContentWidth);
        content->setMaximumWidth(kContentMaxWidth);

        auto* contentCol = new QVBoxLayout(content);
        contentCol->setContentsMargins(0, 0, 0, 0);
        contentCol->setSpacing(tokens::Space16);

        // Above the grid, the same width as it, and fixed at its own height: the
        // strip never grows and never takes a share of the page's spare height.
        m_strip = new vth::widgets::StatusStrip(content);
        connect(m_strip, &vth::widgets::StatusStrip::dismissed,
                this, &DashboardPage::onStripDismissed);
        contentCol->addWidget(m_strip, 0);

        auto* gridHost = new QWidget(content);
        contentCol->addWidget(gridHost, 1);

        // A grid, not independent stacks: cards in a row share a top edge and a
        // height, and the row below starts level.
        auto* grid = new QGridLayout(gridHost);
        grid->setContentsMargins(0, 0, 0, 0);
        grid->setHorizontalSpacing(kGutter);
        grid->setVerticalSpacing(tokens::Space16);

        // Row 0: the chart across A and B, the score beside it in C.
        // Row 1: Storage, Genesis, Get started - one card per column.
        grid->addWidget(buildMonitorCard(),   0, 0, 1, 2);
        grid->addWidget(buildHealthCard(),    0, 2);
        // No alignment flag, so each cell hands its widget the whole row height.
        grid->addWidget(buildStorageCard(),   1, 0);
        grid->addWidget(buildGenesisCard(),   1, 1);
        grid->addWidget(buildStartCard(),     1, 2);

        // Minimums, and stretch in the same ratio, so a wider block keeps its shape.
        grid->setColumnMinimumWidth(0, kColA);
        grid->setColumnMinimumWidth(1, kColB);
        grid->setColumnMinimumWidth(2, kColC);
        grid->setColumnStretch(0, kColA);
        grid->setColumnStretch(1, kColB);
        grid->setColumnStretch(2, kColC);
        // Only the chart row absorbs slack; the rows under it keep their content
        // height so nothing is stretched into looking oversized.
        grid->setRowStretch(0, 1);

        auto* centerRow = new QHBoxLayout;
        centerRow->setContentsMargins(0, 0, 0, 0);
        centerRow->setSpacing(0);
        // The block takes nearly all the width until it reaches its ceiling;
        // only then do the side stretches open up and center it.
        centerRow->addStretch(1);
        centerRow->addWidget(content, 100);
        centerRow->addStretch(1);

        // chart row takes the extra height
        outer->addLayout(centerRow, 1);
    }

    QWidget* DashboardPage::buildMonitorCard()
    {
        auto [wrap, card, col, head] = makeTitledCard(this, QString());
        // This also sets its grid row's height, and the card beside it has
        // nothing to fill the difference with - raising it leaves empty card.
        card->setMinimumHeight(kMonitorCardHeight);

        // GPU only if we can actually sample it
        m_metricSel = new vth::ui::Dropdown(card);
        m_metricSel->setAccessibleName(tr("Metric to chart"));
        m_metricSel->addItem(tr("CPU"));
        if (m_gpuAvailable)
            m_metricSel->addItem(tr("GPU"));
        m_metricSel->addItem(tr("Memory"));
        m_metricSel->setCurrentIndex(0);
        m_metricSel->setMaximumWidth(tokens::FieldWidthCompact);

        // The left label names the detected component once the system scan
        // lands; until then it is the generic metric name.
        auto* header = makeCardHeader(col);
        m_perfSubtitle = new QLabel(tr("CPU"), card);
        m_perfSubtitle->setObjectName(QStringLiteral("dashboardCardSubtitle"));
        // Room for the value and the selector beside it.
        m_perfSubtitle->setMaximumWidth(kLeftCardInner - 240);
        header->addWidget(m_perfSubtitle, 0, Qt::AlignVCenter);
        header->addStretch(1);
        m_perfValue = new QLabel(QStringLiteral("--"), card);
        m_perfValue->setObjectName(QStringLiteral("dashboardMonitorValue"));
        header->addWidget(m_perfValue, 0, Qt::AlignVCenter);
        header->addWidget(m_metricSel, 0, Qt::AlignVCenter);

        // Before the sparklines, which makeSpark() adds to it. GPU joins later,
        // in enableGpuMetric(), once a sample finds a GPU counter.
        m_perfStack = new QStackedWidget(card);
        m_cpuSpark = makeSpark();
        m_hardwareNames << tr("CPU");
        m_memSpark = makeSpark();
        m_hardwareNames << tr("Memory");
        col->addWidget(m_perfStack, 1);

        connect(m_metricSel, &vth::ui::Dropdown::currentIndexChanged, this,
                [this](int i) {
                    if (m_perfStack)
                        m_perfStack->setCurrentIndex(i);
                    if (m_perfSubtitle && i >= 0 && i < m_hardwareNames.size())
                        setElided(m_perfSubtitle, m_hardwareNames.at(i),
                                  m_perfSubtitle->maximumWidth());
                });

        return wrap;
    }

    void DashboardPage::refreshHardwareNames()
    {
        // Off the UI thread, in one fetch because both land in the same panel.
        struct Machine
        {
            vth::win::SystemInfo           info;
            vth::win::biosparser::Snapshot smbios;
        };

        m_hardwareToken.cancel();
        m_hardwareToken = vth::fetch<Machine>(
            this,
            []() {
                Machine m;
                m.info   = vth::win::collect();
                m.smbios = vth::win::biosparser::read();
                return m;
            },
            [this](Machine machine) {
                const vth::win::SystemInfo info = machine.info;
                m_sysInfo     = machine.info;
                m_smbios      = machine.smbios;
                m_haveSysInfo = true;

                const auto tidy = [](const QString& s) {
                    return vth::win::tidyHardwareName(s);
                };

                int i = 0;
                if (i < m_hardwareNames.size() && !info.cpuBrand.isEmpty())
                    m_hardwareNames[i] = tidy(info.cpuBrand);
                ++i;
                if (!info.gpuName.isEmpty())
                    m_gpuName = tidy(info.gpuName);
                if (m_gpuAvailable) {
                    if (i < m_hardwareNames.size() && !info.gpuName.isEmpty())
                        m_hardwareNames[i] = tidy(info.gpuName);
                    ++i;
                }
                if (i < m_hardwareNames.size() && info.ramTotalBytes > 0)
                    m_hardwareNames[i] = tr("Memory, %1").arg(fmtGiB(info.ramTotalBytes));

                const int current = m_metricSel ? m_metricSel->currentIndex() : 0;
                if (m_perfSubtitle && current >= 0 && current < m_hardwareNames.size())
                    setElided(m_perfSubtitle, m_hardwareNames.at(current),
                              m_perfSubtitle->maximumWidth());
            });
    }

    Sparkline* DashboardPage::makeSpark()
    {
        static const QStringList timeTicks = {
            tr("1 min"), tr("45 sec"), tr("30 sec"), tr("15 sec"), tr("now")
        };
        auto* s = new vth::ui::Sparkline(m_perfStack);
        s->setRange(0.0, 100.0);
        s->setXTicks(timeTicks);
        s->setValueFormatter([](double v) { return QString::asprintf("%.0f%%", v); });
        m_perfStack->addWidget(s);
        return s;
    }

    // GPU goes second, between CPU and Memory, with the stack and the names
    // kept in the dropdown's order. The selection stays on the same metric.
    void DashboardPage::enableGpuMetric()
    {
        if (m_gpuAvailable || !m_perfStack || !m_metricSel)
            return;
        m_gpuAvailable = true;

        const int was = m_metricSel->currentIndex();
        m_gpuSpark = makeSpark();
        m_perfStack->removeWidget(m_gpuSpark);
        m_perfStack->insertWidget(1, m_gpuSpark);
        m_hardwareNames.insert(1, m_gpuName.isEmpty() ? tr("GPU") : m_gpuName);

        const QSignalBlocker block(m_metricSel);
        m_metricSel->setItems({ tr("CPU"), tr("GPU"), tr("Memory") });
        const int now = was >= 1 ? was + 1 : 0;
        m_metricSel->setCurrentIndex(now);
        m_perfStack->setCurrentIndex(now);
    }

    void DashboardPage::tickMonitor()
    {
        // Task-Manager behavior: don't sample while another page is shown; the
        // rolling window resumes when the dashboard is visible again.
        if (!isVisible())
            return;

        // PDH reads walk every GPU engine instance, so they run on a worker and
        // the chart takes the answer a moment later.
        if (m_sampling)
            return;
        m_sampling = true;

        struct Sample { double cpu = 0, gpu = 0, mem = 0; bool gpuOk = false; };
        vth::fetch<Sample>(
            this,
            []() {
                Sample s;
                s.cpu   = vth::services::CpuSampler::sample();
                s.gpuOk = vth::services::GpuSampler::isAvailable();
                if (s.gpuOk)
                    s.gpu = vth::services::GpuSampler::sample();
                // MemorySampler returns (load, total, used); only the load is
                // plotted.
                s.mem = std::get<0>(vth::services::MemorySampler::sample());
                return s;
            },
            [this](Sample s) {
                m_sampling = false;
                if (s.gpuOk)
                    enableGpuMetric();

                if (m_cpuSpark) m_cpuSpark->appendSample(s.cpu);
                if (m_gpuSpark) m_gpuSpark->appendSample(s.gpu);
                if (m_memSpark) m_memSpark->appendSample(s.mem);

                // Index 1 is GPU only when GPU is listed; without a sampler the
                // dropdown is CPU/Memory and 1 means Memory.
                const int metric = m_metricSel ? m_metricSel->currentIndex() : 0;
                if (m_perfValue) {
                    const double shown = m_gpuAvailable
                        ? (metric == 1 ? s.gpu : metric == 2 ? s.mem : s.cpu)
                        : (metric == 1 ? s.mem : s.cpu);
                    m_perfValue->setText(QString::asprintf("%.0f%%", shown));
                }
            });
    }

    QWidget* DashboardPage::buildHealthCard()
    {
        auto [wrap, card, col, head] = makeTitledCard(this, QString());

        // The word is centered on the card, with the glyph hung off its left. The trailing
        // spacer matches the glyph and its gap.
        auto* titlePair = new QHBoxLayout;
        titlePair->setContentsMargins(0, 0, 0, 0);
        titlePair->setSpacing(0);
        titlePair->addStretch(1);

        auto* titleMark = new vth::widgets::GlyphLabel(
            QStringLiteral("activity"), &vth::theme::palette::accentOnSurface,
            kHeadingGlyph, card);
        titlePair->addWidget(titleMark, 0, Qt::AlignVCenter);
        titlePair->addSpacing(tokens::CompactGap);

        auto* title = new QLabel(tr("Health"), card);
        title->setObjectName(QStringLiteral("dashboardCardHeading"));
        titlePair->addWidget(title, 0, Qt::AlignVCenter);

        titlePair->addSpacing(kHeadingGlyph + tokens::CompactGap);
        titlePair->addStretch(1);

        col->addLayout(titlePair);

        // Sized to the card's inner width. The height sets the arc's diameter.
        m_gauge = new vth::widgets::HealthScoreGauge(card);
        m_gauge->setFixedSize(kRightCardInner, kCardGaugeHeight);
        m_gauge->setPending();

        // Centered in whatever height this card inherits from the chart beside it.
        col->addStretch(1);
        col->addWidget(m_gauge, 0, Qt::AlignHCenter);

        col->addStretch(1);

        col->addWidget(
            makeCardActionLink(card, tr("About this score"),
                               [this]() { showScoreInfo(); }),
            0, Qt::AlignHCenter);

        return wrap;
    }

    void DashboardPage::refreshHealthScore()
    {
        if (!m_gauge)
            return;

        m_healthToken.cancel();
        m_healthToken = vth::fetch<vth::win::health::Report>(
            this,
            []() { return vth::win::health::evaluate(); },
            [this](vth::win::health::Report r) {
                if (!m_gauge)
                    return;

                m_gauge->setScore(r.score);

                // Kept so the modal can render without re-running the scan.
                m_lastReport = r;
                m_haveReport = true;
            });
    }

    // Built from the cached report rather than re-running the scan: opening a
    // detail view must never cost a WMI query plus a catalog detection pass.
    void DashboardPage::showScoreInfo()
    {
        namespace health = vth::win::health;
        const health::Report r = m_lastReport;

        auto* content = new QWidget;
        auto* root = new QHBoxLayout(content);
        root->setContentsMargins(0, 0, 0, 0);
        root->setSpacing(tokens::Space32);

        if (!m_haveReport) {
            auto* pending = new QLabel(tr("Still measuring this PC..."), content);
            pending->setObjectName(QStringLiteral("overlayBody"));
            root->addWidget(pending, 1, Qt::AlignTop);
            vth::dialogs::showContentModal(this, tr("Health"), content,
                                           vth::widgets::PanelDialog::Mode::Cover);
            return;
        }

        const auto heading = [content](QVBoxLayout* into, const QString& text) {
            modalHeading(content, into, text);
        };

        // left: the score, and what it means.
        auto* left = new QVBoxLayout;
        left->setContentsMargins(0, 0, 0, 0);
        left->setSpacing(0);

        auto* gauge = new vth::widgets::HealthScoreGauge(content);
        gauge->setFixedSize(kModalGaugeWidth, kModalGaugeHeight);
        gauge->setScore(r.score);
        left->addWidget(gauge, 0, Qt::AlignHCenter);
        left->addSpacing(tokens::CardMargin);

        const auto paragraph = [content, left](const QString& text) {
            auto* p = new QLabel(text, content);
            p->setObjectName(QStringLiteral("settingsRowDesc"));
            wrapAt(p, kModalColumnWidth);
            left->addWidget(p);
        };

        heading(left, tr("What this score means"));
        paragraph(tr("How much of what Vethida can optimize on this PC is "
                     "already applied. A PC set up without Vethida starts low, and "
                     "the score rises as you apply tweaks."));
        left->addSpacing(tokens::RegionMargin);

        auto* open = new vth::widgets::CommandButton(
            vth::widgets::CommandButton::Variant::Primary, tr("Open Tweaks"),
            content);
        connect(open, &QPushButton::clicked, this, [this, content]() {
            // close the modal before navigating
            if (QWidget* modal = content->window())
                modal->close();
            Q_EMIT navigateRequested(QStringLiteral("tweaks"));
        });
        auto* openRow = new QHBoxLayout;
        openRow->setContentsMargins(0, 0, 0, 0);
        openRow->addWidget(open, 0, Qt::AlignLeft);
        openRow->addStretch(1);
        left->addLayout(openRow);
        left->addStretch(1);

        // right: what this machine actually is
        auto* right = new QVBoxLayout;
        right->setContentsMargins(0, 0, 0, 0);
        right->setSpacing(0);
        buildMachineReport(content, right, r);

        root->addLayout(left, 1);
        root->addLayout(right, 1);

        // Managed, because the machine report can run past the region's height.
        vth::dialogs::showContentModal(this, tr("Health"), content,
                                       vth::widgets::PanelDialog::Mode::Cover,
                                       vth::dialogs::ModalScroll::Managed);
    }

    // The hardware, as a support engineer or a PC builder wants it.
    void DashboardPage::buildMachineReport(QWidget* host, QVBoxLayout* into,
                                           const vth::win::health::Report& r)
    {
        const auto heading = [host](QVBoxLayout* col, const QString& text) {
            modalHeading(host, col, text);
        };

        // Only writes the row when there's something to say.
        const auto put = [host](QVBoxLayout* col, const QString& key,
                                const QString& value) {
            if (value.trimmed().isEmpty())
                return;
            auto* w = new QWidget(host);
            auto* h = new QHBoxLayout(w);
            h->setContentsMargins(0, tokens::Space2, 0, tokens::Space2);
            h->setSpacing(tokens::CardMargin);
            auto* k = new QLabel(key, w);
            k->setObjectName(QStringLiteral("settingsRowDesc"));
            h->addWidget(k, 0, Qt::AlignVCenter);
            h->addStretch(1);
            auto* v = new QLabel(value, w);
            v->setObjectName(QStringLiteral("dashboardReadoutValue"));
            wrapIfLong(v, value, kModalColumnWidth / 2);
            v->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            h->addWidget(v, 0, Qt::AlignVCenter);
            col->addWidget(w);
        };

        const vth::win::SystemInfo&           s  = m_sysInfo;
        const vth::win::biosparser::Snapshot& fw = m_smbios;

        heading(into, tr("This PC"));
        put(into, tr("Device name"), s.computerName);
        put(into, tr("Manufacturer"), fw.system.manufacturer);
        put(into, tr("Model"), fw.system.productName);
        put(into, tr("Windows"), s.osEdition.isEmpty() ? r.osName : s.osEdition);
        put(into, tr("Version"), s.osDisplayVersion);
        put(into, tr("Build"), s.osBuild);

        heading(into, tr("Processor"));
        put(into, tr("Model"), s.cpuBrand.isEmpty()
                ? vth::win::tidyHardwareName(r.cpuName)
                : vth::win::tidyHardwareName(s.cpuBrand));
        // Cores and threads on one row, because the pair is the fact.
        if (s.cpuPhysicalCores > 0 && s.cpuLogicalProcs > 0) {
            put(into, tr("Cores"), tr("%1 cores, %2 threads")
                    .arg(s.cpuPhysicalCores).arg(s.cpuLogicalProcs));
        } else if (s.cpuLogicalProcs > 0) {
            put(into, tr("Threads"), QString::number(s.cpuLogicalProcs));
        }
        if (s.cpuBaseClockMhz > 0)
            put(into, tr("Base speed"), tr("%1 MHz").arg(s.cpuBaseClockMhz));

        heading(into, tr("Memory"));
        if (s.ramTotalBytes > 0)
            put(into, tr("Installed"), fmtGiB(s.ramTotalBytes));
        else if (r.ramBytes > 0)
            put(into, tr("Installed"), fmtGiB(r.ramBytes));
        // memory modules, identical ones grouped. No sizes if they don't add up (soldered RAM)
        {
            QVector<const decltype(fw.memorySlots)::value_type*> modules;
            quint64 reported = 0;
            for (const auto& slot : fw.memorySlots) {
                if (slot.sizeBytes == 0)
                    continue;
                modules.push_back(&slot);
                reported += slot.sizeBytes;
            }
            const quint64 installed = s.ramTotalBytes > 0 ? s.ramTotalBytes : r.ramBytes;
            // Installed memory is reported slightly under the module total
            // (firmware reservations), so "adds up" allows a small gap.
            const bool addsUp = installed == 0
                || (reported >= installed && reported - installed < reported / 8)
                || (installed >= reported && installed - reported < installed / 8);
            auto typeAndSpeed = [this](const auto& slot) {
                QStringList parts;
                if (!slot.memoryType.isEmpty())
                    parts << slot.memoryType;
                if (slot.speedMtps > 0)
                    parts << tr("%1 MT/s").arg(slot.speedMtps);
                return parts;
            };
            bool identical = !modules.isEmpty();
            for (const auto* m : modules)
                identical = identical && m->sizeBytes == modules.first()->sizeBytes
                         && m->memoryType == modules.first()->memoryType
                         && m->speedMtps == modules.first()->speedMtps;

            if (!modules.isEmpty() && !addsUp) {
                const QStringList parts = typeAndSpeed(*modules.first());
                if (!parts.isEmpty())
                    put(into, tr("Type"), parts.join(QStringLiteral(", ")));
            } else if (identical) {
                QStringList parts = typeAndSpeed(*modules.first());
                parts.prepend(modules.size() > 1
                    ? tr("%1 x %2").arg(modules.size()).arg(fmtGiB(modules.first()->sizeBytes))
                    : fmtGiB(modules.first()->sizeBytes));
                put(into, modules.size() > 1 ? tr("Modules") : tr("Module"),
                    parts.join(QStringLiteral(", ")));
            } else {
                int n = 0;
                for (const auto* m : modules) {
                    QStringList parts = typeAndSpeed(*m);
                    parts.prepend(fmtGiB(m->sizeBytes));
                    put(into, tr("Slot %1").arg(++n), parts.join(QStringLiteral(", ")));
                }
            }
        }

        heading(into, tr("Graphics"));
        put(into, tr("Adapter"), s.gpuName.isEmpty()
                ? vth::win::tidyHardwareName(r.gpuName)
                : vth::win::tidyHardwareName(s.gpuName));
        if (s.displayWidth > 0 && s.displayHeight > 0) {
            put(into, tr("Resolution"),
                tr("%1 x %2").arg(s.displayWidth).arg(s.displayHeight));
        }
        if (s.displayRefreshHz > 0)
            put(into, tr("Refresh rate"), tr("%1 Hz").arg(s.displayRefreshHz));

        // The board and its firmware, as one subject: which motherboard, which
        // BIOS on it, and how it boots.
        heading(into, tr("Motherboard"));
        put(into, tr("Model"), fw.baseboard.manufacturer.isEmpty()
                ? fw.baseboard.product
                : tr("%1 %2").arg(fw.baseboard.manufacturer, fw.baseboard.product).trimmed());
        put(into, tr("BIOS version"), fw.bios.version);
        put(into, tr("Boot mode"), s.uefiBoot ? tr("UEFI") : tr("Legacy BIOS"));
        // A machine with no SecureBoot key booted a legacy BIOS, where it doesn't exist.
        put(into, tr("Secure Boot"),
            s.secureBootReadable ? (s.secureBoot ? tr("On") : tr("Off"))
                                 : tr("Not supported"));

        // Drives, off the list the Storage card already enumerated.
        if (!m_drives.isEmpty()) {
            heading(into, tr("Storage"));
            for (const auto& d : m_drives) {
                const QString letter = d.root.left(2);
                const QString name = d.label.isEmpty()
                    ? letter : tr("%1  %2").arg(letter, d.label);
                QStringList parts;
                parts << tr("%1 free of %2").arg(fmtGiB(d.freeBytes),
                                                 fmtGiB(d.totalBytes));
                if (!d.fileSystem.isEmpty())
                    parts << d.fileSystem;
                put(into, name, parts.join(QStringLiteral(", ")));
            }
        }

        into->addStretch(1);
    }

    QWidget* DashboardPage::buildStorageCard()
    {
        auto [wrap, card, col, head] = makeTitledCard(this, tr("Storage"),
                                               QStringLiteral("folder-cog"));
        // One gap between every element on this card.
        col->setSpacing(tokens::CardMargin);

        m_diskSel = new vth::ui::Dropdown(card);
        m_diskSel->setAccessibleName(tr("Drive"));
        m_diskSel->setMaximumWidth(tokens::FieldWidthCompact);

        m_diskFree = new QLabel(QString(), card);
        m_diskFree->setObjectName(QStringLiteral("dashboardHeadlineValue"));

        auto* header = makeCardHeader(col);
        header->addWidget(m_diskFree, 0, Qt::AlignVCenter);
        header->addStretch(1);
        header->addWidget(m_diskSel, 0, Qt::AlignVCenter);

        // Fixed, because past a dozen pixels the bar reads as a block.
        m_heatmap = new vth::widgets::StorageHeatmap(card);
        m_heatmap->setFixedHeight(tokens::Space32);
        col->addWidget(m_heatmap);

        // Two columns of single-line entries: one column of five is a tall thin
        // list on a card this narrow.
        m_legend = new QWidget(card);
        m_legendGrid = new QGridLayout(m_legend);
        m_legendGrid->setContentsMargins(0, 0, 0, 0);
        m_legendGrid->setHorizontalSpacing(tokens::CompactGap);
        m_legendGrid->setVerticalSpacing(tokens::TightGap);
        m_legendGrid->setColumnStretch(0, 1);
        m_legendGrid->setColumnStretch(1, 1);
        // A stretch either side, so the key sits centered between the bar and the footer.
        col->addStretch(1);
        col->addWidget(m_legend);
        col->addStretch(1);

        m_cleanupBtn = new vth::widgets::CommandButton(vth::widgets::CommandButton::Variant::Secondary,
                                         tr("Clean up"), card);
        m_cleanupBtn->setCursor(Qt::PointingHandCursor);
        m_cleanupBtn->setToolTip(tr("Choose what to remove from the selected drive"));
        connect(m_cleanupBtn, &QPushButton::clicked, this, &DashboardPage::onCleanupClicked);

        // The breakdown on the left as a link, the action on the right.
        auto* foot = makeCardFooter(card, col);
        foot->addWidget(makeCardActionLink(card, tr("What is using it"),
                                           [this]() { showStorageInfo(); }),
                        0, Qt::AlignBottom);
        foot->addStretch(1);
        foot->addWidget(m_cleanupBtn, 0, Qt::AlignBottom);

        // No refreshDiskSpace() here: the drive list arrives asynchronously and
        // its handler kicks off the first measurement once a drive is selected.
        populateDrives();
        return wrap;
    }

    // off the UI thread, a locked or sleeping drive can block for seconds
    void DashboardPage::populateDrives()
    {
        if (!m_diskSel)
            return;

        connect(m_diskSel, &vth::ui::Dropdown::currentIndexChanged, this,
                [this](int) { refreshDiskSpace(); });

        m_drivesToken.cancel();
        m_drivesToken = vth::fetch<std::vector<vth::win::DriveInfo>>(
            this,
            []() { return vth::win::fixedDrives(); },
            [this](std::vector<vth::win::DriveInfo> drives) {
                if (!m_diskSel)
                    return;
                m_driveRoots.clear();
                m_drives.clear();
                int sysIdx = 0;
                const QSignalBlocker block(m_diskSel);
                for (int i = 0; i < static_cast<int>(drives.size()); ++i) {
                    const auto& d = drives.at(static_cast<size_t>(i));
                    const QString letter = d.root.left(2);   // "C:"
                    const QString text = d.label.isEmpty()
                        ? letter : QStringLiteral("%1  %2").arg(letter, d.label);
                    m_diskSel->addItem(text, d.root);
                    m_driveRoots.push_back(d.root);
                    m_drives.push_back(d);
                    if (d.isSystem)
                        sysIdx = i;
                }
                if (m_driveRoots.isEmpty()) {
                    const QString sys = vth::win::systemDriveRoot();
                    m_diskSel->addItem(tr("System drive"), sys);
                    m_driveRoots.push_back(sys);
                }
                m_diskSel->setCurrentIndex(sysIdx);
                refreshDiskSpace();
            });
    }

    QString DashboardPage::selectedDriveRoot() const
    {
        QString root = m_diskSel ? m_diskSel->currentData().toString() : QString();
        if (root.isEmpty())
            root = vth::win::systemDriveRoot();
        return root;
    }

    void DashboardPage::refreshDiskSpace()
    {
        const QString root = selectedDriveRoot();

        // Free space on a worker too: a sleeping hard drive takes seconds to
        // answer GetDiskFreeSpaceEx.
        m_spaceToken.cancel();
        m_spaceToken = vth::fetch<vth::win::DriveSpace>(
            this,
            [root]() { return vth::win::driveSpace(root); },
            [this](vth::win::DriveSpace space) {
                const quint64 used = (space.valid && space.totalBytes >= space.freeBytes)
                    ? space.totalBytes - space.freeBytes : 0;

                if (m_diskFree)
                    m_diskFree->setText(tr("%1 free of %2")
                                            .arg(fmtGiB(space.freeBytes), fmtGiB(space.totalBytes)));

                // Used/free until the real categories land, so the bar is never
                // empty. Skipped once the breakdown is in.
                if (m_haveBreakdown || !m_heatmap)
                    return;
                QVector<vth::widgets::StorageHeatmap::Segment> initial;
                if (used > 0)
                    initial.push_back({ tr("Used"), used, vth::theme::palette::storageWindows() });
                if (space.freeBytes > 0)
                    initial.push_back({ tr("Free"), space.freeBytes, vth::theme::palette::storageFree() });
                m_heatmap->setSegments(initial, space.totalBytes);
            });

        // The key belongs to the breakdown, so it goes to its loading state on every
        // drive switch.
        m_haveBreakdown = false;
        setLegendLoading();

        // The recursive category breakdown is slow; measure it off-thread and
        // refine the bar once it lands.
        m_diskToken.cancel();
        m_diskToken = vth::fetch<vth::win::StorageBreakdown>(
            this,
            [root]() { return vth::win::driveBreakdown(root); },
            [this](vth::win::StorageBreakdown b) {
                if (!m_heatmap)
                    return;
                // unreadable drive still needs to stop the spinner
                if (!b.valid || b.totalBytes == 0) {
                    clearLegend();
                    return;
                }
                const auto segs = segmentsFor(b, Detail::Merged);
                m_heatmap->setSegments(segs, b.totalBytes);
                // Same segments into both, so a band and its key row can never
                // disagree about a color or a size.
                rebuildLegend(segs);
                m_lastBreakdown = b;
                m_haveBreakdown = true;
            });
    }

    QVector<vth::widgets::StorageHeatmap::Segment>
    DashboardPage::segmentsFor(const vth::win::StorageBreakdown& b, Detail detail)
    {
        namespace palette = vth::theme::palette;
        struct Cat { const char* name; quint64 bytes; QColor color; };

        // Merged combines categories that share a color, and keeps labels short
        const QVector<Cat> cats = detail == Detail::Full
            ? QVector<Cat>{
                { QT_TR_NOOP("Windows"),             b.windowsBytes,         palette::storageWindows() },
                { QT_TR_NOOP("Program Files"),       b.programFilesBytes,    palette::storageApps() },
                { QT_TR_NOOP("Program Files (x86)"), b.programFilesX86Bytes, palette::storageApps() },
                { QT_TR_NOOP("Users"),               b.usersBytes,           palette::storageUsers() },
                { QT_TR_NOOP("Page file"),           b.pageFileBytes,        palette::storageSystemFile() },
                { QT_TR_NOOP("Hibernation"),         b.hibernationBytes,     palette::storageSystemFile() },
                { QT_TR_NOOP("Other"),               b.otherBytes,           palette::storageOther() },
              }
            : QVector<Cat>{
                { QT_TR_NOOP("Windows"),      b.windowsBytes,                              palette::storageWindows() },
                { QT_TR_NOOP("Apps"),         b.programFilesBytes + b.programFilesX86Bytes, palette::storageApps() },
                { QT_TR_NOOP("Your files"),   b.usersBytes,                                palette::storageUsers() },
                { QT_TR_NOOP("System files"), b.pageFileBytes + b.hibernationBytes,         palette::storageSystemFile() },
                { QT_TR_NOOP("Other"),        b.otherBytes,                                palette::storageOther() },
              };

        QVector<vth::widgets::StorageHeatmap::Segment> segs;
        for (const auto& c : cats)
            if (c.bytes > 0)
                segs.push_back({ tr(c.name), c.bytes, c.color });
        if (b.freeBytes > 0)
            segs.push_back({ tr("Free"), b.freeBytes, palette::storageFree() });
        return segs;
    }

    // One key row: the segment's color, its name, and its size.
    void DashboardPage::rebuildLegend(
        const QVector<vth::widgets::StorageHeatmap::Segment>& segments)
    {
        clearLegend();
        if (!m_legendGrid || !m_legend)
            return;

        constexpr int kDot = tokens::DotSm;
        const qreal dpr = devicePixelRatioF();
        int cell = 0;
        for (const auto& s : segments) {
            // Free is the headline figure at the top of the card.
            if (s.name == tr("Free"))
                continue;

            // Caption size, so a dot, a name and a figure fit one line.
            auto* tile = new QWidget(m_legend);
            auto* tileRow = new QHBoxLayout(tile);
            tileRow->setContentsMargins(0, 0, 0, 0);
            tileRow->setSpacing(tokens::TightGap);

            QPixmap pm(QSize(kDot, kDot) * dpr);
            pm.setDevicePixelRatio(dpr);
            pm.fill(Qt::transparent);
            {
                QPainter p(&pm);
                p.setRenderHint(QPainter::Antialiasing, true);
                p.setPen(Qt::NoPen);
                p.setBrush(s.color);
                p.drawEllipse(QRectF(0, 0, kDot, kDot));
            }
            auto* dot = new QLabel(tile);
            dot->setFixedSize(kDot, kDot);
            dot->setPixmap(pm);
            tileRow->addWidget(dot, 0, Qt::AlignVCenter);

            auto* name = new QLabel(s.name, tile);
            name->setProperty("uiRole", QStringLiteral("metaText"));
            tileRow->addWidget(name, 0, Qt::AlignVCenter);

            auto* size = new QLabel(fmtGiB(s.bytes), tile);
            size->setObjectName(QStringLiteral("dashboardReadoutValue"));
            tileRow->addWidget(size, 0, Qt::AlignVCenter);

            // The slack goes after the pair, or the figure crowds the next tile.
            tileRow->addStretch(1);

            m_legendGrid->addWidget(tile, cell / 2, cell % 2);
            ++cell;
        }
    }

    void DashboardPage::clearLegend()
    {
        if (!m_legendGrid)
            return;
        while (QLayoutItem* item = m_legendGrid->takeAt(0)) {
            if (QWidget* w = item->widget())
                w->deleteLater();
            delete item;
        }
    }

    void DashboardPage::setLegendLoading()
    {
        clearLegend();
        if (!m_legendGrid || !m_legend)
            return;

        // small so the card doesn't change height when the data loads
        auto* spinner = new vth::widgets::Spinner(m_legend, tokens::Space24);
        m_legendGrid->addWidget(spinner, 0, 0, 1, 2, Qt::AlignCenter);
    }

    // uses the cached breakdown, the scan is too slow to redo here
    void DashboardPage::showStorageInfo()
    {
        const QString root = selectedDriveRoot();
        vth::win::DriveInfo info;
        for (const auto& d : m_drives) {
            if (d.root.compare(root, Qt::CaseInsensitive) == 0) {
                info = d;
                break;
            }
        }
        const auto space = vth::win::driveSpace(root);
        const quint64 used = (space.valid && space.totalBytes >= space.freeBytes)
            ? space.totalBytes - space.freeBytes : 0;

        auto* content = new QWidget;
        auto* root_ = new QHBoxLayout(content);
        root_->setContentsMargins(0, 0, 0, 0);
        root_->setSpacing(tokens::Space32);

        const auto heading = [content](QVBoxLayout* into, const QString& text) {
            modalHeading(content, into, text);
        };

        const auto row = [content](QVBoxLayout* into, const QString& key,
                                   const QString& value, const QColor& dot = {}) {
            auto* w = new QWidget(content);
            auto* h = new QHBoxLayout(w);
            h->setContentsMargins(0, tokens::Space2, 0, tokens::Space2);
            h->setSpacing(tokens::CompactGap);
            if (dot.isValid()) {
                auto* d = new QLabel(w);
                d->setFixedSize(tokens::DotSm, tokens::DotSm);
                d->setPixmap(colorDot(dot, tokens::DotSm, w->devicePixelRatioF()));
                h->addWidget(d, 0, Qt::AlignVCenter);
            }
            auto* k = new QLabel(key, w);
            k->setObjectName(QStringLiteral("settingsRowDesc"));
            h->addWidget(k, 0, Qt::AlignVCenter);
            h->addStretch(1);
            auto* v = new QLabel(value, w);
            v->setObjectName(QStringLiteral("dashboardReadoutValue"));
            h->addWidget(v, 0, Qt::AlignVCenter);
            into->addWidget(w);
        };

        // left: what is on this drive
        auto* left = new QVBoxLayout;
        left->setContentsMargins(0, 0, 0, 0);
        left->setSpacing(0);

        auto* bar = new vth::widgets::StorageHeatmap(content);
        bar->setFixedHeight(kModalBarHeight);

        QVector<vth::widgets::StorageHeatmap::Segment> segs;
        namespace palette = vth::theme::palette;
        if (m_haveBreakdown && m_lastBreakdown.valid) {
            // Full detail here: this surface has the width for the long labels,
            // and it is where the split the card merges is meant to be visible.
            segs = segmentsFor(m_lastBreakdown, Detail::Full);
            bar->setSegments(segs, m_lastBreakdown.totalBytes);
        } else {
            if (used > 0)
                segs.push_back({ tr("Used"), used, palette::storageWindows() });
            if (space.freeBytes > 0)
                segs.push_back({ tr("Free"), space.freeBytes, palette::storageFree() });
            bar->setSegments(segs, space.totalBytes);
        }
        left->addWidget(bar);
        left->addSpacing(tokens::CardMargin);

        heading(left, tr("What is using it"));
        // all numbers from one source so rows don't disagree
        const bool    fromScan  = m_haveBreakdown && m_lastBreakdown.valid;
        const quint64 totalBytes = fromScan ? m_lastBreakdown.totalBytes : space.totalBytes;
        for (const auto& s : segs)
            row(left, s.name, fmtGiB(s.bytes), s.color);
        if (!m_haveBreakdown) {
            left->addSpacing(tokens::CompactGap);
            auto* pending = new QLabel(
                tr("Still measuring the category breakdown."), content);
            pending->setObjectName(QStringLiteral("settingsRowDesc"));
            left->addWidget(pending);
        }

        heading(left, tr("This drive"));
        row(left, tr("Volume"), info.label.isEmpty()
                ? root.left(2) : tr("%1  %2").arg(root.left(2), info.label));
        if (!info.fileSystem.isEmpty())
            row(left, tr("File system"), info.fileSystem);
        row(left, tr("Capacity"), fmtGiB(totalBytes));
        row(left, tr("Windows installed here"), info.isSystem ? tr("Yes") : tr("No"));
        left->addStretch(1);

        // right: what a clean up can remove from this drive
        auto* right = new QVBoxLayout;
        right->setContentsMargins(0, 0, 0, 0);
        right->setSpacing(0);

        heading(right, info.isSystem ? tr("What can be removed")
                                     : tr("What can be removed here"));

        for (const auto* c : vth::win::cleanup::cleanersFor(root)) {
            // Caution separates a cache that regenerates from a rollback that doesn't.
            const bool lasting = c->severity == vth::win::reg::Severity::Caution;
            modalItem(content, right,
                      lasting ? QStringLiteral("triangle-alert-filled")
                              : QStringLiteral("circle-check-filled"),
                      lasting ? &palette::warningText
                              : &palette::scoreGoodText,
                      c->displayName, c->impact);
        }

        right->addStretch(1);

        root_->addLayout(left, 5);
        root_->addLayout(right, 6);

        // Managed: this list is as long as the catalog, which is far past the
        // region's height.
        vth::dialogs::showContentModal(this, tr("Storage"), content,
                                       vth::widgets::PanelDialog::Mode::Cover,
                                       vth::dialogs::ModalScroll::Managed);
    }

    // The chooser, where the decision is made against real sizes.
    void DashboardPage::onCleanupClicked()
    {
        auto* view = new CleanupView(selectedDriveRoot());
        // What a run freed is a free-space delta, so the card re-measures.
        connect(view, &CleanupView::cleaned, this,
                [this](quint64) { refreshDiskSpace(); });

        // ScrimPanel, because this has switches and a button on it. Self, because
        // SettingsList owns its scrolling.
        vth::dialogs::showContentModal(this, tr("Clean up"), view,
                                       vth::widgets::PanelDialog::Mode::ScrimPanel,
                                       vth::dialogs::ModalScroll::Self);
        // The modal runs its own event loop, so this lands after it closes.
        refreshDiskSpace();
    }

    // Every row is a route this install can open at any tier, because navigateTo refuses
    // a gated one. Each ticks when opened.
    QWidget* DashboardPage::buildStartCard()
    {
        auto [wrap, card, col, head] = makeTitledCard(this, tr("Get started"),
                                               QStringLiteral("hand-click"),
                                               CardInset::RowList);

        auto* steps = new QVBoxLayout;
        steps->setContentsMargins(0, 0, 0, 0);
        steps->setSpacing(tokens::TightGap);

        const auto addStep = [this, card = card, steps](
                const QString& text, const QString& why, const QString& route) {
            // The color lambda reads the done state off the widget, so a theme switch gets
            // both.
            auto* mark = new vth::widgets::GlyphLabel(
                QStringLiteral("circle-dashed"), &vth::theme::palette::textMuted,
                tokens::GlyphLg);
            mark->setFixedSize(tokens::GlyphXl, tokens::GlyphXl);
            mark->setColor([mark] {
                return mark->property("stepDone").toBool()
                    ? vth::theme::palette::accentOnSurface()
                    : vth::theme::palette::textMuted();
            });
            m_stepIcons.push_back(mark);
            m_stepRoutes.push_back(route);

            steps->addWidget(makeActionRow(card, mark, text, why,
                QStringLiteral("chevron-right"),
                [this, route]() {
                    markStepVisited(route);
                    Q_EMIT navigateRequested(route);
                }));
        };

        addStep(tr("Speed up Windows"),
                tr("Apply tweaks to this PC."),
                QStringLiteral("tweaks"));
        addStep(tr("Repair what's broken"),
                tr("Run the fixes for common Windows faults."),
                QStringLiteral("system"));
        addStep(tr("Test your connection"),
                tr("Measure speed, latency, and bufferbloat."),
                QStringLiteral("network"));

        col->addLayout(steps, 1);

        for (vth::widgets::GlyphLabel* mark : m_stepIcons)
            setStepDone(mark, false);

        return wrap;
    }

    void DashboardPage::setStepDone(vth::widgets::GlyphLabel* mark, bool done)
    {
        if (!mark)
            return;
        // The state the color lambda reads. Set before the icon swap so the
        // rebake that setIconName triggers already sees the new value.
        mark->setProperty("stepDone", done);
        mark->setIconName(done ? QStringLiteral("circle-check-filled")
                               : QStringLiteral("circle-dashed"));
        // color may have changed even if the icon didn't
        mark->refresh();
    }

    // The items ask whether the user did this in Vethida, because much of the catalog
    // matches a stock Windows install.
    QString DashboardPage::stepKey(const QString& route)
    {
        return QStringLiteral("progress/started/") + route;
    }

    // a step is done once it's been opened
    void DashboardPage::markStepVisited(const QString& route)
    {
        if (auto* settings = vth::ServiceRegistry::instance().get<vth::Settings>())
            settings->setValue(stepKey(route), true);
        refreshChecklist();
    }

    void DashboardPage::refreshChecklist()
    {
        auto* settings = vth::ServiceRegistry::instance().get<vth::Settings>();
        for (int i = 0; i < m_stepIcons.size() && i < m_stepRoutes.size(); ++i) {
            setStepDone(m_stepIcons.at(i), settings
                && settings->value(stepKey(m_stepRoutes.at(i)), false).toBool());
        }
    }

    // tips shown in the strip when there's no warning, next one after closing
    namespace
    {
        struct Tip
        {
            const char* message;
            const char* detail;
            const char* button;   // empty for a tip with nowhere to go
            const char* route;
            // The tier that can reach this tip's destination, because navigateTo refuses a
            // gated route.
            vth::models::ProductTier minTier;
        };

        constexpr Tip kTips[] = {
            { QT_TR_NOOP("Preinstalled apps keep running after you stop using them"),
              QT_TR_NOOP("Debloat removes them and the services behind them."),
              QT_TR_NOOP("Open Debloat"), "debloat", vth::models::ProductTier::Pro },
            { QT_TR_NOOP("A tweak marked Permanent is the only kind that cannot be undone"),
              QT_TR_NOOP("Everything else reverts from the switch that applied it."),
              QT_TR_NOOP("Open Tweaks"), "tweaks", vth::models::ProductTier::Free },
            { QT_TR_NOOP("Startup programs are most of what makes a cold boot slow"),
              QT_TR_NOOP("The System tab lists what starts with Windows."),
              QT_TR_NOOP("Open System"), "system", vth::models::ProductTier::Free },
            { QT_TR_NOOP("Windows keeps files long after you delete them"),
              QT_TR_NOOP("Disk cleanup on this page frees the space they hold."),
              "", "", vth::models::ProductTier::Free },
            { QT_TR_NOOP("The health score is read fresh every time you open this tab"),
              QT_TR_NOOP("It reflects the machine as it is now."),
              "", "", vth::models::ProductTier::Free },
        };

        constexpr int kTipCount = static_cast<int>(std::size(kTips));

        // saved so a dismissed tip doesn't come back next time
        const QString kStripIndexKey = QStringLiteral("dash/stripIndex");
    }

    // Which state the strip is showing, or none. Cheap enough to run whole on every call.
    void DashboardPage::refreshStatusStrip()
    {
        if (!m_strip)
            return;

        using Strip = vth::widgets::StatusStrip;
        // Every path below either sets a state and shows, or falls through to the
        // hide at the end.
        m_strip->hide();
        m_stripRotating = false;

        // dismissed, stays hidden for this session
        if (m_stripDismissed)
            return;

        m_stripRotating = true;

        // only tips whose page this tier can open
        QVector<const Tip*> eligible;
        eligible.reserve(kTipCount);
        for (const Tip& t : kTips)
            if (vth::meetsTier(t.minTier))
                eligible.push_back(&t);

        const bool showUpgrade = !vth::isPro();
        const int  slots = static_cast<int>(eligible.size()) + (showUpgrade ? 1 : 0);
        if (slots == 0)
            return;
        int slot = 0;
        if (auto* settings = vth::ServiceRegistry::instance().get<vth::Settings>())
            slot = settings->value(kStripIndexKey, 0).toInt();
        // the stored counter keeps climbing, wrap it here
        slot = ((slot % slots) + slots) % slots;

        if (showUpgrade && slot == 0) {
            Strip::Payload p;
            p.onAction = []() {
                QDesktopServices::openUrl(QUrl(vth::site::kPricing));
            };
            m_strip->setState(Strip::State::FreeTier, p);
            m_strip->show();
            return;
        }

        const Tip& tip = *eligible.at(showUpgrade ? slot - 1 : slot);
        Strip::Payload p;
        p.message = tr(tip.message);
        p.detail  = tr(tip.detail);
        if (tip.button[0] != '\0') {
            p.buttonLabel = tr(tip.button);
            const QString route = QString::fromLatin1(tip.route);
            p.onAction = [this, route]() { Q_EMIT navigateRequested(route); };
        }
        m_strip->setState(Strip::State::Tip, p);
        m_strip->show();
    }

    // The close control. What was showing decides what closing it means.
    void DashboardPage::onStripDismissed()
    {
        if (m_stripRotating) {
            if (auto* settings = vth::ServiceRegistry::instance().get<vth::Settings>())
                settings->setValue(kStripIndexKey,
                                   settings->value(kStripIndexKey, 0).toInt() + 1);
        }
        m_stripDismissed = true;
        refreshStatusStrip();
    }

    // Genesis on the Dashboard: the state, and the switch that changes it.
    QWidget* DashboardPage::buildGenesisCard()
    {
        const bool ultimate = vth::isUltimate();

        auto [wrap, card, col, head] = makeTitledCard(this, tr("Genesis"),
                                                QStringLiteral("meteor-filled"));
        col->setSpacing(tokens::CardMargin);

        auto* stateRow = new QHBoxLayout;
        stateRow->setContentsMargins(0, 0, 0, 0);
        stateRow->setSpacing(tokens::CompactGap);

        m_genesisDot = new QLabel(card);
        m_genesisDot->setObjectName(QStringLiteral("genesisStateDot"));
        m_genesisDot->setFixedSize(tokens::DotSm, tokens::DotSm);
        stateRow->addWidget(m_genesisDot, 0, Qt::AlignVCenter);

        m_genesisStateWord = new QLabel(card);
        m_genesisStateWord->setObjectName(QStringLiteral("genesisStateWord"));
        stateRow->addWidget(m_genesisStateWord, 0, Qt::AlignVCenter);
        col->addLayout(stateRow);

        // wrapAt makes the height it reports the height it renders at.
        auto* desc = new QLabel(
            tr("Closes background apps and pauses services for a session."),
            card);
        desc->setObjectName(QStringLiteral("genesisStatusText"));
        wrapAt(desc, kGenesisTextWidth);
        col->addWidget(desc);

        auto* rule = new QFrame(card);
        rule->setObjectName(QStringLiteral("settingsRule"));
        col->addWidget(rule);

        // What the mode holds. The value becomes the live figure once a session runs.
        auto* stats = new QVBoxLayout;
        stats->setContentsMargins(0, 0, 0, 0);
        stats->setSpacing(0);
        m_genesisApps     = addStatRow(card, stats, tr("Background apps"));
        m_genesisServices = addStatRow(card, stats, tr("Services"));
        m_genesisNotices  = addStatRow(card, stats, tr("Protected apps"));
        col->addLayout(stats);

        // The slack in a row that equalises to a taller card, parked above the
        // footer so the footer stays on the card's bottom edge.
        col->addStretch(1);

        // configure on the left, toggle on the right
        auto* foot = makeCardFooter(card, col);

        auto* configure = new vth::widgets::CommandButton(
            vth::widgets::CommandButton::Variant::Secondary, tr("Configure"), card);
        configure->setCursor(Qt::PointingHandCursor);
        connect(configure, &QPushButton::clicked, this, [this]() { openGenesisConfig(); });
        foot->addWidget(configure, 0, Qt::AlignBottom);
        foot->addStretch(1);

        m_genesisPower = new vth::widgets::PowerToggle(card);
        m_genesisPower->setToolTip(ultimate
            ? tr("Turn Genesis on or off")
            : tr("Genesis needs %1").arg(
                  vth::models::tierProductName(vth::models::ProductTier::Ultimate)));
        connect(m_genesisPower, &QAbstractButton::clicked, this, [this]() {
            onGenesisToggled(!m_genesisState.active);
        });

        foot->addWidget(m_genesisPower, 0, Qt::AlignBottom);

        // Below Ultimate the card is darkened and the upgrade floats on it.
        if (!ultimate) {
            auto* lock = new vth::widgets::TierLockOverlay(
                card, vth::models::ProductTier::Ultimate,
                vth::widgets::TierLockOverlay::Presentation::Card);
            lock->show();
            lock->raise();
        }

        refreshGenesis();
        return wrap;
    }

    void DashboardPage::openGenesisConfig()
    {
        // The view can activate and deactivate on its own, so the card is re-read
        // afterwards rather than assumed unchanged.
        vth::dialogs::showContentModal(this, tr("Genesis"), new GenesisView,
                                       vth::widgets::PanelDialog::Mode::ScrimPanel);
        refreshGenesis();
    }

    void DashboardPage::refreshGenesis()
    {
        // read the saved state (correct even after a crash)
        m_genesisToken.cancel();
        struct Card {
            vth::win::genesis::State state;
            int                      keptOpen = 0;
        };
        m_genesisToken = vth::fetch<Card>(
            this,
            []() {
                namespace gen = vth::win::genesis;
                Card c;
                c.state    = gen::loadPersistedState();
                c.keptOpen = gen::effectiveExceptions(gen::exceptionsFromSettings()).size();
                return c;
            },
            [this](Card c) {
                m_genesisState    = std::move(c.state);
                m_genesisKeptOpen = c.keptOpen;
                updateGenesisStatus();
            });

        updateGenesisStatus();
    }

    void DashboardPage::updateGenesisStatus()
    {
        // A property the stylesheet selects on does not repaint on its own.
        const auto repolish = [](QWidget* w) {
            w->style()->unpolish(w);
            w->style()->polish(w);
        };

        const bool on = m_genesisState.active;

        if (m_genesisDot) {
            m_genesisDot->setProperty("on", on);
            repolish(m_genesisDot);
        }
        if (m_genesisStateWord) {
            m_genesisStateWord->setText(on ? tr("Active") : tr("Inactive"));
            m_genesisStateWord->setProperty("on", on);
            repolish(m_genesisStateWord);
        }

        // What a session did while it runs, and what it will do while it doesn't.
        if (m_genesisApps) {
            m_genesisApps->setText(on ? tr("%1 closed").arg(m_genesisState.processesKilled)
                                      : tr("Closed and paused"));
        }
        if (m_genesisServices) {
            m_genesisServices->setText(
                on ? tr("%1 stopped").arg(m_genesisState.servicesStopped.size())
                   : tr("Stopped"));
        }
        if (m_genesisNotices) {
            // The same in both states: the protected floor plus the person's own additions.
            m_genesisNotices->setText(tr("%1 kept open").arg(m_genesisKeptOpen));
        }

        if (m_genesisPower) {
            // The disc carries no words, so the accessible name is the whole of
            // what a screen reader gets and has to say which way it points.
            m_genesisPower->setAccessibleName(on ? tr("Turn Genesis off")
                                                 : tr("Turn Genesis on"));
            // Blocked, because this is a repaint from state and not a press: the
            // toggled signal would run onGenesisToggled again.
            const QSignalBlocker block(m_genesisPower);
            m_genesisPower->setChecked(on);
        }
    }

    void DashboardPage::onGenesisToggled(bool on)
    {
        if (!m_genesisPower)
            return;

        // Below Ultimate the toggle is the upgrade prompt.
        if (!vth::isUltimate()) {
            vth::dialogs::showUpgrade(
                this, vth::models::tierLabel(vth::models::ProductTier::Ultimate));
            return;
        }

        namespace gen = vth::win::genesis;

        if (on) {
            if (m_genesisState.active)
                return;
            if (!vth::prefs::askLess()
                && !vth::dialogs::confirm(this, tr("Turn Genesis on"),
                    tr("Vethida will close or suspend background programs and "
                       "stop the services you picked under Configure."),
                    tr("Turn on"))) {
                return;
            }

            m_genesisPower->setEnabled(false);
            struct Outcome {
                gen::ActivateResult r;
                gen::State          state;
                QStringList         closeList;
                QStringList         exceptions;
                int                 keptOpen = 0;
            };
            vth::fetch<Outcome>(
                this,
                [state = m_genesisState]() mutable {
                    // Read here, so the QSettings reads are on the worker with the activation.
                    const gen::ActivateOptions opts = gen::optionsFromSettings();
                    Outcome out;
                    out.closeList  = gen::closeListFromSettings();
                    out.exceptions = gen::exceptionsFromSettings();
                    out.keptOpen   = gen::effectiveExceptions(out.exceptions).size();
                    out.r = gen::activate(opts, out.closeList, out.exceptions, state);
                    out.state = state;
                    return out;
                },
                [this](Outcome out) {
                    m_genesisState    = out.state;
                    m_genesisKeptOpen = out.keptOpen;
                    if (m_genesisPower)
                        m_genesisPower->setEnabled(true);
                    if (!out.r.ok) {
                        vth::dialogs::warn(this, tr("Genesis not started"),
                            out.r.error.isEmpty() ? tr("Genesis could not start.")
                                                  : out.r.error);
                    } else if (auto* monitor =
                                   vth::ServiceRegistry::instance().get<gen::GenesisMonitor>()) {
                        // a service so it keeps running on other pages
                        monitor->start(out.closeList, out.exceptions);
                    }
                    updateGenesisStatus();
                });
            return;
        }

        if (!m_genesisState.active)
            return;

        // Before the teardown, not after: a sweep still in flight would otherwise
        // close a program the teardown had already handed back.
        if (auto* monitor = vth::ServiceRegistry::instance().get<gen::GenesisMonitor>())
            monitor->stop();

        m_genesisPower->setEnabled(false);
        struct Outcome { bool ok = false; gen::State state; };
        vth::fetch<Outcome>(
            this,
            [state = m_genesisState]() mutable {
                const bool ok = gen::deactivate(state);
                return Outcome{ ok, state };
            },
            [this](Outcome out) {
                m_genesisState = out.state;
                if (m_genesisPower)
                    m_genesisPower->setEnabled(true);
                // renames first, that's the one people notice (a program "missing")
                if (!out.state.renames.isEmpty()) {
                    QStringList stillOut;
                    for (const auto& rec : out.state.renames)
                        stillOut << rec.renamedPath;
                    vth::dialogs::warn(this, tr("Some apps are still renamed"),
                        tr("These files still carry the name Genesis gave them:\n\n%1\n\n"
                           "Something is holding them open. Vethida puts the names "
                           "back the next time it starts.")
                            .arg(stillOut.join(QLatin1Char('\n'))));
                } else if (!out.ok) {
                    vth::dialogs::warn(this, tr("Genesis not fully stopped"),
                        tr("Some changes could not be put back. Turn Genesis on "
                           "and off again, or restart the PC to clear the rest."));
                }
                updateGenesisStatus();
            });
    }
}

