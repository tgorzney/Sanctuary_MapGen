// MapImporter_Scenarios_IO_Test.cpp — acceptance test for STEP69_ParamsScenariosRoundTrip_IO's
// pure JSON round-trip (`BuildScenariosJson`/`ReadScenariosJson`) plus one live-document check
// through `MapExporter::BuildSanmapJsonText`/`MapImporter::ParseSanmapJsonText`. Mirrors
// MapImporter_PropsDecals_IO_Test.cpp's posture (own, self-contained test target).
#include "MapExporter_IO.h"
#include "MapExporter_Recipe_IO.h"
#include "MapImporter_IO.h"
#include "MapImporter_Recipe_IO.h"
#include "../params/MapRecipe_PARAMS.h"
#include <cmath>
#include <cstdio>
#include <nlohmann/json.hpp>
#include <string>

using namespace SanmapGen;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL: %s\n", label); ++failureCount; }
}

bool NearlyEqual(float left, float right) { return std::fabs(left - right) <= 1.0e-4f; }

// Populates every field of a ScenarioBody with distinguishable, non-default values — used by both
// the PatternScenario and the CountScenario/DefaultScenario fixtures in the "one of each tier"
// full round-trip test.
void PopulateFullScenarioBody(Params::ScenarioBody& body, const std::string& name) {
    body.name = name;
    body.area.originX = 1.0f; body.area.originZ = 2.0f;
    body.area.width   = 3.0f; body.area.length  = 4.0f;
    body.spawnsUnits = true;
    body.alloyMode = Params::ScenarioAlloyMode::Delta;

    Params::ScenarioSpawn spawn;
    spawn.armyName = "ArmyOne"; spawn.positionX = 10.0f; spawn.positionY = 1.0f; spawn.positionZ = 7.0f;
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

void CheckScenarioBodyEquals(const Params::ScenarioBody& original, const Params::ScenarioBody& loaded,
                             const std::string& label) {
    Check(loaded.name == original.name, (label + " name survives").c_str());
    Check(NearlyEqual(loaded.area.originX, original.area.originX)
          && NearlyEqual(loaded.area.originZ, original.area.originZ)
          && NearlyEqual(loaded.area.width, original.area.width)
          && NearlyEqual(loaded.area.length, original.area.length),
          (label + " area {x,y,width,height} survives").c_str());
    Check(loaded.spawnsUnits == original.spawnsUnits, (label + " spawnsUnits survives").c_str());
    Check(loaded.alloyMode == original.alloyMode, (label + " alloyMode survives").c_str());

    Check(loaded.spawns.size() == original.spawns.size() && loaded.spawns.size() == 1,
          (label + " one spawn survives").c_str());
    if (loaded.spawns.size() == 1) {
        Check(loaded.spawns[0].armyName == original.spawns[0].armyName
              && NearlyEqual(loaded.spawns[0].positionX, original.spawns[0].positionX)
              && NearlyEqual(loaded.spawns[0].positionY, original.spawns[0].positionY)
              && NearlyEqual(loaded.spawns[0].positionZ, original.spawns[0].positionZ),
              (label + " spawn fields survive, including the Z coordinate flip round trip").c_str());
    }

    Check(loaded.alloys.size() == original.alloys.size() && loaded.alloys.size() == 1,
          (label + " one alloy override survives").c_str());
    Check(loaded.alloysToAdd.size() == original.alloysToAdd.size() && loaded.alloysToAdd.size() == 1,
          (label + " one alloysToAdd entry survives").c_str());
    if (loaded.alloysToAdd.size() == 1) {
        Check(NearlyEqual(loaded.alloysToAdd[0].positionZ, original.alloysToAdd[0].positionZ),
              (label + " alloysToAdd Z coordinate flip round trips").c_str());
    }
    Check(loaded.alloysToRemove.size() == original.alloysToRemove.size()
          && loaded.alloysToRemove.size() == 1,
          (label + " one alloysToRemove entry survives").c_str());

    Check(loaded.authoringNote == original.authoringNote, (label + " authoringNote survives").c_str());
}

Params::MapRecipe BuildFixtureRecipe() {
    Params::MapRecipe recipe;
    recipe.geometry.mapSize = 512;

    Params::PatternScenario pattern;
    pattern.slotPattern = "OOOOXXXXOOOOXXXX";
    PopulateFullScenarioBody(pattern.body, "Pattern One");
    recipe.scenarios.patternScenarios.push_back(pattern);

    // TIER 2 order-preservation fixture: 3 entries, deliberately non-alphabetical. "Alpha" also
    // doubles as the full-field-coverage CountScenario (3 AND'd conditions spanning all 3
    // ScenarioCountField values and 3 distinct ScenarioComparator values).
    Params::CountScenario zulu;  zulu.body.name  = "Zulu";
    Params::CountScenario alpha; PopulateFullScenarioBody(alpha.body, "Alpha");
    alpha.conditions.push_back({ Params::ScenarioCountField::Total, Params::ScenarioComparator::GreaterThan, 2 });
    alpha.conditions.push_back({ Params::ScenarioCountField::HumanCount, Params::ScenarioComparator::Equal, 1 });
    alpha.conditions.push_back({ Params::ScenarioCountField::AiCount, Params::ScenarioComparator::LessOrEqual, 3 });
    Params::CountScenario mike;  mike.body.name  = "Mike";
    recipe.scenarios.countScenarios = { zulu, alpha, mike };

    PopulateFullScenarioBody(recipe.scenarios.defaultScenario, "Default");
    recipe.scenarios.maxArmySlotCount = 20;   // non-default, exercises the round trip too

    return recipe;
}

// 1. Load-bearing: CountScenarios order round-trips exactly.
// 5. Full field round trip, one of each tier.
// 6. Empty countScenarios/patternScenarios still serialize as [], not omitted or {}.
void RunPureRoundTripTests() {
    const Params::MapRecipe original = BuildFixtureRecipe();

    nlohmann::ordered_json scenariosJson = Io::BuildScenariosJson(original);
    Check(scenariosJson["PatternScenarios"].is_array() && scenariosJson["PatternScenarios"].size() == 1,
          "PatternScenarios is an array with one entry");
    Check(scenariosJson["CountScenarios"].is_array() && scenariosJson["CountScenarios"].size() == 3,
          "CountScenarios is an array with three entries");

    nlohmann::ordered_json document;
    document["Scenarios"] = scenariosJson;

    Params::MapRecipe loaded;
    loaded.geometry.mapSize = original.geometry.mapSize;
    Io::MapImportResult result;
    Io::ReadScenariosJson(document, loaded, result);

    // --- Item 1: CountScenarios order-preservation (the load-bearing check). ---------------------
    Check(loaded.scenarios.countScenarios.size() == 3, "all three CountScenarios entries survive");
    if (loaded.scenarios.countScenarios.size() == 3) {
        Check(loaded.scenarios.countScenarios[0].body.name == "Zulu", "CountScenarios[0] == Zulu");
        Check(loaded.scenarios.countScenarios[1].body.name == "Alpha", "CountScenarios[1] == Alpha");
        Check(loaded.scenarios.countScenarios[2].body.name == "Mike", "CountScenarios[2] == Mike");
    }

    // --- Item 5: full field round trip, one of each tier. ----------------------------------------
    Check(loaded.scenarios.patternScenarios.size() == 1, "one PatternScenario survives");
    if (loaded.scenarios.patternScenarios.size() == 1) {
        Check(loaded.scenarios.patternScenarios[0].slotPattern == original.scenarios.patternScenarios[0].slotPattern,
              "PatternScenario::slotPattern survives");
        CheckScenarioBodyEquals(original.scenarios.patternScenarios[0].body,
                                loaded.scenarios.patternScenarios[0].body, "PatternScenario");
    }
    if (loaded.scenarios.countScenarios.size() == 3) {
        const Params::CountScenario& originalAlpha = original.scenarios.countScenarios[1];
        const Params::CountScenario& loadedAlpha   = loaded.scenarios.countScenarios[1];
        CheckScenarioBodyEquals(originalAlpha.body, loadedAlpha.body, "CountScenario[Alpha]");
        Check(loadedAlpha.conditions.size() == 3, "CountScenario[Alpha] keeps all 3 conditions");
        if (loadedAlpha.conditions.size() == 3) {
            Check(loadedAlpha.conditions[0].field == Params::ScenarioCountField::Total
                  && loadedAlpha.conditions[0].comparator == Params::ScenarioComparator::GreaterThan
                  && loadedAlpha.conditions[0].value == 2, "condition[0] survives exactly");
            Check(loadedAlpha.conditions[1].field == Params::ScenarioCountField::HumanCount
                  && loadedAlpha.conditions[1].comparator == Params::ScenarioComparator::Equal
                  && loadedAlpha.conditions[1].value == 1, "condition[1] survives exactly");
            Check(loadedAlpha.conditions[2].field == Params::ScenarioCountField::AiCount
                  && loadedAlpha.conditions[2].comparator == Params::ScenarioComparator::LessOrEqual
                  && loadedAlpha.conditions[2].value == 3, "condition[2] survives exactly");
        }
    }
    CheckScenarioBodyEquals(original.scenarios.defaultScenario, loaded.scenarios.defaultScenario, "DefaultScenario");
    Check(loaded.scenarios.maxArmySlotCount == 20, "MaxArmySlotCount round trips (non-default value)");

    // --- Item 6: an empty scenarios set still serializes PatternScenarios/CountScenarios as []. --
    Params::MapRecipe emptyRecipe;
    nlohmann::ordered_json emptyJson = Io::BuildScenariosJson(emptyRecipe);
    Check(emptyJson["PatternScenarios"].is_array() && emptyJson["PatternScenarios"].empty(),
          "an empty patternScenarios still serializes as []");
    Check(emptyJson["CountScenarios"].is_array() && emptyJson["CountScenarios"].empty(),
          "an empty countScenarios still serializes as []");
}

// 2. Absent Scenarios key -> Params::Scenarios{} defaults, zero warnings.
void RunAbsentKeyDefaultsTest() {
    nlohmann::ordered_json document;   // no "Scenarios" key at all
    Params::MapRecipe loaded;
    Io::MapImportResult result;
    Io::ReadScenariosJson(document, loaded, result);

    Check(loaded.scenarios.patternScenarios.empty(), "absent Scenarios: patternScenarios stays empty");
    Check(loaded.scenarios.countScenarios.empty(), "absent Scenarios: countScenarios stays empty");
    Check(loaded.scenarios.defaultScenario.alloyMode == Params::ScenarioAlloyMode::Occupancy,
          "absent Scenarios: defaultScenario.alloyMode stays Occupancy");
    Check(loaded.scenarios.maxArmySlotCount == 16, "absent Scenarios: maxArmySlotCount stays 16");
    Check(result.warningCount == 0, "absent Scenarios key logs zero warnings");
}

// 3. PatternScenarios/CountScenarios present but a JSON object, not array -> warn, treated empty.
void RunMalformedArrayWarningTest() {
    nlohmann::ordered_json document;
    document["Scenarios"]["PatternScenarios"] = nlohmann::json::object();
    document["Scenarios"]["CountScenarios"]   = nlohmann::json::object();

    Params::MapRecipe loaded;
    Io::MapImportResult result;
    Io::ReadScenariosJson(document, loaded, result);

    Check(loaded.scenarios.patternScenarios.empty(), "PatternScenarios-as-object: treated as empty");
    Check(loaded.scenarios.countScenarios.empty(), "CountScenarios-as-object: treated as empty");
    Check(result.warningCount >= 2, "both malformed arrays are logged as warnings");
    Check(result.debugLog.find("order") != std::string::npos,
          "the logged message names ordering (proves it's the ordering-hazard warning)");
}

// 4. AlloyMode absent from a <ScenarioRecord> -> reads back as Occupancy, zero warnings for that.
void RunAlloyModeAbsentDefaultsTest() {
    nlohmann::ordered_json document;
    nlohmann::ordered_json defaultScenario;
    defaultScenario["Name"] = "NoAlloyMode";   // deliberately no "AlloyMode" key
    document["Scenarios"]["DefaultScenario"] = defaultScenario;

    Params::MapRecipe loaded;
    Io::MapImportResult result;
    Io::ReadScenariosJson(document, loaded, result);

    Check(loaded.scenarios.defaultScenario.alloyMode == Params::ScenarioAlloyMode::Occupancy,
          "absent AlloyMode reads back as Occupancy");
    Check(result.warningCount == 0, "absent AlloyMode alone logs no warning");
}

// STEP204 §8 item 1: spawnsUnits round-trips true AND false through .sanmap export->import.
void RunSpawnsUnitsRoundTripBothStatesTest() {
    for (const bool bSpawnsUnits : { true, false }) {
        Params::MapRecipe recipe;
        recipe.geometry.mapSize = 512;
        recipe.scenarios.defaultScenario.name = "SpawnsUnitsCase";
        recipe.scenarios.defaultScenario.spawnsUnits = bSpawnsUnits;

        const std::string documentText = Io::MapExporter::BuildSanmapJsonText(recipe);
        Params::MapRecipe loaded;
        Io::MapImportOptions options;
        Io::MapImportResult result;
        const bool bParsed = Io::MapImporter::ParseSanmapJsonText(documentText, loaded, options, result);
        Check(bParsed, "spawnsUnits round trip: document parses");
        Check(loaded.scenarios.defaultScenario.spawnsUnits == bSpawnsUnits,
              bSpawnsUnits ? "spawnsUnits = true round trips" : "spawnsUnits = false round trips");
    }
}

// STEP204 §8 item 2 -- THE HUMAN'S RULING, PINNED. A legacy .sanmap authored before STEP204,
// carrying the retired "Navy"/"NavalFleet" (+ nested "PondSideByArmy"/"Side") keys, imports
// cleanly, silently drops that content (no error, no warning, no translation), and yields
// spawnsUnits == false (the struct default -- "SpawnsUnits" is simply absent from this fixture,
// exactly like every pre-STEP204 export).
void RunLegacyNavalFleetFixtureDroppedTest() {
    nlohmann::ordered_json document;
    nlohmann::ordered_json legacyDefaultScenario;
    legacyDefaultScenario["Name"] = "LegacyScenario";
    legacyDefaultScenario["Area"] = { { "x", 0.0 }, { "y", 0.0 }, { "width", 0.0 }, { "height", 0.0 } };
    legacyDefaultScenario["Navy"] = true;   // retired flag -- must NOT set spawnsUnits
    legacyDefaultScenario["AlloyMode"] = "occupancy";
    legacyDefaultScenario["Spawns"] = nlohmann::json::array();
    legacyDefaultScenario["Alloys"] = nlohmann::json::array();
    legacyDefaultScenario["AlloysToAdd"] = nlohmann::json::array();
    legacyDefaultScenario["AlloysToRemove"] = nlohmann::json::array();
    legacyDefaultScenario["AuthoringNote"] = "";
    nlohmann::ordered_json legacyNavalFleet;
    legacyNavalFleet["Fleet"] = { { { "TemplateIdentifier", "XSS0201" }, { "Count", 3 } } };
    legacyNavalFleet["PondSideByArmy"] = { { { "ArmyName", "ArmyOne" }, { "Side", -1 } } };
    legacyNavalFleet["SideBiasDistance"] = 90.0;
    legacyDefaultScenario["NavalFleet"] = legacyNavalFleet;
    document["Scenarios"]["PatternScenarios"] = nlohmann::json::array();
    document["Scenarios"]["CountScenarios"]   = nlohmann::json::array();
    document["Scenarios"]["DefaultScenario"]  = legacyDefaultScenario;

    Params::MapRecipe loaded;
    loaded.geometry.mapSize = 512;
    Io::MapImportResult result;
    Io::ReadScenariosJson(document, loaded, result);

    Check(loaded.scenarios.defaultScenario.name == "LegacyScenario",
          "legacy fixture: ordinary fields still import normally");
    Check(loaded.scenarios.defaultScenario.spawnsUnits == false,
          "legacy fixture: spawnsUnits defaults to false -- legacy Navy=true is dropped, not translated");
    Check(result.warningCount == 0,
          "legacy fixture: the retired Navy/NavalFleet keys are dropped silently -- zero warnings");
}

// STEP204 §8 item 3: negative assertions -- exported output never contains retired naval spellings.
void RunNoNavalSpellingsInOutputTest() {
    Params::MapRecipe recipe = BuildFixtureRecipe();
    const std::string documentText = Io::MapExporter::BuildSanmapJsonText(recipe);

    Check(documentText.find("NavalFleet") == std::string::npos, "export: no \"NavalFleet\" key anywhere");
    Check(documentText.find("\"Navy\"") == std::string::npos, "export: no \"Navy\" key anywhere");
    Check(documentText.find("PondSideByArmy") == std::string::npos, "export: no \"PondSideByArmy\" key anywhere");
    Check(documentText.find("SpawnsUnits") != std::string::npos,
          "export: \"SpawnsUnits\" IS present (the replacement field)");
}

// 7. Live-document integration: BuildSanmapJsonText then ParseSanmapJsonText.
void RunLiveDocumentIntegrationTest() {
    const Params::MapRecipe original = BuildFixtureRecipe();
    const std::string documentText = Io::MapExporter::BuildSanmapJsonText(original);

    Params::MapRecipe loaded;
    Io::MapImportOptions options;
    Io::MapImportResult result;
    bool bParsed = Io::MapImporter::ParseSanmapJsonText(documentText, loaded, options, result);
    Check(bParsed, "live document parses successfully");

    Check(loaded.scenarios.countScenarios.size() == 3, "live document: all 3 CountScenarios survive");
    if (loaded.scenarios.countScenarios.size() == 3) {
        Check(loaded.scenarios.countScenarios[0].body.name == "Zulu", "live document: CountScenarios[0] == Zulu");
        Check(loaded.scenarios.countScenarios[1].body.name == "Alpha", "live document: CountScenarios[1] == Alpha");
        Check(loaded.scenarios.countScenarios[2].body.name == "Mike", "live document: CountScenarios[2] == Mike");
    }
    Check(loaded.scenarios.patternScenarios.size() == 1, "live document: one PatternScenario survives");
    if (loaded.scenarios.patternScenarios.size() == 1) {
        CheckScenarioBodyEquals(original.scenarios.patternScenarios[0].body,
                                loaded.scenarios.patternScenarios[0].body, "live document PatternScenario");
    }
    CheckScenarioBodyEquals(original.scenarios.defaultScenario, loaded.scenarios.defaultScenario,
                            "live document DefaultScenario");
    Check(loaded.scenarios.maxArmySlotCount == 20, "live document: MaxArmySlotCount round trips");
}

} // namespace

int main() {
    RunPureRoundTripTests();
    RunAbsentKeyDefaultsTest();
    RunMalformedArrayWarningTest();
    RunAlloyModeAbsentDefaultsTest();
    RunSpawnsUnitsRoundTripBothStatesTest();
    RunLegacyNavalFleetFixtureDroppedTest();
    RunNoNavalSpellingsInOutputTest();
    RunLiveDocumentIntegrationTest();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
