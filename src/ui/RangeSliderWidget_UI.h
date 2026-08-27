// RangeSliderWidget_UI.h — the dual-handle min/max float slider. Layer: UI.
// UI_FRAMEWORK_SPEC bypass toolkit §1: drawn with ImDrawList::AddRectFilled and hit-tested with
// two InvisibleButtons — one per handle — instead of an imgui built-in (faster, fully styleable).
// The UIHelpers.h precedent, generalized into the shared library. Drawing lives in the .cpp;
// everything here is pure and headless-testable, including the drag state machine.
//
// The control owns no app state: the caller holds the value pair and one Ui::RealtimeToggle, and
// reads the WidgetChange back.
#pragma once
#include "RtToggleWidget_UI.h"
#include "WidgetHelpers_UI.h"

namespace SanmapGen {
namespace Ui {

// Track limits plus the gap the two handles keep from each other (Constitution §8: the
// separation is a setting, not a literal buried in the drag code).
struct RangeSliderBounds {
    float lowerLimit        = 0.0f;
    float upperLimit        = 1.0f;
    float minimumSeparation = 0.001f;
};

// The pair being edited. The caller owns it; the widget writes it in place.
struct RangeSliderValues {
    float minimumValue = 0.0f;
    float maximumValue = 1.0f;
};

enum class RangeSliderHandle { None, Minimum, Maximum };

// Repairs nonsense limits once, so every function below shares one interpretation: inverted
// limits swap, a negative separation becomes 0, and a separation wider than the track collapses
// to the track span (Constitution §6 — validate rather than clamp against garbage).
inline RangeSliderBounds ResolvedRangeSliderBounds(RangeSliderBounds bounds) {
    if (bounds.upperLimit < bounds.lowerLimit) {
        const float swap = bounds.lowerLimit;
        bounds.lowerLimit = bounds.upperLimit;
        bounds.upperLimit = swap;
    }
    if (!(bounds.minimumSeparation > 0.0f)) bounds.minimumSeparation = 0.0f;
    const float span = bounds.upperLimit - bounds.lowerLimit;
    if (bounds.minimumSeparation > span) bounds.minimumSeparation = span;
    return bounds;
}

// Forces a value pair legal: inside the limits, in order, at least minimumSeparation apart. Run
// on entry to every edit, so a pair loaded from a recipe that predates the current limits is
// corrected instead of drawn off the track.
inline RangeSliderValues ClampRangeSliderValues(RangeSliderValues values, const RangeSliderBounds& rawBounds) {
    const RangeSliderBounds bounds = ResolvedRangeSliderBounds(rawBounds);
    float minimumValue = ClampToRange(values.minimumValue, bounds.lowerLimit, bounds.upperLimit);
    float maximumValue = ClampToRange(values.maximumValue, bounds.lowerLimit, bounds.upperLimit);
    if (maximumValue < minimumValue) { const float swap = minimumValue; minimumValue = maximumValue; maximumValue = swap; }
    if (maximumValue - minimumValue < bounds.minimumSeparation) {
        maximumValue = minimumValue + bounds.minimumSeparation;
        if (maximumValue > bounds.upperLimit) {
            maximumValue = bounds.upperLimit;
            minimumValue = bounds.upperLimit - bounds.minimumSeparation;
        }
    }
    values.minimumValue = minimumValue;
    values.maximumValue = maximumValue;
    return values;
}

// Drags ONE handle to targetValue with the other held still: the moving handle stops at the
// separation gap rather than shoving its partner (the UIHelpers.h behavior). `None` re-clamps.
inline RangeSliderValues MoveRangeSliderHandle(RangeSliderValues values, const RangeSliderBounds& rawBounds,
                                               RangeSliderHandle handle, float targetValue) {
    const RangeSliderBounds bounds = ResolvedRangeSliderBounds(rawBounds);
    RangeSliderValues result = ClampRangeSliderValues(values, bounds);
    if (handle == RangeSliderHandle::Minimum) {
        const float highestLegalMinimum = result.maximumValue - bounds.minimumSeparation;
        result.minimumValue = ClampToRange(targetValue, bounds.lowerLimit,
            highestLegalMinimum > bounds.lowerLimit ? highestLegalMinimum : bounds.lowerLimit);
    } else if (handle == RangeSliderHandle::Maximum) {
        const float lowestLegalMaximum = result.minimumValue + bounds.minimumSeparation;
        result.maximumValue = ClampToRange(targetValue,
            lowestLegalMaximum < bounds.upperLimit ? lowestLegalMaximum : bounds.upperLimit, bounds.upperLimit);
    }
    return result;
}

// Left-edge offset, in pixels from the track origin, of the handle drawn for `value`. Shared by
// the draw path and the pixel->value mapping so a handle can never render off its own hit-test.
inline float RangeSliderHandleOffset(float value, const RangeSliderBounds& bounds,
                                     float trackWidthPixels, float handleWidthPixels) {
    const RangeSliderBounds resolved = ResolvedRangeSliderBounds(bounds);
    const float usableWidth = trackWidthPixels - handleWidthPixels;
    if (!(usableWidth > 0.0f)) return 0.0f;
    return NormalizedPosition(value, resolved.lowerLimit, resolved.upperLimit) * usableWidth;
}

// One frame of pointer interaction, in VALUE space so it is drivable headless with a synthetic
// mouse sequence.
//   grabbedHandle       — the handle whose InvisibleButton imgui reports active (None = idle).
//   pointerValue        — the track value under the cursor.
//   bNumericFieldActive — a numeric field beside the track is being dragged this frame.
//   bNumericFieldEdited — a numeric field ALREADY wrote the value this frame; the step only has
//                         to fold it into the RT commit decision, not re-apply it.
struct RangeSliderPointerInput {
    RangeSliderHandle grabbedHandle       = RangeSliderHandle::None;
    float             pointerValue        = 0.0f;
    bool              bNumericFieldActive = false;
    bool              bNumericFieldEdited = false;
};

// Applies one frame of interaction and returns the live/expensive pair: with RT off,
// bValueChanged follows every frame of the drag while bCommitted arrives once, on the frame the
// drag ends (UI_FRAMEWORK_SPEC §7).
inline WidgetChange StepRangeSliderInteraction(RealtimeToggle& realtimeToggle, RangeSliderValues& values,
                                               const RangeSliderBounds& bounds,
                                               const RangeSliderPointerInput& input) {
    bool bValueMoved = input.bNumericFieldEdited;
    if (input.grabbedHandle != RangeSliderHandle::None) {
        const RangeSliderValues moved = MoveRangeSliderHandle(values, bounds, input.grabbedHandle, input.pointerValue);
        if (moved.minimumValue != values.minimumValue || moved.maximumValue != values.maximumValue) {
            values = moved;
            bValueMoved = true;
        }
    }
    const bool bEditInProgress = input.grabbedHandle != RangeSliderHandle::None || input.bNumericFieldActive;
    return realtimeToggle.Update(bEditInProgress, bValueMoved);
}

// Draws label + track + two handles + the two numeric fields + the RT button, and runs the
// interaction above. `valueFormat` is the printf format of the numeric fields.
WidgetChange DrawRangeSlider(const char* label, RangeSliderValues& values,
                             const RangeSliderBounds& bounds, RealtimeToggle& realtimeToggle,
                             const WidgetStyle& style = WidgetStyle(),
                             const char* valueFormat = "%.3f");

// The single-line variant a caller composing its OWN row reaches for instead (the
// DrawSliderScalarCompact precedent, SliderScalar_UI.h STEP134; human's own bug report — the
// three-row shape above cannot fit beside other controls, and the human explicitly asked for
// "the two values on left and right of slider so it can be single line"): no label line — a
// fixed-width numeric field for minimumValue, then the fixed-width track with both handles, then a
// fixed-width numeric field for maximumValue, all on one line, `label` used only to scope the
// ImGui ID and as a hover tooltip over the track (mirrors DrawSliderScalarCompact's own
// IsItemHovered()/SetTooltip pattern). DrawRangeSlider above is UNTOUCHED — every existing 3-line
// caller keeps its current shape; min-cannot-cross-max and the minimumSeparation gap (the
// caller-configurable "Min Delta") are the SAME `RangeSliderBounds`/interaction logic above, not
// reimplemented. `bShowRealtimeToggle` mirrors DrawSliderScalarCompact's own escape hatch: a caller
// whose field never triggers anything beyond a cheap preview repaint can drop the RT button
// entirely (the underlying commit timing is unaffected — only the control disappears).
WidgetChange DrawRangeSliderCompact(const char* label, RangeSliderValues& values,
                                    const RangeSliderBounds& bounds, RealtimeToggle& realtimeToggle,
                                    float trackWidthPixels, float fieldWidthPixels,
                                    const WidgetStyle& style = WidgetStyle(),
                                    const char* valueFormat = "%.3f", bool bShowRealtimeToggle = true);

} // namespace Ui
} // namespace SanmapGen
