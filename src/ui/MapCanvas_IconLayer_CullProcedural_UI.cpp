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
#include "../params/Army_PARAMS.h"
#include "../params/MapRecipe_PARAMS.h"

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
        float scale = instances.scaleX[static_cast<std::size_t>(instanceIndex)];   // was const
        const std::string templateIdentifier =
            TemplateIdentifierToString8(instances.templateIdentifier[static_cast<std::size_t>(instanceIndex)].characters);
        float tintRed = 1.0f, tintGreen = 1.0f, tintBlue = 1.0f;
        if (collection == PlacementCollectionKind_UI::Markers && input.recipe != nullptr) {
            const Params::MarkerCategory category =
                static_cast<Params::MarkerCategory>(instances.category[static_cast<std::size_t>(instanceIndex)]);
            ResolveMarkerCategoryTintColor(category, input.recipe->globalMarkerSettings, tintRed, tintGreen, tintBlue);
            scale *= ResolveMarkerCategoryScale(category, input.recipe->globalMarkerSettings);   // STEP122
        } else if (collection == PlacementCollectionKind_UI::Units && input.recipe != nullptr) {
            // ARCH_14_16_PerArmyUnitsOverlayRows.md §14.16-C/B: Data::PlacementInstance::armyIndex
            // (Placement_Rules_PROC.cpp -> Placement_Emit_PROC.cpp) is the real, per-instance owning
            // army — resolve its color directly, same defensive bounds floor as everywhere else this
            // ticket touches armyIndex (corrupt/out-of-range data draws white rather than crashing).
            const int unitArmyIndex = instances.armyIndex[static_cast<std::size_t>(instanceIndex)];
            if (unitArmyIndex >= 0 && static_cast<std::size_t>(unitArmyIndex) < input.recipe->armies.size()) {
                const Params::Army& army = input.recipe->armies[static_cast<std::size_t>(unitArmyIndex)];
                tintRed = army.armyColor[0]; tintGreen = army.armyColor[1]; tintBlue = army.armyColor[2];
            }
        }
        EmitCandidateIfVisible(input, layer, layerIndex, templateIdentifier, worldX, worldZ, scale,
                               collection, instanceIndex, tintRed, tintGreen, tintBlue,
                               stableOrderCounter, diagnostics, outCandidates);
    }
}

} // namespace Ui
} // namespace SanmapGen
