// MapImporter_MarkerLink_IO.h — the top-level `MarkerLinks` array -> `recipe.markerLinks`
// (ARCH §19.28/§19.30, DESIGN_MarkerLink_R1.md §3.3/§3.8). Layer: IO. New file pair, own header —
// mirrors MapExporter_MarkerLink_IO.h's own "brand-new tier gets its own IO file" reasoning.
//
// No range/clamp validation on `Identifier` (ARCH §19.29/§19.30 — a dangling `LinkIdentifier` on
// either `MarkerLayerBundles[i]`/`MarkerGroups[i]` is a soft, LOGGED degrade to "not Link-bound",
// never a hard refusal or a value rewrite, same posture as `ParentBundleIdentifier`/
// `AssemblyIdentifier`'s own dangling-reference rule). `WarnDanglingMarkerLinkIdentifiers` is that
// logging pass — it never mutates `linkIdentifier`, only reports.
#pragma once
#include <nlohmann/json.hpp>

namespace SanmapGen {
namespace Params { struct MapRecipe; }
namespace Io {

struct MapImportResult;

// Populates `outRecipe.markerLinks` from the top-level `MarkerLinks` array — pure population only
// (ARCH §19.33/STEP245: this used to also run `WarnDanglingMarkerLinkIdentifiers` internally, but
// that call moved OUT to `MapImporter_ParseDocument_IO.cpp`'s `ParseEntityDomainsJson`, run
// explicitly AFTER `ReadMarkersJson`, since the warn pass's third, transform-tier loop needs
// `outRecipe.markers` already populated — this reader's own call site runs BEFORE `ReadMarkersJson`
// and cannot see it). Call this AFTER `ReadMarkerLayerBundlesJson`/`ReadMarkerGroupsJson` (no
// ordering dependency the other direction: neither of those two readers looks at
// `outRecipe.markerLinks`).
void ReadMarkerLinksJson(const nlohmann::json& document, Params::MapRecipe& outRecipe,
                         MapImportResult& result);

// Logs (via `result.Warn`) every `MarkerLayerBundle`/`MarkerInstanceLayer`/`MarkerTransform`
// (ARCH §19.33 adds the third, instance tier) whose `linkIdentifier` is neither `-1` nor a match
// against any entry in `outRecipe.markerLinks` — soft, informational, never a repair (the dangling
// value is left exactly as read; a resolver that fails to find the id already treats it as "not
// Link-bound", identical in effect to `-1`). Call this explicitly, once, AFTER `ReadMarkersJson`
// (`outRecipe.markers` must already be populated for the third loop to see anything) — see
// `MapImporter_ParseDocument_IO.cpp`'s `ParseEntityDomainsJson` for the call site.
void WarnDanglingMarkerLinkIdentifiers(const Params::MapRecipe& recipe, MapImportResult& result);

} // namespace Io
} // namespace SanmapGen
