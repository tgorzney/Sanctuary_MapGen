// SliderScalar_UI.h — the single-handle bounded scalar slider, int and float. Layer: UI.
// UI_FRAMEWORK_SPEC "Universal widget library": the workhorse of the rebuild — every bounded
// number in the plan that is not a min/max pair (a RangeSlider) or a knob (a Dial) is this
// control. Drawn with ImDrawList::AddRectFilled behind one InvisibleButton per the bypass toolkit
// §1; drawing lives in the .cpp, everything here is pure and headless-testable.
//
// It carries an Ui::RealtimeToggle, per the v1 pattern the plan keeps: with RT off, scrubbing
// tracks the value every frame but the expensive recompute is paid once, on release
// (UI_FRAMEWORK_SPEC §7). The int twin is the SAME math on an integer lattice, so there is one
// drag implementation, not two.
//
// Owns no app state: the caller holds the number and one RealtimeToggle, and reads the
// WidgetChange back.
#pragma once
#include "RtToggleWidget_UI.h"
#include "WidgetHelpers_UI.h"

namespace SanmapGen {
namespace Ui {

// The scalar's limits and its snap increment — settings, not literals in the drag code
// (Constitution §8).
struct ScalarSliderRange {
    float minimumValue = 0.0f;
    float maximumValue = 1.0f;
    float increment    = 0.0f;     // <= 0: continuous, no snapping
};

// Repairs nonsense limits once (inverted limits swap), so every function below agrees.
inline ScalarSliderRange ResolvedScalarSliderRange(ScalarSliderRange range) {
    if (range.maximumValue < range.minimumValue) {
        const float swap = range.minimumValue;
        range.minimumValue = range.maximumValue;
        range.maximumValue = swap;
    }
    return range;
}

// Legal value: inside the limits and on the increment lattice measured from minimumValue. The
// final clamp is what stops a snap from stepping past the top of the range.
inline float ClampScalarSliderValue(float value, const ScalarSliderRange& rawRange) {
    const ScalarSliderRange range = ResolvedScalarSliderRange(rawRange);
    const float clamped = ClampToRange(value, range.minimumValue, range.maximumValue);
    return ClampToRange(QuantizeToIncrement(clamped, range.minimumValue, range.increment),
                        range.minimumValue, range.maximumValue);
}

// Left-edge offset, in pixels from the track origin, of the handle drawn for `value`. Shared by
// the draw path and the pixel->value mapping so a handle can never render off its own hit-test.
inline float ScalarSliderHandleOffset(float value, const ScalarSliderRange& rawRange,
                                      float trackWidthPixels, float handleWidthPixels) {
    const ScalarSliderRange range = ResolvedScalarSliderRange(rawRange);
    const float usableWidth = trackWidthPixels - handleWidthPixels;
    if (!(usableWidth > 0.0f)) return 0.0f;
    return NormalizedPosition(value, range.minimumValue, range.maximumValue) * usableWidth;
}

// One frame of pointer interaction, in VALUE space so it is drivable headless with a synthetic
// mouse sequence.
//   bHandleGrabbed      — the track's InvisibleButton is active this frame.
//   pointerValue        — the track value under the cursor.
//   bNumericFieldActive — the numeric field beside the track is being dragged this frame.
//   bNumericFieldEdited — the field ALREADY wrote the value this frame; the step only folds it
//                         into the RT commit decision, it does not re-apply it.
struct ScalarSliderPointerInput {
    bool  bHandleGrabbed      = false;
    float pointerValue        = 0.0f;
    bool  bNumericFieldActive = false;
    bool  bNumericFieldEdited = false;
};

// Applies one frame of interaction and returns the live/expensive pair: with RT off, bValueChanged
// follows every frame of the drag while bCommitted arrives once, on release.
inline WidgetChange StepScalarSliderInteraction(RealtimeToggle& realtimeToggle, float& value,
                                                const ScalarSliderRange& range,
                                                const ScalarSliderPointerInput& input) {
    bool bValueMoved = input.bNumericFieldEdited;
    if (input.bHandleGrabbed) {
        const float movedValue = ClampScalarSliderValue(input.pointerValue, range);
        if (movedValue != value) { value = movedValue; bValueMoved = true; }
    }
    return realtimeToggle.Update(input.bHandleGrabbed || input.bNumericFieldActive, bValueMoved);
}

// ---- the integer twin: the same range on a whole-number lattice ------------------------------
// An increment below one is raised to one, so an integer slider can never land between values.
inline ScalarSliderRange IntegerScalarSliderRange(int minimumValue, int maximumValue, int increment = 1) {
    ScalarSliderRange range;
    range.minimumValue = static_cast<float>(minimumValue);
    range.maximumValue = static_cast<float>(maximumValue);
    range.increment    = static_cast<float>(increment >= 1 ? increment : 1);
    return range;
}

// Legal integer: the float clamp above, rounded back to the nearest whole number.
inline int ClampScalarSliderInteger(int value, const ScalarSliderRange& range) {
    const float clamped = ClampScalarSliderValue(static_cast<float>(value), range);
    return static_cast<int>(clamped >= 0.0f ? clamped + 0.5f : clamped - 0.5f);
}

// One frame of interaction on an integer. Runs the float step, then rounds — so the drag, the
// snapping and the RT commit are the SAME code the float slider uses.
inline WidgetChange StepScalarSliderIntegerInteraction(RealtimeToggle& realtimeToggle, int& value,
                                                       const ScalarSliderRange& range,
                                                       const ScalarSliderPointerInput& input) {
    float scalarValue = static_cast<float>(ClampScalarSliderInteger(value, range));
    const WidgetChange change = StepScalarSliderInteraction(realtimeToggle, scalarValue, range, input);
    value = ClampScalarSliderInteger(static_cast<int>(scalarValue >= 0.0f ? scalarValue + 0.5f
                                                                         : scalarValue - 0.5f), range);
    return change;
}

// Draws label + track + handle + the numeric field + the RT button, and runs the interaction
// above. `valueFormat` is the printf format of the numeric field.
WidgetChange DrawSliderScalar(const char* label, float& value, const ScalarSliderRange& range,
                              RealtimeToggle& realtimeToggle, const WidgetStyle& style = WidgetStyle(),
                              const char* valueFormat = "%.3f");
WidgetChange DrawSliderScalarInteger(const char* label, int& value, const ScalarSliderRange& range,
                                     RealtimeToggle& realtimeToggle, const WidgetStyle& style = WidgetStyle(),
                                     const char* valueFormat = "%d");

} // namespace Ui
} // namespace SanmapGen
