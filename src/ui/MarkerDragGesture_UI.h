// MarkerDragGesture_UI.h — canvas drag-and-follow for manually-placed, symmetry-grouped markers
// (STEP94, `DESIGN_MarkerLayerSymmetry_R2.md` §1/§2, `ARCH_16_MarkerLayerSymmetry.md`). Layer: UI.
// Pure, imgui-free, testable with no window — same posture as MarkerLayerIndexRepair_UI.h. This
// file owns the GESTURE STATE MACHINE only (begin/update/end + the reusable one-shot reposition);
// the actual per-frame orbit-slot resolution lives in MarkerOrbitCorrespondence_UI.h (ARCH §1.5 —
// split to stay under the soft-100 ceiling), and canvas draw/hit-test lives in
// MapCanvas_MarkerDrag_UI.h (the one imgui-including half of this ticket).
//
// Deliberately reuses MarkersTab_Manual_UI.h's already-pure, imgui-free accessors
// (`SelectedMarkerGroup`/`SelectedMarkerInstance`/`kSpawnMarkerGroupName`/`NextMarkerInstanceName`)
// instead of re-deriving them — one range-check, one reserved-name constant, one naming convention.
//
// Mask resolution is deliberately NOT `ResolvedPlacementSymmetryMask`/`DrawPlacementSymmetryAxes`
// (`PlacementRuleSections_UI.h:56-61`) — that widget ANDs a mask against only a 4-entry axis table
// and silently drops `Params::SymmetryAxis::Radial` (bit 4). `ResolveEffectiveMarkerSymmetry` below
// reads the already-resolved, already-valid layer/global fields directly; the Radial-dropping
// defect is structurally unreachable from this file, not merely avoided by discipline.
#pragma once
#include <vector>
#include "MarkerOrbitCorrespondence_UI.h"
#include "MarkersTab_Manual_UI.h"
#include "../params/Geometry_PARAMS.h"
#include "../params/MarkerInstance_PARAMS.h"
#include "../pipeline/SymmetryOrbitQuery_PIPELINE.h"

namespace SanmapGen {
namespace Ui {

// The reserved commander-spawn roster name whose cardinality a drag must never change (R2 §3,
// `ARCH_16_08_SpawnArmyShrink.md`). Value-identical to `kSpawnMarkerGroupName`
// (MarkersTab_Manual_UI.h) — restated under this ticket's own requested name so this gesture file
// documents its own Spawn-refusal rule without a reader having to cross-reference the tab header
// for the literal, while still deriving from the ONE reserved string (no second literal to drift).
inline constexpr const char* kArmyKeyedMarkerGroupName = kSpawnMarkerGroupName;

// One gesture's full state: which member is being dragged, its frozen mask/count snapshot, the
// one-shot correspondence table (built once, at BeginMarkerDragGesture, never rebuilt), and this
// frame's ghost/refusal bookkeeping for the draw pass.
struct MarkerDragGestureState {
    bool bActive                = false;
    int  groupIndex              = -1;   // recipe.markers[groupIndex] — the dragged member's group
    int  draggedTransformIndex   = -1;   // recipe.markers[groupIndex].transforms[...]
    int  symmetryGroupIdentifier = 0;    // snapshot at gesture-start; frozen for the gesture. 0 = ungrouped
    int  effectiveSymmetryMask   = 0;    // snapshot — ResolveEffectiveMarkerSymmetry, gesture-start
    int  effectiveRadialRepeatCount = 0; // snapshot
    bool bSpawnGroup             = false; // snapshot: group.name == kArmyKeyedMarkerGroupName

    std::vector<MarkerOrbitCorrespondence> correspondence;   // built once; never rebuilt mid-drag
    int  gestureStartOrbitCount  = 0;    // cardinality at gesture-start — the comparison baseline

    int   lastValidOrbitCount    = 0;    // cardinality as of the last cardinality-matching frame
    float lastValidDraggedWorldX = 0.0f; // dragged member's position as of that same frame —
    float lastValidDraggedWorldZ = 0.0f; // Spawn-refusal freezes here (R2 §3)

