// WaterTab_UI.cpp — the imgui composition of the water tab. Layer: UI.
// Shared widgets only (LabelledDial, RangeSlider); no ImGui::SliderFloat/DragFloat/VSliderFloat.
#include "WaterTab_UI.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../pipeline/PreviewDriver_PIPELINE.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

// The ONE thing a tab does with a commit. WHICH tier it becomes is the driver's derivation from
// the stage parameter hashes, never this call site's decision.
void NotifyChange(bool bCommitted, Pipeline::PreviewDriver* previewDriver) {
    if (bCommitted && previewDriver != nullptr) previewDriver->NotifyParametersChanged();
}

} // namespace

void DrawWaterTab(Params::MapRecipe& recipe, WaterTabState& state,
                  Pipeline::PreviewDriver* previewDriver) {
    Params::Water& water = recipe.water;
    ImGui::PushID("waterTab");
    if (!state.deepWaterDepthToggle.IsCommitDeferred()) LoadWaterTabValues(water, state);

    bool bEnabled = water.bEnabled;
    if (ImGui::Checkbox("Water Enabled", &bEnabled)) {      // no library widget for a boolean
        water.bEnabled = bEnabled;
        NotifyChange(true, previewDriver);
    }
    NotifyChange(DrawLabelledDial("Water Level (game units)", water.waterLevelMaximum,
                                  state.waterLevelRange, state.waterLevelToggle,
                                  WidgetStyle(), "%.2f").bCommitted, previewDriver);

    const WidgetChange change = DrawRangeSlider("Deep Water Depth Window (game units)",
                                                state.deepWaterDepthValues,
                                                state.deepWaterDepthBounds,
                                                state.deepWaterDepthToggle, WidgetStyle(), "%.2f");
    if (change.bValueChanged) StoreWaterTabValues(state, water);
    NotifyChange(change.bCommitted, previewDriver);

    ImGui::Separator();
    ImGui::TextUnformatted("Depth window shades the preview only; the level itself gates placement.");
    ImGui::PopID();
}

} // namespace Ui
} // namespace SanmapGen
