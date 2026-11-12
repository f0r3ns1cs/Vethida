#pragma once

#include "Theme.h"

#include <QColor>
#include <QFont>
#include <QStringList>

namespace vth::theme::tokens
{
    // Never introduce a spacing value that is not a step on this 4px grid.
    constexpr int Space2  = 2;    // optical only: icon against its own label
    constexpr int Space4  = 4;
    constexpr int Space8  = 8;
    constexpr int Space12 = 12;
    constexpr int Space16 = 16;
    constexpr int Space20 = 20;
    constexpr int Space24 = 24;
    constexpr int Space32 = 32;
    constexpr int Space40 = 40;

    // Semantic aliases; prefer these at call sites.
    constexpr int TightGap    = Space4;    // icon <-> text, label <-> sublabel
    constexpr int CompactGap  = Space8;    // between buttons in a row
    constexpr int SmallMargin = Space8;    // control <-> its header
    constexpr int CardMargin  = Space12;   // between content areas; card inner gap
    constexpr int RegionMargin = Space16;  // surface edge -> text

    // Card inner inset. One value for every card in the app.
    constexpr int CardInsetH = Space16;
    constexpr int CardInsetV = Space12;

    constexpr int DialogInsetH      = Space24;
    constexpr int DialogInsetTop    = Space20;
    constexpr int DialogInsetBottom = Space20;
    // Inset for a modal that fills the content region rather than floating at
    // ModalWidthSm.
    constexpr int DialogInsetWideH  = Space32;

    // Inset for a single interactive row (rail item, list row).
    constexpr int RowInsetH = Space12;
    constexpr int RowInsetV = Space8;

    // keeps children off the rounded corner (QSS radius doesn't clip). Keep >= RadiusCard/2.
    constexpr int ListInset = Space8;
    constexpr int ListRowHeight = 36;

    constexpr int HeaderLaneHeight = 28;

    // The dashboard's status strip, tall enough for a ControlHeight button.
    constexpr int StatusStripHeight = 64;

    // Standard page padding. PageScaffold owns it; do not hand-roll page margins.
    constexpr int PageMarginH = 24;
    constexpr int PageMarginV = 24;
    constexpr int PageMarginTop = PageMarginV;
    constexpr int PageMarginBottom = PageMarginV;

    // Modal width scale. Height stays content-driven.
    constexpr int ModalWidthSm = 420;
    // The tinted disc at the top of a prompt that carries its tone glyph.
    constexpr int PromptBadgeSize = 40;   // confirmations & short prompts
    constexpr int ModalWidthMd = 520;   // detail cards, forms, wizards
    constexpr int ModalWidthLg = 680;   // rich content (help, about, explainers)

    // 8 rows
    constexpr int ModalScrollMaxHeight = ListRowHeight * 8;

    // Margin left around a content-region PanelDialog.
    constexpr int PanelModalMargin = Space16;

    constexpr int ButtonHeight = 32;
    constexpr int ButtonHorizontalPadding = 14;
    // Floor for a dialog's action button, so a two-button row comes out even.
    // Padding alone cannot do this: it scales with the text.
    constexpr int ButtonMinWidth = 96;

    // Width of a navigation rail column. A rail that also carries the page's own
    // left margin adds PageMarginH to this rather than restating it.
    constexpr int NavRailWidth = 292;

    // The nav-rail item states its own height; the blanket control metrics
    // apply to everything it does not state.
    constexpr int RailItemHeight = 64;

