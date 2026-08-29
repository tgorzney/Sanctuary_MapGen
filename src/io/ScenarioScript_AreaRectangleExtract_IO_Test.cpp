// ScenarioScript_AreaRectangleExtract_IO_Test.cpp -- pure-logic acceptance test for the closed
// literal-only grammar (STEP215, ARCH §15.11 items 2-7,10). No filesystem, no disk, matches the
// header's own "pure and total" contract.
#include "ScenarioScript_AreaRectangleExtract_IO.h"
#include <cmath>
#include <cstdio>
#include <string>

using namespace SanmapGen;

static int failureCount = 0;

static void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failureCount; }
}

static bool NearFloat(float actual, float expected) { return std::fabs(actual - expected) < 0.001f; }

static const Params::MapArea* FindAreaByName(const Io::ScenarioAreaExtractionResult& result, const char* name) {
    for (const Params::MapArea& area : result.areas) if (area.name == name) return &area;
    return nullptr;
}

// 1. The real, byte-verbatim line from map_scripts_backup/Pandemonium Isthmus_Scenarios_Script.lua
//    .officialbak:61 (named-key form, all-integer values, a trailing "--" comment after the '}').
static void TestNamedKeyFormRealFixtureLine() {
    const std::string source =
        "local AREA_356 = { x = 846, y = 846, width = 356, height = 356 }               "
        "-- the map's own baked default\n";
    const Io::ScenarioAreaExtractionResult result = Io::ExtractAreaRectanglesFromScenarioScriptText(source);
    Check(result.areas.size() == 1, "NamedKeyRealFixture: exactly one area");
    Check(result.nearMisses.empty(), "NamedKeyRealFixture: no near-misses");
    const Params::MapArea* area = FindAreaByName(result, "AREA_356");
    Check(area != nullptr, "NamedKeyRealFixture: AREA_356 present");
    if (area != nullptr) {
        Check(NearFloat(area->originX, 846.0f), "NamedKeyRealFixture: originX");
        Check(NearFloat(area->originZ, 846.0f), "NamedKeyRealFixture: originZ (y->originZ mapping)");
        Check(NearFloat(area->width, 356.0f), "NamedKeyRealFixture: width");
        Check(NearFloat(area->length, 356.0f), "NamedKeyRealFixture: length (height->length mapping)");
    }
}

// 2. The real line :62 -- fractional (non-integer) float values, out-of-x/y/width/height order in
//    the source (x, y, width, height IS the declared order here, so this also exercises the "keys
//    in any order" clause via a THIRD synthetic case below rather than this one).
static void TestNamedKeyFormRealFractionalValues() {
    const std::string source =
        "local AREA_169 = { x = 668.4444444444445, y = 824, width = 711.1111111111111, height = 400 } "
        "-- 400 height, 16:9\n";
    const Io::ScenarioAreaExtractionResult result = Io::ExtractAreaRectanglesFromScenarioScriptText(source);
    const Params::MapArea* area = FindAreaByName(result, "AREA_169");
    Check(area != nullptr, "NamedKeyFractional: AREA_169 present");
    if (area != nullptr) {
        Check(NearFloat(area->originX, 668.4444444444445f), "NamedKeyFractional: originX");
        Check(NearFloat(area->originZ, 824.0f), "NamedKeyFractional: originZ");
        Check(NearFloat(area->width, 711.1111111111111f), "NamedKeyFractional: width");
        Check(NearFloat(area->length, 400.0f), "NamedKeyFractional: length");
    }
}

// 3. Keys in an order OTHER than x,y,width,height (item 4: "either all four keyed pairs ... in any
//    order").
static void TestNamedKeyFormOutOfOrderKeys() {
    const std::string source = "local AREA_ORDER = { height = 200, width = 100, y = 50, x = 25 }\n";
    const Io::ScenarioAreaExtractionResult result = Io::ExtractAreaRectanglesFromScenarioScriptText(source);
    const Params::MapArea* area = FindAreaByName(result, "AREA_ORDER");
    Check(area != nullptr, "OutOfOrderKeys: area present");
    if (area != nullptr) {
        Check(NearFloat(area->originX, 25.0f), "OutOfOrderKeys: originX");
        Check(NearFloat(area->originZ, 50.0f), "OutOfOrderKeys: originZ");
        Check(NearFloat(area->width, 100.0f), "OutOfOrderKeys: width");
        Check(NearFloat(area->length, 200.0f), "OutOfOrderKeys: length");
    }
}

