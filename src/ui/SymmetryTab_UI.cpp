// SymmetryTab_UI.cpp — the imgui composition of the Symmetry tab. Layer: UI.
// Two sections: the global axis row (the batch-A exclusive checkbox group over the presentation
// word) and Detection (the two settings this work-order promoted into Symmetry_PARAMS.h).
// Every control is a shared widget; the only raw imgui here is the label vocabulary.
#include "SymmetryTab_UI.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../pipeline/PreviewDriver_PIPELINE.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

// WHICH tier a commit becomes is the driver's derivation from the stage hashes, never this call
// site's decision.
void NotifyChange(bool bCommitted, Pipeline::PreviewDriver* previewDriver) {
    if (bCommitted && previewDriver != nullptr) previewDriver->NotifyParametersChanged();
}

void DrawAxisRow(Params::MapRecipe& recipe, SymmetryTabState& state,
                 Pipeline::PreviewDriver* previewDriver) {
    LoadSymmetryTabValues(recipe.globalSymmetryMask, state);
    // bAllowNone: "no symmetry" is a legal recipe, so clicking the active option clears it.
    const WidgetChange change = DrawExclusiveCheckboxRow(
        "Axis", state.axisOptionBits, symmetryAxisOptionLabels, kSymmetryAxisOptionCount, true);
    if (change.bValueChanged) StoreSymmetryTabValues(state, recipe.globalSymmetryMask);
    NotifyChange(change.bCommitted, previewDriver);
    if (SymmetryAxisOptionOfMask(recipe.globalSymmetryMask) < 0
        && recipe.globalSymmetryMask != Params::SymmetryAxis::None)
        ImGui::Text("Recipe mask 0x%X is a combination this row cannot show.",
                    static_cast<unsigned int>(recipe.globalSymmetryMask));
}

void DrawDetectionSettings(Params::SymmetryDetection& symmetryDetection, SymmetryTabState& state,
                           Pipeline::PreviewDriver* previewDriver) {
    NotifyChange(DrawSliderScalar("Detection Tolerance", symmetryDetection.detectionTolerance,
                                  state.detectionToleranceRange, state.detectionToleranceToggle,
                                  WidgetStyle(), "%.4f").bCommitted, previewDriver);
    NotifyChange(DrawCheckbox("Snap Imperfect Symmetry",
                              symmetryDetection.bSnapImperfectSymmetry).bCommitted, previewDriver);
}

} // namespace

void DrawSymmetryTab(Params::MapRecipe& recipe, Params::SymmetryDetection& symmetryDetection,
                     SymmetryTabState& state, Pipeline::PreviewDriver* previewDriver) {
    ImGui::PushID("symmetryTab");
    if (DrawSectionBegin("Global Symmetry", state.globalSymmetrySection)) {
        DrawAxisRow(recipe, state, previewDriver);
        DrawSectionEnd();
    }
    if (DrawSectionBegin("Detection", state.detectionSection)) {
        DrawDetectionSettings(symmetryDetection, state, previewDriver);
        DrawSectionEnd();
    }
    ImGui::PopID();
}

} // namespace Ui
} // namespace SanmapGen
