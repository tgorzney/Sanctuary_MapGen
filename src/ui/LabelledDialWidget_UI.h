// LabelledDialWidget_UI.h — the labelled scalar knob + numeric field. Layer: UI.
// UI_FRAMEWORK_SPEC "Universal widget library": the single dial every tab uses for a bounded
// scalar. Drawn with ImDrawList (PathArcTo + AddCircleFilled) behind one InvisibleButton, per the
// bypass toolkit §1; drawing lives in the .cpp, everything here is pure and headless-testable.
//
// Owns no app state: the caller holds the float and one Ui::RealtimeToggle, and reads the
// WidgetChange back.
#pragma once
#include "RtToggleWidget_UI.h"
#include "WidgetHelpers_UI.h"

namespace SanmapGen {
namespace Ui {

// The scalar's limits, its snap increment, and how far the mouse travels to sweep the whole
// range. All three are settings, not literals in the drag code (Constitution §8).
struct DialRange {
    float minimumValue       = 0.0f;
    float maximumValue       = 1.0f;
    float increment          = 0.0f;     // <= 0: continuous, no snapping
    float pixelsForFullSweep = 200.0f;   // vertical drag distance covering minimum -> maximum
};

// Repairs nonsense limits once (inverted limits swap), so every function below agrees.
inline DialRange ResolvedDialRange(DialRange range) {
    if (range.maximumValue < range.minimumValue) {
        const float swap = range.minimumValue;
        range.minimumValue = range.maximumValue;
        range.maximumValue = swap;
    }
    return range;
}

// Legal value: inside the limits and on the increment lattice measured from minimumValue. The
// final clamp is what stops a snap from stepping past the top of the range.
inline float ClampDialValue(float value, const DialRange& rawRange) {
    const DialRange range = ResolvedDialRange(rawRange);
    const float clamped = ClampToRange(value, range.minimumValue, range.maximumValue);
    return ClampToRange(QuantizeToIncrement(clamped, range.minimumValue, range.increment),
                        range.minimumValue, range.maximumValue);
}

// Applies one frame of vertical drag. `dragDeltaY` is in screen pixels with +y DOWN, so dragging
// UP raises the value — the knob convention. A non-positive pixelsForFullSweep freezes the drag
// instead of dividing by zero (Constitution §6).
inline float DialValueAfterDrag(float value, const DialRange& rawRange, float dragDeltaY) {
    const DialRange range = ResolvedDialRange(rawRange);
    if (!(range.pixelsForFullSweep > kMinimumWidgetRange)) return ClampDialValue(value, range);
    const float span = range.maximumValue - range.minimumValue;
    return ClampDialValue(value - (dragDeltaY / range.pixelsForFullSweep) * span, range);
}

// 0..1 along the range — what the drawn arc and the knob's pointer are placed from.
inline float DialNormalizedPosition(float value, const DialRange& rawRange) {
    const DialRange range = ResolvedDialRange(rawRange);
    return NormalizedPosition(value, range.minimumValue, range.maximumValue);
}

// Screen-space angle, in radians, of a 0..1 dial position. imgui's y axis points down, so a
// growing angle sweeps CLOCKWISE on screen: the default 135 deg start with a 270 deg sweep puts
// the minimum at the lower left and the maximum at the lower right.
inline float DialAngleRadians(float normalizedPosition, float sweepStartDegrees, float sweepDegrees) {
    constexpr float kDegreesToRadians = 3.14159265358979323846f / 180.0f;
    return (sweepStartDegrees + ClampToRange(normalizedPosition, 0.0f, 1.0f) * sweepDegrees) * kDegreesToRadians;
}

// One frame of interaction, expressed so a synthetic mouse sequence can drive it headless.
//   bDragInProgress — the knob's hit-test is active this frame.
//   dragDeltaY      — vertical mouse movement since the previous frame, screen pixels (+y down).
//   bFieldActive    — the numeric field beside the knob is being dragged.
//   bFieldEdited    — the numeric field ALREADY wrote the value this frame; the step only folds
//                     it into the RT commit decision, it does not re-apply it.
struct DialPointerInput {
    bool  bDragInProgress = false;
    float dragDeltaY      = 0.0f;
    bool  bFieldActive    = false;
    bool  bFieldEdited    = false;
};

// Applies the drag and returns the live/expensive pair: with RT off, bValueChanged follows every
// frame of the drag while bCommitted arrives once, on release (UI_FRAMEWORK_SPEC §7).
inline WidgetChange StepDialInteraction(RealtimeToggle& realtimeToggle, float& value,
                                        const DialRange& range, const DialPointerInput& input) {
    bool bValueMoved = input.bFieldEdited;
    if (input.bDragInProgress) {
        const float movedValue = DialValueAfterDrag(value, range, input.dragDeltaY);
        if (movedValue != value) { value = movedValue; bValueMoved = true; }
    }
    return realtimeToggle.Update(input.bDragInProgress || input.bFieldActive, bValueMoved);
}

// Draws the label, the knob (track arc + filled value arc + pointer), the numeric field and the
// RT button, and runs the interaction above.
WidgetChange DrawLabelledDial(const char* label, float& value, const DialRange& range,
                              RealtimeToggle& realtimeToggle, const WidgetStyle& style = WidgetStyle(),
                              const char* valueFormat = "%.3f");

// STEP236 — the single-line variant a caller composing its OWN row (a section-header strip, for
// instance) reaches for instead of the 2-line label+field/RT stack above: no label line — `label`
// only scopes the ImGui ID and stands in as a hover tooltip on the knob, mirroring the already-
// shipped IsItemHovered()/SetTooltip pattern at MarkersTab_ManualLayerRowBody_UI.cpp and
// DrawSliderScalarCompact's own STEP134 precedent. Composition: PushID(label) -> knob (diameter =
// the row's own frame height, unless style.dialRadius overrides it) -> SameLine -> fixed-width
// DragFloat -> optional RT button. Shares DrawLabelledDial's arc/pointer draw code and interaction
// step untouched — only the layout differs.
// `bShowRealtimeToggle` (default true, byte-identical to before this parameter existed): a caller
// whose field never triggers anything beyond a cheap preview repaint can pass `false` to drop the
// RT button entirely — `realtimeToggle` still governs the underlying commit timing (unchanged), the
// caller simply never draws the control that would let a user flip it off.
WidgetChange DrawDialCompact(const char* label, float& value, const DialRange& range,
                             RealtimeToggle& realtimeToggle, float fieldWidthPixels,
                             const WidgetStyle& style = WidgetStyle(), const char* valueFormat = "%.2f",
                             bool bShowRealtimeToggle = true);

} // namespace Ui
} // namespace SanmapGen
