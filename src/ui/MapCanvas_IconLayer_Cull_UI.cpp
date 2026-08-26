// MapCanvas_IconLayer_Cull_UI.cpp — §1's top-level orchestration: the once-per-frame view-world
// rectangle, the per-layer world-AABB cache, and the per-frame candidate walk over `overlayLayers`
// in vector order (Z order, §14.2/§14.7). Layer: UI. Pure, imgui-free, headless-testable — no
// imgui, no GL, only STEP47's projection functions and STEP50's CSR bucket index.
//
// The per-layer AABB (§1 item 2) is maintained HERE, not in STEP50's DATA-layer index — STEP50 as
// actually shipped (RuleBucketIndex_DATA.h) is a pure ruleIndex CSR with no spatial/AABB concept at
// all, and this ticket's own scope excludes touching STEP50/PIPELINE files. So the "cached, rebuilt
// only when a layer's own sub-layer membership changes" AABB is a UI-owned cache
// (IconLayerAabbCache_UI), gated on OverlayLayerSettings::layerSettingsRevision — the same counter
// the C2 cache (MapCanvas_IconLayer_Cache_UI.cpp) invalidates on. Nothing here mutates or discards
// Data::PlacementInstances/Data::SpatialGrid/any CSR bucket (§14.11).
#include "MapCanvas_IconLayer_CullInternal_UI.h"
#include "MapCanvasView_UI.h"
#include "PreviewComposite_UI.h"
#include "../params/MapRecipe_PARAMS.h"
#include <limits>

