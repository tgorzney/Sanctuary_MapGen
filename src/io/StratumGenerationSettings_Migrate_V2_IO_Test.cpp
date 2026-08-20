// StratumGenerationSettings_Migrate_V2_IO_Test.cpp — acceptance test (IO_MIGRATION_SPEC.md §1,
// STEP40D items 4-5). Item 5, the mandatory JOINT test, is this ticket's actual point: it runs BOTH
// `SlopeDefaults_Migrate_V2` and `StratumGenerationSettings_Migrate_V2` in the manifest's prescribed
// order against the SAME document and proves neither's write to `StratumGenerationSettings[i]`
// clobbers the other's.
#include "StratumGenerationSettings_Migrate_V2_IO.h"
#include "SlopeDefaults_Migrate_V2_IO.h"
#include <cstdio>

using namespace SanmapGen;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL: %s\n", label); ++failureCount; }
}

nlohmann::json MakeStratum(bool bGateEnabled, float minimumDegrees, float maximumDegrees,
                           float featherLow, float featherHigh, bool bUseSmoothstep,
                           bool bInvert, float strength) {
    nlohmann::json stratumJson;
    stratumJson["SlopeGateEnabled"]        = bGateEnabled;
    stratumJson["MinimumSlopeDegrees"]     = minimumDegrees;
    stratumJson["MaximumSlopeDegrees"]     = maximumDegrees;
    stratumJson["SlopeFeatherDegreesLow"]  = featherLow;
    stratumJson["SlopeFeatherDegreesHigh"] = featherHigh;
    stratumJson["UseSmoothstep"]           = bUseSmoothstep;
    stratumJson["InvertSlopeGate"]         = bInvert;
    stratumJson["SlopeGateStrength"]       = strength;
    return stratumJson;
}

void CheckStratumFields(const nlohmann::json& entry, bool bGate, float minimum, float maximum,
                        float featherLow, float featherHigh, bool bSmooth, bool bInvert,
                        float strength, const char* label) {
    Check(entry["SlopeGateEnabled"] == bGate && entry["MinimumSlopeDegrees"] == minimum
          && entry["MaximumSlopeDegrees"] == maximum && entry["SlopeFeatherDegreesLow"] == featherLow
          && entry["SlopeFeatherDegreesHigh"] == featherHigh && entry["UseSmoothstep"] == bSmooth
          && entry["InvertSlopeGate"] == bInvert && entry["SlopeGateStrength"] == strength, label);
}

// Item 4: alone, relocates all 8 fields correctly, index-aligned, padded to exactly 9 entries.
void CheckAloneRelocatesEightFieldsPaddedToNine() {
    nlohmann::json document;
    document["mapGeneratorData"]["Stratums"] = nlohmann::json::array({
        MakeStratum(true, 5.0f, 85.0f, 1.0f, 3.0f, true, false, 0.9f),
        MakeStratum(false, 15.0f, 75.0f, 2.0f, 4.0f, false, true, 0.1f),
    });

    Io::StratumGenerationSettings_Migrate_V2(document);

    const nlohmann::json& settings = document["StratumGenerationSettings"];
    Check(settings.is_array() && settings.size() == 9, "padded to exactly 9 entries");
    CheckStratumFields(settings[0], true, 5.0f, 85.0f, 1.0f, 3.0f, true, false, 0.9f,
                       "stratum 0's 8 fields relocate correctly, index-aligned");
    CheckStratumFields(settings[1], false, 15.0f, 75.0f, 2.0f, 4.0f, false, true, 0.1f,
                       "stratum 1's 8 fields relocate correctly, index-aligned");
    for (int index = 2; index < 9; ++index)
        Check(settings[index].is_object() && !settings[index].contains("SlopeGateEnabled"),
              "a padding entry beyond Stratums.size() gets no relocated fields (no legacy source)");
    // SlopeDefaults_Migrate_V2 never ran here, so EVERY entry (real and padding) defaults
    // SlopeUseGlobal = true via this migration's own DefaultIfMissing.
    for (int index = 0; index < 9; ++index)
        Check(settings[index]["SlopeUseGlobal"] == true,
              "run alone (sibling never ran), every entry defaults SlopeUseGlobal = true");
}

