// MarkersTab_ManualLayers_UI.cpp — the imgui composition of the Manual Marker Layers block.
// Layer: UI. Shared widgets only: DraggableList for the layer stack, Checkbox / ColorSwatch /
// SliderScalar / TextInput / Section for the rest. Nothing here notifies
// Pipeline::PreviewDriver (MarkersTab_ManualLayers_UI.h SCOPE NOTE 3).
#include "MarkersTab_ManualLayers_UI.h"
#include "Checkbox_UI.h"
#include "MarkerLayerId_UI.h"
#include "MarkerLayerIndexRepair_UI.h"
#include "MarkersTab_ManualInstanceSelection_UI.h"
#include "MarkersTab_ManualLayerHelpers_UI.h"
#include "MarkersTab_ManualLayerRowBody_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

// A removed layer clamps every referencing `layerIndex` to 0 (ClampMarkerLayerIndicesForRemovedLayer);
// a reordered layer renumbers them (RenumberMarkerLayerIndicesForReorder, called BEFORE the layers
// vector moves). Reports whether `markers` moved, which feeds no pipeline stage (SCOPE NOTE 3).
bool ApplyLayerListSignal(std::vector<Params::MarkerInstanceLayer>& markerLayers,
                         std::vector<Params::MarkerInstanceGroup>& markers,
                         const ManualInstanceLayerIndex_UI& instanceIndex, ManualMarkerLayersState& state,
                         int& selectedManualInstanceIdentifier, std::vector<int>& selectedManualInstanceIdentifiers,
                         int& anchorIdentifier, const DraggableListSignal& signal) {
    if (signal.kind == DraggableListSignalKind::Select) {
        state.selectedLayerIndex = signal.sourceRowIndex;
        // Human's own bug report — mirrors the Bundle tree's own Leaf-select branch
        // (MarkersTab_BundleTreeSignals_UI.cpp): a single click on a Layer header also selects every
        // Instance it owns.
        selectedManualInstanceIdentifiers.clear();
        const auto memberIt = instanceIndex.instancesByLayerIndex.find(signal.sourceRowIndex);
        if (memberIt != instanceIndex.instancesByLayerIndex.end())
            for (const std::pair<int, int>& groupTransformIndex : memberIt->second)
                selectedManualInstanceIdentifiers.push_back(
                    markers[static_cast<std::size_t>(groupTransformIndex.first)]
                        .transforms[static_cast<std::size_t>(groupTransformIndex.second)].instanceIdentifier);
        anchorIdentifier = selectedManualInstanceIdentifiers.empty()
                          ? -1 : selectedManualInstanceIdentifiers.front();
        selectedManualInstanceIdentifier = anchorIdentifier;
        return false;
    }
    if (signal.kind == DraggableListSignalKind::ToggleLock) {
        if (signal.sourceRowIndex >= 0 && signal.sourceRowIndex < static_cast<int>(markerLayers.size()))
            markerLayers[static_cast<std::size_t>(signal.sourceRowIndex)].bLocked =
                !markerLayers[static_cast<std::size_t>(signal.sourceRowIndex)].bLocked;
        return false;   // cosmetic-only: no structural move, same posture as Select
    }
    // STEP144 — the row's own built-in visibility icon now does something: MarkerInstanceLayer::
    // bHidden (new field), a straight V/I toggle (no E/D concept for a hand-placed Manual layer, so
    // no coupling the way the Procedural sibling needs — MarkerLayerEnabledVisibilityToggle_UI.h).
    if (signal.kind == DraggableListSignalKind::ToggleVisibility) {
        if (signal.sourceRowIndex >= 0 && signal.sourceRowIndex < static_cast<int>(markerLayers.size()))
            markerLayers[static_cast<std::size_t>(signal.sourceRowIndex)].bHidden =
                !markerLayers[static_cast<std::size_t>(signal.sourceRowIndex)].bHidden;
        return false;   // cosmetic-only: no structural move, same posture as ToggleLock
    }
    const bool bDeleting            = signal.kind == DraggableListSignalKind::Delete;
    const bool bReordering          = signal.kind == DraggableListSignalKind::Reorder;
    const int  sourceLayerIndex     = signal.sourceRowIndex;
    const int  layerCountBeforeMove = static_cast<int>(markerLayers.size());
    bool bMarkersMoved = bReordering && RenumberMarkerLayerIndicesForReorder(
        markers, sourceLayerIndex, signal.targetRowIndex, layerCountBeforeMove);
    if (!ApplyDraggableListSignal(markerLayers, signal)) return bMarkersMoved;
    if (bDeleting)
        bMarkersMoved = ClampMarkerLayerIndicesForRemovedLayer(markers, sourceLayerIndex) || bMarkersMoved;
    if (state.selectedLayerIndex >= static_cast<int>(markerLayers.size()))
        state.selectedLayerIndex = static_cast<int>(markerLayers.size()) - 1;
    return bMarkersMoved;
}

} // namespace

