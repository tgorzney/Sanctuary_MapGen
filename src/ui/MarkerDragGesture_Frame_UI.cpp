// MarkerDragGesture_Frame_UI.cpp — the per-frame half of the gesture state machine:
// UpdateMarkerDragGesture (every drag frame) and EndMarkerDragGesture (mouse-up). See
// MarkerDragGesture_UI.cpp for BeginMarkerDragGesture/RepositionSymmetryGroupMember and the shared
// header comment (ARCH §1.5 split, same posture as MarkersTab_Manual_UI.h/.cpp's own two-file split).
#include "MarkerDragGesture_UI.h"
#include <algorithm>

namespace SanmapGen {
namespace Ui {

void UpdateMarkerDragGesture(MarkerDragGestureState& state, std::vector<Params::MarkerInstanceGroup>& markers,
                             const Params::Geometry& geometry, float newWorldX, float newWorldZ) {
    state.bSpawnCardinalityRefused = false;
    state.bCardinalityGrew         = false;
    state.bCardinalityShrank       = false;
    state.currentGhostPoints.clear();
    if (!state.bActive) return;
    Params::MarkerInstanceGroup* const group = SelectedMarkerGroup(markers, state.groupIndex);
    if (group == nullptr) { state.bActive = false; return; }
    Params::MarkerTransform* const dragged = SelectedMarkerInstance(group->transforms, state.draggedTransformIndex);
    if (dragged == nullptr) { state.bActive = false; return; }

    if (state.symmetryGroupIdentifier == 0) {           // ungrouped: free drag, zero orbit calls
        dragged->transform.positionX = newWorldX;
        dragged->transform.positionZ = newWorldZ;
        return;
    }

    Pipeline::WorldSymmetryOrbitPoint orbitPoints[Params::symmetryOrbitMaximum];
    const int orbitCount = Pipeline::BuildWorldSymmetryOrbit(
        geometry, state.effectiveSymmetryMask, state.effectiveRadialRepeatCount,
        newWorldX, newWorldZ, orbitPoints, Params::symmetryOrbitMaximum);
    const bool bCardinalityChanged = (orbitCount != state.gestureStartOrbitCount);

    if (state.bSpawnGroup && bCardinalityChanged) {
        state.bSpawnCardinalityRefused = true;           // whole group frozen at the last valid frame
        return;
    }

    dragged->transform.positionX = newWorldX;             // unambiguous regardless of cardinality
    dragged->transform.positionZ = newWorldZ;

    std::vector<int> unclaimedSlots;
    MatchCorrespondenceToOrbit(state.correspondence, orbitPoints, orbitCount, &unclaimedSlots);
    for (MarkerOrbitCorrespondence& entry : state.correspondence) {
        entry.bSoftHidden = (entry.lastMatchedOrbitSlot < 0);
        if (bCardinalityChanged || entry.lastMatchedOrbitSlot < 0) continue;   // frozen this frame
        Params::MarkerTransform* const sibling = SelectedMarkerInstance(group->transforms, entry.transformIndex);
        if (sibling == nullptr) continue;
        sibling->transform.positionX = orbitPoints[entry.lastMatchedOrbitSlot].worldPositionX;
        sibling->transform.positionZ = orbitPoints[entry.lastMatchedOrbitSlot].worldPositionZ;
        entry.referenceWorldX = sibling->transform.positionX;   // keep the match anchor fresh
        entry.referenceWorldZ = sibling->transform.positionZ;
    }
    for (int slotIndex : unclaimedSlots) state.currentGhostPoints.push_back(orbitPoints[slotIndex]);

    state.bCardinalityGrew   = bCardinalityChanged && orbitCount > state.gestureStartOrbitCount;
    state.bCardinalityShrank = bCardinalityChanged && orbitCount < state.gestureStartOrbitCount;
    state.pendingOrbitCount  = orbitCount;
    if (!bCardinalityChanged) {
        state.lastValidOrbitCount    = orbitCount;
        state.lastValidDraggedWorldX = newWorldX;
        state.lastValidDraggedWorldZ = newWorldZ;
    }
}

void EndMarkerDragGesture(MarkerDragGestureState& state, std::vector<Params::MarkerInstanceGroup>& markers,
                          const Params::Geometry& geometry) {
    if (!state.bActive) return;
    state.bActive = false;
    if (state.symmetryGroupIdentifier == 0) return;   // ungrouped: nothing structural, zero orbit calls
    Params::MarkerInstanceGroup* const group = SelectedMarkerGroup(markers, state.groupIndex);
    if (group == nullptr) return;
    Params::MarkerTransform* const dragged = SelectedMarkerInstance(group->transforms, state.draggedTransformIndex);
    if (dragged == nullptr) return;
    // Cached BEFORE any erase/push_back below: `dragged` is a raw pointer into `group->transforms`,
    // and both the cascade-delete (erase shifts/destroys tail elements) and the materialize loop
    // (push_back can reallocate) can invalidate it — reading through it afterward would be undefined
    // behavior. The two scalars actually needed post-mutation are captured here instead.
    const float draggedFinalPositionY = dragged->transform.positionY;
    const int   draggedLayerIndex     = dragged->layerIndex;

    Pipeline::WorldSymmetryOrbitPoint orbitPoints[Params::symmetryOrbitMaximum];
    const int finalCount = Pipeline::BuildWorldSymmetryOrbit(
        geometry, state.effectiveSymmetryMask, state.effectiveRadialRepeatCount,
        dragged->transform.positionX, dragged->transform.positionZ, orbitPoints, Params::symmetryOrbitMaximum);

    std::vector<int> unclaimedSlots;
    MatchCorrespondenceToOrbit(state.correspondence, orbitPoints, finalCount, &unclaimedSlots);
    // Settle every matched sibling to the true final orbit now — covers both an ordinary release
    // and "the drag passed through a collapse and came back out" (R2 §2: nothing structural, only
    // the per-frame writes apply).
    for (const MarkerOrbitCorrespondence& entry : state.correspondence) {
        if (entry.lastMatchedOrbitSlot < 0) continue;
        Params::MarkerTransform* const sibling = SelectedMarkerInstance(group->transforms, entry.transformIndex);
        if (sibling == nullptr) continue;
        sibling->transform.positionX = orbitPoints[entry.lastMatchedOrbitSlot].worldPositionX;
        sibling->transform.positionZ = orbitPoints[entry.lastMatchedOrbitSlot].worldPositionZ;
    }
    if (finalCount == state.gestureStartOrbitCount) return;   // no structural change after all

    // Cascade-delete every still-unmatched sibling, descending index order so an earlier erase
    // never invalidates a later one still to be removed (R1's machinery, carried forward by R2).
    std::vector<int> deleteIndices;
    for (const MarkerOrbitCorrespondence& entry : state.correspondence)
        if (entry.lastMatchedOrbitSlot < 0) deleteIndices.push_back(entry.transformIndex);
    std::sort(deleteIndices.begin(), deleteIndices.end(), [](int a, int b) { return a > b; });
    for (int deleteIndex : deleteIndices)
        if (deleteIndex >= 0 && deleteIndex < static_cast<int>(group->transforms.size()))
            group->transforms.erase(group->transforms.begin() + deleteIndex);

    // Materialize every unclaimed slot as a brand-new sibling — same group id, the dragged member's
    // own layer, STEP49's own "Add Instance" naming convention (R1's machinery, carried forward).
    for (int slotIndex : unclaimedSlots) {
        Params::MarkerTransform materialized;
        materialized.name = NextMarkerInstanceName(static_cast<int>(group->transforms.size()));
        materialized.transform.positionX = orbitPoints[slotIndex].worldPositionX;
        materialized.transform.positionZ = orbitPoints[slotIndex].worldPositionZ;
        materialized.transform.positionY = draggedFinalPositionY;   // never RE-touched elsewhere;
                                                                     // a fresh instance still needs one
        materialized.layerIndex               = draggedLayerIndex;
        materialized.symmetryGroupIdentifier  = state.symmetryGroupIdentifier;
        group->transforms.push_back(materialized);
    }
    MakeNamesUnique(group->transforms);
}

} // namespace Ui
} // namespace SanmapGen
