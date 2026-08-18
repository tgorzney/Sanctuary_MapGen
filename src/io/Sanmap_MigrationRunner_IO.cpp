// Sanmap_MigrationRunner_IO.cpp — see the header for the full contract.
#include "Sanmap_MigrationRunner_IO.h"
#include "Sanmap_MigrationManifest_IO.h"
#include "JsonPrimitives_IO.h"
#include "MapImporter_IO.h"
#include <string>

namespace SanmapGen {
namespace Io {
namespace {

constexpr const char* kSanGenVersionKey        = "SanGenVersion";
constexpr const char* kMapGeneratorDataKey      = "mapGeneratorData";
constexpr const char* kLegacyVersionKey         = "MapGeneratorDataVersion";

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

} // namespace

bool RunSanmapMigrations(nlohmann::json& document, MapImportResult& result) {
    int resolvedVersion = 0;
    if (!ResolveSanGenVersion(document, resolvedVersion, result)) {
        result.Log("Refused: the document has no version marker at all (neither a top-level "
                   "SanGenVersion field nor the legacy mapGeneratorData.MapGeneratorDataVersion "
                   "field) — this build never guesses a starting version.");
        return false;
    }
    if (resolvedVersion > kCurrentSanGenVersion) {
        result.Log("Refused: this map was saved by a newer SanGen (SanGenVersion "
                   + std::to_string(resolvedVersion) + ") than this build supports (current "
                   + std::to_string(kCurrentSanGenVersion) + ") and cannot be opened.");
        return false;
    }

    // §4.2: walk forward generically. Zero-iterates whenever resolvedVersion == kCurrentSanGenVersion
    // (the ONLY case this ticket's empty manifest can ever legally reach) — §4.3's pass-through.
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
    return true;
}

} // namespace Io
} // namespace SanmapGen
