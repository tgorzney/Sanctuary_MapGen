// GenerationAssembler_PIPELINE_Test.cpp — the M3-8 end-to-end acceptance test. From a
// MapRecipe and an empty MapFields, ONE Run() must produce a populated heightfield ->
// proportions (sims) -> surface stratum weights (Mask) -> placement -> bake, with sane values.
// This file owns the harness, the ratified stage-order check and the three field families it
// names first (heightfield, proportions, surface weights); the downstream outputs live in
// GenerationAssembler_Outputs_PIPELINE_Test.cpp and the dirty-hash contract in
// GenerationAssembler_Dirty_PIPELINE_Test.cpp (ARCH §1.5 file ceilings).
#include "GenerationAssembler_TestScene_PIPELINE.h"
#include <cmath>
#include <cstdio>

using namespace SanmapGen;
using namespace AssemblerTest;

static int failures = 0;
void AssemblerCheck(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL: %s\n", label); ++failures; }
}
void RunOutputChecks(Pipeline::GenerationAssembler& assembler, const Params::MapRecipe& recipe);
void RunDirtyHashChecks(Pipeline::GenerationAssembler& assembler, Params::MapRecipe& recipe);
void RunGatingChecks();

namespace {

// ARCH 7.4: Mask runs after EVERY sim and before Placement/Bake.
const char* const expectedStageOrder[7] = { "NoiseBlend", "Erosion", "Thermal",
                                            "FlowAccumulation", "Mask", "Placement", "Bake" };

bool RanInDeclaredOrder(const std::vector<std::string>& ran) {
    if (ran.size() != 7) return false;
    for (std::size_t index = 0; index < 7; ++index)
        if (ran[index] != expectedStageOrder[index]) return false;
    return true;
}

void CheckHeightfield(const Data::MapFields& fields) {
    AssemblerCheck(fields.IsSized() && fields.VertexSize() == vertexSize, "fields sized from geometry");
    float minimum = 1.0e30f, maximum = -1.0e30f;
    bool bFinite = true;
    for (std::size_t index = 0; index < fields.heightfield.CellCount(); ++index) {
        const float height = fields.heightfield.Data()[index];
        if (!(height == height) || height < -1.0e6f || height > 1.0e6f) bFinite = false;
        if (height < minimum) minimum = height;
        if (height > maximum) maximum = height;
    }
    AssemblerCheck(bFinite, "heightfield holds only finite values");
    AssemblerCheck(minimum >= 0.0f && maximum <= 1.0f, "heightfield stays inside its clamp window");
    AssemblerCheck(maximum - minimum > 0.05f, "heightfield is populated, not flat");
    std::printf("height min=%.4f max=%.4f\n", minimum, maximum);
}

// The PHYSICAL field: every sim maintains the per-cell partition, so a broken producer shows
// up as a column that no longer sums to one (ARCH §7.2, LAYER_SYSTEM_SPEC).
void CheckMaterialProportions(const Data::MapFields& fields) {
    int populatedStrata = 0;
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum) {
        float stratumMaximum = 0.0f;
        for (std::size_t index = 0; index < fields.materialProportions[stratum].CellCount(); ++index)
            if (fields.materialProportions[stratum].Data()[index] > stratumMaximum)
                stratumMaximum = fields.materialProportions[stratum].Data()[index];
        if (stratumMaximum > 0.05f) ++populatedStrata;
    }
    float worstSum = 1.0f, worstError = 0.0f;
    for (int y = 0; y < vertexSize; ++y)
        for (int x = 0; x < vertexSize; ++x) {
            float sum = 0.0f;
            for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum)
                sum += fields.materialProportions[stratum].Get(x, y);
            const float error = sum > 1.0f ? sum - 1.0f : 1.0f - sum;
            if (error > worstError) { worstError = error; worstSum = sum; }
        }
    AssemblerCheck(populatedStrata >= 2, "at least two strata carry material");
    AssemblerCheck(worstError < 0.1f, "material proportions stay normalized");
    std::printf("populated strata=%d worst proportion sum=%.4f\n", populatedStrata, worstSum);
}

// The VISIBLE field: Mask must have written it, and it must NOT be a copy of the proportions —
// the slope gate on the detail stratum is exactly what separates the two (ARCH §7.2).
void CheckSurfaceStratumWeights(const Data::MapFields& fields) {
    bool bWritten = false, bInRange = true, bDiffersFromProportions = false;
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum)
        for (int y = 0; y < vertexSize; ++y)
            for (int x = 0; x < vertexSize; ++x) {
                const float weight = fields.surfaceStratumWeights[stratum].Get(x, y);
                if (weight > 0.05f) bWritten = true;
                if (weight < 0.0f || weight > 1.0f) bInRange = false;
                if (std::fabs(weight - fields.materialProportions[stratum].Get(x, y)) > 1e-6f)
                    bDiffersFromProportions = true;
            }
    AssemblerCheck(bWritten, "the mask stage populated surfaceStratumWeights");
    AssemblerCheck(bInRange, "surface stratum weights stay inside [0,1]");
    AssemblerCheck(bDiffersFromProportions,
                   "surface weights are gated, not a copy of the material proportions");
}

} // namespace

int main() {
    Params::MapRecipe recipe = MakeRecipe(4242u);
    Pipeline::GenerationAssembler assembler(recipe);
    ConfigureStages(assembler);
    AssemblerCheck(assembler.StageCount() == 7, "seven stages registered");
    AssemblerCheck(!assembler.Fields().IsSized(), "MapFields starts empty");

    const std::vector<std::string> firstRun = assembler.Run();
    AssemblerCheck(RanInDeclaredOrder(firstRun), "first Run() runs all seven stages in order");
    AssemblerCheck(assembler.DidLastRunRegenerate(), "a full regeneration is reported");
    CheckHeightfield(assembler.Fields());
    CheckMaterialProportions(assembler.Fields());
    CheckSurfaceStratumWeights(assembler.Fields());
    RunOutputChecks(assembler, recipe);
    RunDirtyHashChecks(assembler, recipe);
    RunGatingChecks();

    if (failures == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failures);
    return 1;
}
