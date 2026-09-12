# STEP264 — Foreign-`.lua` extractors: match conditions (Shape 1) + slot-pattern strings (Shape 2)

**Layer:** IO/BRIDGE. **Domain:** new `src/io/ScenarioScript_MatchConditionExtract_IO.h/.cpp`, new
`src/io/ScenarioScript_SlotPatternExtract_IO.h/.cpp`. **Executor:** SanGen Coder. **Sequence:** depends
on `STEP262` (shared `ScenarioScript_LuaLiteralLexer_IO`). Implements
`ARCH_15_14_ForeignScenarioFullDataImportAndUnitPlacement.md` Part B, Shapes 1 and 2.

**Read `ARCH_15_14` Part B in full before starting — it is the binding grammar, not a paraphrase to
work from.** Every near-miss rule, every "absolutely not extracted" boundary, and the two closed
templates for Shape 1 are load-bearing law, not suggestions. Where this ticket's text and the ARCH
section disagree, the ARCH section wins.

## 0. Why

`ARCH_15_11`'s existing carve-out only extracts area rectangles. `ARCH_15_14` Part B ratifies two more
narrow, non-executing literal shapes, verified against the real Pandemonium Isthmus reference file to
cover every one of its 8 live `COUNT_SCENARIOS` match functions with zero exceptions. This ticket
builds the first two (of three) extractors; `STEP265` builds the third (unit placements) plus the
shared orchestrator.

## 1. `ScenarioScript_MatchConditionExtract_IO.h/.cpp` — Shape 1

```cpp
ScenarioMatchConditionExtractionResult
    ExtractMatchConditionsFromScenarioScriptText(const std::string& sourceText);
```

Scans for `match = function(t, h, a, pattern) return ... end` fields (comments/strings skipped first,
via the shared lexer). Recognizes **exactly two** closed templates — anything else is a near-miss,
never partially extracted, never guessed:

- **Template A — AND-chain of count comparisons.** `return COND (and COND)*`, each `COND` = `VAR OP N`
  (`VAR` ∈ `{t, h, a}`, `OP` ∈ `==`/`~=`/`>`/`>=`/`<`/`<=`, `N` a signed integer literal). Any `or`
  anywhere in the expression, any `VAR` compared to another `VAR`, any arithmetic on `N`, or any
  identifier other than `t`/`h`/`a`/`pattern` is an automatic near-miss for that whole match function —
  never partially accepted. Each conjunct → one `Params::ScenarioCountCondition{field, comparator,
  value=N}` (`t`→`Total`, `h`→`HumanCount`, `a`→`AiCount`); the full chain → one
  `std::vector<ScenarioCountCondition>` (AND semantics, `§15.5`'s conjunction-only model).
- **Template B — slot-range occupancy.** `return pattern:sub(N, M):find("[^-]") ~= nil` — every token
  except the two integers `N`/`M` must match byte-for-byte (method names, the exact string literal
  `"[^-]"`, the `~= nil` comparison). A different string, method chain, or negated form is a near-miss.
  Maps to the **one fixed pairing** `ScenarioCountCondition{field=SlotRangeOccupiedCount,
  comparator=GreaterOrEqual, value=1, slotRangeStart=N, slotRangeEnd=M}` — the extractor never derives
  a comparator/value itself, always this one fixed pairing for this one fixed template.
- Each recognized `match` function yields one *candidate condition set* (a `std::vector<ScenarioCountCondition>`),
  tagged with whatever surrounding identifier context is available for the human to later recognize it
  by (e.g. the enclosing table's own `name = "..."` sibling field, read as plain context, never
  interpreted further) — but the result type is a bare vector of condition vectors, **never** a
  `CountScenario` or `ScenarioBody`. Wiring an extracted condition set to a scenario name/area is a
  separate human authoring action in the UI (`STEP266`), not this extractor's job.
- Caps: `kMaxScenarioMatchConditionExtractionCount` (candidate condition-sets per file) plus a per-chain
  conjunct-count cap (Template A) — pick generous-but-bounded values following
  `kMaxScenarioAreaExtractionRectangleCount = 512`'s precedent; document the chosen numbers in the
  header with the same "generous above any real scenario observed" framing.
- Result struct mirrors `ScenarioAreaExtractionResult`'s shape: extracted condition-sets in file order,
  a near-miss list (`identifier`/`reason`, reusing whatever context tag is available), a
  `bMatchConditionCountCapExceeded` flag.

## 2. `ScenarioScript_SlotPatternExtract_IO.h/.cpp` — Shape 2

```cpp
ScenarioSlotPatternExtractionResult
    ExtractSlotPatternsFromScenarioScriptText(const std::string& sourceText);
```

Scans a top-level `local <IDENT> = { { ... pattern = "STRING", ... }, ... }`-shaped array of tables
(the `PATTERN_SCENARIOS` shape) for the `pattern` field's string literal verbatim (ordinary Lua
string-literal decoding only, no additional escape processing) alongside its sibling `name` field
verbatim. Output is a **new, extractor-local struct** (never a full `Params::PatternScenario` directly
— that type requires a `ScenarioBody` this extractor does not and must not produce):
```cpp
struct ScenarioSlotPatternExtractionEntry { std::string name; std::string slotPattern; };
```
- Length/charset validation of `slotPattern` against the map's authored `maxArmySlotCount` is **not**
  this extractor's job — import as-authored; the existing `ScenarioSlotRangeValidation_IO.h`-family
  validator (or a sibling) flags a too-long/too-short pattern post-import, never silently
  truncated/padded here.
