// ScenarioScript_DataLua_IO_Test.cpp — acceptance test for STEP70: BuildScenarioDataLuaText's
// rendering of Params::Scenarios into the <MapName>_Scenarios_Data.lua companion file text.
// Mirrors MapImporter_Scenarios_IO_Test.cpp's posture (own, self-contained test target).
//
// Includes Sys::CheckLuaSyntax (STEP65) for item 10's self-check ONLY -- production code
// (ScenarioScript_DataLua_IO.cpp) never depends on Sys.
#include "ScenarioScript_DataLua_IO.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../sys/LuaSyntaxCheck_SYS.h"
#include <cstdio>
#include <cstring>
#include <string>

using namespace SanmapGen;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL: %s\n", label); ++failureCount; }
}

// Same "populate every field with distinguishable, non-default values" fixture shape as
// MapImporter_Scenarios_IO_Test.cpp's PopulateFullScenarioBody, adapted to this ticket's needs.
void PopulateFullScenarioBody(Params::ScenarioBody& body, const std::string& name) {
    body.name = name;
    body.area.originX = 1.0f; body.area.originZ = 2.0f;
    body.area.width   = 3.0f; body.area.length  = 4.0f;
    body.spawnsUnits = true;
    body.alloyMode = Params::ScenarioAlloyMode::Delta;

    Params::ScenarioSpawn spawn;
    spawn.armyName = "ArmyOne"; spawn.positionX = 10.0f; spawn.positionY = 1.0f; spawn.positionZ = 100.0f;
    body.spawns.push_back(spawn);

    Params::ScenarioAlloyOverride alloy;
    alloy.armyName = "ArmyOne"; alloy.markerName = "Mass1";
    alloy.positionX = 11.0f; alloy.positionY = 2.0f; alloy.positionZ = 13.0f;
    body.alloys.push_back(alloy);

    Params::ScenarioAlloyOverride alloyToAdd;
    alloyToAdd.armyName = "ArmyTwo"; alloyToAdd.markerName = "Mass2";
    alloyToAdd.positionX = 21.0f; alloyToAdd.positionY = 3.0f; alloyToAdd.positionZ = 17.0f;
    body.alloysToAdd.push_back(alloyToAdd);

    Params::ScenarioAlloyRemoval alloyToRemove;
    alloyToRemove.armyName = "ArmyTwo"; alloyToRemove.markerName = "Mass3";
    body.alloysToRemove.push_back(alloyToRemove);

    body.authoringNote = "Deliberately non-empty authoring note for " + name;
}

// 1. Load-bearing: COUNT_SCENARIOS array order is rendered verbatim (ARCH_15_06 §15.6).
void TestCountScenariosOrderIsLoadBearing() {
    Params::MapRecipe recipe;
    recipe.geometry.mapSize = 512;

    Params::CountScenario zulu;  zulu.body.name  = "Zulu";
    Params::CountScenario alpha; alpha.body.name = "Alpha";
    Params::CountScenario mike;  mike.body.name  = "Mike";
    recipe.scenarios.countScenarios = { zulu, alpha, mike };

    const std::string output = Io::BuildScenarioDataLuaText(recipe);

    const std::size_t zuluPosition  = output.find("\"Zulu\"");
    const std::size_t alphaPosition = output.find("\"Alpha\"");
    const std::size_t mikePosition  = output.find("\"Mike\"");
    Check(zuluPosition != std::string::npos && alphaPosition != std::string::npos
          && mikePosition != std::string::npos, "all three CountScenario names appear in the output");
    Check(zuluPosition < alphaPosition && alphaPosition < mikePosition,
          "countScenarios renders in the vector's own order: Zulu, then Alpha, then Mike");
}

