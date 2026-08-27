// MarkerInstanceCreateSymmetric_UI.h — CreateSymmetricManualMarkerInstances, the single entry point
// for materializing a brand-new manual marker instance AND its symmetric siblings together. Layer:
// UI. Human's own bug report — "When creating an Instance, symmetry needs to be checked and
// duplicates created for proper symmetry": the "+ Instance" header button (MarkersTab_UI.cpp)
// pushed exactly one `MarkerTransform` and never touched symmetry at all. The materialize LOOP this
// function runs is not new: it is EndMarkerDragGesture's own unclaimed-orbit-slot loop
// (MarkerDragGesture_Frame_UI.cpp), generalized to the "no existing correspondence, every orbit
// point is fresh" case a brand-new instance always is (a drag's own loop only fires for slots an
// EXISTING correspondence table left unclaimed; here every slot is unclaimed, by construction).
#pragma once
#include <vector>
#include "../params/Geometry_PARAMS.h"
#include "../params/MarkerInstance_PARAMS.h"

namespace SanmapGen {
namespace Ui {

// Resolves `layerIndex`'s effective symmetry (ResolveEffectiveMarkerSymmetry, MarkerDragGesture_UI.h)
// against `worldX`/`worldZ`, then pushes one `MarkerTransform` per resulting orbit point into `group`
// — `worldY`/`layerIndex` shared by every point, one freshly-minted `symmetryGroupIdentifier` shared
// by every point WHEN the orbit has more than one point (a 1-point orbit — symmetry off, or a mask
// that resolves to none — stays ungrouped, `symmetryGroupIdentifier == 0`, byte-identical to the old
// single-instance push_back). Every pushed instance mints its own fresh `instanceIdentifier`
// (`NextMarkerInstanceIdentifier`, read once, incremented locally — calling it inside the loop would
// return the SAME id for every point, `EndMarkerDragGesture`'s own established reasoning) and runs
// the shared `MakeNamesUnique` repair once, after every point is pushed. `markers` is the FULL
// roster (every group) — needed to scan a roster-wide-unique `instanceIdentifier`/
// `symmetryGroupIdentifier`, same two-level walk `NextMarkerInstanceIdentifier` already does; `group`
// is the specific group the new instance(s) belong to, which may or may not be an element of
// `markers` at the call site's discretion (mirrors `FindOrCreateMarkerInstanceGroupByName`'s own
// reference-into-`markers` return, MarkersTab_UI.cpp). Returns the SOURCE point's own
// `instanceIdentifier` (the orbit's index-0 entry, always present) so the caller can select it —
// mirrors the old code's own "select what was just added" convention.
int CreateSymmetricManualMarkerInstances(Params::MarkerInstanceGroup& group,
                                         const std::vector<Params::MarkerInstanceGroup>& markers,
                                         const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                         const Params::Geometry& geometry, int globalSymmetryMask,
                                         int globalRadialRepeatCount, int layerIndex,
                                         float worldX, float worldY, float worldZ);

} // namespace Ui
} // namespace SanmapGen
