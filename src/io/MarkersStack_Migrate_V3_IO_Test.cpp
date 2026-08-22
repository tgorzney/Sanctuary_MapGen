// MarkersStack_Migrate_V3_IO_Test.cpp — acceptance test (IO_MIGRATION_SPEC.md §1, STEP67's own 10
// numbered acceptance items). One hand-built OLD-shape (flat, V3) fixture per case, asserting the
// exact NEW (two-level, V4) shape after calling `MarkersStack_Migrate_V3` alone. Not a round-trip
// test, not a runner test.
#include "MarkersStack_Migrate_V3_IO.h"
#include "../params/Symmetry_PARAMS.h"
#include <cstdio>

using namespace SanmapGen;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL: %s\n", label); ++failureCount; }
}

// A minimal old-shape (flat V3) rule: the 3 symmetry keys plus a couple of untouched fields, so
// tests 6/7 can assert the untouched fields survive and the symmetry keys are gone.
nlohmann::json BuildOldRuleJson(bool bSymmetryUseGlobal, int symmetryMask, int radialSymmetryRepeatCount,
                                const char* category, float minSlope, float hydroMultiplier) {
    nlohmann::json rule;
    rule["SymmetryUseGlobal"]         = bSymmetryUseGlobal;
    rule["SymmetryMask"]              = symmetryMask;
    rule["RadialSymmetryRepeatCount"] = radialSymmetryRepeatCount;
    rule["Category"]                  = category;
    rule["MinSlope"]                  = minSlope;
    rule["HydroMultiplier"]           = hydroMultiplier;
    return rule;
}

// (1) All rules share one triplet -> one layer, all rules, correct lifted triplet.
void CheckAllSharedTripletProducesOneLayer() {
    nlohmann::json document;
    document["MarkersStack"] = nlohmann::json::array({
        BuildOldRuleJson(true, 3, 4, "Spawn", 10.0f, 1.0f),
        BuildOldRuleJson(true, 3, 4, "Alloys", 20.0f, 2.0f),
        BuildOldRuleJson(true, 3, 4, "Generic", 30.0f, 3.0f),
    });

    Io::MarkersStack_Migrate_V3(document);

    const nlohmann::json& stack = document["MarkersStack"];
    Check(stack.size() == 1, "test 1: three rules sharing one triplet migrate into exactly one layer");
    Check(stack[0]["SymmetryUseGlobal"] == true && stack[0]["SymmetryMask"] == 3
          && stack[0]["RadialSymmetryRepeatCount"] == 4,
          "test 1: the layer carries the shared triplet");
    Check(stack[0]["Rules"].size() == 3, "test 1: the layer carries all 3 rules");
    Check(stack[0]["Name"] == "Migrated Layer 1", "test 1: the layer is synthesized-named");
}

// (2) Two contiguous groups (A,A,B,B) -> two layers in order, correct membership/triplet each.
void CheckTwoContiguousGroupsProduceTwoLayers() {
    nlohmann::json document;
    document["MarkersStack"] = nlohmann::json::array({
        BuildOldRuleJson(true, 1, 3, "Spawn", 1.0f, 1.0f),
        BuildOldRuleJson(true, 1, 3, "Spawn", 2.0f, 1.0f),
        BuildOldRuleJson(false, 2, 5, "Alloys", 3.0f, 1.0f),
        BuildOldRuleJson(false, 2, 5, "Alloys", 4.0f, 1.0f),
    });

    Io::MarkersStack_Migrate_V3(document);

    const nlohmann::json& stack = document["MarkersStack"];
    Check(stack.size() == 2, "test 2: (A,A,B,B) migrates into exactly two layers");
    Check(stack[0]["Name"] == "Migrated Layer 1" && stack[1]["Name"] == "Migrated Layer 2",
          "test 2: layers are numbered in output order");
    Check(stack[0]["SymmetryUseGlobal"] == true && stack[0]["SymmetryMask"] == 1
          && stack[0]["RadialSymmetryRepeatCount"] == 3 && stack[0]["Rules"].size() == 2,
          "test 2: layer 1 carries the A triplet and its 2 member rules");
    Check(stack[1]["SymmetryUseGlobal"] == false && stack[1]["SymmetryMask"] == 2
          && stack[1]["RadialSymmetryRepeatCount"] == 5 && stack[1]["Rules"].size() == 2,
          "test 2: layer 2 carries the B triplet and its 2 member rules");
    Check(stack[0]["Rules"][0]["MinSlope"] == 1.0f && stack[0]["Rules"][1]["MinSlope"] == 2.0f,
          "test 2: layer 1's rules keep their original relative order");
    Check(stack[1]["Rules"][0]["MinSlope"] == 3.0f && stack[1]["Rules"][1]["MinSlope"] == 4.0f,
          "test 2: layer 2's rules keep their original relative order");
}

