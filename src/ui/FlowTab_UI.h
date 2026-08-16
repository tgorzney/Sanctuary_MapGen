// FlowTab_UI.h — the Flow tab: the flow/accumulation stage's routing constants, its per-stage
// backend override, and the flow overlay + ramp. Layer: UI. Accuracy class: Visual.
// TAB_REBUILD_PLAN "4 · Flow" (constants -> MapUpdate, overlay/ramp -> PreviewRender).
//
// The stage constants are reached through PIPELINE (`GenerationAssembler::FlowAccumulation()`),
// the interim contract that header states in so many words. UI never includes a PROC stage of its
// own, never picks a backend and never routes a drop of water (ARCH §3.1/§3.2).
//
// "USE GPU" IS A PER-STAGE OVERRIDE, NOT A RIVAL TOGGLE. ARCH §4.3 resolves a backend as
// "explicit per-calculation override, else the global CPU/GPU setting, else automatic", so this
// tick writes THIS stage's `Sys::DispatchPolicy::previewBackend` — Gpu when set, Automatic when
// clear, which is what defers to the System tab's global setting. It is a different field from
// the System tab's global backend; neither overwrites the other (ARCH §4: never add a rival).
//
// SCOPE NOTES (ARCH §8.4 — a coder never invents a missing field; reported, not invented):
//  1. The plan's "Slope Adherence" and "Flow Momentum" do not exist on this stage.
//     `Proc::FlowAccumulationConstants` has no such fields, and the only `slopeAdherence` in the
//     tree belongs to `Proc::ErosionLayerSettings` (droplet steering, a different sim). Neither is
//     drawn; adding them means changing the routing kernel, which is a work-order.
//  2. The plan's single "Iterations 1-100" maps onto the two v2 iteration budgets the stage
//     actually carries — `fillIterationsPerSide` and `accumulationIterationsPerSide`. Both are
//     drawn, rather than inventing one control that writes two fields.
//  3. `Proc::FlowAccumulationStage` exposes no dispatch-policy GETTER (unlike `ErosionStage`), so
//     the tick's displayed state lives in the tab state and is PUSHED to the stage. Same standing
//     as SystemTab_UI's determinism note.
#pragma once
#include "Section_UI.h"
#include "SliderScalar_UI.h"
#include "TerrainOverlayTab_UI.h"
#include "GradientEditorWidget_UI.h"
#include "../sys/Dispatch_SYS.h"

namespace SanmapGen {
namespace Pipeline { class GenerationAssembler; class PreviewDriver; }
namespace Ui {

// The policy the tick produces. Set -> the Gpu speed path in the Preview context; clear ->
// Automatic, which falls through to the global setting (ARCH §4.3 step 3). The OUTPUT backend is
// deliberately untouched: flow feeds pathing, so the baked pass stays the Cpu accuracy path
// (ARCH §4.2 defaults for this stage).
inline Sys::DispatchPolicy FlowPreviewDispatchPolicy(bool bUseGpuForPreview) {
    Sys::DispatchPolicy policy;
    policy.previewBackend  = bUseGpuForPreview ? Sys::ComputeBackend::Gpu
                                               : Sys::ComputeBackend::Automatic;
    policy.outputBackend   = Sys::ComputeBackend::Cpu;
    policy.previewAccuracy = Sys::AccuracyClass::Visual;
    policy.outputAccuracy  = Sys::AccuracyClass::Exact;
    return policy;
}

// Caller-owned tab state: the limits (Constitution §8), one RealtimeToggle per drag, the ramp
// editor's own state, and the backend tick (SCOPE NOTE 3).
struct FlowTabState {
    ScalarSliderRange precipitationRateRange{ 0.0f, 10.0f, 0.0f };
    ScalarSliderRange flowVolumeMultiplierRange{ 0.1f, 10.0f, 0.0f };
    ScalarSliderRange stochasticVarianceRange{ 0.0f, 1.0f, 0.0f };
    ScalarSliderRange depressionFillEpsilonRange{ 0.0f, 0.01f, 0.0f };
    ScalarSliderRange iterationsPerSideRange{ 1.0f, 100.0f, 1.0f };
    ScalarSliderRange convergenceCheckIntervalRange{ 1.0f, 256.0f, 1.0f };

    RealtimeToggle precipitationRateToggle;
    RealtimeToggle flowVolumeMultiplierToggle;
    RealtimeToggle stochasticVarianceToggle;
    RealtimeToggle depressionFillEpsilonToggle;
    RealtimeToggle fillIterationsToggle;
    RealtimeToggle accumulationIterationsToggle;
    RealtimeToggle convergenceCheckIntervalToggle;

    SectionState routingSection;
    SectionState iterationSection;
    GradientEditorState gradientEditor;

    bool bUseGpuForPreview = true;   // matches the stage's own ARCH §4.2 preview default
};

// Pushes the tick onto the stage's policy. Split out so the decision is assertable with no window
// open and no pipeline built.
inline bool FlowBackendPreferenceMoved(bool bUseGpuForPreview, const Sys::DispatchPolicy& policy) {
    return FlowPreviewDispatchPolicy(bUseGpuForPreview).previewBackend != policy.previewBackend;
}

void DrawFlowTab(PreviewCompositeSettings& compositeSettings, FlowTabState& state,
                 Pipeline::GenerationAssembler* generationAssembler,
                 Pipeline::PreviewDriver* previewDriver);

} // namespace Ui
} // namespace SanmapGen
