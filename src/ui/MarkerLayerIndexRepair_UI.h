// MarkerLayerIndexRepair_UI.h — the `MarkerTransform::layerIndex` repairs the Manual Marker
// Layers list runs on delete/reorder. Layer: UI. Pure, imgui-free, testable without a window —
// same headless posture as MarkerLayerId_UI.h (STEP60 §2) and UniqueNameList_UI.h.
// Both functions touch ONLY `layerIndex` (plain vector position). `MarkerInstanceLayer::layerId`
// is stable identity and is NEVER renumbered here (ARCH_14_13_OpenItems.md §14.13 item 3
// Work-Order A; STEP60 §2). Split out of MarkersTab_ManualLayers_UI.h to keep that header under
// the ARCH_01_05_FileSizeCeilings.md §1.5 soft-100 ceiling (STEP81 file-size ruling) — logic is a
// type-substituted mirror of PropsTab_Manual_UI.h's ClampPropLayerIndicesForRemovedLayer/
// RenumberPropLayerIndicesForReorder, not a re-derivation.
#pragma once
#include <vector>
#include "../params/MarkerInstance_PARAMS.h"

namespace SanmapGen {
namespace Ui {

// A removed layer CLAMPS every referencing transform to layer 0 rather than dropping the marker —
// the same deliberate divergence from `DropUnitRulesForRemovedArmy` that STEP22 ruling #5 made for
// props (`PropsTab_Manual_UI.h:92-97`): a marker losing its layer tag is still a real marker.
// Matches the clamp-to-0-on-out-of-range semantic STEP60 §4's import-side `ClampMarkerLayerIndex`
// already applies. Reports whether the recipe moved.
inline bool ClampMarkerLayerIndicesForRemovedLayer(std::vector<Params::MarkerInstanceGroup>& markers,
                                                    int removedLayerIndex) {
    if (removedLayerIndex < 0) return false;
    bool bRecipeMoved = false;
    for (auto& group : markers)
        for (auto& transform : group.transforms) {
            if (transform.layerIndex == removedLayerIndex)      { transform.layerIndex = 0; bRecipeMoved = true; }
            else if (transform.layerIndex > removedLayerIndex)  { --transform.layerIndex;   bRecipeMoved = true; }
        }
    return bRecipeMoved;
}

// The Reorder-signal counterpart: keeps every transform's `layerIndex` correct after
// `recipe.markerLayers` is reordered source -> target (the same erase-then-insert move
// `ApplyDraggableListSignal` performs, `DraggableListWidget_UI.h:42-60`). Identical shape/math to
// `RenumberPropLayerIndicesForReorder` (`PropsTab_Manual_UI.h:115-133`).
inline bool RenumberMarkerLayerIndicesForReorder(std::vector<Params::MarkerInstanceGroup>& markers,
                                                  int sourceLayerIndex, int targetLayerIndex,
                                                  int layerCount) {
    if (sourceLayerIndex < 0 || sourceLayerIndex >= layerCount) return false;
    int clampedTarget = targetLayerIndex;
    if (clampedTarget < 0) clampedTarget = 0;
    if (clampedTarget > layerCount - 1) clampedTarget = layerCount - 1;
    if (clampedTarget == sourceLayerIndex) return false;
    bool bRecipeMoved = false;
    for (auto& group : markers)
        for (auto& transform : group.transforms) {
            if (transform.layerIndex == sourceLayerIndex) { transform.layerIndex = clampedTarget; bRecipeMoved = true; }
            else if (sourceLayerIndex < clampedTarget && transform.layerIndex > sourceLayerIndex
                     && transform.layerIndex <= clampedTarget) { --transform.layerIndex; bRecipeMoved = true; }
            else if (sourceLayerIndex > clampedTarget && transform.layerIndex >= clampedTarget
                     && transform.layerIndex < sourceLayerIndex) { ++transform.layerIndex; bRecipeMoved = true; }
        }
    return bRecipeMoved;
}

} // namespace Ui
} // namespace SanmapGen