// 2. Global declaration, not local -- and the Scenario global is never declared here (WO6's job).
void TestGlobalDeclarationsNotLocal() {
    Params::MapRecipe recipe;
    const std::string output = Io::BuildScenarioDataLuaText(recipe);

    Check(output.find("PATTERN_SCENARIOS = {") != std::string::npos, "PATTERN_SCENARIOS declared as a global");
    Check(output.find("COUNT_SCENARIOS = {") != std::string::npos, "COUNT_SCENARIOS declared as a global");
    Check(output.find("DEFAULT_SCENARIO = {") != std::string::npos, "DEFAULT_SCENARIO declared as a global");

    Check(output.find("local PATTERN_SCENARIOS") == std::string::npos, "no local PATTERN_SCENARIOS");
    Check(output.find("local COUNT_SCENARIOS") == std::string::npos, "no local COUNT_SCENARIOS");
    Check(output.find("local DEFAULT_SCENARIO") == std::string::npos, "no local DEFAULT_SCENARIO");

    Check(output.find("Scenario = {}") == std::string::npos,
          "this file does not declare the Scenario global (WO6's job)");
    Check(output.find("Scenario.ResolveAndApply") == std::string::npos,
          "this file does not reference Scenario.ResolveAndApply (WO6's job)");

    // STEP73 acceptance item 2: ARMY_ID_TO_NAME / KNOWN_ALLOY_MARKERS are ALSO global, never local.
    Check(output.find("ARMY_ID_TO_NAME = {") != std::string::npos, "ARMY_ID_TO_NAME declared as a global");
    Check(output.find("KNOWN_ALLOY_MARKERS = {") != std::string::npos, "KNOWN_ALLOY_MARKERS declared as a global");
    Check(output.find("local ARMY_ID_TO_NAME") == std::string::npos, "no local ARMY_ID_TO_NAME");
    Check(output.find("local KNOWN_ALLOY_MARKERS") == std::string::npos, "no local KNOWN_ALLOY_MARKERS");
}

// STEP73 item 1: alphabetical-order proof -- armies authored out of order still render in sorted
// [1]/[2]/[3] slot order.
void TestArmyIdToNameAlphabeticalOrder() {
    Params::MapRecipe recipe;
    Params::Army armyThree; armyThree.name = "ARMY_03";
    Params::Army armyOne;   armyOne.name   = "ARMY_01";
    Params::Army armyTwo;   armyTwo.name   = "ARMY_02";
    recipe.armies = { armyThree, armyOne, armyTwo };   // deliberately out of order

    const std::string output = Io::BuildScenarioDataLuaText(recipe);

    const std::size_t slotOnePosition   = output.find("[1] = \"ARMY_01\"");
    const std::size_t slotTwoPosition   = output.find("[2] = \"ARMY_02\"");
    const std::size_t slotThreePosition = output.find("[3] = \"ARMY_03\"");
    Check(slotOnePosition != std::string::npos && slotTwoPosition != std::string::npos
          && slotThreePosition != std::string::npos, "all three ARMY_ID_TO_NAME slots appear in the output");
    Check(slotOnePosition < slotTwoPosition && slotTwoPosition < slotThreePosition,
          "ARMY_ID_TO_NAME renders in alphabetically-sorted order regardless of authored (roster) order");
}

// STEP73 item 3: an empty armies list still renders a present, empty ARMY_ID_TO_NAME table.
void TestEmptyArmiesRendersEmptyArmyIdToNameTable() {
    Params::MapRecipe recipe;   // recipe.armies is default-empty
    const std::string output = Io::BuildScenarioDataLuaText(recipe);

    Check(output.find("ARMY_ID_TO_NAME = {\n}") != std::string::npos,
          "an empty armies roster still renders ARMY_ID_TO_NAME = {\\n}, never omitted");
}

