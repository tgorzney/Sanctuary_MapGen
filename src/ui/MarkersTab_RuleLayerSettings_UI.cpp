// MarkersTab_RuleLayerSettings_UI.cpp — the non-empty-layer Delete confirm, the Add/Remove buttons,
// the per-layer settings block (name/enabled/hidden/symmetry), and the per-rule detail sections
// (Gates/Quantity/Area/Focus/Placement Gate/Transform/Template Picker). The aspect-split sibling of
// MarkersTab_RuleLayers_UI.cpp (ARCH §1.5 — this file carries the settings half of the two-tier
// list's per-row content so neither .cpp crosses the file-size ceiling), both declared by
// MarkersTab_RuleLayers_UI.h. Shared widgets only: ConfirmDialog / TextInput / Checkbox / the
// shared symmetry section. No ImGui::SliderFloat / DragFloat / VSliderFloat.
#include "MarkersTab_RuleLayers_UI.h"
#include "MarkersTab_UI.h"
#include "Checkbox_UI.h"
#include "ConfirmDialog_UI.h"
#include "TextInput_UI.h"
#include "imgui.h"
#include <cstdio>

namespace SanmapGen {
namespace Ui {

// Drawn every frame a delete might still be pending, so its own modal popup gets the chance to run
// every frame it might be open (the FilesTab_ExportGate_UI.cpp DrawPendingExportWarningDialog
// pattern). The pending index is RE-VALIDATED against the vector's CURRENT size — it may have moved
// between the request frame and this one (Constitution §6: an index is clamped or rejected, never
// trusted).
bool DrawPendingDeleteRuleLayerDialog(std::vector<Params::MarkerRuleLayer>& markerRuleLayers,
                                      MarkersTabState& state) {
    if (state.pendingDeleteRuleLayerIndex < 0) return false;
    if (state.pendingDeleteRuleLayerIndex >= static_cast<int>(markerRuleLayers.size())) {
        state.pendingDeleteRuleLayerIndex = -1;
        return false;
    }
    const Params::MarkerRuleLayer& layer =
        markerRuleLayers[static_cast<std::size_t>(state.pendingDeleteRuleLayerIndex)];
    char body[128];
    std::snprintf(body, sizeof(body), "Delete layer \"%s\" and its %d rule(s)? This cannot be undone.",
                 layer.name.empty() ? "Marker Layer" : layer.name.c_str(),
                 static_cast<int>(layer.rules.size()));
    ConfirmDialogOptions options;
    options.title    = "Delete Marker Layer";
    options.bodyText = body;
    const ConfirmDialogChange change =
        DrawConfirmDialog("markerRuleLayerDeleteConfirm", state.deleteRuleLayerConfirmState, options);
    if (change.bSecondaryClicked) { state.pendingDeleteRuleLayerIndex = -1; return false; }
    if (!change.bPrimaryClicked) return false;
    markerRuleLayers.erase(markerRuleLayers.begin() + state.pendingDeleteRuleLayerIndex);
    state.pendingDeleteRuleLayerIndex = -1;
    return true;
}

// STEP120: extracted out of the old DrawRuleLayerButtons so a Bundle node's own "add a Layer here"
// can reuse it with a non-root parent (MarkersTab_Bundles_UI.cpp). `parentBundleIdentifierForNewLayer
// < 0` is root scope. STEP125: gains `markerTypeNameForNewLayer` (§4) — a Type-section's own
// "Ungrouped Procedural Rules" sub-list (DrawMarkerTypeSections, MarkersTab_TypeSections_UI.cpp)
// seeds it with that section's own type, so a layer added there does not mint with
// `markerTypeName == ""` and visually "vanish" into the Unassigned section next frame. The Bundle
// node's own call site (MarkersTab_BundleNodeBody_UI.cpp) leaves this parameter at its default —
// a Bundle-scoped Layer's own type is resolved at the leaf-index lookup, not seeded here.
bool DrawAddMarkerRuleLayerButton(std::vector<Params::MarkerRuleLayer>& markerRuleLayers, MarkersTabState& state,
                                  int parentBundleIdentifierForNewLayer,
                                  const std::string& markerTypeNameForNewLayer) {
    if (!ImGui::Button(parentBundleIdentifierForNewLayer < 0 ? "Add Layer" : "Add Procedural Layer Here"))
        return false;
    Params::MarkerRuleLayer layer;
    layer.parentBundleIdentifier = parentBundleIdentifierForNewLayer;   // STEP119 field
    layer.markerTypeName         = markerTypeNameForNewLayer;          // NEW — STEP125
    markerRuleLayers.push_back(layer);
    state.selectedRuleLayerIndex = static_cast<int>(markerRuleLayers.size()) - 1;
    state.selectedRuleIndex      = 0;
    return true;
}

// STEP125: Add Rule / Remove Selected Rule only — the "Add Layer" button is now per-Type-section
// (DrawAddMarkerRuleLayerButton, above). Both operate on the tab-wide `state.selectedRuleLayerIndex`/
// `selectedRuleIndex`, which is NOT type-scoped — drawing them once per Type-section would render up
// to N redundant copies, all operating on the same single global selection (a "Remove Selected Rule"
// click inside one section could delete a rule actually selected in another). Called exactly ONCE,
// tab-wide, by DrawMarkerTypeSections (§5(b)) — replaces the retired DrawRuleLayerButtons. Drawn
// disabled (never a silent no-op) when no layer is selected.
void DrawMarkerRuleButtons(std::vector<Params::MarkerRuleLayer>& markerRuleLayers, MarkersTabState& state,
                           Pipeline::PreviewDriver* previewDriver) {
    Params::MarkerRuleLayer* const layer = SelectedMarkerRuleLayer(markerRuleLayers, state);
    bool bRecipeMoved = false;
    ImGui::BeginDisabled(layer == nullptr);
    if (ImGui::Button("Add Rule") && layer != nullptr) {
        layer->rules.push_back(Params::MarkerRule());
        state.selectedRuleIndex = static_cast<int>(layer->rules.size()) - 1;
        bRecipeMoved = true;
    }
    ImGui::SameLine();
    const bool bRuleSelected = layer != nullptr && state.selectedRuleIndex >= 0
        && state.selectedRuleIndex < static_cast<int>(layer->rules.size());
    if (ImGui::Button("Remove Selected Rule") && bRuleSelected) {
        layer->rules.erase(layer->rules.begin() + state.selectedRuleIndex);
        state.selectedRuleIndex = static_cast<int>(layer->rules.size()) - 1;
        bRecipeMoved = true;
    }
    ImGui::EndDisabled();
    NotifyPlacementChange(bRecipeMoved, previewDriver);
}

// Human's own bug report — "Enabled, Hidden and Use Symmetry can be removed from within the
// [Procedural layer] ... it should now be located in the header as buttons": Name/Enabled/Hidden/
// "Use Global Symmetry" all moved OUT of this body — Name and the three toggles now live on the
// header (double-click-to-rename mirroring Manual/Group; [SYM][E/D][V/I][X],
// DrawRuleLayerHeaderNameOverlay/DrawRightAlignedProceduralLayerCluster,
// MarkersTab_BundleHeaderExtras_UI.cpp) — the exact same "gate moves to the header, detail stays in
// the body" split STEP142 already made for Manual Layers. Only the per-axis override checkboxes
// remain here, and only while this layer is NOT following the global mask
// (`!layer.symmetry.bSymmetryUseGlobal` — DrawPlacementSymmetryAxes's own body-half,
// DrawIndependentSymmetryAxes, was already conditional on exactly this before this change; the
// header's own "Use Global Symmetry" checkbox this function used to draw is what moved, not the
// condition itself).
void DrawRuleLayerSettings(Params::MarkerRuleLayer& layer, Pipeline::PreviewDriver* previewDriver) {
    if (!layer.symmetry.bSymmetryUseGlobal)
        DrawIndependentSymmetryAxes(layer.symmetry.symmetryMask, previewDriver);
}

// One rule's detail sections — Gates/Quantity/Area/Focus/Placement Gate/Transform/Template Picker
// — bound to the caller's own row (STEP110: called per rule row, inline, by DrawRuleLayerBody's row
// body, MarkersTab_RuleLayers_UI.cpp; moved out of MarkersTab_UI.cpp's DrawRuleStack, which used to
// draw this once at the bottom for whatever rule happened to be selected). The widget mirrors
// (slope/height/count/category/priority/focusGradient) are ONE shared buffer this tab already
// carries; reloading it from THIS rule right before drawing — the same "load right before use"
// contract LayerEditor_Layer_UI.cpp's DrawHeightBlendSection relies on for its own shared
// Levels/HeightMask mirrors — is what keeps two simultaneously expanded rows from bleeding into
// each other.
void DrawRuleSettings(Params::MarkerRule& rule, MarkersTabState& state,
                      Pipeline::PreviewDriver* previewDriver, const IconAtlasManifest* iconManifest,
                      const ProceduralInstanceListContext_UI& instanceListContext) {
    if (!state.slopeToggle.IsCommitDeferred() && !state.heightToggle.IsCommitDeferred()
        && !state.countToggle.IsCommitDeferred()) LoadMarkerRuleValues(rule, state);
    LoadMarkerRuleEnumIndices(rule, state.ruleDetail);
    DrawMarkerRuleGates(rule, state, previewDriver);
    DrawMarkerRuleQuantity(rule, state, previewDriver);
    DrawMarkerRuleArea(rule, state.ruleDetail, previewDriver);
    DrawMarkerRuleFocus(rule, state.ruleDetail, previewDriver);
    DrawPlacementGateSection(rule.maskStratumIndex, rule.maskWeightMinimum, rule.mapEdgePadding,
                             state.gate, previewDriver);
    DrawPlacementTransformSection(rule.transform, state.transform, previewDriver);
    DrawPlacementTemplatePicker(rule.transform, state.iconGridState, state.iconGridHeight,
                                iconManifest, previewDriver);
    DrawRuleInstanceList(instanceListContext);   // STEP132 (ARCH §19.27)
}

} // namespace Ui
} // namespace SanmapGen
