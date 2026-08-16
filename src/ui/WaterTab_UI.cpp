// WaterTab_UI.cpp — the imgui composition of the Water tab. Layer: UI.
// Two sections — "Water Levels" (the level band, the deep-water window, the preview ramp) and
// "Shore & Wind" (the seven wave settings) — plus the wave-generator blueprint. Every control is
// a shared-library widget; the only raw imgui here is the label vocabulary.
#include "WaterTab_UI.h"
#include "Checkbox_UI.h"
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

// The blueprint path and the ramp name are long, so the field gets the wider cap the shared rules
// already fence (TextInput_UI.h) rather than the 64-character default meant for names.
TextInputRules WaterPathTextRules() {
    TextInputRules rules;
    rules.maximumLength = 200;
    return rules;
}

void DrawWaterLevels(Params::MapRecipe& recipe, WaterTabState& state,
                     Pipeline::PreviewDriver* previewDriver) {
    Params::Water& water = recipe.water;
    NotifyChange(DrawCheckbox("Water Enabled", water.bEnabled).bCommitted, previewDriver);

    ApplyTerrainHeightBandToWaterLevel(recipe.geometry, state);
    if (!state.waterLevelToggle.IsCommitDeferred() && !state.deepWaterDepthToggle.IsCommitDeferred())
        LoadWaterTabValues(water, state);

    WidgetChange change = DrawRangeSlider("Water Level (game units)", state.waterLevelValues,
                                          WaterLevelBounds(state), state.waterLevelToggle,
                                          WidgetStyle(), "%.2f");
    if (change.bValueChanged) StoreWaterLevelValues(state, water);
    NotifyChange(change.bCommitted, previewDriver);

    change = DrawRangeSlider("Deep Water Depth Window (game units)", state.deepWaterDepthValues,
                             state.deepWaterDepthBounds, state.deepWaterDepthToggle,
                             WidgetStyle(), "%.2f");
    if (change.bValueChanged) StoreWaterTabValues(state, water);
    NotifyChange(change.bCommitted, previewDriver);
}

// SCOPE NOTE 3: the ramp belongs to the preview composite, so the tab edits the one it is handed
// and says so plainly when it is handed none.
void DrawWaterGradient(WaterTabState& state, Params::GradientRamp* waterGradientRamp,
                       Pipeline::PreviewDriver* previewDriver) {
    if (waterGradientRamp == nullptr) {
        ImGui::TextUnformatted("No preview composite bound - the water ramp is not editable here.");
        return;
    }
    if (DrawGradientEditor("Water Gradient", *waterGradientRamp, state.waterGradientEditor))
        NotifyChange(true, previewDriver);
}

// The seven shore/wind rows, driven straight off the shared table so the section's order and its
// limits have exactly one statement (WaterTab_UI.h).
void DrawShoreAndWind(WaterTabState& state, Pipeline::PreviewDriver* previewDriver) {
    for (int controlIndex = 0; controlIndex < kWaterShoreWindControlCount; ++controlIndex) {
        const WaterShoreWindControl& control = waterShoreWindControls[controlIndex];
        const WidgetChange change = DrawSliderScalar(control.label, state.shoreWind.*control.value,
                                                     control.range, state.shoreWindToggles[controlIndex],
                                                     WidgetStyle(), "%.3f");
        NotifyChange(change.bCommitted, previewDriver);
    }
    NotifyChange(DrawTextInput("Wave Blueprint", state.waveGeneratorBlueprint, WaterPathTextRules(),
                               WidgetStyle(), "blueprint path").bCommitted, previewDriver);
}

} // namespace

void DrawWaterTab(Params::MapRecipe& recipe, WaterTabState& state,
                  Pipeline::PreviewDriver* previewDriver, Params::GradientRamp* waterGradientRamp) {
    ImGui::PushID("waterTab");
    if (DrawSectionBegin("Water Levels", state.waterLevelSection)) {
        DrawWaterLevels(recipe, state, previewDriver);
        DrawWaterGradient(state, waterGradientRamp, previewDriver);
        DrawSectionEnd();
    }
    if (DrawSectionBegin("Shore & Wind", state.shoreWindSection)) {
        DrawShoreAndWind(state, previewDriver);
        DrawSectionEnd();
    }
    ImGui::Separator();
    ImGui::TextUnformatted("Depth window shades the preview only; the level itself gates placement.");
    ImGui::PopID();
}

} // namespace Ui
} // namespace SanmapGen