// STEP73 item 4: union-across-tiers, dedup proof -- the same (army, marker) pair appearing in a
// PatternScenario's alloys AND a different CountScenario's alloysToAdd renders exactly once.
void TestKnownAlloyMarkersUnionAcrossTiersDedup() {
    Params::MapRecipe recipe;

    Params::PatternScenario pattern;
    pattern.slotPattern = "OO";
    Params::ScenarioAlloyOverride patternAlloy;
    patternAlloy.armyName = "ARMY_01"; patternAlloy.markerName = "AlloyMarker_X";
    pattern.body.alloys.push_back(patternAlloy);
    recipe.scenarios.patternScenarios.push_back(pattern);

    Params::CountScenario countScenario;
    Params::ScenarioAlloyOverride countAlloyToAdd;
    countAlloyToAdd.armyName = "ARMY_01"; countAlloyToAdd.markerName = "AlloyMarker_X";  // same pair
    countScenario.body.alloysToAdd.push_back(countAlloyToAdd);
    recipe.scenarios.countScenarios.push_back(countScenario);

    const std::string output = Io::BuildScenarioDataLuaText(recipe);

    // Scope the dedup check to the KNOWN_ALLOY_MARKERS block itself -- "AlloyMarker_X" legitimately
    // appears a second time elsewhere in the output too (inside the PATTERN_SCENARIOS/COUNT_SCENARIOS
    // scenario-body alloys/alloysToAdd rows themselves, which are NOT deduplicated -- that is
    // authored source data, not the derived roster).
    const std::size_t knownAlloyMarkersStart = output.find("KNOWN_ALLOY_MARKERS = {");
    const std::size_t knownAlloyMarkersEnd   = output.find("PATTERN_SCENARIOS", knownAlloyMarkersStart);
    Check(knownAlloyMarkersStart != std::string::npos && knownAlloyMarkersEnd != std::string::npos,
          "KNOWN_ALLOY_MARKERS block is present and precedes PATTERN_SCENARIOS");
    const std::string knownAlloyMarkersBlock =
        output.substr(knownAlloyMarkersStart, knownAlloyMarkersEnd - knownAlloyMarkersStart);

    const std::size_t firstOccurrence  = knownAlloyMarkersBlock.find("\"AlloyMarker_X\"");
    const std::size_t secondOccurrence = knownAlloyMarkersBlock.find("\"AlloyMarker_X\"", firstOccurrence + 1);
    Check(firstOccurrence != std::string::npos, "AlloyMarker_X appears at least once inside KNOWN_ALLOY_MARKERS");
    Check(secondOccurrence == std::string::npos,
          "AlloyMarker_X appearing in both a PatternScenario's alloys and a CountScenario's "
          "alloysToAdd is deduplicated to exactly one occurrence in KNOWN_ALLOY_MARKERS");
}

// STEP73 item 5: an army appearing ONLY in defaultScenario.alloysToRemove is still included.
void TestKnownAlloyMarkersAlloysToRemoveOnlyArmyIncluded() {
    Params::MapRecipe recipe;
    Params::ScenarioAlloyRemoval removal;
    removal.armyName = "ARMY_02"; removal.markerName = "AlloyMarker_Removed";
    recipe.scenarios.defaultScenario.alloysToRemove.push_back(removal);

    const std::string output = Io::BuildScenarioDataLuaText(recipe);

    const std::size_t knownAlloyMarkersPosition = output.find("KNOWN_ALLOY_MARKERS = {");
    const std::size_t army02KeyPosition = output.find("[\"ARMY_02\"]");
    Check(knownAlloyMarkersPosition != std::string::npos && army02KeyPosition != std::string::npos
          && army02KeyPosition > knownAlloyMarkersPosition, "[\"ARMY_02\"] appears inside KNOWN_ALLOY_MARKERS");
    Check(output.find("\"AlloyMarker_Removed\"") != std::string::npos,
          "an army referenced ONLY via alloysToRemove still contributes its marker to KNOWN_ALLOY_MARKERS");
}

// STEP73 item 6: an authored army never mentioned in any scenario's alloy fields is absent from
// KNOWN_ALLOY_MARKERS entirely -- not rendered as an empty ["ARMY_0N"] = {},.
void TestKnownAlloyMarkersZeroReferenceArmyAbsent() {
    Params::MapRecipe recipe;
    Params::Army unreferencedArmy; unreferencedArmy.name = "ARMY_09";
    recipe.armies.push_back(unreferencedArmy);   // authored, but never referenced by any alloy row

    const std::string output = Io::BuildScenarioDataLuaText(recipe);

    Check(output.find("[\"ARMY_09\"]") == std::string::npos,
          "an army with zero derived alloy-marker references does not appear in KNOWN_ALLOY_MARKERS at all");
}

// STEP73 item 7: bracket-string key form, never a bare identifier.
void TestKnownAlloyMarkersBracketStringKeyForm() {
    Params::MapRecipe recipe;
    Params::ScenarioAlloyOverride alloy;
    alloy.armyName = "ARMY_01"; alloy.markerName = "AlloyMarker_1";
    recipe.scenarios.defaultScenario.alloys.push_back(alloy);

    const std::string output = Io::BuildScenarioDataLuaText(recipe);

    Check(output.find("[\"ARMY_01\"] = { \"AlloyMarker_1\" },") != std::string::npos,
          "KNOWN_ALLOY_MARKERS renders a bracket-string key, [\"ARMY_01\"] = { ... },");
    Check(output.find("\n    ARMY_01 = {") == std::string::npos,
          "KNOWN_ALLOY_MARKERS never renders a bare-identifier key ARMY_01 = {...}");
}