    // Windows 11's rounding scale, 4px for in-page elements and 8px for their
    // containers:
    // learn.microsoft.com/windows/apps/design/signature-experiences/geometry
    constexpr int ButtonRadius  = ButtonHeight / 2;   // buttons, icon buttons
    constexpr int RadiusControl = 8;    // fields: dropdowns, line edits, spin boxes, menu items
    constexpr int RadiusCard    = 8;    // cards, panels, menus, popovers
    constexpr int RadiusPanel   = 8;
    // Prompts and panel modals. A step rounder than the cards under them, so a
    // dialog reads as lifted off the page rather than as one more card on it.
    constexpr int RadiusModal   = 12;
    // Wide backplates: a settings row, a table, a popup list. These follow the
    // container value, never the control one.
    constexpr int RadiusRow     = RadiusCard;
    // Small status badges and count pills only, where a fixed ~20px height makes
    // this a pill or a circle. Never use it on a button or a card.
    constexpr int RadiusBadge   = 12;
    // A capsule: both ends fully round. Half the height of a chip, and it has to
    // stay half the height of a chip.
    constexpr int RadiusPill    = 10;

    // The dashboard's status strip.
    constexpr int RadiusStrip   = 10;
    // checkbox indicator, progress track
    constexpr int RadiusSmall   = 4;
    // A bar only a few pixels thick: the scrollbar handle, the progress chunk.
    // RadiusSmall on either of those rounds the whole bar away.
    constexpr int RadiusHairline = 3;

    // Floor for anything clickable that draws no text.
    constexpr int MinTargetSize = 24;

    // A square icon button: one button tall and as wide as it is tall.
    constexpr int IconButtonSize = ButtonHeight;

    // Scrollbars. The bar is the full thickness; the slim look comes from the
    // handle's margins - see the note above the QScrollBar rules in Theme.cpp.
    constexpr int ScrollBarThickness = 12;
    constexpr int ScrollBarHandleMinLength = 36;

    // checkbox/radio indicator size
    constexpr int RadioIndicatorSize = 18;
    constexpr int RadioIndicatorRadius = RadioIndicatorSize / 2;
    constexpr int CheckIndicatorSize = 16;

    // The progress bar is a band, not a control: it has a floor so it is
    // visible and a ceiling so a layout cannot stretch it into a slab.
    constexpr int ProgressBarMinHeight = 6;
    constexpr int ProgressBarMaxHeight = 18;

    // combo box: content height matches a line edit, the arrow column is reserved width
    constexpr int ComboContentHeight = 26;
    constexpr int ComboArrowWidth = 22;
    constexpr int ComboItemHeight = 24;

    // One row of a QMenu or of the Dropdown popup that mirrors it.
    constexpr int MenuItemHeight = 22;

    constexpr int TweakIconBadgeSize = 56;
    constexpr int TweakIconSize = 28;
    constexpr int SwitchWidth = 36;
    constexpr int SwitchHeight = 20;
    constexpr int SwitchThumbSize = 14;
    constexpr int SwitchInset = 3;
    // Labeled variant.
    constexpr int SwitchLabeledWidth  = 76;
    constexpr int SwitchLabeledHeight = 36;
    constexpr int SwitchLabeledInset  = Space4;
    // The compact track is SwitchHeight tall; the widget is taller so the hit
    // target clears MinTargetSize and the focus ring has room.
    constexpr int SwitchHitHeight = Space24;

    // A modal's dismiss affordance: the return arrow (Cover) and the X
    // (ScrimPanel) are the same control in two guises and share one size.
    constexpr int ModalCloseSize  = Space40;
    constexpr int ModalCloseGlyph = Space24;

    // Leading glyph inside a card row.
    constexpr int CardBadgeSize = 40;
    // The oversized mark on a full-surface confirmation state.
    constexpr int HeroBadgeSize = 64;
    // The centered mark on a state panel - empty-state spinner, tier-overlay lock.
    constexpr int StateMarkSize = 44;

    // Custom title bar. The close mark is drawn larger than the minimize and
    // maximize marks: an X occupies less of its box than a bar or a square.
    constexpr int TitleBarHeight    = 54;
    // The plan pill in the title bar. A step under the icon button beside it,
    // so the pill reads as a label on the bar rather than a second button.
    constexpr int TitleBadgeHeight  = ButtonHeight - Space4;

