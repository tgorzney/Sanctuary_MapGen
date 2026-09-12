# STEP265 — Foreign-`.lua` unit-placement extractor (Shape 3) + full-data-import orchestrator

**Layer:** IO/BRIDGE. **Domain:** new `src/io/ScenarioScript_UnitPlacementExtract_IO.h/.cpp`, new
`src/io/ScenarioScript_FullDataImport_IO.h/.cpp`. **Executor:** SanGen Coder. **Sequence:** depends on
`STEP260` (`Params::ScenarioUnitPlacement`), `STEP262` (shared lexer), `STEP264` (the other two
extractors, composed by the orchestrator here alongside the existing area extractor). Implements
`ARCH_15_14_ForeignScenarioFullDataImportAndUnitPlacement.md` Part B, Shape 3, plus the orchestrator
the IO Architecture Expert specified.

**Read `ARCH_15_14` Part B (Shape 3) and `ARCH_15_11` items 1/8/9/10/11 in full before starting.**

## 1. `ScenarioScript_UnitPlacementExtract_IO.h/.cpp` — Shape 3

```cpp
ScenarioUnitPlacementExtractionResult
    ExtractUnitPlacementsFromScenarioScriptText(const std::string& sourceText, int mapSize);
```

Scans a top-level `local <IDENT> = { { armyName = "S1", templateIdentifier = "S2", x = X, y = Y,
z = Z [, rotation = { x=RX, y=RY, z=RZ, w=RW }] }, ... }`-shaped array of keyed table literals:

- The five keys `armyName`/`templateIdentifier`/`x`/`y`/`z` are **required**, any order; a missing key,
  an extra unrecognized key, or a non-literal value (arithmetic, identifier, function call) is a
  near-miss for **that one row only** — the rest of the array keeps scanning, never abort-the-whole-array.
- `armyName`/`templateIdentifier` — quoted string literals only.
- `x`/`y`/`z` — numeric literals only.
- **Binding rule (the one genuinely new risk this shape introduces): a row keyed by `armyIndex` instead
  of `armyName` is REJECTED as a near-miss naming exactly that ambiguity — never reinterpreted, never
  guessed across.** A raw `armyIndex` integer (the `pairs(Armies)` runtime handle) is not a stable
  `ARMY_XX` identity (`ARCH_15_14` Part A, "Corrections" item 3) and must never be treated as one.
- Field mapping: Lua `x`→`positionX`, `y`→`positionY` verbatim (no arithmetic); Lua `z`→`positionZ` via
  `mapSize - z - 1` (self-inverse of the exporter's `FlipPositionZ`, `STEP260`/`STEP263`) — **this
  extractor is therefore NOT context-free like the area extractor; it takes the target map's `mapSize`
  as an ordinary scalar parameter**, still zero filesystem access, zero Lua execution.
- Optional sixth key, a nested `rotation = { x=, y=, z=, w= }` table — **all four sub-fields required
  together if the key is present at all; a partial rotation table (e.g. only `x`/`y` present) is a
  near-miss for that row**, not a partial fill. Rotation maps verbatim (no flip) to
  `rotationX/Y/Z/rotationW`. **Absence of the whole `rotation` key is NOT a near-miss** — it defaults
  to identity `(0,0,0,1)`, since no known foreign shape (including the live reference) currently emits
  a literal rotation for units at all.
- **Ground-truth note, carried into this ticket's tests:** no file in this repository's
  `map_scripts_backup/` corpus currently contains a literal table of this exact shape — the real
  reference computes positions procedurally. This extractor is ratified as forward-looking
  infrastructure (other foreign maps, hand-transcribed/authoring-tool-produced files) — its test
  coverage must therefore be built from synthetic fixtures written to spec, not from the live corpus.
- Caps: `kMaxScenarioUnitPlacementExtractionCount` (rows per file); reuse
  `kMaxScenarioAreaExtractionRectangleCount`'s sibling `kMaxScenarioAreaCoordinateMagnitude` verbatim
  for `x`/`y`/`z` bounds (same map-scale-float semantic, no new constant needed).
- Result struct mirrors the area extractor's shape: `std::vector<Params::ScenarioUnitPlacement>` in
  file order, a near-miss list, a `bUnitPlacementCountCapExceeded` flag.

## 2. `ScenarioScript_FullDataImport_IO.h/.cpp` — the orchestrator

Mirrors `ScenarioScript_AreaImport_IO`'s shape (one banner/filename refusal guard reused verbatim, one
byte-size-capped disk read — 4 MiB, same cap as the area importer, one file/one read) composing **all
four** pure extractors (the existing area one plus these three new ones) over the same source text:

