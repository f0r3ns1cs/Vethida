#include "pch.h"
#include "Theme.h"

#include "DesignTokens.h"
#include "../Settings/Preferences.h"
#include "../Utils/TablerIcon.h"

#include <QApplication>
#include <QFont>
#include <QHash>
#include <QList>
#include <QMenu>
#include <QPalette>
#include <QProxyStyle>
#include <QRegularExpression>
#include <QStyle>
#include <QStyleFactory>
#include <QWidget>

// Last, and after every Qt header: windows.h defines macros that collide with
// Qt identifiers. Needed for SPI_GETCLIENTAREAANIMATION.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace vth::theme
{
    namespace
    {
        Mode g_currentMode = Mode::Dark;

        // things the stylesheet can't do: tooltip delay, rounded tooltips, focus rect
        class PlatformStyle : public QProxyStyle
        {
        public:
            explicit PlatformStyle(QStyle* base) : QProxyStyle(base) {}

            int styleHint(StyleHint hint, const QStyleOption* option = nullptr,
                          const QWidget* widget = nullptr,
                          QStyleHintReturn* returnData = nullptr) const override
            {
                if (hint == SH_ToolTip_WakeUpDelay)
                    return 0;
                return QProxyStyle::styleHint(hint, option, widget, returnData);
            }

            // Windows' dotted focus rectangle, which is the one focus indicator
            // this application does not draw itself.
            void drawPrimitive(PrimitiveElement element, const QStyleOption* option,
                               QPainter* painter,
                               const QWidget* widget = nullptr) const override
            {
                if (element == PE_FrameFocusRect)
                    return;
                QProxyStyle::drawPrimitive(element, option, painter, widget);
            }

            using QProxyStyle::polish;   // keep the QApplication/QPalette overloads

            // translucent tooltip window so the rounded corners work (QTipLabel is private)
            void polish(QWidget* widget) override
            {
                // isWindow() first, it's cheap
                if (widget && widget->isWindow() && widget->inherits("QTipLabel")
                    && !widget->property(kRounded).toBool()) {
                    widget->setProperty(kRounded, true);

                    // QTipLabel is reused, so re-setting the flags is what recreates its
                    // native window. NoDropShadow stops DWM drawing square corners back on.
                    widget->setAttribute(Qt::WA_TranslucentBackground, true);
                    widget->setWindowFlags(widget->windowFlags()
                                           | Qt::FramelessWindowHint
                                           | Qt::NoDropShadowWindowHint);
                }
                QProxyStyle::polish(widget);
            }

        private:
            // Recreating the window is not free and this runs on every polish
            // pass; once per tooltip label is enough.
            static constexpr const char* kRounded = "vthRoundedTooltip";
        };

        // windowsvista base style. Every scrollbar subcontrol has to be in the QSS or it
        // falls back to native drawing. Tooltips need the QSS and the palette.
        void applyWindowsVistaStyle(QApplication& app)
        {
            if (QStyle* base = QStyleFactory::create(QStringLiteral("windowsvista")))
                app.setStyle(new PlatformStyle(base));
        }

        QPalette applicationPalette()
        {
            namespace palette = vth::theme::palette;

            QPalette p;
            p.setColor(QPalette::Window, palette::appChrome());
            p.setColor(QPalette::WindowText, palette::textPrimary());
            p.setColor(QPalette::Base, palette::cardSurface());
            p.setColor(QPalette::AlternateBase, palette::cardSurfaceRaised());
            p.setColor(QPalette::ToolTipBase, palette::cardSurfaceRaised());
            p.setColor(QPalette::ToolTipText, palette::textPrimary());
            p.setColor(QPalette::Text, palette::textPrimary());
            p.setColor(QPalette::Button, palette::cardSurfaceRaised());
            p.setColor(QPalette::ButtonText, palette::textPrimary());
            p.setColor(QPalette::BrightText, QColor(Qt::white));
            p.setColor(QPalette::Highlight, palette::accentFill());
            p.setColor(QPalette::HighlightedText, QColor(Qt::white));
            p.setColor(QPalette::Link, palette::linkText());
            p.setColor(QPalette::PlaceholderText, palette::textMuted());
            p.setColor(QPalette::Disabled, QPalette::WindowText, palette::textMuted());
            p.setColor(QPalette::Disabled, QPalette::Text, palette::textMuted());
            p.setColor(QPalette::Disabled, QPalette::ButtonText, palette::textMuted());
            return p;
        }

        QString resolveTokens(const QString& sheet)
        {
            static const QRegularExpression token(QStringLiteral("@\\{([A-Za-z0-9]+)\\}"));
            const QHash<QString, QString> values = tokenMap();

            QString out;
            out.reserve(sheet.size());
            qsizetype pos = 0;
            for (auto it = token.globalMatch(sheet); it.hasNext();) {
                const QRegularExpressionMatch m = it.next();
                out += QStringView{ sheet }.mid(pos, m.capturedStart() - pos);
                if (const auto v = values.constFind(m.captured(1)); v != values.constEnd())
                    out += v.value();
                else {
                    // never leave "@{name}" in, the braces break the rules around it
                    qWarning("Theme: unknown style token '%s'", qPrintable(m.captured(1)));
                    Q_ASSERT(!"unknown style token - add it to tokenMap()");
                    out += QStringLiteral("0");
                }
                pos = m.capturedEnd();
            }
            out += QStringView{ sheet }.mid(pos);
            return out;
        }

        QString rawStyleSheet()
        {
            QString sheet = QStringLiteral(R"qss(
QMainWindow, QWidget#centralWidget { background: @{appChrome}; color: @{textPrimary}; }
QMenuBar, QFrame#accountRibbon { background: @{appChrome}; color: @{textPrimary}; border: 0; }
QMenuBar::item:selected { background: @{cardSurface}; }
/* menus and dropdown popups. Rounded corners also need applyMenuChrome() */
QMenu, QFrame#vethDropdownPopup { background: @{cardSurface}; color: @{textPrimary}; border: 1px solid @{controlBorder}; border-radius: @{radiusCard}; }
QMenu { padding: @{space4}; }
QMenu::item, QPushButton#vethDropdownItem { min-height: @{menuItemHeight}; padding: @{space8} @{buttonHPad}; border-radius: @{radiusControl}; }
QMenu::item { margin: 0; }
QMenu::item:selected, QPushButton#vethDropdownItem:hover, QPushButton#vethDropdownItem:focus { background: @{sidebarItemHover}; color: @{textPrimary}; }
QMenu::item:disabled { color: @{textMuted}; background: transparent; }
QMenu::icon { width: 0; }
QMenu::separator { height: 1px; background: @{cardBorder}; margin: @{space4}; }

QFrame#accountRibbon { min-height: @{titleBarHeight}; max-height: @{titleBarHeight}; }

/* Qt doesn't clamp a radius, and most icon buttons aren't sized, so the control radius. */
QToolButton[uiRole="iconButton"] { background: transparent; border: @{ruleThickness} solid transparent; border-radius: @{radiusControl}; padding: @{iconButtonPad}; }
QToolButton[uiRole="iconButton"]:hover { background: @{sidebarItemHover}; }
QToolButton[uiRole="iconButton"]:pressed { background: @{sidebarItemActive}; }
QToolButton[uiRole="iconButton"]:focus { border: @{focusBorder} solid @{accentOnSurface}; padding: @{iconButtonPadFocus}; }
QToolButton[uiRole="iconButton"]:disabled { background: transparent; }
QToolButton[uiRole="iconButton"]::menu-indicator { image: none; width: 0; height: 0; }

QToolButton#captionMinimizeButton, QToolButton#captionMaximizeButton, QToolButton#captionCloseButton { min-width: @{captionButtonWidth}; max-width: @{captionButtonWidth}; min-height: @{titleBarHeight}; max-height: @{titleBarHeight}; padding: 0; border: 0; border-radius: 0; background: transparent; }
QToolButton#captionMinimizeButton[captionHover="true"], QToolButton#captionMaximizeButton[captionHover="true"] { background: @{titleControlHover}; }
QToolButton#captionMinimizeButton[captionPress="true"], QToolButton#captionMaximizeButton[captionPress="true"] { background: @{titleControlPress}; }
QToolButton#captionCloseButton[captionHover="true"] { background: @{titleCloseHover}; }
QToolButton#captionCloseButton[captionPress="true"] { background: @{titleClosePress}; }

/* Sizing only: hover and press come from the shared iconButton role. */
QToolButton#titleHelpButton { min-width: @{iconButtonSize}; max-width: @{iconButtonSize}; min-height: @{iconButtonSize}; max-height: @{iconButtonSize}; }

QAbstractScrollArea { border: 0; background: transparent; }
QAbstractScrollArea::corner { background: transparent; border: 0; }

/* scrollbars need a solid track (transparent shows garbage), slimness comes from
   the handle margins */
QScrollBar:vertical { border: 0; width: @{scrollBarThickness}; background: @{appContent}; }
QScrollBar:horizontal { border: 0; height: @{scrollBarThickness}; background: @{appContent}; }
QScrollBar::groove:vertical, QScrollBar::groove:horizontal { background: transparent; border: 0; }
QScrollBar::handle:vertical { background: @{cardBorderEmphasis}; border: 0; border-radius: @{radiusHairline}; min-height: @{scrollHandleMinLength}; margin: @{space2}; }
QScrollBar::handle:horizontal { background: @{cardBorderEmphasis}; border: 0; border-radius: @{radiusHairline}; min-width: @{scrollHandleMinLength}; margin: @{space2}; }
QScrollBar::handle:vertical:hover, QScrollBar::handle:horizontal:hover { background: @{textMuted}; }
/* Held while dragging, so the handle does not drop back to rest the moment the
   pointer slides off its few pixels of width mid-drag. */
QScrollBar::handle:vertical:pressed, QScrollBar::handle:horizontal:pressed { background: @{textSecondary}; }
QScrollBar::add-line, QScrollBar::sub-line { border: 0; width: 0; height: 0; background: transparent; }
QScrollBar::add-page, QScrollBar::sub-page { border: 0; background: transparent; }
/* track matches whatever it sits on. Text browsers styled as cards are listed directly */
QWidget[uiRole="card"] QScrollBar:vertical, QWidget[uiRole="card"] QScrollBar:horizontal,
QFrame#sectionCard QScrollBar:vertical, QFrame#sectionCard QScrollBar:horizontal,
QFrame#srPointsCard QScrollBar:vertical, QFrame#srPointsCard QScrollBar:horizontal,
QTreeView QScrollBar:vertical, QTreeView QScrollBar:horizontal,
QTextBrowser QScrollBar:vertical, QTextBrowser QScrollBar:horizontal,
QTableView QScrollBar:vertical, QTableView QScrollBar:horizontal { background: @{cardSurface}; }
/* raised surfaces. Second rule is for a base card inside a modal (specificity) */
QWidget[uiRole="cardRaised"] QScrollBar:vertical, QWidget[uiRole="cardRaised"] QScrollBar:horizontal,
QFrame#toolModalCard QScrollBar:vertical, QFrame#toolModalCard QScrollBar:horizontal { background: @{cardSurfaceRaised}; }
QFrame#toolModalCard QWidget[uiRole="card"] QScrollBar:vertical,
QFrame#toolModalCard QWidget[uiRole="card"] QScrollBar:horizontal { background: @{cardSurface}; }

QFrame#sidebarFrame { background: @{appChrome}; border: 0; border-right: 1px solid @{divider}; }
QFrame#navigationSidebar { background: transparent; border: 0; }
QWidget#navigationPrimaryHost, QWidget#navigationUtilityHost { background: transparent; border: 0; }

QLabel { color: @{textPrimary}; background: transparent; }

QStackedWidget#contentStack { background: @{appContent}; border: 0; }
QFrame#contentFrame { background: transparent; border: 0; }
QStackedWidget { background: transparent; border: 0; }
QLabel[uiRole="pageTitle"] { color: @{textPrimary}; font-size: @{fontDisplay}; font-weight: @{weightStrong}; padding: 0; }
QLabel[uiRole="sectionTitle"] { color: @{textPrimary}; font-size: @{fontTitle}; font-weight: @{weightMedium}; padding: 0; }
QLabel[uiRole="bodyText"] { color: @{textSecondary}; padding: 0; }
QLabel[uiRole="fieldLabel"] { color: @{textMuted}; font-weight: @{weightMedium}; padding: 0; }
QLabel[uiRole="fieldValue"] { color: @{textPrimary}; padding: 0; }
QLabel[uiRole="metaText"] { color: @{textMuted}; font-size: @{fontCaption}; padding: 0; }

/* chips. Only the tone changes. radiusPill = half the height (Qt doesn't clamp radius) */
QLabel[uiRole="chip"], QToolButton[uiRole="chip"] { background: @{badgeFill}; color: @{textSecondary}; border: 1px solid @{badgeBorder}; border-radius: @{radiusPill}; padding: @{space2} @{space8}; font-size: @{fontCaption}; font-weight: @{weightMedium}; }
QLabel[uiRole="chip"][tone="good"], QToolButton[uiRole="chip"][tone="good"] { background: @{tagPillFill}; color: @{scoreGoodText}; border-color: @{tagPillFill}; }
QLabel[uiRole="chip"][tone="warn"], QToolButton[uiRole="chip"][tone="warn"] { background: @{warnPillFill}; color: @{warnPillText}; border-color: @{warningEdge}; }
QLabel[uiRole="chip"][tone="critical"], QToolButton[uiRole="chip"][tone="critical"] { background: @{criticalBadgeFill}; color: @{criticalText}; border-color: @{criticalBadgeEdge}; }
/* premium: neutral fill with accent outline and text */
QLabel[uiRole="chip"][tone="premium"], QToolButton[uiRole="chip"][tone="premium"] { background: @{badgeFill}; color: @{accentOnSurface}; border-color: @{accentOnSurface}; }
/* Vendor chips. The label is the *OnSurface hue, never the mark hue - see
   DesignTokens.h. Color is not the signal: the chip's text is the vendor name. */
QLabel[uiRole="chip"][vendor="nvidia"] { background: @{nvidiaCardFill}; color: @{nvidiaOnSurface}; border-color: @{nvidiaCardBorder}; }
QLabel[uiRole="chip"][vendor="amd"] { background: @{amdCardFill}; color: @{amdOnSurface}; border-color: @{amdCardBorder}; }

/* The steps of a scale the reader did NOT land on. */
QLabel[uiRole="chip"][muted="true"], QToolButton[uiRole="chip"][muted="true"] { background: transparent; color: @{textMuted}; border-color: @{cardBorder}; }
/* only clickable chips get hover */
QToolButton[uiRole="chip"]:hover { border-color: @{accentOnSurface}; }
QToolButton[uiRole="chip"]:focus { border-color: @{accentOnSurface}; }
QToolButton[uiRole="chip"][tone="warn"]:hover { background: @{warnPillFillHover}; color: @{warnPillTextHover}; border-color: @{warnPillTextHover}; }
QToolButton[uiRole="chip"][tone="warn"]:focus { border-color: @{warnPillTextHover}; }
QToolButton[uiRole="chip"][tone="premium"]:hover, QToolButton[uiRole="chip"][tone="premium"]:focus { background: @{accentWash}; color: @{accentOnSurfaceHover}; border-color: @{accentOnSurfaceHover}; }
/* Press is a step deeper than hover, as on every button. Stated per tone because
   a tone's own fill outranks a rule without one. */
QToolButton[uiRole="chip"]:pressed { background: @{sidebarItemActive}; }
QToolButton[uiRole="chip"][tone="warn"]:pressed { background: @{warnPillFill}; }
QToolButton[uiRole="chip"][tone="premium"]:pressed { background: @{badgeFill}; }

/* The title bar's plan pill has no rule here: vth::widgets::LicenseBadge paints
   itself from the same palette entries these chips and the tier pills use. */

/* callouts (always with an icon and heading, not just color) */
QFrame[uiRole="callout"] { background: @{badgeFill}; border: 1px solid @{badgeBorder}; border-radius: @{radiusCard}; }
QFrame[uiRole="callout"][tone="warn"] { background: @{warningFill}; border-color: @{warningEdge}; }
QFrame[uiRole="callout"][tone="critical"] { background: @{criticalBadgeFill}; border-color: @{criticalBadgeEdge}; }
QLabel[uiRole="calloutTitle"] { color: @{textPrimary}; font-size: @{fontCaption}; font-weight: @{weightStrong}; padding: 0; }
QLabel[uiRole="calloutTitle"][tone="warn"] { color: @{warningText}; }
QLabel[uiRole="calloutTitle"][tone="critical"] { color: @{criticalText}; }
QLabel[uiRole="calloutBody"] { color: @{textSecondary}; padding: 0; }
/* A warning line in the tweak warnings modal: primary ink, because it is the
   thing the reader opened the modal for. */
QLabel[uiRole="warnLine"] { color: @{textPrimary}; font-size: @{fontBody}; padding: 0; }

/* The one reason a control on this pane cannot act. */
QLabel#disabledReasonText { color: @{textPrimary}; font-size: @{fontBody}; font-weight: @{weightMedium}; padding: 0; }

/* Cards: uiRole="card" on the page, uiRole="cardRaised" on another card or in a modal.
   Don't add a third card style. */
QWidget[uiRole="card"],
QFrame#dashboardStatusCard,
QFrame#listGroup, QFrame#sectionCard,
QFrame#srPointsCard { background: @{cardSurface}; border: 1px solid @{cardBorder}; border-radius: @{radiusCard}; }

/* Interactive-surface states: rest @{cardBorder}, hover @{cardBorderEmphasis},
   focus @{accentOnSurface}. */
QFrame#dashboardStatusCard[hovered="true"] { border-color: @{cardBorderEmphasis}; }
QFrame#dashboardStatusCard:focus { border-color: @{accentOnSurface}; }

/* ListGroup. Result line: green ok, red failed, muted for info */
QLabel#listGroupTitle { color: @{textPrimary}; font-size: @{fontTitle}; font-weight: @{weightMedium}; background: transparent; }
/* The grouped list (settings): a small heading above the card, not in it. */
QFrame#listGroupOuter { background: transparent; border: 0; }
QLabel#listGroupHeading { color: @{textSecondary}; font-size: @{fontBody}; font-weight: @{weightMedium}; background: transparent; }
QLabel#listRowTitle { color: @{textPrimary}; font-size: @{fontBody}; font-weight: @{weightMedium}; background: transparent; }
QLabel#listRowStatus { color: @{textMuted}; font-size: @{fontCaption}; background: transparent; }
QLabel#listRowStatus[tone="good"] { color: @{scoreGoodText}; }
QLabel#listRowStatus[tone="bad"] { color: @{criticalText}; }
QProgressBar#listRowProgress { background: @{divider}; border: 0; border-radius: 2px; min-height: 4px; max-height: 4px; margin-top: @{space4}; }
QProgressBar#listRowProgress::chunk { background: @{accentFill}; border-radius: 2px; }

/* One divider. */
QFrame#settingsRule, QFrame#srRowDivider { background: @{cardBorder}; border: 0; min-height: 1px; max-height: 1px; }

/* The rail divider runs the OTHER way and is a column, not a rule. It must stay
   out of the list above, which clamps HEIGHT to 1px and would make it invisible. */
QWidget#sectionRailDivider { background: @{cardBorder}; border: 0; min-width: 1px; max-width: 1px; }

QLabel#dashboardCardSubtitle { color: @{textSecondary}; padding: 0; }
QLabel#dashboardMonitorValue { color: @{accentOnSurface}; font-size: @{fontDisplay}; font-weight: @{weightStrong}; padding: 0; }

/* Small muted label over a group that is not a card. */
QLabel#dashboardSectionLabel { color: @{textMuted}; background: transparent; font-size: @{fontCaption}; font-weight: @{weightMedium}; padding: 0; }

/* Section heading. The one exception to "only the page title goes Strong": the
   dashboard has no page title, so these headings carry its whole hierarchy. */
QLabel#dashboardCardHeading { color: @{textPrimary}; background: transparent; font-size: @{fontHeading}; font-weight: @{weightStrong}; padding: 0; }

/* dashboard status strip, not a card (no border) */
QWidget#statusStrip { background: @{cardSurface}; border: 0; border-radius: @{radiusStrip}; }
QLabel#statusStripMessage { color: @{textPrimary}; background: transparent; font-size: @{fontBody}; font-weight: @{weightMedium}; padding: 0; }
/* The line under it. Caption and secondary, so the pair reads as one block with
   an order rather than two competing sentences. */
QLabel#statusStripDetail { color: @{textSecondary}; background: transparent; font-size: @{fontCaption}; padding: 0; }

QToolButton#statusStripClose { min-width: @{iconButtonSize}; max-width: @{iconButtonSize}; min-height: @{iconButtonSize}; max-height: @{iconButtonSize}; }

/* The Genesis card's state is a disc and a word, so color isn't the only signal. */
QLabel#genesisStateDot { background: @{textMuted}; border: 0; border-radius: @{dotSmRadius}; min-width: @{dotSm}; max-width: @{dotSm}; min-height: @{dotSm}; max-height: @{dotSm}; }
QLabel#genesisStateDot[on="true"] { background: @{accentOnSurface}; }
QLabel#genesisStateWord { color: @{textMuted}; background: transparent; font-size: @{fontCaption}; font-weight: @{weightMedium}; padding: 0; }
QLabel#genesisStateWord[on="true"] { color: @{accentOnSurface}; }
QLabel#genesisStatusText { color: @{textSecondary}; background: transparent; font-size: @{fontCaption}; padding: 0; }
/* The toggle carries no rule: PowerToggle paints its own ring, fill and mark. */

/* Genesis preset tiles */
QFrame#genesisPresetTile { background: @{cardSurface}; border: 1px solid @{cardBorder}; border-radius: @{radiusCard}; }
QFrame#genesisPresetTile:hover { border-color: @{cardBorderEmphasis}; }
QFrame#genesisPresetTile:focus { border-color: @{accentOnSurface}; }
QFrame#genesisPresetTile[selected="true"] { background: @{accentWash}; border-color: @{accentOnSurface}; }
QLabel#genesisPresetTitle { color: @{textPrimary}; font-size: @{fontBody}; font-weight: @{weightMedium}; background: transparent; }
QLabel#genesisPresetTitle[selected="true"] { color: @{accentOnSurface}; }
/* Two-way switch: a sunken track holding flat segments; the chosen one is
   raised onto the card surface. */
QFrame#segmentTrack { background: @{badgeFill}; border: 1px solid @{badgeBorder}; border-radius: @{radiusControl}; }
QPushButton[uiRole="segment"] { background: transparent; color: @{textSecondary}; border: 1px solid transparent; border-radius: @{space4}; padding: @{space4} @{space16}; font-size: @{fontBody}; min-height: @{space20}; }
QPushButton[uiRole="segment"]:hover, QPushButton[uiRole="segment"]:pressed { background: transparent; color: @{textPrimary}; }
QPushButton[uiRole="segment"]:checked, QPushButton[uiRole="segment"]:checked:hover { background: @{cardSurfaceRaised}; color: @{textPrimary}; border-color: @{cardBorder}; font-weight: @{weightMedium}; }
QPushButton[uiRole="segment"]:focus { border-color: @{accentOnSurface}; }

/* A removable name in a NameListEditor: the chip capsule with an x. */
QFrame#nameChip { background: @{cardSurfaceRaised}; border: 1px solid @{cardBorder}; border-radius: @{radiusBadge}; }
QFrame#nameChip:hover { border-color: @{cardBorderEmphasis}; }
QLabel#nameChipText { color: @{textPrimary}; font-size: @{fontCaption}; background: transparent; }
QToolButton#nameChipRemove { min-width: @{minTarget}; max-width: @{minTarget}; min-height: @{minTarget}; max-height: @{minTarget}; padding: 0; }

/* Right-hand value on a dashboard readout row. */
QLabel#dashboardReadoutValue { color: @{textPrimary}; background: transparent; font-size: @{fontCaption}; font-weight: @{weightMedium}; padding: 0; }

/* The one figure a card exists to answer. Body size carrying the strong weight:
   it shares its row with a dropdown, so weight rather than size separates it. */
QLabel#dashboardHeadlineValue { color: @{textPrimary}; background: transparent; font-size: @{fontBody}; font-weight: @{weightStrong}; padding: 0; }
QLabel#tweakCardTierPill { background: @{badgeFill}; color: @{accentOnSurface}; border: @{ruleThickness} solid @{accentOnSurface}; border-radius: @{radiusPill}; padding: @{space2} @{space8}; font-size: @{fontCaption}; font-weight: @{weightRegular}; }
QLabel#tweakCardTierPill[tier="ultimate"] { background: @{ultimateFill}; color: @{accentText}; border-color: @{ultimateFill}; }

/* The recommended band at the top of a tweak pane. */
QWidget#recommendBand { background: transparent; border: 0; border-bottom: @{ruleThickness} solid @{cardBorder}; }

/* A tweak's impact. The word says the level, so nothing rests on hue alone. */
QLabel#tweakRowState { color: @{textSecondary}; font-size: @{fontCaption}; font-weight: @{weightStrong}; background: transparent; }
QLabel#tweakRowState[impact="low"]      { color: @{textMuted}; font-weight: @{weightMedium}; }
QLabel#tweakRowState[impact="medium"]   { color: @{textSecondary}; }
QLabel#tweakRowState[impact="high"]     { color: @{warningText}; }
QLabel#tweakRowState[impact="critical"] { color: @{criticalText}; }
QLabel#tweakRowState[tone="irreversible"] { color: @{warningText}; }

QLabel#srPromptTitle { color: @{textPrimary}; font-size: @{fontHeading}; font-weight: @{weightStrong}; background: transparent; }
QLabel#srPromptBody { color: @{textSecondary}; font-size: @{fontBody}; background: transparent; }
QLabel#srPromptBadge { background: @{tagPillFill}; border-radius: @{promptBadgeRadius}; }
QLabel#srPromptBadge[tone="warning"] { background: @{warningFill}; border: 1px solid @{warnPillFill}; }
/* RAISED tier - see the card system above. Same border and radius as the base
   tier; only the fill differs. */
QWidget[uiRole="cardRaised"],
QFrame#srPromptCard, QFrame#toolModalCard,
QFrame#advisorInfoCard, QWidget#toastCard { background: @{cardSurfaceRaised}; border: 1px solid @{cardBorder}; border-radius: @{radiusCard}; }
QFrame#srPromptCard, QFrame#toolModalCard { border-radius: @{radiusModal}; }
QLabel#overlayHeading { color: @{textPrimary}; font-size: @{fontTitle}; font-weight: @{weightMedium}; }
QLabel#overlayBody { color: @{textSecondary}; font-size: @{fontBody}; }

/* tier lock veil, sits on a dark scrim in both modes so it uses accentText */
QFrame#tierLockVeil { background: transparent; border: 0; }
QLabel#tierLockVeilTitle { color: @{accentText}; background: transparent; font-size: @{fontHeading}; font-weight: @{weightStrong}; padding: 0; }

QLabel#emptyStateTitle { color: @{textPrimary}; font-size: @{fontTitle}; font-weight: @{weightMedium}; }
QLabel#emptyStateSubtitle { color: @{textSecondary}; font-size: @{fontCaption}; }

QWidget#vethDropdown { background: @{cardSurfaceRaised}; border: 1px solid @{controlBorder}; border-radius: @{radiusControl}; }
QWidget#vethDropdown:hover { border-color: @{cardBorderEmphasis}; }
/* dropdown insets are layout margins (Dropdown.cpp) */
QWidget#vethDropdown:focus { border: @{focusBorder} solid @{accentOnSurface}; }
QWidget#vethDropdown:disabled { background: @{cardSurface}; border-color: @{cardBorder}; }
QLabel#vethDropdownValue { background: transparent; color: @{textPrimary}; border: 0; }
QLabel#vethDropdownValue:disabled { color: @{textMuted}; }
QFrame#vethDropdownContainer { background: transparent; border: 0; }
/* rest of the popup style is with QMenu above */
QPushButton#vethDropdownItem { background: transparent; color: @{textPrimary}; border: 0; text-align: left; }
QPushButton#vethDropdownItem[selected="true"] { background: @{accentFill}; color: @{accentText}; }

/* The feature card: a pane's headline capability. A default panel. */
QFrame[uiRole="featureCard"] { background: @{cardSurface}; border: 1px solid @{cardBorder}; border-radius: @{radiusCard}; }
QLabel[uiRole="featureTitle"] { color: @{textPrimary}; background: transparent; font-size: @{fontTitle}; font-weight: @{weightStrong}; padding: 0; }
/* SectionNav */
QWidget#sectionRailHost { background: transparent; }
QFrame#sectionRailItem { background: transparent; border: 1px solid transparent; border-radius: @{radiusCard}; }
QFrame#sectionRailItem:hover { background: @{sidebarItemHover}; }
QFrame#sectionRailItem:focus { border-color: @{accentOnSurface}; }
QFrame#sectionRailItem[selected="true"] { background: @{sidebarItemSelected}; }
/* A section this license can't open has no hover fill. */
QFrame#sectionRailItem[locked="true"]:hover { background: transparent; }
QLabel#sectionRailTitle { color: @{textSecondary}; font-weight: @{weightRegular}; font-size: @{fontBody}; }
QLabel#sectionRailTitle[selected="true"] { color: @{textPrimary}; font-weight: @{weightMedium}; }
QLabel#sectionRailTitle[locked="true"] { color: @{textMuted}; }
QLabel#sectionRailSubtitle { color: @{textMuted}; font-size: @{fontCaption}; }

/* Tab strip (TabStrip). The selected tab's underline reads as part of the bar's
   rule, so don't give the bar vertical padding. */
QWidget#tabStripBar { background: transparent; border: 0; border-bottom: 1px solid @{cardBorder}; }
QFrame#tabStripItem { background: transparent; border: 0; border-bottom: @{tabUnderline} solid transparent; }
QFrame#tabStripItem:hover { border-bottom-color: @{cardBorderEmphasis}; }
QFrame#tabStripItem:focus { border-bottom-color: @{accentOnSurface}; }
/* The selected tab's underline is an AccentPill that slides between tabs; the
   selected tab draws no border of its own under it. */
QFrame#tabStripItem[selected="true"] { border-bottom-color: transparent; }
QLabel#tabStripTitle { color: @{textSecondary}; font-size: @{fontBody}; font-weight: @{weightMedium}; }
QLabel#tabStripTitle[selected="true"] { color: @{textPrimary}; }

QScrollArea#settingsScroll, QScrollArea#settingsScroll > QWidget#qt_scrollarea_viewport { background: transparent; border: 0; }
QWidget#scrollContentHost { background: transparent; border: 0; }
/* The settings row, and the only row treatment in this app. Anything with a fill
   and a border at rest is a card, not a row; see the two card tiers above. */
QFrame#settingsRow { background: transparent; border: 1px solid transparent; border-radius: @{radiusRow}; }
QFrame#settingsRow[interactive="true"]:hover { background: @{sidebarItemHover}; }
/* clickable rows are DashboardCard, not QPushButton (its sizeHint ignores the layout) */
QLabel#settingsRowTitle { color: @{textPrimary}; font-size: @{fontBody}; background: transparent; }
QLabel#settingsRowDesc { color: @{textSecondary}; font-size: @{fontCaption}; background: transparent; }
QLabel#settingsSectionLabel { color: @{textMuted}; font-size: @{fontCaption}; font-weight: @{weightMedium}; background: transparent; padding: 0; }
QRadioButton { color: @{textPrimary}; spacing: @{space12}; background: transparent; font-size: @{fontTitle}; padding: @{space8} 0; }
/* Unchecked disc rides @{controlBorder}, not the decorative card border: it is
   an interactive control and has to separate from the surface. */
QRadioButton::indicator { width: @{radioIndicator}; height: @{radioIndicator}; border-radius: @{radioIndicatorRadius}; border: none; background-color: @{controlBorder}; }
QRadioButton::indicator:hover { background-color: @{textMuted}; }
QRadioButton::indicator:checked { background-color: @{accentOnSurface}; }
QRadioButton::indicator:checked:hover { background-color: @{accentOnSurfaceHover}; }
QRadioButton:disabled { color: @{textMuted}; }
QRadioButton::indicator:disabled { background-color: @{cardBorder}; }

/* Buttons: primary (one per surface) and secondary, through CommandButton. Hover and
   press only change the fill. The focus ring is keyboard only (TabFocus). */
QPushButton, QPushButton[commandVariant="secondary"] { background: @{cardSurfaceRaised}; color: @{textPrimary}; border: 1px solid @{cardBorderEmphasis}; border-radius: @{buttonRadius}; min-height: @{buttonHeight}; max-height: @{buttonHeight}; padding: 0 @{buttonHPad}; font-weight: @{weightMedium}; }
QPushButton:hover, QPushButton[commandVariant="secondary"]:hover { background: @{sidebarItemHover}; }
QPushButton:pressed, QPushButton[commandVariant="secondary"]:pressed { background: @{sidebarItemActive}; }
/* thicker focus border, height/padding shrink to compensate */
QPushButton:focus, QPushButton[commandVariant="secondary"]:focus { border: @{focusBorder} solid @{accentOnSurface}; min-height: @{buttonHeightFocus}; max-height: @{buttonHeightFocus}; padding: 0 @{buttonHPadFocus}; }
QPushButton:disabled { color: @{textMuted}; background: transparent; border-color: @{cardBorder}; }

QPushButton[commandVariant="primary"] { background: @{accentFill}; color: @{accentText}; border: 1px solid @{accentFill}; border-radius: @{buttonRadius}; min-height: @{buttonHeight}; max-height: @{buttonHeight}; padding: 0 @{buttonHPad}; font-weight: @{weightMedium}; }
QPushButton[commandVariant="primary"]:hover { background: @{accentFillHover}; border-color: @{accentFillHover}; }
QPushButton[commandVariant="primary"]:pressed { background: @{accentFillPressed}; border-color: @{accentFillPressed}; }
/* accentOnSurfaceHover, not accentOnSurface: the ring sits ON the accent fill,
   and in light mode accentOnSurface is nearly the fill itself. */
QPushButton[commandVariant="primary"]:focus { border: @{focusBorder} solid @{accentOnSurfaceHover}; min-height: @{buttonHeightFocus}; max-height: @{buttonHeightFocus}; padding: 0 @{buttonHPadFocus}; }
QPushButton[commandVariant="primary"]:disabled { color: @{textMuted}; background: transparent; border-color: @{cardBorder}; }

QLabel[uiRole="link"] { color: @{linkText}; background: transparent; border: 0; text-decoration: none; padding: 0; }
QLabel[uiRole="link"]:hover { color: @{accentOnSurfaceHover}; text-decoration: underline; }
QLabel[uiRole="link"]:disabled { color: @{textMuted}; text-decoration: none; }
QToolButton[uiRole="link"] { color: @{linkText}; background: transparent; border: 0; padding: 0; font-weight: @{weightMedium}; text-decoration: none; }
QToolButton[uiRole="link"]:hover, QToolButton[uiRole="link"]:focus { color: @{accentOnSurfaceHover}; background: transparent; border: 0; text-decoration: underline; }
QToolButton[uiRole="link"]:pressed { color: @{accentOnSurface}; background: transparent; border: 0; }
QToolButton[uiRole="link"]:disabled { color: @{textMuted}; }

QLabel#toolModalTitle { color: @{textPrimary}; font-size: @{fontHeading}; font-weight: @{weightMedium}; background: transparent; }

/* activation key field. Letter spacing is set in code. Errors color the border. */
QLineEdit#activateKeyField { font-size: @{fontTitle}; min-height: @{space24}; padding: @{space8} @{space12}; }
QLineEdit#activateKeyField[state="error"] { border-color: @{criticalBadgeEdge}; }
QLineEdit#activateKeyField[state="error"]:focus { border-color: @{criticalText}; }
QLabel[uiRole="formError"] { color: @{criticalText}; font-size: @{fontCaption}; background: transparent; padding: 0; }

QWidget#tweakCategoryPage { background: transparent; border: 0; }

/* pages use the stack's background, don't give a page its own */

QWidget#startupApplicationsPage { background: transparent; }

/* The headline of the plan card. Heading size, not Display: Display is the page
   title step, and this label sits directly under one. */
QLabel#accountTierName { color: @{textPrimary}; font-size: @{fontHeading}; font-weight: @{weightStrong}; background: transparent; padding: 0; }

/* account status pill: good / warn */
QLabel#accountStatusPill { background: @{badgeFill}; color: @{textSecondary}; border: 1px solid @{badgeBorder}; border-radius: @{radiusPill}; padding: @{space2} @{space8}; font-size: @{fontCaption}; font-weight: @{weightMedium}; }
QLabel#accountStatusPill[tone="good"] { background: @{tagPillFill}; color: @{accentOnSurface}; border-color: @{accentOnSurface}; }
QLabel#accountStatusPill[tone="warn"] { background: @{warnPillFill}; color: @{warnPillText}; border-color: @{warningEdge}; }
/* The app's only progress bar treatment: a role, not a per-widget id. */
QProgressBar[uiRole="progressBar"] { background: @{divider}; border: 1px solid @{cardBorder}; border-radius: @{radiusSmall}; min-height: @{progressMinHeight}; max-height: @{progressMaxHeight}; padding: 0; text-align: center; }
QProgressBar[uiRole="progressBar"]::chunk { background: @{accentFill}; border-radius: @{radiusHairline}; }

/* The raised surface + control border match the other floating surfaces
   (menu and dropdown popups). */
QToolTip { background-color: @{cardSurfaceRaised}; color: @{textPrimary}; border: 1px solid @{controlBorder}; border-radius: @{radiusControl}; padding: @{space4} @{space8}; font-size: @{fontCaption}; }
QCheckBox { color: @{textPrimary}; spacing: @{space8}; background: transparent; }
QCheckBox::indicator, QTreeView::indicator, QTableView::indicator { width: @{checkIndicator}; height: @{checkIndicator}; border-radius: @{radiusSmall}; border: 1px solid @{controlBorder}; background: @{cardSurface}; }
QCheckBox::indicator:hover, QTreeView::indicator:hover, QTableView::indicator:hover { border-color: @{cardBorderEmphasis}; }
QCheckBox::indicator:checked, QTreeView::indicator:checked, QTableView::indicator:checked { background: @{accentFill}; border-color: @{accentFill}; }
QCheckBox::indicator:indeterminate, QTreeView::indicator:indeterminate { background: @{accentOnSurface}; border-color: @{accentFill}; }
/* focus border is thicker, padding is reduced to match (FocusBorderWidth) */
QLineEdit { background: @{cardSurface}; color: @{textPrimary}; border: @{ruleThickness} solid @{controlBorder}; border-radius: @{radiusControl}; padding: @{fieldPadV} @{fieldPadH}; min-height: @{fieldContentHeight}; }
QLineEdit:focus { border: @{focusBorder} solid @{accentOnSurface}; padding: @{fieldPadVFocus} @{fieldPadHFocus}; }

QComboBox, QSpinBox { background: @{cardSurface}; color: @{textPrimary}; border: @{ruleThickness} solid @{controlBorder}; border-radius: @{radiusControl}; padding: @{fieldPadV} @{fieldPadH}; min-height: @{comboContentHeight}; }
QComboBox:hover, QSpinBox:hover { border-color: @{cardBorderEmphasis}; }
QComboBox:focus, QSpinBox:focus { border: @{focusBorder} solid @{accentOnSurface}; padding: @{fieldPadVFocus} @{fieldPadHFocus}; }
QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: center right; width: @{comboArrowWidth}; border: 0; }
QComboBox QAbstractItemView { background: @{cardSurfaceRaised}; color: @{textPrimary}; border: 1px solid @{controlBorder}; border-radius: @{radiusCard}; padding: @{space4}; outline: 0; }
QComboBox QAbstractItemView::item { min-height: @{comboItemHeight}; padding: @{space4} @{space8}; }
QComboBox QAbstractItemView::item:selected { background: @{accentFill}; color: @{accentText}; }

/* outline: 0 suppresses Qt's focus rectangle on the current item, so every view
   belongs in this rule. */
QTableView, QTreeView, QTableWidget, QTreeWidget, QListView, QListWidget { background: @{cardSurface}; alternate-background-color: @{cardSurfaceRaised}; color: @{textPrimary}; border: 1px solid @{cardBorder}; border-radius: @{radiusCard}; gridline-color: @{cardBorder}; outline: 0; }
QTableView::item, QTreeView::item, QTableWidget::item, QTreeWidget::item, QListView::item, QListWidget::item { padding: @{space4} @{space8}; min-height: @{listRowHeight}; border: 0; outline: 0; }
/* Rounded only where one item is one row. */
QListView::item, QListWidget::item { border-radius: @{radiusRow}; }
QTableView::item:selected, QTreeView::item:selected, QTableWidget::item:selected, QTreeWidget::item:selected, QListView::item:selected, QListWidget::item:selected { background: @{accentFill}; color: @{accentText}; }
/* Rows answer the pointer. QTreeView promotes hover to the whole row whenever
   selectionBehavior is not SelectItems, so this lights the row, not the cell. */
QTableView::item:hover, QTreeView::item:hover, QTableWidget::item:hover, QTreeWidget::item:hover,
QListView::item:hover, QListWidget::item:hover { background: @{sidebarItemHover}; }
QTableView::item:selected:hover, QTreeView::item:selected:hover, QListView::item:selected:hover, QListWidget::item:selected:hover { background: @{accentFill}; color: @{accentText}; }
/* The current-item outline again, for the focus state specifically: Qt draws it
   from the view's focus policy, not the item's, so both need suppressing. */
QTableView:focus, QTreeView:focus, QTableWidget:focus, QTreeWidget:focus, QListView:focus, QListWidget:focus { outline: 0; }
/* No background on headers (it would cover the rounded corners). Padding must match ::item. */
QHeaderView { background: transparent; border: 0; }
QHeaderView::section { background: transparent; color: @{textMuted}; border: 0; border-bottom: 1px solid @{cardBorder}; padding: @{space8} @{space8}; font-size: @{fontCaption}; font-weight: @{weightMedium}; }
QHeaderView::section:hover { color: @{textPrimary}; }
QTableCornerButton::section { background: transparent; border: 0; border-bottom: 1px solid @{cardBorder}; }

/* list card draws the surface, the view itself draws nothing (see ListView.h) */
QFrame#listCard { background: @{cardSurface}; border: 1px solid @{cardBorder}; border-radius: @{radiusCard}; }
QTreeView#listView { background: transparent; border: 0; border-radius: 0; }
/* A scrollbar cannot be given a transparent track, so the card fill is repainted
   here and only the handle stays visible. */
QFrame#listCard QScrollBar:vertical, QFrame#listCard QScrollBar:horizontal { background: @{cardSurface}; }

/* The toast. */
QLabel#toastIconLabel { background: transparent; border: 0; }
QLabel#toastTitleLabel { color: @{textPrimary}; font-weight: @{weightMedium}; padding: 0; }
QLabel#toastBodyLabel { color: @{textSecondary}; padding: 0; }
QToolButton#toastCloseButton { background: transparent; border: 0; border-radius: @{radiusSmall}; padding: 0; }
QToolButton#toastCloseButton:hover { background: @{sidebarItemHover}; }
QToolButton#toastCloseButton:pressed { background: @{sidebarItemActive}; }
)qss");

            return sheet;
        }

        // The resolved sheet for a mode, built once.
        QString styleSheet(Mode mode)
        {
            static QHash<int, QString> resolved;
            const int key = static_cast<int>(mode);
            if (const auto it = resolved.constFind(key); it != resolved.constEnd())
                return it.value();
            const QString sheet = resolveTokens(rawStyleSheet());
            resolved.insert(key, sheet);
            return sheet;
        }
    }

    QHash<QString, QString> tokenMap()
    {
        namespace palette = vth::theme::palette;
        namespace tokens = vth::theme::tokens;
        namespace type = vth::theme::type;

        auto css = [](const QColor& color) { return color.name(QColor::HexRgb); };
        auto px = [](int value) { return QString::number(value) + QStringLiteral("px"); };
        auto num = [](int value) { return QString::number(value); };

        QHash<QString, QString> map = {
            { QStringLiteral("fontDisplay"),          px(type::Display) },
            { QStringLiteral("fontHeading"),          px(type::Heading) },
            { QStringLiteral("fontTitle"),            px(type::Title) },
            { QStringLiteral("fontBody"),             px(type::Body) },
            { QStringLiteral("fontCaption"),          px(type::Caption) },
            { QStringLiteral("weightRegular"),        num(type::WeightRegular) },
            { QStringLiteral("weightMedium"),         num(type::WeightMedium) },
            { QStringLiteral("weightStrong"),         num(type::WeightStrong) },
            { QStringLiteral("controlBorder"),        css(palette::controlBorder()) },
            { QStringLiteral("accentOnSurface"),      css(palette::accentOnSurface()) },
            { QStringLiteral("accentOnSurfaceHover"), css(palette::accentOnSurfaceHover()) },
            { QStringLiteral("appChrome"),            css(palette::appChrome()) },
            { QStringLiteral("appContent"),           css(palette::appContent()) },
            { QStringLiteral("cardSurface"),          css(palette::cardSurface()) },
            { QStringLiteral("cardSurfaceRaised"),    css(palette::cardSurfaceRaised()) },
            { QStringLiteral("cardBorder"),           css(palette::cardBorder()) },
            { QStringLiteral("cardBorderEmphasis"),   css(palette::cardBorderEmphasis()) },
            { QStringLiteral("divider"),              css(palette::divider()) },
            { QStringLiteral("textPrimary"),          css(palette::textPrimary()) },
            { QStringLiteral("textSecondary"),        css(palette::textSecondary()) },
            { QStringLiteral("textMuted"),            css(palette::textMuted()) },
            { QStringLiteral("badgeFill"),            css(palette::badgeFill()) },
            { QStringLiteral("badgeBorder"),          css(palette::badgeBorder()) },
            { QStringLiteral("tagPillFill"),    css(palette::tagPillFill()) },
            { QStringLiteral("accentWash"),     css(palette::licensePillFill()) },
            { QStringLiteral("tagPillText"),    css(palette::tagPillText()) },
            { QStringLiteral("linkText"),             css(palette::linkText()) },
            { QStringLiteral("accentFill"),           css(palette::accentFill()) },
            { QStringLiteral("accentFillHover"),      css(palette::accentFillHover()) },
            { QStringLiteral("accentFillPressed"),    css(palette::accentFillPressed()) },
            { QStringLiteral("accentText"),           css(palette::accentText()) },
            { QStringLiteral("nvidiaCardFill"),       css(palette::nvidiaCardFill()) },
            { QStringLiteral("nvidiaCardBorder"),     css(palette::nvidiaCardBorder()) },
            { QStringLiteral("nvidiaOnSurface"),      css(palette::nvidiaOnSurface()) },
            { QStringLiteral("amdCardFill"),          css(palette::amdCardFill()) },
            { QStringLiteral("amdCardBorder"),        css(palette::amdCardBorder()) },
            { QStringLiteral("amdOnSurface"),         css(palette::amdOnSurface()) },
            { QStringLiteral("premiumFill"),          css(palette::premiumFill()) },
            { QStringLiteral("premiumFillHover"),     css(palette::premiumFillHover()) },
            { QStringLiteral("premiumFillPressed"),   css(palette::premiumFillPressed()) },
            { QStringLiteral("premiumText"),          css(palette::premiumText()) },
            { QStringLiteral("sidebarItemHover"),     css(palette::sidebarItemHover()) },
            { QStringLiteral("sidebarItemActive"),    css(palette::sidebarItemActive()) },
            { QStringLiteral("sidebarItemSelected"),  css(palette::sidebarItemSelected()) },
            { QStringLiteral("titleControlHover"),    css(palette::titleControlHover()) },
            { QStringLiteral("titleControlPress"),    css(palette::titleControlPress()) },
            { QStringLiteral("titleCloseHover"),      css(palette::titleCloseHover()) },
            { QStringLiteral("titleClosePress"),      css(palette::titleClosePress()) },
            { QStringLiteral("criticalText"),         css(palette::criticalText()) },
            { QStringLiteral("criticalBadgeFill"),    css(palette::criticalBadgeFill()) },
            { QStringLiteral("criticalBadgeEdge"),    css(palette::criticalBadgeEdge()) },
            { QStringLiteral("warningText"),          css(palette::warningText()) },
            { QStringLiteral("scoreGoodText"),        css(palette::scoreGoodText()) },
            { QStringLiteral("warningFill"),          css(palette::warningFill()) },
            { QStringLiteral("warningEdge"),          css(palette::warningEdge()) },
            { QStringLiteral("warnPillFill"),         css(palette::warnPillFill()) },
            { QStringLiteral("warnPillText"),         css(palette::warnPillText()) },
            { QStringLiteral("warnPillFillHover"),    css(palette::warnPillFillHover()) },
            { QStringLiteral("warnPillTextHover"),    css(palette::warnPillTextHover()) },
            { QStringLiteral("radiusCard"),           px(tokens::RadiusCard) },
            { QStringLiteral("radiusModal"),          px(tokens::RadiusModal) },
            { QStringLiteral("promptBadgeRadius"),    px(tokens::PromptBadgeSize / 2) },
            { QStringLiteral("radiusControl"),        px(tokens::RadiusControl) },
            { QStringLiteral("radiusBadge"),          px(tokens::RadiusBadge) },
            { QStringLiteral("radiusPill"),           px(tokens::RadiusPill) },
            { QStringLiteral("ultimateFill"),         css(palette::ultimateFill()) },
            { QStringLiteral("minTarget"),            px(tokens::MinTargetSize) },
            { QStringLiteral("radiusRow"),            px(tokens::RadiusRow) },
            { QStringLiteral("buttonRadius"),         px(tokens::ButtonRadius) },
            { QStringLiteral("buttonHeight"),         px(tokens::ButtonHeight) },
            { QStringLiteral("listInset"),            px(tokens::ListInset) },
            { QStringLiteral("listRowHeight"),        px(tokens::ListRowHeight) },
            { QStringLiteral("buttonHPad"),           px(tokens::ButtonHorizontalPadding) },
            // The spacing scale goes in whole. Omitting a step because it has
            // no user yet is what leaves an unresolved token in the QSS.
            { QStringLiteral("fieldContentHeight"), px(tokens::FieldContentHeight) },
            { QStringLiteral("space2"),               px(tokens::Space2) },
            { QStringLiteral("space4"),               px(tokens::Space4) },
            { QStringLiteral("space8"),               px(tokens::Space8) },
            { QStringLiteral("ruleThickness"),        px(tokens::RuleThickness) },
            { QStringLiteral("focusBorder"),          px(tokens::FocusBorderWidth) },
            { QStringLiteral("fieldPadV"),            px(tokens::FieldPadV) },
            { QStringLiteral("fieldPadH"),            px(tokens::FieldPadH) },
            { QStringLiteral("fieldPadVFocus"),       px(tokens::FieldPadVFocus) },
            { QStringLiteral("fieldPadHFocus"),       px(tokens::FieldPadHFocus) },
            { QStringLiteral("buttonHeightFocus"),    px(tokens::ButtonContentHeightFocus) },
            { QStringLiteral("buttonHPadFocus"),      px(tokens::ButtonHPadFocus) },
            { QStringLiteral("iconButtonPad"),        px(tokens::IconButtonPad) },
            { QStringLiteral("iconButtonPadFocus"),   px(tokens::IconButtonPadFocus) },
            { QStringLiteral("space12"),              px(tokens::Space12) },
            { QStringLiteral("space16"),              px(tokens::Space16) },
            { QStringLiteral("space20"),              px(tokens::Space20) },
            { QStringLiteral("space24"),              px(tokens::Space24) },
            { QStringLiteral("radiusSmall"),          px(tokens::RadiusSmall) },
            { QStringLiteral("radiusHairline"),       px(tokens::RadiusHairline) },
            { QStringLiteral("titleBarHeight"),       px(tokens::TitleBarHeight) },
            { QStringLiteral("captionButtonWidth"),   px(tokens::CaptionButtonWidth) },
            { QStringLiteral("iconButtonSize"),       px(tokens::IconButtonSize) },
            { QStringLiteral("scrollBarThickness"),   px(tokens::ScrollBarThickness) },
            { QStringLiteral("scrollHandleMinLength"), px(tokens::ScrollBarHandleMinLength) },
            { QStringLiteral("tabUnderline"),         px(tokens::TabUnderlineThickness) },
            { QStringLiteral("radioIndicator"),       px(tokens::RadioIndicatorSize) },
            { QStringLiteral("radioIndicatorRadius"), px(tokens::RadioIndicatorRadius) },
            { QStringLiteral("checkIndicator"),       px(tokens::CheckIndicatorSize) },
            { QStringLiteral("progressMinHeight"),    px(tokens::ProgressBarMinHeight) },
            { QStringLiteral("progressMaxHeight"),    px(tokens::ProgressBarMaxHeight) },
            { QStringLiteral("comboContentHeight"),   px(tokens::ComboContentHeight) },
            { QStringLiteral("comboArrowWidth"),      px(tokens::ComboArrowWidth) },
            { QStringLiteral("comboItemHeight"),      px(tokens::ComboItemHeight) },
            { QStringLiteral("menuItemHeight"),       px(tokens::MenuItemHeight) },
            { QStringLiteral("radiusPanel"),          px(tokens::RadiusPanel) },
            { QStringLiteral("radiusStrip"),          px(tokens::RadiusStrip) },
            { QStringLiteral("dotSm"),                px(tokens::DotSm) },
            { QStringLiteral("dotSmRadius"),          px(tokens::DotSmRadius) },
            { QStringLiteral("shellMinWidth"),        px(tokens::ShellMinWidth) },
            { QStringLiteral("shellMinHeight"),       px(tokens::ShellMinHeight) },
        };
        return map;
    }

    void apply(QApplication& app)
    {
        applyWindowsVistaStyle(app);

        // Tabular figures, application-wide.
        QFont f = app.font();
        f.setFeature("tnum", 1);
        app.setFont(f);

        app.setPalette(applicationPalette());
        app.setStyleSheet(styleSheet(g_currentMode));
    }

    Mode currentMode()
    {
        return g_currentMode;
    }

    void applyMenuChrome(QMenu* menu)
    {
        if (!menu)
            return;
        // translucent window so the rounded corners show
        menu->setWindowFlags(menu->windowFlags() | Qt::FramelessWindowHint
                                                 | Qt::NoDropShadowWindowHint);
        menu->setAttribute(Qt::WA_TranslucentBackground, true);
    }

    bool animationsEnabled()
    {
        // Vethida's own switch first, read live so it applies at once.
        if (vth::prefs::reduceMotion())
            return false;
        // Cached: a system preference, not live state.
        static const bool enabled = []() {
            BOOL on = TRUE;
            if (!SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &on, 0))
                return true;
            return on != FALSE;
        }();
        return enabled;
    }

    int animationDuration(int preferredMs)
    {
        // Zero, not "short": QPropertyAnimation with duration 0 assigns the end
        // value on start, so the widget lands in its final state with no travel.
        return animationsEnabled() ? preferredMs : 0;
    }
}

