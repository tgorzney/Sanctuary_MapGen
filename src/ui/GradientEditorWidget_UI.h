// GradientEditorWidget_UI.h — the color-ramp editor: add / move / delete / recolor the stops of
// ONE Params::GradientRamp, plus its smooth-vs-linear toggle (M5-3).
// Layer: UI (UI_FRAMEWORK_SPEC, "universal widget library" — one shared implementation every tab
// draws from). Accuracy class: Visual.
//
// The widget does NOT own the ramp and does NOT bake it. The caller holds the
// Params::GradientRamp, passes it by reference, and — when a draw call returns true — re-bakes
// with Ui::BakeGradientLut (M4-2) and trips its own dirty flag (UI_FRAMEWORK_SPEC two-tier
// flags: a recolor is bNeedsPreviewRender, never a full regen). Reading and mutating PARAMS from
// UI is downward and legal (ARCH §3.1); nothing here simulates or re-derives a DATA field (§3.2).
//
// The mutations below are deliberately pure and imgui-free (GradientEditorWidget_UI.cpp) so the
// edit semantics are sandbox-testable without a live imgui frame; only the drawing needs one
// (GradientEditorWidget_Draw_UI.cpp — the ARCH §1.5 aspect split behind this one small header).
#pragma once
#include "../params/GradientRamp_PARAMS.h"
#include "RtToggleWidget_UI.h"

namespace SanmapGen {
namespace Ui {

// Validation fence on the stop count (Constitution §6): it only fences off nonsense input (a
// runaway click-to-add), it is not the setting itself.
enum : int { kMaximumGradientStopCount = 1024 };

// Channel count of one stop color (linear RGBA), mirroring Ui::kLookupChannelCount.
enum : int { kGradientStopChannelCount = 4 };

// Per-widget interaction state, owned by the caller — one per editor instance. It holds no ramp
// data, only what a live drag needs plus the widget's own tweakables (Constitution §8).
struct GradientEditorState {
    int   selectedStopIndex = -1;      // -1 when nothing is selected
    int   draggedStopIndex  = -1;      // -1 when no drag is in flight
    float stripeHeight      = 28.0f;   // on-screen height of the ramp strip, in pixels
    float handleRadius      = 6.0f;    // stop-handle radius / grab tolerance, in pixels
    int   stripeSampleCount = 64;      // LUT entries used to paint the strip (never hardcoded)
    // Drives the selected stop's Ui::DrawColorSwatch. The picker already writes the stop's color
    // array live every frame it is open (ColorSwatch_UI.cpp), matching the old ImGui::ColorEdit4
    // path's own every-frame write; this toggle only governs DrawColorSwatch's own RT button.
    RealtimeToggle stopColorRealtimeToggle;
};

// Per-call tweakables (Constitution §8) for one DrawGradientEditor instance. Defaults reproduce
// today's behavior byte-for-byte for every call site that does not opt in — Slope is currently the
// only one that does (SlopeTab_UI.cpp), because it is the only one of the five gradient tabs with a
// bounded real-world unit for its stop locations (WO BUGFIX_SlopeTabUICorrection_R1 Part 3).
struct GradientEditorOptions {
    // nullptr on either pair member means identity (the stop's raw 0..1 `location`, unchanged
    // behavior). When both are set, the stop-location slider's range becomes
    // [ToDisplayUnits(0), ToDisplayUnits(1)] instead of the hardcoded 0..1.
    float (*ToDisplayUnits)(float normalizedLocation) = nullptr;
    float (*FromDisplayUnits)(float displayValue)     = nullptr;
    const char* locationSliderLabel                   = "Stop location";
    // Skips the ImGui::TextUnformatted(label) line only; ImGui::PushID(label) always still runs,
    // so this never affects the widget's ID scoping (mirrors ColorSwatchOptions::bLabelHidden).
    bool bLabelHidden = false;
};

// ---------------------------------------------------------------------------------------------
// Pure edit semantics — no imgui, no GL. Each reports whether the ramp ACTUALLY changed, so a
// no-op edit never trips the caller's dirty flag or forces a re-bake.
// ---------------------------------------------------------------------------------------------

// Inserts a stop at `location` (clamped to 0..1) with `color` (linear RGBA; nullptr keeps the
// Params::GradientStop default), before the first stop that sits strictly later — so a sorted
// ramp stays sorted. Returns the new stop's index, or -1 when the ramp already holds
// kMaximumGradientStopCount stops.
int AddGradientStop(Params::GradientRamp& ramp, float location,
                    const float color[kGradientStopChannelCount]);

// Moves one stop to `newLocation` (clamped to 0..1). Stop IDENTITY (its index) is preserved even
// when it is dragged past a neighbour, so a drag never swaps out from under the cursor; the
// vector may therefore end up unsorted, which Ui::BakeGradientLut sorts defensively into its own
// local copy (M4-2), so the baked LUT is identical either way.
bool MoveGradientStop(Params::GradientRamp& ramp, int stopIndex, float newLocation);

// Erases one stop. Deleting the last stop is legal: an empty ramp bakes a safe constant table.
bool DeleteGradientStop(Params::GradientRamp& ramp, int stopIndex);

// Replaces one stop's linear RGBA.
bool RecolorGradientStop(Params::GradientRamp& ramp, int stopIndex,
                         const float color[kGradientStopChannelCount]);

// The smooth (smoothstep) vs linear interpolation toggle.
bool SetGradientSmoothInterpolation(Params::GradientRamp& ramp, bool bSmoothInterpolation);

// Index of the stop closest to `location`, or -1 for an empty ramp (the drag pick-up query).
int NearestGradientStopIndex(const Params::GradientRamp& ramp, float location);

// ---------------------------------------------------------------------------------------------
// The widget itself (GradientEditorWidget_Draw_UI.cpp) — needs a live imgui frame.
// ---------------------------------------------------------------------------------------------

// Draws the strip, its stop handles, the smooth/linear toggle and the selected-stop controls,
// mutating `ramp` through the functions above. Returns true iff the ramp changed this frame —
// the caller's cue to re-bake via Ui::BakeGradientLut and trip its preview dirty flag.
// `options` defaults to identity/visible, so every call site but Slope's is source- and
// behavior-unchanged.
bool DrawGradientEditor(const char* label, Params::GradientRamp& ramp, GradientEditorState& state,
                        const GradientEditorOptions& options = GradientEditorOptions());

} // namespace Ui
} // namespace SanmapGen
