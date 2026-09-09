// Section_UI.h — the collapsing section header every tab is built from. Layer: UI.
// Accuracy class: Visual. UI_FRAMEWORK_SPEC "Universal widget library": the v2 tabs are a stack
// of collapsing sections (Sun, Skylight, Fog, Soil Physics, Hydraulic Erosion, Advanced
// (constants) ...), so the header is written ONCE here instead of per tab.
//
// Drawn with the bypass toolkit — an InvisibleButton over an ImDrawList bar and arrow, not
// ImGui::CollapsingHeader — so it is styled by WidgetStyle like the rest of the library
// (Section_UI.cpp). The open/closed decision itself is pure and lives here.
//
// Owns no app state: the caller holds one SectionState per section (in its own tab state, never
// a function static — the v1 bug this library exists to kill).
#pragma once
#include "WidgetHelpers_UI.h"

namespace SanmapGen {
namespace Ui {

// Per-section tweakables (Constitution §8).
struct SectionOptions {
    bool  bDefaultOpen   = true;
    bool  bArrowShown    = true;
    float indentWidth    = 0.0f;    // <= 0: imgui's own indent for the section body
    float headerRounding = -1.0f;   // < 0: the WidgetStyle/theme rounding
    // Pixels of the header bar's right edge left undrawn/unclickable, for a caller to compose a
    // button into via SameLine() immediately after DrawSectionBegin returns. 0 = today's exact
    // behavior, a full-width header (STEP104).
    float reservedRightWidth = 0.0f;
    // STEP142 — vertical gap DrawSectionBegin leaves ABOVE its own header bar (human's own
    // instruction: sections ran together with no visual separation). Applied at the START of every
    // DrawSectionBegin call, not the end of DrawSectionEnd, so the gap appears BEFORE the section
    // whether the PREVIOUS one was left open or collapsed (DrawSectionEnd only ever runs for an open
    // section's own caller, so a trailing-gap approach would miss the collapsed case).
    float topSpacing = 6.0f;
    // STEP258 — opt-in only: default false preserves every existing header's single-click-to-toggle
    // behavior UNCHANGED. When true, ONLY the second click of a double-click (the frame Dear ImGui
    // itself resolves as click-count 2 — its own io.MouseClickedCount, imgui.cpp's UpdateMouseInputs)
    // toggles bOpen; a plain single click (count 1) does not touch bOpen at all and instead reports
    // bPlainSingleClicked back through SectionChange/DrawSectionBegin's own out-param, for a caller that
    // wants a single click to mean something OTHER than expand/collapse (STEP258's own Link-header
    // "select all its instances" use, MarkersTab_Links_UI.h). A triple-plus click (count >= 3) does
    // neither — inert by construction, not a defect: no repeated select-all, no accidental extra toggle.
    // No other header in the app sets this — LinkSectionHeaderOptions() is, as of STEP258, the ONLY
    // SectionOptions call site in the entire codebase that does.
    bool bDoubleClickToggle = false;
};

// The caller-owned bit. One per section instance.
struct SectionState {
    bool bOpen = true;
};

// A state seeded from the options — how a tab initializes a section it wants closed by default.
inline SectionState InitialSectionState(const SectionOptions& options) {
    SectionState state;
    state.bOpen = options.bDefaultOpen;
    return state;
}

// What one header did this frame.
//   bOpenChanged — the section toggled, i.e. the caller may want to remember the new layout.
//   bBodyVisible — draw the body this frame.
struct SectionChange {
    bool bOpenChanged = false;
    bool bBodyVisible = false;
    // STEP258 — true on the exact frame a header configured with SectionOptions::bDoubleClickToggle
    // received a PLAIN single click (click-count 1, not the second click of a double-click) — the
    // caller's cue to run ITS OWN single-click action instead of a toggle. Always false whenever
    // bDoubleClickToggle is false (every pre-existing header): those toggle on any click and never
    // populate this field, so an existing caller that ignores it loses nothing.
    bool bPlainSingleClicked = false;
};

// One frame of header interaction: pure so a tab's open/closed behavior is testable without an
// imgui frame. `headerClickCount` is 0 (no click), 1 (a plain single click) or 2+ (the second-or-
// later click of a multi-click) this frame — Section_UI.cpp is the only caller that ever derives
// this from real mouse state (ImGui::GetMouseClickedCount). `bDoubleClickToggle` mirrors
// SectionOptions of the same name (STEP258): false (every pre-existing header) toggles on ANY
// click, exactly today's behavior; true toggles ONLY on count==2 and reports a count==1 click via
// bPlainSingleClicked instead of touching bOpen (count >= 3 does neither, see SectionOptions' own
// comment).
inline SectionChange StepSectionHeader(SectionState& state, int headerClickCount,
                                       bool bDoubleClickToggle = false) {
    SectionChange change;
    const bool bTogglingClick = bDoubleClickToggle ? (headerClickCount == 2) : (headerClickCount >= 1);
    if (bTogglingClick) {
        state.bOpen = !state.bOpen;
        change.bOpenChanged = true;
    }
    change.bPlainSingleClicked = bDoubleClickToggle && headerClickCount == 1;
    change.bBodyVisible = state.bOpen;
    return change;
}

// Draws the header bar and returns true when the BODY should be drawn. Call DrawSectionEnd only
// on the frames this returned true (the imgui Begin/End convention):
//
//   if (Ui::DrawSectionBegin("Sun", tabState.sunSection)) { ...controls...; Ui::DrawSectionEnd(); }
//
// `outPlainSingleClicked`, if non-null, is written every call (STEP258): true only on the frame
// SectionChange::bPlainSingleClicked is true (see there). Every EXISTING call site passes nullptr
// (the default) and is completely unaffected; only a caller that also set
// SectionOptions::bDoubleClickToggle has any reason to read it.
bool DrawSectionBegin(const char* label, SectionState& state,
                      const SectionOptions& options = SectionOptions(),
                      const WidgetStyle& style = WidgetStyle(),
                      bool* outPlainSingleClicked = nullptr);

// Closes the body opened above, unwinding exactly the indent DrawSectionBegin applied.
void DrawSectionEnd(const SectionOptions& options = SectionOptions());

} // namespace Ui
} // namespace SanmapGen
