// MapExporter_GeneralMapSettings_IO.cpp — `recipe.geometry`/`recipe.generalMapSettings` -> the
// top-level `.sanmap` `GeneralMapSettings` object. Layer: IO. SANMAP_FORMAT_SPEC Correction 2:
// relocates `Seed`/`ScaleFeaturesToMapSize`/`TerrainMinHeight`/`WorldUnitsPerCell` OUT of the
// legacy `mapGeneratorData` blob (BuildMapGeneratorDataJson no longer writes them — see that
// function's own comment) and adds one genuinely new field, `GlobalGravity`. One flat object,
// sibling of `armies`/`atmosphere`/`SlopeDefaults`, NOT nested in `mapGeneratorData` — same
// posture as MapExporter_SlopeDefaults_IO.cpp.
#include "MapExporter_Recipe_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {

nlohmann::ordered_json BuildGeneralMapSettingsJson(const Params::MapRecipe& recipe) {
    const Params::Geometry& geometry = recipe.geometry;
    nlohmann::ordered_json json;
    json["Seed"]                   = geometry.seed;
    json["ScaleFeaturesToMapSize"] = geometry.bScaleFeaturesToMapSize;
    json["GlobalGravity"]          = recipe.generalMapSettings.globalGravity;
    json["TerrainMinHeight"]       = geometry.terrainMinHeight;
    json["WorldUnitsPerCell"]      = geometry.worldUnitsPerCell;
    return json;
}

} // namespace Io
} // namespace SanmapGen
