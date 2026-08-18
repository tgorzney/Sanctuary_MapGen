// MapExporter_Chains_IO.cpp — `recipe.chains` -> the top-level `.sanmap` `chains` dictionary.
// Layer: IO. Own file (not shared with Markers): independent top-level format key, same split
// ruling applied from STEP2_ArmiesAreas_IO (STEP3_MarkersChains_IO).
// `chains` is a THIRD, different container shape from `markers`/`armies`/`areas` — a name-keyed
// object of bare JSON ARRAYS, not an object of objects (finding 3, confirmed `SanMap.cs:150`
// `Dictionary<string, MarkerChain.Marker[]> chains`). `Params::MarkerChain::markers` is a
// convenience C++ member name; it does NOT appear as a JSON key — the array IS the whole value.
// `ChainMarker` has no position/rotation field at all, so no coordinate flip applies here.
#include "MapExporter_Recipe_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {

nlohmann::ordered_json BuildChainsJson(const Params::MapRecipe& recipe) {
    nlohmann::ordered_json chains = nlohmann::ordered_json::object();
    for (const Params::MarkerChain& chain : recipe.chains) {
        nlohmann::ordered_json markersArray = nlohmann::ordered_json::array();
        for (const Params::ChainMarker& marker : chain.markers)
            markersArray.push_back({ { "type", marker.type }, { "name", marker.name } });
        chains[chain.name] = markersArray;
    }
    return chains;
}

} // namespace Io
} // namespace SanmapGen
