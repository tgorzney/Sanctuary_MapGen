// MarkersTab_ManualLayerRowBody_UI.cpp — DrawLayerRowBody, the aspect-split sibling of
// MarkersTab_ManualLayers_UI.cpp (ARCH §1.5 — the single-file draft crossed the 150-line hard
// ceiling once STEP120 needed this function callable from outside its own translation unit), both
// declared by MarkersTab_ManualLayers_UI.h. Mirrors MarkersTab_RuleLayers_UI.cpp/
// MarkersTab_RuleLayerSettings_UI.cpp's own established aspect split.
#include "MarkersTab_ManualLayerRowBody_UI.h"
#include "Checkbox_UI.h"
#include "MarkerLayerSymmetrySection_UI.h"
#include "SymmetryClusterInstanceList_UI.h"
#include "TextInput_UI.h"
#include "imgui.h"
#include <string>
#include <utility>

namespace SanmapGen {
namespace Ui {
namespace {

// One instance row's own body — unchanged from STEP126, just extracted so both the flat-list branch
// and the (STEP132) symmetry-cluster branch draw it identically, never a second near-duplicate copy.
void DrawManualInstanceRow(std::vector<Params::MarkerInstanceGroup>& markers,
                           const std::pair<int, int>& groupTransformIndex,
                           int& selectedManualInstanceIdentifier,
                           const std::function<void(int)>& selectManualMarkerInstanceCallback) {
    const Params::MarkerInstanceGroup& instanceGroup =
        markers[static_cast<std::size_t>(groupTransformIndex.first)];
    const Params::MarkerTransform& instanceTransform =
        instanceGroup.transforms[static_cast<std::size_t>(groupTransformIndex.second)];
    const std::string rowLabel = instanceGroup.name + " - " + (!instanceTransform.name.empty()
        ? instanceTransform.name : std::to_string(groupTransformIndex.second));
    const bool bRowSelected = selectedManualInstanceIdentifier == instanceTransform.instanceIdentifier;
    if (ImGui::Selectable(rowLabel.c_str(), bRowSelected)) {
        selectedManualInstanceIdentifier = instanceTransform.instanceIdentifier;
        // ARCH §19.25, item 5 — IN ADDITION TO the tab-local write above, not instead of it:
        // drives the canvas's own real selection, so the REAL icon-sprite render path
        // (MapCanvas_IconLayer_CullEmit_UI.cpp's `instance.bSelected`) reflects this click too.
        if (selectManualMarkerInstanceCallback)
            selectManualMarkerInstanceCallback(instanceTransform.instanceIdentifier);
    }
}

} // namespace

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
                      Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings, ManualMarkerLayersState& state,
                      const ManualInstanceLayerIndex_UI& instanceIndex, int& selectedManualInstanceIdentifier,
                      const std::function<void(int)>& selectManualMarkerInstanceCallback) {
    TextInputRules nameRules;
    nameRules.maximumLength = 48;
    nameRules.bAllowEmpty   = false;
    nameRules.fallbackText  = "Marker Layer";
    const bool bNameCommitted = DrawTextInput("Name", layer.name, nameRules).bCommitted;
    // STEP130: Color Override no longer has a body copy — it is reachable from the row header on
    // every row (ungrouped via DraggableList's header-extra slot, bundled via the Bundle tree's
    // `drawLeafHeaderExtra` slot), so a second, body-drawn control is redundant (see
    // DrawManualMarkerLayerColorOverrideHeaderControl below).
    DrawSliderScalar("Icon Scale", layer.iconScale, state.iconScaleRange,
                     state.selectedLayerIconScaleToggle, WidgetStyle(), "%.2f");
    const bool bSnapCommitted = DrawCheckbox("Snap to Grid", layer.bGridSnapEnabled).bCommitted;
    ImGui::BeginDisabled(!layer.bGridSnapEnabled);
    const bool bSnapSizeCommitted = DrawSliderScalar("Grid Size", layer.gridSnapSizeWorldUnits,
        state.gridSnapSizeRange, state.selectedLayerGridSnapToggle, WidgetStyle(), "%.2f").bCommitted;
    ImGui::EndDisabled();
    DrawLayerSymmetrySection(layer, layerIndex, markerLayers, markers, geometry, globalSymmetryMask,
                             globalRadialRepeatCount, markerSymmetryFixSettings, state);

    // STEP126, Open Q7 — the per-Layer instance list. Plain ImGui::Selectable rows, NOT a DraggableList
    // instantiation (an instance's own home group can differ from this Layer, so there is no single
    // homogeneous backing vector for a reorder/delete signal to apply against — see the design doc's
    // own reasoning for rejecting DraggableList<Params::MarkerTransform> here). No delete/reorder
    // affordance: deletion/repositioning stays owned by the roster editor (MarkersTab_Manual_UI.h).
    // STEP132 (ARCH §19.26) — partitioned by `symmetryGroupIdentifier`: non-zero buckets render first
    // as their own collapsible "Symmetry Group N (k)" node, `== 0` (never-dragged/-repaired) instances
    // list flat after, via the shared cluster-list helper (SymmetryClusterInstanceList_UI.h) Part B's
    // procedural instance list also calls, parameterized on the OPPOSITE (bucket-size) predicate.
    ImGui::Separator();
    ImGui::TextUnformatted("Instances");
    const auto instanceIt = instanceIndex.instancesByLayerIndex.find(layerIndex);
    if (instanceIt == instanceIndex.instancesByLayerIndex.end() || instanceIt->second.empty()) {
        ImGui::TextDisabled("(none)");
    } else {
        DrawSymmetryClusterInstanceList<std::pair<int, int>>(instanceIt->second,
            [&](const std::pair<int, int>& groupTransformIndex) {
                return markers[static_cast<std::size_t>(groupTransformIndex.first)]
                    .transforms[static_cast<std::size_t>(groupTransformIndex.second)]
                    .symmetryGroupIdentifier;
            },
            [](int groupIdentifier, int /*bucketSize*/) { return groupIdentifier != 0; },
            [&](const std::pair<int, int>& groupTransformIndex) {
                DrawManualInstanceRow(markers, groupTransformIndex, selectedManualInstanceIdentifier,
                                      selectManualMarkerInstanceCallback);
            });
    }
    return bNameCommitted || bSnapCommitted || bSnapSizeCommitted;
}

// STEP123: the row header's own compact Color Override control — checkbox + a small inline swatch,
// drawn on EVERY row's collapsed header line via DraggableList's/TreeListWidget's header-extra slot,
// NOT gated on the row's own expand state. Disabled (not hidden) while state.bUseGroupColor forces
// one shared tint, so the header's own width never shifts when that block-wide toggle flips.
// STEP130: this is now the ONLY place Color Override draws for EITHER an ungrouped row (the
// STEP123 DraggableList slot) or a bundled row (the Bundle tree's `drawLeafHeaderExtra` slot,
// ARCH §19.23) — the body copy this comment used to explain away is deleted, since both paths now
// reach this function.
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
    headerSwatchOptions.bRealtimeToggleHidden = true;   // color edits are always realtime, no choice
    const bool bColorCommitted = DrawColorSwatch("ColorOverrideHeaderSwatch", layer.color,
        headerSwatchOptions, state.selectedLayerColorToggle).bCommitted;
    ImGui::EndDisabled();
    ImGui::EndDisabled();
    if (bOverrideCommitted || bColorCommitted) bAnyCommitted = true;
}

// STEP130 (ARCH §19.24): the row header's own Symmetry-toggle control — a plain checkbox bound to
// `layer.bSymmetryEnabled`, no swatch, mirroring the Color Override checkbox's own empty-label +
// hover-tooltip shape exactly. Placed LEFT of Color Override at every call site. Never touches
// `layer.symmetry`'s own fields — toggling only flips the gate `ResolveEffectiveMarkerSymmetry`
// reads (MarkerDragGesture_UI.h), so re-enabling restores the prior configuration unchanged.
void DrawMarkerLayerSymmetryToggleHeaderControl(Params::MarkerInstanceLayer& layer, bool& bAnyCommitted) {
    const bool bSymmetryCommitted = DrawCheckbox("", layer.bSymmetryEnabled).bCommitted;
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Symmetry");
    if (bSymmetryCommitted) bAnyCommitted = true;
}

} // namespace Ui
} // namespace SanmapGen