// STEP73 item 8: live-reference parity self-check -- a fixture hand-transcribing Pandemonium
// Isthmus's real 4-army roster (3 markers each) renders the same army-to-marker-set membership and
// the same 1..4 slot index order as the live hand-authored table (ARMY_01's exact marker set is
// quoted verbatim in STEP73 §0: AlloyMarker_219/237/97; the other three armies' marker names are
// representative -- their real values are not reproduced in this pack -- but the derivation being
// proven (union of alloys/alloysToAdd/alloysToRemove per army, in roster-alphabetical slot order) is
// identical regardless of the literal marker names).
void TestLiveReferenceParitySelfCheck() {
    Params::MapRecipe recipe;

    Params::Army armyOne;   armyOne.name   = "ARMY_01";
    Params::Army armyTwo;   armyTwo.name   = "ARMY_02";
    Params::Army armyThree; armyThree.name = "ARMY_03";
    Params::Army armyFour;  armyFour.name  = "ARMY_04";
    recipe.armies = { armyOne, armyTwo, armyThree, armyFour };

    auto addAlloy = [](Params::ScenarioBody& body, const char* armyName, const char* markerName) {
        Params::ScenarioAlloyOverride alloy;
        alloy.armyName = armyName; alloy.markerName = markerName;
        body.alloys.push_back(alloy);
    };

    // "1v1" pattern scenario -- ARMY_01 and ARMY_02's rosters (ARMY_01's set matches STEP73 §0 verbatim).
    Params::PatternScenario oneVOne;
    oneVOne.slotPattern = "OO";
    addAlloy(oneVOne.body, "ARMY_01", "AlloyMarker_219");
    addAlloy(oneVOne.body, "ARMY_01", "AlloyMarker_237");
    addAlloy(oneVOne.body, "ARMY_01", "AlloyMarker_97");
    addAlloy(oneVOne.body, "ARMY_02", "AlloyMarker_301");
    addAlloy(oneVOne.body, "ARMY_02", "AlloyMarker_302");
    addAlloy(oneVOne.body, "ARMY_02", "AlloyMarker_303");
    recipe.scenarios.patternScenarios.push_back(oneVOne);

    // "4human" count scenario -- ARMY_03's roster.
    Params::CountScenario fourHuman;
    addAlloy(fourHuman.body, "ARMY_03", "AlloyMarker_401");
    addAlloy(fourHuman.body, "ARMY_03", "AlloyMarker_402");
    addAlloy(fourHuman.body, "ARMY_03", "AlloyMarker_403");
    recipe.scenarios.countScenarios.push_back(fourHuman);

    // "1h3ai" count scenario -- ARMY_04's roster.
    Params::CountScenario oneHumanThreeAi;
    addAlloy(oneHumanThreeAi.body, "ARMY_04", "AlloyMarker_501");
    addAlloy(oneHumanThreeAi.body, "ARMY_04", "AlloyMarker_502");
    addAlloy(oneHumanThreeAi.body, "ARMY_04", "AlloyMarker_503");
    recipe.scenarios.countScenarios.push_back(oneHumanThreeAi);

    const std::string output = Io::BuildScenarioDataLuaText(recipe);

    // Same 1..4 slot index order.
    const std::size_t slot1 = output.find("[1] = \"ARMY_01\"");
    const std::size_t slot2 = output.find("[2] = \"ARMY_02\"");
    const std::size_t slot3 = output.find("[3] = \"ARMY_03\"");
    const std::size_t slot4 = output.find("[4] = \"ARMY_04\"");
    Check(slot1 != std::string::npos && slot2 != std::string::npos && slot3 != std::string::npos
          && slot4 != std::string::npos && slot1 < slot2 && slot2 < slot3 && slot3 < slot4,
          "ARMY_ID_TO_NAME renders ARMY_01..ARMY_04 in slots 1..4, matching the live reference's order");

    // Same army -> marker-set membership.
    Check(output.find("[\"ARMY_01\"] = { \"AlloyMarker_219\", \"AlloyMarker_237\", \"AlloyMarker_97\" },")
          != std::string::npos, "ARMY_01's rendered marker set matches STEP73 §0's live-reference quote verbatim");
    Check(output.find("[\"ARMY_02\"]") != std::string::npos, "ARMY_02 present in KNOWN_ALLOY_MARKERS");
    Check(output.find("[\"ARMY_03\"]") != std::string::npos, "ARMY_03 present in KNOWN_ALLOY_MARKERS");
    Check(output.find("[\"ARMY_04\"]") != std::string::npos, "ARMY_04 present in KNOWN_ALLOY_MARKERS");
}

