// GenerationAssembler_PIPELINE.h — assembles the seven PROC stages into one
// GenerationPipeline. Layer: PIPELINE — the ONE place that knows the generation stage order
// (Constitution §1, ARCH §3.2 "no layer knows the pipeline shape except PIPELINE"):
// NoiseBlend -> Erosion -> Thermal -> FlowAccumulation -> Mask -> Placement -> Bake (ARCH §7.4).
// Mask runs after EVERY sim so its gate sees the final slope and the final proportions, and
// before Placement/Bake, which both consume the resolved surface weights.
// Each stage is registered with its own ComputeParameterHash and its Run(), and Run() hands
// the stage to Sys::Dispatch with that stage's DispatchPolicy (ARCH §4.2) — the assembler
// never selects a backend, never re-implements a stage, and holds no rival toggle.
// It owns the computed DATA the stages write (MapFields, PlacementResults, BakedTextureSet)
// plus the loaded stratum art they read, and takes the settings from the caller's MapRecipe.
#pragma once
#include <cstddef>
#include <functional>
#include <string>
#include <vector>
#include "Generation_PIPELINE.h"
#include "../data/MapFields_DATA.h"
#include "../data/PlacementResults_DATA.h"
#include "../data/StratumArt_DATA.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../proc/Bake_PROC.h"
#include "../proc/Erosion_PROC.h"
#include "../proc/FlowAccumulation_PROC.h"
#include "../proc/Mask_PROC.h"
#include "../proc/NoiseBlend_PROC.h"
#include "../proc/Placement_PROC.h"
#include "../proc/Thermal_PROC.h"
#include "../sys/Dispatch_SYS.h"

namespace SanmapGen {
namespace Pipeline {

// Two-tier dirty distinction (ARCH §4.5 determinism scope, M4 preview work builds on it):
// FullRegeneration = a gameplay-authoritative stage whose output the game and the AI read;
// PreviewOnly = a decorative, determinism-exempt stage the preview can refresh on its own.
enum class RegenerationTier { FullRegeneration, PreviewOnly };

struct StageDescription {
    std::string      name;
    RegenerationTier tier = RegenerationTier::FullRegeneration;
};

class GenerationAssembler {
public:
    explicit GenerationAssembler(const Params::MapRecipe& recipeSettings);

    // Runs the dirty-hash conductor; returns the stages that actually ran this call.
    std::vector<std::string> Run();
    void InvalidateAll() { pipeline.InvalidateAll(); }
    std::size_t StageCount() const { return pipeline.StageCount(); }

    // Execution settings fanned out to every stage (PIPELINE resolves policy, ARCH §3.3).
    void SetGenerationContext(Sys::GenerationContext context);
    void SetGlobalBackend(Sys::ComputeBackend backend);
    void SetGpuResourceManager(Sys::GpuResourceManager* manager);
    void SetThreadPool(Sys::ThreadPool* pool);

    // The computed output the stages write.
    Data::MapFields&        Fields()        { return mapFields; }
    const Data::MapFields&  Fields() const  { return mapFields; }
    Data::PlacementResults& Placements()    { return placementResults; }
    Proc::BakedTextureSet&  BakedTextures() { return bakedTextures; }

    // The stages themselves — the tweakable constants each one owns (Constitution §8) are
    // reached through these until the remaining *_PARAMS homes exist (UI wiring is M4/M5).
    Proc::NoiseBlendStage&       NoiseBlend()       { return noiseBlendStage; }
    Proc::ErosionStage&          Erosion()          { return erosionStage; }
    Proc::ThermalStage&          Thermal()          { return thermalStage; }
    Proc::FlowAccumulationStage& FlowAccumulation() { return flowAccumulationStage; }
    Proc::MaskStage&             Mask()             { return maskStage; }
    Proc::PlacementStage&        Placement()        { return placementStage; }
    Proc::BakeStage&             Bake()             { return bakeStage; }
    // The loaded stratum art (imported masks + albedo texels) the Mask and Bake stages read.
    // Settings live in the recipe's `strata`; only the loaded pixels live here (ARCH §7.1).
    std::vector<Data::StratumArt>& StratumArt() { return stratumArt; }

    // Two-tier dirty distinction.
    const std::vector<StageDescription>& StageDescriptions() const { return stageDescriptions; }
    const std::vector<std::string>& StagesThatRan() const { return stagesThatRan; }
    RegenerationTier TierOfStage(const std::string& stageName) const;
    bool DidLastRunRegenerate() const;    // a FullRegeneration-tier stage ran
    bool WasLastRunPreviewOnly() const;   // only PreviewOnly-tier stages ran

private:
    void RegisterStages();                // GenerationAssembler_Stages_PIPELINE.cpp
    void AddStage(const std::string& stageName, RegenerationTier tier,
                  std::function<std::size_t()> computeParameterHash, std::function<void()> run);

    const Params::MapRecipe&      recipe;
    std::vector<Data::StratumArt> stratumArt;

    Data::MapFields        mapFields;          // declared before the stages that reference them
    Data::PlacementResults placementResults;
    Proc::BakedTextureSet  bakedTextures;

    Proc::NoiseBlendStage       noiseBlendStage;
    Proc::ErosionStage          erosionStage;
    Proc::ThermalStage          thermalStage;
    Proc::FlowAccumulationStage flowAccumulationStage;
    Proc::MaskStage             maskStage;
    Proc::PlacementStage        placementStage;
    Proc::BakeStage             bakeStage;

    GenerationPipeline            pipeline;
    std::vector<StageDescription> stageDescriptions;
    std::vector<std::string>      stagesThatRan;
};

} // namespace Pipeline
} // namespace SanmapGen
