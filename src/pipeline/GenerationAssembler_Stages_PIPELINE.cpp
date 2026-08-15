// GenerationAssembler_Stages_PIPELINE.cpp — THE stage order, in one place (ARCH §3.2/§3.3).
// NoiseBlend -> Erosion -> Thermal -> FlowAccumulation -> Mask -> Placement -> Bake (ARCH §7.4),
// each registered with its own ComputeParameterHash (what makes it dirty) and its Run (which
// dispatches CPU/GPU through Sys::Dispatch with the stage's own DispatchPolicy, ARCH §4.2).
// Generation_PIPELINE mixes each hash forward, so a stage re-runs when its own settings move
// OR when anything upstream did — which is why no stage has to know the pipeline shape.
// Tier: every stage that feeds the gameplay-authoritative output is FullRegeneration; the
// bake is decorative and determinism-exempt (ARCH §4.2/§4.5), so it is PreviewOnly.
// Mask sits between the sim block and Placement: its gate must see the FINAL slope and the
// FINAL proportions, and Placement/Bake must see resolved surface weights. Any future sim
// (FUTURE_SIM_TYPES_SPEC) is inserted into the sim block, BEFORE Mask — a standing rule.
#include "GenerationAssembler_PIPELINE.h"

namespace SanmapGen {
namespace Pipeline {

void GenerationAssembler::AddStage(const std::string& stageName, RegenerationTier tier,
                                   std::function<std::size_t()> computeParameterHash,
                                   std::function<void()> run) {
    stageDescriptions.push_back(StageDescription{ stageName, tier });
    pipeline.AddStage(stageName, std::move(computeParameterHash), std::move(run));
}

void GenerationAssembler::RegisterStages() {
    const RegenerationTier full = RegenerationTier::FullRegeneration;

    AddStage("NoiseBlend", full, [this] { return noiseBlendStage.ComputeParameterHash(); },
             [this] { noiseBlendStage.Run(); });
    AddStage("Erosion", full, [this] { return erosionStage.ComputeParameterHash(); },
             [this] { erosionStage.Run(); });
    AddStage("Thermal", full, [this] { return thermalStage.ComputeParameterHash(); },
             [this] { thermalStage.Run(); });
    AddStage("FlowAccumulation", full, [this] { return flowAccumulationStage.ComputeParameterHash(); },
             [this] { flowAccumulationStage.Run(); });
    AddStage("Mask", full, [this] { return maskStage.ComputeParameterHash(); },
             [this] { maskStage.Run(); });
    AddStage("Placement", full, [this] { return placementStage.ComputeParameterHash(); },
             [this] { placementStage.Run(); });
    AddStage("Bake", RegenerationTier::PreviewOnly, [this] { return bakeStage.ComputeParameterHash(); },
             [this] { bakeStage.Run(); });
}

} // namespace Pipeline
} // namespace SanmapGen
