// SymmetryTab_UI.cpp — the imgui composition of the Symmetry tab. Layer: UI.
// Two sections: the global axis row (four independent tick boxes over the real bit mask, shared
// with the per-rule symmetry override via `DrawIndependentSymmetryAxes`) and Detection (the two
// settings this work-order promoted into Symmetry_PARAMS.h). Every control is a shared widget; the
// only raw imgui here is the label vocabulary.
#include "SymmetryTab_UI.h"
#include "PlacementRuleSections_UI.h"
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

void DrawAxisRow(Params::MapRecipe& recipe, Pipeline::PreviewDriver* previewDriver) {
    DrawIndependentSymmetryAxes(recipe.globalSymmetryMask, previewDriver);
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
        DrawAxisRow(recipe, previewDriver);
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
