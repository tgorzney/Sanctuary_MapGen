// MapImporter_HeightmapStack_IO.cpp — the top-level `HeightmapStack` object -> `Params::LayerStack`.
// Layer: IO. The exact inverse of MapExporter_HeightmapStack_IO.cpp, key for key. SANMAP_FORMAT_SPEC
// Correction 3: `HeightmapStack` REPLACES the legacy `mapGeneratorData.GeoLayers`/`SimulationGrouping`
// pair (relocated, not duplicated) — same tier and calling contract as `PropsStack`/`SlopeDefaults`/
// etc.: takes the top-level `document` directly and must be called unconditionally, BEFORE the
// `mapGeneratorData` presence gate (`MapImporter_IO.cpp`). `ReadLayerJson`/`ReadGeoLayerJson`'s
// bodies are relocated verbatim from the deleted `MapImporter_Layers_IO.cpp` functions of the same
// name — only their container and the two new symmetry fields changed. Enum values are fenced to
// their declared range (GenerationEnums_PARAMS.h), so a document from a newer build degrades to the
// default member rather than casting a wild integer into an enum (Constitution §6).
#include "JsonPrimitives_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {
namespace {

// The declared value counts of the enums this file reads — the fence, in one place.
constexpr int noiseTypeCount          = 7;   // OpenSimplex2 .. None
constexpr int fractalTypeCount        = 4;   // None .. PingPong
constexpr int heightBlendModeCount    = 6;   // Add .. Minimum
constexpr int geoLayerModeCount       = 2;   // Material, Shaper
constexpr int simulationGroupingCount = 2;   // Separate, Unified

void ReadLayerJson(const nlohmann::json& json, Params::Layer& layer) {
    ReadJsonText(json, "Name", layer.name);
    ReadJsonBoolean(json, "Enabled", layer.bEnabled);
    ReadJsonBoolean(json, "Locked", layer.bLocked);
    ReadJsonInteger(json, "StratumIndex", layer.stratumIndex);

    int enumerationValue = static_cast<int>(layer.noiseType);
    if (ReadJsonEnumeration(json, "NoiseType", noiseTypeCount, enumerationValue))
        layer.noiseType = static_cast<Params::NoiseType>(enumerationValue);
    enumerationValue = static_cast<int>(layer.fractalType);
    if (ReadJsonEnumeration(json, "FractalType", fractalTypeCount, enumerationValue))
        layer.fractalType = static_cast<Params::FractalType>(enumerationValue);

    ReadJsonFloat(json, "Frequency", layer.frequency);
    ReadJsonInteger(json, "Octaves", layer.octaves);
    ReadJsonFloat(json, "Gain", layer.gain);
    ReadJsonFloat(json, "Lacunarity", layer.lacunarity);
    ReadJsonFloat(json, "WeightedStrength", layer.weightedStrength);
    ReadJsonFloat(json, "PingPongStrength", layer.pingPongStrength);
    ReadJsonFloat(json, "CellularJitter", layer.cellularJitter);

    ReadJsonFloat(json, "LandDensity", layer.landDensity);
    ReadJsonFloat(json, "MountainDensity", layer.mountainDensity);
    ReadJsonFloat(json, "PlateauDensity", layer.plateauDensity);
    ReadJsonFloat(json, "RampDensity", layer.rampDensity);

    ReadJsonFloat(json, "LevelsShadows", layer.levelsShadows);
    ReadJsonFloat(json, "LevelsMidtones", layer.levelsMidtones);
    ReadJsonFloat(json, "LevelsHighlights", layer.levelsHighlights);
    ReadJsonFloat(json, "LevelsOutputBlack", layer.levelsOutputBlack);
    ReadJsonFloat(json, "LevelsOutputWhite", layer.levelsOutputWhite);

    enumerationValue = static_cast<int>(layer.blendMode);
    if (ReadJsonEnumeration(json, "BlendMode", heightBlendModeCount, enumerationValue))
        layer.blendMode = static_cast<Params::HeightBlendMode>(enumerationValue);
    ReadJsonFloat(json, "Opacity", layer.opacity);
    ReadJsonFloat(json, "HeightBlendContrast", layer.heightBlendContrast);
    ReadJsonFloat(json, "HeightBlendMinimum", layer.heightBlendMinimum);
    ReadJsonFloat(json, "HeightBlendMaximum", layer.heightBlendMaximum);

    // SANMAP_FORMAT_SPEC Correction 3, "Named gap, explicitly deferred": the real map format's
    // per-layer MinHeight/MaxHeight/MinSlope/MaxSlope height-and-slope gates have no equivalent
    // field on Params::Layer at all today (confirmed absent) — not added here; logged for a future
    // LAYER_SYSTEM_SPEC conversation, not an oversight of THIS ticket.

    // Correction 3's genuinely new fields: a local symmetry override, ZERO PROC consumer today (no
    // heightfield-symmetry stage exists yet; Correction 4/ARCH territory, explicitly deferred).
    ReadJsonBoolean(json, "SymmetryUseGlobal", layer.bSymmetryUseGlobal);
    ReadJsonInteger(json, "SymmetryMask", layer.symmetryMask);
    ReadJsonInteger(json, "RadialSymmetryRepeatCount", layer.radialSymmetryRepeatCount);
}

void ReadGeoLayerJson(const nlohmann::json& json, Params::GeoLayer& geoLayer) {
    ReadJsonText(json, "Name", geoLayer.name);
    ReadJsonBoolean(json, "Enabled", geoLayer.bEnabled);
    int enumerationValue = static_cast<int>(geoLayer.mode);
    if (ReadJsonEnumeration(json, "Mode", geoLayerModeCount, enumerationValue))
        geoLayer.mode = static_cast<Params::GeoLayerMode>(enumerationValue);
    ReadJsonBoolean(json, "ErodeBelow", geoLayer.bErodeBelow);
    enumerationValue = static_cast<int>(geoLayer.blendMode);
    if (ReadJsonEnumeration(json, "BlendMode", heightBlendModeCount, enumerationValue))
        geoLayer.blendMode = static_cast<Params::HeightBlendMode>(enumerationValue);
    ReadJsonInteger(json, "StratumIndex", geoLayer.stratumIndex);
    // Correction 3's genuinely new fields — same posture as Layer's own pair above.
    ReadJsonBoolean(json, "SymmetryUseGlobal", geoLayer.bSymmetryUseGlobal);
    ReadJsonInteger(json, "SymmetryMask", geoLayer.symmetryMask);
    ReadJsonInteger(json, "RadialSymmetryRepeatCount", geoLayer.radialSymmetryRepeatCount);
    if (!json.contains("Layers") || !json["Layers"].is_array()) return;
    for (const nlohmann::json& layerJson : json["Layers"]) {
        Params::Layer layer;
        if (layerJson.is_object()) ReadLayerJson(layerJson, layer);
        geoLayer.layers.push_back(layer);
    }
}

} // namespace

void ReadHeightmapStackJson(const nlohmann::json& document, Params::LayerStack& outLayerStack) {
    if (!document.contains("HeightmapStack") || !document["HeightmapStack"].is_object()) return;
    const nlohmann::json& heightmapStack = document["HeightmapStack"];
    int groupingValue = static_cast<int>(outLayerStack.simulationGrouping);
    if (ReadJsonEnumeration(heightmapStack, "SimulationGrouping", simulationGroupingCount, groupingValue))
        outLayerStack.simulationGrouping = static_cast<Params::SimulationGrouping>(groupingValue);
    if (!heightmapStack.contains("GeoLayers") || !heightmapStack["GeoLayers"].is_array()) return;
    outLayerStack.geoLayers.clear();
    for (const nlohmann::json& geoLayerJson : heightmapStack["GeoLayers"]) {
        Params::GeoLayer geoLayer;
        if (geoLayerJson.is_object()) ReadGeoLayerJson(geoLayerJson, geoLayer);
        outLayerStack.geoLayers.push_back(geoLayer);
    }
}

} // namespace Io
} // namespace SanmapGen
