// ScenarioScript_AreaRectangleExtract_IO.h -- the pure, disk-free, filename-agnostic closed
// literal-only grammar for extracting Params::MapArea rectangles from FOREIGN scenario .lua text
// (`ARCH_15_11_ForeignScenarioAreaImport.md` §15.11 items 2-7, 10). Layer: IO.
//
// This is NOT "the reader half" of ScenarioScript_DataLua_IO (§15.11 item 11) -- it is a physically
// separate translation unit that never touches a file SanGen itself wrote; the refusal guard that
// keeps those two file sets disjoint lives ONLY in the disk-touching caller
// (ScenarioScript_AreaImport_IO.h), never here. This header takes text, returns values, and performs
// NO Lua execution of any kind -- not LuaTableEvaluate_SYS, not a variant of it, ever (§15.11 item 3).
#pragma once
#include <cstddef>
#include <string>
#include <vector>
#include "../params/MapArea_PARAMS.h"

namespace SanmapGen {
namespace Io {

// One candidate that entered the grammar (matched `[local] IDENTIFIER = { ... }`) but failed it --
// never partially filled into a rectangle, never guessed (§15.11 item 6). `identifier` is always
// non-empty: a diagnostic is only ever created after an identifier token was already matched.
struct ScenarioAreaExtractionNearMiss {
    std::string identifier;
    std::string reason;
};

// Constitution §6 cap -- extraction stops (does not crash, does not unbounded-grow) once this many
// VALID rectangles have been extracted from one file (§15.11 item 10). Chosen generously above any
// real scenario script observed (the reference file defines four) while still bounding pathological
// input; not a map-design limit of any kind.
inline constexpr std::size_t kMaxScenarioAreaExtractionRectangleCount = 512;

// Constitution §6 "absurd coordinate" bound (§15.11 item 10). Real Sanctuary maps top out at a few
// thousand world units per side (the reference file's own AREA_FULL is 2048x2048); this is a
// generous backstop against garbage/overflow-adjacent values, not a tight map-size validator -- a
// later, map-size-aware check (if ever wanted) is a separate, unrelated concern from this
// grammar-level sanity gate. width/height must additionally be strictly positive -- a zero or
// negative extent is not a rectangle.
inline constexpr float kMaxScenarioAreaCoordinateMagnitude = 1.0e6f;

struct ScenarioAreaExtractionResult {
    std::vector<Params::MapArea>               areas;                 // in file order; in-file
                                                                        // collisions already resolved
                                                                        // last-write-wins (item 7)
    std::vector<std::string>                   collisionIdentifiers;  // identifiers reassigned within
                                                                        // THIS file (item 7, logged)
    std::vector<ScenarioAreaExtractionNearMiss> nearMisses;            // item 6
    bool bRectangleCountCapExceeded = false;    // kMaxScenarioAreaExtractionRectangleCount reached --
                                                 // extraction stopped early; the remainder of the
                                                 // source text was never scanned (item 10)
};

// Scans sourceText ONCE, skipping `--` line comments, `--[[ ]]` long comments, and single/double
// -quoted strings (item 5), for `[local] IDENTIFIER = { ... }` candidates whose body matches the
// closed grammar: EITHER all four keyed pairs x/y/width/height in any order, OR exactly four
// positional values read as x, y, width, height in that order (item 4). Arithmetic, identifiers,
// function calls, string keys, nesting, a fifth/missing key, and mixed keyed/positional in one table
// are all rejected outright, never evaluated. Field mapping is fixed and verbatim per item 7: Lua `x`
// -> MapArea::originX, `y` -> originZ, `width` -> width, `height` -> length; the Lua identifier
// becomes MapArea::name VERBATIM (never prettified/de-prefixed/invented -- it is a load-bearing
// gameplay identifier, GameUtils.GetArea(name)).
//
// Pure and total: never throws, never touches the filesystem, never partially mutates its output on
// a mid-scan failure (a rejected candidate contributes zero rectangles and exactly one near-miss).
ScenarioAreaExtractionResult ExtractAreaRectanglesFromScenarioScriptText(const std::string& sourceText);

} // namespace Io
} // namespace SanmapGen
