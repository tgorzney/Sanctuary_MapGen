// SlopeDefaults_Migrate_V2_IO_Test.cpp — acceptance test (IO_MIGRATION_SPEC.md §1, STEP40D items
// 1-3): hand-built OLD-shape (V2) fixtures, asserting the exact synthesis rule (mode/mean/tie-break)
// after calling `SlopeDefaults_Migrate_V2` alone. The mandatory JOINT test proving this migration's
// `SlopeUseGlobal` write survives the sibling `StratumGenerationSettings_Migrate_V2` running second
// lives in `StratumGenerationSettings_Migrate_V2_IO_Test.cpp` (that sibling's own paired test, per
// the work-order's item 5) — not duplicated here.
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

// Item 1: a boolean tie case (confirms tie -> false) and a non-trivial float mean (confirms real
// averaging, not first-value). 2 strata: SlopeGateEnabled true/false (tie -> false); minimumSlope
// 10/20 (mean 15, not 10).
void CheckModeAndMeanSynthesis() {
    nlohmann::json document;
    document["mapGeneratorData"]["Stratums"] = nlohmann::json::array({
        MakeStratum(/*gate*/ true,  /*min*/ 10.0f, /*max*/ 80.0f, /*fl*/ 2.0f, /*fh*/ 4.0f,
                   /*smooth*/ true,  /*invert*/ false, /*strength*/ 0.25f),
        MakeStratum(/*gate*/ false, /*min*/ 20.0f, /*max*/ 60.0f, /*fl*/ 6.0f, /*fh*/ 8.0f,
                   /*smooth*/ false, /*invert*/ true,  /*strength*/ 0.75f),
    });

    Io::SlopeDefaults_Migrate_V2(document);

    Check(document.contains("SlopeDefaults") && document["SlopeDefaults"].is_object(),
          "a non-empty Stratums array produces a SlopeDefaults write");
    const nlohmann::json& slopeDefaults = document["SlopeDefaults"];
    // Booleans: 1 true / 1 false each -> tie -> false, for all three.
    Check(slopeDefaults["bSlopeGateEnabled"] == false, "bSlopeGateEnabled tie (1/1) resolves to false");
    Check(slopeDefaults["bUseSmoothstep"] == false, "bUseSmoothstep tie (1/1) resolves to false");
    Check(slopeDefaults["bInvertSlopeGate"] == false, "bInvertSlopeGate tie (1/1) resolves to false");
    // Floats: real arithmetic mean, not the first stratum's value.
    Check(slopeDefaults["minimumSlopeDegrees"] == 15.0f, "minimumSlopeDegrees means (10+20)/2 = 15, not 10");
    Check(slopeDefaults["maximumSlopeDegrees"] == 70.0f, "maximumSlopeDegrees means (80+60)/2 = 70");
    Check(slopeDefaults["slopeFeatherDegreesLow"] == 4.0f, "slopeFeatherDegreesLow means (2+6)/2 = 4");
    Check(slopeDefaults["slopeFeatherDegreesHigh"] == 6.0f, "slopeFeatherDegreesHigh means (4+8)/2 = 6");
    Check(slopeDefaults["slopeGateStrength"] == 0.5f, "slopeGateStrength means (0.25+0.75)/2 = 0.5");
}

// A clean majority case (not a tie) also lands correctly — 2 true / 1 false -> true.
void CheckNonTieBooleanMode() {
    nlohmann::json document;
    document["mapGeneratorData"]["Stratums"] = nlohmann::json::array({
        MakeStratum(true, 0, 90, 0, 0, false, false, 1.0f),
        MakeStratum(true, 0, 90, 0, 0, false, false, 1.0f),
        MakeStratum(false, 0, 90, 0, 0, false, false, 1.0f),
    });
    Io::SlopeDefaults_Migrate_V2(document);
    Check(document["SlopeDefaults"]["bSlopeGateEnabled"] == true,
          "a genuine 2/1 majority resolves to the majority value, not a tie-break");
}