namespace SanmapGen {
namespace Ui {

ViewWorldRect_UI ComputeViewWorldRect(const PreviewComposite& composite, const MapCanvasView& view,
                                      float regionSidePixels) {
    const PreviewPixelCoordinate lowPixel  = view.ResolvePreviewPixel(0.0f, 0.0f);
    const PreviewPixelCoordinate highPixel = view.ResolvePreviewPixel(regionSidePixels, regionSidePixels);
    const PreviewComposite::PreviewWorldPoint lowWorld  = composite.PreviewPixelToWorld(
        static_cast<float>(lowPixel.pixelX), static_cast<float>(lowPixel.pixelY));
    const PreviewComposite::PreviewWorldPoint highWorld = composite.PreviewPixelToWorld(
        static_cast<float>(highPixel.pixelX), static_cast<float>(highPixel.pixelY));
    ViewWorldRect_UI rect;
    rect.lowWorldX  = lowWorld.worldX < highWorld.worldX ? lowWorld.worldX : highWorld.worldX;
    rect.highWorldX = lowWorld.worldX < highWorld.worldX ? highWorld.worldX : lowWorld.worldX;
    rect.lowWorldZ  = lowWorld.worldZ < highWorld.worldZ ? lowWorld.worldZ : highWorld.worldZ;
    rect.highWorldZ = lowWorld.worldZ < highWorld.worldZ ? highWorld.worldZ : lowWorld.worldZ;
    return rect;
}

namespace {

// One layer's AABB, position-only (viewRect == nullptr — see the internal header's contract).
LayerWorldAabb_UI BuildOneLayerAabb(const DrawOverlayIconLayersInput& input, const OverlayLayer_UI& layer,
                                    int layerIndex) {
    LayerWorldAabb_UI aabb;
    std::vector<OverlayVisibleInstance> discarded;   // AABB-only pass never appends
    int stableOrderCounter = 0;
    for (const OverlaySubLayerRef_UI& subLayerRef : layer.subLayers) {
        if (!subLayerRef.bEnabled) continue;
        if (subLayerRef.kind == OverlaySubLayerKind_UI::ProceduralRule) {
            PlacementCollectionKind_UI collection;
            if (!TryResolveDomainCollection(layer.domainKind, collection)) continue;
            ResolveProceduralSubLayer(input, layer, layerIndex, collection, subLayerRef.index,
                                      &stableOrderCounter, &aabb, nullptr, nullptr, discarded);
        } else {
            ResolveManualSubLayer(input, layer, layerIndex, subLayerRef.index, &stableOrderCounter,
                                  &aabb, nullptr, nullptr, discarded);
        }
    }
    return aabb;
}

} // namespace

void EnsureLayerAabbCache(const DrawOverlayIconLayersInput& input, IconLayerAabbCache_UI& aabbCache) {
    // Defensive against direct (test) calls, not just the ResolveVisibleCandidates path, which
    // already checks these before calling here.
    if (input.overlayLayerSettings == nullptr || input.placements == nullptr || input.ruleBucketIndex == nullptr) {
        aabbCache.perLayerAabb.clear();
        return;
    }
    const std::vector<OverlayLayer_UI>& layers = input.overlayLayerSettings->overlayLayers;
    const std::uint64_t revision = input.overlayLayerSettings->layerSettingsRevision;
    if (aabbCache.cachedForRevision == revision && aabbCache.perLayerAabb.size() == layers.size()) return;
    aabbCache.perLayerAabb.resize(layers.size());
    for (std::size_t layerIndex = 0; layerIndex < layers.size(); ++layerIndex)
        aabbCache.perLayerAabb[layerIndex] = BuildOneLayerAabb(input, layers[layerIndex], static_cast<int>(layerIndex));
    aabbCache.cachedForRevision = revision;
}

void ResolveVisibleCandidates(const DrawOverlayIconLayersInput& input, IconLayerAabbCache_UI& aabbCache,
                              IconLayerCullDiagnostics_UI* diagnostics,
                              std::vector<OverlayVisibleInstance>& outCandidates) {
    if (input.overlayLayerSettings == nullptr || input.composite == nullptr || input.view == nullptr
        || input.placements == nullptr || input.ruleBucketIndex == nullptr) return;
    EnsureLayerAabbCache(input, aabbCache);
    const ViewWorldRect_UI viewRect = ComputeViewWorldRect(*input.composite, *input.view, input.regionSidePixels);
    const std::vector<OverlayLayer_UI>& layers = input.overlayLayerSettings->overlayLayers;
    int stableOrderCounter = 0;
    for (std::size_t layerIndex = 0; layerIndex < layers.size(); ++layerIndex) {
        const OverlayLayer_UI& layer = layers[layerIndex];
        if (!layer.bEnabled) continue;
        if (layerIndex < aabbCache.perLayerAabb.size() && aabbCache.perLayerAabb[layerIndex].bValid
            && !WorldRectsIntersect(aabbCache.perLayerAabb[layerIndex], viewRect))
            continue;   // §1 item 2 — layer AABB early-out; no sub-layer walk fires below
        for (const OverlaySubLayerRef_UI& subLayerRef : layer.subLayers) {
            if (!subLayerRef.bEnabled) continue;
            if (subLayerRef.kind == OverlaySubLayerKind_UI::ProceduralRule) {
                PlacementCollectionKind_UI collection;
                if (!TryResolveDomainCollection(layer.domainKind, collection)) continue;
                ResolveProceduralSubLayer(input, layer, static_cast<int>(layerIndex), collection,
                                          subLayerRef.index, &stableOrderCounter, nullptr, &viewRect,
                                          diagnostics, outCandidates);
            } else {
                ResolveManualSubLayer(input, layer, static_cast<int>(layerIndex), subLayerRef.index,
                                      &stableOrderCounter, nullptr, &viewRect, diagnostics, outCandidates);
            }
        }
    }
}

namespace {

// ARCH §19.25 — the manual-marker half of ResolveSelectedInstanceCandidate: `input.selectedInstanceKey.
// instanceIndex` is a MarkerTransform::instanceIdentifier here, NOT a Data::PlacementInstances array
// position (the procedural branch below's own assumption), so it cannot walk `input.placements->
// markers` — it must re-run the same manual-marker resolution ResolveMarkersManual already performs
// for a full cull pass, scoped to the ONE selected instanceIdentifier. The viewRect passed is
// deliberately unrestricted (not `ComputeViewWorldRect`, which this replay-frame path never
// recomputes): the selected instance keeps rendering even if the view moved without invalidating the
// cache, mirroring the procedural branch's own "no view-membership re-test" behavior below.
bool ResolveSelectedManualMarkerCandidate(const DrawOverlayIconLayersInput& input,
                                          std::vector<OverlayVisibleInstance>& outCandidates) {
    if (input.overlayLayerSettings == nullptr || input.recipe == nullptr) return false;
    const int targetInstanceIdentifier = input.selectedInstanceKey.instanceIndex;
    const ViewWorldRect_UI unrestrictedRect{
        std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::max(),    std::numeric_limits<float>::max() };
    const std::vector<OverlayLayer_UI>& layers = input.overlayLayerSettings->overlayLayers;
    for (std::size_t layerIndex = 0; layerIndex < layers.size(); ++layerIndex) {
        const OverlayLayer_UI& layer = layers[layerIndex];
        if (!layer.bEnabled || (layer.domainKind != OverlayDomainKind_UI::Alloy
                              && layer.domainKind != OverlayDomainKind_UI::SpawnsArmies))
            continue;
        for (const OverlaySubLayerRef_UI& subLayerRef : layer.subLayers) {
            if (!subLayerRef.bEnabled || subLayerRef.kind == OverlaySubLayerKind_UI::ProceduralRule) continue;
            int stableOrderCounter = 0;
            ResolveMarkersManual(input, layer, static_cast<int>(layerIndex), subLayerRef.index,
                                 &stableOrderCounter, nullptr, &unrestrictedRect, nullptr, outCandidates,
                                 &targetInstanceIdentifier);
            if (!outCandidates.empty()) return true;
        }
    }
    return false;
}

} // namespace

bool ResolveSelectedInstanceCandidate(const DrawOverlayIconLayersInput& input,
                                      std::vector<OverlayVisibleInstance>& outCandidates) {
    if (!input.selectedInstanceKey.bValid || input.overlayLayerSettings == nullptr || input.placements == nullptr)
        return false;
    if (input.selectedInstanceKey.collection != PlacementCollectionKind_UI::Markers)
        return false;   // no other domain has a working picker yet (STEP48)
    if (input.selectedInstanceKey.bManual)
        return ResolveSelectedManualMarkerCandidate(input, outCandidates);   // ARCH §19.25
    const Data::PlacementInstances& markers = input.placements->markers;
    const std::int32_t instanceIndex = input.selectedInstanceKey.instanceIndex;
    if (instanceIndex < 0 || static_cast<std::size_t>(instanceIndex) >= markers.Count()) return false;
    const int ruleIndex = markers.ruleIndex[static_cast<std::size_t>(instanceIndex)];
    const std::vector<OverlayLayer_UI>& layers = input.overlayLayerSettings->overlayLayers;
    for (std::size_t layerIndex = 0; layerIndex < layers.size(); ++layerIndex) {
        const OverlayLayer_UI& layer = layers[layerIndex];
        if (!layer.bEnabled || (layer.domainKind != OverlayDomainKind_UI::Alloy
                              && layer.domainKind != OverlayDomainKind_UI::SpawnsArmies))
            continue;
        for (const OverlaySubLayerRef_UI& subLayerRef : layer.subLayers) {
            if (!subLayerRef.bEnabled || subLayerRef.kind != OverlaySubLayerKind_UI::ProceduralRule
                || subLayerRef.index != ruleIndex)
                continue;
            const std::size_t index = static_cast<std::size_t>(instanceIndex);
            const std::string templateIdentifier = TemplateIdentifierToString8(markers.templateIdentifier[index].characters);
            float tintRed = 1.0f, tintGreen = 1.0f, tintBlue = 1.0f;
            if (input.recipe != nullptr) {
                const Params::MarkerCategory category =
                    static_cast<Params::MarkerCategory>(markers.category[index]);
                ResolveMarkerCategoryTintColor(category, input.recipe->globalMarkerSettings, tintRed, tintGreen, tintBlue);
            }
            int stableOrderCounter = 0;
            EmitCandidateIfVisible(input, layer, static_cast<int>(layerIndex), templateIdentifier,
                                   markers.positionX[index], markers.positionZ[index], markers.scaleX[index],
                                   PlacementCollectionKind_UI::Markers, instanceIndex, tintRed, tintGreen, tintBlue,
                                   &stableOrderCounter, nullptr, outCandidates);
            return !outCandidates.empty();
        }
    }
    return false;
}

} // namespace Ui
} // namespace SanmapGen
