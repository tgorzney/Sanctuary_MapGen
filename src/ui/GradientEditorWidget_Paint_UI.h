// GradientEditorWidget_Paint_UI.h — the gradient editor's ImDrawList painting, split out of the
// interaction TU to keep both inside the ARCH §1.5 ceilings. Internal to the widget: only
// GradientEditorWidget_*_UI.cpp include it, which is why imgui may appear here.
// Painting only — it never mutates the ramp.
#pragma once
#include <vector>
#include "GradientEditorWidget_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {

// One linear-RGBA LUT entry (or stop color) as a packed imgui color.
ImU32 GradientColorFromLookupEntry(const float* entry);

// The ramp strip, painted from a Ui::BakeGradientLut table (M4-2) as one multi-color quad per
// adjacent sample pair — no second interpolator that could drift from the baked truth.
void PaintGradientStrip(ImDrawList* drawList, const std::vector<float>& lookupTable,
                        int sampleCount, const GradientEditorState& state, const ImVec2& origin,
                        float width);

// One handle per stop, at `origin.x + width * location`; the selected stop gets the bright ring.
void PaintGradientStopHandles(ImDrawList* drawList, const Params::GradientRamp& ramp,
                              const GradientEditorState& state, const ImVec2& origin, float width);

} // namespace Ui
} // namespace SanmapGen