// (3) Alternating (A,B,A) -> three single-rule layers, original order preserved end-to-end.
void CheckAlternatingTripletsProduceThreeSingleRuleLayers() {
    nlohmann::json document;
    document["MarkersStack"] = nlohmann::json::array({
        BuildOldRuleJson(true, 1, 3, "Spawn", 1.0f, 1.0f),
        BuildOldRuleJson(false, 2, 5, "Alloys", 2.0f, 1.0f),
        BuildOldRuleJson(true, 1, 3, "Generic", 3.0f, 1.0f),
    });

    Io::MarkersStack_Migrate_V3(document);

    const nlohmann::json& stack = document["MarkersStack"];
    Check(stack.size() == 3, "test 3: (A,B,A) migrates into three single-rule layers, not two merged");
    Check(stack[0]["Rules"].size() == 1 && stack[1]["Rules"].size() == 1 && stack[2]["Rules"].size() == 1,
          "test 3: every layer carries exactly its one rule");
    Check(stack[0]["Rules"][0]["MinSlope"] == 1.0f && stack[1]["Rules"][0]["MinSlope"] == 2.0f
          && stack[2]["Rules"][0]["MinSlope"] == 3.0f,
          "test 3: flattening the 3 layers back in order reproduces the exact original rule order");
    Check(stack[0]["SymmetryMask"] == 1 && stack[1]["SymmetryMask"] == 2 && stack[2]["SymmetryMask"] == 1,
          "test 3: non-adjacent A groups are never merged — the first and third layer stay separate");
}

// (4) A rule missing a symmetry key entirely compares equal to a neighbor explicitly carrying that
// field's own struct default — no spurious group-splitting from a hand-edited/partial file.
void CheckMissingKeyRuleMatchesExplicitDefaultNeighbor() {
    nlohmann::json document;
    nlohmann::json ruleWithNoSymmetryKeysAtAll;
    ruleWithNoSymmetryKeysAtAll["Category"] = "Generic";
    nlohmann::json ruleWithExplicitDefaults =
        BuildOldRuleJson(/*bSymmetryUseGlobal=*/true, /*symmetryMask=*/0,
                         /*radialSymmetryRepeatCount=*/3, "Generic", 0.0f, 1.0f);
    document["MarkersStack"] = nlohmann::json::array({ ruleWithNoSymmetryKeysAtAll, ruleWithExplicitDefaults });

    Io::MarkersStack_Migrate_V3(document);

    const nlohmann::json& stack = document["MarkersStack"];
    Check(stack.size() == 1, "test 4: a missing-key rule and an explicit-default neighbor group together");
    Check(stack[0]["Rules"].size() == 2, "test 4: both rules land in the same single layer");
}

// (5) An out-of-range value normalizes/clamps on the layer, not passed through raw.
void CheckOutOfRangeValueClampsOnTheLayer() {
    nlohmann::json document;
    document["MarkersStack"] = nlohmann::json::array({
        BuildOldRuleJson(true, 1, /*radialSymmetryRepeatCount=*/500, "Generic", 0.0f, 1.0f),
    });

    Io::MarkersStack_Migrate_V3(document);

    const nlohmann::json& stack = document["MarkersStack"];
    Check(stack[0]["RadialSymmetryRepeatCount"] == Params::radialSymmetryRepeatCountMaximum,
          "test 5: an out-of-range RadialSymmetryRepeatCount clamps to the maximum on the layer, "
          "same normalization the live importer already applies — not new lossy behavior");
}

// (6) Post-migration, no rule object retains any of the 3 removed keys.
void CheckNoRuleRetainsARemovedSymmetryKey() {
    nlohmann::json document;
    document["MarkersStack"] = nlohmann::json::array({
        BuildOldRuleJson(true, 3, 4, "Spawn", 1.0f, 1.0f),
        BuildOldRuleJson(true, 3, 4, "Alloys", 2.0f, 1.0f),
    });

    Io::MarkersStack_Migrate_V3(document);

    const nlohmann::json& rules = document["MarkersStack"][0]["Rules"];
    for (const nlohmann::json& rule : rules) {
        Check(!rule.contains("SymmetryUseGlobal") && !rule.contains("SymmetryMask")
              && !rule.contains("RadialSymmetryRepeatCount"),
              "test 6: no rule object retains any of the 3 removed symmetry keys");
    }
}