// STEP73 item 9: both new globals render between MAX_ARMY_SLOT_COUNT and PATTERN_SCENARIOS.
void TestArmyRosterPositionInFile() {
    Params::MapRecipe recipe;
    Params::Army army; army.name = "ARMY_01";
    recipe.armies.push_back(army);

    const std::string output = Io::BuildScenarioDataLuaText(recipe);

    const std::size_t maxArmySlotCountPosition = output.find("MAX_ARMY_SLOT_COUNT");
    const std::size_t armyIdToNamePosition     = output.find("ARMY_ID_TO_NAME = {");
    const std::size_t knownAlloyMarkersPosition = output.find("KNOWN_ALLOY_MARKERS = {");
    const std::size_t patternScenariosPosition = output.find("PATTERN_SCENARIOS");

    Check(maxArmySlotCountPosition != std::string::npos && armyIdToNamePosition != std::string::npos
          && knownAlloyMarkersPosition != std::string::npos && patternScenariosPosition != std::string::npos,
          "all four anchors appear in the output");
    Check(maxArmySlotCountPosition < armyIdToNamePosition
          && armyIdToNamePosition < knownAlloyMarkersPosition
          && knownAlloyMarkersPosition < patternScenariosPosition,
          "ARMY_ID_TO_NAME then KNOWN_ALLOY_MARKERS render between MAX_ARMY_SLOT_COUNT and PATTERN_SCENARIOS");
}

// 3. Banner first line exact match.
void TestBannerFirstLineExactMatch() {
    Params::MapRecipe recipe;
    const std::string output = Io::BuildScenarioDataLuaText(recipe);

    const std::size_t bannerLength = std::strlen(Io::kScenarioGeneratedFileBannerLine);
    Check(output.substr(0, bannerLength) == Io::kScenarioGeneratedFileBannerLine,
          "the output's first line is exactly kScenarioGeneratedFileBannerLine");
}

// 4. alloyMode renders all four spellings.
void TestAlloyModeAllFourSpellings() {
    const struct { Params::ScenarioAlloyMode mode; const char* spelling; } cases[] = {
        { Params::ScenarioAlloyMode::Explicit,  "\"explicit\"" },
        { Params::ScenarioAlloyMode::Occupancy, "\"occupancy\"" },
        { Params::ScenarioAlloyMode::KeepAll,   "\"keepAll\"" },
        { Params::ScenarioAlloyMode::Delta,     "\"delta\"" },
    };
    for (const auto& testCase : cases) {
        Params::MapRecipe recipe;
        recipe.scenarios.defaultScenario.alloyMode = testCase.mode;
        const std::string output = Io::BuildScenarioDataLuaText(recipe);
        Check(output.find(std::string("alloyMode = ") + testCase.spelling) != std::string::npos,
              (std::string("alloyMode renders exact spelling ") + testCase.spelling).c_str());
    }
}

// 5. Coordinate flip, deterministic.
void TestCoordinateFlipDeterministic() {
    Params::MapRecipe recipe;
    recipe.geometry.mapSize = 512;

    Params::ScenarioSpawn spawn;
    spawn.armyName = "ArmyOne"; spawn.positionX = 5.0f; spawn.positionY = 6.0f; spawn.positionZ = 100.0f;
    recipe.scenarios.defaultScenario.spawns.push_back(spawn);

    const std::string output = Io::BuildScenarioDataLuaText(recipe);

    Check(output.find("z = 411") != std::string::npos, "512 - 100 - 1 == 411 is rendered for the flipped z");
    Check(output.find("z = 100") == std::string::npos, "the unflipped positionZ value never appears");
    Check(output.find("x = 5") != std::string::npos, "x renders unflipped");
    Check(output.find("y = 6") != std::string::npos, "y renders unflipped");
}

