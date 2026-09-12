// ScenarioScript_SlotPatternExtract_IO.h -- the pure, disk-free, filename-agnostic closed
// literal-only grammar for extracting {name, slotPattern} pairs verbatim from a `PATTERN_SCENARIOS`
// -shaped array-of-tables inside FOREIGN scenario .lua text
// (`ARCH_15_14_ForeignScenarioFullDataImportAndUnitPlacement.md` Part B, Shape 2). Layer: IO.
//
// Same posture as ScenarioScript_AreaRectangleExtract_IO.h / ScenarioScript_MatchConditionExtract_IO.h:
// this is NOT "the reader half" of ScenarioScript_DataLua_IO, never touches a file SanGen itself
// wrote, takes text, returns values, and performs NO Lua execution of any kind. This is the
// lowest-risk of the three Part-B shapes -- a bare string-literal read, the same risk class as the
// ratified area-rectangle extractor's numeric literals.
#pragma once
#include <cstddef>
#include <string>
#include <vector>

namespace SanmapGen {
namespace Io {

// One `{ ..., pattern = "STRING", ... }` element that had a `pattern` key at all (so it entered the
// grammar) but failed to yield a clean {name, slotPattern} pair -- never partially extracted, never
// guessed. `identifier` carries the element's own `name` string if one was found (even though the
// element is being rejected for some OTHER reason), else empty.
struct ScenarioSlotPatternExtractionNearMiss {
    std::string identifier;
    std::string reason;
};

// One successfully recognized element. Deliberately NOT Params::PatternScenario -- that type
// requires a Params::ScenarioBody this extractor does not and must not produce.
struct ScenarioSlotPatternExtractionEntry {
    std::string name;
    std::string slotPattern;
};

// Constitution §6 cap -- extraction stops (does not crash, does not unbounded-grow) once this many
// VALID entries have been extracted from one file. Chosen generously above any real scenario script
// observed (today's live reference file's own PATTERN_SCENARIOS array is empty) while still bounding
// pathological input, following kMaxScenarioAreaExtractionRectangleCount = 512's precedent verbatim.
inline constexpr std::size_t kMaxScenarioSlotPatternExtractionCount = 512;

// An absurd-length guard on ONE slotPattern string, separate from (and much larger than) any real
// map's authored `maxArmySlotCount` -- this extractor has no map context to check that real limit
// against (that is a later, map-aware validator's job, per Part B's own text). This is purely a
// backstop against a pathologically long string literal in foreign input; a pattern exceeding it is
// rejected as a near-miss, never silently truncated.
inline constexpr std::size_t kMaxScenarioSlotPatternStringLength = 4096;

struct ScenarioSlotPatternExtractionResult {
    std::vector<ScenarioSlotPatternExtractionEntry>    entries;      // in file order
    std::vector<ScenarioSlotPatternExtractionNearMiss> nearMisses;
    bool bSlotPatternCountCapExceeded = false;   // kMaxScenarioSlotPatternExtractionCount reached --
                                                  // extraction stopped early
};

// Scans sourceText ONCE (comments/strings already skipped by the shared
// ScenarioScript_LuaLiteralLexer_IO tokenizer) for `[local] IDENTIFIER = { ... }` bindings whose body
// is itself a comma-separated array of POSITIONAL `{ ... }` sub-tables (as opposed to a flat
// key=value table, or a table of KEYED sub-tables like `spawns = { ARMY_01 = {...} } }` -- those
// shapes are structurally distinguished and never mistaken for this one). Each positional sub-table
// is a candidate: if it has a `pattern = "STRING"` field among its own top-level keys (nested tables
// within it, e.g. an `area`/`spawns`/`alloys` sibling field, are never inspected for this), it is
// graded: a `pattern` value that is not a single plain string literal, or a `pattern` field with no
// sibling `name = "STRING"` field, is a near-miss; otherwise it yields one
// ScenarioSlotPatternExtractionEntry with BOTH strings verbatim (ordinary Lua string-literal decoding
// only -- no additional escape processing, matching the shared lexer's own string handling). A
// sub-table with no `pattern` key at all is simply not a candidate for this shape and contributes
// nothing (not a near-miss) -- this lets the same scan pass safely over unrelated arrays-of-tables
// elsewhere in the file (e.g. COUNT_SCENARIOS) without spurious diagnostics.
//
// Pure and total: never throws, never touches the filesystem, never partially mutates its output on
// a mid-scan failure.
ScenarioSlotPatternExtractionResult
    ExtractSlotPatternsFromScenarioScriptText(const std::string& sourceText);

} // namespace Io
} // namespace SanmapGen