    // The shortest the main window may be dragged, and the narrowest.
    constexpr int ShellMinWidth     = 1120;
    constexpr int ShellMinHeight    = 800;
    // Wider than it is tall, which is the Windows caption-button proportion.
    constexpr int CaptionButtonWidth = 58;
    constexpr int CaptionGlyph      = 15;
    constexpr int CaptionGlyphClose = 19;

    // The square a rendered glyph occupies, which is NOT the control carrying it:
    // ModalCloseSize above is a hit target, these are what gets drawn inside one.
    constexpr int GlyphSm = 14;   // inside a compact control: toast dismiss
    constexpr int GlyphMd = 16;   // default inline glyph beside text
    constexpr int GlyphLg = 20;   // standalone marker: section nav, wizard step
    constexpr int GlyphXl = 24;   // leading glyph on a card header row

    // The round on/off control. Dots and duty cycle solve the dash pattern against
    // the circumference at paint time.
    constexpr int   ToggleDiscSize      = 44;
    constexpr int   ToggleMarkSize      = 20;
    constexpr qreal ToggleRingWidth     = 1.5;
    constexpr int   ToggleRingDots      = 16;
    constexpr qreal ToggleRingDutyCycle = 0.4;

    // A card footer's row height, from the tallest control a footer carries, so
    // buttons line up across cards in one row.
    constexpr int CardFooterHeight = ToggleDiscSize;

    // Status dot. Color is the state; the size is fixed. The radius is derived so
    // it cannot stop being a circle when the size moves.
    constexpr int DotSm = 8;
    constexpr int DotMd = 12;
    constexpr int DotSmRadius = DotSm / 2;

    constexpr int RuleThickness = 1;

    // A tab's selected underline is the bar's own rule, thickened, so it reads
    // as a segment of that line rather than a second line floating above it.
    constexpr int TabUnderlineThickness = RuleThickness * 2;

    // The thicker focus border would move the text, so the focus padding is 1px smaller.
    // content + 2*FieldPadV + 2*RuleThickness == content + 2*FieldPadVFocus + 2*FocusBorderWidth
    constexpr int FocusBorderWidth = 2;
    constexpr int FieldPadV = Space4;
    constexpr int FieldPadH = Space8;
    constexpr int FieldPadVFocus = FieldPadV - (FocusBorderWidth - RuleThickness);
    constexpr int FieldPadHFocus = FieldPadH - (FocusBorderWidth - RuleThickness);

    // Same compensation for a button, whose height is pinned by min-height AND
    // max-height - both measured against the content box.
    constexpr int ButtonContentHeightFocus =
        ButtonHeight - 2 * (FocusBorderWidth - RuleThickness);
    constexpr int ButtonHPadFocus =
        ButtonHorizontalPadding - (FocusBorderWidth - RuleThickness);

    // And for an icon button, whose padding is its only inset.
    constexpr int IconButtonPad = Space4;
    constexpr int IconButtonPadFocus = IconButtonPad - (FocusBorderWidth - RuleThickness);

    // Minimum width of a filter or search control above a list.
    constexpr int FilterControlWidth = 240;

    // Width of a settings-page dropdown.
    constexpr int SettingsControlWidth = 220;
    // A cap, not a floor: a control holding its own short content.
    constexpr int FieldWidthCompact = 96;
    // A control that has to show a long value in full: an adapter name, a
    // firmware setting's stored string.
    constexpr int FieldWidthWide = 280;
    // Height of a text-entry-sized control (dropdown trigger, line edit).
    constexpr int ControlHeight = ButtonHeight;
    // QSS min-height is the content box, so take off padding and border
    constexpr int FieldContentHeight = ControlHeight - 2 * Space4 - 2;
}