// 6. conditions spellings match STEP69's own tables.
void TestConditionSpellingsMatchStep69() {
    Params::MapRecipe recipe;

    Params::CountScenario countScenario;
    countScenario.body.name = "AllFields";
    countScenario.conditions.push_back(
        { Params::ScenarioCountField::Total, Params::ScenarioComparator::GreaterOrEqual, 2 });
    countScenario.conditions.push_back(
        { Params::ScenarioCountField::HumanCount, Params::ScenarioComparator::Equal, 1 });
    countScenario.conditions.push_back(
        { Params::ScenarioCountField::AiCount, Params::ScenarioComparator::LessThan, 3 });
    recipe.scenarios.countScenarios.push_back(countScenario);

    const std::string output = Io::BuildScenarioDataLuaText(recipe);

    Check(output.find("field = \"Total\"") != std::string::npos, "field spelling Total");
    Check(output.find("field = \"HumanCount\"") != std::string::npos, "field spelling HumanCount");
    Check(output.find("field = \"AiCount\"") != std::string::npos, "field spelling AiCount");
    Check(output.find("comparator = \"GreaterOrEqual\"") != std::string::npos, "comparator spelling GreaterOrEqual");
    Check(output.find("comparator = \"Equal\"") != std::string::npos, "comparator spelling Equal");
    Check(output.find("comparator = \"LessThan\"") != std::string::npos, "comparator spelling LessThan");
}

// STEP204 §8 item 4: spawnsUnits renders in both states, `navy`/`navalFleet` render in neither.
void TestSpawnsUnitsRendersBothStates() {
    for (const bool bSpawnsUnits : { true, false }) {
        Params::MapRecipe recipe;
        recipe.scenarios.defaultScenario.spawnsUnits = bSpawnsUnits;

        const std::string output = Io::BuildScenarioDataLuaText(recipe);

        Check(output.find(std::string("spawnsUnits = ") + (bSpawnsUnits ? "true" : "false")) != std::string::npos,
              bSpawnsUnits ? "spawnsUnits renders true" : "spawnsUnits renders false");
        Check(output.find("navy = ") == std::string::npos, "the retired `navy` key never renders");
        Check(output.find("navalFleet") == std::string::npos, "the retired `navalFleet` table never renders");
    }
}

// 9. Empty Params::Scenarios{} still renders a complete, non-empty file.
void TestEmptyScenariosRendersCompleteFile() {
    Params::MapRecipe recipe;   // default-constructed: Params::Scenarios{}
    const std::string output = Io::BuildScenarioDataLuaText(recipe);

    Check(!output.empty(), "an empty Scenarios still renders a non-empty file");
    Check(output.find("PATTERN_SCENARIOS = {\n}") != std::string::npos,
          "PATTERN_SCENARIOS renders as an empty table, never omitted");
    Check(output.find("COUNT_SCENARIOS = {\n}") != std::string::npos,
          "COUNT_SCENARIOS renders as an empty table, never omitted");
    Check(output.find("DEFAULT_SCENARIO = {") != std::string::npos,
          "DEFAULT_SCENARIO still renders with struct defaults");
    Check(output.find("alloyMode = \"occupancy\"") != std::string::npos,
          "DEFAULT_SCENARIO's default alloyMode (Occupancy) renders as \"occupancy\"");
    Check(output.find("MAX_ARMY_SLOT_COUNT = 16") != std::string::npos,
          "MAX_ARMY_SLOT_COUNT renders the ARCH_15_10 default of 16");
}

// 9b. MAX_ARMY_SLOT_COUNT renders as a bare global, before the three tables.
void TestMaxArmySlotCountBareGlobalBeforeTables() {
    Params::MapRecipe recipe;
    recipe.scenarios.maxArmySlotCount = 8;
    const std::string output = Io::BuildScenarioDataLuaText(recipe);

    Check(output.find("MAX_ARMY_SLOT_COUNT = 8") != std::string::npos, "exact substring MAX_ARMY_SLOT_COUNT = 8");
    const std::size_t maxArmySlotCountPosition = output.find("MAX_ARMY_SLOT_COUNT = 8");
    const std::size_t patternScenariosPosition = output.find("PATTERN_SCENARIOS");
    Check(maxArmySlotCountPosition != std::string::npos && patternScenariosPosition != std::string::npos
          && maxArmySlotCountPosition < patternScenariosPosition,
          "MAX_ARMY_SLOT_COUNT appears before PATTERN_SCENARIOS");
    Check(output.find("local MAX_ARMY_SLOT_COUNT") == std::string::npos, "no local MAX_ARMY_SLOT_COUNT");
    Check(output.find("MAX_ARMY_SLOT_COUNT = 8,") == std::string::npos,
          "no trailing comma after the value -- a global statement, not a table member");
}

