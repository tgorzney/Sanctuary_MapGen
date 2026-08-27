// MarkersTab_ManualLayerRowBody_UI.cpp — DrawLayerRowBody, the aspect-split sibling of
// MarkersTab_ManualLayers_UI.cpp (ARCH §1.5 — the single-file draft crossed the 150-line hard
// ceiling once STEP120 needed this function callable from outside its own translation unit), both
// declared by MarkersTab_ManualLayers_UI.h. Mirrors MarkersTab_RuleLayers_UI.cpp/
// MarkersTab_RuleLayerSettings_UI.cpp's own established aspect split.
#include "MarkersTab_ManualLayerRowBody_UI.h"
#include "Checkbox_UI.h"
#include "SymmetryClusterInstanceList_UI.h"
#include "TextInput_UI.h"
#include "imgui.h"
#include <string>
#include <utility>

namespace SanmapGen {
namespace Ui {

// One instance row's own body — unchanged from STEP126, just extracted so both the flat-list branch
// and the (STEP132) symmetry-cluster branch draw it identically, never a second near-duplicate copy.
// STEP138: promoted out of this file's own anonymous namespace (declared in
// MarkersTab_ManualLayerRowBody_UI.h) so MarkersTab_UI.cpp's base-section instance list — the one
// "no Layer" case the current data model can represent, human's own instruction — can draw an
// identical row rather than a near-duplicate copy.
void DrawManualInstanceRow(std::vector<Params::MarkerInstanceGroup>& markers,
                           const std::pair<int, int>& groupTransformIndex,
                           ManualInstanceRowInteractionContext_UI& interaction) {
    const Params::MarkerInstanceGroup& instanceGroup =
        markers[static_cast<std::size_t>(groupTransformIndex.first)];
    const Params::MarkerTransform& instanceTransform =
        instanceGroup.transforms[static_cast<std::size_t>(groupTransformIndex.second)];
    const int instanceIdentifier = instanceTransform.instanceIdentifier;
    const std::string rowLabel = instanceGroup.name + " - " + (!instanceTransform.name.empty()
        ? instanceTransform.name : std::to_string(groupTransformIndex.second));
    const bool bRowSelected = interaction.selectedIdentifiers != nullptr
        && IsManualInstanceSelected(*interaction.selectedIdentifiers, instanceIdentifier);
    if (ImGui::Selectable(rowLabel.c_str(), bRowSelected)) {
        // STEP141 — Ctrl (toggle)/Shift (range within THIS list)/plain click, "typical expectations".
        if (interaction.selectedIdentifiers != nullptr && interaction.anchorIdentifier != nullptr
            && interaction.rowOrder != nullptr)
            ApplyManualInstanceSelectionClick(*interaction.rowOrder, instanceIdentifier,
                                              ImGui::GetIO().KeyCtrl, ImGui::GetIO().KeyShift,
                                              *interaction.selectedIdentifiers, *interaction.anchorIdentifier);
        if (interaction.primaryIdentifier != nullptr) *interaction.primaryIdentifier = instanceIdentifier;
        // ARCH §19.25, item 5 — IN ADDITION TO the tab-local write above, not instead of it:
        // drives the canvas's own real selection, so the REAL icon-sprite render path
        // (MapCanvas_IconLayer_CullEmit_UI.cpp's `instance.bSelected`) reflects this click too.
        if (interaction.selectManualMarkerInstanceCallback)
            interaction.selectManualMarkerInstanceCallback(instanceIdentifier);
    }
    // STEP141 — drag SOURCE: carries just this row's own instanceIdentifier; the RECEIVER (a Layer's
    // own drop target, DrawManualLayerInstanceDropTarget) decides whether the WHOLE multi-select
    // moves (this row is part of it) or just this one (it isn't) — the standard "drag a
    // non-selected item" convention.
    if (ImGui::BeginDragDropSource()) {
        ImGui::SetDragDropPayload("markerInstanceDrag", &instanceIdentifier, sizeof(int));
        const int movedCount = (interaction.selectedIdentifiers != nullptr
                               && IsManualInstanceSelected(*interaction.selectedIdentifiers, instanceIdentifier))
                              ? static_cast<int>(interaction.selectedIdentifiers->size()) : 1;
        ImGui::Text("Moving %d marker(s)", movedCount);
        ImGui::EndDragDropSource();
    }
}

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
                      std::vector<int>& selectedManualInstanceIdentifiers, int& anchorIdentifier,
                      const std::function<void(int)>& selectManualMarkerInstanceCallback) {
    (void)markerLayers; (void)geometry; (void)globalSymmetryMask; (void)globalRadialRepeatCount;
    (void)markerSymmetryFixSettings;
    // STEP142/human's own correction: no Name field here — double-click the header instead
    // (DrawLayerHeaderNameOverlay). No per-layer Symmetry configuration section either ("I think it
    // was designed already but ignore [per-layer] for now" — SymmetrySetting::bSymmetryUseGlobal
    // defaults true and nothing in this body flips it false anymore, so every layer stays on GLOBAL
    // symmetry whenever its own SYM toggle is on).
    DrawSliderScalar("Icon Scale", layer.iconScale, state.iconScaleRange,
                     state.selectedLayerIconScaleToggle, WidgetStyle(), "%.2f");
    const bool bSnapCommitted = DrawCheckbox("Snap to Grid", layer.bGridSnapEnabled).bCommitted;
    ImGui::BeginDisabled(!layer.bGridSnapEnabled);
    const bool bSnapSizeCommitted = DrawSliderScalar("Grid Size", layer.gridSnapSizeWorldUnits,
        state.gridSnapSizeRange, state.selectedLayerGridSnapToggle, WidgetStyle(), "%.2f").bCommitted;
    ImGui::EndDisabled();

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
        // STEP141 — this list's own display-order identifiers, for Shift-range selection.
        std::vector<int> rowOrder;
        rowOrder.reserve(instanceIt->second.size());
        for (const std::pair<int, int>& groupTransformIndex : instanceIt->second)
            rowOrder.push_back(markers[static_cast<std::size_t>(groupTransformIndex.first)]
                .transforms[static_cast<std::size_t>(groupTransformIndex.second)].instanceIdentifier);

        ManualInstanceRowInteractionContext_UI interaction;
        interaction.primaryIdentifier   = &selectedManualInstanceIdentifier;
        interaction.selectedIdentifiers = &selectedManualInstanceIdentifiers;
        interaction.anchorIdentifier    = &anchorIdentifier;
        interaction.rowOrder            = &rowOrder;
        interaction.selectManualMarkerInstanceCallback = selectManualMarkerInstanceCallback;

        DrawSymmetryClusterInstanceList<std::pair<int, int>>(instanceIt->second,
            [&](const std::pair<int, int>& groupTransformIndex) {
                return markers[static_cast<std::size_t>(groupTransformIndex.first)]
                    .transforms[static_cast<std::size_t>(groupTransformIndex.second)]
                    .symmetryGroupIdentifier;
            },
            [](int groupIdentifier, int /*bucketSize*/) { return groupIdentifier != 0; },
            [&](const std::pair<int, int>& groupTransformIndex) {
                DrawManualInstanceRow(markers, groupTransformIndex, interaction);
            });
    }
    return bSnapCommitted || bSnapSizeCommitted;
}

