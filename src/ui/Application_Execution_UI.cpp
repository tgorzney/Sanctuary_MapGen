// Application_Execution_UI.cpp — the Performance panel's fan-out: the toggles in
// Application_Execution_UI.h read from and written onto each stage's own `Sys::DispatchPolicy`.
// Layer: UI. The UI drives PIPELINE and SYS, never a PROC kernel (ARCH §3.1): every line below is a
// settings read/write through `Pipeline::GenerationAssembler`'s stage accessors, and nothing here
// runs, orders or re-implements a stage.
#include "Application_Execution_UI.h"
#include "../pipeline/GenerationAssembler_PIPELINE.h"

namespace SanmapGen {
namespace Ui {
namespace {

// Read-modify-write of ONE stage's policy, so the stage stays the single home for it and the shell
// never has to know what that stage's ARCH §4.2 default was. Reports whether the policy moved.
template <typename Stage, typename PolicyEdit>
bool EditStagePolicy(Stage& stage, PolicyEdit&& editPolicy) {
    Sys::DispatchPolicy dispatchPolicy = stage.ActiveDispatchPolicy();
    if (!editPolicy(dispatchPolicy)) return false;
    stage.SetDispatchPolicy(dispatchPolicy);
    return true;
}

// The five stages the WYSIWYG toggle rewrites, visited in the order their snapshots are indexed.
// The shell names them; it still knows no stage ORDER (ARCH §3.2).
template <typename StageVisitor>
void VisitWysiwygStages(Pipeline::GenerationAssembler& generationAssembler, StageVisitor&& visitStage) {
    visitStage(generationAssembler.NoiseBlend(),       0);
    visitStage(generationAssembler.Erosion(),          1);
    visitStage(generationAssembler.Thermal(),          2);
    visitStage(generationAssembler.FlowAccumulation(), 3);
    visitStage(generationAssembler.Mask(),             4);
}

// Every registered stage, for the settings that reach all of them (determinism).
template <typename StageVisitor>
void VisitAllStages(Pipeline::GenerationAssembler& generationAssembler, StageVisitor&& visitStage) {
    VisitWysiwygStages(generationAssembler, [&](auto& stage, int) { visitStage(stage); });
    visitStage(generationAssembler.Placement());
    visitStage(generationAssembler.Bake());
}

bool ApplyGpuToggles(const ApplicationExecutionSettings& executionSettings,
                     Pipeline::GenerationAssembler& generationAssembler) {
    const bool bUseGpuTerrain = executionSettings.bUseGpuTerrain;
    auto storeTerrainBackend = [bUseGpuTerrain](Sys::DispatchPolicy& policy) {
        return StorePreviewBackend(bUseGpuTerrain, policy);
    };
    bool bPolicyMoved = EditStagePolicy(generationAssembler.NoiseBlend(), storeTerrainBackend);
    bPolicyMoved = EditStagePolicy(generationAssembler.Erosion(), storeTerrainBackend) || bPolicyMoved;
    bPolicyMoved = EditStagePolicy(generationAssembler.Thermal(), storeTerrainBackend) || bPolicyMoved;
    const bool bUseGpuFlow = executionSettings.bUseGpuFlow;
    bPolicyMoved = EditStagePolicy(generationAssembler.FlowAccumulation(),
                                   [bUseGpuFlow](Sys::DispatchPolicy& policy) {
                                       return StorePreviewBackend(bUseGpuFlow, policy);
                                   }) || bPolicyMoved;
    return bPolicyMoved;
}

// STEP19_AppSettings_IO "Flagged, not blocking": the same one-line pattern the terrain/flow
// toggles use, aimed at Placement instead — its Cpu path stays authoritative (ARCH §4.2, Exact
// class), the Gpu path only ever speeds up the preview density gate.
bool ApplyMarkersGpuToggle(bool bUseGpuMarkers, Pipeline::GenerationAssembler& generationAssembler) {
    return EditStagePolicy(generationAssembler.Placement(),
                           [bUseGpuMarkers](Sys::DispatchPolicy& policy) {
                               return StorePreviewBackend(bUseGpuMarkers, policy);
                           });
}

bool ApplyWysiwygBaking(ApplicationExecutionSettings& executionSettings,
                        Pipeline::GenerationAssembler& generationAssembler) {
    bool bPolicyMoved = false;
    VisitWysiwygStages(generationAssembler, [&](auto& stage, int snapshotIndex) {
        OutputPassSnapshot& snapshot = executionSettings.outputPassBeforeWysiwyg[snapshotIndex];
        const bool bEngaged = executionSettings.bWysiwygBaking;
        bPolicyMoved = EditStagePolicy(stage, [&](Sys::DispatchPolicy& policy) {
            return bEngaged ? EngageWysiwygOnPolicy(policy, snapshot)
                            : ReleaseWysiwygOnPolicy(policy, snapshot);
        }) || bPolicyMoved;
    });
    return bPolicyMoved;
}

} // namespace

// The ticks show what the stages actually run on: NoiseBlend answers for the terrain group (its two
// companions are written together and can never disagree) and FlowAccumulation for the flow one.
void LoadExecutionSettings(Pipeline::GenerationAssembler& generationAssembler,
                           ApplicationExecutionSettings& executionSettings) {
    executionSettings.bUseGpuTerrain =
        IsPreviewBackendGpu(generationAssembler.NoiseBlend().ActiveDispatchPolicy());
    executionSettings.bUseGpuFlow =
        IsPreviewBackendGpu(generationAssembler.FlowAccumulation().ActiveDispatchPolicy());
}

bool ApplyExecutionSettings(ApplicationExecutionSettings& executionSettings, bool bDeterministic,
                            bool bUseGpuMarkers, Pipeline::GenerationAssembler& generationAssembler) {
    bool bPolicyMoved = ApplyGpuToggles(executionSettings, generationAssembler);
    // WYSIWYG runs AFTER the backend toggles: it copies the preview pass, so it has to see the
    // backend the user just chose rather than the one the previous frame carried.
    bPolicyMoved = ApplyWysiwygBaking(executionSettings, generationAssembler) || bPolicyMoved;
    bPolicyMoved = ApplyMarkersGpuToggle(bUseGpuMarkers, generationAssembler) || bPolicyMoved;
    VisitAllStages(generationAssembler, [&](auto& stage) {
        bPolicyMoved = EditStagePolicy(stage, [bDeterministic](Sys::DispatchPolicy& policy) {
            return StoreDeterministic(bDeterministic, policy);
        }) || bPolicyMoved;
    });
    return bPolicyMoved;
}

} // namespace Ui
} // namespace SanmapGen
