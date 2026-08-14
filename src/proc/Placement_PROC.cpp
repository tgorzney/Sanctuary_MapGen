// Placement_PROC.cpp — stage lifecycle: dispatch hand-off and the per-run driver.
// The per-backend work lives in Placement_Rules_PROC.cpp (rule flattening),
// Placement_Fields_PROC.cpp (derived slope / exclusion / gate fields),
// Placement_Metrics_PROC.cpp (clearance + variance priorities),
// Placement_Scatter_PROC.cpp (the Poisson-disk acceptance) and
// Placement_Gpu_PROC.cpp (the preview density gate).
#include "Placement_PROC.h"

namespace SanmapGen {
namespace Proc {

PlacementStage::PlacementStage(const Params::MapRecipe& recipeSettings,
                               const Data::MapFields& inputFields,
                               Data::PlacementResults& outputResults)
    : recipe(recipeSettings), mapFields(inputFields), results(outputResults) {
    // ARCH §4.2 defaults for Placement: Cpu everywhere, Accurate in preview, Exact on output.
    dispatchPolicy.previewBackend  = Sys::ComputeBackend::Cpu;
    dispatchPolicy.outputBackend   = Sys::ComputeBackend::Cpu;
    dispatchPolicy.previewAccuracy = Sys::AccuracyClass::Accurate;
    dispatchPolicy.outputAccuracy  = Sys::AccuracyClass::Exact;
}

Sys::ComputeBackend PlacementStage::Run() {
    // Placement's data lives on the Cpu (the instance SoA is a Cpu buffer), so Automatic
    // resolution must not be tempted onto the Gpu by residency.
    lastBackend = Sys::Dispatch(*this, dispatchPolicy, generationContext, globalBackend,
                                Sys::DataResidency::OnCpu);
    return lastBackend;
}

void PlacementStage::RunOnCpu() { RunScatter(false); }

// The Gpu path is the PREVIEW density gate only: the per-cell rule gate is evaluated in
// Placement_PROC.glsl, and the identical Cpu acceptance then samples that field. Placement
// itself stays Cpu-authoritative (PLACEMENT_SCATTER_SPEC "CPU vs GPU"). If no resource
// manager / program is available the gate falls back to the Cpu twin — same result, no
// silent second code path.
void PlacementStage::RunOnGpu() { RunScatter(true); }

void PlacementStage::RunScatter(bool bUseGpuGate) {
    results.Clear();
    evaluatedCandidateCount = 0;
    acceptedCandidateCount  = 0;
    nextSymmetryIdentifier  = 1;
    bGpuGateUsed            = false;
    bObstacleFieldBuilt     = false;
    bGpuFieldsUploaded      = false;
    if (!recipe.IsValid() || !mapFields.IsSized()) return;

    BuildRuleConfigurations();
    BuildDerivedFields();

    for (std::size_t index = 0; index < ruleConfigurations.size(); ++index) {
        bool bGateReady = false;
        if (bUseGpuGate && BuildGateFieldGpu(index)) { bGateReady = true; bGpuGateUsed = true; }
        if (!bGateReady) BuildGateFieldCpu(index);
        ScatterRule(index);
    }
}

Data::PlacementInstances& PlacementStage::CollectionFor(int collectionIndex) {
    if (collectionIndex == 1) return results.props;
    if (collectionIndex == 2) return results.units;
    if (collectionIndex == 3) return results.decals;
    return results.markers;
}

} // namespace Proc
} // namespace SanmapGen
