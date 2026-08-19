// Sanmap_MigrationRunner_IO.cpp — see the header for the full contract.
#include "Sanmap_MigrationRunner_IO.h"
#include "Sanmap_MigrationManifest_IO.h"
#include "JsonPrimitives_IO.h"
#include "MapImporter_IO.h"
#include "UnknownImportBag_IO.h"
#include <string>

namespace SanmapGen {
namespace Io {
namespace {

constexpr const char* kSanGenVersionKey   = "SanGenVersion";
constexpr const char* kMapGeneratorDataKey = "mapGeneratorData";
constexpr const char* kLegacyVersionKey    = "MapGeneratorDataVersion";

// §4.1: `SanGenVersion` is read FIRST; the legacy nested field is only consulted when that key is
// absent. Returns false — outResolvedVersion untouched — when NEITHER marker is present.
bool ResolveSanGenVersion(const nlohmann::json& document, int& outResolvedVersion,
                          MapImportResult& result) {
    if (ReadJsonInteger(document, kSanGenVersionKey, outResolvedVersion)) return true;

    if (document.contains(kMapGeneratorDataKey) && document[kMapGeneratorDataKey].is_object()) {
        const nlohmann::json& generatorData = document[kMapGeneratorDataKey];
        if (ReadJsonInteger(generatorData, kLegacyVersionKey, outResolvedVersion)) {
            result.Warn("No top-level SanGenVersion field; fell back to the legacy "
                       "mapGeneratorData.MapGeneratorDataVersion field.");
            return true;
        }
    }
    return false;
}

// Ruling 4's population step. Called strictly AFTER the forward-walk below, so any legacy key a
// migration step deliberately deleted via `DeleteKeyIfPresent` (§3's `legacyKeysToDelete`) is
// already physically gone from `document` — ordering alone provides the "deliberately deleted" vs.
// "genuinely unknown" distinction, no extra bookkeeping needed.
//
// STEP28_UnknownImportNesting_IO: seed the bag FIRST from any incoming `UnknownImport` object's own
// children, flattened one level (NOT re-wrapped under a nested `UnknownImport` key) — a document
// that was itself exported with a nested `UnknownImport` bag must round-trip without accumulating
// nesting on every load/save cycle. The per-key loop below then continues to capture any OTHER
// genuinely-unrecognized top-level keys into the same bag, unaffected.
void CaptureUnknownTopLevelKeys(const nlohmann::json& document, UnknownImportBag& outUnknownData) {
    if (document.contains("UnknownImport") && document["UnknownImport"].is_object())
        for (const auto& [key, value] : document["UnknownImport"].items())
            outUnknownData.unknownTopLevelKeys[key] = value;

    for (const auto& [key, value] : document.items()) {
        if (IsKnownTopLevelSanmapKey(key)) continue;
        outUnknownData.unknownTopLevelKeys[key] = value;
    }
}

} // namespace

void RunSanmapMigrations(nlohmann::json& document, MapImportResult& result,
                         UnknownImportBag* outUnknownData) {
    int resolvedVersion = kCurrentSanGenVersion;
    if (!ResolveSanGenVersion(document, resolvedVersion, result)) {
        // No version marker of any kind (§6): never a blind version-1 walk — resolve to a
        // zero-iteration state so the loop below naturally does nothing and control falls straight
        // through to the readers, current-shape keys only (plus the existing legacy
        // mapGeneratorData-gated readers, plus this function's own Unknown-Import capture below).
        result.Warn("No SanGenVersion or legacy version field found; skipping migration, "
                    "recovering via direct field match only.");
        resolvedVersion = kCurrentSanGenVersion;
    } else if (resolvedVersion > kCurrentSanGenVersion) {
        result.Warn("This map was saved by a newer SanGen (SanGenVersion "
                   + std::to_string(resolvedVersion) + "); recovering best-effort — fields this "
                   "build does not recognize are preserved, not applied.");
    }

    // §4.2: walk forward generically. Zero-iterates for both cases above (resolvedVersion ==
    // kCurrentSanGenVersion, or resolvedVersion > kCurrentSanGenVersion) with no extra branch.
    const std::vector<MigrationStep>& manifest = SanmapMigrationManifest();
    for (int sourceVersion = resolvedVersion; sourceVersion < kCurrentSanGenVersion; ++sourceVersion) {
        const MigrationStep* step = nullptr;
        for (const MigrationStep& candidate : manifest) {
            if (candidate.sourceVersion == sourceVersion) { step = &candidate; break; }
        }
        if (step != nullptr) {
            // Ordering is load-bearing law (§2 rule 2) — run exactly the manifest's declared order.
            for (MigrationFunction migration : step->migrations) migration(document);
            for (const char* legacyKey : step->legacyKeysToDelete) DeleteKeyIfPresent(document, legacyKey);
        }
        // Writing the version field is the RUNNER's job, never an individual migration's (§4.2) —
        // every step leaves the document honestly stamped, whether or not further steps follow.
        document[kSanGenVersionKey] = sourceVersion + 1;
    }

    // Ruling 4: Unknown-Import capture, strictly after the walk (and any deletions it ran) above.
    if (outUnknownData != nullptr) CaptureUnknownTopLevelKeys(document, *outUnknownData);
}

} // namespace Io
} // namespace SanmapGen
