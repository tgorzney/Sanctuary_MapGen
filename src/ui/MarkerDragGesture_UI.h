// MarkerDragGesture_UI.h — the Markers-domain instantiation of the generic drag-gesture substrate
// (InstanceDragGesture_UI.h, ARCH §21.3). Shrunk from STEP94's original ~150-line Marker-only state
// machine to just `MarkerDragTraits` (thin static wrappers over the already-existing free functions
// this file always deferred to — zero behavior change, every wrapped function keeps its own current
// name and file) plus `kArmyKeyedMarkerGroupName`, unchanged, plus a set of concrete-name thin
// wrapper functions so every existing Markers call site (`BeginMarkerDragGesture`,
// `UpdateMarkerDragGesture`, `EndMarkerDragGesture`, `RepositionSymmetryGroupMember`,
// `MarkerDragGestureState`, `IsMarkerSoftHiddenThisFrame`) keeps compiling with its own established
// name — the exact same "old concrete name survives as a one-line wrapper over the new generic
// template" posture §21.3 itself rules for `HitTestManualMarkers`, applied here to this file's own
// four gesture-lifecycle entry points and its state-struct alias. `Traits::Group` appears only in a
// non-deduced context (`typename Traits::Group`), so a template call always needs its `<Traits>`
// explicit — meaning these non-template wrappers are the ONLY viable overload for every existing
// unqualified call site; no ambiguity, no accidental double-definition (Constitution §6: verified,
// not assumed).
#pragma once
#include <string>
#include <vector>
#include "InstanceDragGesture_UI.h"
#include "MarkerInstanceId_UI.h"
#include "MarkersTab_ManualLayerHelpers_UI.h"
#include "MarkersTab_ManualLayers_UI.h"
#include "MarkersTab_Manual_UI.h"
#include "UniqueNameList_UI.h"
#include "../params/Geometry_PARAMS.h"
#include "../params/MarkerInstance_PARAMS.h"

namespace SanmapGen {
namespace Ui {

// The reserved commander-spawn roster name whose cardinality a drag must never change (R2 §3,
// `ARCH_16_08_SpawnArmyShrink.md`). Value-identical to `kSpawnMarkerGroupName`
// (MarkersTab_Manual_UI.h) — restated under this ticket's own requested name so this gesture file
// documents its own Spawn-refusal rule without a reader having to cross-reference the tab header
// for the literal, while still deriving from the ONE reserved string (no second literal to drift).
inline constexpr const char* kArmyKeyedMarkerGroupName = kSpawnMarkerGroupName;

// The Markers-domain state struct is byte-identical to the generic one (ARCH §21.3 refinement 1) —
// an alias, not a second type, so every existing `MarkerDragGestureState` field access/construction
// keeps compiling unchanged.
using MarkerDragGestureState = InstanceDragGestureState;

// ARCH §21.3's `Traits` contract, instantiated for Markers. Every method is a one-line forward to a
// pre-existing, already-tested free function — this struct adds no new logic of its own.
struct MarkerDragTraits {
    using Group     = Params::MarkerInstanceGroup;
    using Transform  = Params::MarkerTransform;
    using Layer      = Params::MarkerInstanceLayer;

    static Group* SelectedGroup(std::vector<Group>& markers, int groupIndex) {
        return SelectedMarkerGroup(markers, groupIndex);
    }
    static Transform* SelectedInstance(std::vector<Transform>& transforms, int transformIndex) {
        return SelectedMarkerInstance(transforms, transformIndex);
    }
    static bool IsInstanceLayerLocked(const std::vector<Layer>& layers, int layerIndex) {
        return IsMarkerInstanceLayerLocked(layers, layerIndex);
    }
    static void QuantizePositionToLayerGrid(const std::vector<Layer>& layers, int layerIndex,
                                            float& x, float& z) {
        QuantizeMarkerPositionToLayerGrid(layers, layerIndex, x, z);
    }
    static void ResolveEffectiveSymmetry(const std::vector<Layer>& layers, int layerIndex,
                                         int globalMask, int globalRadialCount, int& outMask,
                                         int& outRadialCount) {
        ResolveEffectiveMarkerSymmetry(layers, layerIndex, globalMask, globalRadialCount, outMask, outRadialCount);
    }
    // Markers: the reserved Spawn roster's cardinality is frozen (R2 §3). Props/Decals: always false
    // (PropDragGesture_UI.h/DecalDragGesture_UI.h) — there is no equivalent reserved group there.
    static bool IsCardinalityFrozenGroup(const Group& group) { return IsSpawnMarkerGroup(group); }
    static int  NextInstanceIdentifier(const std::vector<Group>& markers) {
        return NextMarkerInstanceIdentifier(markers);
    }
    static void SeedInstanceName(Transform& materialized, int existingTransformCount) {
        materialized.name = NextMarkerInstanceName(existingTransformCount);
    }
    static void MakeInstanceNamesUnique(std::vector<Transform>& transforms) {
        MakeNamesUnique(transforms);
    }
};

inline bool BeginMarkerDragGesture(MarkerDragGestureState& state,
                                   const std::vector<Params::MarkerInstanceGroup>& markers,
                                   const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                   const Params::Geometry& geometry, int globalSymmetryMask,
                                   int globalRadialRepeatCount, int groupIndex, int transformIndex) {
    return BeginInstanceDragGesture<MarkerDragTraits>(state, markers, markerLayers, geometry,
                                                       globalSymmetryMask, globalRadialRepeatCount,
                                                       groupIndex, transformIndex);
}

inline void UpdateMarkerDragGesture(MarkerDragGestureState& state,
                                    std::vector<Params::MarkerInstanceGroup>& markers,
                                    const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                    const Params::Geometry& geometry, float newWorldX, float newWorldZ) {
    UpdateInstanceDragGesture<MarkerDragTraits>(state, markers, markerLayers, geometry, newWorldX, newWorldZ);
}

inline void EndMarkerDragGesture(MarkerDragGestureState& state,
                                 std::vector<Params::MarkerInstanceGroup>& markers,
                                 const Params::Geometry& geometry) {
    EndInstanceDragGesture<MarkerDragTraits>(state, markers, geometry);
}

inline bool RepositionSymmetryGroupMember(std::vector<Params::MarkerInstanceGroup>& markers,
                                          const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                          const Params::Geometry& geometry, int globalSymmetryMask,
                                          int globalRadialRepeatCount, int groupIndex,
                                          int movedTransformIndex, float newWorldX, float newWorldZ) {
    return RepositionSymmetryGroupMember<MarkerDragTraits>(markers, markerLayers, geometry,
                                                            globalSymmetryMask, globalRadialRepeatCount,
                                                            groupIndex, movedTransformIndex,
                                                            newWorldX, newWorldZ);
}

inline bool IsMarkerSoftHiddenThisFrame(const MarkerDragGestureState& state, int groupIndex,
                                        int transformIndex) {
    return IsInstanceSoftHiddenThisFrame(state, groupIndex, transformIndex);
}

} // namespace Ui
} // namespace SanmapGen
