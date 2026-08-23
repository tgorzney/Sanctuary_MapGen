// ScenarioScript_DataLua_IO.cpp -- see ScenarioScript_DataLua_IO.h for the file-level contract.
// Composes ONLY LuaTableWriter_IO.h's generic primitives (STEP63); never JsonPrimitives_IO, never
// hand-rolled Lua string concatenation outside those primitives.
//
// Domain-local string-spelling tables below are a DELIBERATE duplicate of
// MapExporter_Scenarios_IO.cpp's own kAlloyModeSpellings/kCountFieldSpellings/kComparatorSpellings
// (STEP69) -- "each file owns its own copy" precedent (STEP69 §5); this file must never #include
// MapExporter_Scenarios_IO.cpp. Spellings are reused VERBATIM -- one shared vocabulary between the
// .sanmap JSON persistence leg and this Lua-rendering leg.
#include "ScenarioScript_DataLua_IO.h"
#include "LuaTableWriter_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {
namespace {

// Index == the enum's own declaration order (ARCH_15_05_ParamsScenariosType.md §15.5) -- do not
// reorder. Verbatim duplicate of MapExporter_Scenarios_IO.cpp's kAlloyModeSpellings.
constexpr const char* kScenarioAlloyModeSpellings[4] = { "explicit", "occupancy", "keepAll", "delta" };
constexpr const char* kScenarioCountFieldSpellings[3] = { "Total", "HumanCount", "AiCount" };
constexpr const char* kScenarioComparatorSpellings[6] =
    { "Equal", "NotEqual", "GreaterThan", "GreaterOrEqual", "LessThan", "LessOrEqual" };

// ⚠️ ATTENTION -- COORDINATE FLIP UNCONFIRMED FOR SCENARIOS (Lua-rendering leg).
// Applies the SAME `mapSize - z - 1` flip STEP69's MapExporter_Scenarios_IO.cpp applies to the
// .sanmap JSON leg, for the same reason (every other InstancedTransform-shaped position field
// flips z on export). NOT independently ratified for this Lua-text leg by ARCH_15_05_ParamsScenariosType.md §15.5 or
// MAP_SCENARIO_SPEC.md -- chosen for consistency with STEP69's own ruling, per the human's
// 2026-08-21 decision to implement now and verify later.
// IF SCENARIO SPAWNS/ALLOYS APPEAR MIRRORED ALONG Z IN-GAME, THIS IS THE FIRST PLACE TO LOOK --
// alongside the matching comment in MapExporter_Scenarios_IO.cpp (STEP69). Round-trip tests
// CANNOT catch a wrong choice here (there is no import path for this file at all, ARCH_15_03_ExportOnlyLuaRatified.md §15.3) --
// only in-game verification will.
float FlipPositionZ(float positionZ, int mapSize) {
    return static_cast<float>(mapSize) - positionZ - 1.0f;
}

// One flat `armyName = "...", x = ..., y = ..., z = ...` row for a spawns/alloys/alloysToAdd entry.
// `markerName` is only appended when non-null (spawns carry no marker; alloys/alloysToAdd do).
std::string BuildPositionedRowBody(const std::string& armyName, const std::string* markerName,
                                   float positionX, float positionY, float positionZ, int mapSize) {
    std::string row;
    row += "armyName = " + QuotedLuaString(armyName) + ", ";
    if (markerName != nullptr) row += "markerName = " + QuotedLuaString(*markerName) + ", ";
    row += "x = " + RenderLuaNumber(positionX) + ", ";
    row += "y = " + RenderLuaNumber(positionY) + ", ";
    row += "z = " + RenderLuaNumber(FlipPositionZ(positionZ, mapSize));
    return row;
}

std::vector<std::string> BuildSpawnRowBodies(const std::vector<Params::ScenarioSpawn>& spawns, int mapSize) {
    std::vector<std::string> rows;
    rows.reserve(spawns.size());
    for (const Params::ScenarioSpawn& spawn : spawns) {
        rows.push_back(BuildPositionedRowBody(spawn.armyName, nullptr, spawn.positionX, spawn.positionY,
                                              spawn.positionZ, mapSize));
    }
    return rows;
}

std::vector<std::string> BuildAlloyOverrideRowBodies(const std::vector<Params::ScenarioAlloyOverride>& overrides,
                                                      int mapSize) {
    std::vector<std::string> rows;
    rows.reserve(overrides.size());
    for (const Params::ScenarioAlloyOverride& entry : overrides) {
        rows.push_back(BuildPositionedRowBody(entry.armyName, &entry.markerName, entry.positionX,
                                              entry.positionY, entry.positionZ, mapSize));
    }
    return rows;
}

std::vector<std::string> BuildAlloyRemovalRowBodies(const std::vector<Params::ScenarioAlloyRemoval>& removals) {
    std::vector<std::string> rows;
    rows.reserve(removals.size());
    for (const Params::ScenarioAlloyRemoval& removal : removals) {
        rows.push_back("armyName = " + QuotedLuaString(removal.armyName) + ", markerName = "
                       + QuotedLuaString(removal.markerName));
    }
    return rows;
}

// navalFleet -- ALWAYS emitted, even when navy == false (matches STEP69 §6's rule). Fully
// self-contained: opens/closes its own "navalFleet" table at indentLevel.
void AppendNavalFleetTable(std::string& out, int indentLevel, const Params::ScenarioNavalFleet& navalFleet) {
    OpenTable(out, indentLevel, "navalFleet");

    std::vector<std::string> fleetRows;
    fleetRows.reserve(navalFleet.fleet.size());
    for (const Params::ScenarioNavalFleetEntry& entry : navalFleet.fleet) {
        fleetRows.push_back("templateIdentifier = " + QuotedLuaString(entry.templateIdentifier)
                            + ", count = " + RenderLuaNumber(entry.count));
    }
    AppendArrayOfTables(out, indentLevel + 1, "fleet", fleetRows);

    // Raw signed integer (-1/1), NEVER QuotedLuaString/an enum-spelling lookup -- mirrors STEP69
    // §4's "do NOT use ReadJsonEnumerationText for Side" ruling, applied symmetrically on write.
    std::vector<std::string> pondSideRows;
    pondSideRows.reserve(navalFleet.pondSideByArmy.size());
    for (const Params::ScenarioNavalPondAssignment& assignment : navalFleet.pondSideByArmy) {
        pondSideRows.push_back("armyName = " + QuotedLuaString(assignment.armyName) + ", side = "
                               + RenderLuaNumber(static_cast<int>(assignment.side)));
    }
    AppendArrayOfTables(out, indentLevel + 1, "pondSideByArmy", pondSideRows);

    AppendKeyValueLine(out, indentLevel + 1, "sideBiasDistance", RenderLuaNumber(navalFleet.sideBiasDistance));

    CloseTable(out, indentLevel, true);
}

// Appends the 10 ScenarioBody fields as key=value/nested-table lines INSIDE an already-opened
// table (the caller opens/closes the outer `{ ... }`; this only fills it). Field order mirrors
// STEP69 §6's JSON emission order for direct cross-reference. Lua keys are lowerCamelCase mirroring
// the C++ member names -- NOT the JSON's PascalCase spellings.
void AppendScenarioBodyFields(std::string& out, int indentLevel, const Params::ScenarioBody& body, int mapSize) {
    AppendKeyValueLine(out, indentLevel, "name", QuotedLuaString(body.name));

    OpenTable(out, indentLevel, "area");
    AppendKeyValueLine(out, indentLevel + 1, "x", RenderLuaNumber(body.area.originX));
    AppendKeyValueLine(out, indentLevel + 1, "y", RenderLuaNumber(body.area.originZ));
    AppendKeyValueLine(out, indentLevel + 1, "width", RenderLuaNumber(body.area.width));
    AppendKeyValueLine(out, indentLevel + 1, "height", RenderLuaNumber(body.area.length));
    CloseTable(out, indentLevel, true);

    AppendKeyValueLine(out, indentLevel, "navy", RenderLuaBoolean(body.navy));
    AppendKeyValueLine(out, indentLevel, "alloyMode",
                       QuotedLuaString(kScenarioAlloyModeSpellings[static_cast<int>(body.alloyMode)]));

    AppendArrayOfTables(out, indentLevel, "spawns", BuildSpawnRowBodies(body.spawns, mapSize));
    AppendArrayOfTables(out, indentLevel, "alloys", BuildAlloyOverrideRowBodies(body.alloys, mapSize));
    AppendArrayOfTables(out, indentLevel, "alloysToAdd", BuildAlloyOverrideRowBodies(body.alloysToAdd, mapSize));
    AppendArrayOfTables(out, indentLevel, "alloysToRemove", BuildAlloyRemovalRowBodies(body.alloysToRemove));

    // Real string data, never rendered as a `--` Lua comment (ARCH_15_05_ParamsScenariosType.md
    // §15.5: "as real data now that this is no longer hand-authored Lua text").
    AppendKeyValueLine(out, indentLevel, "authoringNote", QuotedLuaString(body.authoringNote));

    AppendNavalFleetTable(out, indentLevel, body.navalFleet);
}

// AppendArrayOfTables (STEP63) is NOT used for the three tier tables below -- it assumes a flat,
// single-line row body; ScenarioBody is multi-field/nested (sub-table area, several sub-arrays,
// nested navalFleet), so each element is opened/closed manually with OpenTable/CloseTable, still
// composing nothing but STEP63's primitives.

std::string BuildPatternScenariosTable(const std::vector<Params::PatternScenario>& patternScenarios, int mapSize) {
    std::string out;
    OpenTable(out, 0, "PATTERN_SCENARIOS");
    for (const Params::PatternScenario& entry : patternScenarios) {
        OpenTable(out, 1, "");
        // Lua key "pattern" -- matches MAP_SCENARIO_SPEC.md §4's scenario.pattern, NOT slotPattern.
        AppendKeyValueLine(out, 1, "pattern", QuotedLuaString(entry.slotPattern));
        AppendScenarioBodyFields(out, 1, entry.body, mapSize);
        CloseTable(out, 1, true);
    }
    CloseTable(out, 0, false);
    return out;
}

// ⚠️ Load-bearing: iterates countScenarios in the std::vector's own order, one for loop, no
// staging/sorting container of any kind (ARCH_15_06_CountScenariosOrdering.md §15.6).
std::string BuildCountScenariosTable(const std::vector<Params::CountScenario>& countScenarios, int mapSize) {
    std::string out;
    OpenTable(out, 0, "COUNT_SCENARIOS");
    for (const Params::CountScenario& entry : countScenarios) {
        OpenTable(out, 1, "");

        // Conditions within one CountScenario are conjunction-only (AND'd, ARCH_15_05
        // §15.5) -- the emitted array carries no OR/grouping structure, matching
        // Params::CountScenario::conditions' flat std::vector shape exactly.
        std::vector<std::string> conditionRows;
        conditionRows.reserve(entry.conditions.size());
        for (const Params::ScenarioCountCondition& condition : entry.conditions) {
            conditionRows.push_back(
                "field = " + QuotedLuaString(kScenarioCountFieldSpellings[static_cast<int>(condition.field)])
                + ", comparator = " + QuotedLuaString(kScenarioComparatorSpellings[static_cast<int>(condition.comparator)])
                + ", value = " + RenderLuaNumber(condition.value));
        }
        AppendArrayOfTables(out, 2, "conditions", conditionRows);

        AppendScenarioBodyFields(out, 1, entry.body, mapSize);
        CloseTable(out, 1, true);
    }
    CloseTable(out, 0, false);
    return out;
}

// DEFAULT_SCENARIO is one record, not a list (ARCH_15_06_CountScenariosOrdering.md §15.6: only
// countScenarios carries an ordering requirement).
std::string BuildDefaultScenarioTable(const Params::ScenarioBody& body, int mapSize) {
    std::string out;
    OpenTable(out, 0, "DEFAULT_SCENARIO");
    AppendScenarioBodyFields(out, 1, body, mapSize);
    CloseTable(out, 0, false);
    return out;
}

} // namespace

std::string BuildScenarioDataLuaText(const Params::MapRecipe& recipe) {
    const int mapSize = recipe.geometry.mapSize;
    const Params::Scenarios& scenarios = recipe.scenarios;

    std::string out;
    out += std::string(kScenarioGeneratedFileBannerLine) + "\n";
    out += "-- Source: " + recipe.mapName + ".sanmap \"Scenarios\" section (see MAP_SCENARIO_SPEC.md).\n\n";

    // Bare global assignment, NOT a table member -- AppendKeyValueLine is not used here (that
    // emits a trailing comma for a table member; this is a file-level global statement). Global,
    // never `local` -- the runtime Import()s this file and captures only globals.
    out += "MAX_ARMY_SLOT_COUNT = " + RenderLuaNumber(scenarios.maxArmySlotCount) + "\n\n";

    out += BuildPatternScenariosTable(scenarios.patternScenarios, mapSize);
    out += "\n";
    out += BuildCountScenariosTable(scenarios.countScenarios, mapSize);
    out += "\n";
    out += BuildDefaultScenarioTable(scenarios.defaultScenario, mapSize);

    return out;
}

} // namespace Io
} // namespace SanmapGen
