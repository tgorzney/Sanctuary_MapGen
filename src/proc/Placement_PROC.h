// Placement_PROC.h — the placement stage: ONE seeded scatter module for markers, props,
// units and decals. Layer: PROC. Replaces the three disagreeing v1 mechanisms
// (declared-but-empty procedural placement, the Gpu preview density gate, and the GUI
// widget's rand() unit grid — PLACEMENT_SCATTER_SPEC).
// Poisson-disk / blue-noise by hashed candidate priority: every candidate is a pure function
// of (seed, rule, position), so the same recipe always yields the same map (DETERMINISM_SPEC).
// Cpu is authoritative (Exact class, ARCH §4.2); the Gpu path only evaluates the per-cell
// density gate for the preview and the SAME Cpu acceptance then samples it — the preview
// never re-filters (ARCH §3.2). The stage never picks a backend: it hands itself to
// Sys::Dispatch with the policy PIPELINE gave it.
#pragma once
#include <cstddef>
#include <vector>
#include "Placement_Kernel_PROC.h"
#include "Placement_Candidate_PROC.h"
#include "Placement_Symmetry_PROC.h"
#include "../data/MapFields_DATA.h"
#include "../data/PlacementResults_DATA.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../sys/Dispatch_SYS.h"

namespace SanmapGen {
namespace Sys { class GpuResourceManager; class ThreadPool; }

namespace Proc {

class PlacementStage {
public:
    PlacementStage(const Params::MapRecipe& recipeSettings, const Data::MapFields& inputFields,
                   Data::PlacementResults& outputResults);

    PlacementConstants& Constants() { return constants; }
    const PlacementConstants& Constants() const { return constants; }
    void SetDispatchPolicy(const Sys::DispatchPolicy& policy) { dispatchPolicy = policy; }
    void SetGenerationContext(Sys::GenerationContext context) { generationContext = context; }
    void SetGlobalBackend(Sys::ComputeBackend backend) { globalBackend = backend; }
    void SetGpuResourceManager(Sys::GpuResourceManager* manager) { gpuResourceManager = manager; }
    void SetThreadPool(Sys::ThreadPool* pool) { threadPool = pool; }

    // Hash of everything this stage consumes (geometry seed/size, every rule, water,
    // global symmetry) — what PIPELINE registers as the stage's computeParamHash.
    std::size_t ComputeParameterHash() const;

    // Resolves the backend through Sys::Dispatch and runs there. Returns the backend used.
    Sys::ComputeBackend Run();

    // The two backends (public because Sys::Dispatch calls them; PIPELINE uses Run()).
    void RunOnCpu();
    void RunOnGpu();

    // Observability for tests/telemetry.
    Sys::ComputeBackend LastBackend() const { return lastBackend; }
    bool WasGpuGateUsed() const { return bGpuGateUsed; }
    bool IsGpuAvailable() const { return bGpuProgramReady; }
    int  EvaluatedCandidateCount() const { return evaluatedCandidateCount; }
    int  AcceptedCandidateCount() const { return acceptedCandidateCount; }
    const std::vector<ScatterRuleConfiguration>& RuleConfigurations() const { return ruleConfigurations; }
    const Data::FloatField& SlopeGradientField() const { return slopeGradientField; }
    const Data::FloatField& ObstacleDistanceField() const { return obstacleDistanceField; }

private:
    // Placement_Rules_PROC.cpp — flattens every enabled rule into the shared kernel record.
    void BuildRuleConfigurations();
    // Placement_Fields_PROC.cpp — derived slope, the Jump-Flood exclusion field, the gate field.
    void BuildDerivedFields();
    void BuildSlopeGradientField();
    void BuildGateFieldCpu(std::size_t configurationIndex);
    float SampleMaskWeight(int stratumIndex, int cellX, int cellY) const;
    float SampleClearanceRadius(const ScatterRuleConfiguration& configuration, int cellX, int cellY) const;
    float SampleHeightVariance(int cellX, int cellY) const;
    // Placement_Scatter_PROC.cpp — the Poisson-disk candidate pass.
    void ScatterRule(std::size_t configurationIndex);
    void CollectCandidates(std::size_t configurationIndex, std::vector<ScatterCandidate>& outCandidates);
    float ComputeCandidateSortKey(const ScatterRuleConfiguration& configuration, int cellX, int cellY,
                                  const ScatterCandidate& candidate, bool& bRejected) const;
    // Placement_Accept_PROC.cpp — spacing + symmetry acceptance.
    void AcceptCandidates(std::size_t configurationIndex, const std::vector<ScatterCandidate>& candidates);
    bool IsOrbitPlaceable(const ScatterRuleConfiguration& configuration, const SymmetryOrbitPoint* orbit,
                          int orbitCount, const class SpacingGrid& spacingGrid) const;
    // Placement_Emit_PROC.cpp — one accepted position -> one SoA instance.
    void EmitInstance(std::size_t configurationIndex, const SymmetryOrbitPoint& point,
                      uint32_t sourcePositionHash, int symmetryIdentifier);
    int  DominantStratumIndex(int cellX, int cellY) const;
    Data::PlacementInstances& CollectionFor(int collectionIndex);
    // Placement_Gpu_PROC.cpp — the preview density gate.
    bool EnsureGpuResources();
    bool BuildGateFieldGpu(std::size_t configurationIndex);

    void RunScatter(bool bUseGpuGate);

    const Params::MapRecipe&  recipe;
    const Data::MapFields&    mapFields;
    Data::PlacementResults&   results;
    PlacementConstants        constants;

    Sys::DispatchPolicy      dispatchPolicy;   // ARCH §4.2: Placement is Cpu/Exact on output
    Sys::GenerationContext   generationContext = Sys::GenerationContext::Output;
    Sys::ComputeBackend      globalBackend     = Sys::ComputeBackend::Cpu;
    Sys::GpuResourceManager* gpuResourceManager = nullptr;
    Sys::ThreadPool*         threadPool         = nullptr;

    std::vector<ScatterRuleConfiguration>    ruleConfigurations;
    std::vector<Data::TemplateIdentifier>    ruleTemplateIdentifiers;
    Data::FloatField                         slopeGradientField;      // squared height gradient
    Data::FloatField                         obstacleDistanceField;   // Jump-Flood distance
    Data::FloatField                         gateWeightField;         // per-rule, reused
    std::vector<float>                       gpuGateTransferBuffer;

    Sys::ComputeBackend lastBackend = Sys::ComputeBackend::Cpu;
    bool bGpuGateUsed      = false;
    bool bGpuProgramReady  = false;
    bool bObstacleFieldBuilt = false;
    bool bGpuFieldsUploaded  = false;   // height/slope/obstacle are per-run, not per-rule
    int  gpuProgramIndex   = -1;
    int  evaluatedCandidateCount = 0;
    int  acceptedCandidateCount  = 0;
    int  nextSymmetryIdentifier  = 1;
};

} // namespace Proc
} // namespace SanmapGen
