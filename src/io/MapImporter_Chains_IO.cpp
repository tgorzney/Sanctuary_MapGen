// MapImporter_Chains_IO.cpp — the top-level `.sanmap` `chains` dictionary -> `recipe.chains`.
// Layer: IO. The exact inverse of MapExporter_Chains_IO.cpp. `chains` is a name-keyed object of
// bare JSON ARRAYS (finding 3), not an object of objects — a plain array walk, no name-keyed-object
// helper needed (and no coordinate flip: `ChainMarker` has no position field at all).
#include "JsonPrimitives_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {

void ReadChainsJson(const nlohmann::json& document, Params::MapRecipe& outRecipe) {
    if (!document.contains("chains") || !document["chains"].is_object()) return;
    outRecipe.chains.clear();
    for (const auto& [key, arrayJson] : document["chains"].items()) {
        if (!arrayJson.is_array()) continue;   // degrade gracefully: skip the one bad entry
        Params::MarkerChain chain;
        chain.name = key;
        for (const nlohmann::json& markerJson : arrayJson) {
            if (!markerJson.is_object()) continue;
            Params::ChainMarker marker;
            ReadJsonText(markerJson, "type", marker.type);
            ReadJsonText(markerJson, "name", marker.name);
            chain.markers.push_back(marker);
        }
        outRecipe.chains.push_back(chain);
    }
}

} // namespace Io
} // namespace SanmapGen
