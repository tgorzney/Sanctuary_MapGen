// MapExporter_Recipe_IO.cpp — the `.sanmap` document itself: the format's own top-level fields
// plus the `mapGeneratorData` generator-state block. Layer: IO.
// Keys are the format's verbatim strings (ARCH §1.1 naming exception — "identifiers the file
// format dictates ... verbatim so import/export round-trips").
#include "MapExporter_Recipe_IO.h"
#include "MapExporter_IO.h"
#include "../params/MapRecipe_PARAMS.h"
#include <cmath>

namespace SanmapGen {
namespace Io {

nlohmann::ordered_json BuildStratumLayersJson(const Params::MapRecipe& recipe) {
    nlohmann::ordered_json stratumLayers = nlohmann::ordered_json::array();
    for (int stratumIndex = 0; stratumIndex < sanmapStratumCount; ++stratumIndex) {
        const bool bHasSettings = stratumIndex < static_cast<int>(recipe.strata.size());
        const Params::Stratum stratum = bHasSettings ? recipe.strata[stratumIndex] : Params::Stratum();
        nlohmann::ordered_json layer;
        layer["name"]        = "Stratum " + std::to_string(stratumIndex);
        layer["albedo"]      = { { "path", "" } };
        layer["normal"]      = { { "path", "" } };
        layer["mask"]        = { { "path", "" } };
        layer["tileSize"]    = { { "x", stratum.tileCount }, { "y", stratum.tileCount } };
        layer["tileSizeFar"] = { { "x", stratum.tileCount }, { "y", stratum.tileCount } };
        layer["diffuseRemap"] = { { "r", stratum.tintRed }, { "g", stratum.tintGreen },
                                  { "b", stratum.tintBlue }, { "a", 1.0f } };
        // Real Vector4, matching the C# ground truth `SanMap.Types.cs` (ARCH §7.2 item 10) — not
        // the bare scalar this line used to write.
        layer["maskRemapMin"] = { {"x", stratum.maskRemapMinimum[0]}, {"y", stratum.maskRemapMinimum[1]},
                                  {"z", stratum.maskRemapMinimum[2]}, {"w", stratum.maskRemapMinimum[3]} };
        layer["maskRemapMax"] = { {"x", stratum.maskRemapMaximum[0]}, {"y", stratum.maskRemapMaximum[1]},
                                  {"z", stratum.maskRemapMaximum[2]}, {"w", stratum.maskRemapMaximum[3]} };
        stratumLayers.push_back(layer);
    }
    return stratumLayers;
}

nlohmann::ordered_json BuildMapGeneratorDataJson(const Params::MapRecipe& recipe) {
    nlohmann::ordered_json generatorData;
    generatorData["MapGeneratorDataVersion"] = mapGeneratorDataVersion;
    generatorData["MapSize"]                = recipe.geometry.mapSize;
    generatorData["Seed"]                   = recipe.geometry.seed;
    generatorData["TerrainMinHeight"]       = recipe.geometry.terrainMinHeight;
    generatorData["TerrainMaxHeight"]       = recipe.geometry.terrainMaxHeight;
    generatorData["ScaleFeaturesToMapSize"] = recipe.geometry.bScaleFeaturesToMapSize;
    generatorData["WorldUnitsPerCell"]      = recipe.geometry.worldUnitsPerCell;
    generatorData["GlobalSymmetryMask"]     = recipe.globalSymmetryMask;
    generatorData["SimulationGrouping"]     = static_cast<int>(recipe.layerStack.simulationGrouping);
    generatorData["GeoLayers"]              = BuildLayerStackJson(recipe.layerStack);
    generatorData["Stratums"]               = BuildStrataSettingsJson(recipe);
    nlohmann::ordered_json water;
    water["Enabled"]           = recipe.water.bEnabled;
    water["WaterLevelMax"]     = recipe.water.waterLevelMaximum;
    water["DeepWaterDepthMin"] = recipe.water.deepWaterDepthMinimum;
    water["DeepWaterDepthMax"] = recipe.water.deepWaterDepthMaximum;
    generatorData["Water"] = water;
    generatorData["PlacementRules"] = BuildPlacementRulesJson(recipe);
    return generatorData;
}

std::string MapExporter::BuildSanmapJsonText(const Params::MapRecipe& recipe,
                                             const MapExportOptions& options) {
    const Params::Geometry& geometry = recipe.geometry;
    nlohmann::ordered_json document;
    document["fileVersion"] = sanmapFileVersion;
    document["mapVersion"]  = sanmapMapVersion;
    document["name"]        = options.mapName;
    document["credits"]     = options.mapCredits;
    document["width"]       = geometry.mapSize;
    document["length"]      = geometry.mapSize;
    // The format types `height` as a C# int (SanMap.cs:24). Round, don't truncate — a
    // designer-set 127.6 must land on 128, not silently drop to 127 in Newtonsoft's coercion.
    document["height"]      = static_cast<int>(std::lround(geometry.terrainMaxHeight));
    document["heightmapResolution"] = geometry.VertexSize();

    document["hasWater"]   = recipe.water.bEnabled;
    document["waterLevel"] = recipe.water.waterLevelMaximum;
    document["waterDepth"] = recipe.water.deepWaterDepthMaximum;

    document["shader"]            = "RTS/TerrainLit";
    document["heightTransition"]  = 0.5f;
    document["fadeDistance"]      = 128.0f;
    document["fadeStartDistance"] = 1.0f;
    document["stratumLayers"]     = BuildStratumLayersJson(recipe);

    // SCOPE NOTE 1 (MapExporter_IO.h): `areas`/`armies` now round-trip real content; the remaining
    // entity domains are still written empty and VALID rather than omitted, so an importer that
    // expects them never sees a missing key.
    document["areas"]   = BuildAreasJson(recipe);
    document["armies"]  = BuildArmiesJson(recipe);
    document["markers"] = nlohmann::ordered_json::object();
    document["chains"]  = nlohmann::ordered_json::object();
    document["decals"]  = nlohmann::ordered_json::array();
    document["props"]   = nlohmann::ordered_json::array();

    document["mapGeneratorData"] = BuildMapGeneratorDataJson(recipe);
    const int indent = options.jsonIndentSpaceCount > 0 ? options.jsonIndentSpaceCount : -1;
    return document.dump(indent);
}

} // namespace Io
} // namespace SanmapGen
