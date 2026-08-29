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
#include <algorithm>
#include <utility>

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

// Appends the 9 ScenarioBody fields as key=value/nested-table lines INSIDE an already-opened
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

    // RETIRED 2026-08-28 (STEP204): "navy"/"navalFleet" are gone. The Lua key spelling
    // "spawnsUnits" is fixed by the runtime (`matchedScenario.spawnsUnits`), not a free IO-tier
    // naming choice.
    AppendKeyValueLine(out, indentLevel, "spawnsUnits", RenderLuaBoolean(body.spawnsUnits));
    AppendKeyValueLine(out, indentLevel, "alloyMode",
                       QuotedLuaString(kScenarioAlloyModeSpellings[static_cast<int>(body.alloyMode)]));

    AppendArrayOfTables(out, indentLevel, "spawns", BuildSpawnRowBodies(body.spawns, mapSize));
    AppendArrayOfTables(out, indentLevel, "alloys", BuildAlloyOverrideRowBodies(body.alloys, mapSize));
    AppendArrayOfTables(out, indentLevel, "alloysToAdd", BuildAlloyOverrideRowBodies(body.alloysToAdd, mapSize));
    AppendArrayOfTables(out, indentLevel, "alloysToRemove", BuildAlloyRemovalRowBodies(body.alloysToRemove));

    // Real string data, never rendered as a `--` Lua comment (ARCH_15_05_ParamsScenariosType.md
    // §15.5: "as real data now that this is no longer hand-authored Lua text").
    AppendKeyValueLine(out, indentLevel, "authoringNote", QuotedLuaString(body.authoringNote));
}

// AppendArrayOfTables (STEP63) is NOT used for the three tier tables below -- it assumes a flat,
// single-line row body; ScenarioBody is multi-field/nested (sub-table area, several sub-arrays), so
// each element is opened/closed manually with OpenTable/CloseTable, still composing nothing but
// STEP63's primitives.

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

using AlloyRosterEntry = std::pair<std::string, std::string>;  // (armyName, markerName)

// Every (armyName, markerName) pair a ScenarioBody's alloy fields reference -- alloys/alloysToAdd/
// alloysToRemove ALL carry both fields (`ARCH_15_05_ParamsScenariosType.md` §15.5), so all three contribute to the roster.
void CollectScenarioBodyAlloyRosterEntries(const Params::ScenarioBody& body,
                                            std::vector<AlloyRosterEntry>& outEntries) {
    for (const auto& row : body.alloys)         outEntries.emplace_back(row.armyName, row.markerName);
    for (const auto& row : body.alloysToAdd)    outEntries.emplace_back(row.armyName, row.markerName);
    for (const auto& row : body.alloysToRemove) outEntries.emplace_back(row.armyName, row.markerName);
}

// ⚠️ ATTENTION -- EXTERNAL ENGINE BEHAVIOR, NOT INDEPENDENTLY VERIFIED (STEP73 §0). mapStartSlotIndex
// (what player.armyID is matched against, common/gameUtils.lua's CreateArmies()) is claimed -- by a
// comment in the live reference's own _data.lua (lines 51-54) -- to assign 1..N to this map's
// authored armies IN ALPHABETICAL NAME ORDER. This pack cannot read gameUtils.lua directly; the
// claim is inherited from that comment, not re-derived. IF IN-GAME ARMY SLOT ASSIGNMENT EVER LOOKS
// WRONG (the wrong army spawns in the wrong lobby slot), THIS IS THE FIRST PLACE TO LOOK.
// NOTE (STEP76): Army::name is now machine-minted by AssignArmyIdentities from the roster's own
// 1-based position specifically so that an alphabetical sort equals roster order; sorting here is
// still performed explicitly rather than trusted, so this function's own correctness never depends
// on that upstream invariant holding.
std::string BuildArmyIdToNameTable(const std::vector<Params::Army>& armies) {
    std::vector<std::string> sortedNames;
    sortedNames.reserve(armies.size());
    for (const auto& army : armies) sortedNames.push_back(army.name);
    std::sort(sortedNames.begin(), sortedNames.end());

    std::string out;
    OpenTable(out, 0, "ARMY_ID_TO_NAME");
    for (std::size_t i = 0; i < sortedNames.size(); ++i) {
        AppendKeyValueLine(out, 1, "[" + std::to_string(i + 1) + "]", QuotedLuaString(sortedNames[i]));
    }
    CloseTable(out, 0, false);
    return out;
}

