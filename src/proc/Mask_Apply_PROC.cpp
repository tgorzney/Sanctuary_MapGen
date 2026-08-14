// Mask_Apply_PROC.cpp — the CPU accuracy path: slope-gate every stratum's procedural mask and
// merge the stored art into the same MaterialMasks field. Twin of Mask_PROC.glsl, expression
// for expression, through the shared Mask_Slope_PROC.h / Mask_Merge_PROC.h math.
#include "Mask_PROC.h"
#include "Mask_Slope_PROC.h"
#include "Mask_Merge_PROC.h"
#include "../sys/ThreadPool_SYS.h"

namespace SanmapGen {
namespace Proc {
namespace {

// One cell of one stratum: gate -> merge -> remap, written back in place. The stored art is
// sampled ONLY when a merge mode wants it (Disabled never pays for a resample).
void ApplyStratumCell(const MaskStratumConfiguration& configuration, const float* storedValues,
                      Data::FloatField& materialMask, float slopeGradient, int x, int y) {
    const float gateWeight = SlopeGateWeight(slopeGradient, configuration);
    const float proceduralWeight = materialMask.Get(x, y) * gateWeight;
    const float storedWeight = configuration.mergeMode == kMergeModeDisabled
                             ? 0.0f : SampleStoredMaskBilinear(storedValues, configuration, x, y);
    const float mergedWeight = MergeStoredMask(proceduralWeight, storedWeight, configuration);
    materialMask.Set(x, y, RemapMaskValue(mergedWeight, configuration));
}

} // namespace

void MaskStage::RunOnCpu() {
    PrepareRun();
    const int vertexSize = geometry.VertexSize();
    if (!mapFields.IsSized() || mapFields.VertexSize() != vertexSize) return;   // validate input

    const float* heightValues = mapFields.heightfield.Data();
    const float* storedValues = packedStoredMaskValues.data();
    const MaskStratumConfiguration& slopeConfiguration = stratumConfigurations[0];
    const auto maskRow = [&](int y) {
        for (int x = 0; x < vertexSize; ++x) {
            const float slopeGradient = SlopeGradientMagnitude(heightValues, x, y, vertexSize, slopeConfiguration);
            for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum)
                ApplyStratumCell(stratumConfigurations[stratum], storedValues,
                                 mapFields.materialMasks[stratum], slopeGradient, x, y);
        }
    };
    if (threadPool != nullptr) threadPool->ParallelFor(0, vertexSize, maskRow);
    else for (int y = 0; y < vertexSize; ++y) maskRow(y);
    lastBackend = Sys::ComputeBackend::Cpu;
}

} // namespace Proc
} // namespace SanmapGen