// 10. Self-check via STEP65: a fixture covering every field/tier/enum value -- including the STEP73
// ARMY_ID_TO_NAME/KNOWN_ALLOY_MARKERS globals -- is syntactically valid Lua.
void TestSelfCheckViaLuaSyntaxCheck() {
    Params::MapRecipe recipe;
    recipe.mapName = "TestMap";
    recipe.geometry.mapSize = 512;
    recipe.scenarios.maxArmySlotCount = 12;

    Params::Army armyOne; armyOne.name = "ARMY_01";
    Params::Army armyTwo; armyTwo.name = "ARMY_02";
    recipe.armies = { armyOne, armyTwo };

    Params::PatternScenario pattern;
    pattern.slotPattern = "OOOOXXXX";
    PopulateFullScenarioBody(pattern.body, "Pattern One");
    recipe.scenarios.patternScenarios.push_back(pattern);

    Params::CountScenario countScenario;
    PopulateFullScenarioBody(countScenario.body, "Count One");
    countScenario.conditions.push_back(
        { Params::ScenarioCountField::Total, Params::ScenarioComparator::GreaterThan, 2 });
    countScenario.conditions.push_back(
        { Params::ScenarioCountField::HumanCount, Params::ScenarioComparator::NotEqual, 1 });
    recipe.scenarios.countScenarios.push_back(countScenario);

    PopulateFullScenarioBody(recipe.scenarios.defaultScenario, "Default");

    const std::string output = Io::BuildScenarioDataLuaText(recipe);

    Check(output.find("ARMY_ID_TO_NAME = {") != std::string::npos,
          "the syntax-check fixture's output includes ARMY_ID_TO_NAME");
    Check(output.find("KNOWN_ALLOY_MARKERS = {") != std::string::npos,
          "the syntax-check fixture's output includes KNOWN_ALLOY_MARKERS");

    const Sys::LuaSyntaxCheckResult result = Sys::CheckLuaSyntax(output);
    Check(result.bSucceeded, "the renderer's own output is syntactically valid Lua (Sys::CheckLuaSyntax)");
    if (!result.bSucceeded) {
        std::printf("  Lua syntax error at line %d: %s\n", result.lineNumber, result.message.c_str());
    }
}

// STEP209 item 12: hit case -- a resolved areaName renders the NAMED Area's numbers, not the stale
// body.area ones. Proves this Lua leg resolves independently of the JSON leg (§1's discrepancy fix).
void TestAreaNameHitRendersResolvedNumbers() {
    Params::MapRecipe recipe;
    recipe.geometry.mapSize = 512;
    Params::MapArea foo; foo.name = "Foo";
    foo.originX = 5.0f; foo.originZ = 6.0f; foo.width = 7.0f; foo.length = 8.0f;
    recipe.areas.push_back(foo);

    recipe.scenarios.defaultScenario.name = "AreaNameHit";
    recipe.scenarios.defaultScenario.areaName = "Foo";
    recipe.scenarios.defaultScenario.area.originX = 0.0f; recipe.scenarios.defaultScenario.area.originZ = 0.0f;
    recipe.scenarios.defaultScenario.area.width   = 0.0f; recipe.scenarios.defaultScenario.area.length  = 0.0f;

    const std::string output = Io::BuildScenarioDataLuaText(recipe);
    // The area rect renders x/y/width/height (originX/originZ/width/length) with NO z-flip -- unlike
    // spawn/alloy Position fields, Area is a 2D rectangle, not an InstancedTransform position.
    Check(output.find("x = 5") != std::string::npos, "resolved x renders");
    Check(output.find("y = 6") != std::string::npos, "resolved y (== originZ, unflipped) renders");
    Check(output.find("width = 7") != std::string::npos, "resolved width renders");
    Check(output.find("height = 8") != std::string::npos, "resolved height renders");
}

