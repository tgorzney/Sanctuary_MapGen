// GenerationAssembler_PIPELINE.cpp — assembler lifecycle: construction (every stage bound to
// the recipe it reads and the DATA it writes), the execution-settings fan-out, the Run()
// hand-off to the dirty-hash conductor, and the two-tier dirty queries. The stage ORDER and
// its registration live in GenerationAssembler_Stages_PIPELINE.cpp (ARCH §1.5 — one class,
// several small .cpp aspects behind one header).
#include "GenerationAssembler_PIPELINE.h"

namespace SanmapGen {
namespace Pipeline {

GenerationAssembler::GenerationAssembler(const Params::MapRecipe& recipeSettings)
    : recipe(recipeSettings),
      stratumArt(static_cast<std::size_t>(Data::MapFields::stratumCount)),
      noiseBlendStage(recipeSettings.geometry, recipeSettings.layerStack, mapFields),
      erosionStage(recipeSettings.geometry, mapFields),
      thermalStage(recipeSettings.geometry, mapFields),
      flowAccumulationStage(recipeSettings.geometry, mapFields),
      maskStage(recipeSettings.geometry, recipeSettings.strata, stratumArt, mapFields,
                recipeSettings.slopeDefaults),
      placementStage(recipeSettings, mapFields, placementResults),
      bakeStage(recipeSettings.geometry, recipeSettings.strata, stratumArt, mapFields, bakedTextures) {
    RegisterStages();
}

// The recipe is the only validation gate the conductor needs: an invalid geometry would have
// every stage size its buffers from a nonsense extent (Constitution §6, validate then report).
std::vector<std::string> GenerationAssembler::Run() {
    stagesThatRan.clear();
    if (!recipe.IsValid()) return stagesThatRan;
    stagesThatRan = pipeline.Run();
    return stagesThatRan;
}

void GenerationAssembler::SetGenerationContext(Sys::GenerationContext context) {
    noiseBlendStage.SetGenerationContext(context);
    erosionStage.SetGenerationContext(context);
    thermalStage.SetGenerationContext(context);
    flowAccumulationStage.SetGenerationContext(context);
    maskStage.SetGenerationContext(context);
    placementStage.SetGenerationContext(context);
    bakeStage.SetGenerationContext(context);
}

void GenerationAssembler::SetGlobalBackend(Sys::ComputeBackend backend) {
    noiseBlendStage.SetGlobalBackend(backend);
    erosionStage.SetGlobalBackend(backend);
    thermalStage.SetGlobalBackend(backend);
    flowAccumulationStage.SetGlobalBackend(backend);
    maskStage.SetGlobalBackend(backend);
    placementStage.SetGlobalBackend(backend);
    bakeStage.SetGlobalBackend(backend);
}

void GenerationAssembler::SetGpuResourceManager(Sys::GpuResourceManager* manager) {
    noiseBlendStage.SetGpuResourceManager(manager);
    erosionStage.SetGpuResourceManager(manager);
    thermalStage.SetGpuResourceManager(manager);
    flowAccumulationStage.SetGpuResourceManager(manager);
    maskStage.SetGpuResourceManager(manager);
    placementStage.SetGpuResourceManager(manager);
    bakeStage.SetGpuResourceManager(manager);
}

// ErosionStage has no thread pool seam (its Cpu path is sequential by construction — droplet
// feedback is order-dependent, DETERMINISM_SPEC), so it is deliberately absent here.
void GenerationAssembler::SetThreadPool(Sys::ThreadPool* pool) {
    noiseBlendStage.SetThreadPool(pool);
    thermalStage.SetThreadPool(pool);
    flowAccumulationStage.SetThreadPool(pool);
    maskStage.SetThreadPool(pool);
    placementStage.SetThreadPool(pool);
    bakeStage.SetThreadPool(pool);
}

// Evaluating a stage's own hash costs a walk of its settings, never a stage run — so the two-tier
// dirty derivation can ask every stage "is this parameter yours?" between two user keystrokes.
std::size_t GenerationAssembler::ComputeStageParameterHash(std::size_t stageIndex) const {
    if (stageIndex >= stageParameterHashFunctions.size()) return 0;
    return stageParameterHashFunctions[stageIndex]();
}

RegenerationTier GenerationAssembler::TierOfStage(const std::string& stageName) const {
    for (const StageDescription& description : stageDescriptions)
        if (description.name == stageName) return description.tier;
    return RegenerationTier::FullRegeneration;
}

bool GenerationAssembler::DidLastRunRegenerate() const {
    for (const std::string& stageName : stagesThatRan)
        if (TierOfStage(stageName) == RegenerationTier::FullRegeneration) return true;
    return false;
}

bool GenerationAssembler::WasLastRunPreviewOnly() const {
    return !stagesThatRan.empty() && !DidLastRunRegenerate();
}

} // namespace Pipeline
} // namespace SanmapGen