// Type scale (major second) from Base and Ratio. Don't add one-off sizes.
// Body/caption Regular, headings Medium, page title Strong.
namespace vth::theme::type
{
    constexpr int    Base  = 14;      // ms(0), body
    constexpr double Ratio = 1.125;

    constexpr int ms(int step)
    {
        double v = Base;
        for (int i = 0; i < step; ++i) v *= Ratio;
        for (int i = 0; i > step; --i) v /= Ratio;
        return static_cast<int>(v + 0.5);
    }

    constexpr int Caption = ms(-1);   // 12 - metadata, pills, secondary labels
    constexpr int Body    = ms(0);    // 14 - all primary reading content
    constexpr int Title   = ms(1);    // 16 - card and row titles
    constexpr int Heading = ms(2);    // 18 - section and modal headings
    constexpr int Display = ms(3);    // 20 - page title
    // A single large numeral read at a glance.
    constexpr int Readout = ms(9);    // 40 - the score gauge's number

    constexpr int WeightRegular = 400;
    constexpr int WeightMedium  = 500;
    constexpr int WeightStrong  = 600;
}

namespace vth::theme::palette
{
    inline bool isDark()
    {
        return currentMode() == Mode::Dark;
    }

    inline QColor hx(unsigned rgb)
    {
        return QColor(static_cast<int>((rgb >> 16) & 0xFF),
                      static_cast<int>((rgb >> 8) & 0xFF),
                      static_cast<int>(rgb & 0xFF));
    }

    inline QColor pick(unsigned dark, unsigned light)
    {
        return isDark() ? hx(dark) : hx(light);
    }

    // Surfaces get lighter with elevation (chrome, canvas, card, modal), each step
    // lighter than the last in dark mode. Slight blue tint on purpose.
    inline QColor appChrome()          { return pick(0x0B0B0C, 0xE4E7EB); }
    inline QColor appContent()         { return pick(0x101012, 0xEAECF0); }
    inline QColor cardSurface()        { return pick(0x17171A, 0xFBFCFD); }
    inline QColor cardSurfaceRaised()  { return pick(0x1F1F22, 0xFFFFFF); }
    // Carries the card on its own where the fills are close, so it has to be
    // legible against BOTH of them rather than a hairline that disappears.
    inline QColor cardBorder()         { return pick(0x2A2A2F, 0xD3D8DE); }
    inline QColor cardBorderEmphasis() { return pick(0x3A3A40, 0xC4C4C4); }
    inline QColor divider()            { return pick(0x222226, 0xE2E2E2); }
    // control borders, >= 3:1 against every surface they sit on
    inline QColor controlBorder()      { return pick(0x7A7A80, 0x787878); }

    inline QColor textPrimary()        { return pick(0xF2F2F3, 0x161616); }
    inline QColor textSecondary()      { return pick(0xBDBDC2, 0x555A64); }
    // Muted is the quietest text, but it is still text: solved against the worst
    // of the three surfaces it appears on.
    inline QColor textMuted()          { return pick(0x9C9CA3, 0x595D66); }

    // A pill sits on a card.
    inline QColor badgeFill()          { return pick(0x111114, 0xECEFF3); }
    inline QColor badgeBorder()        { return pick(0x29292E, 0xCFD5DD); }

