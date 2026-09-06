// MapCanvas_ManualDragDispatch_UI.cpp — the 3-way (Markers/Props/Decals) manual-instance drag
// dispatcher (ARCH §21.2/§21.3). `TryBeginManualInstanceDrag` is deliberately hand-written, NOT
// templated — it touches three concrete `Params::` group types by name in one function body, which
// §21.3's own ruling routes to hand-written dispatch, never a template (the class of code
// §3.5/§19.2 already test this way). `Continue`/`EndManualInstanceDrag` dispatch to EVERY domain
// whose own `b*ManualDragActive` flag is set — no longer mutually exclusive, since
// BUGFIX_UniversalCoordinateConversionAndDragRewrite_UI's multi-select drag can move Markers, Props
// AND Decals together in one press when the current selection spans domains.
//
// BUGFIX_UniversalCoordinateConversionAndDragRewrite_UI, Part 2 — delta-based, one code path for one
// instance or many: mouse-down records `dragAnchorWorld` plus every currently-dragged instance's own
// CURRENT world position (`ManualInstanceDragEntry_UI::startWorldX/Z`); every frame while dragging
// feeds each instance's own `InstanceDragGestureState` `startWorld + delta` (`delta` = this frame's
// world position minus the anchor) instead of the cursor's raw world position — exactly today's
// existing `UpdateInstanceDragGesture`/`Traits::QuantizePositionToLayerGrid`, unchanged, just fed a
// different position. `ScreenToWorld` (CoordinateSpace_UI.h) is used throughout instead of the old
// floored `MapCanvasView::ResolvePreviewPixel` + `PreviewComposite::PreviewPixelToWorld` composition
// — that floor was the root cause of the old "large increment"/frozen-frame drag bug.
#include "CoordinateSpace_UI.h"
#include "MapCanvas_UI.h"
#include "PreviewComposite_UI.h"
#include "../params/MapRecipe_PARAMS.h"
#include <utility>

