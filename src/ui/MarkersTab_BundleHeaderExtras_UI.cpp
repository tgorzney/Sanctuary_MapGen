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
#include "MarkerLayerEnabledVisibilityToggle_UI.h"
#include "MarkersTab_ManualInstanceSelection_UI.h"
#include "MarkersTab_ManualLayerRowBody_UI.h"
#include "PlacementRuleSections_UI.h"
#include "TextInput_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

// STEP143 (human's own bug report — "why does X appear with empty space to its right") — the root
// cause every width in this file now avoids: CalcTextSize's own `hide_text_after_double_hash`
// defaults to false, so measuring a "X##deleteGroup"-style label with the 2-arg overload counts the
// invisible "##..." suffix as visible text, wildly over-estimating the button's own width and
// landing well short of flush-right. Every measurement below passes `true` explicitly.
float VisibleButtonWidth(const char* label) {
    return ImGui::CalcTextSize(label, nullptr, true).x + ImGui::GetStyle().FramePadding.x * 2.0f;
}

// The gap between buttons in a right-aligned cluster — mirrors MarkersTab_UI.cpp's own
// kHeaderButtonSpacingPixels (a separate, file-local constant there; not exported).
constexpr float kClusterButtonSpacingPixels = 8.0f;

// Right-aligns a lone "X" delete button within whatever's left of the row's own reserved
// header-extra zone — a Group node (nothing else drawn there) calls this directly; Layer leaves
// (both kinds) fold it into their own multi-button cluster helpers below instead.
bool DrawRightAlignedDeleteButton(const char* label) {
    const float buttonWidth = VisibleButtonWidth(label);
    const float availableWidth = ImGui::GetContentRegionAvail().x;
    if (availableWidth > buttonWidth)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + availableWidth - buttonWidth);
    return ImGui::SmallButton(label);
}

