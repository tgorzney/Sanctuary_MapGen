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
};

// One frame of header interaction: a click toggles, anything else reports the current state. Pure
// so a tab's open/closed behavior is testable without an imgui frame.
inline SectionChange StepSectionHeader(SectionState& state, bool bHeaderClicked) {
    SectionChange change;
    if (bHeaderClicked) {
        state.bOpen = !state.bOpen;
        change.bOpenChanged = true;
    }
    change.bBodyVisible = state.bOpen;
    return change;
}

// Draws the header bar and returns true when the BODY should be drawn. Call DrawSectionEnd only
// on the frames this returned true (the imgui Begin/End convention):
//
//   if (Ui::DrawSectionBegin("Sun", tabState.sunSection)) { ...controls...; Ui::DrawSectionEnd(); }
//
bool DrawSectionBegin(const char* label, SectionState& state,
                      const SectionOptions& options = SectionOptions(),
                      const WidgetStyle& style = WidgetStyle());

// Closes the body opened above, unwinding exactly the indent DrawSectionBegin applied.
void DrawSectionEnd(const SectionOptions& options = SectionOptions());

} // namespace Ui
} // namespace SanmapGen
