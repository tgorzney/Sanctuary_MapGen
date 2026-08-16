// SliderScalar_Track_UI.h — the scalar slider's track geometry + ImDrawList painting, split out of
// the two interaction TUs to keep every file inside the ARCH §1.5 ceilings. Internal to the widget:
// only SliderScalar_*_UI.cpp include it, which is why imgui may appear here.
// Geometry and painting only — nothing here mutates the caller's value.
#pragma once
#include "SliderScalar_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {

struct ScalarSliderTrackGeometry {
    ImVec2 origin      = ImVec2(0.0f, 0.0f);
    float  width       = 0.0f;
    float  height      = 0.0f;
    float  handleWidth = 1.0f;
    float  usableWidth = 0.0f;   // the span the handle's LEFT edge may travel over
};

// Reserves the track row and hit-tests it with ONE InvisibleButton spanning the whole track, so
// clicking anywhere on it seizes the handle and jumps it under the cursor. The caller reads
// ImGui::IsItemActive() straight after.
ScalarSliderTrackGeometry ReserveScalarSliderTrack(const WidgetStyle& style);

// The track value under a screen-space cursor x, measured against the handle CENTER so grabbing
// the handle does not shift it by half its width.
float ScalarSliderPointerValueAt(const ScalarSliderTrackGeometry& geometry,
                                 const ScalarSliderRange& range, float cursorX);

// Groove + fill + handle, all placed from the ONE offset mapping in SliderScalar_UI.h, so the
// handle can never render off its own hit-test.
void PaintScalarSliderTrack(const ScalarSliderTrackGeometry& geometry, float value,
                            const ScalarSliderRange& range, const WidgetStyle& style,
                            bool bHandleGrabbed);

// The width left for the numeric field once the RT button has taken its share of the row.
float ScalarSliderNumericFieldWidth(const WidgetStyle& style);

} // namespace Ui
} // namespace SanmapGen
