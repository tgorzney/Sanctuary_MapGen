// ScenarioScript_UnitPlacementExtract_IO.h -- the pure, disk-free, filename-agnostic closed
// literal-only grammar for extracting Params::ScenarioUnitPlacement rows from a top-level
// `local <IDENT> = { { armyName = "...", templateIdentifier = "...", x = ..., y = ..., z = ... }, ... }`
// -shaped array inside FOREIGN scenario .lua text
// (`ARCH_15_14_ForeignScenarioFullDataImportAndUnitPlacement.md` Part B, Shape 3). Layer: IO.
//
// Same posture as ScenarioScript_AreaRectangleExtract_IO.h / ScenarioScript_SlotPatternExtract_IO.h /
// ScenarioScript_MatchConditionExtract_IO.h: this is NOT "the reader half" of ScenarioScript_DataLua_IO,
// never touches a file SanGen itself wrote (that refusal guard lives only in a disk-touching caller,
// never here), takes text plus the target map's own mapSize as an ordinary scalar parameter, returns
// values, and performs NO Lua execution of any kind -- not LuaTableEvaluate_SYS, not a variant of it,
// ever.
#pragma once
#include <cstddef>
#include <string>
#include <vector>
#include "../params/Scenario_PARAMS.h"

namespace SanmapGen {
namespace Io {

// One candidate element that entered the grammar (had a `templateIdentifier` or `armyIndex` key --
// see the .cpp's ChooseIfCandidate doc comment for why those two, not `armyName`, are the shape's
// recognition anchor) but failed it -- never partially filled, never guessed. `armyName` carries the
// row's own `armyName` string if one was found and parsed cleanly (even though the row is being
// rejected for some OTHER reason), else empty -- best-effort human-facing context only, mirroring
// ScenarioSlotPatternExtractionNearMiss::identifier's own posture, never interpreted further.
struct ScenarioUnitPlacementExtractionNearMiss {
    std::string armyName;
    std::string reason;
};

// Constitution §6 cap -- extraction stops (does not crash, does not unbounded-grow) once this many
// VALID rows have been extracted from one file. Chosen following
// kMaxScenarioAreaExtractionRectangleCount = 512's precedent verbatim -- not a real scenario-design
// limit of any kind (this shape has zero real-world exercise today, per ARCH_15_14 Part B's own
// ground-truth caveat).
inline constexpr std::size_t kMaxScenarioUnitPlacementExtractionCount = 512;

struct ScenarioUnitPlacementExtractionResult {
    std::vector<Params::ScenarioUnitPlacement>            placements;   // in file order
    std::vector<ScenarioUnitPlacementExtractionNearMiss>  nearMisses;
    bool bUnitPlacementCountCapExceeded = false;   // kMaxScenarioUnitPlacementExtractionCount reached
                                                    // -- extraction stopped early, the remainder of the
                                                    // source text was never scanned
};

// Scans sourceText ONCE (comments/strings already skipped by the shared
// ScenarioScript_LuaLiteralLexer_IO tokenizer) for `[local] IDENTIFIER = { ... }` top-level bindings
// whose body is itself a comma-separated array of POSITIONAL `{ ... }` sub-tables (the same
// array-of-tables shape ScenarioScript_SlotPatternExtract_IO.h scans -- a KEYED sub-table, e.g.
// `spawns = { ARMY_01 = {...} }`, is structurally distinguished and never mistaken for one of these
// elements). Each positional sub-table carrying a `templateIdentifier` or `armyIndex` key is a
// candidate for this shape (this deliberately excludes `Params::ScenarioSpawnPoint`'s own
// SCENARIO_SPAWN_POINTS array shape, which shares `armyName`/`x`/`y`/`z` but never carries either of
// those two keys); a sub-table with neither key is simply not a candidate for this shape and
// contributes nothing (not a near-miss), letting this same scan pass safely over an unrelated
// COUNT_SCENARIOS/PATTERN_SCENARIOS/SCENARIO_SPAWN_POINTS array elsewhere in the file.
//
// A candidate is then graded against the closed grammar:
// - `armyName`/`templateIdentifier` -- required, quoted string literals only.
// - `x`/`y`/`z` -- required, numeric literals only (optional leading sign), each bounded by
//   ScenarioScript_AreaRectangleExtract_IO.h's own kMaxScenarioAreaCoordinateMagnitude (reused
//   verbatim -- same map-scale-float semantic, no new constant needed).
// - Any missing required key, any extra/unrecognized key, or a non-literal value is a near-miss for
//   THAT ONE ROW only -- the rest of the array keeps scanning, never abort-the-whole-array.
// - **Binding rule:** a row keyed by `armyIndex` instead of `armyName` is REJECTED as a near-miss
//   naming exactly this ambiguity, checked and reported before any other grading -- a raw
//   `pairs(Armies)` runtime handle is never reinterpreted as a stable `ARMY_XX` identity
//   (ARCH_15_14 Part A, "Corrections" item 3).
// - Optional sixth key, a nested `rotation = { x = RX, y = RY, z = RZ, w = RW }` table -- all four
//   sub-fields required together if the key is present at all; a partial rotation table is a
//   near-miss for that row. Absence of the whole `rotation` key is NOT a near-miss -- it defaults to
//   identity (0,0,0,1), matching Params::ScenarioUnitPlacement's own default-constructed fields.
// - Field mapping: Lua `x`->positionX, `y`->positionY verbatim (no arithmetic); Lua `z`->positionZ
//   via `mapSize - z - 1` (self-inverse of ScenarioScript_DataLua_IO.cpp's own FlipPositionZ) --
//   THIS extractor is therefore NOT context-free like the area-rectangle extractor; it takes the
//   target map's `mapSize` as an ordinary scalar parameter, still performing zero filesystem access
//   and zero Lua execution. `rotation` maps verbatim to rotationX/Y/Z/rotationW -- never flipped.
//
// Pure and total: never throws, never touches the filesystem, never partially mutates its output on
// a mid-scan failure.
ScenarioUnitPlacementExtractionResult
    ExtractUnitPlacementsFromScenarioScriptText(const std::string& sourceText, int mapSize);

} // namespace Io
} // namespace SanmapGen