namespace SanmapGen {
namespace Ui {

bool MapCanvas::HitTestManualInstanceAcrossDomains(float regionLocalX, float regionLocalY,
                                                    PlacementCollectionKind_UI& outCollection,
                                                    int& outGroupIndex, int& outTransformIndex) const {
    outGroupIndex = -1; outTransformIndex = -1;
    if (composite == nullptr) return false;
    bool bHitAny = false;
    float bestDistanceSquared = 0.0f;

    static const std::vector<Params::MarkerLink> kNoMarkerLinksHitTest;
    if (manualMarkerDragMarkers != nullptr) {
        int groupIndex = -1, transformIndex = -1; float distanceSquared = 0.0f;
        const std::vector<Params::MarkerInstanceLayer>* layers = manualMarkerDragLayers;
        const std::vector<Params::MarkerLink>& links =
            manualMarkerDragRecipe != nullptr ? manualMarkerDragRecipe->markerLinks : kNoMarkerLinksHitTest;
        const std::function<bool(const Params::MarkerTransform&)> isLocked =
            [layers, &links](const Params::MarkerTransform& t) {
                return layers != nullptr && IsMarkerInstanceLocked(t, *layers, links);
            };
        if (HitTestManualInstances<Params::MarkerInstanceGroup>(*manualMarkerDragMarkers, *composite, view,
                regionLocalX, regionLocalY, pickRadiusScreenPixels, isLocked, groupIndex, transformIndex,
                &distanceSquared)
            && (!bHitAny || distanceSquared < bestDistanceSquared)) {
            bHitAny = true; bestDistanceSquared = distanceSquared;
            outCollection = PlacementCollectionKind_UI::Markers;
            outGroupIndex = groupIndex; outTransformIndex = transformIndex;
        }
    }
    // Strict '<' (not '<=') below: a same-distance tie keeps the EARLIER-tested domain's hit —
    // Markers, then Props, then Decals (§21.2's own fixed evaluation order).
    if (manualPropDrag.props != nullptr) {
        int groupIndex = -1, transformIndex = -1; float distanceSquared = 0.0f;
        const std::vector<Params::PropInstanceLayer>* layers = manualPropDrag.layers;
        const std::function<bool(const Params::PropTransform&)> isLocked =
            [layers](const Params::PropTransform& t) {
                return layers != nullptr && IsPropInstanceLayerLocked(*layers, t.layerIndex);
            };
        if (HitTestManualInstances<Params::PropInstanceGroup>(*manualPropDrag.props, *composite, view,
                regionLocalX, regionLocalY, pickRadiusScreenPixels, isLocked, groupIndex, transformIndex,
                &distanceSquared)
            && (!bHitAny || distanceSquared < bestDistanceSquared)) {
            bHitAny = true; bestDistanceSquared = distanceSquared;
            outCollection = PlacementCollectionKind_UI::Props;
            outGroupIndex = groupIndex; outTransformIndex = transformIndex;
        }
    }
    if (manualDecalDrag.decals != nullptr) {
        int groupIndex = -1, transformIndex = -1; float distanceSquared = 0.0f;
        const std::vector<Params::DecalInstanceLayer>* layers = manualDecalDrag.layers;
        const std::function<bool(const Params::DecalTransform&)> isLocked =
            [layers](const Params::DecalTransform& t) {
                return layers != nullptr && IsDecalInstanceLayerLocked(*layers, t.layerIndex);
            };
        if (HitTestManualInstances<Params::DecalInstanceGroup>(*manualDecalDrag.decals, *composite, view,
                regionLocalX, regionLocalY, pickRadiusScreenPixels, isLocked, groupIndex, transformIndex,
                &distanceSquared)
            && (!bHitAny || distanceSquared < bestDistanceSquared)) {
            bHitAny = true; bestDistanceSquared = distanceSquared;
            outCollection = PlacementCollectionKind_UI::Decals;
            outGroupIndex = groupIndex; outTransformIndex = transformIndex;
        }
    }
    return bHitAny;
}

namespace {

// Builds this press's drag-entry list for ONE domain, generic over its Traits (ARCH §21.3's own
// per-Traits genericity — the dispatcher above stays hand-written because IT compares across
// domains; building entries for an ALREADY-known domain is ordinary Traits-generic logic, the same
// posture InstanceDragGesture_UI.h's own Begin/Update/End already take).
// `dragKeys` is the whole press's drag set (either just the grabbed instance, or the whole current
// selection) — entries whose `collection` does not match this domain are skipped. Part 2's mirror-
// pair rule: a selected instance sharing an already-claimed (groupIndex, non-zero
// symmetryGroupIdentifier) with an EARLIER entry in `dragKeys` is skipped entirely — its own
// orbit-follow write from the earlier entry's gesture already covers it, so beginning a second
// gesture for it would race that write.
template<typename Traits>
void BeginDragEntriesForDomain(const std::vector<typename Traits::Group>& instances,
                               const std::vector<typename Traits::Layer>& layers,
                               const std::vector<typename Traits::Link>& links,
                               const Params::Geometry& geometry, int globalSymmetryMask,
                               int globalRadialRepeatCount, PlacementCollectionKind_UI collection,
                               const std::vector<OverlayInstanceKey_UI>& dragKeys,
                               std::vector<ManualInstanceDragEntry_UI>& outEntries) {
    std::vector<std::pair<int, int>> claimedSymmetryGroups;   // (groupIndex, symmetryGroupIdentifier)
    for (const OverlayInstanceKey_UI& key : dragKeys) {
        if (!key.bValid || !key.bManual || key.collection != collection) continue;
        int groupIndex = -1, transformIndex = -1;
        if (!LocateManualInstanceByIdentifier<typename Traits::Group>(instances, key.instanceIndex,
                                                                       groupIndex, transformIndex))
            continue;
        const typename Traits::Group& group = instances[static_cast<std::size_t>(groupIndex)];
        const typename Traits::Transform& transform = group.transforms[static_cast<std::size_t>(transformIndex)];
        const int symmetryGroupIdentifier = transform.symmetryGroupIdentifier;
        if (symmetryGroupIdentifier != 0) {
            bool bAlreadyClaimed = false;
            for (const std::pair<int, int>& claim : claimedSymmetryGroups)
                if (claim.first == groupIndex && claim.second == symmetryGroupIdentifier) { bAlreadyClaimed = true; break; }
            if (bAlreadyClaimed) continue;
            claimedSymmetryGroups.emplace_back(groupIndex, symmetryGroupIdentifier);
        }
        ManualInstanceDragEntry_UI entry;
        if (!BeginInstanceDragGesture<Traits>(entry.state, instances, layers, links, geometry,
                                              globalSymmetryMask, globalRadialRepeatCount,
                                              groupIndex, transformIndex))
            continue;   // effectively locked or an out-of-range index — no entry
        entry.startWorldX = transform.transform.positionX;
        entry.startWorldZ = transform.transform.positionZ;
        outEntries.push_back(entry);
    }
}

} // namespace

bool MapCanvas::TryBeginManualInstanceDrag(float regionLocalX, float regionLocalY) {
    bManualMarkerDragActive = false; bManualPropDragActive = false; bManualDecalDragActive = false;
    manualMarkerDragEntries.clear(); manualPropDrag.entries.clear(); manualDecalDrag.entries.clear();

    PlacementCollectionKind_UI hitCollection = PlacementCollectionKind_UI::Markers;
    int hitGroupIndex = -1, hitTransformIndex = -1;
    if (!HitTestManualInstanceAcrossDomains(regionLocalX, regionLocalY, hitCollection, hitGroupIndex, hitTransformIndex))
        return false;

    // STEP113's own rule ("a drag may only BEGIN while the [matching] panel is the shell's active
    // tab"), extended per-domain: null (no shell has wired a panel source) refuses, never defaults
    // to permitting a drag — same null-safe-refuses posture as every other injected pointer in this
    // class. A click can still SELECT any domain regardless of tab — see
    // HitTestManualInstanceAcrossDomains' own header comment; only the DRAG is gated here.
    const ApplicationPanel requiredPanel = hitCollection == PlacementCollectionKind_UI::Markers
        ? ApplicationPanel::Markers
        : (hitCollection == PlacementCollectionKind_UI::Props ? ApplicationPanel::Props : ApplicationPanel::Decals);
    if (activePanelSource == nullptr || *activePanelSource != requiredPanel) return false;
    if (composite == nullptr) return false;   // no world anchor to record without a baked composite

    const WorldPoint anchorWorld = ScreenToWorld(view, *composite, ScreenPoint{regionLocalX, regionLocalY});
    manualDragAnchorWorldX = anchorWorld.worldX;
    manualDragAnchorWorldZ = anchorWorld.worldZ;

    const OverlayInstanceKey_UI grabbedKey = ResolveManualHitKey(hitCollection, hitGroupIndex, hitTransformIndex);
    const bool bGrabbedInSelection = grabbedKey.bValid && SelectionSetContains(selectedInstanceKeys, grabbedKey);

    // Part 2 step 1 — "if only one instance is being dragged (nothing else selected, or the grabbed
    // instance isn't part of the current selection), that set is just the one instance." The grabbed
    // key is always placed first so it is the one entry `DrawManualMarkerDragPass` shows a live
    // ghost/refusal overlay for (see that file's own comment on this deliberate simplification).
    std::vector<OverlayInstanceKey_UI> dragKeys;
    dragKeys.push_back(grabbedKey);
    if (bGrabbedInSelection)
        for (const OverlayInstanceKey_UI& key : selectedInstanceKeys.keys)
            if (key.bValid && key.bManual && !OverlayInstanceKeysEqual(key, grabbedKey))
                dragKeys.push_back(key);

    static const std::vector<Params::MarkerInstanceLayer> kNoMarkerLayers;
    static const std::vector<Params::PropInstanceLayer>   kNoPropLayers;
    static const std::vector<Params::DecalInstanceLayer>  kNoDecalLayers;
    static const std::vector<NoInstanceLink>              kNoLinks;   // ARCH §21.9 — shared by Props/Decals

    if (manualMarkerDragMarkers != nullptr && manualMarkerDragGeometry != nullptr && manualMarkerDragRecipe != nullptr) {
        BeginDragEntriesForDomain<MarkerDragTraits>(*manualMarkerDragMarkers,
            manualMarkerDragLayers != nullptr ? *manualMarkerDragLayers : kNoMarkerLayers,
            manualMarkerDragRecipe->markerLinks, *manualMarkerDragGeometry,
            manualMarkerDragRecipe->globalSymmetryMask, manualMarkerDragRecipe->radialSymmetryRepeatCount,
            PlacementCollectionKind_UI::Markers, dragKeys, manualMarkerDragEntries);
        bManualMarkerDragActive = !manualMarkerDragEntries.empty();
    }
    if (manualPropDrag.props != nullptr && manualPropDrag.geometry != nullptr && manualPropDrag.recipe != nullptr) {
        BeginDragEntriesForDomain<PropDragTraits>(*manualPropDrag.props,
            manualPropDrag.layers != nullptr ? *manualPropDrag.layers : kNoPropLayers, kNoLinks,
            *manualPropDrag.geometry, manualPropDrag.recipe->globalSymmetryMask,
            manualPropDrag.recipe->radialSymmetryRepeatCount, PlacementCollectionKind_UI::Props,
            dragKeys, manualPropDrag.entries);
        bManualPropDragActive = !manualPropDrag.entries.empty();
    }
    if (manualDecalDrag.decals != nullptr && manualDecalDrag.geometry != nullptr && manualDecalDrag.recipe != nullptr) {
        BeginDragEntriesForDomain<DecalDragTraits>(*manualDecalDrag.decals,
            manualDecalDrag.layers != nullptr ? *manualDecalDrag.layers : kNoDecalLayers, kNoLinks,
            *manualDecalDrag.geometry, manualDecalDrag.recipe->globalSymmetryMask,
            manualDecalDrag.recipe->radialSymmetryRepeatCount, PlacementCollectionKind_UI::Decals,
            dragKeys, manualDecalDrag.entries);
        bManualDecalDragActive = !manualDecalDrag.entries.empty();
    }
    return bManualMarkerDragActive || bManualPropDragActive || bManualDecalDragActive;
}

void MapCanvas::ContinueManualInstanceDrag(float regionLocalX, float regionLocalY) {
    if (composite == nullptr) return;
    const WorldPoint currentWorld = ScreenToWorld(view, *composite, ScreenPoint{regionLocalX, regionLocalY});
    const float deltaX = currentWorld.worldX - manualDragAnchorWorldX;
    const float deltaZ = currentWorld.worldZ - manualDragAnchorWorldZ;

    static const std::vector<Params::MarkerInstanceLayer> kNoMarkerLayers;
    static const std::vector<Params::PropInstanceLayer>   kNoPropLayers;
    static const std::vector<Params::DecalInstanceLayer>  kNoDecalLayers;
    static const std::vector<Params::MarkerLink>          kNoMarkerLinks;
    static const std::vector<NoInstanceLink>              kNoLinks;   // ARCH §21.9 — shared by Props/Decals

    if (bManualMarkerDragActive && manualMarkerDragMarkers != nullptr && manualMarkerDragGeometry != nullptr) {
        const std::vector<Params::MarkerLink>& links =
            manualMarkerDragRecipe != nullptr ? manualMarkerDragRecipe->markerLinks : kNoMarkerLinks;
        for (ManualInstanceDragEntry_UI& entry : manualMarkerDragEntries)
            UpdateInstanceDragGesture<MarkerDragTraits>(entry.state, *manualMarkerDragMarkers,
                manualMarkerDragLayers != nullptr ? *manualMarkerDragLayers : kNoMarkerLayers, links,
                *manualMarkerDragGeometry, entry.startWorldX + deltaX, entry.startWorldZ + deltaZ);
    }
    if (bManualPropDragActive && manualPropDrag.props != nullptr && manualPropDrag.geometry != nullptr) {
        for (ManualInstanceDragEntry_UI& entry : manualPropDrag.entries)
            UpdateInstanceDragGesture<PropDragTraits>(entry.state, *manualPropDrag.props,
                manualPropDrag.layers != nullptr ? *manualPropDrag.layers : kNoPropLayers, kNoLinks,
                *manualPropDrag.geometry, entry.startWorldX + deltaX, entry.startWorldZ + deltaZ);
    }
    if (bManualDecalDragActive && manualDecalDrag.decals != nullptr && manualDecalDrag.geometry != nullptr) {
        for (ManualInstanceDragEntry_UI& entry : manualDecalDrag.entries)
            UpdateInstanceDragGesture<DecalDragTraits>(entry.state, *manualDecalDrag.decals,
                manualDecalDrag.layers != nullptr ? *manualDecalDrag.layers : kNoDecalLayers, kNoLinks,
                *manualDecalDrag.geometry, entry.startWorldX + deltaX, entry.startWorldZ + deltaZ);
    }
}

void MapCanvas::EndManualInstanceDrag() {
    if (bManualMarkerDragActive && manualMarkerDragMarkers != nullptr && manualMarkerDragGeometry != nullptr) {
        for (ManualInstanceDragEntry_UI& entry : manualMarkerDragEntries)
            EndInstanceDragGesture<MarkerDragTraits>(entry.state, *manualMarkerDragMarkers, *manualMarkerDragGeometry);
    }
    if (bManualPropDragActive && manualPropDrag.props != nullptr && manualPropDrag.geometry != nullptr) {
        for (ManualInstanceDragEntry_UI& entry : manualPropDrag.entries)
            EndInstanceDragGesture<PropDragTraits>(entry.state, *manualPropDrag.props, *manualPropDrag.geometry);
    }
    if (bManualDecalDragActive && manualDecalDrag.decals != nullptr && manualDecalDrag.geometry != nullptr) {
        for (ManualInstanceDragEntry_UI& entry : manualDecalDrag.entries)
            EndInstanceDragGesture<DecalDragTraits>(entry.state, *manualDecalDrag.decals, *manualDecalDrag.geometry);
    }
    manualMarkerDragEntries.clear(); manualPropDrag.entries.clear(); manualDecalDrag.entries.clear();
    bManualMarkerDragActive = false; bManualPropDragActive = false; bManualDecalDragActive = false;
}

} // namespace Ui
} // namespace SanmapGen
