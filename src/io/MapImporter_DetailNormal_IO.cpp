// MapImporter_DetailNormal_IO.cpp — the top-level `.sanmap` `DetailNormal` object ->
// `recipe.detailNormal`. Layer: IO. The exact inverse of MapExporter_DetailNormal_IO.cpp. Same
// tier and calling contract as `Flow`/`SlopeDefaults`/`areas`/`armies`: takes the top-level
// `document` directly and MUST be called unconditionally, BEFORE the `mapGeneratorData` presence
// gate in MapImporter_IO.cpp.
#include "JsonPrimitives_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {

void ReadDetailNormalJson(const nlohmann::json& document, Params::MapRecipe& outRecipe) {
    if (!document.contains("DetailNormal") || !document["DetailNormal"].is_object()) return;
    const nlohmann::json& json = document["DetailNormal"];
    ReadJsonInteger(json, "DetailNormalMapSize", outRecipe.detailNormal.mapSize);
}

} // namespace Io
} // namespace SanmapGen
