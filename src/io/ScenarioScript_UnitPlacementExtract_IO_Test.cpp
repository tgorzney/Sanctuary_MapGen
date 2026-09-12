// ScenarioScript_UnitPlacementExtract_IO_Test.cpp -- pure-logic acceptance test for the closed
// literal-only Shape 3 grammar (STEP265, ARCH_15_14 Part B). No filesystem, no disk, matches the
// header's own "pure and total" contract. Per the work-order's own ground-truth note, no file in this
// repository's map_scripts_backup/ corpus contains a literal table of this shape -- every fixture here
// is synthetic, written to spec.
#include "ScenarioScript_UnitPlacementExtract_IO.h"
#include <cstdio>
#include <string>

using namespace SanmapGen;

static int failureCount = 0;

static void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failureCount; }
}

// 1a. The full 5-key form extracts verbatim, with rotation defaulting to identity.
static void TestFullFiveKeyFormExtractsWithIdentityRotation() {
    const std::string source =
        "local UNIT_PLACEMENTS = {\n"
        "    { armyName = \"ARMY_01\", templateIdentifier = \"ucn3001\", x = 10, y = 0, z = 20 },\n"
        "}\n";
    const Io::ScenarioUnitPlacementExtractionResult result =
        Io::ExtractUnitPlacementsFromScenarioScriptText(source, 100);
    Check(result.placements.size() == 1, "FiveKey: one placement");
    Check(result.nearMisses.empty(), "FiveKey: no near-misses");
    if (result.placements.size() == 1) {
        const Params::ScenarioUnitPlacement& placement = result.placements[0];
        Check(placement.armyName == "ARMY_01", "FiveKey: armyName verbatim");
        Check(placement.templateIdentifier == "ucn3001", "FiveKey: templateIdentifier verbatim");
        Check(placement.positionX == 10.0f, "FiveKey: positionX verbatim");
        Check(placement.positionY == 0.0f, "FiveKey: positionY verbatim");
        Check(placement.positionZ == 79.0f, "FiveKey: positionZ flipped (100 - 20 - 1 = 79)");
        Check(placement.rotationX == 0.0f && placement.rotationY == 0.0f && placement.rotationZ == 0.0f
                  && placement.rotationW == 1.0f,
              "FiveKey: rotation defaults to identity when the whole key is absent");
    }
}

// 1b. The 5-key+rotation form extracts the nested rotation table verbatim, unflipped.
static void TestFiveKeyPlusRotationFormExtractsRotationVerbatim() {
    const std::string source =
        "local UNIT_PLACEMENTS = {\n"
        "    { armyName = \"ARMY_02\", templateIdentifier = \"ucn3002\", x = 1, y = 2, z = 3,\n"
        "      rotation = { x = 0.1, y = 0.2, z = 0.3, w = 0.9 } },\n"
        "}\n";
    const Io::ScenarioUnitPlacementExtractionResult result =
        Io::ExtractUnitPlacementsFromScenarioScriptText(source, 50);
    Check(result.placements.size() == 1, "FiveKeyPlusRotation: one placement");
    Check(result.nearMisses.empty(), "FiveKeyPlusRotation: no near-misses");
    if (result.placements.size() == 1) {
        const Params::ScenarioUnitPlacement& placement = result.placements[0];
        Check(placement.rotationX == 0.1f && placement.rotationY == 0.2f
                  && placement.rotationZ == 0.3f && placement.rotationW == 0.9f,
              "FiveKeyPlusRotation: rotation maps verbatim, no flip");
    }
}

// 1c. A row missing one required key (here, 'z') is a near-miss for that row only -- a sibling valid
//     row in the same array still extracts.
static void TestRowMissingRequiredKeyIsNearMissOnlyForThatRow() {
    const std::string source =
        "local UNIT_PLACEMENTS = {\n"
        "    { armyName = \"ARMY_03\", templateIdentifier = \"ucn3003\", x = 1, y = 2 },\n"   // missing z
        "    { armyName = \"ARMY_04\", templateIdentifier = \"ucn3004\", x = 4, y = 5, z = 6 },\n"
        "}\n";
    const Io::ScenarioUnitPlacementExtractionResult result =
        Io::ExtractUnitPlacementsFromScenarioScriptText(source, 10);
    Check(result.placements.size() == 1 && result.placements[0].armyName == "ARMY_04",
          "MissingKey: only the well-formed sibling row extracted");
    Check(result.nearMisses.size() == 1, "MissingKey: exactly one near-miss");
    if (result.nearMisses.size() == 1) {
        Check(result.nearMisses[0].armyName == "ARMY_03", "MissingKey: near-miss context is the row's own armyName");
        Check(result.nearMisses[0].reason.find("z") != std::string::npos,
              "MissingKey: reason names the missing key 'z'");
    }
}

