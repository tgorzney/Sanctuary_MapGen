// GenerationAssembler_TestScene_PIPELINE.h — test-only scaffolding for the M3-8 end-to-end
// acceptance test (not part of the layer graph; nothing in src/ includes it outside a
// *_Test.cpp). Builds the MapRecipe the whole pipeline runs on — a two-layer stack, the ONE
// per-stratum settings array (slope gate + tints, ARCH 7.1), one enabled erosion layer and
// marker + prop scatter rules — plus the small field probes the assertions read.
#pragma once
#include "GenerationAssembler_PIPELINE.h"
#include <cstring>

namespace AssemblerTest {

using namespace SanmapGen;

constexpr int mapSize            = 64;
constexpr int vertexSize         = mapSize + 1;
constexpr int detailStratumIndex = 2;
constexpr int erosionDropletCount = 4000;
constexpr int thermalIterationCount = 8;
constexpr int markerCount        = 4;

inline Params::ScatterTransform MakeTransform(const char* templateIdentifier) {
    Params::ScatterTransform transform;
    for (int index = 0; index < 7 && templateIdentifier[index] != '\0'; ++index)
        transform.templateIdentifier[index] = templateIdentifier[index];
    return transform;
}

// One GeoLayer with a broad base layer and a finer detail layer that owns its own stratum,
// so the mask stage has two strata to gate and the bake has two weights to composite.
inline void AddLayerStack(Params::MapRecipe& recipe) {
    Params::GeoLayer group;
    group.name = "Terrain";
    Params::Layer baseLayer;
    baseLayer.stratumIndex = 0;
    baseLayer.frequency    = 0.02f;
    baseLayer.octaves      = 4;
    group.layers.push_back(baseLayer);
    Params::Layer detailLayer;
    detailLayer.stratumIndex        = detailStratumIndex;
    detailLayer.frequency           = 0.08f;
    detailLayer.octaves             = 3;
    detailLayer.opacity             = 0.4f;
    detailLayer.heightBlendMinimum  = 0.2f;
    detailLayer.heightBlendMaximum  = 0.9f;
    group.layers.push_back(detailLayer);
    recipe.layerStack.geoLayers.push_back(group);
}

// The one per-stratum settings array the Mask and Bake stages both read (no rival arrays).
inline void AddStrata(Params::MapRecipe& recipe) {
    recipe.strata.assign(static_cast<std::size_t>(Data::MapFields::stratumCount), Params::Stratum());
    Params::Stratum& detailStratum = recipe.strata[detailStratumIndex];
    detailStratum.bSlopeUseGlobal         = false;   // exercise its OWN window, not slopeDefaults
    detailStratum.bSlopeGateEnabled       = true;
    detailStratum.maximumSlopeDegrees     = 25.0f;
    detailStratum.bUseSmoothstep          = true;
    detailStratum.slopeFeatherDegreesHigh = 5.0f;
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum) {
        recipe.strata[stratum].tintRed   = 0.1f + 0.1f * static_cast<float>(stratum);
        recipe.strata[stratum].tintGreen = 1.0f - 0.1f * static_cast<float>(stratum);
        recipe.strata[stratum].tintBlue  = 0.5f;
    }
}

inline Params::MapRecipe MakeRecipe(unsigned int seed) {
    Params::MapRecipe recipe;
    recipe.geometry.mapSize          = mapSize;
    recipe.geometry.seed             = seed;
    recipe.geometry.terrainMaxHeight = 128.0f;
    AddLayerStack(recipe);
    AddStrata(recipe);

    Params::MarkerRule spawnRule;            // hashed selection: no clearance scoring needed
    spawnRule.category         = Params::MarkerCategory::Spawn;
    spawnRule.count            = markerCount;
    spawnRule.clearanceSpacing = 12.0f;
    spawnRule.mapEdgePadding   = 6;
    spawnRule.bRandomSelection = true;
    spawnRule.transform        = MakeTransform("m002");
    recipe.markerRules.push_back(spawnRule);

    Params::PropRule propRule;
    propRule.density        = 0.3f;
    propRule.spacingMinimum = 5.0f;
    propRule.mapEdgePadding = 3;
    propRule.transform      = MakeTransform("edbm014");
    recipe.propRules.push_back(propRule);
    return recipe;
}

// The stage-owned constants (the per-stratum settings live in the recipe, ARCH 7.1).
inline void ConfigureStages(Pipeline::GenerationAssembler& assembler) {
    Proc::ErosionLayerSettings& erosionLayer = assembler.Erosion().LayerSettings(0);
    erosionLayer.bEnabled     = true;
    erosionLayer.dropletCount = erosionDropletCount;

    assembler.Thermal().Constants().iterationCount = thermalIterationCount;
}

inline unsigned long long FieldChecksum(const Data::FloatField& field) {
    unsigned long long checksum = 1469598103934665603ull;
    for (std::size_t index = 0; index < field.CellCount(); ++index) {
        unsigned int bits = 0;
        std::memcpy(&bits, field.Data() + index, sizeof(bits));
        checksum = (checksum ^ bits) * 1099511628211ull;
    }
    return checksum;
}

// Largest height difference between axis neighbours — thermal relaxation drives this down to
// the talus threshold, so it is the probe that proves the slump pass actually ran.
inline float MaximumNeighbourDrop(const Data::FloatField& field) {
    float maximum = 0.0f;
    for (int y = 0; y < field.Height(); ++y)
        for (int x = 0; x + 1 < field.Width(); ++x) {
            const float horizontal = field.Get(x, y) - field.Get(x + 1, y);
            const float absolute = horizontal < 0.0f ? -horizontal : horizontal;
            if (absolute > maximum) maximum = absolute;
        }
    return maximum;
}

} // namespace AssemblerTest
