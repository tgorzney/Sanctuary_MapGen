// LayerEditor_Erosion_UI.cpp — the Hydraulic Erosion, Precipitation and Deposition sub-panels.
// Layer: UI. They edit the erosion record of the STRATUM the selected layer points at, reached
// through PIPELINE (LayerEditor_Erosion_UI.h explains why, and what changes when the PARAMS home
// lands). The Advanced (constants) section is LayerEditor_Advanced_UI.cpp.
#include "LayerEditor_Draw_UI.h"
#include "LayerEditor_Erosion_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

void DrawHydraulicErosionSection(Proc::ErosionLayerSettings& erosionSettings, LayerEditorState& state,
                                 Pipeline::PreviewDriver* previewDriver) {
    if (!DrawSectionBegin("Hydraulic Erosion", state.hydraulicErosionSection)) return;
    DrawLayerEditorCheckboxRow("Enable", erosionSettings.bEnabled, previewDriver);
    DrawLayerEditorCheckboxRow("Erode Beneath", erosionSettings.bErodeBeneath, previewDriver);
    DrawLayerEditorIntegerRow(LayerEditorScalar::ErosionDropletCount, erosionSettings.dropletCount,
                              state, previewDriver);
    DrawLayerEditorIntegerRow(LayerEditorScalar::ErosionMaximumLifetime,
                              erosionSettings.maximumLifetime, state, previewDriver);
    DrawLayerEditorScalarRow(LayerEditorScalar::ErosionEvaporationRate,
                             erosionSettings.evaporationRate, state, previewDriver);
    DrawLayerEditorScalarRow(LayerEditorScalar::ErosionFluidViscosity,
                             erosionSettings.fluidViscosity, state, previewDriver);
    DrawLayerEditorScalarRow(LayerEditorScalar::ErosionCarryingCapacityScale,
                             erosionSettings.carryingCapacityScale, state, previewDriver);
    DrawLayerEditorScalarRow(LayerEditorScalar::ErosionGravity, erosionSettings.gravity,
                             state, previewDriver);
    // Base Absorption is per-MATERIAL in v2 and lives in Soil Physics; a copy here would be a
    // rival control over one field (LayerEditor_UI.h SCOPE NOTE 3).
    DrawSectionEnd();
}

void DrawPrecipitationSection(Proc::ErosionLayerSettings& erosionSettings, LayerEditorState& state,
                              Pipeline::PreviewDriver* previewDriver) {
    if (!DrawSectionBegin("Precipitation", state.precipitationSection)) return;
    DrawLayerEditorCheckboxRow("Rain Noise", erosionSettings.bUseRainNoise, previewDriver);
    DrawLayerEditorScalarRow(LayerEditorScalar::RainNoiseFrequency,
                             erosionSettings.rainNoiseFrequency, state, previewDriver);
    DrawLayerEditorIntegerRow(LayerEditorScalar::RainNoiseOctaveCount,
                              erosionSettings.rainNoiseOctaves, state, previewDriver);
    DrawLayerEditorCheckboxRow("Orographic", erosionSettings.bUseOrographicRain, previewDriver);
    DrawLayerEditorScalarRow(LayerEditorScalar::RainWindAngleDegrees,
                             erosionSettings.windAngleDegrees, state, previewDriver);
    DrawSectionEnd();
}

// The spawn band is a min/max pair stored as two loose floats, so the range slider edits a mirror
// that is reloaded only while the drag is not deferring a commit.
void DrawDepositionSection(Proc::ErosionLayerSettings& erosionSettings, LayerEditorState& state,
                           Pipeline::PreviewDriver* previewDriver) {
    if (!DrawSectionBegin("Deposition", state.depositionSection)) return;
    DrawLayerEditorCheckboxRow("Enable", erosionSettings.bDepositionMode, previewDriver);
    DrawLayerEditorScalarRow(LayerEditorScalar::DepositionInitialSedimentLoad,
                             erosionSettings.initialSedimentLoad, state, previewDriver);
    if (!state.spawnHeightToggle.IsCommitDeferred())
        LoadDepositionSpawnBand(erosionSettings, state.spawnHeightValues);
    const WidgetChange change = DrawRangeSlider("Spawn Height", state.spawnHeightValues,
                                                state.spawnHeightBounds, state.spawnHeightToggle);
    if (change.bValueChanged)
        StoreDepositionSpawnBand(state.spawnHeightValues, state.spawnHeightBounds, erosionSettings);
    NotifyLayerEditorChange(change.bCommitted, previewDriver);
    DrawSectionEnd();
}

} // namespace

void DrawLayerEditorErosionSections(int stratumIndex, LayerEditorState& state,
                                    Pipeline::GenerationAssembler* generationAssembler,
                                    Pipeline::PreviewDriver* previewDriver) {
    if (generationAssembler == nullptr || !IsLayerEditorStratumIndex(stratumIndex)) {
        ImGui::TextUnformatted("No pipeline bound - erosion has nothing to edit.");
        return;
    }
    ImGui::PushID("erosionSections");
    Proc::ErosionLayerSettings& erosionSettings =
        generationAssembler->Erosion().LayerSettings(stratumIndex);
    DrawHydraulicErosionSection(erosionSettings, state, previewDriver);
    DrawPrecipitationSection(erosionSettings, state, previewDriver);
    DrawDepositionSection(erosionSettings, state, previewDriver);
    DrawLayerEditorAdvancedSection(stratumIndex, state, generationAssembler, previewDriver);
    ImGui::PopID();
}

} // namespace Ui
} // namespace SanmapGen
