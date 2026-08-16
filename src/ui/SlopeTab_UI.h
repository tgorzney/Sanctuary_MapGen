// SlopeTab_UI.h — the Slope overlay tab: show/hide, the 0..90 degree domain, and the slope ramp.
// Layer: UI. Accuracy class: Visual. TAB_REBUILD_PLAN "3 · Slope" (all -> PreviewRender).
//
// THE UNIT. The Mask stage bakes slope as GRADIENT MAGNITUDE (rise/run), which is the pinned unit
// (MASKING_SPEC 1.8, PreviewComposite_Settings_UI.h). Designers think in DEGREES, and the plan
// states the domain as 0-90 degrees — so the tab edits degrees and converts once, on the way into
// the layer's domain. It converts a DISPLAY BOUND, not a field: nothing here re-derives slope, so
// the shadow-sim stays dead (ARCH §3.2).
//
// The composite's settings are presentation, not recipe (PreviewComposite_Settings_UI.h), so an
// edit here trips no stage hash and `PreviewDriver` derives bNeedsPreviewRender on its own.
#pragma once
#include "SliderScalar_UI.h"
#include "TerrainOverlayTab_UI.h"
#include "GradientEditorWidget_UI.h"
#include <cmath>

namespace SanmapGen {
namespace Pipeline { class PreviewDriver; }
namespace Ui {

// The steepest angle the tab will offer. 90 degrees is a vertical wall, whose tangent is
// unbounded, so the row stops just short of it — the same guard the thermal kernel applies to a
// talus angle (Constitution §6).
inline constexpr float kSlopeDegreeCeiling = 89.0f;

// degrees -> gradient magnitude (the baked field's unit). Clamped into 0..ceiling first, so no
// caller can drive tan() through its asymptote.
inline float SlopeGradientFromDegrees(float degrees) {
    const float clampedDegrees = ClampToRange(degrees, 0.0f, kSlopeDegreeCeiling);
    return std::tan(clampedDegrees * 0.01745329252f);
}

// ...and back, for showing a domain a recipe already carries.
inline float SlopeDegreesFromGradient(float gradient) {
    const float safeGradient = gradient > 0.0f ? gradient : 0.0f;
    return ClampToRange(std::atan(safeGradient) * 57.2957795131f, 0.0f, kSlopeDegreeCeiling);
}

// Caller-owned tab state. The two degree values are MIRRORS of the layer's gradient domain, never
// a second home for it.
struct SlopeTabState {
    ScalarSliderRange slopeDegreeRange{ 0.0f, kSlopeDegreeCeiling, 0.0f };
    RealtimeToggle    minimumDegreesToggle;
    RealtimeToggle    maximumDegreesToggle;
    GradientEditorState gradientEditor;
    float minimumDegrees = 0.0f;
    float maximumDegrees = 45.0f;
};

// layer domain -> the degree mirrors.
inline void LoadSlopeTabValues(const PreviewFieldLayer& layer, SlopeTabState& state) {
    state.minimumDegrees = SlopeDegreesFromGradient(layer.domainMinimum);
    state.maximumDegrees = SlopeDegreesFromGradient(layer.domainMaximum);
}

// the degree mirrors -> layer domain, in the baked field's unit. Keeps the pair ordered (an
// inverted domain would paint the ramp backwards) and reports whether the layer actually moved.
inline bool StoreSlopeTabValues(const SlopeTabState& state, PreviewFieldLayer& layer) {
    float lowDegrees  = state.minimumDegrees;
    float highDegrees = state.maximumDegrees;
    if (highDegrees < lowDegrees) { const float swap = lowDegrees; lowDegrees = highDegrees; highDegrees = swap; }
    const float domainMinimum = SlopeGradientFromDegrees(lowDegrees);
    const float domainMaximum = SlopeGradientFromDegrees(highDegrees);
    const bool bMoved = domainMinimum != layer.domainMinimum || domainMaximum != layer.domainMaximum;
    layer.domainMinimum = domainMinimum;
    layer.domainMaximum = domainMaximum;
    return bMoved;
}

// Draws the tab. `previewDriver` may be null; a composite carrying no Slope layer says so rather
// than editing a layer of some other kind.
void DrawSlopeTab(PreviewCompositeSettings& compositeSettings, SlopeTabState& state,
                  Pipeline::PreviewDriver* previewDriver);

} // namespace Ui
} // namespace SanmapGen