// 4. Positional form (item 4's second accepted shape). NOT present in any real file scanned for
//    this ticket -- a synthetic fixture exercising the closed grammar's own second accepted branch,
//    values chosen to match TestNamedKeyFormRealFixtureLine's AREA_356 for an easy cross-check.
static void TestPositionalFormSynthetic() {
    const std::string source = "local AREA_POS = { 846, 846, 356, 356 }\n";
    const Io::ScenarioAreaExtractionResult result = Io::ExtractAreaRectanglesFromScenarioScriptText(source);
    const Params::MapArea* area = FindAreaByName(result, "AREA_POS");
    Check(area != nullptr, "PositionalSynthetic: area present");
    if (area != nullptr) {
        Check(NearFloat(area->originX, 846.0f), "PositionalSynthetic: originX from position 1 (x)");
        Check(NearFloat(area->originZ, 846.0f), "PositionalSynthetic: originZ from position 2 (y)");
        Check(NearFloat(area->width, 356.0f), "PositionalSynthetic: width from position 3");
        Check(NearFloat(area->length, 356.0f), "PositionalSynthetic: length from position 4");
    }
}

// 5. A negative-signed value (item 4: "optional sign").
static void TestNegativeSignedValue() {
    const std::string source = "local AREA_NEG = { x = -50, y = 10, width = 100, height = 100 }\n";
    const Io::ScenarioAreaExtractionResult result = Io::ExtractAreaRectanglesFromScenarioScriptText(source);
    const Params::MapArea* area = FindAreaByName(result, "AREA_NEG");
    Check(area != nullptr, "NegativeSigned: area present");
    if (area != nullptr) Check(NearFloat(area->originX, -50.0f), "NegativeSigned: originX is -50");
}

// 6. Line comments (`--`) hide a fake area entirely (item 5).
static void TestLineCommentIsSkipped() {
    const std::string source =
        "-- local AREA_FAKE = { x=1,y=1,width=1,height=1 }\n"
        "local AREA_REAL = { x = 5, y = 5, width = 5, height = 5 }\n";
    const Io::ScenarioAreaExtractionResult result = Io::ExtractAreaRectanglesFromScenarioScriptText(source);
    Check(FindAreaByName(result, "AREA_FAKE") == nullptr, "LineComment: commented area not extracted");
    Check(FindAreaByName(result, "AREA_REAL") != nullptr, "LineComment: real area still extracted");
}

// 7. A `--[[ ]]` long comment block hides a fake area entirely (item 5).
static void TestLongCommentIsSkipped() {
    const std::string source =
        "--[[ local AREA_FAKE2 = { x=1, y=1, width=1, height=1 } ]]\n"
        "local AREA_REAL2 = { x = 6, y = 6, width = 6, height = 6 }\n";
    const Io::ScenarioAreaExtractionResult result = Io::ExtractAreaRectanglesFromScenarioScriptText(source);
    Check(FindAreaByName(result, "AREA_FAKE2") == nullptr, "LongComment: commented area not extracted");
    Check(FindAreaByName(result, "AREA_REAL2") != nullptr, "LongComment: real area still extracted");
}

// 8. A string literal containing `{`/`}` and an `=` sign must not confuse the parser (item 5).
static void TestStringLiteralWithBracesDoesNotConfuseParser() {
    const std::string source =
        "local LABEL = \"some {weird} text with = signs and a fake area = { x=1,y=1,width=1,height=1 }\"\n"
        "local AREA_REAL3 = { x = 7, y = 7, width = 7, height = 7 }\n";
    const Io::ScenarioAreaExtractionResult result = Io::ExtractAreaRectanglesFromScenarioScriptText(source);
    Check(FindAreaByName(result, "AREA_REAL3") != nullptr, "StringBraces: real area after string extracted");
    // LABEL itself is a near-miss (its RHS is a string, not `{`), never an area -- confirms the
    // string's own content was never mistaken for the start of a table.
    Check(FindAreaByName(result, "LABEL") == nullptr, "StringBraces: LABEL itself never became an area");
}