// KNOWN_ALLOY_MARKERS is the union, per army, of every alloy marker name recipe.scenarios itself
// already references (STEP73 §0) -- NOT authored anywhere separately. Collected in a FIXED,
// deterministic order (pattern tier, then count tier -- each in its own vector order -- then the
// single default), matching BuildScenarioDataLuaText's own tier-emission order, so output is
// reproducible byte-for-byte from the same recipe with no staging container that could reorder
// (same discipline `ARCH_15_06_CountScenariosOrdering.md` §15.6 requires of CountScenarios itself).
std::string BuildKnownAlloyMarkersTable(const Params::Scenarios& scenarios) {
    std::vector<AlloyRosterEntry> allEntries;
    for (const auto& entry : scenarios.patternScenarios) CollectScenarioBodyAlloyRosterEntries(entry.body, allEntries);
    for (const auto& entry : scenarios.countScenarios)   CollectScenarioBodyAlloyRosterEntries(entry.body, allEntries);
    CollectScenarioBodyAlloyRosterEntries(scenarios.defaultScenario, allEntries);

    // Group by armyName (first-seen order), dedup markerName within each group (first-seen order).
    // Linear scan, not a hash map -- typical roster sizes are tiny, and this sidesteps any
    // container-iteration-order question outright.
    std::vector<std::string> armyOrder;
    std::vector<std::vector<std::string>> markersPerArmy;
    for (const auto& [armyName, markerName] : allEntries) {
        std::size_t armyIndex = 0;
        for (; armyIndex < armyOrder.size(); ++armyIndex) {
            if (armyOrder[armyIndex] == armyName) break;
        }
        if (armyIndex == armyOrder.size()) {
            armyOrder.push_back(armyName);
            markersPerArmy.emplace_back();
        }
        std::vector<std::string>& markers = markersPerArmy[armyIndex];
        if (std::find(markers.begin(), markers.end(), markerName) == markers.end()) {
            markers.push_back(markerName);
        }
    }

    std::string out;
    OpenTable(out, 0, "KNOWN_ALLOY_MARKERS");
    for (std::size_t i = 0; i < armyOrder.size(); ++i) {
        // Bracket-string key (["ARMY_01"] = {...}), never a bare identifier -- Army::name is a
        // free-form authored string (Army_PARAMS.h imposes no identifier-safety constraint on it),
        // so this is the one form guaranteed valid regardless of what characters the name contains.
        // Functionally identical for pairs()/[] lookup either way -- STEP72's runtime reads
        // KNOWN_ALLOY_MARKERS[armyName], which works against either syntax.
        AppendArrayOfQuotedStrings(out, 1, "[" + QuotedLuaString(armyOrder[i]) + "]", markersPerArmy[i]);
    }
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

    // ARMY_ID_TO_NAME / KNOWN_ALLOY_MARKERS -- both DERIVED from data already in `recipe`, never
    // authored separately (STEP73 §0). Map-wide, non-tiered globals like MAX_ARMY_SLOT_COUNT above,
    // so they are grouped with it, ahead of the three scenario tier tables.
    out += BuildArmyIdToNameTable(recipe.armies) + "\n";
    out += BuildKnownAlloyMarkersTable(scenarios) + "\n";

    out += BuildPatternScenariosTable(scenarios.patternScenarios, mapSize);
    out += "\n";
    out += BuildCountScenariosTable(scenarios.countScenarios, mapSize);
    out += "\n";
    out += BuildDefaultScenarioTable(scenarios.defaultScenario, mapSize);

    return out;
}

} // namespace Io
} // namespace SanmapGen
