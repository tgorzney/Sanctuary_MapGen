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
#include "MarkersTab_MarkerLinkResolvers_UI.h"
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

} // namespace

// Human's own bug report — the "SYM" button alone, extracted so the flat/ungrouped list (which
// already gets Enabled/Hidden/Delete from DraggableList's own built-in affordance strip — adding a
// second explicit set of E/D/V/I/X buttons there would duplicate that strip, not fix anything) can
// reuse just this piece, while DrawRightAlignedProceduralLayerCluster below (the Bundle tree's own,
// which has no built-in strip at all) composes it into the full [SYM][E/D][V/I][X] cluster. Flips
// `layer.symmetry.bSymmetryUseGlobal` directly — TRUE ("on"/highlighted) means this layer follows
// the recipe's global symmetry, exactly Manual's own SYM=on polarity
// (DrawMarkerLayerSymmetryToggleHeaderControl, MarkersTab_ManualLayerRowBody_UI.cpp); OFF reveals
// the per-axis override checkboxes that stay in the body (DrawRuleLayerSettings).
void DrawRuleLayerSymmetryToggleHeaderControl(Params::MarkerRuleLayer& layer,
                                              Pipeline::PreviewDriver* previewDriver) {
    if (layer.symmetry.bSymmetryUseGlobal)
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    const bool bSymmetryCommitted = ImGui::SmallButton("SYM##symmetry");
    if (layer.symmetry.bSymmetryUseGlobal) ImGui::PopStyleColor();
    if (bSymmetryCommitted) {
        layer.symmetry.bSymmetryUseGlobal = !layer.symmetry.bSymmetryUseGlobal;
        NotifyPlacementChange(true, previewDriver);
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Use Global Symmetry (off = this layer's own axes, below)");
}

// Human's own bug report — "Enabled, Hidden and Use Symmetry can be removed from within the
// [Procedural layer] ... it should now be located in the header as buttons" — [SYM][E/D][V/I][X],
// right-aligned as one cluster. The E/D+V/I coupled toggle rules live in
// MarkerLayerEnabledVisibilityToggle_UI.h; a non-structural field flip notifies immediately (unlike
// delete, which stays deferred — MarkerLayerBundlesState's own pending-delete fields). Width uses
// the SAME fixed reserved-zone-budget constants (MarkersTab_Bundles_UI.h) Manual's own cluster uses
// — not per-label VisibleButtonWidth measurement (E vs D / V vs I share the same real width in
// practice, so a fixed budget is exact, and it's what `reservedZoneWidthPixels` — see this
// function's own declaration comment for why it replaces GetContentRegionAvail() — is expressed
// against in the first place).
void DrawRightAlignedProceduralLayerCluster(Params::MarkerRuleLayer& layer, int layerIndex,
                                            MarkerLayerBundlesState& bundlesState,
                                            Pipeline::PreviewDriver* previewDriver,
                                            float reservedZoneWidthPixels) {
    const char* const enabledLabel = layer.bEnabled ? "E##enabled" : "D##enabled";
    const char* const visibleLabel = layer.bHidden  ? "I##visible" : "V##visible";
    const char* const deleteLabel  = "X##deleteLayer";
    const float clusterWidth = kMarkerRuleLayerSymmetryButtonWidthPixels + kClusterButtonSpacingPixels
                             + kMarkerRuleLayerEnabledButtonWidthPixels + kClusterButtonSpacingPixels
                             + kMarkerRuleLayerVisibilityButtonWidthPixels + kClusterButtonSpacingPixels
                             + kMarkerRuleLayerDeleteButtonWidthPixels;
    if (reservedZoneWidthPixels > clusterWidth)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + reservedZoneWidthPixels - clusterWidth);

    DrawRuleLayerSymmetryToggleHeaderControl(layer, previewDriver);
    ImGui::SameLine(0.0f, kClusterButtonSpacingPixels);

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

// Human's own bug report — see this function's own declaration comment (MarkersTab_Bundles_UI.h) for
// the full contract. Mirrors DrawLayerHeaderNameOverlay's own shape (Manual Layers,
// MarkersTab_ManualLayerRowBody_UI.cpp) exactly, one tier over.
bool DrawRuleLayerHeaderNameOverlay(int layerIndex, Params::MarkerRuleLayer& layer,
                                    MarkerLayerBundlesState& state, Pipeline::PreviewDriver* previewDriver) {
    const ImVec2 itemMin = ImGui::GetItemRectMin();
    const float labelStartX = itemMin.x + ImGui::GetTreeNodeToLabelSpacing();

    if (state.renamingProceduralLayerIndex == layerIndex) {
        ImGui::SetCursorScreenPos(ImVec2(labelStartX, itemMin.y));
        if (state.bRenameProceduralFocusPending) {
            ImGui::SetKeyboardFocusHere();
            state.bRenameProceduralFocusPending = false;
        }
        TextInputRules nameRules;
        nameRules.maximumLength = 48; nameRules.bAllowEmpty = false; nameRules.fallbackText = "Marker Layer";
        DrawTextInput("##renameRuleLayer", state.renameProceduralScratchText, nameRules, WidgetStyle(), nullptr,
                     /*bLabelHidden=*/true);
        if (ImGui::IsItemDeactivated()) {
            layer.name = SanitizeTextInput(state.renameProceduralScratchText, nameRules);
            state.renamingProceduralLayerIndex = -1;
            NotifyPlacementChange(true, previewDriver);
        }
        return true;
    }

    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        state.renamingProceduralLayerIndex  = layerIndex;
        state.renameProceduralScratchText   = layer.name;
        state.bRenameProceduralFocusPending = true;
        return true;
    }
    return false;
}

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
    // DrawMarkerGroupLeafHeaderExtra's own Manual-leaf drop target already follows). Lands on the
    // Group's first Manual Layer when one exists; when FirstManualLayerIndexInBundle returns -1
    // (the Group has none yet), STEP148's correction creates one instead of no-op-ing — recorded
    // into `state`'s pending-create fields (a structural instanceLayers mutation, unsafe mid-walk,
    // MarkerLayerBundlesState's own comment) for the caller to apply after the walk finishes.
    const int firstLayerIndex = FirstManualLayerIndexInBundle(bundleIdentifier, instanceLayers);
    if (firstLayerIndex >= 0) {
        DrawManualLayerInstanceDropTarget(firstLayerIndex, markers, selectedManualInstanceIdentifiers);
    } else {
        const std::vector<int> droppedIdentifiers =
            DetectManualInstanceDropTarget(selectedManualInstanceIdentifiers);
        if (!droppedIdentifiers.empty()) {
            for (const Params::MarkerLayerBundle& bundle : bundles)
                if (bundle.identifier == bundleIdentifier) {
                    state.pendingCreateLayerForBundleIdentifier = bundleIdentifier;
                    state.pendingCreateLayerMarkerTypeName      = bundle.markerTypeName;
                    state.pendingCreateLayerInstanceIdentifiers = droppedIdentifiers;
                    break;
                }
        }
    }

    const ImVec2 itemMin = ImGui::GetItemRectMin();
    const float labelStartX = itemMin.x + ImGui::GetTreeNodeToLabelSpacing();

    if (state.renamingBundleIdentifier == bundleIdentifier) {
        for (Params::MarkerLayerBundle& bundle : bundles) {
            if (bundle.identifier != bundleIdentifier) continue;
            ImGui::SetCursorScreenPos(ImVec2(labelStartX, itemMin.y));
            if (state.bRenameFocusPending) { ImGui::SetKeyboardFocusHere(); state.bRenameFocusPending = false; }
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

    // STEP241, ARCH §19.31 correction: Name is now read-and-resolve at the Bundle tier too — while
    // this Group is Link-bound, a double-click never starts a rename (the tree's own displayed node
    // label is already the Link-resolved name, EffectiveMarkerLayerBundleName, drawn by
    // DrawMarkerLayerBundleTree's own node-label lambda). A field-local check only, no `links`
    // parameter needed here.
    bool bLinked = false;
    for (const Params::MarkerLayerBundle& bundle : bundles)
        if (bundle.identifier == bundleIdentifier) { bLinked = bundle.linkIdentifier >= 0; break; }

    if (!bLinked && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        state.renamingBundleIdentifier = bundleIdentifier;
        state.bRenameFocusPending      = true;
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
                                    Pipeline::PreviewDriver* previewDriver, bool& bAnyCommitted,
                                    const std::vector<Params::MarkerLink>& links) {
    if (leaf.kind == MarkerGroupLeafKey_UI::Kind::Manual) {
        DrawManualLayerInstanceDropTarget(leaf.layerIndex, markers, selectedManualInstanceIdentifiers);
        if (leaf.layerIndex < 0 || leaf.layerIndex >= static_cast<int>(instanceLayers.size())) return;
        Params::MarkerInstanceLayer& layer = instanceLayers[static_cast<std::size_t>(leaf.layerIndex)];
        // STEP142 — double-click-the-header rename FIRST: while active, it claims the rest of the
        // row (the name box fills whatever's left), so SYM/COL/V-I/X don't draw this frame.
        if (DrawLayerHeaderNameOverlay(leaf.layerIndex, layer, manualLayersState, bAnyCommitted)) return;
        // Human's own bug report — Icon Size/Snap to Grid now live in the header, mirroring how
        // SYM/COL already do (left of them, same [SYM][COL][swatch][V/I][X] cluster convention).
        // STEP241, ARCH §19.31 correction: `links` threads through to every one of these now —
        // widened from STEP239's color-only treatment.
        DrawMarkerLayerIconSizeHeaderControl(layer, manualLayersState, bAnyCommitted, links);
        ImGui::SameLine();
        DrawMarkerLayerGridSnapHeaderControl(layer, manualLayersState, bAnyCommitted, links);
        ImGui::SameLine();
        DrawMarkerLayerSymmetryToggleHeaderControl(layer, bAnyCommitted, links);
        ImGui::SameLine();
        DrawManualMarkerLayerColorOverrideHeaderControl(layer, manualLayersState, bAnyCommitted, links);
        ImGui::SameLine();
        // STEP144 -> STEP241/ARCH §19.31 correction: bHidden IS now read-and-resolved from a bound
        // Link, exactly like every other governed field (retracts STEP239's "no such field exists on
        // Params::MarkerLink... stays independently editable" text) — the toggle goes inert while
        // linked and its displayed icon reflects the RESOLVED value, never `layer.bHidden` directly.
        {
            const bool bHiddenLinked = layer.linkIdentifier >= 0;
            const bool bEffectiveHidden = EffectiveManualMarkerLayerHidden(layer, links);
            ImGui::BeginDisabled(bHiddenLinked);
            if (ImGui::SmallButton(bEffectiveHidden ? "I##visible" : "V##visible")) {
                layer.bHidden = !layer.bHidden;
                bAnyCommitted = true;
            }
            ImGui::EndDisabled();
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Visible / Invisible in preview");
        }
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
    Params::MarkerRuleLayer& ruleLayer = ruleLayers[static_cast<std::size_t>(leaf.layerIndex)];
    // Human's own bug report — double-click-the-header rename FIRST, mirroring the Manual leaf
    // branch above exactly: while active, it claims the rest of the row.
    if (DrawRuleLayerHeaderNameOverlay(leaf.layerIndex, ruleLayer, bundlesState, previewDriver)) return;
    DrawRightAlignedProceduralLayerCluster(ruleLayer, leaf.layerIndex, bundlesState, previewDriver,
                                           kMarkerLayerHeaderExtraCombinedWidthPixels);
}

} // namespace Ui
} // namespace SanmapGen
