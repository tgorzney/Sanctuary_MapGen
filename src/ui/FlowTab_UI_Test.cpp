// FlowTab_UI_Test.cpp — WO C1 acceptance for the Flow tab: the per-stage backend override, the
// routing constants it binds, and the flow overlay lookup. The constants live on a real
// Pipeline::GenerationAssembler (the tab reaches them through PIPELINE), which is built but never
// run — so no GL context and no window.
// NOT YET REGISTERED IN CMake — WO C1 does not own CMakeLists.txt.
#include "FlowTab_UI.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../pipeline/GenerationAssembler_PIPELINE.h"
#include <cstdio>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

// ARCH §4.3: an explicit per-calculation override beats the global setting; clearing the tick must
// hand the decision BACK to the global setting rather than pinning the Cpu.
void RunBackendOverrideChecks() {
    const Sys::DispatchPolicy gpuPolicy = FlowPreviewDispatchPolicy(true);
    Check(gpuPolicy.previewBackend == Sys::ComputeBackend::Gpu,
          "ticked: the preview pass takes the Gpu speed path");
    const Sys::DispatchPolicy automaticPolicy = FlowPreviewDispatchPolicy(false);
    Check(automaticPolicy.previewBackend == Sys::ComputeBackend::Automatic,
          "cleared: the preview pass defers to the global backend, it does not pin the Cpu");
    Check(gpuPolicy.outputBackend == Sys::ComputeBackend::Cpu
          && automaticPolicy.outputBackend == Sys::ComputeBackend::Cpu,
          "either way the baked output stays on the accuracy path");
    Check(gpuPolicy.outputAccuracy == Sys::AccuracyClass::Exact,
          "flow feeds pathing, so its output pass is gameplay-authoritative");

    // The resolution rule itself, so the tick's meaning is asserted and not just its field value.
    Check(Sys::ResolveBackend(automaticPolicy, Sys::GenerationContext::Preview,
                              Sys::ComputeBackend::Cpu, Sys::DataResidency::Either)
          == Sys::ComputeBackend::Cpu,
          "cleared + a global Cpu setting resolves to the Cpu");
    Check(Sys::ResolveBackend(gpuPolicy, Sys::GenerationContext::Preview,
                              Sys::ComputeBackend::Cpu, Sys::DataResidency::Either)
          == Sys::ComputeBackend::Gpu,
          "and the explicit override outranks that global setting");

    FlowTabState state;
    Check(state.bUseGpuForPreview, "the tab opens on the stage's own ARCH 4.2 preview default");
    Check(!FlowBackendPreferenceMoved(true, gpuPolicy), "re-applying the same preference is free");
    Check(FlowBackendPreferenceMoved(false, gpuPolicy), "and a flip reports the move");
}

void RunConstantBindingChecks() {
    Params::MapRecipe recipe;
    Pipeline::GenerationAssembler assembler(recipe);
    Proc::FlowAccumulationConstants& constants = assembler.FlowAccumulation().Constants();

    const std::size_t settledHash = assembler.FlowAccumulation().ComputeParameterHash();
    constants.cellWeight = 4.0f;
    Check(assembler.FlowAccumulation().ComputeParameterHash() != settledHash,
          "moving Precipitation Rate moves the stage hash, so the tab's edit re-runs generation");

    constants.flowMagnitudeScale = 2.5f;
    constants.flowNoiseImpact    = 0.4f;
    constants.bFillDepressions   = false;
    Check(constants.flowMagnitudeScale == 2.5f && constants.flowNoiseImpact == 0.4f
          && !constants.bFillDepressions,
          "Flow Volume Multiplier, Stochastic Variance and Fill Depressions are all writable");

    constants.fillIterationsPerSide         = 12;
    constants.accumulationIterationsPerSide = 20;
    Check(constants.fillIterationsPerSide == 12 && constants.accumulationIterationsPerSide == 20,
          "v1's single Iterations control binds to the TWO budgets this stage carries");

    FlowTabState state;
    Check(state.iterationsPerSideRange.minimumValue == 1.0f
          && state.iterationsPerSideRange.maximumValue == 100.0f,
          "the iteration sliders carry the plan's 1-100 limits");
    Check(state.iterationsPerSideRange.increment >= 1.0f,
          "and snap to whole iterations");
    Check(ClampScalarSliderValue(0.0f, state.precipitationRateRange) == 0.0f
          && ClampScalarSliderValue(99.0f, state.precipitationRateRange) == 10.0f,
          "Precipitation Rate carries the plan's 0-10 limits");
}

void RunOverlayLookupChecks() {
    PreviewCompositeSettings settings;
    PreviewFieldLayer flowLayer;
    flowLayer.kind = PreviewLayerKind::Flow;
    flowLayer.gradientRampIndex = 0;
    settings.fieldLayers.push_back(flowLayer);
    settings.gradientRamps.push_back(Params::GradientRamp());

    PreviewFieldLayer* const layer = PreviewFieldLayerOfKind(settings, PreviewLayerKind::Flow);
    Check(layer != nullptr, "the Flow overlay is found");
    Check(PreviewRampOfFieldLayer(settings, *layer) == &settings.gradientRamps[0],
          "and it owns its OWN ramp - the v1 flow/accumulation ramp aliasing is retired");
    Check(PreviewFieldLayerOfKind(settings, PreviewLayerKind::Accumulation) == nullptr,
          "accumulation is a different layer, never this one");
}

} // namespace

int main() {
    RunBackendOverrideChecks();
    RunConstantBindingChecks();
    RunOverlayLookupChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