    inline QColor tagPillFill()   { return pick(0x1A2B4A, 0xDFE7F8); }
    inline QColor tagPillText()   { return pick(0x9DB8E0, 0x1D56C8); }
    // The one neutral icon color. vth::icons::iconColor() returns this; never
    // hard-code an icon gray, which silently drifts off the ramp.
    inline QColor iconNeutral()        { return pick(0x88888F, 0x565656); }
    inline QColor iconDisabled()       { return pick(0x3A3A40, 0xA5A5A5); }
    // accentFill is for fills under white text, accentOnSurface for text/focus rings.
    // Don't merge them.
    inline QColor accentFill()         { return pick(0x3A5F9F, 0x3A5F9F); }
    inline QColor accentFillHover()    { return pick(0x446BAD, 0x446BAD); }
    inline QColor accentFillPressed()  { return pick(0x34578F, 0x34578F); }
    inline QColor accentOnSurface()      { return pick(0x7FA3D9, 0x1D56C8); }
    inline QColor accentOnSurfaceHover() { return pick(0xA3BFE6, 0x1747A6); }
    inline QColor linkText()             { return accentOnSurface(); }
    inline QColor accentText()         { return pick(0xFFFFFF, 0xFFFFFF); }
    inline QColor featureMark()        { return pick(0x4C8DFF, 0x2F6BFF); }
    // The top tier's own mark. A fill, under accentText.
    inline QColor ultimateFill()       { return pick(0x6D4FD0, 0x6D28D9); }
    // The title-bar plan pill is the one clickable thing wearing this fill.
    // Both steps still carry accentText at 4.5:1.
    inline QColor ultimateFillHover()  { return pick(0x7A5DDA, 0x7C3AED); }
    inline QColor ultimateFillPressed(){ return pick(0x5F43BE, 0x5B21B6); }
    // vendor colors (set through FeatureCard::Accent)
    inline QColor nvidiaMark()         { return pick(0x76B900, 0x5E9400); }
    inline QColor amdMark()            { return pick(0xED1C24, 0xC8102E); }
    // readable text versions of the vendor colors (use these for chip text)
    inline QColor nvidiaOnSurface()    { return pick(0xB4E600, 0x3F6300); }
    inline QColor amdOnSurface()       { return pick(0xFF8A6E, 0x9B0C22); }
    // The vendor chip wash.
    inline QColor nvidiaCardFill()     { return pick(0x18280E, 0xEFF7E0); }
    inline QColor nvidiaCardBorder()   { return pick(0x3C5C1A, 0xC2DE93); }
    inline QColor amdCardFill()        { return pick(0x2A1214, 0xFCEBEB); }
    inline QColor amdCardBorder()      { return pick(0x6B2226, 0xF0BFBF); }
    // gold is only for the lock badges, tier pills use the accent
    inline QColor premiumFill()        { return pick(0xE9C046, 0xE9C046); }
    inline QColor premiumFillHover()   { return pick(0xF2CD5C, 0xF2CD5C); }
    inline QColor premiumFillPressed() { return pick(0xD8AF35, 0xD8AF35); }
    inline QColor premiumText()        { return pick(0x1F1808, 0x1F1808); }
    // The accent wash: a selected tile, and the Pro plan pill in the title bar.
    // The pill is the one place it answers the pointer, hence the hover step.
    inline QColor licensePillFill()      { return pick(0x132036, 0xE6ECF8); }
    inline QColor licensePillFillHover() { return pick(0x1A2B48, 0xDAE3F5); }
    inline QColor licensePillText()      { return pick(0x9DB8E0, 0x1D56C8); }
    inline QColor licensePillBorder()    { return pick(0x2E4670, 0xC2D2F2); }
    inline QColor sidebarItemHover()   { return pick(0x28282E, 0xDEE0E8); }
    inline QColor sidebarItemActive()  { return pick(0x313137, 0xD5D7DE); }
    // accent tint so it stands out in light mode
    inline QColor sidebarItemSelected() { return pick(0x172540, 0xD2DDF2); }
    // The same ink as the label under it.
    inline QColor sidebarIconIdle()    { return pick(0xBDBDC2, 0x555555); }
    inline QColor titleCloseHover()    { return pick(0xEE2A20, 0xDE1810); }
    // Solved against appChrome, which is what the caption buttons sit on.
    inline QColor titleControlHover()  { return pick(0x2A2A30, 0xD4D7DF); }
    inline QColor titleControlPress()  { return pick(0x36363C, 0xC7CAD3); }
    inline QColor titleClosePress()    { return pick(0xC81E14, 0xB01108); }

