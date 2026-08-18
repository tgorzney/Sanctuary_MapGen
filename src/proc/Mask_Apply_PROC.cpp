// Mask_Apply_PROC.cpp — the CPU accuracy path: slope-gate each stratum's material proportion,
// merge the stored art, remap once, and write the result into `surfaceStratumWeights`. Twin of
// Mask_PROC.glsl, expression for expression, through the shared Mask_Slope_PROC.h /
// Mask_Merge_PROC.h math.
// The proportion field is READ ONLY here (ARCH §7.2.3): input and output are different fields,
// which is what makes this stage idempotent under a lone re-run (ARCH §3.4.2).
// It writes TWO outputs, both its own (§3.4.1): `surfaceStratumWeights` and the `slope` field —
// the very gradient the gate already needs, baked so Placement and the preview SAMPLE it rather
// than each deriving a private copy (M5-0c).
#include "Mask_PROC.h"
#include "Mask_Slope_PROC.h"
#include "Mask_Merge_PROC.h"
#include "../sys/ThreadPool_SYS.h"

namespace SanmapGen {
namespace Proc {
namespace {

// One cell of one stratum: gate -> merge. The stored art is sampled ONLY when a merge mode
// wants it (Disabled never pays for a resample). The merged, clamped weight IS the output —
// no further per-stratum remap (that field is material/appearance pass-through, not a Mask
// input; Generator Expert ruling).
// NOTE (MASKING_SPEC 1.9 / ARCH §7.5): `materialProportion` is a VOLUME FRACTION standing in
// for surface exposure until the ordered thickness stack lands in M6. Same shape, same kernel.
float ResolveStratumCell(const MaskStratumConfiguration& configuration, const float* storedValues,
                         float materialProportion, float slopeGradient, int x, int y) {
    const float gateWeight = SlopeGateWeight(slopeGradient, configuration);
    const float proceduralWeight = materialProportion * gateWeight;
    const float storedWeight = configuration.mergeMode == kMergeModeDisabled
                             ? 0.0f : SampleStoredMaskBilinear(storedValues, configuration, x, y);
    return MergeStoredMask(proceduralWeight, storedWeight, configuration);
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
            mapFields.slope.Set(x, y, slopeGradient);      // the baked field, in the pinned unit
            for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum)
                mapFields.surfaceStratumWeights[stratum].Set(x, y,
                    ResolveStratumCell(stratumConfigurations[stratum], storedValues,
                                       mapFields.materialProportions[stratum].Get(x, y),
                                       slopeGradient, x, y));
        }
    };
    if (threadPool != nullptr) threadPool->ParallelFor(0, vertexSize, maskRow);
    else for (int y = 0; y < vertexSize; ++y) maskRow(y);
    lastBackend = Sys::ComputeBackend::Cpu;
}

} // namespace Proc
} // namespace SanmapGen