// (7) Every untouched rule field survives verbatim.
void CheckUntouchedFieldsSurviveVerbatim() {
    nlohmann::json document;
    document["MarkersStack"] = nlohmann::json::array({
        BuildOldRuleJson(true, 3, 4, "Alloys", 12.5f, 3.5f),
    });

    Io::MarkersStack_Migrate_V3(document);

    const nlohmann::json& rule = document["MarkersStack"][0]["Rules"][0];
    Check(rule["Category"] == "Alloys" && rule["MinSlope"] == 12.5f && rule["HydroMultiplier"] == 3.5f,
          "test 7: every untouched rule field survives verbatim");
}

// (8) Empty/missing MarkersStack -> total no-op.
void CheckEmptyOrMissingMarkersStackIsNoOp() {
    nlohmann::json documentWithMissingKey = { {"someOtherField", 1} };
    Io::MarkersStack_Migrate_V3(documentWithMissingKey);
    Check(!documentWithMissingKey.contains("MarkersStack"),
          "test 8: a document with no MarkersStack key at all produces no MarkersStack section");
    Check(documentWithMissingKey["someOtherField"] == 1, "test 8: the rest of the document is untouched");

    nlohmann::json documentWithEmptyArray;
    documentWithEmptyArray["MarkersStack"] = nlohmann::json::array();
    Io::MarkersStack_Migrate_V3(documentWithEmptyArray);
    Check(documentWithEmptyArray["MarkersStack"].empty(),
          "test 8: an already-empty MarkersStack array stays empty (total no-op)");
}

// (9) Second call on already-V4-shaped output -> safe no-op (idempotency).
void CheckSecondCallOnAlreadyMigratedOutputIsNoOp() {
    nlohmann::json document;
    document["MarkersStack"] = nlohmann::json::array({
        BuildOldRuleJson(true, 3, 4, "Spawn", 1.0f, 1.0f),
        BuildOldRuleJson(false, 1, 5, "Alloys", 2.0f, 1.0f),
    });

    Io::MarkersStack_Migrate_V3(document);
    const nlohmann::json afterFirstCall = document["MarkersStack"];

    Io::MarkersStack_Migrate_V3(document);
    Check(document["MarkersStack"] == afterFirstCall,
          "test 9: a second call on an already-V4-shaped MarkersStack array is a safe no-op");
}

// (10) `bIndependentlySelectable` isolation assertion: running this migration ALONE (no sibling
// migration/domain having run first) reproduces the exact same result — this migration reads/writes
// exclusively within its own MarkersStack key, no cross-domain read (unlike SlopeDefaults_Migrate_V2,
// correctly `bIndependentlySelectable = false`). Trivially true here (nothing else touches
// MarkersStack), but the convention requires the explicit test.
void CheckIndependentlySelectableIsolation() {
    nlohmann::json documentRunAlone;
    documentRunAlone["MarkersStack"] = nlohmann::json::array({
        BuildOldRuleJson(true, 3, 4, "Spawn", 1.0f, 1.0f),
    });
    // A sibling top-level section this migration must never read or touch, present to prove
    // isolation — the result below is identical whether or not this key exists.
    documentRunAlone["Symmetry"] = { {"GlobalSymmetryMask", 7} };

    Io::MarkersStack_Migrate_V3(documentRunAlone);

    Check(documentRunAlone["Symmetry"]["GlobalSymmetryMask"] == 7,
          "test 10: an unrelated sibling top-level section is untouched by running this migration alone");
    Check(documentRunAlone["MarkersStack"].size() == 1
          && documentRunAlone["MarkersStack"][0]["SymmetryMask"] == 3,
          "test 10: running this migration alone reproduces the full-step MarkersStack result");
}

} // namespace

int main() {
    CheckAllSharedTripletProducesOneLayer();
    CheckTwoContiguousGroupsProduceTwoLayers();
    CheckAlternatingTripletsProduceThreeSingleRuleLayers();
    CheckMissingKeyRuleMatchesExplicitDefaultNeighbor();
    CheckOutOfRangeValueClampsOnTheLayer();
    CheckNoRuleRetainsARemovedSymmetryKey();
    CheckUntouchedFieldsSurviveVerbatim();
    CheckEmptyOrMissingMarkersStackIsNoOp();
    CheckSecondCallOnAlreadyMigratedOutputIsNoOp();
    CheckIndependentlySelectableIsolation();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