namespace {

// A pressed/highlighted look for an on/off SmallButton toggle — the shared visual convention every
// STEP142 button-toggle in this file uses (mirrors the ordinary ButtonActive theme color, the
// standard imgui "this is currently on" cue).
void PushToggleButtonStyle(bool bOn) {
    if (bOn) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
}
void PopToggleButtonStyle(bool bOn) {
    if (bOn) ImGui::PopStyleColor();
}

} // namespace

// STEP123/STEP142: the row header's own compact Color Override control — a "COL" SmallButton toggle
// (was a checkbox, human's own instruction: no more checkboxes) + a small inline swatch, drawn on
// EVERY row's collapsed header line via DraggableList's/TreeListWidget's header-extra slot, NOT
// gated on the row's own expand state. Disabled (not hidden) while state.bUseGroupColor forces one
// shared tint, so the header's own width never shifts when that block-wide toggle flips. STEP130:
// this is now the ONLY place Color Override draws for EITHER an ungrouped row (the STEP123
// DraggableList slot) or a bundled row (the Bundle tree's `drawLeafHeaderExtra` slot, ARCH §19.23).
void DrawManualMarkerLayerColorOverrideHeaderControl(Params::MarkerInstanceLayer& layer,
                                                      ManualMarkerLayersState& state, bool& bAnyCommitted) {
    ImGui::BeginDisabled(state.bUseGroupColor);
    PushToggleButtonStyle(layer.bColorOverrideEnabled);
    const bool bOverrideCommitted = ImGui::SmallButton("COL##colorOverride");
    PopToggleButtonStyle(layer.bColorOverrideEnabled);
    if (bOverrideCommitted) layer.bColorOverrideEnabled = !layer.bColorOverrideEnabled;
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

// STEP130/STEP142: the row header's own Symmetry-toggle control — a "SYM" SmallButton (was a plain
// checkbox), highlighted while on, mirroring the Color Override button's own empty-tooltip shape.
// Placed LEFT of Color Override at every call site. Never touches `layer.symmetry`'s own fields —
// toggling only flips the gate `ResolveEffectiveMarkerSymmetry` reads (MarkerDragGesture_UI.h), so
// re-enabling restores the prior configuration unchanged. Human's own instruction: ignore per-layer
// symmetry configuration for now — `layer.symmetry.bSymmetryUseGlobal` defaults true and nothing
// left in DrawLayerRowBody's own body flips it, so SYM=on always resolves to GLOBAL symmetry.
void DrawMarkerLayerSymmetryToggleHeaderControl(Params::MarkerInstanceLayer& layer, bool& bAnyCommitted) {
    PushToggleButtonStyle(layer.bSymmetryEnabled);
    const bool bSymmetryCommitted = ImGui::SmallButton("SYM##symmetry");
    PopToggleButtonStyle(layer.bSymmetryEnabled);
    if (bSymmetryCommitted) { layer.bSymmetryEnabled = !layer.bSymmetryEnabled; bAnyCommitted = true; }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Symmetry (global)");
}

// STEP142 — see the header's own comment (MarkersTab_ManualLayerRowBody_UI.h) for the full "why".
// `bAnyCommitted` is set true exactly when a rename COMMITS (not merely while typing) — the SAME
// signal DrawLayerRowBody's own return used to carry for the retired body Name field, so the
// caller's existing MakeNamesUnique repair (the wire format keys Layers by name) still runs.
bool DrawLayerHeaderNameOverlay(int layerIndex, Params::MarkerInstanceLayer& layer,
                                ManualMarkerLayersState& state, bool& bAnyCommitted) {
    const ImVec2 itemMin = ImGui::GetItemRectMin();
    const float labelStartX = itemMin.x + ImGui::GetTreeNodeToLabelSpacing();

    if (state.renamingLayerIndex == layerIndex) {
        ImGui::SetCursorScreenPos(ImVec2(labelStartX, itemMin.y));
        TextInputRules nameRules;
        nameRules.maximumLength = 48; nameRules.bAllowEmpty = false; nameRules.fallbackText = "Marker Layer";
        DrawTextInput("##renameLayer", state.renameScratchText, nameRules, WidgetStyle(), nullptr,
                     /*bLabelHidden=*/true);
        if (ImGui::IsItemDeactivated()) {
            layer.name = SanitizeTextInput(state.renameScratchText, nameRules);
            state.renamingLayerIndex = -1;
            bAnyCommitted = true;
        }
        return true;
    }

    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        state.renamingLayerIndex = layerIndex;
        state.renameScratchText  = layer.name;
        return true;
    }
    return false;
}

} // namespace Ui
} // namespace SanmapGen
