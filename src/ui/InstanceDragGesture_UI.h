// InstanceDragGesture_UI.h — the generic canvas drag-and-follow gesture for manually-placed,
// symmetry-grouped instances (Markers, Props, Decals). Layer: UI. ARCH §21.3 — genericized from
// STEP94's Marker-only MarkerDragGesture_UI.h/.cpp/_Frame_UI.cpp; the algorithm is unchanged, only
// the four functions that read/write `Params::` group/transform/layer vectors became templates over
// a per-domain `Traits` policy struct. Header-only (no `.cpp` counterpart): the state struct itself
// needs no template parameter at all (every field is a plain int/float/bool/
// vector<InstanceOrbitCorrespondence> — confirmed by direct read of the original
// MarkerDragGestureState), and the four template functions below, plus their one internal helper,
// are impossible to split into a separately-compiled `.cpp` without an explicit-instantiation list
// to keep in sync by hand — the same tradeoff `TreeListWidget_UI.h` (this directory's own generic
// widget) already makes the same way, header-only, no companion `.cpp`.
//
// Pure, imgui-free, testable with no window — same posture as the file this replaces. Mask
// resolution is deliberately NOT `ResolvedPlacementSymmetryMask`/`DrawPlacementSymmetryAxes`
// (`PlacementRuleSections_UI.h:56-61`) — that widget ANDs a mask against only a 4-entry axis table
// and silently drops `Params::SymmetryAxis::Radial` (bit 4). Each domain's `Traits::
// ResolveEffectiveSymmetry` reads the already-resolved, already-valid layer/global fields directly;
// the Radial-dropping defect is structurally unreachable from this file, not merely avoided by
// discipline.
#pragma once
#include <algorithm>
#include <vector>
#include "InstanceOrbitCorrespondence_UI.h"
#include "../params/Geometry_PARAMS.h"
#include "../params/Symmetry_PARAMS.h"
#include "../pipeline/SymmetryOrbitQuery_PIPELINE.h"

namespace SanmapGen {
namespace Ui {

// A domain with no Link concept (Props, Decals) instantiates every `Traits::Link`-shaped parameter
// with this permanently-empty, never-populated, never-read placeholder — shared so Props/Decals
// don't each invent their own dummy type for the same purpose. ARCH §21.9.
struct NoInstanceLink {};

// One gesture's full state: which member is being dragged, its frozen mask/count snapshot, the
// one-shot correspondence table (built once, at BeginInstanceDragGesture, never rebuilt), and this
// frame's ghost/refusal bookkeeping for the draw pass. Shared VERBATIM by every domain — no template
// parameter (ARCH §21.3's own refinement 1: every field here is already domain-neutral).
struct InstanceDragGestureState {
    bool bActive                = false;
    int  groupIndex              = -1;   // instances[groupIndex] — the dragged member's own group
    int  draggedTransformIndex   = -1;   // instances[groupIndex].transforms[...]
    int  symmetryGroupIdentifier = 0;    // snapshot at gesture-start; frozen for the gesture. 0 = ungrouped
    int  effectiveSymmetryMask   = 0;    // snapshot — Traits::ResolveEffectiveSymmetry, gesture-start
    int  effectiveRadialRepeatCount = 0; // snapshot
    bool bCardinalityFrozen      = false; // snapshot: Traits::IsCardinalityFrozenGroup(group) — renamed
                                           // from MarkerDragGestureState::bSpawnGroup (ARCH §21.3)

    std::vector<InstanceOrbitCorrespondence> correspondence;   // built once; never rebuilt mid-drag
    int  gestureStartOrbitCount  = 0;    // cardinality at gesture-start — the comparison baseline

    int   lastValidOrbitCount    = 0;    // cardinality as of the last cardinality-matching frame
    float lastValidDraggedWorldX = 0.0f; // dragged member's position as of that same frame —
    float lastValidDraggedWorldZ = 0.0f; // cardinality-frozen refusal freezes here (R2 §3)

