// MapExporter_Layers_IO.cpp — the layer stack and the per-stratum settings, as `mapGeneratorData`
// JSON. Layer: IO. One writer function per PARAMS struct, each a literal field-for-field mirror so
// a reader of MapImporter_Layers_IO.cpp can diff the two by eye.
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
    json["Locked"]       = layer.bLocked;
    json["StratumIndex"] = layer.stratumIndex;

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
    return json;
}

nlohmann::ordered_json BuildGeoLayerJson(const Params::GeoLayer& geoLayer) {
    nlohmann::ordered_json json;
    json["Name"]         = geoLayer.name;
    json["Enabled"]      = geoLayer.bEnabled;
    json["Mode"]         = static_cast<int>(geoLayer.mode);
    json["ErodeBelow"]   = geoLayer.bErodeBelow;
    json["BlendMode"]    = static_cast<int>(geoLayer.blendMode);
    json["StratumIndex"] = geoLayer.stratumIndex;
    nlohmann::ordered_json layers = nlohmann::ordered_json::array();
    for (const Params::Layer& layer : geoLayer.layers) layers.push_back(BuildLayerJson(layer));
    json["Layers"] = layers;
    return json;
}

nlohmann::ordered_json BuildStratumJson(const Params::Stratum& stratum) {
    nlohmann::ordered_json json;
    json["SlopeGateEnabled"]       = stratum.bSlopeGateEnabled;
    json["MinimumSlopeDegrees"]    = stratum.minimumSlopeDegrees;
    json["MaximumSlopeDegrees"]    = stratum.maximumSlopeDegrees;
    json["SlopeFeatherDegreesLow"] = stratum.slopeFeatherDegreesLow;
    json["SlopeFeatherDegreesHigh"] = stratum.slopeFeatherDegreesHigh;
    json["UseSmoothstep"]          = stratum.bUseSmoothstep;
    json["InvertSlopeGate"]        = stratum.bInvertSlopeGate;
    json["SlopeGateStrength"]      = stratum.slopeGateStrength;
    json["ImportedMaskMode"]       = static_cast<int>(stratum.importedMaskMode);
    json["MaskRemapMinimum"]       = stratum.maskRemapMinimum;
    json["MaskRemapMaximum"]       = stratum.maskRemapMaximum;
    json["Enabled"]                = stratum.bEnabled;
    json["TintRed"]                = stratum.tintRed;
    json["TintGreen"]              = stratum.tintGreen;
    json["TintBlue"]               = stratum.tintBlue;
    json["TileCount"]              = stratum.tileCount;
    return json;
}

} // namespace

nlohmann::ordered_json BuildLayerStackJson(const Params::LayerStack& layerStack) {
    nlohmann::ordered_json geoLayers = nlohmann::ordered_json::array();
    for (const Params::GeoLayer& geoLayer : layerStack.geoLayers)
        geoLayers.push_back(BuildGeoLayerJson(geoLayer));
    return geoLayers;
}

nlohmann::ordered_json BuildStrataSettingsJson(const Params::MapRecipe& recipe) {
    nlohmann::ordered_json strata = nlohmann::ordered_json::array();
    for (const Params::Stratum& stratum : recipe.strata) strata.push_back(BuildStratumJson(stratum));
    return strata;
}

} // namespace Io
} // namespace SanmapGen
