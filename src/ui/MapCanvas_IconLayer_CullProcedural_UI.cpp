// MapCanvas_IconLayer_CullProcedural_UI.cpp — §1 item 3: one procedural sub-layer's candidate
// instances, walked via STEP50's ruleIndex CSR bucket (Data::RuleBucketIndex). Layer: UI. Pure,
// imgui-free, headless-testable.
//
// STEP50 boundary resolution (flagged by this ticket as open — resolved here against STEP50's
// REAL shipped shape, read directly): RuleBucketIndex_DATA.h ships a pure ruleIndex CSR with no
// spatial-locality concept at all, and Data::SpatialGrid (GenerationAssembler_PIPELINE.h) is a
// SINGLE shared grid over the markers domain only (not one per layer, not one per Props/Units/
// Decals). So there is no "per-layer Query(worldMin, worldMax)" call to consume — every domain,
// including markers, resolves a sub-layer's candidates by walking its rule bucket and applying a
// per-instance world-rect test here, uniformly. This avoids special-casing markers onto a
// rule-agnostic grid it would then have to re-filter by rule membership anyway.
#include "MapCanvas_IconLayer_CullInternal_UI.h"

namespace SanmapGen {
namespace Ui {

void ResolveProceduralSubLayer(const DrawOverlayIconLayersInput& input, const OverlayLayer_UI& layer,
                               int layerIndex, PlacementCollectionKind_UI collection, int ruleIndex,
                               int* stableOrderCounter, LayerWorldAabb_UI* outAabb,
                               const ViewWorldRect_UI* viewRect,
                               IconLayerCullDiagnostics_UI* diagnostics,
                               std::vector<OverlayVisibleInstance>& outCandidates) {
    if (input.placements == nullptr || input.ruleBucketIndex == nullptr) return;
    if (diagnostics != nullptr) ++diagnostics->subLayerWalksIssued;
    const Data::PlacementInstances& instances = CollectionInstances(*input.placements, collection);
    const Data::RuleBucketIndex& ruleBucket = CollectionRuleBucket(*input.ruleBucketIndex, collection);
    const std::int32_t bucketBegin = ruleBucket.BucketBegin(ruleIndex);
    const std::int32_t bucketEnd   = ruleBucket.BucketEnd(ruleIndex);
    for (std::int32_t position = bucketBegin; position < bucketEnd; ++position) {
        const std::int32_t instanceIndex = ruleBucket.InstanceIndexAt(position);
        if (instanceIndex < 0 || static_cast<std::size_t>(instanceIndex) >= instances.Count()) continue;
        const float worldX = instances.positionX[static_cast<std::size_t>(instanceIndex)];
        const float worldZ = instances.positionZ[static_cast<std::size_t>(instanceIndex)];
        if (outAabb != nullptr) WidenAabb(*outAabb, worldX, worldZ);
        if (viewRect == nullptr) continue;   // AABB-only pass (see the internal header's contract)
        if (worldX < viewRect->lowWorldX || worldX > viewRect->highWorldX
            || worldZ < viewRect->lowWorldZ || worldZ > viewRect->highWorldZ)
            continue;
        const float scale = instances.scaleX[static_cast<std::size_t>(instanceIndex)];
        const std::string templateIdentifier =
            TemplateIdentifierToString8(instances.templateIdentifier[static_cast<std::size_t>(instanceIndex)].characters);
        EmitCandidateIfVisible(input, layer, layerIndex, templateIdentifier, worldX, worldZ, scale,
                               collection, instanceIndex, stableOrderCounter, diagnostics, outCandidates);
    }
}

} // namespace Ui
} // namespace SanmapGen