// 9. In-file identifier collision resolves last-write-wins, and is logged (item 7).
static void TestInFileCollisionLastWriteWins() {
    const std::string source =
        "local AREA_X = { x = 1, y = 1, width = 1, height = 1 }\n"
        "local AREA_X = { x = 2, y = 2, width = 2, height = 2 }\n";
    const Io::ScenarioAreaExtractionResult result = Io::ExtractAreaRectanglesFromScenarioScriptText(source);
    Check(result.areas.size() == 1, "InFileCollision: exactly one AREA_X survives");
    Check(result.collisionIdentifiers.size() == 1 && result.collisionIdentifiers[0] == "AREA_X",
          "InFileCollision: collision logged by identifier");
    const Params::MapArea* area = FindAreaByName(result, "AREA_X");
    if (area != nullptr) Check(NearFloat(area->originX, 2.0f), "InFileCollision: second assignment wins");
}

// 10. A nested table is rejected outright (item 4).
static void TestNestedTableRejected() {
    const std::string source = "local AREA_BAD = { x = 1, y = 1, width = { 1 }, height = 1 }\n";
    const Io::ScenarioAreaExtractionResult result = Io::ExtractAreaRectanglesFromScenarioScriptText(source);
    Check(result.areas.empty(), "NestedTable: no area extracted");
    Check(result.nearMisses.size() == 1 && result.nearMisses[0].identifier == "AREA_BAD",
          "NestedTable: near-miss logged for AREA_BAD");
}

// 11. Mixed keyed/positional in one table is rejected (item 4).
static void TestMixedKeyedPositionalRejected() {
    const std::string source = "local AREA_BAD2 = { x = 1, 2, width = 3, height = 4 }\n";
    const Io::ScenarioAreaExtractionResult result = Io::ExtractAreaRectanglesFromScenarioScriptText(source);
    Check(result.areas.empty(), "MixedForm: no area extracted");
    Check(!result.nearMisses.empty() && result.nearMisses[0].identifier == "AREA_BAD2", "MixedForm: near-miss logged");
}

// 12. A missing key (only three fields) is rejected (item 4).
static void TestMissingFieldRejected() {
    const std::string source = "local AREA_BAD3 = { x = 1, y = 1, width = 1 }\n";
    const Io::ScenarioAreaExtractionResult result = Io::ExtractAreaRectanglesFromScenarioScriptText(source);
    Check(result.areas.empty(), "MissingField: no area extracted");
}

// 13. A duplicate key within one table is rejected (item 4).
static void TestDuplicateFieldRejected() {
    const std::string source = "local AREA_BAD4 = { x = 1, x = 2, width = 1, height = 1 }\n";
    const Io::ScenarioAreaExtractionResult result = Io::ExtractAreaRectanglesFromScenarioScriptText(source);
    Check(result.areas.empty(), "DuplicateField: no area extracted");
}

// 14. An unrecognized fifth-key-shaped field (right count, wrong key) is rejected (item 4).
static void TestUnrecognizedFieldRejected() {
    const std::string source = "local AREA_BAD5 = { x = 1, y = 1, w = 1, height = 1 }\n";
    const Io::ScenarioAreaExtractionResult result = Io::ExtractAreaRectanglesFromScenarioScriptText(source);
    Check(result.areas.empty(), "UnrecognizedField: no area extracted");
}

// 15. An absurd coordinate magnitude and a non-positive extent are both rejected (item 10).
static void TestAbsurdCoordinateAndNonPositiveExtentRejected() {
    const std::string sourceAbsurd = "local AREA_BAD6 = { x = 99999999, y = 1, width = 1, height = 1 }\n";
    Check(Io::ExtractAreaRectanglesFromScenarioScriptText(sourceAbsurd).areas.empty(),
          "AbsurdCoordinate: no area extracted");
    const std::string sourceZeroWidth = "local AREA_BAD7 = { x = 1, y = 1, width = 0, height = 1 }\n";
    Check(Io::ExtractAreaRectanglesFromScenarioScriptText(sourceZeroWidth).areas.empty(),
          "ZeroWidth: no area extracted");
}

