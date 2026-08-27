// MarkersTab_BundleHeaderExtras_UI.cpp — STEP140: DrawMarkerLayerBundleNodeHeaderExtra (a Group's
// own double-click-to-rename + "X" delete) and DrawMarkerGroupLeafHeaderExtra (a Layer leaf's own
// STEP130 Symmetry/Color Override controls, now plus its own "X" delete). Both declared in
// MarkersTab_Bundles_UI.h; split into their own file (ARCH §1.5 — MarkersTab_Bundles_UI.cpp has no
// headroom left), mirroring MarkersTab_BundleNodeBody_UI.cpp's own aspect-split precedent. Neither
// function mutates bundles/ruleLayers/instanceLayers/markers directly — a rename edits `bundle.name`
// in place (non-structural, safe mid-walk), but every delete only RECORDS a pending choice into
// MarkerLayerBundlesState for the caller to apply AFTER the tree's own recursive walk finishes this
// frame (see that struct's own field comments, MarkersTab_Bundles_UI.h).
#include "MarkersTab_Bundles_UI.h"
#include "MarkersTab_ManualLayerRowBody_UI.h"
#include "TextInput_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

// Right-aligns a lone "X" delete button within whatever's left of the row's own reserved
// header-extra zone — a Group node (nothing else drawn there) and a Procedural leaf (no Symmetry/
// Color Override fields to draw first) both call this so their own "X" lands at the SAME right edge
// a Manual leaf's own X naturally reaches after its two preceding controls (mirrors
// MarkersTab_UI.cpp's own DrawRightAlignedHideToggleButton).
bool DrawRightAlignedDeleteButton(const char* label) {
    const float buttonWidth = ImGui::CalcTextSize(label).x + ImGui::GetStyle().FramePadding.x * 2.0f;
    const float availableWidth = ImGui::GetContentRegionAvail().x;
    if (availableWidth > buttonWidth)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + availableWidth - buttonWidth);
    return ImGui::SmallButton(label);
}

} // namespace

// A Group's own header-extra: double-click the header (the LAST-submitted item when this runs,
// RenderNode's own contract, TreeListWidget_RowLayout_UI.h) starts a rename; otherwise an "X" opens
// a two-choice delete popup. Renaming and deleting are mutually exclusive on one row at a time —
// while renaming, no delete button is drawn (nothing to click through it accidentally).
void DrawMarkerLayerBundleNodeHeaderExtra(int bundleIdentifier,
                                          std::vector<Params::MarkerLayerBundle>& bundles,
                                          MarkerLayerBundlesState& state) {
    if (state.renamingBundleIdentifier == bundleIdentifier) {
        for (Params::MarkerLayerBundle& bundle : bundles) {
            if (bundle.identifier != bundleIdentifier) continue;
            TextInputRules nameRules;
            nameRules.maximumLength = 48; nameRules.bAllowEmpty = false; nameRules.fallbackText = "Group";
            DrawTextInput("##renameGroup", bundle.name, nameRules, WidgetStyle(), nullptr,
                         /*bLabelHidden=*/true);
            if (ImGui::IsItemDeactivated()) state.renamingBundleIdentifier = -1;
            return;
        }
        state.renamingBundleIdentifier = -1;   // the bundle vanished (deleted elsewhere) this frame
        return;
    }

    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        state.renamingBundleIdentifier = bundleIdentifier;
        return;
    }
    if (DrawRightAlignedDeleteButton("X##deleteGroup")) ImGui::OpenPopup("deleteGroupPopup");
    if (ImGui::BeginPopup("deleteGroupPopup")) {
        if (ImGui::MenuItem("Delete Group Only (keep contents)")) {
            state.pendingDeleteBundleIdentifier = bundleIdentifier;
            state.bPendingDeleteBundleCascade   = false;
        }
        if (ImGui::MenuItem("Delete All (group + contents)")) {
            state.pendingDeleteBundleIdentifier = bundleIdentifier;
            state.bPendingDeleteBundleCascade   = true;
        }
        ImGui::EndPopup();
    }
}

// A Layer leaf's own header-extra: STEP130's Symmetry/Color Override pair (Manual only — Procedural
// has neither field) plus, STEP140, an "X" delete on EVERY leaf. Manual offers a choice (its
// Instances are separable content); Procedural is a single action (its Rules are not).
void DrawMarkerGroupLeafHeaderExtra(const MarkerGroupLeafKey_UI& leaf,
                                    std::vector<Params::MarkerInstanceLayer>& instanceLayers,
                                    ManualMarkerLayersState& manualLayersState,
                                    MarkerLayerBundlesState& bundlesState, bool& bAnyCommitted) {
    if (leaf.kind == MarkerGroupLeafKey_UI::Kind::Manual) {
        if (leaf.layerIndex >= 0 && leaf.layerIndex < static_cast<int>(instanceLayers.size())) {
            Params::MarkerInstanceLayer& layer = instanceLayers[static_cast<std::size_t>(leaf.layerIndex)];
            DrawMarkerLayerSymmetryToggleHeaderControl(layer, bAnyCommitted);
            ImGui::SameLine();
            DrawManualMarkerLayerColorOverrideHeaderControl(layer, manualLayersState, bAnyCommitted);
            ImGui::SameLine();
        }
        if (ImGui::SmallButton("X##deleteLayer")) ImGui::OpenPopup("deleteManualLayerPopup");
        if (ImGui::BeginPopup("deleteManualLayerPopup")) {
            if (ImGui::MenuItem("Delete Layer Only (keep instances)")) {
                bundlesState.pendingDeleteManualLayerIndex    = leaf.layerIndex;
                bundlesState.bPendingDeleteManualLayerCascade = false;
            }
            if (ImGui::MenuItem("Delete All (layer + instances)")) {
                bundlesState.pendingDeleteManualLayerIndex    = leaf.layerIndex;
                bundlesState.bPendingDeleteManualLayerCascade = true;
            }
            ImGui::EndPopup();
        }
        return;
    }

    if (DrawRightAlignedDeleteButton("X##deleteLayer")) ImGui::OpenPopup("deleteRuleLayerPopup");
    if (ImGui::BeginPopup("deleteRuleLayerPopup")) {
        if (ImGui::MenuItem("Delete Layer")) bundlesState.pendingDeleteProceduralLayerIndex = leaf.layerIndex;
        ImGui::EndPopup();
    }
}

} // namespace Ui
} // namespace SanmapGen
