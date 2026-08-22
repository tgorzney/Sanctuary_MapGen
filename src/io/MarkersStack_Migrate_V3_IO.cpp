// MarkersStack_Migrate_V3_IO.cpp — see the header for the full contract. STEP67: eliminates the
// lossy-collapse risk by construction, not by picking a winner. Walks the old flat array left to
// right; a maximal contiguous run of rules sharing an identical EFFECTIVE
// (SymmetryUseGlobal, SymmetryMask, RadialSymmetryRepeatCount) triplet becomes exactly one
// `MarkerRuleLayer`. Non-adjacent same-triplet groups are never merged (out of scope — would
// reorder flattened rule-execution order, needs a Generator Expert order-independence ruling).
#include "MarkersStack_Migrate_V3_IO.h"
#include "JsonPrimitives_IO.h"
#include "../params/Symmetry_PARAMS.h"
#include <string>
#include <utility>
#include <vector>

namespace SanmapGen {
namespace Io {
namespace {

// Reads the rule's EFFECTIVE triplet: a scratch `SymmetrySetting` seeded with the same struct
// defaults `MarkerRule` carried before STEP66 promoted these fields onto the layer
// (bSymmetryUseGlobal = true, symmetryMask = 0, radialSymmetryRepeatCount = 3), then overwritten
// by whatever the rule's own JSON actually carries — a missing key therefore compares equal to a
// neighbor that explicitly writes that field's default, and an out-of-range value is clamped the
// same way the live importer already clamps it.
Params::SymmetrySetting ReadEffectiveSymmetryTriplet(const nlohmann::json& rule) {
    Params::SymmetrySetting triplet;
    ReadJsonBoolean(rule, "SymmetryUseGlobal", triplet.bSymmetryUseGlobal);
    ReadJsonInteger(rule, "SymmetryMask", triplet.symmetryMask);
    ReadJsonIntegerClamped(rule, "RadialSymmetryRepeatCount", Params::radialSymmetryRepeatCountMinimum,
                          Params::radialSymmetryRepeatCountMaximum, triplet.radialSymmetryRepeatCount);
    return triplet;
}

bool TripletsMatch(const Params::SymmetrySetting& a, const Params::SymmetrySetting& b) {
    return a.bSymmetryUseGlobal == b.bSymmetryUseGlobal && a.symmetryMask == b.symmetryMask
        && a.radialSymmetryRepeatCount == b.radialSymmetryRepeatCount;
}

// Builds one `MarkerRuleLayer` JSON object from a contiguous run of old-shape rule objects sharing
// `triplet`. `Enabled`/`Hidden` are left unset — the rewritten importer's own struct defaults
// govern on read (same precedent DetailNormal_Migrate_V2/Accumulation_Migrate_V2 set for
// genuinely-new fields with no legacy source). `ruleGroup` is consumed (moved from).
nlohmann::json BuildMigratedLayerJson(int layerIndex, const Params::SymmetrySetting& triplet,
                                      std::vector<nlohmann::json>& ruleGroup) {
    nlohmann::json layer = nlohmann::json::object();
    layer["Name"] = "Migrated Layer " + std::to_string(layerIndex);
    layer["SymmetryUseGlobal"]         = triplet.bSymmetryUseGlobal;
    layer["SymmetryMask"]              = triplet.symmetryMask;
    layer["RadialSymmetryRepeatCount"] = triplet.radialSymmetryRepeatCount;

    nlohmann::json rules = nlohmann::json::array();
    for (nlohmann::json& rule : ruleGroup) {
        DeleteKeyIfPresent(rule, "SymmetryUseGlobal");
        DeleteKeyIfPresent(rule, "SymmetryMask");
        DeleteKeyIfPresent(rule, "RadialSymmetryRepeatCount");
        rules.push_back(std::move(rule));
    }
    layer["Rules"] = std::move(rules);
    return layer;
}

} // namespace

void MarkersStack_Migrate_V3(nlohmann::json& document) {
    if (!document.contains("MarkersStack") || !document["MarkersStack"].is_array()) return;
    nlohmann::json& oldArray = document["MarkersStack"];
    if (oldArray.empty()) return;

    // Idempotency: an already-V4-shaped array's elements are `MarkerRuleLayer` objects, each
    // carrying a `Rules` array — a flat V3 `MarkerRule` element never does. A second call is a
    // safe no-op, detected from the first element's shape alone.
    if (oldArray[0].is_object() && oldArray[0].contains("Rules")) return;

    nlohmann::json migratedArray = nlohmann::json::array();
    std::vector<nlohmann::json> currentGroup;
    Params::SymmetrySetting     currentTriplet;
    int                         layerCount = 0;

    for (std::size_t index = 0; index < oldArray.size(); ++index) {
        Params::SymmetrySetting triplet = ReadEffectiveSymmetryTriplet(oldArray[index]);
        if (index == 0 || !TripletsMatch(triplet, currentTriplet)) {
            if (!currentGroup.empty()) {
                ++layerCount;
                migratedArray.push_back(BuildMigratedLayerJson(layerCount, currentTriplet, currentGroup));
                currentGroup.clear();
            }
            currentTriplet = triplet;
        }
        currentGroup.push_back(std::move(oldArray[index]));
    }
    if (!currentGroup.empty()) {
        ++layerCount;
        migratedArray.push_back(BuildMigratedLayerJson(layerCount, currentTriplet, currentGroup));
    }

    oldArray = std::move(migratedArray);
}

} // namespace Io
} // namespace SanmapGen
