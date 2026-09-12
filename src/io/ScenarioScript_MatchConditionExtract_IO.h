// ScenarioScript_MatchConditionExtract_IO.h -- the pure, disk-free, filename-agnostic closed
// literal-only grammar for extracting Params::ScenarioCountCondition vectors from a `match =
// function(t, h, a, pattern) return ... end` field inside FOREIGN scenario .lua text
// (`ARCH_15_14_ForeignScenarioFullDataImportAndUnitPlacement.md` Part B, Shape 1). Layer: IO.
//
// Same posture as ScenarioScript_AreaRectangleExtract_IO.h: this is NOT "the reader half" of
// ScenarioScript_DataLua_IO, never touches a file SanGen itself wrote (that refusal guard lives only
// in a disk-touching caller, never here), takes text, returns values, and performs NO Lua execution
// of any kind -- not LuaTableEvaluate_SYS, not a variant of it, ever. Shape 1's `and`/`or`/comparator
// syntax makes this the extractor most likely to tempt "just evaluate the boolean expression" --
// do not; every recognized shape below is matched token-for-token against one of exactly two closed
// templates, never interpreted.
#pragma once
#include <cstddef>
#include <string>
#include <vector>
#include "../params/Scenario_PARAMS.h"

namespace SanmapGen {
namespace Io {

// One candidate `match = function(t, h, a, pattern) ... end` field that was recognized as entering
// the grammar (the exact fixed preamble `match = function(t, h, a, pattern)` matched) but whose body
// failed BOTH closed templates -- never partially extracted, never guessed. `identifier` carries
// whatever surrounding-context tag was available (the nearest preceding sibling `name = "..."`
// string seen in the file scan so far) purely so a human can later recognize which scenario record
// this came from; it is read as plain context, never interpreted further, and may be empty if no
// such context was seen yet.
struct ScenarioMatchConditionExtractionNearMiss {
    std::string identifier;
    std::string reason;
};

// One successfully recognized `match` field: the AND-chain of conditions it closed-form matched
// (Template A or B), tagged with the same best-effort context identifier as the near-miss struct
// above. Deliberately NOT a Params::CountScenario or Params::ScenarioBody (Part B: "wiring an
// extracted condition set to a scenario name/area is a separate human authoring action in the UI,
// not this extractor's job") -- this is a bare vector of conditions plus a context tag, nothing more.
struct ScenarioMatchConditionCandidate {
    std::string                                identifier;   // context tag; may be empty
    std::vector<Params::ScenarioCountCondition> conditions;   // AND semantics, in source order
};

// Constitution §6 cap -- extraction stops (does not crash, does not unbounded-grow) once this many
// VALID candidate condition-sets have been extracted from one file. Chosen generously above any real
// scenario script observed (the reference file defines eight COUNT_SCENARIOS match functions total)
// while still bounding pathological input, following kMaxScenarioAreaExtractionRectangleCount = 512's
// precedent verbatim.
inline constexpr std::size_t kMaxScenarioMatchConditionExtractionCount = 512;

// A separate, per-chain bound on Template A's conjunct count (the number of `VAR OP N` terms ANDed
// together in one `match` body). The real reference tops out at three conjuncts (`2h1ai`); this is a
// generous-but-bounded safety backstop against a pathologically long AND-chain in foreign input, not
// a real scenario-design limit of any kind. A chain exceeding this cap is rejected as a near-miss for
// that one match function -- it does NOT set bMatchConditionCountCapExceeded, which is reserved for
// the file-wide candidate-count cap above.
inline constexpr std::size_t kMaxScenarioMatchConditionConjunctCountPerChain = 64;

struct ScenarioMatchConditionExtractionResult {
    std::vector<ScenarioMatchConditionCandidate>            conditionSets;   // in file order
    std::vector<ScenarioMatchConditionExtractionNearMiss>   nearMisses;
    bool bMatchConditionCountCapExceeded = false;   // kMaxScenarioMatchConditionExtractionCount
                                                     // reached -- extraction stopped early, the
                                                     // remainder of the source text was never scanned
};

// Scans sourceText ONCE (comments/strings already skipped by the shared
// ScenarioScript_LuaLiteralLexer_IO tokenizer) for `match = function(t, h, a, pattern) return ...
// end` fields -- that exact fixed preamble is the recognition anchor; a field whose parameter list
// differs is not recognized as a candidate at all (silently skipped, not a near-miss). Once the
// preamble is matched, the body between `return` and the closing `end` is graded against exactly two
// closed templates:
//
//   Template A -- AND-chain of `t`/`h`/`a` count comparisons: `return COND (and COND)*`, each COND =
//   `VAR OP N` (VAR in {t,h,a}, OP one of the six literal comparator spellings, N a signed integer
//   literal). A single optional pair of parentheses wrapping the WHOLE chain is tolerated. `or`
//   ANYWHERE in the body is an automatic near-miss for the whole field, checked before anything else.
//   Each conjunct becomes one ScenarioCountCondition{field=Total|HumanCount|AiCount, comparator,
//   value=N} (t->Total, h->HumanCount, a->AiCount); the whole chain becomes one candidate's
//   `conditions` vector, verbatim AND order.
//
//   Template B -- slot-range occupancy: `return pattern:sub(N, M):find("[^-]") ~= nil`, every token
//   except the two integers N/M matched byte-for-byte (method names, the exact string literal
//   "[^-]", the `~= nil` comparison). Maps to the ONE fixed pairing
//   ScenarioCountCondition{field=SlotRangeOccupiedCount, comparator=GreaterOrEqual, value=1,
//   slotRangeStart=N, slotRangeEnd=M} -- never a derived comparator/value.
//
// Anything else (a different body shape, a VAR compared to another VAR, arithmetic on N, any
// identifier other than t/h/a/pattern, a different Template-B string/method/negation) is a near-miss
// for that whole match function -- zero conditions contributed, exactly one near-miss entry.
//
// Pure and total: never throws, never touches the filesystem, never partially mutates its output on
// a mid-scan failure.
ScenarioMatchConditionExtractionResult
    ExtractMatchConditionsFromScenarioScriptText(const std::string& sourceText);

} // namespace Io
} // namespace SanmapGen
