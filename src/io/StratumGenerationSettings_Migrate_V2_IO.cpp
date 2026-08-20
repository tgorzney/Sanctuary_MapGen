// StratumGenerationSettings_Migrate_V2_IO.cpp — see the header for the full contract.
//
// Additive-write discipline (STEP40D, the ticket's actual point): this migration NEVER does
// `document["StratumGenerationSettings"] = newArray` — it grows the existing array in place and
// sets only its own 8 slope-gate keys per entry, so the sibling `SlopeDefaults_Migrate_V2`'s
// `SlopeUseGlobal` write (which runs FIRST, same step) survives untouched. Padding indices beyond
// `Stratums.size()` (which the sibling never touches) get `SlopeUseGlobal = true` via
// `DefaultIfMissing` — never a raw overwrite — so all 9 final entries end up with the key without
// ever clobbering a real computed value.
#include "StratumGenerationSettings_Migrate_V2_IO.h"
#include "JsonPrimitives_IO.h"

namespace SanmapGen {
namespace Io {
namespace {

constexpr int kEntryCount = 9; // sanmapStratumCount (MapExporter_IO.h) — the format's fixed count.

// The 8 slope-gate keys, carried over VERBATIM (same key name both sides — SANMAP_FORMAT_SPEC
// Correction 12). Soil physics (this array's other 6 fields) has no legacy source and is untouched.
constexpr const char* kSlopeGateKeys[8] = {
    "SlopeGateEnabled", "MinimumSlopeDegrees", "MaximumSlopeDegrees", "SlopeFeatherDegreesLow",
    "SlopeFeatherDegreesHigh", "UseSmoothstep", "InvertSlopeGate", "SlopeGateStrength",
};

// Copies `key` from `sourceStratum` onto `destinationEntry` verbatim IF present on the source —
// never `MoveKey`/`RenameKey` (both erase the source; this migration's shared source is read-only
// here, per this ticket's shared-source discipline — the manifest's `legacyKeysToDelete` handles
// deletion once every migration in the step has run).
void CopyKeyIfPresent(const nlohmann::json& sourceStratum, nlohmann::json& destinationEntry, const char* key) {
    if (!sourceStratum.contains(key)) return;
    destinationEntry[key] = sourceStratum[key];
}

void RelocateSlopeGateFields(const nlohmann::json& sourceStratum, nlohmann::json& destinationEntry) {
    for (const char* key : kSlopeGateKeys) CopyKeyIfPresent(sourceStratum, destinationEntry, key);
}

} // namespace

void StratumGenerationSettings_Migrate_V2(nlohmann::json& document) {
    // N = 0 short-circuit, mirroring the sibling `SlopeDefaults_Migrate_V2`'s exact pattern
    // (STEP41_PostMigrationImportGaps_IO): no `mapGeneratorData`, no `Stratums` array, or an empty
    // one means no write to `StratumGenerationSettings` at all — not even the padding array. Without
    // this, every document walking the V2->V3 step got a spurious 9-entry array with no source data
    // behind it, which then tripped `ReadStratumGenerationSettingsJson`'s cardinality check.
    if (!document.contains("mapGeneratorData") || !document["mapGeneratorData"].is_object()) return;
    const nlohmann::json& generatorData = document["mapGeneratorData"];
    if (!generatorData.contains("Stratums") || !generatorData["Stratums"].is_array()) return;
    const nlohmann::json stratums = generatorData["Stratums"]; // copy: document mutates below.
    if (stratums.empty()) return; // N = 0: no write to StratumGenerationSettings at all.

    // Additive: never replace the array wholesale, only grow it to exactly kEntryCount entries.
    nlohmann::json& settings = document["StratumGenerationSettings"];
    if (!settings.is_array()) settings = nlohmann::json::array();
    while (settings.size() < static_cast<std::size_t>(kEntryCount)) settings.push_back(nlohmann::json::object());

    const std::size_t relocateCount = (stratums.size() < static_cast<std::size_t>(kEntryCount))
                                     ? stratums.size() : static_cast<std::size_t>(kEntryCount);
    for (std::size_t index = 0; index < relocateCount; ++index) {
        if (!settings[index].is_object()) settings[index] = nlohmann::json::object();
        if (stratums[index].is_object()) RelocateSlopeGateFields(stratums[index], settings[index]);
    }

    for (int index = 0; index < kEntryCount; ++index) {
        if (!settings[index].is_object()) settings[index] = nlohmann::json::object();
        DefaultIfMissing(settings[index], "SlopeUseGlobal", true);
    }
}

} // namespace Io
} // namespace SanmapGen
