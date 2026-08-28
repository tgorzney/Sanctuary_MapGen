// MapImporter_PropLayerBundle_IO.cpp — the top-level `PropLayerBundles` array ->
// `recipe.propLayerBundles` (ARCH §20). Own file, mirrors MapImporter_MarkerLayerBundle_IO.cpp
// exactly (own-file-per-Bundle-domain split precedent).
//
// No range/clamp validation on `ParentBundleIdentifier`/`AssemblyIdentifier` (ARCH §19.4, restated
// at §20 for Props) — a dangling reference is a query-time miss, not a structural error. The one
// thing THIS file does validate: a cyclic `ParentBundleIdentifier` chain, logged and treated as
// root, never a refusal.
#include "JsonPrimitives_IO.h"
#include "MapImporter_IO.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../params/ScatterLayerBundle_PARAMS.h"

namespace SanmapGen {
namespace Io {
namespace {

void PopulatePropLayerBundlesFromJson(const nlohmann::json& document, Params::MapRecipe& outRecipe) {
    if (!document.contains("PropLayerBundles") || !document["PropLayerBundles"].is_array()) return;
    outRecipe.propLayerBundles.clear();
    for (const nlohmann::json& bundleJson : document["PropLayerBundles"]) {
        Params::PropLayerBundle bundle;
        if (bundleJson.is_object()) {
            ReadJsonInteger(bundleJson, "Identifier", bundle.identifier);
            ReadJsonText(bundleJson, "Name", bundle.name);
            ReadJsonInteger(bundleJson, "ParentBundleIdentifier", bundle.parentBundleIdentifier);
            ReadJsonText(bundleJson, "PropTypeName", bundle.propTypeName);
            ReadJsonInteger(bundleJson, "AssemblyIdentifier", bundle.assemblyIdentifier);
        }
        outRecipe.propLayerBundles.push_back(bundle);
    }
}

// Mirrors RepairCyclicMarkerLayerBundleParents exactly, including the immutable-snapshot rationale
// (MapImporter_MarkerLayerBundle_IO.cpp's own comment) — evaluated against a snapshot taken before
// this pass starts so an N-node mutual cycle flags every member, not just the first one visited.
void RepairCyclicPropLayerBundleParents(std::vector<Params::PropLayerBundle>& bundles,
                                        MapImportResult& result) {
    const std::vector<Params::PropLayerBundle> originalBundles = bundles;
    for (Params::PropLayerBundle& bundle : bundles) {
        if (bundle.parentBundleIdentifier == -1) continue;
        if (!Params::WouldReparentPropLayerBundleCreateCycle(bundle.identifier,
                                                  bundle.parentBundleIdentifier, originalBundles))
            continue;
        result.Warn("PropLayerBundle \"" + bundle.name + "\" (Identifier "
                   + std::to_string(bundle.identifier)
                   + ") has a cyclic ParentBundleIdentifier chain; treated as root.");
        bundle.parentBundleIdentifier = -1;
    }
}

} // namespace

void ReadPropLayerBundlesJson(const nlohmann::json& document, Params::MapRecipe& outRecipe,
                              MapImportResult& result) {
    PopulatePropLayerBundlesFromJson(document, outRecipe);
    RepairCyclicPropLayerBundleParents(outRecipe.propLayerBundles, result);
}

} // namespace Io
} // namespace SanmapGen
