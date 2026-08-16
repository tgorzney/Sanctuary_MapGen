// AccumulationTab_UI_Test.cpp — WO C1 acceptance for the Accumulation tab: the bulk write of the
// ordered-spillover settings across every stratum's erosion record, and the accumulation overlay
// lookup. Drives a real Pipeline::GenerationAssembler without ever running it — no GL, no window.
// NOT YET REGISTERED IN CMake — WO C1 does not own CMakeLists.txt.
#include "AccumulationTab_UI.h"
#include "../params/MapRecipe_PARAMS.h"
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

// v1 had ONE global pair; v2 stores them per stratum. The tab writes every slot, so no stratum is
// left simulating against a value the user cannot see.
void RunBulkWriteChecks() {
    Params::MapRecipe recipe;
    Pipeline::GenerationAssembler assembler(recipe);

    AccumulationSpilloverSettings settings = AccumulationSettingsOfAssembler(assembler);
    Check(!settings.bAccurateSimultaneousAccumulation,
          "the accurate DAG is off by default (it is the Cpu-only ordered path)");
    Check(settings.spilloverThreshold == 0.05f && settings.spilloverShare == 0.5f,
          "and the two numbers read back from the stage, not from a second store");
    Check(!ApplyAccumulationSettingsToErosion(settings, assembler),
          "writing back what the stage already holds costs no regeneration");

    settings.bAccurateSimultaneousAccumulation = true;
    settings.spilloverThreshold = 0.2f;
    settings.spilloverShare     = 0.75f;
    Check(ApplyAccumulationSettingsToErosion(settings, assembler), "a change reports the move");

    bool bEveryStratumFollowed = true;
    for (int stratumIndex = 0; stratumIndex < kLayerEditorStratumCount; ++stratumIndex) {
        const Proc::ErosionLayerSettings& erosionSettings =
            assembler.Erosion().LayerSettings(stratumIndex);
        if (!erosionSettings.bAccurateSimultaneousAccumulation
            || erosionSettings.spilloverThreshold != 0.2f
            || erosionSettings.spilloverShare != 0.75f) bEveryStratumFollowed = false;
    }
    Check(bEveryStratumFollowed, "every stratum's erosion record followed the global control");
    Check(!ApplyAccumulationSettingsToErosion(settings, assembler),
          "and re-applying it is free the second time");

    const AccumulationSpilloverSettings readBack = AccumulationSettingsOfAssembler(assembler);
    Check(readBack.bAccurateSimultaneousAccumulation && readBack.spilloverThreshold == 0.2f
          && readBack.spilloverShare == 0.75f, "the tab reads its own write back from slot 0");

    // One stratum edited behind the tab's back must be pulled back into line, not ignored.
    assembler.Erosion().LayerSettings(4).spilloverThreshold = 0.9f;
    Check(ApplyAccumulationSettingsToErosion(settings, assembler),
          "a stratum that drifted is detected and rewritten");
}

// The tier of the tab's edit is derived from the erosion stage's own hash, and that hash
// DELIBERATELY skips a stratum whose erosion is disabled (Erosion_PROC.cpp: an off layer
// contributes only its index). So a spillover edit is free while nothing erodes, and re-runs
// generation the moment something does — which is the correct two-tier behaviour, not a gap.
void RunStageHashChecks() {
    Params::MapRecipe recipe;
    Pipeline::GenerationAssembler assembler(recipe);
    AccumulationSpilloverSettings settings = AccumulationSettingsOfAssembler(assembler);

    const std::size_t idleHash = assembler.Erosion().ComputeParameterHash();
    settings.spilloverThreshold = 0.42f;
    ApplyAccumulationSettingsToErosion(settings, assembler);
    Check(assembler.Erosion().ComputeParameterHash() == idleHash,
          "with every erosion layer off, a spillover edit costs no regeneration");

    assembler.Erosion().LayerSettings(2).bEnabled = true;
    const std::size_t erodingHash = assembler.Erosion().ComputeParameterHash();
    settings.spilloverThreshold = 0.61f;
    ApplyAccumulationSettingsToErosion(settings, assembler);
    Check(assembler.Erosion().ComputeParameterHash() != erodingHash,
          "once a layer erodes, the same edit moves the stage hash and re-runs generation");

    const std::size_t settledHash = assembler.Erosion().ComputeParameterHash();
    ApplyAccumulationSettingsToErosion(settings, assembler);
    Check(assembler.Erosion().ComputeParameterHash() == settledHash,
          "and re-applying the settled value moves nothing");
}

void RunOverlayLookupChecks() {
    PreviewCompositeSettings settings;
    PreviewFieldLayer flowLayer;
    flowLayer.kind = PreviewLayerKind::Flow;
    flowLayer.gradientRampIndex = 0;
    PreviewFieldLayer accumulationLayer;
    accumulationLayer.kind = PreviewLayerKind::Accumulation;
    accumulationLayer.gradientRampIndex = 1;
    settings.fieldLayers.push_back(flowLayer);
    settings.fieldLayers.push_back(accumulationLayer);
    settings.gradientRamps.resize(2);

    PreviewFieldLayer* const layer =
        PreviewFieldLayerOfKind(settings, PreviewLayerKind::Accumulation);
    Check(layer != nullptr, "the Accumulation overlay is found past the Flow layer above it");
    Check(PreviewRampOfFieldLayer(settings, *layer) == &settings.gradientRamps[1],
          "and it colorizes its own ramp, not the flow one");

    AccumulationTabState state;
    Check(state.spilloverThresholdRange.maximumValue == 1.0f,
          "Spillover Threshold carries the plan's 0-1 limits");
    Check(ClampScalarSliderValue(-3.0f, state.spilloverShareRange) == 0.0f,
          "and a share driven below zero is held at zero");
}

} // namespace

int main() {
    RunBulkWriteChecks();
    RunStageHashChecks();
    RunOverlayLookupChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
