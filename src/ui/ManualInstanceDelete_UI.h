// ManualInstanceDelete_UI.h — the generic manual-instance erase-by-instanceIdentifier primitive.
// STEP234, DESIGN_MarkerLink_R1.md §1.2. `DeleteManualInstancesById<GroupT>`'s own algorithm touches
// only `.transforms[].instanceIdentifier` and `.transforms[].layerIndex`, name-identical across
// `Params::MarkerInstanceGroup`/`PropInstanceGroup`/`DecalInstanceGroup` — the same plain
// (non-`Traits`) duck-typed template `HitTestManualInstances<GroupT>` already uses
// (ManualInstanceHitTest_UI.h, ARCH §21.3), explicitly instantiated in the sibling `.cpp` for the
// same closed, three-domain set (Units are out of scope). Kept in its OWN file, not folded into
// ManualInstanceHitTest_UI.h — that file is query-only; this one mutates, a real file-boundary
// reason, not arbitrary (DESIGN_MarkerLink_R1.md §1.2's own stated rationale).
#pragma once
#include <functional>
#include <vector>

namespace SanmapGen {
namespace Params {
struct MarkerInstanceGroup;
struct MarkerInstanceLayer;
struct MarkerLink;
struct PropInstanceGroup;
struct PropInstanceLayer;
struct DecalInstanceGroup;
struct DecalInstanceLayer;
} // namespace Params

namespace Ui {

// Erases every transform across `instances` whose `instanceIdentifier` is in `identifiers` AND whose
// owning instance is NOT effectively locked (`isInstanceLocked(transform) == false`). Returns the
// count actually erased; a locked or missing identifier is a silent per-identifier no-op
// (Constitution §6 — a partial delete due to a locked member is a soft degrade, not a refusal of the
// whole batch). WIDENED (ARCH §21.9, STEP249) from `bool(int layerIndex)` to
// `bool(const typename GroupT::TransformType&)` — mirrors ManualInstanceHitTest_UI.h's identical
// predicate widening, same reason (a Link can lock an instance independent of its owning Layer).
template<typename GroupT>
int DeleteManualInstancesById(std::vector<GroupT>& instances, const std::vector<int>& identifiers,
                              const std::function<bool(const typename GroupT::TransformType&)>& isInstanceLocked);

// Thin per-domain wrappers binding each domain's own already-existing lock predicate — mirrors
// `HitTestManualMarkers`'s own "one-line wrapper over the template" closing convention (ARCH §21.3).
// Each takes its domain's own layer roster (the predicate's own required argument), never captured
// globally — same posture as `MarkerDragTraits`'s own layer-roster-by-parameter convention
// (MarkerDragGesture_UI.h). `DeleteSelectedManualMarkerInstances` additionally takes `markerLinks`
// (ARCH §21.9) to build its instance-tier-aware lock predicate; Props/Decals have no Link concept,
// so their own wrappers are unchanged.
int DeleteSelectedManualMarkerInstances(std::vector<Params::MarkerInstanceGroup>& markers,
                                        const std::vector<int>& identifiers,
                                        const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                        const std::vector<Params::MarkerLink>& markerLinks);
int DeleteSelectedManualPropInstances(std::vector<Params::PropInstanceGroup>& props,
                                      const std::vector<int>& identifiers,
                                      const std::vector<Params::PropInstanceLayer>& propLayers);
int DeleteSelectedManualDecalInstances(std::vector<Params::DecalInstanceGroup>& decals,
                                       const std::vector<int>& identifiers,
                                       const std::vector<Params::DecalInstanceLayer>& decalLayers);

} // namespace Ui
} // namespace SanmapGen