// Item 2: N = 0 (no Stratums array at all, and a separately-checked empty array) performs NO write
// to SlopeDefaults at all.
void CheckZeroStrataIsNoWrite() {
    nlohmann::json noBlobDocument = { {"someOtherField", 1} };
    Io::SlopeDefaults_Migrate_V2(noBlobDocument);
    Check(!noBlobDocument.contains("SlopeDefaults"), "no mapGeneratorData at all: no SlopeDefaults write");

    nlohmann::json emptyArrayDocument;
    emptyArrayDocument["mapGeneratorData"]["Stratums"] = nlohmann::json::array();
    Io::SlopeDefaults_Migrate_V2(emptyArrayDocument);
    Check(!emptyArrayDocument.contains("SlopeDefaults"), "an empty Stratums array: no SlopeDefaults write");
    Check(!emptyArrayDocument.contains("StratumGenerationSettings"),
          "an empty Stratums array: no StratumGenerationSettings write either");
}

// Item 3: bSlopeUseGlobal lands false for strata whose values genuinely differ, true for one whose
// 8 values are deliberately set to equal the synthesized mean/mode. 3 strata: stratum 0 IS the
// target (false, 10, 80, 2, 4, false, false, 0.5); strata 1/2 are constructed so the mean/mode over
// all 3 lands exactly on stratum 0's own values (real averaging: their float values differ from
// stratum 0's, they just average out to it) while both individually differ from the target.
void CheckSlopeUseGlobalExactMatch() {
    nlohmann::json document;
    document["mapGeneratorData"]["Stratums"] = nlohmann::json::array({
        MakeStratum(/*gate*/ false, 10.0f, 80.0f, 2.0f, 4.0f, /*smooth*/ false, /*invert*/ false, 0.5f),
        MakeStratum(/*gate*/ false, 5.0f, 70.0f, 1.0f, 3.0f, /*smooth*/ true, /*invert*/ false, 0.4f),
        MakeStratum(/*gate*/ true, 15.0f, 90.0f, 3.0f, 5.0f, /*smooth*/ false, /*invert*/ true, 0.6f),
    });

    Io::SlopeDefaults_Migrate_V2(document);

    const nlohmann::json& slopeDefaults = document["SlopeDefaults"];
    Check(slopeDefaults["bSlopeGateEnabled"] == false && slopeDefaults["bUseSmoothstep"] == false
          && slopeDefaults["bInvertSlopeGate"] == false, "sanity: all 3 booleans land a clean 2/1 false majority");
    Check(slopeDefaults["minimumSlopeDegrees"] == 10.0f && slopeDefaults["maximumSlopeDegrees"] == 80.0f
          && slopeDefaults["slopeFeatherDegreesLow"] == 2.0f && slopeDefaults["slopeFeatherDegreesHigh"] == 4.0f
          && slopeDefaults["slopeGateStrength"] == 0.5f,
          "sanity: the 5 float means land exactly on stratum 0's values by construction");

    const nlohmann::json& settings = document["StratumGenerationSettings"];
    Check(settings[0]["SlopeUseGlobal"] == true,
          "a stratum whose 8 values exactly equal the synthesized default lands bSlopeUseGlobal=true");
    Check(settings[1]["SlopeUseGlobal"] == false,
          "a stratum whose values genuinely differ (even if they average out to the default) lands "
          "bSlopeUseGlobal=false");
    Check(settings[2]["SlopeUseGlobal"] == false,
          "a second, differently-differing stratum also lands bSlopeUseGlobal=false");
}

// A stratum missing some of the 8 keys entirely can never be verified to match -> false.
void CheckMissingFieldNeverMatches() {
    nlohmann::json document;
    nlohmann::json incompleteStratum = MakeStratum(false, 0.0f, 90.0f, 0.0f, 0.0f, false, false, 1.0f);
    incompleteStratum.erase("SlopeGateStrength"); // now missing one of the 8 keys.
    document["mapGeneratorData"]["Stratums"] = nlohmann::json::array({ incompleteStratum });

    Io::SlopeDefaults_Migrate_V2(document);

    Check(document["StratumGenerationSettings"][0]["SlopeUseGlobal"] == false,
          "a stratum missing one of the 8 legacy keys can never be verified to match -> false");
}

} // namespace

int main() {
    CheckModeAndMeanSynthesis();
    CheckNonTieBooleanMode();
    CheckZeroStrataIsNoWrite();
    CheckSlopeUseGlobalExactMatch();
    CheckMissingFieldNeverMatches();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
