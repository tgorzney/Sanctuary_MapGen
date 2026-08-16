// LayerEditor_Soil_UI.cpp — the Soil Physics sub-panel. Layer: UI.
// TAB_REBUILD_PLAN "§ Layer Editor / Soil Physics": a presets menu plus Hardness, Friction,
// Cohesion, Capacity Multiplier and Absorption Rate, written onto the STRATUM the selected layer
// points at — so two layers sharing a stratum index share one soil, which is the palette model
// LAYER_SYSTEM_SPEC pins ("a material layer points at a stratum index and inherits all of it").
#include "LayerEditor_Draw_UI.h"
#include "LayerEditor_Erosion_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

// The preset row. A pick fills the five numbers below it and is then forgotten — the menu shows
// no "current preset", because the sliders under it are free to leave any preset behind.
void DrawSoilPresetRow(Proc::MaterialPhysics& material, LayerEditorState& state,
                       Pipeline::PreviewDriver* previewDriver) {
    ComboOptions options;
    options.labels     = soilPresetLabels;
    options.count      = kSoilPresetCount;
    options.emptyLabel = "Presets...";
    const WidgetChange change = DrawCombo("Preset", state.soilPresetIndex, options);
    if (!change.bValueChanged || !IsSoilPresetIndex(state.soilPresetIndex)) return;
    const bool bMoved = ApplySoilPresetToMaterial(
        static_cast<SoilPreset>(state.soilPresetIndex), material);
    NotifyLayerEditorChange(bMoved, previewDriver);
}

} // namespace

void DrawLayerEditorSoilSection(int stratumIndex, LayerEditorState& state,
                                Pipeline::GenerationAssembler* generationAssembler,
                                Pipeline::PreviewDriver* previewDriver) {
    if (!DrawSectionBegin("Soil Physics", state.soilPhysicsSection)) return;
    if (generationAssembler == nullptr) {
        ImGui::TextUnformatted("No pipeline bound - soil physics has nothing to edit.");
        DrawSectionEnd();
        return;
    }
    if (!IsLayerEditorStratumIndex(stratumIndex)) {
        ImGui::Text("Stratum %d is outside the %d-slot palette.", stratumIndex,
                    kLayerEditorStratumCount);
        DrawSectionEnd();
        return;
    }
    ImGui::PushID("soilPhysics");
    Proc::MaterialPhysics& material = generationAssembler->Erosion().Material(stratumIndex);
    ImGui::Text("Stratum %d", stratumIndex);
    DrawSoilPresetRow(material, state, previewDriver);
    DrawLayerEditorScalarRow(LayerEditorScalar::SoilHardness, material.hardness, state, previewDriver);
    DrawLayerEditorScalarRow(LayerEditorScalar::SoilFriction, material.friction, state, previewDriver);
    DrawLayerEditorScalarRow(LayerEditorScalar::SoilCohesion, material.cohesion, state, previewDriver);
    DrawLayerEditorScalarRow(LayerEditorScalar::SoilCapacityMultiplier, material.capacityMultiplier,
                             state, previewDriver);
    DrawLayerEditorScalarRow(LayerEditorScalar::SoilAbsorptionRate, material.absorptionRate,
                             state, previewDriver);
    DrawLayerEditorCheckboxRow("Erodable", material.bErodable, previewDriver);
    ImGui::PopID();
    DrawSectionEnd();
}

} // namespace Ui
} // namespace SanmapGen
