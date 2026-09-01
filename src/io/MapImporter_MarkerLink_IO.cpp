// MapImporter_MarkerLink_IO.cpp — see the header for the full contract.
#include "MapImporter_MarkerLink_IO.h"
#include "JsonPrimitives_IO.h"
#include "MapImporter_IO.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../params/Symmetry_PARAMS.h"   // radialSymmetryRepeatCountMinimum/Maximum

namespace SanmapGen {
namespace Io {
namespace {

// STEP243: the 7 fields STEP241/242 added to `Params::MarkerLink` read back with the same
// wire-key spelling/shape as `MarkerGroups[]`'s identical fields (ReadMarkerGroupsJson in
// MapImporter_MarkerGroups_IO.cpp). Absent-on-import (a pre-STEP243 `.sanmap`) leaves each field
// at the struct's own default — no special-casing needed, `ReadJson*` already no-ops on a missing
// key.
void PopulateMarkerLinksFromJson(const nlohmann::json& document, Params::MapRecipe& outRecipe) {
    if (!document.contains("MarkerLinks") || !document["MarkerLinks"].is_array()) return;
    outRecipe.markerLinks.clear();
    for (const nlohmann::json& linkJson : document["MarkerLinks"]) {
        Params::MarkerLink link;
        if (linkJson.is_object()) {
            ReadJsonInteger(linkJson, "Identifier", link.identifier);
            ReadJsonText(linkJson, "Name", link.name);
            ReadJsonBoolean(linkJson, "ColorOverrideEnabled", link.bColorOverrideEnabled);
            if (linkJson.contains("Color") && linkJson["Color"].is_object()) {
                const nlohmann::json& color = linkJson["Color"];
                ReadJsonFloat(color, "r", link.color[0]);
                ReadJsonFloat(color, "g", link.color[1]);
                ReadJsonFloat(color, "b", link.color[2]);
                ReadJsonFloat(color, "a", link.color[3]);
            }
            ReadJsonBoolean(linkJson, "Hidden", link.bHidden);
            ReadJsonFloat(linkJson, "IconScale", link.iconScale);
            ReadJsonBoolean(linkJson, "GridSnapEnabled", link.bGridSnapEnabled);
            ReadJsonFloat(linkJson, "GridSnapSizeWorldUnits", link.gridSnapSizeWorldUnits);
            ReadJsonBoolean(linkJson, "SymmetryEnabled", link.bSymmetryEnabled);
            ReadJsonBoolean(linkJson, "SymmetryUseGlobal", link.symmetry.bSymmetryUseGlobal);
            ReadJsonInteger(linkJson, "SymmetryMask", link.symmetry.symmetryMask);
            ReadJsonIntegerClamped(linkJson, "RadialSymmetryRepeatCount",
                                  Params::radialSymmetryRepeatCountMinimum,
                                  Params::radialSymmetryRepeatCountMaximum,
                                  link.symmetry.radialSymmetryRepeatCount);
            ReadJsonBoolean(linkJson, "Locked", link.bLocked);
        }
        outRecipe.markerLinks.push_back(link);
    }
}

bool MarkerLinkExists(const std::vector<Params::MarkerLink>& links, int linkIdentifier) {
    for (const Params::MarkerLink& link : links) {
        if (link.identifier == linkIdentifier) return true;
    }
    return false;
}

} // namespace

void ReadMarkerLinksJson(const nlohmann::json& document, Params::MapRecipe& outRecipe,
                         MapImportResult& result) {
    (void)result;   // STEP245/ARCH §19.33: pure population only now — the warn pass moved to an
                    // explicit call site in MapImporter_ParseDocument_IO.cpp, run AFTER
                    // ReadMarkersJson (see this function's own header comment for why).
    PopulateMarkerLinksFromJson(document, outRecipe);
}

// ARCH §19.29/§19.30/§19.33: a dangling `linkIdentifier` is a soft, logged degrade, never a repair —
// unlike `RepairCyclicMarkerLayerBundleParents`, this pass never rewrites the field. The caller
// (MapImporter_ParseDocument_IO.cpp) must run this AFTER `ReadMarkersJson`: the third loop below
// needs `recipe.markers` already populated.
void WarnDanglingMarkerLinkIdentifiers(const Params::MapRecipe& recipe, MapImportResult& result) {
    for (const Params::MarkerLayerBundle& bundle : recipe.markerLayerBundles) {
        if (bundle.linkIdentifier == -1) continue;
        if (MarkerLinkExists(recipe.markerLinks, bundle.linkIdentifier)) continue;
        result.Warn("MarkerLayerBundle \"" + bundle.name + "\" (Identifier "
                   + std::to_string(bundle.identifier) + ") has a dangling LinkIdentifier ("
                   + std::to_string(bundle.linkIdentifier) + "); resolves as not Link-bound.");
    }
    for (const Params::MarkerInstanceLayer& layer : recipe.markerLayers) {
        if (layer.linkIdentifier == -1) continue;
        if (MarkerLinkExists(recipe.markerLinks, layer.linkIdentifier)) continue;
        result.Warn("MarkerInstanceLayer \"" + layer.name + "\" has a dangling LinkIdentifier ("
                   + std::to_string(layer.linkIdentifier) + "); resolves as not Link-bound.");
    }
    // ARCH §19.33 — third, instance tier: MarkerTransform::linkIdentifier, identical shape/soft-warn
    // posture as the two loops above.
    for (const Params::MarkerInstanceGroup& group : recipe.markers) {
        for (const Params::MarkerTransform& transform : group.transforms) {
            if (transform.linkIdentifier == -1) continue;
            if (MarkerLinkExists(recipe.markerLinks, transform.linkIdentifier)) continue;
            result.Warn("MarkerTransform \"" + transform.name + "\" (type \"" + group.name
                       + "\") has a dangling LinkIdentifier (" + std::to_string(transform.linkIdentifier)
                       + "); resolves as not Link-bound.");
        }
    }
}

} // namespace Io
} // namespace SanmapGen
