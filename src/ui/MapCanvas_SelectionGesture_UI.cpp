// MapCanvas_SelectionGesture_UI.cpp — the modifier-aware click/marquee selection resolvers (ARCH
// §21.2/§21.5/§21.6). `ApplyClick` (MapCanvas_UI.cpp) wraps `ApplyClickGesture` with
// bCtrlHeld=bShiftHeld=false, preserving its own pre-existing public behavior byte-identically for
// every existing caller/test.
#include "MapCanvas_UI.h"
#include "Picking_UI.h"
#include "PreviewComposite_UI.h"
#include "../data/PlacementResults_DATA.h"
#include "../params/MapRecipe_PARAMS.h"
#include <algorithm>
#include <utility>

namespace SanmapGen {
namespace Ui {

OverlayInstanceKey_UI MapCanvas::ResolveManualHitKey(PlacementCollectionKind_UI collection, int groupIndex,
                                                     int transformIndex) const {
    switch (collection) {
    case PlacementCollectionKind_UI::Markers:
        if (manualMarkerDragMarkers == nullptr) break;
        return OverlayInstanceKey_UI{PlacementCollectionKind_UI::Markers,
            (*manualMarkerDragMarkers)[static_cast<std::size_t>(groupIndex)]
                .transforms[static_cast<std::size_t>(transformIndex)].instanceIdentifier, true, true};
    case PlacementCollectionKind_UI::Props:
        if (manualPropDrag.props == nullptr) break;
        return OverlayInstanceKey_UI{PlacementCollectionKind_UI::Props,
            (*manualPropDrag.props)[static_cast<std::size_t>(groupIndex)]
                .transforms[static_cast<std::size_t>(transformIndex)].instanceIdentifier, true, true};
    case PlacementCollectionKind_UI::Decals:
        if (manualDecalDrag.decals == nullptr) break;
        return OverlayInstanceKey_UI{PlacementCollectionKind_UI::Decals,
            (*manualDecalDrag.decals)[static_cast<std::size_t>(groupIndex)]
                .transforms[static_cast<std::size_t>(transformIndex)].instanceIdentifier, true, true};
    default: break;
    }
    return OverlayInstanceKey_UI{};   // out-of-range/unwired source: the default invalid key
}

// ARCH §21.2 — a manual hit (any of the three domains, nearest-hit-wins, lock-gated) wins first; a
// miss falls through to the pre-existing procedural Markers-only PickMarker path, unchanged from
// the pre-§21.2 ApplyClick body other than routing through ApplySelectionGesture instead of the
// bare SetSelection.
void MapCanvas::ApplyClickGesture(float regionLocalX, float regionLocalY, bool bCtrlHeld, bool bShiftHeld) {
    lastPickedPixel = view.ResolvePreviewPixel(regionLocalX, regionLocalY);

    PlacementCollectionKind_UI hitCollection = PlacementCollectionKind_UI::Markers;
    int hitGroupIndex = -1, hitTransformIndex = -1;
    if (HitTestManualInstanceAcrossDomains(regionLocalX, regionLocalY, hitCollection, hitGroupIndex,
                                           hitTransformIndex)) {
        ApplySelectionGesture(ResolveManualHitKey(hitCollection, hitGroupIndex, hitTransformIndex),
                              bCtrlHeld, bShiftHeld);
        return;
    }

    const OverlayInstanceKey_UI missKey{PlacementCollectionKind_UI::Markers,
        static_cast<std::int32_t>(Data::EntityIdBuffer::emptySentinel), false, false};
    if (!lastPickedPixel.bInsideImage || pickMarkerInstances == nullptr
        || pickMarkerSpatialGrid == nullptr || composite == nullptr
        || composite->PixelsPerPreviewCell() <= 0.0f) {
        ApplySelectionGesture(missKey, bCtrlHeld, bShiftHeld);
        return;
    }
    const PreviewComposite::PreviewWorldPoint worldPoint =
        composite->PreviewPixelToWorld(static_cast<float>(lastPickedPixel.pixelX),
                                       static_cast<float>(lastPickedPixel.pixelY));
    const float pickRadiusWorldUnits = pickRadiusScreenPixels
        * view.PreviewPixelsPerRegionPixel()
        * composite->Settings().worldUnitsPerCell / composite->PixelsPerPreviewCell();
    const std::int32_t pickedIndex = PickMarker(*pickMarkerSpatialGrid, *pickMarkerInstances,
                                                worldPoint.worldX, worldPoint.worldZ, pickRadiusWorldUnits);
    if (pickedIndex != kNoMarkerPicked) {
        ApplySelectionGesture(OverlayInstanceKey_UI{PlacementCollectionKind_UI::Markers, pickedIndex, true, false},
                              bCtrlHeld, bShiftHeld);
        return;
    }
    ApplySelectionGesture(missKey, bCtrlHeld, bShiftHeld);
}

namespace {

// A press-start/release-time region-local point pair resolves, via the SAME region-local ->
// preview-pixel -> world chain ApplyClick already uses, to one world-space axis-aligned rectangle —
// whichever corner pair is min/max, since a press can end left-of or right-of its start in either
// axis (ARCH §21.2's own instruction).
struct WorldRect { float minX = 0.0f, minZ = 0.0f, maxX = 0.0f, maxZ = 0.0f; };

WorldRect ResolveMarqueeWorldRect(const MapCanvas& canvas, const MapCanvasView& view,
                                  const PreviewComposite& composite, float pressRegionLocalX,
                                  float pressRegionLocalY, float releaseRegionLocalX,
                                  float releaseRegionLocalY) {
    (void)canvas;
    const PreviewPixelCoordinate pressPixel = view.ResolvePreviewPixel(pressRegionLocalX, pressRegionLocalY);
    const PreviewPixelCoordinate releasePixel = view.ResolvePreviewPixel(releaseRegionLocalX, releaseRegionLocalY);
    const PreviewComposite::PreviewWorldPoint pressWorld = composite.PreviewPixelToWorld(
        static_cast<float>(pressPixel.pixelX), static_cast<float>(pressPixel.pixelY));
    const PreviewComposite::PreviewWorldPoint releaseWorld = composite.PreviewPixelToWorld(
        static_cast<float>(releasePixel.pixelX), static_cast<float>(releasePixel.pixelY));
    WorldRect rect;
    rect.minX = std::min(pressWorld.worldX, releaseWorld.worldX);
    rect.maxX = std::max(pressWorld.worldX, releaseWorld.worldX);
    rect.minZ = std::min(pressWorld.worldZ, releaseWorld.worldZ);
    rect.maxZ = std::max(pressWorld.worldZ, releaseWorld.worldZ);
    return rect;
}

} // namespace

// ARCH §21.2/§21.6 — every resulting key across all six queries (3 procedural PickInstancesInRegion
// + 3 manual CollectManualInstancesInWorldRegion, each manual query lock-gated per §21.5) is
// concatenated into one ordered list and resolved through ApplySelectionGesture's batch overload.
void MapCanvas::ApplyMarqueeGesture(float pressRegionLocalX, float pressRegionLocalY,
                                    float releaseRegionLocalX, float releaseRegionLocalY,
                                    bool bCtrlHeld, bool bShiftHeld) {
    if (composite == nullptr) {
        ApplySelectionGesture(std::vector<OverlayInstanceKey_UI>{}, bCtrlHeld, bShiftHeld);
        return;
    }
    const WorldRect rect = ResolveMarqueeWorldRect(*this, view, *composite, pressRegionLocalX,
                                                   pressRegionLocalY, releaseRegionLocalX, releaseRegionLocalY);
    std::vector<OverlayInstanceKey_UI> hits;

    // Procedural half — Markers/Props/Decals (Units out of scope, §21's own closing note).
    if (overlayPlacements != nullptr && pickSpatialGridSet != nullptr) {
        std::vector<std::int32_t> indices;
        PickInstancesInRegion(pickSpatialGridSet->markers, overlayPlacements->markers,
                              rect.minX, rect.minZ, rect.maxX, rect.maxZ, indices);
        for (std::int32_t index : indices)
            hits.push_back(OverlayInstanceKey_UI{PlacementCollectionKind_UI::Markers, index, true, false});
        PickInstancesInRegion(pickSpatialGridSet->props, overlayPlacements->props,
                              rect.minX, rect.minZ, rect.maxX, rect.maxZ, indices);
        for (std::int32_t index : indices)
            hits.push_back(OverlayInstanceKey_UI{PlacementCollectionKind_UI::Props, index, true, false});
        PickInstancesInRegion(pickSpatialGridSet->decals, overlayPlacements->decals,
                              rect.minX, rect.minZ, rect.maxX, rect.maxZ, indices);
        for (std::int32_t index : indices)
            hits.push_back(OverlayInstanceKey_UI{PlacementCollectionKind_UI::Decals, index, true, false});
    }

    // Manual half — Markers/Props/Decals, each lock-gated (ARCH §21.5).
    if (manualMarkerDragMarkers != nullptr) {
        const std::vector<Params::MarkerInstanceLayer>* layers = manualMarkerDragLayers;
        const std::function<bool(int)> isLocked = [layers](int layerIndex) {
            return layers != nullptr && IsMarkerInstanceLayerLocked(*layers, layerIndex);
        };
        std::vector<std::pair<int, int>> pairs;
        CollectManualInstancesInWorldRegion<Params::MarkerInstanceGroup>(
            *manualMarkerDragMarkers, rect.minX, rect.minZ, rect.maxX, rect.maxZ, isLocked, pairs);
        for (const std::pair<int, int>& pair : pairs)
            hits.push_back(ResolveManualHitKey(PlacementCollectionKind_UI::Markers, pair.first, pair.second));
    }
    if (manualPropDrag.props != nullptr) {
        const std::vector<Params::PropInstanceLayer>* layers = manualPropDrag.layers;
        const std::function<bool(int)> isLocked = [layers](int layerIndex) {
            return layers != nullptr && IsPropInstanceLayerLocked(*layers, layerIndex);
        };
        std::vector<std::pair<int, int>> pairs;
        CollectManualInstancesInWorldRegion<Params::PropInstanceGroup>(
            *manualPropDrag.props, rect.minX, rect.minZ, rect.maxX, rect.maxZ, isLocked, pairs);
        for (const std::pair<int, int>& pair : pairs)
            hits.push_back(ResolveManualHitKey(PlacementCollectionKind_UI::Props, pair.first, pair.second));
    }
    if (manualDecalDrag.decals != nullptr) {
        const std::vector<Params::DecalInstanceLayer>* layers = manualDecalDrag.layers;
        const std::function<bool(int)> isLocked = [layers](int layerIndex) {
            return layers != nullptr && IsDecalInstanceLayerLocked(*layers, layerIndex);
        };
        std::vector<std::pair<int, int>> pairs;
        CollectManualInstancesInWorldRegion<Params::DecalInstanceGroup>(
            *manualDecalDrag.decals, rect.minX, rect.minZ, rect.maxX, rect.maxZ, isLocked, pairs);
        for (const std::pair<int, int>& pair : pairs)
            hits.push_back(ResolveManualHitKey(PlacementCollectionKind_UI::Decals, pair.first, pair.second));
    }

    ApplySelectionGesture(hits, bCtrlHeld, bShiftHeld);
}

} // namespace Ui
} // namespace SanmapGen
