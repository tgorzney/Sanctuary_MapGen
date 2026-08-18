// MapImporter_Areas_IO.cpp — the top-level `.sanmap` `areas` dictionary -> `recipe.areas`.
// Layer: IO. The exact inverse of MapExporter_Areas_IO.cpp. `areas` is a JSON OBJECT keyed by
// MapArea::name (a dictionary, not an array — ENTITY_AUTHORING_PARAMS_SPEC finding 1), unlike the
// MapExporter_Rules_IO.cpp/MapImporter_Rules_IO.cpp array pattern this deliberately does not copy.
// Flat; no recursion needed. No coordinate flip (finding 3).
#include "JsonPrimitives_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {

void ReadAreasJson(const nlohmann::json& document, Params::MapRecipe& outRecipe) {
    if (!document.contains("areas") || !document["areas"].is_object()) return;
    outRecipe.areas.clear();
    for (const auto& [key, value] : document["areas"].items()) {
        if (!value.is_object()) continue;   // degrade gracefully: skip the one bad entry, not the domain
        Params::MapArea area;
        area.name = key;
        ReadJsonFloat(value, "x", area.originX);
        ReadJsonFloat(value, "y", area.originZ);
        ReadJsonFloat(value, "width", area.width);
        ReadJsonFloat(value, "height", area.length);
        outRecipe.areas.push_back(area);
    }
}

} // namespace Io
} // namespace SanmapGen
