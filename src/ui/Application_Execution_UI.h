// Application_Execution_UI.h — the Performance panel's execution settings and the pure rules that
// turn them into `Sys::DispatchPolicy` edits. Layer: UI. Accuracy class: Visual (it selects HOW work
// runs; it computes nothing). TAB_REBUILD_PLAN "SYSTEM · Performance".
// A member file of Application_UI.h (ARCH §1.5); no imgui here, so every rule below is headless.
//
// WHY THE SHELL OWNS THIS. SystemTab_UI.h SCOPE NOTE 2 states it: `Sys::DispatchPolicy` is per
// STAGE and `Pipeline::GenerationAssembler` fans out context and global backend only, so applying a
// policy to every stage is the app shell's job until a PIPELINE-level setter is ordered. The shell
// therefore names the stages its toggles reach — exactly as AccumulationTab_UI.h names Erosion — and
// it still knows no stage ORDER, which remains PIPELINE's alone (ARCH §3.2).
//
// NOTHING HERE IS A SECOND HOME. Each toggle is a MIRROR read back out of the stage's own policy by
// LoadExecutionSettings(); v1's `UseGPUTerrain`/`UseGPUFlowMap`/`WYSIWYGBaking` bools are retired
// into the one policy object ARCH §4.1 defines.
#pragma once
#include "../sys/Dispatch_SYS.h"

namespace SanmapGen {
namespace Pipeline { class GenerationAssembler; }
namespace Ui {

// The stages whose OUTPUT pass the WYSIWYG toggle rewrites: NoiseBlend, Erosion, Thermal,
// FlowAccumulation, Mask — the simulation chain whose result the bake samples. This is the count of
// stages this PANEL edits, not the pipeline's shape (only PIPELINE knows that, ARCH §3.2).
inline constexpr int kWysiwygStageCount = 5;

// One stage's output pass as it stood BEFORE WYSIWYG was engaged, so releasing the toggle restores
// the stage's own ARCH §4.2 default instead of the shell guessing at it.
struct OutputPassSnapshot {
    Sys::ComputeBackend backend    = Sys::ComputeBackend::Cpu;
    Sys::AccuracyClass  accuracy   = Sys::AccuracyClass::Accurate;
    bool                bCaptured  = false;
};

struct ApplicationExecutionSettings {
    bool bUseGpuTerrain = true;    // NoiseBlend + Erosion + Thermal preview backend
    bool bUseGpuFlow    = true;    // FlowAccumulation preview backend
    bool bWysiwygBaking = false;   // the output pass runs what the preview pass ran
    OutputPassSnapshot outputPassBeforeWysiwyg[kWysiwygStageCount];
};

// toggle -> backend. Cpu is the accuracy path, Gpu the speed path (Constitution §4).
inline Sys::ComputeBackend PreviewBackendOfToggle(bool bUseGpu) {
    return bUseGpu ? Sys::ComputeBackend::Gpu : Sys::ComputeBackend::Cpu;
}

// ...and back, for showing the tick a stage's policy already implies.
inline bool IsPreviewBackendGpu(const Sys::DispatchPolicy& dispatchPolicy) {
    return dispatchPolicy.previewBackend == Sys::ComputeBackend::Gpu;
}

// Writes the preview backend. Reports whether the policy actually moved, so re-applying an
// unchanged toggle costs no regeneration.
inline bool StorePreviewBackend(bool bUseGpu, Sys::DispatchPolicy& dispatchPolicy) {
    const Sys::ComputeBackend backend = PreviewBackendOfToggle(bUseGpu);
    if (dispatchPolicy.previewBackend == backend) return false;
    dispatchPolicy.previewBackend = backend;
    return true;
}

// WYSIWYG on: the output pass becomes the preview pass, so the baked result is the image the user
// approved (ARCH §4.4 — the preview samples the bake, it never re-simulates). The pre-WYSIWYG pair
// is captured once, on the frame the toggle engages.
inline bool EngageWysiwygOnPolicy(Sys::DispatchPolicy& dispatchPolicy, OutputPassSnapshot& snapshot) {
    if (!snapshot.bCaptured) {
        snapshot.backend   = dispatchPolicy.outputBackend;
        snapshot.accuracy  = dispatchPolicy.outputAccuracy;
        snapshot.bCaptured = true;
    }
    if (dispatchPolicy.outputBackend == dispatchPolicy.previewBackend
            && dispatchPolicy.outputAccuracy == dispatchPolicy.previewAccuracy) return false;
    dispatchPolicy.outputBackend  = dispatchPolicy.previewBackend;
    dispatchPolicy.outputAccuracy = dispatchPolicy.previewAccuracy;
    return true;
}

// WYSIWYG off: the stage's own output pass comes back. With nothing captured the policy is left
// exactly as it is rather than being snapped onto a guessed default (Constitution §6).
inline bool ReleaseWysiwygOnPolicy(Sys::DispatchPolicy& dispatchPolicy, OutputPassSnapshot& snapshot) {
    if (!snapshot.bCaptured) return false;
    const bool bMoved = dispatchPolicy.outputBackend  != snapshot.backend
                     || dispatchPolicy.outputAccuracy != snapshot.accuracy;
    dispatchPolicy.outputBackend  = snapshot.backend;
    dispatchPolicy.outputAccuracy = snapshot.accuracy;
    snapshot.bCaptured = false;
    return bMoved;
}

// Determinism reaches EVERY stage, not just the Exact-class ones: `Sys::ResolveBackend` reads the
// flag off the stage's own policy, so a stage that never saw it could not honour it.
inline bool StoreDeterministic(bool bDeterministic, Sys::DispatchPolicy& dispatchPolicy) {
    if (dispatchPolicy.bDeterministic == bDeterministic) return false;
    dispatchPolicy.bDeterministic = bDeterministic;
    return true;
}

// stages -> the toggles' mirrors. Called before the panel draws, so the ticks show what the stages
// actually run on and the shell keeps no rival default. (Application_Execution_UI.cpp)
void LoadExecutionSettings(Pipeline::GenerationAssembler& generationAssembler,
                           ApplicationExecutionSettings& executionSettings);

// the toggles -> every stage's policy, plus the shell's own determinism flag and `bUseGpuMarkers`
// (STEP19_AppSettings_IO "Flagged, not blocking": placementStage's own preview backend, seeded from
// AppSettings at startup — it has a real live target but no Performance-panel checkbox yet, so it
// travels as a parameter here rather than a fourth ApplicationExecutionSettings field). Reports
// whether any stage's policy moved, which is exactly when the caller owes a regeneration.
// (Application_Execution_UI.cpp)
bool ApplyExecutionSettings(ApplicationExecutionSettings& executionSettings,
                            bool bDeterministic,
                            bool bUseGpuMarkers,
                            Pipeline::GenerationAssembler& generationAssembler);

} // namespace Ui
} // namespace SanmapGen
