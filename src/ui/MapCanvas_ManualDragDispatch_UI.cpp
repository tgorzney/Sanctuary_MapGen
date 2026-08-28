// MapCanvas_ManualDragDispatch_UI.cpp — the 3-way (Markers/Props/Decals) manual-instance drag
// dispatcher (ARCH §21.2/§21.3). `TryBeginManualInstanceDrag` is deliberately hand-written, NOT
// templated — it touches three concrete `Params::` group types by name in one function body, which
// §21.3's own ruling routes to hand-written dispatch, never a template (the class of code
// §3.5/§19.2 already test this way). `Continue`/`EndManualInstanceDrag` dispatch to whichever ONE of
// the three `b*ManualDragActive` flags this press actually set.
#include "MapCanvas_UI.h"
#include "PreviewComposite_UI.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Ui {

bool MapCanvas::HitTestManualInstanceAcrossDomains(float regionLocalX, float regionLocalY,
                                                    PlacementCollectionKind_UI& outCollection,
                                                    int& outGroupIndex, int& outTransformIndex) const {
    outGroupIndex = -1; outTransformIndex = -1;
    if (composite == nullptr) return false;
    bool bHitAny = false;
    float bestDistanceSquared = 0.0f;

    if (manualMarkerDragMarkers != nullptr) {
        int groupIndex = -1, transformIndex = -1; float distanceSquared = 0.0f;
        const std::vector<Params::MarkerInstanceLayer>* layers = manualMarkerDragLayers;
        const std::function<bool(int)> isLocked = [layers](int layerIndex) {
            return layers != nullptr && IsMarkerInstanceLayerLocked(*layers, layerIndex);
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
        const std::function<bool(int)> isLocked = [layers](int layerIndex) {
            return layers != nullptr && IsPropInstanceLayerLocked(*layers, layerIndex);
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
        const std::function<bool(int)> isLocked = [layers](int layerIndex) {
            return layers != nullptr && IsDecalInstanceLayerLocked(*layers, layerIndex);
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

bool MapCanvas::TryBeginManualInstanceDrag(float regionLocalX, float regionLocalY) {
    bManualMarkerDragActive = false; bManualPropDragActive = false; bManualDecalDragActive = false;

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

    static const std::vector<Params::MarkerInstanceLayer> kNoMarkerLayers;
    static const std::vector<Params::PropInstanceLayer>   kNoPropLayers;
    static const std::vector<Params::DecalInstanceLayer>  kNoDecalLayers;

    switch (hitCollection) {
    case PlacementCollectionKind_UI::Markers:
        if (manualMarkerDragMarkers == nullptr || manualMarkerDragGeometry == nullptr
            || manualMarkerDragRecipe == nullptr) return false;
        bManualMarkerDragActive = BeginInstanceDragGesture<MarkerDragTraits>(
            manualMarkerDragState, *manualMarkerDragMarkers,
            manualMarkerDragLayers != nullptr ? *manualMarkerDragLayers : kNoMarkerLayers,
            *manualMarkerDragGeometry, manualMarkerDragRecipe->globalSymmetryMask,
            manualMarkerDragRecipe->radialSymmetryRepeatCount, hitGroupIndex, hitTransformIndex);
        return bManualMarkerDragActive;
    case PlacementCollectionKind_UI::Props:
        if (manualPropDrag.props == nullptr || manualPropDrag.geometry == nullptr
            || manualPropDrag.recipe == nullptr) return false;
        bManualPropDragActive = BeginInstanceDragGesture<PropDragTraits>(
            manualPropDrag.state, *manualPropDrag.props,
            manualPropDrag.layers != nullptr ? *manualPropDrag.layers : kNoPropLayers,
            *manualPropDrag.geometry, manualPropDrag.recipe->globalSymmetryMask,
            manualPropDrag.recipe->radialSymmetryRepeatCount, hitGroupIndex, hitTransformIndex);
        return bManualPropDragActive;
    case PlacementCollectionKind_UI::Decals:
        if (manualDecalDrag.decals == nullptr || manualDecalDrag.geometry == nullptr
            || manualDecalDrag.recipe == nullptr) return false;
        bManualDecalDragActive = BeginInstanceDragGesture<DecalDragTraits>(
            manualDecalDrag.state, *manualDecalDrag.decals,
            manualDecalDrag.layers != nullptr ? *manualDecalDrag.layers : kNoDecalLayers,
            *manualDecalDrag.geometry, manualDecalDrag.recipe->globalSymmetryMask,
            manualDecalDrag.recipe->radialSymmetryRepeatCount, hitGroupIndex, hitTransformIndex);
        return bManualDecalDragActive;
    default:
        return false;
    }
}

void MapCanvas::ContinueManualInstanceDrag(float regionLocalX, float regionLocalY) {
    if (composite == nullptr) return;
    const PreviewPixelCoordinate previewPixel = view.ResolvePreviewPixel(regionLocalX, regionLocalY);
    const PreviewComposite::PreviewWorldPoint worldPoint = composite->PreviewPixelToWorld(
        static_cast<float>(previewPixel.pixelX), static_cast<float>(previewPixel.pixelY));
    static const std::vector<Params::MarkerInstanceLayer> kNoMarkerLayers;
    static const std::vector<Params::PropInstanceLayer>   kNoPropLayers;
    static const std::vector<Params::DecalInstanceLayer>  kNoDecalLayers;

    if (bManualMarkerDragActive && manualMarkerDragMarkers != nullptr && manualMarkerDragGeometry != nullptr) {
        UpdateInstanceDragGesture<MarkerDragTraits>(manualMarkerDragState, *manualMarkerDragMarkers,
            manualMarkerDragLayers != nullptr ? *manualMarkerDragLayers : kNoMarkerLayers,
            *manualMarkerDragGeometry, worldPoint.worldX, worldPoint.worldZ);
    } else if (bManualPropDragActive && manualPropDrag.props != nullptr && manualPropDrag.geometry != nullptr) {
        UpdateInstanceDragGesture<PropDragTraits>(manualPropDrag.state, *manualPropDrag.props,
            manualPropDrag.layers != nullptr ? *manualPropDrag.layers : kNoPropLayers,
            *manualPropDrag.geometry, worldPoint.worldX, worldPoint.worldZ);
    } else if (bManualDecalDragActive && manualDecalDrag.decals != nullptr && manualDecalDrag.geometry != nullptr) {
        UpdateInstanceDragGesture<DecalDragTraits>(manualDecalDrag.state, *manualDecalDrag.decals,
            manualDecalDrag.layers != nullptr ? *manualDecalDrag.layers : kNoDecalLayers,
            *manualDecalDrag.geometry, worldPoint.worldX, worldPoint.worldZ);
    }
}

void MapCanvas::EndManualInstanceDrag() {
    if (bManualMarkerDragActive) {
        if (manualMarkerDragMarkers != nullptr && manualMarkerDragGeometry != nullptr)
            EndInstanceDragGesture<MarkerDragTraits>(manualMarkerDragState, *manualMarkerDragMarkers,
                                                     *manualMarkerDragGeometry);
        else
            manualMarkerDragState = MarkerDragGestureState{};
    } else if (bManualPropDragActive) {
        if (manualPropDrag.props != nullptr && manualPropDrag.geometry != nullptr)
            EndInstanceDragGesture<PropDragTraits>(manualPropDrag.state, *manualPropDrag.props,
                                                   *manualPropDrag.geometry);
        else
            manualPropDrag.state = InstanceDragGestureState{};
    } else if (bManualDecalDragActive) {
        if (manualDecalDrag.decals != nullptr && manualDecalDrag.geometry != nullptr)
            EndInstanceDragGesture<DecalDragTraits>(manualDecalDrag.state, *manualDecalDrag.decals,
                                                    *manualDecalDrag.geometry);
        else
            manualDecalDrag.state = InstanceDragGestureState{};
    }
    bManualMarkerDragActive = false; bManualPropDragActive = false; bManualDecalDragActive = false;
}

} // namespace Ui
} // namespace SanmapGen