    bool  bSpawnCardinalityRefused = false; // this frame's UI-feedback flag
    bool  bCardinalityGrew         = false; // this frame: unclaimed orbit slots are pending ghosts
    bool  bCardinalityShrank       = false; // this frame: unmatched entries are pending cascade-deletes
    int   pendingOrbitCount        = 0;     // this frame's freshly recomputed orbit count
    std::vector<Pipeline::WorldSymmetryOrbitPoint> currentGhostPoints; // this frame's unclaimed slots, for the draw pass only
};

// True when `transformIndex` (within the gesture's own group) is this frame's soft-hidden (an
// existing sibling the current orbit no longer produces — collapsed, not erased). Used by the draw
// pass; false whenever no gesture is active or the index belongs to a different group.
inline bool IsInstanceSoftHiddenThisFrame(const InstanceDragGestureState& state, int groupIndex,
                                          int transformIndex) {
    if (!state.bActive || state.groupIndex != groupIndex) return false;
    for (const InstanceOrbitCorrespondence& entry : state.correspondence)
        if (entry.transformIndex == transformIndex) return entry.bSoftHidden;
    return false;
}

namespace Detail {

// Every sibling in `group` sharing `symmetryGroupIdentifier` (excluding `excludedTransformIndex`,
// the dragged/moved member itself), seeded with its OWN current position as the match reference —
// exact-value equality with the freshly-built orbit by the gesture-start proof (R2 §1).
template<typename Traits>
std::vector<InstanceOrbitCorrespondence> BuildInstanceCorrespondenceSeed(
        const typename Traits::Group& group, int symmetryGroupIdentifier, int excludedTransformIndex) {
    std::vector<InstanceOrbitCorrespondence> correspondence;
    for (int index = 0; index < static_cast<int>(group.transforms.size()); ++index) {
        if (index == excludedTransformIndex) continue;
        const typename Traits::Transform& sibling = group.transforms[static_cast<std::size_t>(index)];
        if (sibling.symmetryGroupIdentifier != symmetryGroupIdentifier) continue;
        InstanceOrbitCorrespondence entry;
        entry.transformIndex  = index;
        entry.referenceWorldX = sibling.transform.positionX;
        entry.referenceWorldZ = sibling.transform.positionZ;
        correspondence.push_back(entry);
    }
    return correspondence;
}

} // namespace Detail

// Mouse-down: hit-testing already happened (ManualInstanceHitTest_UI.h); this only builds the
// gesture state from the resolved (groupIndex, transformIndex). A `symmetryGroupIdentifier == 0` hit
// still returns true (a free, unrestricted drag) with an empty correspondence table and NO
// `Pipeline::BuildWorldSymmetryOrbit` call. Returns false (state left inactive) for an out-of-range
// group/transform index, or an effectively-locked instance (`Traits::IsInstanceEffectivelyLocked` —
// ARCH §21.9, checks the instance's own Link before its owning layer).
template<typename Traits>
bool BeginInstanceDragGesture(InstanceDragGestureState& state,
                              const std::vector<typename Traits::Group>& instances,
                              const std::vector<typename Traits::Layer>& layers,
                              const std::vector<typename Traits::Link>& links,
                              const Params::Geometry& geometry, int globalSymmetryMask,
                              int globalRadialRepeatCount, int groupIndex, int transformIndex) {
    state = InstanceDragGestureState{};
    if (groupIndex < 0 || groupIndex >= static_cast<int>(instances.size())) return false;
    const typename Traits::Group& group = instances[static_cast<std::size_t>(groupIndex)];
    if (transformIndex < 0 || transformIndex >= static_cast<int>(group.transforms.size())) return false;
    const typename Traits::Transform& dragged = group.transforms[static_cast<std::size_t>(transformIndex)];
    if (Traits::IsInstanceEffectivelyLocked(layers, dragged, links)) return false;

    state.bActive               = true;
    state.groupIndex            = groupIndex;
    state.draggedTransformIndex = transformIndex;
    state.symmetryGroupIdentifier = dragged.symmetryGroupIdentifier;
    state.bCardinalityFrozen    = Traits::IsCardinalityFrozenGroup(group);
    state.lastValidDraggedWorldX = dragged.transform.positionX;
    state.lastValidDraggedWorldZ = dragged.transform.positionZ;

    if (state.symmetryGroupIdentifier == 0) {          // ungrouped: free drag, zero orbit calls
        state.gestureStartOrbitCount = 1;
        state.lastValidOrbitCount    = 1;
        return true;
    }

    Traits::ResolveEffectiveSymmetry(layers, dragged, links, globalSymmetryMask,
                                     globalRadialRepeatCount, state.effectiveSymmetryMask,
                                     state.effectiveRadialRepeatCount);

    Pipeline::WorldSymmetryOrbitPoint orbitPoints[Params::symmetryOrbitMaximum];
    const int orbitCount = Pipeline::BuildWorldSymmetryOrbit(
        geometry, state.effectiveSymmetryMask, state.effectiveRadialRepeatCount,
        dragged.transform.positionX, dragged.transform.positionZ, orbitPoints, Params::symmetryOrbitMaximum);

    state.correspondence =
        Detail::BuildInstanceCorrespondenceSeed<Traits>(group, state.symmetryGroupIdentifier, transformIndex);
    MatchCorrespondenceToOrbit(state.correspondence, orbitPoints, orbitCount, nullptr);
    state.gestureStartOrbitCount = orbitCount;
    state.lastValidOrbitCount    = orbitCount;
    return true;
}

// One drag frame: writes the dragged member's position always (unambiguous, follows the cursor);
// writes every matched sibling's position too UNLESS this frame's orbit cardinality differs from
// `gestureStartOrbitCount` (R2 §2's structural-changes-wait-for-release rule) — a cardinality-frozen
// group additionally freezes the dragged member itself and writes nothing at all for a
// cardinality-changing frame (R2 §3). `newWorldX`/`newWorldZ` are the cursor's CURRENT world
// position. No-op if `state` is not active.
template<typename Traits>
void UpdateInstanceDragGesture(InstanceDragGestureState& state, std::vector<typename Traits::Group>& instances,
                               const std::vector<typename Traits::Layer>& layers,
                               const std::vector<typename Traits::Link>& links,
                               const Params::Geometry& geometry, float newWorldX, float newWorldZ) {
    state.bSpawnCardinalityRefused = false;
    state.bCardinalityGrew         = false;
    state.bCardinalityShrank       = false;
    state.currentGhostPoints.clear();
    if (!state.bActive) return;
    typename Traits::Group* const group = Traits::SelectedGroup(instances, state.groupIndex);
    if (group == nullptr) { state.bActive = false; return; }
    typename Traits::Transform* const dragged =
        Traits::SelectedInstance(group->transforms, state.draggedTransformIndex);
    if (dragged == nullptr) { state.bActive = false; return; }

    if (state.symmetryGroupIdentifier == 0) {           // ungrouped: free drag, zero orbit calls
        float quantizedX = newWorldX, quantizedZ = newWorldZ;
        Traits::QuantizePositionToLayerGrid(layers, *dragged, links, quantizedX, quantizedZ);
        dragged->transform.positionX = quantizedX;
        dragged->transform.positionZ = quantizedZ;
        return;
    }

    Pipeline::WorldSymmetryOrbitPoint orbitPoints[Params::symmetryOrbitMaximum];
    const int orbitCount = Pipeline::BuildWorldSymmetryOrbit(
        geometry, state.effectiveSymmetryMask, state.effectiveRadialRepeatCount,
        newWorldX, newWorldZ, orbitPoints, Params::symmetryOrbitMaximum);
    const bool bCardinalityChanged = (orbitCount != state.gestureStartOrbitCount);

    if (state.bCardinalityFrozen && bCardinalityChanged) {
        state.bSpawnCardinalityRefused = true;           // whole group frozen at the last valid frame
        return;
    }

    {
        float quantizedDraggedX = newWorldX, quantizedDraggedZ = newWorldZ;
        Traits::QuantizePositionToLayerGrid(layers, *dragged, links, quantizedDraggedX, quantizedDraggedZ);
        dragged->transform.positionX = quantizedDraggedX;  // unambiguous regardless of cardinality
        dragged->transform.positionZ = quantizedDraggedZ;
    }

    std::vector<int> unclaimedSlots;
    MatchCorrespondenceToOrbit(state.correspondence, orbitPoints, orbitCount, &unclaimedSlots);
    for (InstanceOrbitCorrespondence& entry : state.correspondence) {
        entry.bSoftHidden = (entry.lastMatchedOrbitSlot < 0);
        if (bCardinalityChanged || entry.lastMatchedOrbitSlot < 0) continue;   // frozen this frame
        typename Traits::Transform* const sibling =
            Traits::SelectedInstance(group->transforms, entry.transformIndex);
        if (sibling == nullptr) continue;
        float siblingX = orbitPoints[entry.lastMatchedOrbitSlot].worldPositionX;
        float siblingZ = orbitPoints[entry.lastMatchedOrbitSlot].worldPositionZ;
        Traits::QuantizePositionToLayerGrid(layers, *sibling, links, siblingX, siblingZ);
        sibling->transform.positionX = siblingX;
        sibling->transform.positionZ = siblingZ;
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

// Mouse-up: if the final orbit's cardinality still differs from `gestureStartOrbitCount`,
// materializes every unclaimed slot as a new transform (same `symmetryGroupIdentifier`, the dragged
// member's own `layerIndex`, `Traits::SeedInstanceName`'s own naming convention) and cascade-deletes
// every still-unmatched correspondence entry — otherwise only settles the live per-frame writes one
// final time. Clears `state` unconditionally. No-op (besides clearing `state`) for an ungrouped drag.
template<typename Traits>
void EndInstanceDragGesture(InstanceDragGestureState& state, std::vector<typename Traits::Group>& instances,
                            const Params::Geometry& geometry) {
    if (!state.bActive) return;
    state.bActive = false;
    if (state.symmetryGroupIdentifier == 0) return;   // ungrouped: nothing structural, zero orbit calls
    typename Traits::Group* const group = Traits::SelectedGroup(instances, state.groupIndex);
    if (group == nullptr) return;
    typename Traits::Transform* const dragged =
        Traits::SelectedInstance(group->transforms, state.draggedTransformIndex);
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
    for (const InstanceOrbitCorrespondence& entry : state.correspondence) {
        if (entry.lastMatchedOrbitSlot < 0) continue;
        typename Traits::Transform* const sibling =
            Traits::SelectedInstance(group->transforms, entry.transformIndex);
        if (sibling == nullptr) continue;
        sibling->transform.positionX = orbitPoints[entry.lastMatchedOrbitSlot].worldPositionX;
        sibling->transform.positionZ = orbitPoints[entry.lastMatchedOrbitSlot].worldPositionZ;
    }
    if (finalCount == state.gestureStartOrbitCount) return;   // no structural change after all

    // Cascade-delete every still-unmatched sibling, descending index order so an earlier erase
    // never invalidates a later one still to be removed (R1's machinery, carried forward by R2).
    std::vector<int> deleteIndices;
    for (const InstanceOrbitCorrespondence& entry : state.correspondence)
        if (entry.lastMatchedOrbitSlot < 0) deleteIndices.push_back(entry.transformIndex);
    std::sort(deleteIndices.begin(), deleteIndices.end(), [](int a, int b) { return a > b; });
    for (int deleteIndex : deleteIndices)
        if (deleteIndex >= 0 && deleteIndex < static_cast<int>(group->transforms.size()))
            group->transforms.erase(group->transforms.begin() + deleteIndex);

    // Materialize every unclaimed slot as a brand-new sibling — same group id, the dragged member's
    // own layer. `nextInstanceIdentifier` is read once, then incremented locally per materialized
    // sibling — `Traits::NextInstanceIdentifier` itself scans `instances` fresh each call, so calling
    // it inside the loop would return the SAME id for every unclaimed slot.
    int nextInstanceIdentifier = Traits::NextInstanceIdentifier(instances);
    for (int slotIndex : unclaimedSlots) {
        typename Traits::Transform materialized;
        Traits::SeedInstanceName(materialized, static_cast<int>(group->transforms.size()));
        materialized.instanceIdentifier = nextInstanceIdentifier++;
        materialized.transform.positionX = orbitPoints[slotIndex].worldPositionX;
        materialized.transform.positionZ = orbitPoints[slotIndex].worldPositionZ;
        materialized.transform.positionY = draggedFinalPositionY;   // never RE-touched elsewhere;
                                                                     // a fresh instance still needs one
        materialized.layerIndex               = draggedLayerIndex;
        materialized.symmetryGroupIdentifier  = state.symmetryGroupIdentifier;
        group->transforms.push_back(materialized);
    }
    Traits::MakeInstanceNamesUnique(group->transforms);
}

// The roster-slider counterpart (R2 §4) — callable from a position field's on-edit path with no
// canvas/gesture-state dependency. MUST be called BEFORE the caller writes `newWorldX`/`newWorldZ`
// into `instances[groupIndex].transforms[movedTransformIndex]` — it reads that transform's CURRENT
// (pre-move) position itself to build the one-shot correspondence (the same "unmoved cloud, exact
// match" identity §1 proves), then performs both the moved member's write and every matched
// sibling's write. Refuses (returns false, moved member's own position still applied) if the target
// position would change the group's orbit cardinality — this single-call form has no gesture to
// defer a ghost/materialize step to. Also refuses (returns false, NO write at all) if the moved
// member is effectively locked (`Traits::IsInstanceEffectivelyLocked`).
template<typename Traits>
bool RepositionSymmetryGroupMember(std::vector<typename Traits::Group>& instances,
                                   const std::vector<typename Traits::Layer>& layers,
                                   const std::vector<typename Traits::Link>& links,
                                   const Params::Geometry& geometry, int globalSymmetryMask,
                                   int globalRadialRepeatCount, int groupIndex,
                                   int movedTransformIndex, float newWorldX, float newWorldZ) {
    typename Traits::Group* const group = Traits::SelectedGroup(instances, groupIndex);
    if (group == nullptr) return false;
    typename Traits::Transform* const dragged = Traits::SelectedInstance(group->transforms, movedTransformIndex);
    if (dragged == nullptr) return false;
    if (Traits::IsInstanceEffectivelyLocked(layers, *dragged, links)) return false;

    if (dragged->symmetryGroupIdentifier == 0) {
        dragged->transform.positionX = newWorldX; dragged->transform.positionZ = newWorldZ;
        return true;
    }

    int effectiveMask = 0, effectiveRadialRepeatCount = 0;
    Traits::ResolveEffectiveSymmetry(layers, *dragged, links, globalSymmetryMask,
                                     globalRadialRepeatCount, effectiveMask, effectiveRadialRepeatCount);

    Pipeline::WorldSymmetryOrbitPoint startOrbit[Params::symmetryOrbitMaximum];
    const int startCount = Pipeline::BuildWorldSymmetryOrbit(
        geometry, effectiveMask, effectiveRadialRepeatCount, dragged->transform.positionX,
        dragged->transform.positionZ, startOrbit, Params::symmetryOrbitMaximum);
    std::vector<InstanceOrbitCorrespondence> correspondence = Detail::BuildInstanceCorrespondenceSeed<Traits>(
        *group, dragged->symmetryGroupIdentifier, movedTransformIndex);
    MatchCorrespondenceToOrbit(correspondence, startOrbit, startCount, nullptr);

    Pipeline::WorldSymmetryOrbitPoint newOrbit[Params::symmetryOrbitMaximum];
    const int newCount = Pipeline::BuildWorldSymmetryOrbit(geometry, effectiveMask, effectiveRadialRepeatCount,
                                                            newWorldX, newWorldZ, newOrbit, Params::symmetryOrbitMaximum);
    dragged->transform.positionX = newWorldX;
    dragged->transform.positionZ = newWorldZ;
    if (newCount != startCount) return false;    // no gesture to defer a create/delete to — refuse

    MatchCorrespondenceToOrbit(correspondence, newOrbit, newCount, nullptr);
    for (const InstanceOrbitCorrespondence& entry : correspondence) {
        if (entry.lastMatchedOrbitSlot < 0) continue;
        typename Traits::Transform* const sibling = Traits::SelectedInstance(group->transforms, entry.transformIndex);
        if (sibling == nullptr) continue;
        sibling->transform.positionX = newOrbit[entry.lastMatchedOrbitSlot].worldPositionX;
        sibling->transform.positionZ = newOrbit[entry.lastMatchedOrbitSlot].worldPositionZ;
    }
    return true;
}

} // namespace Ui
} // namespace SanmapGen
