// PlacementRuleSections_UI.cpp — the imgui composition of the shared placement-rule blocks.
// Layer: UI. Shared widgets only: Checkbox / RangeSlider / SliderScalar / Section / IconGrid.
// No ImGui::SliderFloat / DragFloat / VSliderFloat in this file.
#include "PlacementRuleSections_UI.h"
#include "Checkbox_UI.h"
#include "../pipeline/PreviewDriver_PIPELINE.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {

void NotifyPlacementChange(bool bCommitted, Pipeline::PreviewDriver* previewDriver) {
    if (bCommitted && previewDriver != nullptr) previewDriver->NotifyParametersChanged();
}

// "Use Global Symmetry" plus the per-axis bits it hides.
void DrawPlacementSymmetryAxes(const char* label, bool& bSymmetryUseGlobal, int& symmetryMask,
                               Pipeline::PreviewDriver* previewDriver) {
    ImGui::PushID(label);
    NotifyPlacementChange(DrawCheckbox("Use Global Symmetry", bSymmetryUseGlobal).bCommitted,
                          previewDriver);
    if (bSymmetryUseGlobal) { ImGui::PopID(); return; }
    DrawIndependentSymmetryAxes(symmetryMask, previewDriver);
    ImGui::PopID();
}

// Four independent tick boxes over the real bit mask. The mask is REPAIRED on the way in, so a
// bit no v2 axis owns is dropped the moment the row is drawn rather than at the next click
// (Constitution §6) — for every caller, not just the per-rule override.
void DrawIndependentSymmetryAxes(int& symmetryMask, Pipeline::PreviewDriver* previewDriver) {
    const int repairedMask = ResolvedPlacementSymmetryMask(symmetryMask);
    if (repairedMask != symmetryMask) symmetryMask = repairedMask;
    for (int axisIndex = 0; axisIndex < kPlacementSymmetryAxisCount; ++axisIndex) {
        bool bAxisSet = IsPlacementSymmetryAxisSet(symmetryMask, axisIndex);
        const WidgetChange change = DrawCheckbox(placementSymmetryAxisLabels[axisIndex], bAxisSet);
        if (change.bValueChanged) symmetryMask = PlacementSymmetryMaskAfterToggle(symmetryMask, axisIndex);
        NotifyPlacementChange(change.bCommitted, previewDriver);
    }
}

// The instance transform: the scale and yaw BANDS the scatter samples inside, plus the two
// gameplay flags. Both bands are range sliders because both are min/max pairs.
void DrawPlacementTransformSection(Params::ScatterTransform& transform, PlacementTransformState& state,
                                   Pipeline::PreviewDriver* previewDriver) {
    if (!DrawSectionBegin("Instance Transform", state.transformSection)) return;
    if (!state.scaleToggle.IsCommitDeferred() && !state.rotationToggle.IsCommitDeferred())
        LoadPlacementTransformValues(transform, state);
    WidgetChange change = DrawRangeSlider("Scale Range", state.scaleValues, state.scaleBounds,
                                          state.scaleToggle, WidgetStyle(), "%.2f");
    if (change.bValueChanged) StorePlacementTransformValues(state, transform);
    NotifyPlacementChange(change.bCommitted, previewDriver);
    change = DrawRangeSlider("Rotation Range (degrees)", state.rotationValues, state.rotationBounds,
                             state.rotationToggle, WidgetStyle(), "%.1f");
    if (change.bValueChanged) StorePlacementTransformValues(state, transform);
    NotifyPlacementChange(change.bCommitted, previewDriver);
    NotifyPlacementChange(DrawCheckbox("Align To Terrain Normal", transform.bAlignToTerrainNormal).bCommitted,
                          previewDriver);
    NotifyPlacementChange(DrawCheckbox("Collidable (gameplay relevant)", transform.bCollidable).bCommitted,
                          previewDriver);
    DrawSectionEnd();
}

// The biome gate and the map-edge fence. `maskStratumIndex` of -1 means "no gate", which is why
// its slider starts one step below zero rather than at it.
void DrawPlacementGateSection(int& maskStratumIndex, float& maskWeightMinimum, int& mapEdgePadding,
                              PlacementGateState& state, Pipeline::PreviewDriver* previewDriver) {
    if (!DrawSectionBegin("Biome & Edge Gate", state.gateSection)) return;
    NotifyPlacementChange(DrawSliderScalarInteger("Mask Stratum Index (-1 = no gate)", maskStratumIndex,
                                                  state.maskStratumIndexRange,
                                                  state.maskStratumIndexToggle).bCommitted, previewDriver);
    NotifyPlacementChange(DrawSliderScalar("Mask Weight Minimum", maskWeightMinimum,
                                           state.maskWeightRange, state.maskWeightToggle,
                                           WidgetStyle(), "%.3f").bCommitted, previewDriver);
    NotifyPlacementChange(DrawSliderScalarInteger("Map Edge Padding (cells)", mapEdgePadding,
                                                  state.edgePaddingRange,
                                                  state.edgePaddingToggle).bCommitted, previewDriver);
    DrawSectionEnd();
}

// The template (`tpId`) a rule spawns: typed here, browsed in the resident atlas beside it.
// ICON PICKER SCOPE (ARCH §8.4): the manifest belongs to the app shell (M5-7) and is passed in;
// it carries an `iconId` and nothing in the tree maps that id back to a game `tpId`, so the grid
// reports the SELECTION while the tpId itself is typed. Wiring the two needs a manifest that
// carries the tpId — a work-order this one does not own.
void DrawPlacementTemplatePicker(Params::ScatterTransform& transform, IconGridState& iconGridState,
                                 float iconGridHeight, const IconAtlasManifest* iconManifest,
                                 Pipeline::PreviewDriver* previewDriver) {
    // The field writes the tpId live and commits when it is left — the RT-toggle contract, in the
    // one control the shared library has no widget for.
    ImGui::InputText("Template Id (tpId)", transform.templateIdentifier,
                     IM_ARRAYSIZE(transform.templateIdentifier));
    NotifyPlacementChange(ImGui::IsItemDeactivatedAfterEdit(), previewDriver);
    if (iconManifest == nullptr) {
        ImGui::TextUnformatted("No resident icon atlas: type the template id above.");
        return;
    }
    DrawIconGrid("Template Atlas", *iconManifest, iconGridState, iconGridHeight);
    ImGui::Text("Selected icon id: %d", iconGridState.selectedIconId);
}

} // namespace Ui
} // namespace SanmapGen
