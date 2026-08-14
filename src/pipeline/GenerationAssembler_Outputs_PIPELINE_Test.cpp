// GenerationAssembler_Outputs_PIPELINE_Test.cpp — the downstream half of the M3-8 end-to-end
// acceptance test: the simulated fields (erosion / thermal / flow-accumulation), the placement
// instances and the baked texture set, plus two control pipelines that isolate what the mask
// gate and the two slump/carve passes actually contributed — so "the stage ran" is proved by
// its effect on the output, not merely by its counter.
#include "GenerationAssembler_TestScene_PIPELINE.h"
#include <cstdio>

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
// different stratum field is proof the gate reached the output rather than being overwritten.
void CheckMaskGateReachedOutput(const Pipeline::GenerationAssembler& assembler,
                                const Params::MapRecipe& recipe) {
    Pipeline::GenerationAssembler control(recipe);
    ConfigureStages(control);
    control.StratumMaskSettings()[detailStratumIndex].bSlopeGateEnabled = false;
    control.Run();
    const unsigned long long gated = FieldChecksum(assembler.Fields().materialMasks[detailStratumIndex]);
    const unsigned long long ungated = FieldChecksum(control.Fields().materialMasks[detailStratumIndex]);
    AssemblerCheck(gated != ungated, "the mask stage's slope gate changed the stratum weights");
}

} // namespace

void RunOutputChecks(Pipeline::GenerationAssembler& assembler, const Params::MapRecipe& recipe) {
    CheckSimulationFields(assembler);
    CheckPlacement(assembler.Placements());
    CheckBake(assembler.BakedTextures());
    CheckSimulationChangedTerrain(assembler, recipe);
    CheckMaskGateReachedOutput(assembler, recipe);
}
