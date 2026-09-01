// ManualInstanceHitTest_UI.h — the generic manual-instance click/marquee hit-test, genericized from
// STEP126's Marker-only MapCanvas_MarkerHitTest_UI.cpp (ARCH §21.3/§21.5). Layer: UI.
// `HitTestManualMarkers`'s own algorithm touches only `.transforms[].transform.positionX/positionZ`
// and `.layerIndex`, name-identical across `Params::MarkerInstanceGroup`/`PropInstanceGroup`/
// `DecalInstanceGroup` (all three wrap `InstancedTransform transform` the same way) — so this is
// plain (non-`Traits`) position-only duck-typing over `GroupT`, no per-domain policy struct needed.
// Explicitly instantiated in the sibling `.cpp` for the three concrete group types (a closed,
// already-known set — Units are explicitly out of scope, ARCH §21's own closing note) rather than
// header-only, so the algorithm itself is compiled exactly once per domain, not once per including
// translation unit.
//
// Both functions take an `isInstanceLocked` predicate (ARCH §21.5/§21.9) so a locked instance never
// becomes a click-select or marquee-collect candidate in the first place — one shared gate, not a
// second copy per call site. WIDENED (ARCH §21.9, STEP249) from `bool(int layerIndex)` to
// `bool(const typename GroupT::TransformType&)` (STEP244's alias) — a Link can lock an instance
// independent of its owning Layer, and a bare layerIndex can no longer answer that alone.
// `HitTestManualMarkers` (MapCanvas_MarkerHitTest_UI.cpp) stays as a one-line wrapper over
// `HitTestManualInstances<Params::MarkerInstanceGroup>`, unchanged name and signature, legal to bind
// either an always-false-locked predicate (preserving the old bare behavior) or the real
// `IsMarkerInstanceLocked`-bound predicate — both are instantiations of the one template, never two
// copies of the algorithm.
#pragma once
#include <functional>
#include <utility>
#include <vector>
#include "MapCanvasView_UI.h"

namespace SanmapGen {
namespace Ui {

class PreviewComposite;

// Nearest manual instance (any group) within `pickRadiusScreenPixels` of the region-local cursor —
// projected via `PreviewComposite::WorldToPreviewPixel` + `MapCanvasView::
// ProjectPreviewPixelToRegionLocal`. O(instance count) — legitimate at "tens, not tens of thousands"
// (the authoring-scale sizing posture this file's Marker-only predecessor already established); NOT
// `Picking_UI::PickMarker`/`Data::SpatialGrid`, which operate only over `Data::PlacementInstances`
// (manual instances have no presence there). Ties keep the first (lowest group, then lowest
// transform) index. An effectively-locked instance (`isInstanceLocked(transform)` true) is skipped
// entirely — never becomes a candidate, ARCH §21.5/§21.9. Answers false (both out-params left at -1)
// for an unbaked composite, an empty roster, or no unlocked instance within radius.
// `outDistanceSquared`, when given, receives the winning candidate's own squared screen-pixel
// distance from the cursor — ARCH §21.2's `TryBeginManualInstanceDrag`/click-resolution 3-way
// cross-domain dispatcher needs it to compare a hit found in one domain against a hit found in
// another (this function itself only ever ranks WITHIN its own one domain's instances).
template<typename GroupT>
bool HitTestManualInstances(const std::vector<GroupT>& instances, const PreviewComposite& composite,
                            const MapCanvasView& view, float regionLocalX, float regionLocalY,
                            float pickRadiusScreenPixels,
                            const std::function<bool(const typename GroupT::TransformType&)>& isInstanceLocked,
                            int& outGroupIndex, int& outTransformIndex,
                            float* outDistanceSquared = nullptr);

// The marquee/box-select counterpart: every (groupIndex, transformIndex) pair whose exact world
// position falls inside [worldMinX,worldMaxX] x [worldMinZ,worldMaxZ] AND whose owning layer is not
// locked (ARCH §21.5) — appended to `outGroupTransformPairs` in (group, then transform) order,
// WITHOUT clearing it first (the release-time marquee resolver concatenates this against Markers/
// Props/Decals AND the two procedural `PickInstancesInRegion` queries into one combined list, ARCH
// §21.2 — clearing here would erase the earlier queries' own results).
template<typename GroupT>
void CollectManualInstancesInWorldRegion(const std::vector<GroupT>& instances,
                                         float worldMinX, float worldMinZ, float worldMaxX, float worldMaxZ,
                                         const std::function<bool(const typename GroupT::TransformType&)>& isInstanceLocked,
                                         std::vector<std::pair<int, int>>& outGroupTransformPairs);

} // namespace Ui
} // namespace SanmapGen
