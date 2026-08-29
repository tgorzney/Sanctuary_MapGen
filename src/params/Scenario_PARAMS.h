// Scenario_PARAMS.h — `Params::Scenarios`, the per-map lobby-resolved spawn/alloy scenario data
// (ARCH_15_05_ParamsScenariosType.md §15.5, amended by ARCH_15_10_SlotPatternConstructionMoves.md
// §15.10's `maxArmySlotCount`). Layer: PARAMS. Hand-authored, pass-through data (same posture as
// Army_PARAMS.h/MapArea_PARAMS.h) — no PROC stage computes or reinterprets it.
//
// Source of truth is ARCH_15_05_ParamsScenariosType.md §15.5 (and §15.10 for `maxArmySlotCount`);
// this file is a VERBATIM transcription, not a reinterpretation — any future shape change is an
// ARCH ratification first, this file second.
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "MapArea_PARAMS.h"

namespace SanmapGen {
namespace Params {

enum class ScenarioAlloyMode { Explicit, Occupancy, KeepAll, Delta };  // §5, MAP_SCENARIO_SPEC

// DEVIATION FROM §15.5's LITERAL TEXT, flagged (STEP69 coder): the ratified block declares these
// three trailing floats with no `= 0.0f` (unlike every other numeric field in this file). Same
// indeterminate-value hazard as `ScenarioCountCondition` below — added here too. Shape unchanged.
struct ScenarioSpawn         { std::string armyName; float positionX = 0.0f, positionY = 0.0f, positionZ = 0.0f; };
struct ScenarioAlloyOverride { std::string armyName; std::string markerName; float positionX = 0.0f, positionY = 0.0f, positionZ = 0.0f; };
struct ScenarioAlloyRemoval  { std::string armyName; std::string markerName; };

struct ScenarioBody {
    std::string name;                                    // log/debug identifier AND the dispatch
                                                            // key for unit spawning — see the ARCH
                                                            // §15.5 OPEN item, MAP_SCENARIO_SPEC §11
    // Reuses Params::MapArea wholesale (§15.5) — but the wire `Area` JSON object is `{x,y,width,
    // height}` only, no nested `name` key (the record's own `Name` is a sibling, not nested).
    // `area.name` is left empty/unused by the importer/exporter; do not "fix" this by populating
    // it from ScenarioBody::name — that would not round-trip (nothing on the wire reads it back).
    Params::MapArea area;
    std::string areaName;                                  // empty (default) = "use area directly" --
                                                            // today's exact behavior, fully backward
                                                            // compatible. Non-empty names a live
                                                            // Params::MapArea::name in recipe.areas,
                                                            // resolved into the wire Area/area rect AT
                                                            // EXPORT TIME ONLY (ARCH_15_05_ParamsScenariosType.md
                                                            // §15.5, "AMENDED 2026-08-28").
    bool spawnsUnits = false;                              // RETIRED 2026-08-28: was `navy` /
                                                            // `ScenarioNavalFleet navalFleet`
                                                            // (STEP204, ARCH_15_05 §15.5 amended).
                                                            // Generic opt-in only: true alone spawns
                                                            // nothing — a matching name-keyed branch
                                                            // must also exist in the runtime dispatch
                                                            // (ARCH_15_05 §15.5 OPEN item 2)
    ScenarioAlloyMode alloyMode = ScenarioAlloyMode::Occupancy;   // RATIFIED default, see below
    std::vector<ScenarioSpawn> spawns;                     // §6 HARD REQUIREMENT — empty is only
                                                            // legal with documented intent
    std::vector<ScenarioAlloyOverride> alloys;             // meaningful for `Explicit` only
    std::vector<ScenarioAlloyOverride> alloysToAdd;         // meaningful for `Delta` only
    std::vector<ScenarioAlloyRemoval>  alloysToRemove;      // meaningful for `Delta` only
    std::string authoringNote;                             // carries forward §6's "document that
                                                            // intent in the entry's own comment" as
                                                            // real data now that this is no longer
                                                            // hand-authored Lua text
};

struct PatternScenario { ScenarioBody body; std::string slotPattern; };            // TIER 1

enum class ScenarioComparator { Equal, NotEqual, GreaterThan, GreaterOrEqual, LessThan, LessOrEqual };
enum class ScenarioCountField { Total, HumanCount, AiCount };
// DEVIATION FROM §15.5's LITERAL TEXT, flagged (STEP69 coder): the ratified block leaves `field`/
// `comparator` with no default member initializer, unlike every sibling field in this same block
// (`alloyMode`/`side`/`value` all have one). A bare `Params::ScenarioCountCondition condition;`
// (the exact pattern `MapImporter_Scenarios_IO.cpp`'s array loop uses to build one per JSON
// element) would leave these two enums indeterminate — reading an uninitialized enum's value is
// undefined behavior, not just "unset." Given `int value = 0` already establishes the pattern's
// own intent, this file adds the two missing initializers (first-enumerator default, same
// convention as every other 0-valued default in this file) rather than transcribe a
// correctness bug verbatim. Shape (fields/types) is unchanged; only a default VALUE is added.
struct ScenarioCountCondition {
    ScenarioCountField field           = ScenarioCountField::Total;
    ScenarioComparator comparator      = ScenarioComparator::Equal;
    int                value           = 0;
};
struct CountScenario { ScenarioBody body; std::vector<ScenarioCountCondition> conditions; };  // TIER 2,
                                                            // conditions are AND'd (conjunction)

// ⚠️ `Scenarios` carries a fourth member beyond §15.5's original three — ARCH_15_10 §15.10 amends
// the shape with `maxArmySlotCount`, top-level and map-wide (NOT per-scenario): every authored
// `PatternScenario::slotPattern` in a map must share one length (TIER 1 exact-match).
struct Scenarios {
    std::vector<PatternScenario> patternScenarios;   // TIER 1 — pattern, §4
    std::vector<CountScenario>   countScenarios;     // TIER 2 — ORDER IS LOAD-BEARING, §15.6
    ScenarioBody                 defaultScenario;    // TIER 3 — always matches, exactly one
    int                          maxArmySlotCount = 16;  // §15.10 — slotPattern string length;
                                                         // rendered as the Lua global
                                                         // MAX_ARMY_SLOT_COUNT (STEP70)
};

} // namespace Params
} // namespace SanmapGen
