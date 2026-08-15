// GenerationAssembler_Dirty_PIPELINE_Test.cpp — the dirty-hash half of the M3-8 acceptance
// test: a second Run() with nothing changed skips every stage; changing one layer's frequency
// re-runs the pipeline from that stage onward; changing a mid-pipeline stage's own constant
// re-runs only that stage and its downstream; and the two-tier stub reports a bake-only
// refresh as preview-only rather than a full regeneration.
#include "GenerationAssembler_TestScene_PIPELINE.h"
#include <cstdio>

using namespace SanmapGen;
using namespace AssemblerTest;

void AssemblerCheck(bool bCondition, const char* label);   // GenerationAssembler_PIPELINE_Test.cpp

namespace {

bool RanExactly(const std::vector<std::string>& ran, const char* const* expected, std::size_t count) {
    if (ran.size() != count) return false;
    for (std::size_t index = 0; index < count; ++index)
        if (ran[index] != expected[index]) return false;
    return true;
}

} // namespace

void RunDirtyHashChecks(Pipeline::GenerationAssembler& assembler, Params::MapRecipe& recipe) {
    const unsigned long long settledChecksum = FieldChecksum(assembler.Fields().heightfield);

    // --- nothing changed: every stage is skipped and no output moves.
    const std::vector<std::string> cleanRun = assembler.Run();
    AssemblerCheck(cleanRun.empty(), "an unchanged second Run() skips every stage");
    AssemblerCheck(!assembler.DidLastRunRegenerate(), "a skipped run reports no regeneration");
    AssemblerCheck(FieldChecksum(assembler.Fields().heightfield) == settledChecksum,
                   "a skipped run leaves the heightfield untouched");

    // --- one layer's frequency moves: the FIRST stage owns it, so the whole pipeline re-runs.
    static const char* const allStages[7] = { "NoiseBlend", "Erosion", "Thermal",
                                              "FlowAccumulation", "Mask", "Placement", "Bake" };
    recipe.layerStack.geoLayers[0].layers[0].frequency = 0.035f;
    const std::vector<std::string> frequencyRun = assembler.Run();
    AssemblerCheck(RanExactly(frequencyRun, allStages, 7),
                   "a layer frequency change re-runs from the noise stage onward");
    AssemblerCheck(FieldChecksum(assembler.Fields().heightfield) != settledChecksum,
                   "the re-run actually produced a different heightfield");
    AssemblerCheck(assembler.DidLastRunRegenerate(), "a terrain change is a full regeneration");

    // --- a mid-pipeline constant moves: that stage and everything BELOW it, nothing above.
    static const char* const flowOnward[4] = { "FlowAccumulation", "Mask", "Placement", "Bake" };
    assembler.FlowAccumulation().Constants().cellWeight = 2.0f;
    const std::vector<std::string> flowRun = assembler.Run();
    AssemblerCheck(RanExactly(flowRun, flowOnward, 4),
                   "a flow constant re-runs flow, mask, placement and bake only");

    // --- a MASK parameter moves: mask + its consumers, and NO sim re-runs. This is the proof
    // of ARCH 7.2/3.4 purity — the gate lives in its own output field, so nothing upstream of
    // Mask has to be replayed to keep the physical proportions correct.
    static const char* const maskOnward[3] = { "Mask", "Placement", "Bake" };
    recipe.strata[detailStratumIndex].maximumSlopeDegrees = 40.0f;
    const std::vector<std::string> maskRun = assembler.Run();
    AssemblerCheck(RanExactly(maskRun, maskOnward, 3),
                   "a mask parameter re-runs mask, placement and bake only (no sim replay)");

    // --- the last stage moves: only the bake, and the two-tier stub calls it preview-only.
    static const char* const bakeOnly[1] = { "Bake" };
    assembler.Bake().Constants().compositeAlphaValue = 0.9f;
    const std::vector<std::string> bakeRun = assembler.Run();
    AssemblerCheck(RanExactly(bakeRun, bakeOnly, 1), "a bake constant re-runs the bake only");
    AssemblerCheck(assembler.WasLastRunPreviewOnly(), "a bake-only refresh is preview-only");
    AssemblerCheck(!assembler.DidLastRunRegenerate(), "a bake-only refresh is not a regeneration");

    // --- the tier stub itself.
    AssemblerCheck(assembler.TierOfStage("Erosion") == Pipeline::RegenerationTier::FullRegeneration,
                   "erosion is a full-regeneration stage");
    AssemblerCheck(assembler.TierOfStage("Bake") == Pipeline::RegenerationTier::PreviewOnly,
                   "the bake is a preview-only stage");
    AssemblerCheck(assembler.StageDescriptions().size() == 7, "every stage carries a tier");

    // --- an explicit invalidation forces the whole pipeline again (resize / recipe reload).
    assembler.InvalidateAll();
    AssemblerCheck(RanExactly(assembler.Run(), allStages, 7), "InvalidateAll re-runs everything");
}
