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
#include "MarkersTab_ManualInstanceSelection_UI.h"
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
    // STEP143 (human's own bug report — "why does X appear with empty space to its right") — the
    // root cause: `label` carries an imgui "##id" suffix ("X##deleteGroup"), and CalcTextSize's own
    // `hide_text_after_double_hash` defaults to false, so the ORIGINAL call measured the ENTIRE
    // string (including the invisible "##deleteGroup" part) instead of just the "X" SmallButton
    // actually renders — wildly over-estimating buttonWidth, which pushed the button too far LEFT of
    // where flush-right actually is. Passing `true` here measures only what's really drawn.
    const float buttonWidth = ImGui::CalcTextSize(label, nullptr, true).x
                             + ImGui::GetStyle().FramePadding.x * 2.0f;
    const float availableWidth = ImGui::GetContentRegionAvail().x;
    if (availableWidth > buttonWidth)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + availableWidth - buttonWidth);
    return ImGui::SmallButton(label);
}

} // namespace

// A Group's own header-extra: double-click the header (the LAST-submitted item when this runs,
// RenderNode's own contract, TreeListWidget_RowLayout_UI.h) starts a rename, positioned OVER the
// header's own name text (GetItemRectMin + GetTreeNodeToLabelSpacing — human's own correction: not
// the far-right header-extra zone); otherwise an "X" opens a two-choice delete popup. Renaming and
// deleting are mutually exclusive on one row at a time — while renaming, no delete button is drawn.
// A SCRATCH buffer, not `bundle.name` directly — see DrawLayerHeaderNameOverlay's own header comment
// (MarkersTab_ManualLayerRowBody_UI.h) for why live-editing the real field is the wrong move here.
void DrawMarkerLayerBundleNodeHeaderExtra(int bundleIdentifier,
                                          std::vector<Params::MarkerLayerBundle>& bundles,
                                          MarkerLayerBundlesState& state) {
    const ImVec2 itemMin = ImGui::GetItemRectMin();
    const float labelStartX = itemMin.x + ImGui::GetTreeNodeToLabelSpacing();

    if (state.renamingBundleIdentifier == bundleIdentifier) {
        for (Params::MarkerLayerBundle& bundle : bundles) {
            if (bundle.identifier != bundleIdentifier) continue;
            ImGui::SetCursorScreenPos(ImVec2(labelStartX, itemMin.y));
            TextInputRules nameRules;
            nameRules.maximumLength = 48; nameRules.bAllowEmpty = false; nameRules.fallbackText = "Group";
            DrawTextInput("##renameGroup", state.renameScratchText, nameRules, WidgetStyle(), nullptr,
                         /*bLabelHidden=*/true);
            if (ImGui::IsItemDeactivated()) {
                bundle.name = SanitizeTextInput(state.renameScratchText, nameRules);
                state.renamingBundleIdentifier = -1;
            }
            return;
        }
        state.renamingBundleIdentifier = -1;   // the bundle vanished (deleted elsewhere) this frame
        return;
    }

    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        state.renamingBundleIdentifier = bundleIdentifier;
        for (const Params::MarkerLayerBundle& bundle : bundles)
            if (bundle.identifier == bundleIdentifier) { state.renameScratchText = bundle.name; break; }
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
// has neither field) plus, STEP140, an "X" delete on EVERY leaf, plus, STEP141, a drag-drop TARGET
// on a Manual leaf's own row (an Instance dropped here reassigns to THIS layerIndex — Procedural
// leaves accept no Instances). The drop-target check runs FIRST, before anything else in this
// function draws a new widget, so it still attaches to the leaf's own TreeNodeEx row (the "last
// item" at the moment this callback starts, RenderLeaf's own contract).
void DrawMarkerGroupLeafHeaderExtra(const MarkerGroupLeafKey_UI& leaf,
                                    std::vector<Params::MarkerInstanceLayer>& instanceLayers,
                                    std::vector<Params::MarkerInstanceGroup>& markers,
                                    ManualMarkerLayersState& manualLayersState,
                                    MarkerLayerBundlesState& bundlesState,
                                    const std::vector<int>& selectedManualInstanceIdentifiers,
                                    bool& bAnyCommitted) {
    if (leaf.kind == MarkerGroupLeafKey_UI::Kind::Manual) {
        DrawManualLayerInstanceDropTarget(leaf.layerIndex, markers, selectedManualInstanceIdentifiers);
        if (leaf.layerIndex < 0 || leaf.layerIndex >= static_cast<int>(instanceLayers.size())) return;
        Params::MarkerInstanceLayer& layer = instanceLayers[static_cast<std::size_t>(leaf.layerIndex)];
        // STEP142 — double-click-the-header rename FIRST: while active, it claims the rest of the
        // row (the name box fills whatever's left), so SYM/COL/X don't draw this frame.
        if (DrawLayerHeaderNameOverlay(leaf.layerIndex, layer, manualLayersState, bAnyCommitted)) return;
        DrawMarkerLayerSymmetryToggleHeaderControl(layer, bAnyCommitted);
        ImGui::SameLine();
        DrawManualMarkerLayerColorOverrideHeaderControl(layer, manualLayersState, bAnyCommitted);
        ImGui::SameLine();
        if (DrawRightAlignedDeleteButton("X##deleteLayer")) ImGui::OpenPopup("deleteManualLayerPopup");
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