// 1d. A row keyed by 'armyIndex' instead of 'armyName' is rejected as a near-miss naming exactly that
//     ambiguity -- never reinterpreted as a stable identity, and never guessed across.
static void TestRowKeyedByArmyIndexIsRejectedWithExactReason() {
    const std::string source =
        "local UNIT_PLACEMENTS = {\n"
        "    { armyIndex = 3, templateIdentifier = \"ucn3005\", x = 1, y = 2, z = 3 },\n"
        "}\n";
    const Io::ScenarioUnitPlacementExtractionResult result =
        Io::ExtractUnitPlacementsFromScenarioScriptText(source, 10);
    Check(result.placements.empty(), "ArmyIndex: zero placements extracted");
    Check(result.nearMisses.size() == 1, "ArmyIndex: exactly one near-miss");
    if (result.nearMisses.size() == 1) {
        Check(result.nearMisses[0].reason.find("armyIndex") != std::string::npos,
              "ArmyIndex: reason names 'armyIndex' exactly");
        Check(result.nearMisses[0].reason.find("never reinterpreted") != std::string::npos,
              "ArmyIndex: reason states the ambiguity explicitly, not a generic 'unrecognized field'");
    }
}

// 2. Z-flip round-trip: z = 10 with mapSize = 100 extracts to positionZ == 89; applying the SAME
//    self-inverse `mapSize - positionZ - 1` transform the exporter's own FlipPositionZ uses
//    (ScenarioScript_DataLua_IO.cpp) recovers z == 10 exactly.
static void TestZFlipRoundTrip() {
    const std::string source =
        "local UNIT_PLACEMENTS = {\n"
        "    { armyName = \"ARMY_05\", templateIdentifier = \"ucn3006\", x = 0, y = 0, z = 10 },\n"
        "}\n";
    const Io::ScenarioUnitPlacementExtractionResult result =
        Io::ExtractUnitPlacementsFromScenarioScriptText(source, 100);
    Check(result.placements.size() == 1, "ZFlip: one placement");
    if (result.placements.size() == 1) {
        Check(result.placements[0].positionZ == 89.0f, "ZFlip: 100 - 10 - 1 = 89");
        const float roundTrippedZ = 100.0f - result.placements[0].positionZ - 1.0f;
        Check(roundTrippedZ == 10.0f, "ZFlip: applying FlipPositionZ's own formula a second time recovers z == 10");
    }
}

// 3a. Absence of the whole 'rotation' key is NOT a near-miss (already covered by
//     TestFullFiveKeyFormExtractsWithIdentityRotation above); this test isolates the PARTIAL case: a
//     rotation table missing one of its four sub-fields is a near-miss for that row.
static void TestPartialRotationTableIsNearMiss() {
    const std::string source =
        "local UNIT_PLACEMENTS = {\n"
        "    { armyName = \"ARMY_06\", templateIdentifier = \"ucn3007\", x = 1, y = 2, z = 3,\n"
        "      rotation = { x = 0.1, y = 0.2 } },\n"   // missing z, w
        "}\n";
    const Io::ScenarioUnitPlacementExtractionResult result =
        Io::ExtractUnitPlacementsFromScenarioScriptText(source, 10);
    Check(result.placements.empty(), "PartialRotation: zero placements extracted");
    Check(result.nearMisses.size() == 1, "PartialRotation: exactly one near-miss");
    if (result.nearMisses.size() == 1) {
        Check(result.nearMisses[0].reason.find("partial rotation") != std::string::npos,
              "PartialRotation: reason names the partial-rotation rule");
    }
}

// 4. An extra, unrecognized key on an otherwise well-formed row is a near-miss for that row.
static void TestExtraUnrecognizedKeyIsNearMiss() {
    const std::string source =
        "local UNIT_PLACEMENTS = {\n"
        "    { armyName = \"ARMY_07\", templateIdentifier = \"ucn3008\", x = 1, y = 2, z = 3, scale = 2 },\n"
        "}\n";
    const Io::ScenarioUnitPlacementExtractionResult result =
        Io::ExtractUnitPlacementsFromScenarioScriptText(source, 10);
    Check(result.placements.empty(), "ExtraKey: zero placements extracted");
    Check(result.nearMisses.size() == 1 && result.nearMisses[0].reason.find("scale") != std::string::npos,
          "ExtraKey: reason names the unrecognized field 'scale'");
}

