// SlopeDefaults_Migrate_V2_IO.cpp — see the header for the full contract. Synthesis rule (ruled by
// STEP40D, not left to the coder to invent): the 3 booleans (`SlopeGateEnabled`/`UseSmoothstep`/
// `InvertSlopeGate`) synthesize by MODE across every `Stratums[i]` entry that HAS the key, tie ->
// false (matches each field's own `SlopeDefaults_PARAMS.h` hardcoded default — also correctly
// covers "zero entries have the key", a 0-0 tie). The 5 floats synthesize by arithmetic MEAN across
// every entry that has the key, falling back to that field's own PARAMS default only in the
// (unspecified by the ruling) case where no entry has it at all. N = 0 (no `Stratums`, or empty):
// no write to `SlopeDefaults` at all.
#include "SlopeDefaults_Migrate_V2_IO.h"
#include "JsonPrimitives_IO.h"

namespace SanmapGen {
namespace Io {
namespace {

// Copied verbatim from `SlopeDefaults_PARAMS.h`'s hardcoded defaults — stays pure JSON here (no
// PARAMS include), matching every sibling `<Domain>_Migrate_V2_IO.cpp` in this step.
constexpr float kDefaultMinimumSlopeDegrees     = 0.0f;
constexpr float kDefaultMaximumSlopeDegrees     = 90.0f;
constexpr float kDefaultSlopeFeatherDegreesLow  = 0.0f;
constexpr float kDefaultSlopeFeatherDegreesHigh = 0.0f;
constexpr float kDefaultSlopeGateStrength       = 1.0f;

// Mode across every entry that HAS `key`; ties (including zero entries) resolve to false.
bool SynthesizeBooleanMode(const nlohmann::json& stratums, const char* key) {
    int trueCount = 0, falseCount = 0;
    for (const nlohmann::json& stratumJson : stratums) {
        if (!stratumJson.is_object()) continue;
        bool value = false;
        if (ReadJsonBoolean(stratumJson, key, value)) { if (value) ++trueCount; else ++falseCount; }
    }
    return trueCount > falseCount;
}

// Arithmetic mean across every entry that HAS `key`; falls back to `fallback` if none do.
float SynthesizeFloatMean(const nlohmann::json& stratums, const char* key, float fallback) {
    float sum = 0.0f; int count = 0;
    for (const nlohmann::json& stratumJson : stratums) {
        if (!stratumJson.is_object()) continue;
        float value = 0.0f;
        if (ReadJsonFloat(stratumJson, key, value)) { sum += value; ++count; }
    }
    return (count > 0) ? (sum / static_cast<float>(count)) : fallback;
}

// Plain locals, not `Params::SlopeDefaults` — keeps this migration a pure JSON transform.
struct SynthesizedSlopeDefaults {
    bool  bSlopeGateEnabled       = false;
    float minimumSlopeDegrees     = kDefaultMinimumSlopeDegrees;
    float maximumSlopeDegrees     = kDefaultMaximumSlopeDegrees;
    float slopeFeatherDegreesLow  = kDefaultSlopeFeatherDegreesLow;
    float slopeFeatherDegreesHigh = kDefaultSlopeFeatherDegreesHigh;
    bool  bUseSmoothstep          = false;
    bool  bInvertSlopeGate        = false;
    float slopeGateStrength       = kDefaultSlopeGateStrength;
};

SynthesizedSlopeDefaults Synthesize(const nlohmann::json& stratums) {
    SynthesizedSlopeDefaults r;
    r.bSlopeGateEnabled       = SynthesizeBooleanMode(stratums, "SlopeGateEnabled");
    r.minimumSlopeDegrees     = SynthesizeFloatMean(stratums, "MinimumSlopeDegrees", kDefaultMinimumSlopeDegrees);
    r.maximumSlopeDegrees     = SynthesizeFloatMean(stratums, "MaximumSlopeDegrees", kDefaultMaximumSlopeDegrees);
    r.slopeFeatherDegreesLow  = SynthesizeFloatMean(stratums, "SlopeFeatherDegreesLow", kDefaultSlopeFeatherDegreesLow);
    r.slopeFeatherDegreesHigh = SynthesizeFloatMean(stratums, "SlopeFeatherDegreesHigh", kDefaultSlopeFeatherDegreesHigh);
    r.bUseSmoothstep          = SynthesizeBooleanMode(stratums, "UseSmoothstep");
    r.bInvertSlopeGate        = SynthesizeBooleanMode(stratums, "InvertSlopeGate");
    r.slopeGateStrength       = SynthesizeFloatMean(stratums, "SlopeGateStrength", kDefaultSlopeGateStrength);
    return r;
}

void WriteSlopeDefaultsJson(nlohmann::json& document, const SynthesizedSlopeDefaults& s) {
    nlohmann::json json;
    json["bSlopeGateEnabled"] = s.bSlopeGateEnabled;
    json["minimumSlopeDegrees"] = s.minimumSlopeDegrees;
    json["maximumSlopeDegrees"] = s.maximumSlopeDegrees;
    json["slopeFeatherDegreesLow"] = s.slopeFeatherDegreesLow;
    json["slopeFeatherDegreesHigh"] = s.slopeFeatherDegreesHigh;
    json["bUseSmoothstep"] = s.bUseSmoothstep;
    json["bInvertSlopeGate"] = s.bInvertSlopeGate;
    json["slopeGateStrength"] = s.slopeGateStrength;
    document["SlopeDefaults"] = std::move(json);
}

// True only if all 8 legacy fields are PRESENT and exactly equal the synthesized global — no
// tolerance (STEP40D: "not a bug to fuzz away"). A missing field can never be verified to match.
bool StratumMatchesSynthesizedDefault(const nlohmann::json& j, const SynthesizedSlopeDefaults& s) {
    bool bv = false; float fv = 0.0f;
    if (!ReadJsonBoolean(j, "SlopeGateEnabled", bv) || bv != s.bSlopeGateEnabled) return false;
    if (!ReadJsonFloat(j, "MinimumSlopeDegrees", fv) || fv != s.minimumSlopeDegrees) return false;
    if (!ReadJsonFloat(j, "MaximumSlopeDegrees", fv) || fv != s.maximumSlopeDegrees) return false;
    if (!ReadJsonFloat(j, "SlopeFeatherDegreesLow", fv) || fv != s.slopeFeatherDegreesLow) return false;
    if (!ReadJsonFloat(j, "SlopeFeatherDegreesHigh", fv) || fv != s.slopeFeatherDegreesHigh) return false;
    if (!ReadJsonBoolean(j, "UseSmoothstep", bv) || bv != s.bUseSmoothstep) return false;
    if (!ReadJsonBoolean(j, "InvertSlopeGate", bv) || bv != s.bInvertSlopeGate) return false;
    if (!ReadJsonFloat(j, "SlopeGateStrength", fv) || fv != s.slopeGateStrength) return false;
    return true;
}

// Additive-write discipline (STEP40D, the ticket's actual point): grows
// `StratumGenerationSettings` only up to `min(stratums.size(), 9)` entries and sets ONLY
// `SlopeUseGlobal` per real-stratum index — NEVER a wholesale array replace. The sibling
// `StratumGenerationSettings_Migrate_V2` (running SECOND) owns the other 8 keys and any padding.
void WriteSlopeUseGlobalFlags(nlohmann::json& document, const nlohmann::json& stratums,
                              const SynthesizedSlopeDefaults& synthesized) {
    constexpr std::size_t kMaxEntryCount = 9; // sanmapStratumCount (MapExporter_IO.h).
    const std::size_t entryCount = (stratums.size() < kMaxEntryCount) ? stratums.size() : kMaxEntryCount;

    nlohmann::json& settings = document["StratumGenerationSettings"];
    if (!settings.is_array()) settings = nlohmann::json::array();
    while (settings.size() < entryCount) settings.push_back(nlohmann::json::object());

    for (std::size_t index = 0; index < entryCount; ++index) {
        if (!settings[index].is_object()) settings[index] = nlohmann::json::object();
        const nlohmann::json& stratumJson = stratums[index];
        settings[index]["SlopeUseGlobal"] = stratumJson.is_object()
            && StratumMatchesSynthesizedDefault(stratumJson, synthesized);
    }
}

} // namespace

void SlopeDefaults_Migrate_V2(nlohmann::json& document) {
    if (!document.contains("mapGeneratorData") || !document["mapGeneratorData"].is_object()) return;
    const nlohmann::json& generatorData = document["mapGeneratorData"];
    if (!generatorData.contains("Stratums") || !generatorData["Stratums"].is_array()) return;
    const nlohmann::json stratums = generatorData["Stratums"]; // copy: document mutates below.
    if (stratums.empty()) return; // N = 0: no write to SlopeDefaults at all.

    const SynthesizedSlopeDefaults synthesized = Synthesize(stratums);
    WriteSlopeDefaultsJson(document, synthesized);
    WriteSlopeUseGlobalFlags(document, stratums, synthesized);
}

} // namespace Io
} // namespace SanmapGen
