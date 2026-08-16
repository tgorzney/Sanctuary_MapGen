// LayerEditor_Advanced_UI.cpp — the "Advanced (constants)" sub-section. Layer: UI.
// TAB_REBUILD_PLAN "§ Layer Editor": the sim numbers v1 hardcoded and never showed anyone —
// BaseErosionRate, BaseDepositionRate, MeanderStrength, DivergenceThreshold, ThermalIterations and
// ThermalRate — each promoted to a slider. Constitution §8 in its most literal form: "any variable
// can be changed — even constants — for interesting creative results."
//
// All six are ALREADY settable fields in the tree (`Proc::ErosionLayerSettings` and
// `Proc::ThermalConstants` — the M3 stage work-orders killed the shader's hardcoded 0.3 pair and
// the thermal "/2.0"); what was missing was the control. Nothing was promoted into a new type here.
//
// The section is seeded CLOSED (LayerEditor_Scalars_UI.cpp) — these are the numbers a designer
// reaches for last, and an open forty-row panel would bury the layer controls above it.
#include "LayerEditor_Draw_UI.h"
#include "LayerEditor_Erosion_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {

void DrawLayerEditorAdvancedSection(int stratumIndex, LayerEditorState& state,
                                    Pipeline::GenerationAssembler* generationAssembler,
                                    Pipeline::PreviewDriver* previewDriver) {
    if (!DrawSectionBegin("Advanced (constants)", state.advancedConstantsSection)) return;
    if (generationAssembler == nullptr || !IsLayerEditorStratumIndex(stratumIndex)) {
        ImGui::TextUnformatted("No pipeline bound - the sim constants have nothing to edit.");
        DrawSectionEnd();
        return;
    }
    ImGui::PushID("advancedConstants");
    Proc::ErosionLayerSettings& erosionSettings =
        generationAssembler->Erosion().LayerSettings(stratumIndex);
    DrawLayerEditorScalarRow(LayerEditorScalar::BaseErosionRate, erosionSettings.baseErosionRate,
                             state, previewDriver);
    DrawLayerEditorScalarRow(LayerEditorScalar::BaseDepositionRate,
                             erosionSettings.baseDepositionRate, state, previewDriver);
    DrawLayerEditorScalarRow(LayerEditorScalar::MeanderStrength, erosionSettings.meanderStrength,
                             state, previewDriver);
    DrawLayerEditorScalarRow(LayerEditorScalar::DivergenceThreshold,
                             erosionSettings.divergenceThreshold, state, previewDriver);

    // Thermal is a stage-wide relaxation, not a per-stratum record: one iteration count and one
    // rate for the whole map, so both are drawn from the thermal stage's own constants.
    ImGui::Separator();
    Proc::ThermalConstants& thermalConstants = generationAssembler->Thermal().Constants();
    DrawLayerEditorIntegerRow(LayerEditorScalar::ThermalIterationCount,
                              thermalConstants.iterationCount, state, previewDriver);
    DrawLayerEditorScalarRow(LayerEditorScalar::ThermalRelaxationRate,
                             thermalConstants.relaxationRate, state, previewDriver);
    ImGui::PopID();
    DrawSectionEnd();
}

} // namespace Ui
} // namespace SanmapGen