// 5. A non-literal value (an identifier, not a quoted string/number) is a near-miss for that row.
static void TestNonLiteralValueIsNearMiss() {
    const std::string source =
        "local UNIT_PLACEMENTS = {\n"
        "    { armyName = SOME_VARIABLE, templateIdentifier = \"ucn3009\", x = 1, y = 2, z = 3 },\n"
        "}\n";
    const Io::ScenarioUnitPlacementExtractionResult result =
        Io::ExtractUnitPlacementsFromScenarioScriptText(source, 10);
    Check(result.placements.empty(), "NonLiteral: zero placements extracted");
    Check(result.nearMisses.size() == 1, "NonLiteral: exactly one near-miss");
}

// 6. A positional array element belonging to an UNRELATED shape (here, Params::ScenarioSpawnPoint's
//    own SCENARIO_SPAWN_POINTS shape: spawnId/armyName/x/y/z, no templateIdentifier, no armyIndex) is
//    never mistaken for a Shape-3 candidate -- no entry, no near-miss.
static void TestSpawnPointShapedElementNeverMistakenForCandidate() {
    const std::string source =
        "local SCENARIO_SPAWN_POINTS = {\n"
        "    { spawnId = \"North_1v1\", armyName = \"ARMY_01\", x = 10, y = 0, z = 20 },\n"
        "}\n";
    const Io::ScenarioUnitPlacementExtractionResult result =
        Io::ExtractUnitPlacementsFromScenarioScriptText(source, 100);
    Check(result.placements.empty(), "SpawnPointShape: zero placements extracted");
    Check(result.nearMisses.empty(), "SpawnPointShape: no near-misses (not a candidate at all)");
}

// 7. Caps: a synthetic file exceeding kMaxScenarioUnitPlacementExtractionCount stops extraction at the
//    cap and sets the flag, without crashing or unbounded-scanning.
static void TestPlacementCountCapEnforced() {
    std::string source = "local UNIT_PLACEMENTS = {\n";
    for (std::size_t index = 0; index < Io::kMaxScenarioUnitPlacementExtractionCount + 8; ++index) {
        source += "    { armyName = \"ARMY_01\", templateIdentifier = \"ucn3001\", x = 0, y = 0, z = " +
                   std::to_string(index) + " },\n";
    }
    source += "}\n";
    const Io::ScenarioUnitPlacementExtractionResult result =
        Io::ExtractUnitPlacementsFromScenarioScriptText(source, 10000);
    Check(result.placements.size() == Io::kMaxScenarioUnitPlacementExtractionCount, "CountCap: extraction stops at the cap");
    Check(result.bUnitPlacementCountCapExceeded, "CountCap: flag set");
}

// 8. An out-of-range coordinate (exceeding kMaxScenarioAreaCoordinateMagnitude, reused verbatim) is a
//    near-miss, never silently clamped.
static void TestOutOfRangeCoordinateIsNearMiss() {
    const std::string source =
        "local UNIT_PLACEMENTS = {\n"
        "    { armyName = \"ARMY_08\", templateIdentifier = \"ucn3010\", x = 99999999, y = 0, z = 0 },\n"
        "}\n";
    const Io::ScenarioUnitPlacementExtractionResult result =
        Io::ExtractUnitPlacementsFromScenarioScriptText(source, 10);
    Check(result.placements.empty(), "OutOfRange: zero placements extracted");
    Check(result.nearMisses.size() == 1, "OutOfRange: exactly one near-miss");
}

// 9. No Lua execution of any kind: a string value that looks like code is extracted completely
//    inertly, verbatim, never interpreted -- same posture already established across this family
//    (mirrors ScenarioScript_SlotPatternExtract_IO_Test.cpp's own TestNoLuaExecutionOfAnyKind).
static void TestNoLuaExecutionOfAnyKind() {
    const std::string source =
        "local UNIT_PLACEMENTS = {\n"
        "    { armyName = \"os.exit(1)\", templateIdentifier = \"ucn3011\", x = 0, y = 0, z = 0 },\n"
        "}\n";
    const Io::ScenarioUnitPlacementExtractionResult result =
        Io::ExtractUnitPlacementsFromScenarioScriptText(source, 10);
    Check(result.placements.size() == 1 && result.placements[0].armyName == "os.exit(1)",
          "NoExecution: verbatim, never evaluated, process did not exit");
}

int main() {
    TestFullFiveKeyFormExtractsWithIdentityRotation();
    TestFiveKeyPlusRotationFormExtractsRotationVerbatim();
    TestRowMissingRequiredKeyIsNearMissOnlyForThatRow();
    TestRowKeyedByArmyIndexIsRejectedWithExactReason();
    TestZFlipRoundTrip();
    TestPartialRotationTableIsNearMiss();
    TestExtraUnrecognizedKeyIsNearMiss();
    TestNonLiteralValueIsNearMiss();
    TestSpawnPointShapedElementNeverMistakenForCandidate();
    TestPlacementCountCapEnforced();
    TestOutOfRangeCoordinateIsNearMiss();
    TestNoLuaExecutionOfAnyKind();

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