// STEP142/144 — [SYM][COL][swatch], right-aligned as a cluster so it sits flush against
// DraggableList's own built-in [o]/[L]/[X] strip with no dead gap — this ungrouped row draws no
// delete button of its own (that strip's "X##delete" already covers it, unlike the Bundle tree's
// Manual leaf, which has no such strip and right-aligns its own X directly,
// DrawRightAlignedDeleteButton, MarkersTab_BundleHeaderExtras_UI.cpp). STEP145: promoted out of the
// anonymous namespace — see the header's own comment (MarkersTab_ManualLayers_UI.h) for why.
void DrawRightAlignedSymmetryColorOverrideCluster(Params::MarkerInstanceLayer& layer,
                                                  ManualMarkerLayersState& state, bool& bAnyCommitted) {
    // Human's own bug report — Icon Size/Snap to Grid join this cluster, left of [SYM], same
    // right-align-against-the-reserved-zone math as before (V/I and X are NOT part of clusterWidth
    // here — DraggableList's own built-in affordance strip already draws those, see this function's
    // own header comment above).
    const float clusterWidth = kMarkerLayerIconSizeControlWidthPixels
                              + kMarkerLayerGridSizeControlWidthPixels
                              + kMarkerLayerSymmetryButtonWidthPixels
                              + kMarkerLayerColorOverrideButtonWidthPixels
                              + kMarkerLayerColorOverrideSwatchWidthPixels;
    // Right-align within the row's own FIXED header-extra budget, not GetContentRegionAvail(): the
    // live content region reaches all the way to the row's TRUE right edge, which is PAST the
    // built-in [o]/[L]/[X] strip's own reserved kAffordanceStripWidthPixels (DrawRowAffordances,
    // DraggableListWidget_RowAffordances_UI.h) — using it here pushed this cluster 84px too far
    // right, landing it squarely on top of the strip (the human's own "buttons ... overlapping each
    // other" report). kMarkerLayerHeaderExtraCombinedWidthPixels IS this row's own reserved zone
    // width (the same fixed budget DraggableList<T>::Render was given as headerExtraWidthPixels), so
    // right-aligning against IT lands this cluster flush against the strip with no gap and no overlap.
    if (kMarkerLayerHeaderExtraCombinedWidthPixels > clusterWidth)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + kMarkerLayerHeaderExtraCombinedWidthPixels - clusterWidth);
    DrawMarkerLayerIconSizeHeaderControl(layer, state, bAnyCommitted);
    ImGui::SameLine();
    DrawMarkerLayerGridSnapHeaderControl(layer, state, bAnyCommitted);
    ImGui::SameLine();
    DrawMarkerLayerSymmetryToggleHeaderControl(layer, bAnyCommitted);
    ImGui::SameLine();
    DrawManualMarkerLayerColorOverrideHeaderControl(layer, state, bAnyCommitted);
}

