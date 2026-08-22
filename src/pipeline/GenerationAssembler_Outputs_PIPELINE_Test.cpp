// GenerationAssembler_Outputs_PIPELINE_Test.cpp — the downstream half of the M3-8 end-to-end
// acceptance test: the simulated fields (erosion / thermal / flow-accumulation), the placement
// instances and the baked texture set, plus two control pipelines that isolate what the mask
// gate and the two slump/carve passes actually contributed — so "the stage ran" is proved by
// its effect on the output, not merely by its counter.
#include "GenerationAssembler_TestScene_PIPELINE.h"
#include <cstdint>
#include <cstdio>
#include <vector>

using namespace SanmapGen;
using namespace AssemblerTest;

void AssemblerCheck(bool bCondition, const char* label);   // GenerationAssembler_PIPELINE_Test.cpp

namespace {

void CheckSimulationFields(Pipeline::GenerationAssembler& assembler) {
    AssemblerCheck(assembler.Erosion().ProcessedLayerCount() == 1, "erosion ran its one enabled layer");
    AssemblerCheck(assembler.Erosion().LastDropletCount() == erosionDropletCount,
                   "erosion traced every requested droplet");
    AssemblerCheck(assembler.Thermal().CompletedIterationCount() == thermalIterationCount,
                   "thermal ran every requested sweep");
    float accumulationMaximum = 0.0f, flowMaximum = 0.0f;
    const Data::MapFields& fields = assembler.Fields();
    for (std::size_t index = 0; index < fields.accumulation.CellCount(); ++index) {
        if (fields.accumulation.Data()[index] > accumulationMaximum)
            accumulationMaximum = fields.accumulation.Data()[index];
        if (fields.flow.Data()[index] > flowMaximum) flowMaximum = fields.flow.Data()[index];
    }
    AssemblerCheck(accumulationMaximum > 10.0f, "drainage accumulates down the flow graph");
    AssemblerCheck(flowMaximum > 0.0f, "flow magnitude is written");
    std::printf("accumulation max=%.1f flow max=%.4f sinks=%d\n", accumulationMaximum, flowMaximum,
                assembler.FlowAccumulation().SinkCount());
}

void CheckPlacement(const Data::PlacementResults& placements) {
    AssemblerCheck(placements.markers.Count() == static_cast<std::size_t>(markerCount),
                   "the spawn rule placed exactly its count");
    AssemblerCheck(placements.props.Count() > 10, "the prop rule scattered a population");
    bool bInsideMap = true;
    for (std::size_t index = 0; index < placements.props.Count(); ++index) {
        const float positionX = placements.props.positionX[index];
        const float positionZ = placements.props.positionZ[index];
        if (positionX < 0.0f || positionZ < 0.0f) bInsideMap = false;
        if (positionX > static_cast<float>(vertexSize) || positionZ > static_cast<float>(vertexSize))
            bInsideMap = false;
    }
    AssemblerCheck(bInsideMap, "every placed instance sits inside the map");
    std::printf("markers=%zu props=%zu\n", placements.markers.Count(), placements.props.Count());
}

// STEP50: the CSR bucket index is derived infrastructure over placementResults, built at the same
// lifecycle point as markerSpatialGrid (right after Placement). This is the one integration check
// that proves the PIPELINE wiring actually fires, not just the standalone type (that is
// RuleBucketIndex_DATA_Test.cpp's job). AssemblerTest::MakeRecipe registers exactly 1 marker rule
// (inside 1 markerRuleLayer) and 1 prop rule, and 0 unit/decal rules.
void CheckRuleBucketIndex(const Pipeline::GenerationAssembler& assembler) {
    const Data::RuleBucketIndexSet& buckets = assembler.RuleBucketIndex();
    const Data::PlacementResults& placements = assembler.Placements();

    const std::int32_t markersBegin = buckets.markers.BucketBegin(0);
    const std::int32_t markersEnd   = buckets.markers.BucketEnd(0);
    AssemblerCheck(buckets.markers.BucketCount() == 1, "one marker rule registered -> one bucket");
    AssemblerCheck(markersEnd - markersBegin == static_cast<std::int32_t>(placements.markers.Count()),
                   "the single marker bucket contains every placed marker");
    std::vector<bool> markerSeen(placements.markers.Count(), false);
    for (std::int32_t position = markersBegin; position < markersEnd; ++position) {
        const std::int32_t instance = buckets.markers.InstanceIndexAt(position);
        AssemblerCheck(instance >= 0 && static_cast<std::size_t>(instance) < placements.markers.Count(),
                       "marker bucket entries resolve inside the markers collection");
        markerSeen[static_cast<std::size_t>(instance)] = true;
    }
    bool bEveryMarkerInBucket = true;
    for (bool seen : markerSeen) if (!seen) bEveryMarkerInBucket = false;
    AssemblerCheck(bEveryMarkerInBucket, "no marker index sits outside the single bucket");

    const std::int32_t propsBegin = buckets.props.BucketBegin(0);
    const std::int32_t propsEnd   = buckets.props.BucketEnd(0);
    AssemblerCheck(buckets.props.BucketCount() == 1, "one prop rule registered -> one bucket");
    AssemblerCheck(propsEnd - propsBegin == static_cast<std::int32_t>(placements.props.Count()),
                   "the single prop bucket contains every placed prop");
    std::vector<bool> propSeen(placements.props.Count(), false);
    for (std::int32_t position = propsBegin; position < propsEnd; ++position) {
        const std::int32_t instance = buckets.props.InstanceIndexAt(position);
        AssemblerCheck(instance >= 0 && static_cast<std::size_t>(instance) < placements.props.Count(),
                       "prop bucket entries resolve inside the props collection");
        propSeen[static_cast<std::size_t>(instance)] = true;
    }
    bool bEveryPropInBucket = true;
    for (bool seen : propSeen) if (!seen) bEveryPropInBucket = false;
    AssemblerCheck(bEveryPropInBucket, "no prop index sits outside the single bucket");

    AssemblerCheck(buckets.units.IsEmpty() && buckets.units.BucketCount() == 0,
                   "zero unit rules registered -> empty units index (real bucketTotal==0 path)");
    AssemblerCheck(buckets.decals.IsEmpty() && buckets.decals.BucketCount() == 0,
                   "zero decal rules registered -> empty decals index (real bucketTotal==0 path)");
}

void CheckBake(const Proc::BakedTextureSet& textures) {
    AssemblerCheck(textures.IsSized(), "the bake produced a sized texture set");
    AssemblerCheck(textures.resolution == mapSize * 2, "bake resolution follows the multiplier");
    const unsigned int firstTexel = textures.compositeAlbedo.empty() ? 0u : textures.compositeAlbedo[0];
    bool bVaries = false, bOpaque = true;
    for (unsigned int texel : textures.compositeAlbedo) {
        if (texel != firstTexel) bVaries = true;
        if ((texel >> 24) != 255u) bOpaque = false;
    }
    AssemblerCheck(bVaries, "the composite albedo varies across the map");
    AssemblerCheck(bOpaque, "the composite albedo is written opaque");
    bool bMaskTextureWritten = false;
    for (unsigned int texel : textures.stratumMaskLow)
        if (texel != 0u) { bMaskTextureWritten = true; break; }
    AssemblerCheck(bMaskTextureWritten, "the packed stratum-mask texture is written");
}

// Thermal relaxation can only ever REDUCE the steepest neighbour drop, so a control with the
// carve/slump passes disabled bounds the simulated result from above.
void CheckSimulationChangedTerrain(const Pipeline::GenerationAssembler& assembler,
                                   const Params::MapRecipe& recipe) {
    Pipeline::GenerationAssembler control(recipe);
    ConfigureStages(control);
    control.Erosion().LayerSettings(0).bEnabled = false;
    control.Thermal().Constants().iterationCount = 0;
    control.Run();
    const float simulatedDrop = MaximumNeighbourDrop(assembler.Fields().heightfield);
    const float rawDrop       = MaximumNeighbourDrop(control.Fields().heightfield);
    AssemblerCheck(simulatedDrop < rawDrop, "erosion + thermal reshaped the raw blended terrain");
    std::printf("max neighbour drop simulated=%.5f raw=%.5f\n", simulatedDrop, rawDrop);
}

// The mask stage's slope gate is the only difference between these two pipelines, so a
// different SURFACE-WEIGHT field is proof the gate reached the output — while an IDENTICAL
// PROPORTION field is proof the gate never touched the physical field the sims own. That
// second assertion is the regression guard for the in-place-overwrite defect (ARCH §7.2).
void CheckMaskGateReachedOutput(const Pipeline::GenerationAssembler& assembler,
                                const Params::MapRecipe& recipe) {
    Params::MapRecipe controlRecipe = recipe;
    controlRecipe.strata[detailStratumIndex].bSlopeGateEnabled = false;
    Pipeline::GenerationAssembler control(controlRecipe);
    ConfigureStages(control);
    control.Run();

    const unsigned long long gatedWeights =
        FieldChecksum(assembler.Fields().surfaceStratumWeights[detailStratumIndex]);
    const unsigned long long ungatedWeights =
        FieldChecksum(control.Fields().surfaceStratumWeights[detailStratumIndex]);
    AssemblerCheck(gatedWeights != ungatedWeights,
                   "the mask stage's slope gate changed the surface stratum weights");

    const unsigned long long gatedProportions =
        FieldChecksum(assembler.Fields().materialProportions[detailStratumIndex]);
    const unsigned long long ungatedProportions =
        FieldChecksum(control.Fields().materialProportions[detailStratumIndex]);
    AssemblerCheck(gatedProportions == ungatedProportions,
                   "the slope gate left materialProportions identical (single writer)");
}

} // namespace

void RunOutputChecks(Pipeline::GenerationAssembler& assembler, const Params::MapRecipe& recipe) {
    CheckSimulationFields(assembler);
    CheckPlacement(assembler.Placements());
    CheckRuleBucketIndex(assembler);
    CheckBake(assembler.BakedTextures());
    CheckSimulationChangedTerrain(assembler, recipe);
    CheckMaskGateReachedOutput(assembler, recipe);
}