// STEP209 item 13: empty areaName renders body.area verbatim -- regression guard against the
// resolver accidentally engaging when it shouldn't.
void TestAreaNameEmptyRendersBodyAreaVerbatim() {
    Params::MapRecipe recipe;
    recipe.geometry.mapSize = 512;
    recipe.scenarios.defaultScenario.area.originX = 1.0f; recipe.scenarios.defaultScenario.area.originZ = 2.0f;
    recipe.scenarios.defaultScenario.area.width   = 3.0f; recipe.scenarios.defaultScenario.area.length  = 4.0f;

    const std::string output = Io::BuildScenarioDataLuaText(recipe);
    Check(output.find("x = 1") != std::string::npos, "unresolved x renders body.area verbatim");
    Check(output.find("width = 3") != std::string::npos, "unresolved width renders body.area verbatim");
    Check(output.find("height = 4") != std::string::npos, "unresolved height renders body.area verbatim");
}

// STEP209 item 14: stale areaName falls back to body.area, matching the JSON leg's own fallback.
void TestAreaNameStaleFallsBackToBodyArea() {
    Params::MapRecipe recipe;
    recipe.geometry.mapSize = 512;
    recipe.scenarios.defaultScenario.areaName = "DoesNotExist";
    recipe.scenarios.defaultScenario.area.originX = 9.0f; recipe.scenarios.defaultScenario.area.originZ = 10.0f;
    recipe.scenarios.defaultScenario.area.width   = 11.0f; recipe.scenarios.defaultScenario.area.length = 12.0f;

    const std::string output = Io::BuildScenarioDataLuaText(recipe);
    Check(output.find("x = 9") != std::string::npos, "stale areaName: x falls back to body.area");
    Check(output.find("y = 10") != std::string::npos, "stale areaName: y falls back to body.area");
    Check(output.find("width = 11") != std::string::npos, "stale areaName: width falls back to body.area");
    Check(output.find("height = 12") != std::string::npos, "stale areaName: height falls back to body.area");
}

// STEP209 item 15: negative assertion -- the Lua output never contains "areaName"/"AreaName" anywhere,
// confirming the correctly-zero-Lua-field part of ARCH_15_05 stays true even after this ticket's real
// changes to the *numbers* (ARCH_15_05_ParamsScenariosType.md §15.5 AMENDED 2026-08-28).
void TestAreaNameNeverRenderedAsAKey() {
    Params::MapRecipe recipe;
    recipe.areas.push_back(Params::MapArea());
    recipe.scenarios.defaultScenario.areaName = "Foo";
    const std::string output = Io::BuildScenarioDataLuaText(recipe);

    Check(output.find("areaName") == std::string::npos, "no lowerCamelCase \"areaName\" substring anywhere");
    Check(output.find("AreaName") == std::string::npos, "no PascalCase \"AreaName\" substring anywhere");
}

} // namespace

int main() {
    TestCountScenariosOrderIsLoadBearing();
    TestGlobalDeclarationsNotLocal();
    TestBannerFirstLineExactMatch();
    TestAlloyModeAllFourSpellings();
    TestCoordinateFlipDeterministic();
    TestConditionSpellingsMatchStep69();
    TestSpawnsUnitsRendersBothStates();
    TestEmptyScenariosRendersCompleteFile();
    TestMaxArmySlotCountBareGlobalBeforeTables();
    TestArmyIdToNameAlphabeticalOrder();
    TestEmptyArmiesRendersEmptyArmyIdToNameTable();
    TestKnownAlloyMarkersUnionAcrossTiersDedup();
    TestKnownAlloyMarkersAlloysToRemoveOnlyArmyIncluded();
    TestKnownAlloyMarkersZeroReferenceArmyAbsent();
    TestKnownAlloyMarkersBracketStringKeyForm();
    TestLiveReferenceParitySelfCheck();
    TestArmyRosterPositionInFile();
    TestSelfCheckViaLuaSyntaxCheck();
    TestAreaNameHitRendersResolvedNumbers();
    TestAreaNameEmptyRendersBodyAreaVerbatim();
    TestAreaNameStaleFallsBackToBodyArea();
    TestAreaNameNeverRenderedAsAKey();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