// STEP125: promoted out of the anonymous namespace (was DrawLayerSettings), called exactly once,
// tab-wide, by DrawMarkerTypeSections — see MarkersTab_ManualLayers_UI.h.
void DrawManualMarkerLayerBlockSettings(ManualMarkerLayersState& state) {
    DrawCheckbox("Use Group Color", state.bUseGroupColor);
    if (state.bUseGroupColor)
        DrawColorSwatch("Group Color", state.groupColor, state.previewColorOptions,
                        state.groupColorToggle);
}

DraggableListSignal DrawLayerList(std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                  std::vector<Params::MarkerInstanceGroup>& markers,
                                  const Params::Geometry& geometry, int globalSymmetryMask, int globalRadialRepeatCount,
                                  Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings,
                                  ManualMarkerLayersState& state, bool& bAnyNameCommitted,
                                  const ManualInstanceLayerIndex_UI& instanceIndex,
                                  int& selectedManualInstanceIdentifier,
                                  std::vector<int>& selectedManualInstanceIdentifiers, int& anchorIdentifier,
                                  const std::string& markerTypeNameFilter,
                                  const std::function<void(int)>& selectManualMarkerInstanceCallback) {
    return DraggableList<Params::MarkerInstanceLayer>::Render(
        "manualMarkerLayers", markerLayers,
        [&](int rowIndex) {
            DraggableListRow row;
            row.bRowSuppressed = IsMarkerInstanceLayerRowSuppressed(
                markerLayers[static_cast<std::size_t>(rowIndex)], markerTypeNameFilter);   // CHANGED — STEP125
            row.label   = ManualMarkerLayerRowLabel(markerLayers[static_cast<std::size_t>(rowIndex)]);
            row.bLocked = markerLayers[static_cast<std::size_t>(rowIndex)].bLocked;
            // STEP144/145 — the built-in [o]/[-] icon's own displayed state: bHidden wasn't wired
            // here when the field was added, so the icon always showed "visible" regardless of the
            // real state (clicking it still correctly flipped bHidden via ApplyLayerListSignal's
            // ToggleVisibility branch, but the icon itself never reflected it back).
            row.bVisible = !markerLayers[static_cast<std::size_t>(rowIndex)].bHidden;
            return row;
        },
        [&](int rowIndex) {
            Params::MarkerInstanceLayer& layer = markerLayers[static_cast<std::size_t>(rowIndex)];
            // Human's own instruction: no "Marker Type" input anywhere — a Layer's type comes only
            // from the Type-section its own "Add Layer" button was clicked from, never re-editable.
            if (DrawLayerRowBody(layer, rowIndex, markerLayers, markers,
                                 geometry, globalSymmetryMask, globalRadialRepeatCount, markerSymmetryFixSettings,
                                 state, instanceIndex, selectedManualInstanceIdentifier,
                                 selectedManualInstanceIdentifiers, anchorIdentifier,
                                 selectManualMarkerInstanceCallback))
                bAnyNameCommitted = true;
        },
        [&](int rowIndex) {
            // STEP141 — this row's own drag-drop TARGET first (same "run before any new widget"
            // reasoning DrawMarkerGroupLeafHeaderExtra's own comment gives, MarkersTab_BundleHeaderExtras_UI.cpp).
            DrawManualLayerInstanceDropTarget(rowIndex, markers, selectedManualInstanceIdentifiers);
            Params::MarkerInstanceLayer& layer = markerLayers[static_cast<std::size_t>(rowIndex)];
            // STEP142 — double-click-the-header rename FIRST: while active it claims the row.
            if (DrawLayerHeaderNameOverlay(rowIndex, layer, state, bAnyNameCommitted)) return;
            // STEP130 (ARCH §19.24)/STEP142: [SYM][COL][swatch], right-aligned as a cluster so it
            // sits flush against DraggableList's own [o]/[L]/[X] strip with no dead gap (this row
            // draws no delete button of its own — that strip's built-in "X##delete" already covers
            // it, unlike the Bundle tree's Manual leaf, which has none and right-aligns its own X).
            DrawRightAlignedSymmetryColorOverrideCluster(layer, state, bAnyNameCommitted);
        },
        kMarkerLayerHeaderExtraCombinedWidthPixels,
        state.selectedLayerIndex);
}

