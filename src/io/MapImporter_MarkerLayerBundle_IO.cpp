// MapImporter_MarkerLayerBundle_IO.cpp — the top-level `MarkerLayerBundles` array ->
// `recipe.markerLayerBundles` (ARCH §19, Correction 19). Own file, not folded into
// MapImporter_Markers_IO.cpp (already at the ARCH §1.5 line-count ceiling) — mirrors the
// MapImporter_ParseDocument_IO.cpp-out-of-MapImporter_IO.cpp (STEP35) / MapImporter_
// MarkerLayerReconcile_IO.cpp-out-of-MapImporter_Markers_IO.cpp (STEP115) split precedent.
//
// No range/clamp validation on `ParentBundleIdentifier`/`AssemblyIdentifier` (ARCH §19.4 — a
// dangling reference is a query-time miss, not a structural error, same posture as
// symmetryGroupIdentifier). The one thing THIS file does validate: a cyclic ParentBundleIdentifier
// chain, logged and treated as root, never a refusal (ARCH §19.12, Assembly's own already-decided
// convention restated at the Bundle tier).
#include "JsonPrimitives_IO.h"
#include "MapImporter_IO.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../params/MarkerLayerBundle_PARAMS.h"

namespace SanmapGen {
namespace Io {
namespace {

void PopulateMarkerLayerBundlesFromJson(const nlohmann::json& document, Params::MapRecipe& outRecipe) {
    if (!document.contains("MarkerLayerBundles") || !document["MarkerLayerBundles"].is_array()) return;
    outRecipe.markerLayerBundles.clear();
    for (const nlohmann::json& bundleJson : document["MarkerLayerBundles"]) {
        Params::MarkerLayerBundle bundle;
        if (bundleJson.is_object()) {
            ReadJsonInteger(bundleJson, "Identifier", bundle.identifier);
            ReadJsonText(bundleJson, "Name", bundle.name);
            ReadJsonInteger(bundleJson, "ParentBundleIdentifier", bundle.parentBundleIdentifier);
            ReadJsonText(bundleJson, "MarkerTypeName", bundle.markerTypeName);
            ReadJsonInteger(bundleJson, "AssemblyIdentifier", bundle.assemblyIdentifier);
        }
        outRecipe.markerLayerBundles.push_back(bundle);
    }
}

// ARCH §19.12: a cyclic ParentBundleIdentifier chain is logged and treated as root, never a
// refusal — same convention as Assembly's own already-ratified cycle-on-import rule, applied here
// at the Bundle tier. Runs AFTER the whole table is populated: WouldReparentMarkerLayerBundle
// CreateCycle needs every entry present to walk the chain.
//
// Evaluates every entry's cycle predicate against an immutable SNAPSHOT taken before this pass
// starts, not the live `bundles` vector being repaired in place — deliberate deviation from
// checking against the mutating table itself. Checking against the live table is order-dependent
// and under-detects: repairing one member of an N-node cycle severs the chain for every OTHER
// member still to be visited, so a plain N=2 mutual cycle ({1,2},{2,1}) would flag only the first
// entry visited, not both. The snapshot makes every entry's cyclic status independent of visit
// order, which is what a "both entries are cyclic, both get warned and repaired" outcome requires.
void RepairCyclicMarkerLayerBundleParents(std::vector<Params::MarkerLayerBundle>& bundles,
                                          MapImportResult& result) {
    const std::vector<Params::MarkerLayerBundle> originalBundles = bundles;
    for (Params::MarkerLayerBundle& bundle : bundles) {
        if (bundle.parentBundleIdentifier == -1) continue;
        if (!Params::WouldReparentMarkerLayerBundleCreateCycle(bundle.identifier,
                                                    bundle.parentBundleIdentifier, originalBundles))
            continue;
        result.Warn("MarkerLayerBundle \"" + bundle.name + "\" (Identifier "
                   + std::to_string(bundle.identifier)
                   + ") has a cyclic ParentBundleIdentifier chain; treated as root.");
        bundle.parentBundleIdentifier = -1;
    }
}

} // namespace

void ReadMarkerLayerBundlesJson(const nlohmann::json& document, Params::MapRecipe& outRecipe,
                                MapImportResult& result) {
    PopulateMarkerLayerBundlesFromJson(document, outRecipe);
    RepairCyclicMarkerLayerBundleParents(outRecipe.markerLayerBundles, result);
}

} // namespace Io
} // namespace SanmapGen
