// MapExporter_DetailNormal_IO.cpp — `recipe.detailNormal` -> the top-level `.sanmap` `DetailNormal`
// object. Layer: IO. SANMAP_FORMAT_SPEC Correction 8: one flat object, sibling of `armies`/
// `atmosphere`/`SlopeDefaults`/`Flow`, NOT nested in `mapGeneratorData`. Reserves the one live
// field, `DetailNormalMapSize`; the layered-heightmap-delta system itself is out of scope.
#include "MapExporter_Recipe_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {

nlohmann::ordered_json BuildDetailNormalJson(const Params::MapRecipe& recipe) {
    nlohmann::ordered_json json;
    json["DetailNormalMapSize"] = recipe.detailNormal.mapSize;
    return json;
}

} // namespace Io
} // namespace SanmapGen
