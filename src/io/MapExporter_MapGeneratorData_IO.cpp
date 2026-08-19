// MapExporter_MapGeneratorData_IO.cpp — `BuildMapGeneratorDataJson`, moved out of
// MapExporter_Recipe_IO.cpp verbatim (STEP31_ExporterRecipeOrchestrator_IO). Real per-field domain
// logic for exactly one top-level section (`mapGeneratorData`), same footing as
// `BuildAreasJson`/every other domain builder — its declaration stays in MapExporter_Recipe_IO.h,
// only the implementation moved here. Layer: IO.
#include "MapExporter_Recipe_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {

nlohmann::ordered_json BuildMapGeneratorDataJson(const Params::MapRecipe& recipe) {
    nlohmann::ordered_json generatorData;
    // Seed/ScaleFeaturesToMapSize/TerrainMinHeight/WorldUnitsPerCell RELOCATED to the top-level
    // `GeneralMapSettings` object (SANMAP_FORMAT_SPEC Correction 2) — no longer written here.
    generatorData["MapSize"]                = recipe.geometry.mapSize;
    generatorData["TerrainMaxHeight"]       = recipe.geometry.terrainMaxHeight;
    // GlobalSymmetryMask RELOCATED to the top-level `Symmetry` object (SANMAP_FORMAT_SPEC
    // Correction 4, STEP16, BuildSymmetryJson below) — no longer written here.
    // SimulationGrouping/GeoLayers RELOCATED to the top-level `HeightmapStack` object
    // (SANMAP_FORMAT_SPEC Correction 3, BuildHeightmapStackJson below) — no longer written here.
    generatorData["Stratums"]               = BuildStrataSettingsJson(recipe);
    nlohmann::ordered_json water;
    water["Enabled"]           = recipe.water.bEnabled;
    water["WaterLevelMax"]     = recipe.water.waterLevelMaximum;
    water["DeepWaterDepthMin"] = recipe.water.deepWaterDepthMinimum;
    water["DeepWaterDepthMax"] = recipe.water.deepWaterDepthMaximum;
    generatorData["Water"] = water;
    // The four placement-rule vectors RELOCATED to the top-level `MarkersStack`/`PropsStack`/
    // `DecalsStack`/`UnitsStack` keys (SANMAP_FORMAT_SPEC Correction 7, ruling #3) — no longer
    // written here.
    return generatorData;
}

} // namespace Io
} // namespace SanmapGen