    bool  bSpawnCardinalityRefused = false; // this frame's UI-feedback flag
    bool  bCardinalityGrew         = false; // this frame: unclaimed orbit slots are pending ghosts
    bool  bCardinalityShrank       = false; // this frame: unmatched entries are pending cascade-deletes
    int   pendingOrbitCount        = 0;     // this frame's freshly recomputed orbit count
    std::vector<Pipeline::WorldSymmetryOrbitPoint> currentGhostPoints; // this frame's unclaimed slots, for the draw pass only
};

// The raw, already-valid mask/count for `layerIndex` — `layer.symmetry.bSymmetryUseGlobal` selects
// between the layer's own fields and the two global ones, per STEP68's own two-line ternary
// (`SymmetryOrbitQuery_PIPELINE.h`'s wrapper deliberately does not resolve this itself). An
// out-of-range `layerIndex` (Constitution §6) falls back to the global pair.
inline void ResolveEffectiveMarkerSymmetry(const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                           int layerIndex, int globalSymmetryMask,
                                           int globalRadialRepeatCount, int& outMask,
                                           int& outRadialRepeatCount) {
    if (layerIndex < 0 || layerIndex >= static_cast<int>(markerLayers.size())) {
        outMask = globalSymmetryMask; outRadialRepeatCount = globalRadialRepeatCount; return;
    }
    const Params::MarkerInstanceLayer& layer = markerLayers[static_cast<std::size_t>(layerIndex)];
    outMask = layer.symmetry.bSymmetryUseGlobal ? globalSymmetryMask : layer.symmetry.symmetryMask;
    outRadialRepeatCount = layer.symmetry.bSymmetryUseGlobal ? globalRadialRepeatCount
                                                              : layer.symmetry.radialSymmetryRepeatCount;
}

// True when `transformIndex` (within the gesture's own group) is this frame's soft-hidden (an
// existing sibling the current orbit no longer produces — collapsed, not erased). Used by the draw
// pass; false whenever no gesture is active or the index belongs to a different group.
inline bool IsMarkerSoftHiddenThisFrame(const MarkerDragGestureState& state, int groupIndex,
                                        int transformIndex) {
    if (!state.bActive || state.groupIndex != groupIndex) return false;
    for (const MarkerOrbitCorrespondence& entry : state.correspondence)
        if (entry.transformIndex == transformIndex) return entry.bSoftHidden;
    return false;
}

// MarkerDragGesture_UI.cpp — mouse-down: hit-testing already happened (MapCanvas_MarkerDrag_UI.cpp,
// Gap 5); this only builds the gesture state from the resolved (groupIndex, transformIndex). A
// `symmetryGroupIdentifier == 0` hit still returns true (a free, unrestricted drag) with an empty
// correspondence table and NO `Pipeline::BuildWorldSymmetryOrbit` call. Returns false (state left
// inactive) only for an out-of-range group/transform index.
bool BeginMarkerDragGesture(MarkerDragGestureState& state,
                            const std::vector<Params::MarkerInstanceGroup>& markers,
                            const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                            const Params::Geometry& geometry, int globalSymmetryMask,
                            int globalRadialRepeatCount, int groupIndex, int transformIndex);

// MarkerDragGesture_Frame_UI.cpp — one drag frame: writes the dragged member's position always
// (unambiguous, follows the cursor); writes every matched sibling's position too UNLESS this
// frame's orbit cardinality differs from `gestureStartOrbitCount` (R2 §2's structural-changes-wait-
// for-release rule) — a Spawn group additionally freezes the dragged member itself and writes
// nothing at all for a cardinality-changing frame (R2 §3). `newWorldX`/`newWorldZ` are the cursor's
// CURRENT world position (STEP47's `PreviewComposite::PreviewPixelToWorld`). No-op if `state` is
// not active.
void UpdateMarkerDragGesture(MarkerDragGestureState& state, std::vector<Params::MarkerInstanceGroup>& markers,
                             const Params::Geometry& geometry, float newWorldX, float newWorldZ);

// MarkerDragGesture_Frame_UI.cpp — mouse-up: if the final orbit's cardinality still differs from
// `gestureStartOrbitCount`, materializes every unclaimed slot as a new `MarkerTransform` (same
// `symmetryGroupIdentifier`, the dragged member's own `layerIndex`, STEP49's own instance-naming
// convention) and cascade-deletes every still-unmatched correspondence entry — otherwise only
// settles the live per-frame writes one final time. Clears `state` unconditionally. No-op (besides
// clearing `state`) for an ungrouped drag.
void EndMarkerDragGesture(MarkerDragGestureState& state, std::vector<Params::MarkerInstanceGroup>& markers,
                          const Params::Geometry& geometry);

// The roster-slider counterpart (R2 §4) — callable from a Position slider's on-edit path with no
// canvas/gesture-state dependency (STEP49's own file is NOT wired to call this by this ticket; see
// STEP94's "roster-slider counterpart" coordination note). MUST be called BEFORE the caller writes
// `newWorldX`/`newWorldZ` into `markers[groupIndex].transforms[movedTransformIndex]` — it reads
// that transform's CURRENT (pre-move) position itself to build the one-shot correspondence (the
// same "unmoved cloud, exact match" identity §1 proves), then performs both the moved member's
// write and every matched sibling's write. Refuses (returns false, moved member's own position
// still applied) if the target position would change the group's orbit cardinality — this single-
// call form has no gesture to defer a ghost/materialize step to.
bool RepositionSymmetryGroupMember(std::vector<Params::MarkerInstanceGroup>& markers,
                                   const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                   const Params::Geometry& geometry, int globalSymmetryMask,
                                   int globalRadialRepeatCount, int groupIndex,
                                   int movedTransformIndex, float newWorldX, float newWorldZ);

} // namespace Ui
} // namespace SanmapGen
