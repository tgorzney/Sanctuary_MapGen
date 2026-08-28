// MapImporter_DecalLayerBundle_IO.cpp — the top-level `DecalLayerBundles` array ->
// `recipe.decalLayerBundles` (ARCH §20). Own file, mirrors MapImporter_PropLayerBundle_IO.cpp
// exactly minus the type-tag field (Decals has exactly one Type Section).
//
// No range/clamp validation on `ParentBundleIdentifier`/`AssemblyIdentifier` — a dangling reference
// is a query-time miss, not a structural error. The one thing THIS file does validate: a cyclic
// `ParentBundleIdentifier` chain, logged and treated as root, never a refusal.
#include "JsonPrimitives_IO.h"
#include "MapImporter_IO.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../params/ScatterLayerBundle_PARAMS.h"

namespace SanmapGen {
namespace Io {
namespace {

void PopulateDecalLayerBundlesFromJson(const nlohmann::json& document, Params::MapRecipe& outRecipe) {
    if (!document.contains("DecalLayerBundles") || !document["DecalLayerBundles"].is_array()) return;
    outRecipe.decalLayerBundles.clear();
    for (const nlohmann::json& bundleJson : document["DecalLayerBundles"]) {
        Params::DecalLayerBundle bundle;
        if (bundleJson.is_object()) {
            ReadJsonInteger(bundleJson, "Identifier", bundle.identifier);
            ReadJsonText(bundleJson, "Name", bundle.name);
            ReadJsonInteger(bundleJson, "ParentBundleIdentifier", bundle.parentBundleIdentifier);
            ReadJsonInteger(bundleJson, "AssemblyIdentifier", bundle.assemblyIdentifier);
        }
        outRecipe.decalLayerBundles.push_back(bundle);
    }
}

// Mirrors RepairCyclicPropLayerBundleParents exactly, including the immutable-snapshot rationale.
void RepairCyclicDecalLayerBundleParents(std::vector<Params::DecalLayerBundle>& bundles,
                                         MapImportResult& result) {
    const std::vector<Params::DecalLayerBundle> originalBundles = bundles;
    for (Params::DecalLayerBundle& bundle : bundles) {
        if (bundle.parentBundleIdentifier == -1) continue;
        if (!Params::WouldReparentDecalLayerBundleCreateCycle(bundle.identifier,
                                                   bundle.parentBundleIdentifier, originalBundles))
            continue;
        result.Warn("DecalLayerBundle \"" + bundle.name + "\" (Identifier "
                   + std::to_string(bundle.identifier)
                   + ") has a cyclic ParentBundleIdentifier chain; treated as root.");
        bundle.parentBundleIdentifier = -1;
    }
}

} // namespace

void ReadDecalLayerBundlesJson(const nlohmann::json& document, Params::MapRecipe& outRecipe,
                               MapImportResult& result) {
    PopulateDecalLayerBundlesFromJson(document, outRecipe);
    RepairCyclicDecalLayerBundleParents(outRecipe.decalLayerBundles, result);
}

} // namespace Io
} // namespace SanmapGen
