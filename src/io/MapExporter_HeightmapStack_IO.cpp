// MapExporter_HeightmapStack_IO.cpp — `recipe.layerStack` -> the top-level `HeightmapStack` object.
// Layer: IO. SANMAP_FORMAT_SPEC Correction 3: relocates `GeoLayers`/`SimulationGrouping` out of the
// legacy `mapGeneratorData` blob (where `BuildLayerStackJson` used to write them as two separate
// stray keys, `MapExporter_Recipe_IO.cpp`) into this one new top-level section, a sibling of
// `mapGeneratorData`. `BuildLayerJson`/`BuildGeoLayerJson`'s bodies are relocated verbatim from the
// deleted `MapExporter_Layers_IO.cpp` functions of the same name — only their container and the two
// new symmetry fields changed; `GeoLayer`/`Layer`'s existing keys keep their exact spellings.
#include "MapExporter_Recipe_IO.h"
#include "MapExporter_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {
namespace {

nlohmann::ordered_json BuildLayerJson(const Params::Layer& layer) {
    nlohmann::ordered_json json;
    json["Name"]         = layer.name;
    json["Enabled"]      = layer.bEnabled;
    // STEP152: generation-inclusion ONLY, independent of Enabled (UI visibility) above.
    json["Disabled"]     = layer.bDisabled;
    json["Locked"]       = layer.bLocked;
    json["StratumIndex"] = layer.stratumIndex;

    // STEP99_BakedImageLayer_PARAMS: baked/image-source state. NOT one-way (see Layer_PARAMS.h) —
    // a baked NOISE layer's recipe below still round-trips verbatim so it can resume live
    // generation when unbaked.
    json["Baked"]           = layer.bBaked;
    json["BakedImagePath"]  = layer.bakedImagePath;
    json["LayerIdentifier"] = layer.layerIdentifier;

    json["NoiseType"]        = static_cast<int>(layer.noiseType);
    json["FractalType"]      = static_cast<int>(layer.fractalType);
    json["Frequency"]        = layer.frequency;
    json["Octaves"]          = layer.octaves;
    json["Gain"]             = layer.gain;
    json["Lacunarity"]       = layer.lacunarity;
    json["WeightedStrength"] = layer.weightedStrength;
    json["PingPongStrength"] = layer.pingPongStrength;
    json["CellularJitter"]   = layer.cellularJitter;

    json["LandDensity"]     = layer.landDensity;
    json["MountainDensity"] = layer.mountainDensity;
    json["PlateauDensity"]  = layer.plateauDensity;
    json["RampDensity"]     = layer.rampDensity;

    json["LevelsShadows"]     = layer.levelsShadows;
    json["LevelsMidtones"]    = layer.levelsMidtones;
    json["LevelsHighlights"]  = layer.levelsHighlights;
    json["LevelsOutputBlack"] = layer.levelsOutputBlack;
    json["LevelsOutputWhite"] = layer.levelsOutputWhite;

    json["BlendMode"]           = static_cast<int>(layer.blendMode);
    json["Opacity"]             = layer.opacity;
    json["HeightBlendContrast"] = layer.heightBlendContrast;
    json["HeightBlendMinimum"]  = layer.heightBlendMinimum;
    json["HeightBlendMaximum"]  = layer.heightBlendMaximum;

    // SANMAP_FORMAT_SPEC Correction 3, "Named gap, explicitly deferred" (matching the gap-comment
    // pattern StratumAppearance_PARAMS.h uses for a field the format has but PARAMS doesn't yet):
    // the real map format's per-layer MinHeight/MaxHeight/MinSlope/MaxSlope height-and-slope gates
    // have no equivalent field on Params::Layer at all today (confirmed absent) — not added here;
    // logged for a future LAYER_SYSTEM_SPEC conversation, not an oversight of THIS ticket.

    // Correction 3's genuinely new fields: a local symmetry override, matching the pattern already
    // live on MarkerRule/PropRule/UnitRule — settings-only, ZERO PROC consumer (no heightfield-
    // symmetry stage exists yet; Correction 4/ARCH territory, explicitly deferred).
    json["SymmetryUseGlobal"] = layer.bSymmetryUseGlobal;
    json["SymmetryMask"]      = layer.symmetryMask;
    json["RadialSymmetryRepeatCount"] = layer.radialSymmetryRepeatCount;
    return json;
}

nlohmann::ordered_json BuildGeoLayerJson(const Params::GeoLayer& geoLayer) {
    nlohmann::ordered_json json;
    json["Name"]         = geoLayer.name;
    json["Enabled"]      = geoLayer.bEnabled;
    // STEP152: generation-inclusion ONLY, independent of Enabled (UI visibility) above.
    json["Disabled"]     = geoLayer.bDisabled;
    json["Mode"]         = static_cast<int>(geoLayer.mode);
    json["ErodeBelow"]   = geoLayer.bErodeBelow;
    json["BlendMode"]    = static_cast<int>(geoLayer.blendMode);
    json["StratumIndex"] = geoLayer.stratumIndex;
    // Correction 3's genuinely new fields — same posture as Layer's own pair above.
    json["SymmetryUseGlobal"] = geoLayer.bSymmetryUseGlobal;
    json["SymmetryMask"]      = geoLayer.symmetryMask;
    json["RadialSymmetryRepeatCount"] = geoLayer.radialSymmetryRepeatCount;
    nlohmann::ordered_json layers = nlohmann::ordered_json::array();
    for (const Params::Layer& layer : geoLayer.layers) layers.push_back(BuildLayerJson(layer));
    json["Layers"] = layers;
    return json;
}

} // namespace

nlohmann::ordered_json BuildHeightmapStackJson(const Params::LayerStack& layerStack) {
    nlohmann::ordered_json heightmapStack;
    heightmapStack["SimulationGrouping"] = static_cast<int>(layerStack.simulationGrouping);
    nlohmann::ordered_json geoLayers = nlohmann::ordered_json::array();
    for (const Params::GeoLayer& geoLayer : layerStack.geoLayers)
        geoLayers.push_back(BuildGeoLayerJson(geoLayer));
    heightmapStack["GeoLayers"] = geoLayers;
    return heightmapStack;
}

} // namespace Io
} // namespace SanmapGen