    inline QColor criticalBorder()     { return pick(0x8C3030, 0xB94A4A); }
    inline QColor criticalText()       { return pick(0xD88383, 0x8A2727); }
    inline QColor criticalBadgeFill()  { return pick(0x211010, 0xF2E4E4); }
    inline QColor criticalBadgeEdge()  { return pick(0x5A2727, 0xD0A0A0); }

    inline QColor warningText()        { return pick(0xEDA52E, 0x7D520F); }
    inline QColor warningFill()        { return pick(0x251B05, 0xFBF0D4); }
    inline QColor warningEdge()        { return pick(0xC8861F, 0xC4912A); }
    inline QColor warnPillFill()       { return pick(0x4A3911, 0xFBE6BE); }
    inline QColor warnPillText()       { return pick(0xF6B53E, 0x7A4E0C); }
    inline QColor warnPillFillHover()  { return pick(0x5C4614, 0xF8DBA4); }
    inline QColor warnPillTextHover()  { return pick(0xFFC75A, 0x6A4209); }
    inline QColor cautionBannerFill()  { return pick(0xF0A030, 0xF0A030); }
    inline QColor cautionBannerText()  { return pick(0x1A1206, 0x1A1206); }

    // dark track + white thumb in both modes
    inline QColor toggleTrack()        { return pick(0x7B7C80, 0x797A7E); }
    // darker, the ON/OFF label is white on the track
    inline QColor toggleTrackLabeled() { return pick(0x676971, 0x666A71); }
    // The accent itself, aliased rather than restated.
    inline QColor toggleTrackOn()      { return accentFill(); }
    inline QColor toggleThumb()        { return pick(0xFFFFFF, 0xFFFFFF); }
    inline QColor toggleThumbOn()      { return pick(0xFFFFFF, 0xFFFFFF); }
    // Hover states are stated rather than derived: computing them with
    // .lighter()/.darker() at the paint site puts them outside check-contrast.
    inline QColor toggleTrackHover()        { return pick(0x86878B, 0x858689); }
    inline QColor toggleTrackOnHover()      { return accentFillHover(); }
    inline QColor toggleTrackLabeledHover() { return pick(0x74767D, 0x73777D); }

    inline QColor scoreGoodText()      { return pick(0x40D2BA, 0x116B5D); }
    inline QColor scoreFairText()      { return pick(0xE2C700, 0x725D00); }
    inline QColor scorePoorText()      { return pick(0xD88383, 0x8A2727); }

    // gauge arc gradient start: accentOnSurface 35% toward white
    inline QColor gaugeArcStart()      { return pick(0xACC3E6, 0x6C91DB); }

    inline QColor storageWindows()     { return pick(0x3B82F6, 0x2563EB); }
    inline QColor storageApps()        { return pick(0x22C1CC, 0x0E9AA7); }
    inline QColor storageUsers()       { return pick(0x8B7CF6, 0x6D5DE0); }
    inline QColor storageSystemData()  { return pick(0x38BDF8, 0x0EA5E0); }
    inline QColor storageTemporary()   { return pick(0xF6B45E, 0xD98A2B); }
    inline QColor storageSystemFile()  { return pick(0x5EC8F2, 0x2F9BD4); }
    inline QColor storageRecycleBin()  { return pick(0x4ADE80, 0x1FA85A); }
    inline QColor storageOther()       { return pick(0x8DA0C6, 0x6E7E9E); }
    inline QColor storageFree()        { return pick(0x28282E, 0xD7DCE6); }

    // Two scrims, because they do opposite jobs.
    inline QColor scrimOverlay()       { return QColor(0, 0, 0, 150); }
    inline QColor scrimLocked()        { return QColor(0, 0, 0, 214); }
    // The drop shadow under a modal card, lifting it off the scrim.
    inline QColor modalShadow()        { return QColor(0, 0, 0, isDark() ? 140 : 56); }
}