- Caps: `kMaxScenarioSlotPatternExtractionCount` (entries per file) and
  `kMaxScenarioSlotPatternStringLength` (an absurd-length guard, separate from the real
  `maxArmySlotCount` check, which happens later and needs the target map's actual value — this
  extractor has no map context).
- Result struct mirrors the area extractor's shape: entries in file order, a near-miss list, a
  `bSlotPatternCountCapExceeded` flag.

## 3. Shared machinery, both files

- Both use `ScenarioScript_LuaLiteralLexer_IO` (`STEP262`) for tokenization/comment-and-string
  skipping — no private re-implementation.
- **No Lua execution of any kind, ever** — same absolute rule as `ARCH_15_11` item 3, restated here
  because Shape 1's `and`/`or`/comparator syntax makes this the shape most likely to tempt "just
  evaluate the boolean expression." Do not.
- Pure and total: never throw, never touch the filesystem, never partially mutate output on a
  mid-scan failure (a rejected candidate contributes nothing and exactly one near-miss entry).
- File placement: `Io::` namespace, `ScenarioScript_*_IO` family, never `MapImporter_*` (`§15.2`).

## 4. Tests

1. **Template A**: each of the six real reference conjunctions (`1v1`, `4human`, `1h3ai`, `6total`,
   `2hRestAI`, `floor169` — get exact source text from
   `map_scripts_backup/Pandemonium Isthmus_Scenarios_Script.lua.officialbak`) extracts to the expected
   `ScenarioCountCondition` vector, comparator/value/field mapping exact.
2. **Template B**: `pattern:sub(5,8):find("[^-]") ~= nil` extracts to exactly
   `{field=SlotRangeOccupiedCount, comparator=GreaterOrEqual, value=1, slotRangeStart=5, slotRangeEnd=8}`.
3. **Near-miss, not partial extraction**: an `or`-bearing match function, a `VAR OP VAR` comparison, a
   comparator chained with arithmetic, and Template B with a different string literal or method name
   each produce zero extracted conditions plus one near-miss — never a partially-filled result.
4. **Slot pattern**: a `PATTERN_SCENARIOS`-shaped array with 3 entries extracts 3
   `{name, slotPattern}` pairs verbatim, including a pattern containing every allowed character
   (`-`/`h`/`a` or whatever the real corpus uses).
5. **Caps**: a synthetic file exceeding each count cap stops extraction at the cap and sets the
   corresponding `bXxxCountCapExceeded` flag, without crashing or unbounded-scanning.
6. **No execution**: confirm (by construction/code review in the test, e.g. asserting no LuaJIT/
   `LuaTableEvaluate_SYS` symbol is linked into either new translation unit) that neither extractor
   performs any Lua execution.

## 5. Out of scope

- Shape 3 (unit placements) and the shared orchestrator — `STEP265`.
- Wiring extracted candidates into the UI or into any `Params::Scenarios` structure — `STEP266`.
- Any widening beyond Templates A/B for Shape 1, or any wildcard/partial matching for Shape 2 — a
  future third shape is a new ARCH ratification, never a coder's inference.

## 6. Files touched

**New:** `src/io/ScenarioScript_MatchConditionExtract_IO.h`, `.cpp`,
`src/io/ScenarioScript_SlotPatternExtract_IO.h`, `.cpp`, plus test files for both.
