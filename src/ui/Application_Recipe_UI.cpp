// Application_Recipe_UI.cpp — the shell's "new map" defaults: the MapRecipe a fresh launch
// generates from, the stage constants it starts with, and the preview composition it shows.
// Layer: UI. These are DEFAULT VALUES of settings types that already exist — no PARAMS type is
// extended and no constant is hidden at a use site (Constitution §8); a designer moves every one
// of them from a tab. The shell is allowed to choose defaults because it is the caller that
// constructs the recipe; it does not thereby own any generation logic (ARCH §3.2).
#include "Application_UI.h"

namespace SanmapGen {
namespace Ui {
namespace {

constexpr int   defaultMapSize            = 256;
constexpr int   defaultDetailStratumIndex = 2;
constexpr int   defaultErosionDropletCount = 24000;
constexpr int   defaultThermalIterationCount = 16;
constexpr float defaultTerrainMaxHeight   = 128.0f;
constexpr float defaultWaterLevel         = 26.0f;
constexpr float defaultDeepWaterDepth     = 34.0f;

// One GeoLayer with a broad base and a finer detail layer that owns its own stratum, so the Mask
// stage has two strata to gate and the splat has two weights to composite.
void AddDefaultLayerStack(Params::MapRecipe& recipe) {
    Params::GeoLayer group;
    group.name = "Terrain";
    Params::Layer baseLayer;
    baseLayer.stratumIndex = 0;
    baseLayer.frequency    = 0.006f;
    baseLayer.octaves      = 5;
    group.layers.push_back(baseLayer);
    Params::Layer detailLayer;
    detailLayer.stratumIndex       = defaultDetailStratumIndex;
    detailLayer.frequency          = 0.024f;
    detailLayer.octaves            = 4;
    detailLayer.opacity            = 0.35f;
    detailLayer.heightBlendMinimum = 0.25f;
    detailLayer.heightBlendMaximum = 0.95f;
    group.layers.push_back(detailLayer);
    recipe.layerStack.geoLayers.push_back(group);
}

// The ONE per-stratum settings array both Mask and Bake read (ARCH §7.1) — no rival array.
void AddDefaultStrata(Params::MapRecipe& recipe) {
    recipe.strata.assign(static_cast<std::size_t>(Data::MapFields::stratumCount), Params::Stratum());
    Params::Stratum& detailStratum = recipe.strata[defaultDetailStratumIndex];
    detailStratum.bSlopeGateEnabled       = true;
    detailStratum.maximumSlopeDegrees     = 28.0f;
    detailStratum.bUseSmoothstep          = true;
    detailStratum.slopeFeatherDegreesHigh = 6.0f;
    recipe.strata[0].tintRed = 0.34f; recipe.strata[0].tintGreen = 0.30f; recipe.strata[0].tintBlue = 0.26f;
    recipe.strata[1].tintRed = 0.52f; recipe.strata[1].tintGreen = 0.47f; recipe.strata[1].tintBlue = 0.38f;
    recipe.strata[2].tintRed = 0.29f; recipe.strata[2].tintGreen = 0.44f; recipe.strata[2].tintBlue = 0.22f;
}

// STEP80 §5: the same spawn MarkerRule as before, now seeded inside one default MarkerRuleLayer
// ("Spawn Layer") rather than pushed onto a flat vector. The layer's `symmetry` keeps
// SymmetrySetting's defaults (bSymmetryUseGlobal = true) — identical effective behaviour to the
// old rule-level default, so no default map changes shape.
void AddDefaultPlacementRules(Params::MapRecipe& recipe) {
    Params::MarkerRule spawnRule;
    spawnRule.category         = Params::MarkerCategory::Spawn;
    spawnRule.count            = 4;
    spawnRule.clearanceSpacing = 24.0f;
    spawnRule.mapEdgePadding   = 12;
    spawnRule.bRandomSelection = true;
    Params::MarkerRuleLayer spawnLayer;
    spawnLayer.name = "Spawn Layer";
    spawnLayer.rules.push_back(spawnRule);
    recipe.markerRuleLayers.push_back(spawnLayer);

    Params::PropRule propRule;
    propRule.density        = 0.15f;
    propRule.spacingMinimum = 6.0f;
    propRule.mapEdgePadding = 4;
    recipe.propRules.push_back(propRule);
}

} // namespace

Params::MapRecipe MakeDefaultMapRecipe() {
    Params::MapRecipe recipe;
    recipe.geometry.mapSize          = defaultMapSize;
    recipe.geometry.seed             = 20260815u;
    recipe.geometry.terrainMaxHeight = defaultTerrainMaxHeight;
    recipe.water.bEnabled              = true;
    recipe.water.waterLevelMaximum     = defaultWaterLevel;
    recipe.water.deepWaterDepthMaximum = defaultDeepWaterDepth;
    AddDefaultLayerStack(recipe);
    AddDefaultStrata(recipe);
    AddDefaultPlacementRules(recipe);
    return recipe;
}

// Stage-owned constants (the per-stratum ones live in the recipe, ARCH §7.1). Reached through the
// assembler's stage accessors, which is the only door PIPELINE opens for them.
void ConfigureDefaultStages(Pipeline::GenerationAssembler& assembler) {
    Proc::ErosionLayerSettings& erosionLayer = assembler.Erosion().LayerSettings(0);
    erosionLayer.bEnabled     = true;
    erosionLayer.dropletCount = defaultErosionDropletCount;
    assembler.Thermal().Constants().iterationCount = defaultThermalIterationCount;
}

} // namespace Ui
} // namespace SanmapGen