// N = 0 short-circuit (STEP41_PostMigrationImportGaps_IO): no legacy Stratums data at all produces
// NO `StratumGenerationSettings` key whatsoever — not even the padding array — mirroring the
// sibling `SlopeDefaults_Migrate_V2`'s own N = 0 short-circuit. Before this fix, this migration
// unconditionally padded to 9 entries even with zero source data, which then tripped
// `ReadStratumGenerationSettingsJson`'s cardinality check against `stratumLayers`'s real length.
void CheckNoLegacyDataProducesNoKeyAtAll() {
    nlohmann::json document = { {"someOtherField", 1} };
    Io::StratumGenerationSettings_Migrate_V2(document);
    Check(!document.contains("StratumGenerationSettings"),
          "no legacy Stratums at all produces no StratumGenerationSettings key at all");
}

// Same short-circuit, three more ways to arrive at N = 0: an empty `Stratums` array, a `Stratums`
// key present but not an array, and a `mapGeneratorData` present but not an object.
void CheckEmptyStratumsArrayProducesNoKeyAtAll() {
    nlohmann::json document;
    document["mapGeneratorData"]["Stratums"] = nlohmann::json::array();
    Io::StratumGenerationSettings_Migrate_V2(document);
    Check(!document.contains("StratumGenerationSettings"),
          "an empty Stratums array produces no StratumGenerationSettings key at all");
}

// Item 5 (THE mandatory joint test): SlopeDefaults_Migrate_V2 runs FIRST and sets SlopeUseGlobal per
// real-stratum index; StratumGenerationSettings_Migrate_V2 runs SECOND and additively sets its own
// 8 keys onto the SAME per-index objects. Neither may clobber the other's write.
void CheckJointMigrationsAdditiveDiscipline() {
    nlohmann::json document;
    document["mapGeneratorData"]["Stratums"] = nlohmann::json::array({
        MakeStratum(true, 10.0f, 80.0f, 2.0f, 4.0f, true, false, 0.25f),
        MakeStratum(false, 20.0f, 60.0f, 6.0f, 8.0f, false, true, 0.75f),
    });

    Io::SlopeDefaults_Migrate_V2(document);              // runs FIRST: sets SlopeUseGlobal per index.
    Io::StratumGenerationSettings_Migrate_V2(document);   // runs SECOND: additively sets its own 8 keys.

    const nlohmann::json& settings = document["StratumGenerationSettings"];
    Check(settings.is_array() && settings.size() == 9, "the joint result is padded to exactly 9 entries");

    // Neither real-stratum index lost SlopeDefaults_Migrate_V2's SlopeUseGlobal write. Both strata
    // here genuinely differ from the synthesized mean/mode, so the correct value is false for both —
    // a wholesale-overwrite bug (e.g. StratumGenerationSettings_Migrate_V2 defaulting it to true via
    // an unconditional DefaultIfMissing regardless of whether it's already set) would show up here.
    Check(settings[0]["SlopeUseGlobal"] == false,
          "stratum 0's SlopeUseGlobal (computed false by SlopeDefaults_Migrate_V2) survives the "
          "second migration");
    Check(settings[1]["SlopeUseGlobal"] == false,
          "stratum 1's SlopeUseGlobal (computed false by SlopeDefaults_Migrate_V2) survives the "
          "second migration");

    // Neither real-stratum index lost StratumGenerationSettings_Migrate_V2's own 8 relocated keys —
    // a wholesale `StratumGenerationSettings = newArray` bug would have discarded SlopeUseGlobal
    // above; the inverse bug (never actually writing the 8 keys) shows up here.
    CheckStratumFields(settings[0], true, 10.0f, 80.0f, 2.0f, 4.0f, true, false, 0.25f,
                       "stratum 0's 8 relocated fields land correctly, additively alongside SlopeUseGlobal");
    CheckStratumFields(settings[1], false, 20.0f, 60.0f, 6.0f, 8.0f, false, true, 0.75f,
                       "stratum 1's 8 relocated fields land correctly, additively alongside SlopeUseGlobal");

    // Padding indices (2..8): SlopeDefaults_Migrate_V2 never touches them (i in [0, Stratums.size())
    // only); StratumGenerationSettings_Migrate_V2's DefaultIfMissing gives them SlopeUseGlobal = true
    // and no relocated fields (no legacy source for a stratum that doesn't exist).
    for (int index = 2; index < 9; ++index) {
        Check(settings[index]["SlopeUseGlobal"] == true,
              "a padding entry gets SlopeUseGlobal = true via DefaultIfMissing, never a raw overwrite");
        Check(!settings[index].contains("SlopeGateEnabled"),
              "a padding entry gets no relocated fields — no legacy source exists for it");
    }
}

} // namespace

int main() {
    CheckAloneRelocatesEightFieldsPaddedToNine();
    CheckNoLegacyDataProducesNoKeyAtAll();
    CheckEmptyStratumsArrayProducesNoKeyAtAll();
    CheckJointMigrationsAdditiveDiscipline();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
