// MapExporter_Areas_IO.cpp — `recipe.areas` -> the top-level `.sanmap` `areas` dictionary.
// Layer: IO. Own file (not shared with Armies): `areas`/`armies` are independent top-level format
// keys with no shared JSON parent (IO Architecture Expert ruling, STEP2_ArmiesAreas_IO).
// Flat; no recursion, no shared helper needed. Verbatim pass-through, no coordinate flip
// (ENTITY_AUTHORING_PARAMS_SPEC finding 3: MapArea never passes through TextureToWorldOrigin).
#include "MapExporter_Recipe_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {

nlohmann::ordered_json BuildAreasJson(const Params::MapRecipe& recipe) {
    nlohmann::ordered_json areas = nlohmann::ordered_json::object();
    for (const Params::MapArea& area : recipe.areas) {
        nlohmann::ordered_json areaJson;
        areaJson["x"]      = area.originX;
        areaJson["y"]      = area.originZ;
        areaJson["width"]  = area.width;
        areaJson["height"] = area.length;
        areas[area.name] = areaJson;
    }
    return areas;
}

} // namespace Io
} // namespace SanmapGen
