// MarkersTab_RuleLayerSettings_UI.cpp — the non-empty-layer Delete confirm, the Add/Remove buttons,
// and the Selected Layer settings block (name/enabled/hidden/symmetry). The aspect-split sibling of
// MarkersTab_RuleLayers_UI.cpp (ARCH §1.5), both declared by MarkersTab_RuleLayers_UI.h. Shared
// widgets only: ConfirmDialog / TextInput / Checkbox / the shared symmetry section. No
// ImGui::SliderFloat / DragFloat / VSliderFloat.
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

// Add Layer is always available, including at zero layers. Add Rule / Remove Selected Rule operate
// on the selected layer and are drawn disabled (never a silent no-op) when none is selected.
void DrawRuleLayerButtons(std::vector<Params::MarkerRuleLayer>& markerRuleLayers, MarkersTabState& state,
                          Pipeline::PreviewDriver* previewDriver) {
    bool bRecipeMoved = false;
    if (ImGui::Button("Add Layer")) {
        markerRuleLayers.push_back(Params::MarkerRuleLayer());
        state.selectedRuleLayerIndex = static_cast<int>(markerRuleLayers.size()) - 1;
        state.selectedRuleIndex      = 0;
        bRecipeMoved = true;
    }
    Params::MarkerRuleLayer* const layer = SelectedMarkerRuleLayer(markerRuleLayers, state);
    ImGui::SameLine();
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

// The one place a layer's own fields are edited — bound to SelectedMarkerRuleLayer, never a row
// body (STEP80 §2, mirroring how DrawLayersTab draws per-layer controls BELOW the nested list
// rather than inside a row body).
void DrawSelectedRuleLayerSettings(std::vector<Params::MarkerRuleLayer>& markerRuleLayers,
                                   MarkersTabState& state, Pipeline::PreviewDriver* previewDriver) {
    Params::MarkerRuleLayer* const layer = SelectedMarkerRuleLayer(markerRuleLayers, state);
    if (layer == nullptr) {
        ImGui::TextUnformatted("Select a marker layer to edit it.");
        return;
    }
    TextInputRules nameRules;
    nameRules.maximumLength = 48;
    nameRules.bAllowEmpty   = false;
    nameRules.fallbackText  = "Marker Layer";
    NotifyPlacementChange(DrawTextInput("Layer Name", layer->name, nameRules).bCommitted, previewDriver);
    NotifyPlacementChange(DrawCheckbox("Enabled (off = this whole layer is not generated)",
                                       layer->bEnabled).bCommitted, previewDriver);
    NotifyPlacementChange(DrawCheckbox("Hidden (still generated for clearance/fairness, not drawn)",
                                       layer->bHidden).bCommitted, previewDriver);
    DrawPlacementSymmetryAxes("markerLayerSymmetry", layer->symmetry.bSymmetryUseGlobal,
                              layer->symmetry.symmetryMask, previewDriver);
}

} // namespace Ui
} // namespace SanmapGen
