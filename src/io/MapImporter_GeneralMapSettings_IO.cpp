// MapImporter_GeneralMapSettings_IO.cpp — the top-level `.sanmap` `GeneralMapSettings` object ->
// `recipe.geometry`/`recipe.generalMapSettings`. Layer: IO. The exact inverse of
// MapExporter_GeneralMapSettings_IO.cpp. Same tier and calling contract as `SlopeDefaults`/
// `areas`/`armies`: takes the top-level `document` directly and MUST be called unconditionally,
// BEFORE the `mapGeneratorData` presence gate in MapImporter_IO.cpp.
//
// Load-bearing ordering note (ARCH Expert finding, SANMAP_FORMAT_SPEC Correction 2): this reader
// MUST run before `ReadGeometryJson` (MapImporter_Recipe_IO.cpp). That function's own clamp/`Warn`
// block at its end enforces the TerrainMinHeight/TerrainMaxHeight band and the WorldUnitsPerCell
// positivity floor; it stays correct post-relocation only because `geometry.terrainMinHeight`/
// `geometry.worldUnitsPerCell` are already set from THIS reader by the time that block runs.
#include "JsonPrimitives_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {

void ReadGeneralMapSettingsJson(const nlohmann::json& document, Params::MapRecipe& outRecipe) {
    if (!document.contains("GeneralMapSettings") || !document["GeneralMapSettings"].is_object()) return;
    const nlohmann::json& json = document["GeneralMapSettings"];
    Params::Geometry& geometry = outRecipe.geometry;

    // Seed's negative-value guard, replicated verbatim from ReadGeometryJson's own pre-relocation
    // read: a negative signed value clamps to 0 rather than wrapping around to ~4 billion when cast
    // to unsigned.
    int seed = static_cast<int>(geometry.seed);
    if (ReadJsonInteger(json, "Seed", seed))
        geometry.seed = seed > 0 ? static_cast<unsigned int>(seed) : 0u;

    ReadJsonBoolean(json, "ScaleFeaturesToMapSize", geometry.bScaleFeaturesToMapSize);
    ReadJsonFloat(json, "GlobalGravity", outRecipe.generalMapSettings.globalGravity);
    ReadJsonFloat(json, "TerrainMinHeight", geometry.terrainMinHeight);
    ReadJsonFloat(json, "WorldUnitsPerCell", geometry.worldUnitsPerCell);
}

} // namespace Io
} // namespace SanmapGen
