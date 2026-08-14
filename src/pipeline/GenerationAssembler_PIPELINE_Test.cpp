// GenerationAssembler_PIPELINE_Test.cpp — the M3-8 end-to-end acceptance test. From a
// MapRecipe and an empty MapFields, ONE Run() must produce a populated heightfield -> masks
// -> eroded/thermal/flow -> placement -> bake, with sane values. This file owns the harness,
// the stage-order check and the two field halves it names first (heightfield, masks); the
// downstream outputs live in GenerationAssembler_Outputs_PIPELINE_Test.cpp and the dirty-hash
// contract in GenerationAssembler_Dirty_PIPELINE_Test.cpp (ARCH §1.5 file ceilings).
#include "GenerationAssembler_TestScene_PIPELINE.h"
#include <cstdio>

using namespace SanmapGen;
using namespace AssemblerTest;

static int failures = 0;
void AssemblerCheck(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL: %s\n", label); ++failures; }
}
void RunOutputChecks(Pipeline::GenerationAssembler& assembler, const Params::MapRecipe& recipe);
void RunDirtyHashChecks(Pipeline::GenerationAssembler& assembler, Params::MapRecipe& recipe);

namespace {

const char* const expectedStageOrder[7] = { "NoiseBlend", "Mask", "Erosion", "Thermal",
                                            "FlowAccumulation", "Placement", "Bake" };

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

// Every stage from the blend onward maintains the per-cell weight partition, so a broken
// producer shows up as a column that no longer sums to one (MASKING_SPEC, LAYER_SYSTEM_SPEC).
void CheckMaterialMasks(const Data::MapFields& fields) {
    int populatedStrata = 0;
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum) {
        float stratumMaximum = 0.0f;
        for (std::size_t index = 0; index < fields.materialMasks[stratum].CellCount(); ++index)
            if (fields.materialMasks[stratum].Data()[index] > stratumMaximum)
                stratumMaximum = fields.materialMasks[stratum].Data()[index];
        if (stratumMaximum > 0.05f) ++populatedStrata;
    }
    float worstSum = 1.0f, worstError = 0.0f;
    for (int y = 0; y < vertexSize; ++y)
        for (int x = 0; x < vertexSize; ++x) {
            float sum = 0.0f;
            for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum)
                sum += fields.materialMasks[stratum].Get(x, y);
            const float error = sum > 1.0f ? sum - 1.0f : 1.0f - sum;
            if (error > worstError) { worstError = error; worstSum = sum; }
        }
    AssemblerCheck(populatedStrata >= 2, "at least two strata carry mask weight");
    AssemblerCheck(worstError < 0.1f, "material mask weights stay normalized");
    std::printf("populated strata=%d worst mask sum=%.4f\n", populatedStrata, worstSum);
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
    CheckMaterialMasks(assembler.Fields());
    RunOutputChecks(assembler, recipe);
    RunDirtyHashChecks(assembler, recipe);

    if (failures == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failures);
    return 1;
}
