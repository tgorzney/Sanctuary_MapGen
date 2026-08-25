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
#include <cstdint>

namespace SanmapGen {
namespace Pipeline {

void GenerationAssembler::AddStage(const std::string& stageName, RegenerationTier tier,
                                   std::function<std::size_t()> computeParameterHash,
                                   std::function<void()> run) {
    stageDescriptions.push_back(StageDescription{ stageName, tier });
    stageParameterHashFunctions.push_back(computeParameterHash);
    pipeline.AddStage(stageName, std::move(computeParameterHash), std::move(run));
}

// ARCH §8.3: the spatial grid is a derived INDEX over the resolved markers, not a new physical
// quantity, so it gets no PROC stage of its own — PIPELINE is its single writer (§3.4.1) and
// builds it immediately after Placement, inside the same registered run. A refresh that does not
// re-run Placement therefore cannot move it, which is exactly what the preview tier relies on.
// The horizontal pair is positionX/positionZ; positionY is terrain HEIGHT (PlacementInstance_DATA),
// and the picker compares against those same two columns (Picking_UI).
void GenerationAssembler::BuildMarkerSpatialGrid() {
    const Data::PlacementInstances& markers = placementResults.markers;
    markerSpatialGrid.Configure(static_cast<float>(recipe.geometry.mapSize) * WorldUnitsPerCell(),
                                spatialGridChunkResolution);
    markerSpatialGrid.Build(markers.positionX.data(), markers.positionZ.data(),
                            static_cast<std::int32_t>(markers.Count()));
    ++spatialGridBuildCount;
}

// ARCH_14_09_RenderingPerformance.md §14.9: one Data::RuleBucketIndex per PlacementResults collection, keyed on that collection's
// own ruleIndex column, sized from that collection's own rule-array count (ruleIndex is per-family,
// not a global index — Placement_Rules_PROC.cpp). Same single-writer lifecycle as
// BuildMarkerSpatialGrid immediately above.
void GenerationAssembler::BuildRuleBucketIndex() {
    // CONFIRMED (STEP79 "⭐ Downstream authority ruling"): markers' bucketTotal sums rule counts
    // across the markerRuleLayers/rules nest (STEP66), matching ruleIndex's confirmed flat/global-
    // over-the-marker-family numbering — the same numbering STEP51's SeedMarkerDomains assumes,
    // kept consistent between the two tickets. STEP79 states this function is "correct as written";
    // no change needed.
    int markerRuleCount = 0;
    for (const Params::MarkerRuleLayer& layer : recipe.markerRuleLayers)
        markerRuleCount += static_cast<int>(layer.rules.size());
    ruleBucketIndex.markers.Build(placementResults.markers.ruleIndex.data(),
        static_cast<std::int32_t>(placementResults.markers.Count()),
        markerRuleCount);
    ruleBucketIndex.props.Build(placementResults.props.ruleIndex.data(),
        static_cast<std::int32_t>(placementResults.props.Count()),
        static_cast<int>(recipe.propRules.size()));
    ruleBucketIndex.units.Build(placementResults.units.ruleIndex.data(),
        static_cast<std::int32_t>(placementResults.units.Count()),
        static_cast<int>(recipe.unitRules.size()));
    ruleBucketIndex.decals.Build(placementResults.decals.ruleIndex.data(),
        static_cast<std::int32_t>(placementResults.decals.Count()),
        static_cast<int>(recipe.decalRules.size()));
}

void GenerationAssembler::RegisterStages() {
    const RegenerationTier full = RegenerationTier::FullRegeneration;

    AddStage("NoiseBlend", full, [this] { return noiseBlendStage.ComputeParameterHash(); },
             [this] { noiseBlendStage.Run(); });
    // STEP152: gated behind HasActiveProceduralLayer() -- once every layer is baked/disabled/
    // recipe-less, none of these three sims has anything live to act on, so their bodies are
    // skipped entirely (a known, ratified tradeoff: even a frozen single-layer stack no longer
    // runs erosion/thermal/flow, LAYER_SYSTEM_SPEC's older "redundant recompute only" framing
    // notwithstanding). ComputeParameterHash() is untouched -- the dirty-hash bookkeeping is
    // unaffected; only the run body becomes a cheap early-return "nothing to do" when gated.
    AddStage("Erosion", full, [this] { return erosionStage.ComputeParameterHash(); },
             [this] { if (recipe.layerStack.HasActiveProceduralLayer()) erosionStage.Run(); });
    AddStage("Thermal", full, [this] { return thermalStage.ComputeParameterHash(); },
             [this] { if (recipe.layerStack.HasActiveProceduralLayer()) thermalStage.Run(); });
    AddStage("FlowAccumulation", full, [this] { return flowAccumulationStage.ComputeParameterHash(); },
             [this] { if (recipe.layerStack.HasActiveProceduralLayer()) flowAccumulationStage.Run(); });
    AddStage("Mask", full, [this] { return maskStage.ComputeParameterHash(); },
             [this] { maskStage.Run(); });
    AddStage("Placement", full, [this] { return placementStage.ComputeParameterHash(); },
             [this] { placementStage.Run(); BuildMarkerSpatialGrid(); BuildRuleBucketIndex(); });
    AddStage("Bake", RegenerationTier::PreviewOnly, [this] { return bakeStage.ComputeParameterHash(); },
             [this] { bakeStage.Run(); });
}

} // namespace Pipeline
} // namespace SanmapGen