```cpp
struct ScenarioFullDataImportResult {
    ScenarioAreaExtractionResult          areas;              // existing item-9 auto-reconciliation
                                                                // into recipe.areas, UNCHANGED behavior
    ScenarioMatchConditionExtractionResult matchConditions;    // raw candidates -- NOT auto-attached
    ScenarioSlotPatternExtractionResult    slotPatterns;       // raw candidates -- NOT auto-attached
    ScenarioUnitPlacementExtractionResult  unitPlacements;     // raw candidates -- NOT auto-attached
    bool bRefusedAsSanGenOwnedFile = false;   // same guard as ScenarioScript_AreaImport_IO
};

ScenarioFullDataImportResult ImportFullScenarioDataFromScenarioScriptFile(
    const std::string& sourceFilePath, int mapSize, Params::MapRecipe& recipe);
```

**Only the area result auto-reconciles into `recipe.areas`**, exactly `ScenarioScript_AreaImport_IO`'s
existing item-9 collision policy, unchanged. Match-conditions/slot-patterns/unit-placements are **never**
auto-attached to any scenario record — `ARCH_15_14` Part B is explicit that wiring extracted data to a
name/area is "a separate human authoring action," not something this extractor infers or bundles. The
result struct surfaces three raw candidate lists for `STEP266`'s UI to present for manual assignment.

Reuse the existing `kScenarioGeneratedFileBannerLine` + SanGen-owned-filename refusal guard verbatim
(`ARCH_15_11` item 1) — one guard, not four independently invented copies.

## 3. Tests

1. **Shape 3 recognition**: a synthetic fixture with the full 5-key form, the 5-key+rotation form, and
   a row missing one required key (near-miss) or keyed by `armyIndex` (near-miss, exact rejection
   reason) — all behave per spec above.
2. **Z-flip round-trip**: a row authored with `z = 10` and `mapSize = 100` extracts to
   `positionZ == 89`; combined with `STEP260`'s importer, re-exporting recovers `z == 10`.
3. **Rotation absence vs. partial**: no `rotation` key → identity default, not a near-miss; a
   `rotation` key missing one of its four sub-fields → near-miss for that row.
4. **Orchestrator composition**: a synthetic file containing one of each shape (an area rectangle, a
   Template-A match function, a `PATTERN_SCENARIOS` entry, a Shape-3 unit-placement row) run through
   `ImportFullScenarioDataFromScenarioScriptFile` populates all four result fields correctly in one
   pass; only `recipe.areas` is mutated, the other three candidate lists are returned but leave
   `recipe.scenarios` untouched.
5. **Refusal guard**: a file bearing SanGen's own generated-file banner is refused wholesale (all four
   sub-results empty, `bRefusedAsSanGenOwnedFile == true`) — same behavior as
   `ScenarioScript_AreaImport_IO`'s existing guard, now shared.
6. **Byte cap**: a source file exceeding 4 MiB is refused/truncated per the existing area-importer's
   own established behavior (confirm and reuse, don't reinvent).

## 4. Out of scope

- UI wiring, the review/assign flow, and partial-success reporting — `STEP266`.
- Any general fallback capture for shapes that don't match one of the three ratified templates — not
  authorized by `ARCH_15_14`, not this ticket's job.

## 5. Files touched

**New:** `src/io/ScenarioScript_UnitPlacementExtract_IO.h`, `.cpp`,
`src/io/ScenarioScript_FullDataImport_IO.h`, `.cpp`, plus test files for both.