// 16. The rectangle-count cap halts extraction (item 10) -- build 520 distinct valid area
//     assignments (> kMaxScenarioAreaExtractionRectangleCount == 512) and confirm the cap bites.
static void TestRectangleCountCapEnforced() {
    std::string source;
    for (int index = 0; index < 520; ++index) {
        source += "local AREA_GEN_" + std::to_string(index) + " = { x = 1, y = 1, width = 1, height = 1 }\n";
    }
    const Io::ScenarioAreaExtractionResult result = Io::ExtractAreaRectanglesFromScenarioScriptText(source);
    Check(result.areas.size() == Io::kMaxScenarioAreaExtractionRectangleCount, "CountCap: extraction stops at the cap");
    Check(result.bRectangleCountCapExceeded, "CountCap: flag set");
}

// 17. A real multi-area block from the reference file (:61-64,68-69), verbatim, including two
//     genuinely-present non-rectangle tables (IDENTITY_ROTATION has a 'w' key + only 4 fields but
//     wrong keys; IDENTITY_SCALE has only 3 fields) that MUST be rejected as near-misses, never
//     imported -- proving item 2's "no other kind of value may ever be extracted" holds structurally,
//     not just by policy, even against real adjacent-in-file content shaped almost like a rectangle.
static void TestRealMultiAreaBlockFromReferenceFile() {
    const std::string source =
        "-- format: {x, y = world z, width, height}.\n"
        "local AREA_356 = { x = 846, y = 846, width = 356, height = 356 }               -- the map's own baked default\n"
        "local AREA_169 = { x = 668.4444444444445, y = 824, width = 711.1111111111111, height = 400 } -- 400 height, 16:9\n"
        "local AREA_1024 = { x = 537, y = 472, width = 974, height = 1104 }             -- 6-player, centered on map center\n"
        "local AREA_FULL = { x = 0, y = 0, width = 2048, height = 2048 }\n"
        "local IDENTITY_ROTATION = { w = 1.0, x = 0.0, y = 0.0, z = 0.0 }\n"
        "local IDENTITY_SCALE = { x = 1.0, y = 1.0, z = 1.0 }\n";
    const Io::ScenarioAreaExtractionResult result = Io::ExtractAreaRectanglesFromScenarioScriptText(source);
    Check(result.areas.size() == 4, "RealBlock: exactly the four real rectangles extracted");
    Check(FindAreaByName(result, "AREA_356") != nullptr, "RealBlock: AREA_356 present");
    Check(FindAreaByName(result, "AREA_169") != nullptr, "RealBlock: AREA_169 present");
    Check(FindAreaByName(result, "AREA_1024") != nullptr, "RealBlock: AREA_1024 present");
    Check(FindAreaByName(result, "AREA_FULL") != nullptr, "RealBlock: AREA_FULL present");
    Check(FindAreaByName(result, "IDENTITY_ROTATION") == nullptr, "RealBlock: IDENTITY_ROTATION never imported");
    Check(FindAreaByName(result, "IDENTITY_SCALE") == nullptr, "RealBlock: IDENTITY_SCALE never imported");
    Check(result.nearMisses.size() == 2, "RealBlock: exactly two near-misses logged");
}

// 18. A spawn-shaped 3-field x/y/z table (the real ARMY_01 shape, §15.11 item 2's own named concern)
//     is structurally excluded -- wrong field count AND wrong key set, never imported.
static void TestSpawnShapedTableNeverImported() {
    const std::string source = "ARMY_01 = { x = 855, y = 79.12979888916016, z = 920 }\n";
    const Io::ScenarioAreaExtractionResult result = Io::ExtractAreaRectanglesFromScenarioScriptText(source);
    Check(result.areas.empty(), "SpawnShaped: never imported as an area (item 2)");
}

int main() {
    TestNamedKeyFormRealFixtureLine();
    TestNamedKeyFormRealFractionalValues();
    TestNamedKeyFormOutOfOrderKeys();
    TestPositionalFormSynthetic();
    TestNegativeSignedValue();
    TestLineCommentIsSkipped();
    TestLongCommentIsSkipped();
    TestStringLiteralWithBracesDoesNotConfuseParser();
    TestInFileCollisionLastWriteWins();
    TestNestedTableRejected();
    TestMixedKeyedPositionalRejected();
    TestMissingFieldRejected();
    TestDuplicateFieldRejected();
    TestUnrecognizedFieldRejected();
    TestAbsurdCoordinateAndNonPositiveExtentRejected();
    TestRectangleCountCapEnforced();
    TestRealMultiAreaBlockFromReferenceFile();
    TestSpawnShapedTableNeverImported();

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
