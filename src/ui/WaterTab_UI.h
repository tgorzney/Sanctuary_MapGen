// WaterTab_UI.h — the water tab: surface level and the deep-water shading window.
// Layer: UI. Accuracy class: Visual. Edits exactly one recipe slice — `Params::Water`.
//
// This tab is the clearest demonstration of the two-tier dirty model being DERIVED rather than
// declared, because its four settings split across both tiers and the tab does not know it:
//   - `bEnabled` / `waterLevelMaximum` are consumed by the Placement stage's parameter hash (a
//     prop gated on water moves when the surface does), so a commit there derives a MAP UPDATE.
//   - `deepWaterDepthMinimum` / `deepWaterDepthMaximum` are read only by the preview composite,
//     so no stage claims them and a commit derives a PREVIEW RENDER — the cheap recolor.
// Both go through the identical call: Pipeline::PreviewDriver::NotifyParametersChanged().
#pragma once
#include "LabelledDialWidget_UI.h"
#include "RangeSliderWidget_UI.h"
#include "../params/Water_PARAMS.h"

namespace SanmapGen {
namespace Params { struct MapRecipe; }
namespace Pipeline { class PreviewDriver; }
namespace Ui {

struct WaterTabState {
    // Limits are settings, never literals at a use site (Constitution §8). The depth window is
    // expressed in game units, like the surface level it is measured down from.
    DialRange waterLevelRange{ 0.0f, 1024.0f, 0.0f, 400.0f };
    RangeSliderBounds deepWaterDepthBounds{ 0.0f, 256.0f, 0.01f };

    RealtimeToggle waterLevelToggle;
    RealtimeToggle deepWaterDepthToggle;
    RangeSliderValues deepWaterDepthValues;   // Water::deepWaterDepthMinimum/Maximum mirror
};

// water -> widget mirrors (the paired min/max the range slider edits).
inline void LoadWaterTabValues(const Params::Water& water, WaterTabState& state) {
    state.deepWaterDepthValues.minimumValue = water.deepWaterDepthMinimum;
    state.deepWaterDepthValues.maximumValue = water.deepWaterDepthMaximum;
}

// widget mirrors -> water. Reports whether the recipe actually moved.
inline bool StoreWaterTabValues(const WaterTabState& state, Params::Water& water) {
    const bool bMoved = state.deepWaterDepthValues.minimumValue != water.deepWaterDepthMinimum
                     || state.deepWaterDepthValues.maximumValue != water.deepWaterDepthMaximum;
    water.deepWaterDepthMinimum = state.deepWaterDepthValues.minimumValue;
    water.deepWaterDepthMaximum = state.deepWaterDepthValues.maximumValue;
    return bMoved;
}

void DrawWaterTab(Params::MapRecipe& recipe, WaterTabState& state,
                  Pipeline::PreviewDriver* previewDriver);

} // namespace Ui
} // namespace SanmapGen