// STEP120: gains an optional Bundle-scoped parent; moved out of the anonymous namespace so
// MarkersTab_Bundles_UI.cpp can call it for a non-root parent. Markers ship with `layerId` from day
// one (STEP60), unlike Props' still-unimplemented retrofit, so the fresh row mints one via
// `NextMarkerLayerId` BEFORE the push_back — that function scans for max(layerId) + 1, and calling
// it after would scan the new row's own `-1`. STEP125: gains `markerTypeNameForNewLayer`.
bool DrawLayerListButtons(std::vector<Params::MarkerInstanceLayer>& markerLayers, ManualMarkerLayersState& state,
                          int parentBundleIdentifierForNewLayer, const std::string& markerTypeNameForNewLayer) {
    if (!ImGui::Button(parentBundleIdentifierForNewLayer < 0 ? "Add Marker Layer" : "Add Manual Layer Here"))
        return false;
    Params::MarkerInstanceLayer layer;
    layer.name                   = NextMarkerLayerName(static_cast<int>(markerLayers.size()));
    layer.layerId                = NextMarkerLayerId(markerLayers);
    layer.parentBundleIdentifier = parentBundleIdentifierForNewLayer;   // STEP119 field
    layer.markerTypeName         = markerTypeNameForNewLayer;          // NEW — STEP125
    markerLayers.push_back(layer);
    state.selectedLayerIndex = static_cast<int>(markerLayers.size()) - 1;
    return true;
}

void DrawManualMarkerLayerListBody(ManualMarkerLayersState& state,
                                   std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                   std::vector<Params::MarkerInstanceGroup>& markers,
                                   const Params::Geometry& geometry, int globalSymmetryMask,
                                   int globalRadialRepeatCount,
                                   Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings,
                                   const std::string& markerTypeNameFilter,
                                   int& selectedManualInstanceIdentifier,
                                   std::vector<int>& selectedManualInstanceIdentifiers, int& anchorIdentifier,
                                   const std::function<void(int)>& selectManualMarkerInstanceCallback) {
    // STEP138/human's own correction: no "Add Marker Layer" button here — fully redundant with the
    // Type-section header's own "+ Layer" (MarkersTab_UI.cpp), and drawing both produced the same
    // confusing double-add the header's "+ Group" duplicate did.
    bool bLayersMoved = false;
    bool bAnyNameCommitted = false;
    const ManualInstanceLayerIndex_UI instanceIndex = BuildManualInstanceLayerIndex(markers);
    const DraggableListSignal signal = DrawLayerList(markerLayers, markers, geometry, globalSymmetryMask,
        globalRadialRepeatCount, markerSymmetryFixSettings, state, bAnyNameCommitted,
        instanceIndex, selectedManualInstanceIdentifier,
        selectedManualInstanceIdentifiers, anchorIdentifier, markerTypeNameFilter,
        selectManualMarkerInstanceCallback);
    if (signal.bHasSignal())
        ApplyLayerListSignal(markerLayers, markers, instanceIndex, state, selectedManualInstanceIdentifier,
                             selectedManualInstanceIdentifiers, anchorIdentifier, signal);
    bLayersMoved = bAnyNameCommitted || bLayersMoved;
    // The export keys layers by NAME parity with Armies/Areas/Props (cosmetic here — `MarkerGroups`
    // exports as a plain array, STEP60 §3) — the repair runs on the frames a name settled.
    if (bLayersMoved) MakeNamesUnique(markerLayers);
}

} // namespace Ui
} // namespace SanmapGen
