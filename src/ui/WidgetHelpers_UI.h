// WidgetHelpers_UI.h — the shared, imgui-free core of the universal widget library.
// Layer: UI. UI_FRAMEWORK_SPEC "Universal widget library": one implementation, one look, DRY —
// tabs compose these controls, they never hand-roll imgui.
//
// THE SPLIT (binding on every widget in this library): everything here and in each widget's
// header is a PURE function or a plain settings/result struct, so all clamping and interaction
// logic is testable headless — no imgui frame, no window, no GL context. The `ImDrawList`
// drawing that consumes it lives in each widget's `.cpp`, the only place `imgui.h` is included.
// That is what lets the acceptance tests link without imgui at all.
//
// No control owns app state: no global, no function static, no retained PARAMS reference. The
// caller supplies the value and reads the result back (ARCH §3.2 — UI sets params and trips
// dirty flags; it never holds the model).
#pragma once
#include <cmath>

namespace SanmapGen {
namespace Ui {

// A packed 0xAABBGGRR color, byte-identical to imgui's IM_COL32/ImU32 but declared as a plain
// unsigned so the shared style carries no imgui dependency. kThemeColor is alpha 0 — which no
// visible color has — and means "resolve from the active imgui theme at draw time".
using PackedColor = unsigned int;
enum : PackedColor { kThemeColor = 0u };

// One style for every control (Constitution §8: sizes and colors are adjustable settings, not
// literals buried in draw code). A default-constructed WidgetStyle follows the imgui theme, so
// "styleable" costs the caller nothing until it wants to style something.
struct WidgetStyle {
    float trackHeight         = 0.0f;    // <= 0: imgui's frame height
    float handleWidth         = 10.0f;   // width of one range-slider handle, in pixels
    float cornerRounding      = -1.0f;   // <  0: imgui's style frame rounding
    float realtimeButtonWidth = 30.0f;
    float dialRadius          = 0.0f;    // <= 0: derived from imgui's frame height
    float dialThickness       = 4.0f;    // stroke width of the dial arc, in pixels
    float dialSweepStartDegrees = 135.0f; // where the dial's zero sits, clockwise from +x (screen)
    float dialSweepDegrees      = 270.0f; // arc the whole value range covers, clockwise
    PackedColor trackColor         = kThemeColor;
    PackedColor fillColor          = kThemeColor;
    PackedColor handleColor        = kThemeColor;
    PackedColor handleActiveColor  = kThemeColor;
    PackedColor realtimeActiveColor = kThemeColor;
};

// What one control did this frame — the whole return contract of the library.
//   bValueChanged — the LIVE edit: the caller's value has already moved this frame.
//   bCommitted    — the EXPENSIVE signal: the frame on which the caller trips bNeedsMapUpdate
//                   or bNeedsPreviewRender (PreviewDriver_PIPELINE). With the RT toggle OFF the
//                   two differ — the value tracks the drag every frame, the commit arrives once,
//                   on release (UI_FRAMEWORK_SPEC §7). WHICH flag a commit trips is the caller's
//                   decision, never the widget's: a widget that named a flag would be holding
//                   app state, and the tier is derived from the DAG, not from a per-widget list.
struct WidgetChange {
    bool bValueChanged = false;
    bool bCommitted    = false;
};

// Below this a range is treated as degenerate rather than divided by (Constitution §6).
inline constexpr float kMinimumWidgetRange = 1.0e-9f;

// Clamps into [minimumValue, maximumValue]; inverted limits are swapped and NaN answers the low
// limit, so no caller can drive a handle off the track with bad input.
inline float ClampToRange(float value, float minimumValue, float maximumValue) {
    if (maximumValue < minimumValue) { const float swap = minimumValue; minimumValue = maximumValue; maximumValue = swap; }
    if (!(value == value)) return minimumValue;                  // NaN
    return value < minimumValue ? minimumValue : (value > maximumValue ? maximumValue : value);
}

// value -> 0..1 along [minimumValue, maximumValue]. A degenerate range answers 0.
inline float NormalizedPosition(float value, float minimumValue, float maximumValue) {
    const float range = maximumValue - minimumValue;
    if (!(range > kMinimumWidgetRange)) return 0.0f;
    return ClampToRange((value - minimumValue) / range, 0.0f, 1.0f);
}

// 0..1 -> value along [minimumValue, maximumValue]. The inverse of NormalizedPosition.
inline float ValueAtNormalizedPosition(float normalizedPosition, float minimumValue, float maximumValue) {
    return minimumValue + ClampToRange(normalizedPosition, 0.0f, 1.0f) * (maximumValue - minimumValue);
}

// Snaps to the lattice originValue + n * increment (nearest, ties away from zero). A
// non-positive or non-finite increment means "continuous" and passes the value through.
inline float QuantizeToIncrement(float value, float originValue, float increment) {
    if (!(increment > kMinimumWidgetRange)) return value;
    if (!(value == value)) return originValue;                   // NaN
    return originValue + std::floor((value - originValue) / increment + 0.5f) * increment;
}

} // namespace Ui
} // namespace SanmapGen
