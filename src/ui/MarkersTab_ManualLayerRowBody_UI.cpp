// MarkersTab_ManualLayerRowBody_UI.cpp — DrawLayerRowBody, the aspect-split sibling of
// MarkersTab_ManualLayers_UI.cpp (ARCH §1.5 — the single-file draft crossed the 150-line hard
// ceiling once STEP120 needed this function callable from outside its own translation unit), both
// declared by MarkersTab_ManualLayers_UI.h. Mirrors MarkersTab_RuleLayers_UI.cpp/
// MarkersTab_RuleLayerSettings_UI.cpp's own established aspect split.
#include "MarkersTab_ManualLayers_UI.h"
#include "Checkbox_UI.h"
#include "MarkerLayerSymmetrySection_UI.h"
#include "TextInput_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {

// The row's own name, tint, icon scale, grid snap and symmetry setting — STEP110: drawn inline in THIS
// row's own expanded body, not "selected"-gated. Tint hides under the block's shared-color mode
// (ARCH §4 rival-control rule). Layer-level symmetry is the deliberate, separately-ratified exception
// manual markers get over Props/Decals (ARCH_14_13_OpenItems.md §14.13 Ruling 3) — returns whether the
// name committed, so the caller can re-run the uniqueness repair. STEP120: lives in its own
// translation unit (not MarkersTab_ManualLayers_UI.cpp's anonymous namespace) so
// MarkersTab_Bundles_UI.cpp can reuse it UNCHANGED as the tree's Manual leaf-body callback
// (ARCH_19_07's "good news" finding).
bool DrawLayerRowBody(Params::MarkerInstanceLayer& layer, int layerIndex,
                      const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                      std::vector<Params::MarkerInstanceGroup>& markers, const Params::Geometry& geometry,
                      int globalSymmetryMask, int globalRadialRepeatCount,
                      Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings, ManualMarkerLayersState& state) {
    TextInputRules nameRules;
    nameRules.maximumLength = 48;
    nameRules.bAllowEmpty   = false;
    nameRules.fallbackText  = "Marker Layer";
    const bool bNameCommitted = DrawTextInput("Name", layer.name, nameRules).bCommitted;
    bool bColorOverrideCommitted = false;
    if (!state.bUseGroupColor) {
        bColorOverrideCommitted = DrawCheckbox("Color Override", layer.bColorOverrideEnabled).bCommitted;
        ImGui::BeginDisabled(!layer.bColorOverrideEnabled);
        DrawColorSwatch("Color", layer.color, state.previewColorOptions, state.selectedLayerColorToggle);
        ImGui::EndDisabled();
    }
    DrawSliderScalar("Icon Scale", layer.iconScale, state.iconScaleRange,
                     state.selectedLayerIconScaleToggle, WidgetStyle(), "%.2f");
    const bool bSnapCommitted = DrawCheckbox("Snap to Grid", layer.bGridSnapEnabled).bCommitted;
    ImGui::BeginDisabled(!layer.bGridSnapEnabled);
    const bool bSnapSizeCommitted = DrawSliderScalar("Grid Size", layer.gridSnapSizeWorldUnits,
        state.gridSnapSizeRange, state.selectedLayerGridSnapToggle, WidgetStyle(), "%.2f").bCommitted;
    ImGui::EndDisabled();
    DrawLayerSymmetrySection(layer, layerIndex, markerLayers, markers, geometry, globalSymmetryMask,
                             globalRadialRepeatCount, markerSymmetryFixSettings, state);
    return bNameCommitted || bColorOverrideCommitted || bSnapCommitted || bSnapSizeCommitted;
}

// STEP123: the row header's own compact Color Override control — checkbox + a small inline swatch,
// drawn on EVERY row's collapsed header line via DraggableList's new header-extra slot, NOT gated on
// the row's own expand state. Disabled (not hidden) while state.bUseGroupColor forces one shared
// tint, so the header's own width never shifts when that block-wide toggle flips — deliberately
// unlike the (unchanged, still-hidden-when-forced) body copy above; see the Out of Scope note in
// STEP123_MarkerLayerColorOverrideOnHeader_UI.md for why the body copy is NOT removed: bundled
// Manual Marker Layers reach this function ONLY through the Bundle tree's leaf-body callback
// (TreeListWidget_UI, which has no header-extra mechanism of its own), so deleting the body copy
// would silently strip Color Override access from every bundled layer.
void DrawManualMarkerLayerColorOverrideHeaderControl(Params::MarkerInstanceLayer& layer,
                                                      ManualMarkerLayersState& state, bool& bAnyCommitted) {
    ImGui::BeginDisabled(state.bUseGroupColor);
    const bool bOverrideCommitted = DrawCheckbox("", layer.bColorOverrideEnabled).bCommitted;
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Color Override");
    ImGui::SameLine();
    ImGui::BeginDisabled(!layer.bColorOverrideEnabled);
    Ui::ColorSwatchOptions headerSwatchOptions = state.previewColorOptions;  // COPY: do not mutate
                                                                              // the shared block-level
                                                                              // options struct
    headerSwatchOptions.bLabelHidden = true;
    headerSwatchOptions.swatchWidth  = kMarkerLayerColorOverrideSwatchWidthPixels;
    const bool bColorCommitted = DrawColorSwatch("ColorOverrideHeaderSwatch", layer.color,
        headerSwatchOptions, state.selectedLayerColorToggle).bCommitted;
    ImGui::EndDisabled();
    ImGui::EndDisabled();
    if (bOverrideCommitted || bColorCommitted) bAnyCommitted = true;
}

} // namespace Ui
} // namespace SanmapGen
