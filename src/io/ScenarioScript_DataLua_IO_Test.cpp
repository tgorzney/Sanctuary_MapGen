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
    body.navy = true;
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

    Params::ScenarioNavalFleetEntry fleetEntry;
    fleetEntry.templateIdentifier = "XSS0201"; fleetEntry.count = 2;
    body.navalFleet.fleet.push_back(fleetEntry);

    Params::ScenarioNavalPondAssignment pondAssignment;
    pondAssignment.armyName = "ArmyOne"; pondAssignment.side = Params::ScenarioNavalPondSide::West;
    body.navalFleet.pondSideByArmy.push_back(pondAssignment);

    body.navalFleet.sideBiasDistance = 90.0f;
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

// 7. navalFleet always emitted, even when navy == false.
void TestNavalFleetAlwaysEmitted() {
    Params::MapRecipe recipe;
    recipe.scenarios.defaultScenario.navy = false;   // deliberately false
    Params::ScenarioNavalFleetEntry fleetEntry;
    fleetEntry.templateIdentifier = "XSS0201"; fleetEntry.count = 2;
    recipe.scenarios.defaultScenario.navalFleet.fleet.push_back(fleetEntry);

    const std::string output = Io::BuildScenarioDataLuaText(recipe);

    Check(output.find("navy = false") != std::string::npos, "navy renders false");
    Check(output.find("navalFleet = {") != std::string::npos, "navalFleet block is still emitted");
    Check(output.find("fleet = {") != std::string::npos, "fleet sub-table is still emitted");
    Check(output.find("pondSideByArmy = {") != std::string::npos, "pondSideByArmy sub-table is still emitted");
    Check(output.find("sideBiasDistance =") != std::string::npos, "sideBiasDistance is still emitted");
}

// 8. ScenarioNavalPondSide renders as a raw signed integer, never a quoted string.
void TestPondSideRawSignedInteger() {
    Params::MapRecipe recipe;
    Params::ScenarioNavalPondAssignment assignment;
    assignment.armyName = "ArmyOne"; assignment.side = Params::ScenarioNavalPondSide::West;
    recipe.scenarios.defaultScenario.navalFleet.pondSideByArmy.push_back(assignment);

    const std::string output = Io::BuildScenarioDataLuaText(recipe);

    Check(output.find("side = -1") != std::string::npos, "West renders as raw side = -1");
    Check(output.find("side = \"West\"") == std::string::npos, "side never renders as a quoted enum spelling");
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

// 10. Self-check via STEP65: a fixture covering every field/tier/enum value is syntactically valid Lua.
void TestSelfCheckViaLuaSyntaxCheck() {
    Params::MapRecipe recipe;
    recipe.mapName = "TestMap";
    recipe.geometry.mapSize = 512;
    recipe.scenarios.maxArmySlotCount = 12;

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
    const Sys::LuaSyntaxCheckResult result = Sys::CheckLuaSyntax(output);
    Check(result.bSucceeded, "the renderer's own output is syntactically valid Lua (Sys::CheckLuaSyntax)");
    if (!result.bSucceeded) {
        std::printf("  Lua syntax error at line %d: %s\n", result.lineNumber, result.message.c_str());
    }
}

} // namespace

int main() {
    TestCountScenariosOrderIsLoadBearing();
    TestGlobalDeclarationsNotLocal();
    TestBannerFirstLineExactMatch();
    TestAlloyModeAllFourSpellings();
    TestCoordinateFlipDeterministic();
    TestConditionSpellingsMatchStep69();
    TestNavalFleetAlwaysEmitted();
    TestPondSideRawSignedInteger();
    TestEmptyScenariosRendersCompleteFile();
    TestMaxArmySlotCountBareGlobalBeforeTables();
    TestSelfCheckViaLuaSyntaxCheck();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