// STEP144 — a Procedural Layer leaf's own [E/D][V/I][X], right-aligned as one cluster (the tree has
// no built-in affordance strip the way DraggableList's ungrouped rows do, so this is a clean
// addition, not a duplicate of anything). The coupled toggle rules live in
// MarkerLayerEnabledVisibilityToggle_UI.h; a non-structural field flip notifies immediately (unlike
// delete, which stays deferred — MarkerLayerBundlesState's own pending-delete fields).
void DrawRightAlignedProceduralLayerCluster(Params::MarkerRuleLayer& layer, int layerIndex,
                                            MarkerLayerBundlesState& bundlesState,
                                            Pipeline::PreviewDriver* previewDriver) {
    const char* const enabledLabel = layer.bEnabled ? "E##enabled" : "D##enabled";
    const char* const visibleLabel = layer.bHidden  ? "I##visible" : "V##visible";
    const char* const deleteLabel  = "X##deleteLayer";
    const float totalWidth = VisibleButtonWidth(enabledLabel) + kClusterButtonSpacingPixels
                            + VisibleButtonWidth(visibleLabel) + kClusterButtonSpacingPixels
                            + VisibleButtonWidth(deleteLabel);
    const float availableWidth = ImGui::GetContentRegionAvail().x;
    if (availableWidth > totalWidth)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + availableWidth - totalWidth);

    if (ImGui::SmallButton(enabledLabel)) {
        ApplyMarkerRuleLayerEnabledToggle(layer.bEnabled, layer.bHidden);
        NotifyPlacementChange(true, previewDriver);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Enabled / Disabled");
    ImGui::SameLine(0.0f, kClusterButtonSpacingPixels);
    if (ImGui::SmallButton(visibleLabel)) {
        ApplyMarkerRuleLayerVisibilityToggle(layer.bEnabled, layer.bHidden);
        NotifyPlacementChange(true, previewDriver);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Visible / Invisible in preview");
    ImGui::SameLine(0.0f, kClusterButtonSpacingPixels);
    if (ImGui::SmallButton(deleteLabel)) ImGui::OpenPopup("deleteRuleLayerPopup");
    if (ImGui::BeginPopup("deleteRuleLayerPopup")) {
        if (ImGui::MenuItem("Delete Layer")) bundlesState.pendingDeleteProceduralLayerIndex = layerIndex;
        ImGui::EndPopup();
    }
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
                                          const std::vector<Params::MarkerInstanceLayer>& instanceLayers,
                                          std::vector<Params::MarkerInstanceGroup>& markers,
                                          const std::vector<int>& selectedManualInstanceIdentifiers,
                                          MarkerLayerBundlesState& state) {
    // STEP148 (human's own bug report — "I tried to drag an instance to a group, and it stayed
    // where it was") — run FIRST, before any other widget in this callback draws (the drop target
    // binds to the LAST item imgui submitted, the Group's own CollapsingHeader, RenderNode's own
    // contract, TreeListWidget_RowLayout_UI.h — same "run first" reasoning
    // DrawMarkerGroupLeafHeaderExtra's own Manual-leaf drop target already follows). Human's own
    // choice: land on the Group's first Manual Layer; FirstManualLayerIndexInBundle returns -1 when
    // the Group has none yet, in which case this is skipped entirely — the drop silently does
    // nothing, same as before this fix, the accepted behavior for that specific case.
    const int firstLayerIndex = FirstManualLayerIndexInBundle(bundleIdentifier, instanceLayers);
    if (firstLayerIndex >= 0)
        DrawManualLayerInstanceDropTarget(firstLayerIndex, markers, selectedManualInstanceIdentifiers);

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
// has neither field) plus, STEP144, a "V/I" visibility toggle on a Manual leaf (MarkerInstanceLayer::
// bHidden, new field) and an "E/D"+"V/I" coupled pair on a Procedural leaf
// (DrawRightAlignedProceduralLayerCluster, above — MarkerRuleLayer already carries bEnabled/bHidden),
// plus, STEP140, an "X" delete on EVERY leaf, plus, STEP141, a drag-drop TARGET on a Manual leaf's
// own row (an Instance dropped here reassigns to THIS layerIndex — Procedural leaves accept no
// Instances). The drop-target check runs FIRST, before anything else in this function draws a new
// widget, so it still attaches to the leaf's own TreeNodeEx row (the "last item" at the moment this
// callback starts, RenderLeaf's own contract).
void DrawMarkerGroupLeafHeaderExtra(const MarkerGroupLeafKey_UI& leaf,
                                    std::vector<Params::MarkerRuleLayer>& ruleLayers,
                                    std::vector<Params::MarkerInstanceLayer>& instanceLayers,
                                    std::vector<Params::MarkerInstanceGroup>& markers,
                                    ManualMarkerLayersState& manualLayersState,
                                    MarkerLayerBundlesState& bundlesState,
                                    const std::vector<int>& selectedManualInstanceIdentifiers,
                                    Pipeline::PreviewDriver* previewDriver, bool& bAnyCommitted) {
    if (leaf.kind == MarkerGroupLeafKey_UI::Kind::Manual) {
        DrawManualLayerInstanceDropTarget(leaf.layerIndex, markers, selectedManualInstanceIdentifiers);
        if (leaf.layerIndex < 0 || leaf.layerIndex >= static_cast<int>(instanceLayers.size())) return;
        Params::MarkerInstanceLayer& layer = instanceLayers[static_cast<std::size_t>(leaf.layerIndex)];
        // STEP142 — double-click-the-header rename FIRST: while active, it claims the rest of the
        // row (the name box fills whatever's left), so SYM/COL/V-I/X don't draw this frame.
        if (DrawLayerHeaderNameOverlay(leaf.layerIndex, layer, manualLayersState, bAnyCommitted)) return;
        DrawMarkerLayerSymmetryToggleHeaderControl(layer, bAnyCommitted);
        ImGui::SameLine();
        DrawManualMarkerLayerColorOverrideHeaderControl(layer, manualLayersState, bAnyCommitted);
        ImGui::SameLine();
        // STEP144 — a straight V/I toggle, no E/D coupling (a hand-placed Manual layer has no
        // "generation enabled" concept the way a Procedural one does).
        if (ImGui::SmallButton(layer.bHidden ? "I##visible" : "V##visible")) {
            layer.bHidden = !layer.bHidden;
            bAnyCommitted = true;
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Visible / Invisible in preview");
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

    if (leaf.layerIndex < 0 || leaf.layerIndex >= static_cast<int>(ruleLayers.size())) return;
    DrawRightAlignedProceduralLayerCluster(ruleLayers[static_cast<std::size_t>(leaf.layerIndex)],
                                           leaf.layerIndex, bundlesState, previewDriver);
}

} // namespace Ui
} // namespace SanmapGen
