[← ARCH index](ARCH.md) · [§15 ARCH_15_MapScenarioSystem](ARCH_15_MapScenarioSystem.md) · SanGen ARCH §15.5. **Only the ARCH Expert writes this file.**

### 15.5 `Params::Scenarios` — the new PARAMS type (shape ruling)

**Binding shape** (`src/params/Scenario_PARAMS.h`, `Params::` namespace; naming per §1.1/§1.8 —
pass-through, human-authored round-trip data, so field names default to the format's own
spelling per §1.8's first bucket, converted for case only):

```cpp
enum class ScenarioAlloyMode { Explicit, Occupancy, KeepAll, Delta };  // §5, MAP_SCENARIO_SPEC

struct ScenarioSpawn         { std::string armyName; float positionX, positionY, positionZ; };
struct ScenarioAlloyOverride { std::string armyName; std::string markerName; float positionX, positionY, positionZ; };
struct ScenarioAlloyRemoval  { std::string armyName; std::string markerName; };

struct ScenarioBody {
    std::string name;                                    // log/debug identifier AND the
                                                            // dispatch key for unit spawning —
                                                            // see the OPEN item below, §11
    Params::MapArea area;                                 // reuse the existing §9 type — same
                                                            // world-space rect shape MAP_SCENARIO_SPEC
                                                            // §5 already describes for `area`
    std::string areaName;                                 // ADDED 2026-08-28, see "AMENDED" note
                                                            // below — empty (default) means "use
                                                            // `area` directly," today's exact
                                                            // behavior, fully backward compatible.
                                                            // Non-empty names a live
                                                            // `Params::MapArea::name` in
                                                            // `recipe.areas`, resolved into the
                                                            // wire `Area` key AT EXPORT TIME ONLY —
                                                            // at BOTH export legs, see the
                                                            // 2026-08-28 CORRECTED note below.
    bool spawnsUnits = false;                              // RENAMED 2026-08-28, was `navy` —
                                                            // see "RETIRED" note below. Generic
                                                            // opt-in only: true alone spawns
                                                            // nothing (MAP_SCENARIO_SPEC §11) —
                                                            // a matching name-keyed branch must
                                                            // also exist in the runtime dispatch
                                                            // (OPEN item below)
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
struct ScenarioCountCondition { ScenarioCountField field; ScenarioComparator comparator; int value = 0; };
struct CountScenario { ScenarioBody body; std::vector<ScenarioCountCondition> conditions; };  // TIER 2,
                                                            // conditions are AND'd (conjunction)

struct Scenarios {
    std::vector<PatternScenario> patternScenarios;   // TIER 1 — pattern, §4
    std::vector<CountScenario>   countScenarios;      // TIER 2 — ORDER IS LOAD-BEARING, §15.6
    ScenarioBody                 defaultScenario;     // TIER 3 — always matches, exactly one
    int                          maxArmySlotCount = 16;  // §15.10 point 2 — unaffected by this
                                                          // amendment, shown here for completeness
};
```
`MapRecipe::scenarios` — a flat sibling, same pattern as `armies`/`areas` (§9).

- **Why a flat sibling field set, not a tagged union/variant.** `alloys`/`alloysToAdd`/
  `alloysToRemove` sit side by side on `ScenarioBody`; only the field(s) matching the record's
  own `alloyMode` are meaningful — mirrors how Lua's untyped tables leave the non-selected fields
  simply absent, and matches this codebase's existing style (no `std::variant` precedent anywhere
  in `src/params/`); a variant would be more type-precise but is a new pattern this ratification
  does not introduce without cause.
- **Comparator vocabulary is deliberately small** (`Equal`/`NotEqual`/`GreaterThan`/
  `GreaterOrEqual`/`LessThan`/`LessOrEqual` × `Total`/`HumanCount`/`AiCount`, conjunction-only) —
  per the human's own settled framing: "every live predicate is a simple conjunction over
  total/human/AI counts, so a small vocabulary suffices." This is taken as given (the human's
  design ruling), not independently re-derived from the live Lua `match` closures, which this
  pack cannot read (external, game-install-only, `MAP_SCENARIO_SPEC.md`'s own sourcing caveat).
- ✅ **RATIFIED: `alloyMode`'s default is `Occupancy`.** Confirmed by the human (2026-08-21) —
  promoted from the structurally-safe placeholder this ruling originally used (it neither invents
  alloy data nor blanks it) to settled law; the field's own default-initializer above
  (`ScenarioAlloyMode::Occupancy`) already reflected it correctly and needs no code change.

---

### AMENDED 2026-08-28 — `ScenarioBody::areaName` (named-`Area` reference, additive)

**This is a shape amendment to an already-ratified type, recorded the same way the "RETIRED"
correction below is** — auditable, not a silent bolt-on. Human-approved design, independently
verified against the real code before ratification (`Scenario_PARAMS.h`,
`MapExporter_Scenarios_IO.cpp`, `MapImporter_ScenarioRecord_IO.cpp`, `ScenarioScript_DataLua_IO.cpp`,
`ScenariosTab_Detail_UI.cpp`, `MapArea_PARAMS.h`, `AreasTab_List_UI.h`,
`resources/lua/SanGenScenarioRuntime.lua`).

**What changes.** One new field, `std::string areaName` (default empty), added to `ScenarioBody`
immediately after `area` (shown in the binding shape above). Ratifies the human's explicit choice —
a `Scenario` may **reference** a named `Params::MapArea` from `recipe.areas`, not just hold a
disconnected private rectangle — over the simpler, explicitly-rejected "copy the area's values in
once" alternative. Empty `areaName` is the ONLY state meaning "no reference, use `area` directly" —
today's exact behavior. Every already-authored/exported scenario round-trips into exactly that
state: fully backward compatible, no migration entry needed.

**Export-time resolution — TWO independent legs, both need it (see the 2026-08-28 CORRECTED note
below for why this is now stated as two, not one).** SanGen exports two separate, never-merged
artifacts from the same `Params::Scenarios` data (`ScenarioScript_Export_IO.h`'s own header
comment: "Two independent calls, two independent result types, never merged"):
1. The `.sanmap` JSON leg — `MapExporter_Scenarios_IO.cpp`'s `BuildScenarioRecordJson`, called
   only from `BuildScenariosJson` → `MapExporter::BuildSanmapJsonText`.
2. The `<MapName>_Scenarios_Data.lua` leg — `ScenarioScript_DataLua_IO.cpp`'s
   `AppendScenarioBodyFields`, called only from `BuildScenarioDataLuaText`, itself invoked by
   `ScenarioScript_Export_IO.cpp`'s `ExportMapScenario` — **the file the game actually loads**
   (`MAP_SCENARIO_SPEC.md` §14).

Both builders currently read `body.area.originX/originZ/width/length` directly, and **both** must
resolve `areaName` against `recipe.areas` before reading those fields — the algorithm below is
identical at both call sites, ported/duplicated rather than shared, matching this exact file
family's own established "each leg owns its own copy" precedent (`ScenarioScript_DataLua_IO.cpp`'s
own header comment already states this for its `kAlloyModeSpellings`/etc. duplication versus
`MapExporter_Scenarios_IO.cpp`'s copies of the same tables — the same posture now applies one level
up, to the rect-resolution helper itself).

Resolution algorithm, first-match linear scan (mirrors this exact file family's own established
idiom — `AreasTab_List_UI.h`'s `ResolveAreaColor`, `UniqueNameList_UI.h`'s `NameIsTakenBefore` — both
already resolve/repair by first/earliest name match, never last-wins), applied identically at
**both** of the two call sites above:
1. `body.areaName` empty → emit the rect from `body.area` exactly as today. Unchanged code path.
2. `body.areaName` non-empty and found in `areas` (first match by `.name`) → emit the rect from the
   resolved `Params::MapArea`'s live values, not from `body.area`.
3. `body.areaName` non-empty and NOT found (a stale reference — the named Area was renamed or
   deleted after the scenario picked it) → emit the rect from `body.area` unchanged (the last
   live-preview copy the UI wrote, below) — never crash, never emit garbage — AND still emit the
   stale `AreaName` string as-authored on the JSON leg (never silently cleared: references are
   trusted strings never validated against a live roster anywhere else in this file, e.g.
   `ScenarioSpawn::armyName`/`ScenarioAlloyOverride::markerName`, so this is consistent, not a new
   posture). Both legs log a loud, non-blocking warning naming the scenario and the missing area —
   the same "loud, logged, never a silent fallback, never blocks the export" idiom `ARCH_15_10`
   point 2 already ratifies for `maxArmySlotCount`, not a new pattern; the two legs' warnings are
   wired through one new shared validator (the Format Expert's STEP209 §5 call), not two
   independently-invented copies of the warning text.
   - **Duplicate Area names.** `MakeNamesUnique(recipe.areas)` already exists to keep `recipe.areas`
     unique, but (confirmed by reading `AreasTab_UI.cpp:142`) it only runs while the Areas tab
     itself is drawn — a pre-existing gap this ruling inherits, does not introduce, and is not this
     amendment's to close. Should a duplicate somehow reach export, first-match-wins, same as every
     other name lookup already used in this exact codebase family.
- **Function-signature correction (both legs).** `BuildScenarioRecordJson`
  (`MapExporter_Scenarios_IO.cpp`) AND `AppendScenarioBodyFields`
  (`ScenarioScript_DataLua_IO.cpp`) must each widen to also take `const std::vector<Params::MapArea>&
  areas`, threaded from `recipe.areas` at every one of their existing call sites — a real
  signature change at both files, not a body-only edit confined to either function's current line
  range.

**Wire format — new additive key `AreaName`, no version bump. Ruling on the human's open question
1 (round-trip-the-reference vs. export-only-bake): ROUND-TRIP THE REFERENCE.** This ruling is about
the `.sanmap` JSON leg only — the Lua leg never carries an `areaName`/`AreaName` key at all, see the
STRING/NUMBERS split below. The exported `<ScenarioRecord>` gains one new sibling string key,
`"AreaName"`, immediately after `"Area"`, always emitted (even when empty — matching this record's
own established "every scalar field is always present" convention, e.g. `SpawnsUnits`/
`AuthoringNote`). Export-only baking is rejected: it loses the reference on every reload,
degrading the feature to exactly the "copy values in once" design the human explicitly chose
against. This codebase has direct, repeated precedent for additive wire keys carrying no
`SanGenVersion` bump (`SANMAP_FORMAT_SPEC.md` Corrections 12/14/17, per
`sangen_arch_pack/INDEX.md`'s own record) — this follows the same posture.
- `MapImporter_ScenarioRecord_IO.cpp`'s `ReadScenarioBodyJson` gains one line,
  `ReadJsonText(json, "AreaName", body.areaName)` — the same idiom as every other string field in
  that function. An absent key (every pre-existing `.sanmap`) leaves `body.areaName` at its struct
  default (empty); legacy files are unaffected, no migration entry needed.

**CORRECTED 2026-08-28 (same day, second pass) — the original "Zero changes anywhere else" claim
below was WRONG for the NUMBERS, right only for the STRING. Recorded here as a formal correction,
not a silent patch, per this file's own established audit style (see the "RETIRED" section below
for the precedent this follows).** The original text read: *"Zero changes anywhere else.
`ScenarioScript_DataLua_IO.cpp`, `resources/lua/SanGenScenarioRuntime.lua`, and the runtime's
`ResolveAndApply`/`ApplyScenario` never see `areaName`/`AreaName` at all — they only ever consume
the single resolved flat rect the exporter already writes into `area`/`Area`, exactly as today."**
This was verified against the wrong premise — that "the exporter" is one shared resolution point
for both SanGen-authored artifacts. Direct re-read (triggered by the Format Expert's STEP209 draft
finding the discrepancy) shows there is no such shared point:
- The **STRING** claim stands, unchanged and correct: `areaName`/`AreaName` genuinely never needs
  to reach Lua. `resources/lua/SanGenScenarioRuntime.lua`'s `ResolveAndApply` returns `chosenArea`
  straight from the matched scenario's own `area` field with no name-lookup capability of its own —
  there is nothing for a Lua-side `areaName` string to do even if it were rendered.
- The **NUMBERS** claim was wrong. `ScenarioScript_DataLua_IO.cpp`'s `AppendScenarioBodyFields`
  reads `body.area.originX/originZ/width/length` **directly and independently** of
  `MapExporter_Scenarios_IO.cpp`'s `BuildScenarioRecordJson` — the two builders live in different
  files, are never called from a shared parent, and (per `ScenarioScript_Export_IO.h`'s own header
  comment) are triggered by two entirely separate UI actions with two entirely separate result
  types (`MapExportResult` vs. `ScenarioExportResult`, "never merged"). Left as originally stated,
  the `.sanmap` JSON leg would re-resolve `areaName` fresh against `recipe.areas` on every export
  and get it right, while the Lua leg — **the file the game actually loads**
  (`MAP_SCENARIO_SPEC.md` §14) — would keep rendering whatever stale numbers happened to sit in
  `body.area`, the moment a referenced Area is resized without the scenario being reselected. The
  two SanGen-authored artifacts would silently diverge, and the one that matters for gameplay would
  be the wrong one — exactly the class of silent-wrong-result failure Constitution §6 forbids.
  **Ruled fix (already specified in full by the Format Expert, `work_orders/STEP209_
  ScenarioAreaNameReference_PARAMS_IO_UI.md` §5, not re-designed here): the identical
  resolved-rect-with-fallback algorithm above is threaded into `ScenarioScript_DataLua_IO.cpp`'s
  `AppendScenarioBodyFields` as a second, duplicated resolver (matching this file's own established
  per-leg-duplication precedent, cited above), and the same stale-reference warning is wired into
  `ScenarioScript_Export_IO.cpp` via one new shared validator header used by both legs.** This does
  not change the ratified algorithm (first-match resolve, fallback on miss, never crash, never
  silently clear the authored name) — it applies it at the second real call site this ruling's
  original verification missed.

**UI (`ScenariosTab_Detail_UI.cpp`'s `DrawScenarioAreaFields`) — corrections to the plan, both open
questions ruled.**
- The Combo is **not** a drop-in reuse of `DrawArmyNameField`'s exact shape — a real, load-bearing
  difference the plan's "built like the existing `DrawArmyNameField` pattern" framing understates.
  `DrawArmyNameField` has no concept of an explicit "no reference" choice — an empty `armyNameKey`
  there just means "not chosen yet," a transient authoring state. Here, empty `areaName` is a real,
  permanent, load-bearing authored state ("this scenario owns its own private rectangle") the human
  must be able to select back into deliberately. The Combo therefore needs one extra, leading
  sentinel entry (e.g. "-- Custom (no Area reference) --") `DrawArmyNameField` does not have.
  Selecting a real `recipe.areas` entry sets `body.areaName` to that `Params::MapArea::name` (its
  row label already IS its name — reuse `AreasTab_List_UI.h`'s existing `AreaRowLabel` directly, do
  not reinvent it) and copies its current rect into `body.area` (the live-preview copy the plan's
  item 4 already specified, unchanged). Selecting the sentinel clears `body.areaName` to empty.
- **Ruling on open question 2: the four rectangle sliders go READ-ONLY** (`ImGui::BeginDisabled`/
  `EndDisabled` — already an established, codebase-wide idiom, not a new pattern) **whenever
  `body.areaName` is non-empty.** Rejected alternative: editable-with-silent-clear-on-edit. A
  `DrawSliderScalar` drag is easy to nudge by a pixel unintentionally; silently detaching a
  scenario from its named Area on any stray edit, with no confirmation and no visible cause, is a
  worse authoring-safety hazard than a slider that visibly refuses to respond while a reference is
  active. The sliders stay **visible** (never hidden) so the resolved numbers are never a black
  box — only interaction is disabled. Detaching is only ever the deliberate, one-click
  sentinel-Combo action above.
- Stale reference in the UI: when `body.areaName` no longer matches any `recipe.areas` entry, the
  Combo's own selection-resolution loop (the same "no match ⇒ `selectedIndex == -1`" idiom
  `DrawArmyNameField` already uses) naturally shows neither the sentinel nor any real Area
  highlighted — sufficient signal; the export-time loud/logged warning above is the authoritative
  record of this state, not a UI toast duplicating it.

**Rejected alternatives, recorded so a future reader does not re-propose them:**
1. Export-only bake with no `AreaName` wire key — rejected; degrades to the human's
   explicitly-rejected "copy values in once" design.
2. Editable sliders with silent-clear-on-edit — rejected in favor of read-only-while-referenced,
   for the reason above.
3. Resolving `areaName` at only one of the two export legs (whichever was verified first) —
   rejected by the 2026-08-28 correction above; both legs read `body.area` independently and both
   must resolve it, or the Lua leg silently ships stale numbers.

---

### RETIRED 2026-08-28 — `ScenarioNavalFleet`/`ScenarioNavalFleetEntry`/`ScenarioNavalPondSide`/`ScenarioNavalPondAssignment`, `ScenarioBody::navy`

**This is a correction, not a silent deletion.** Recorded here so the removal is auditable.

**What existed.** The 2026-08-21 ratification shaped `ScenarioNavalFleetEntry`
(`templateIdentifier`/`count`), `ScenarioNavalPondSide` (`West=-1`/`East=1`),
`ScenarioNavalPondAssignment` (`armyName`/`side`), and `ScenarioNavalFleet`
(`fleet`/`pondSideByArmy`/`sideBiasDistance`), plus a `ScenarioNavalFleet navalFleet` member and a
`bool navy` opt-in flag on `ScenarioBody` — all shaped from a live read of a `Scenario.
SpawnNavalFleets(area)` function body in `Pandemonium Isthmus_Scenarios_Script.lua` as it existed
that day.

**Why they are gone.** That function no longer exists. A 2026-08-27 rewrite of the live reference
script (confirmed working in-game 2026-08-28, `Pandemonium Isthmus_Scenarios_Script.lua`, current
version) replaced the naval-only spawn machinery entirely:
- `Scenario.SpawnNavalFleets`, `NAVAL_FLEET`, `NAVAL_POND_SIDE_BY_ARMY`, `NAVAL_SIDE_BIAS_DISTANCE`,
  `NavalFindSpot`, and every other `NAVAL_*` tuning constant are deleted from the live file — there
  is no reader left anywhere in `SCEN` for the shape those four types described.
- The scenario record's opt-in field actually read is `spawnsUnits` (bool), not `navy`. `navy` is
  now a confirmed **dead field** — 5 live entries still carry `navy = true`/`false` in comments or
  history, but nothing reads it (`MAP_SCENARIO_SPEC.md` §6, §11.1). The live author has since
  removed it from the reference script's `4human` entry as well.
- Placement is no longer pond-side-assignment-based (a per-army `West`/`East` bias plus a spiral
  search against two hardcoded pond centroids). The replacement derives an anchor **live** from
  each army's own `Spawn` marker and searches both flanks for the deepest nearby water
  (`FindFleetAnchorForArmy`/`FindDeepestWaterNear`, `MAP_SCENARIO_SPEC.md` §11.1). There is no
  surviving concept of "which side of a named pond an army prefers" for this PARAMS shape to carry.
- Fleet **composition** (which units, how many) is no longer scenario-level structured data read
  by a generic executor from a `fleet`/`count` list. It is now inlined, per-generator, hand-authored
  Lua constants (e.g. `BATTLESHIP_TPID`, `BATTLESHIPS_PER_PLAYER_PER_POND`, `FIGHTER_TPID`,
  `FIGHTERS_PER_PLAYER` inside `BuildSlots5to8Instructions`) — see the OPEN item directly below for
  why this is not simply re-encoded as a renamed PARAMS field.

**What replaced them, in the live design (`MAP_SCENARIO_SPEC.md` §11, §11.1):**
1. `ScenarioBody::spawnsUnits` (bool) — the generic opt-in, replacing `navy`. Ratified above.
2. A name-keyed dispatch, `Scenario.SpawnMatchedScenarioUnits(area)`, which `if`/`elseif`s on the
   matched scenario's own `name` to call a per-scenario generator function, which builds a flat
   `{armyIndex, templateIdentifier, x, y, z}` instruction array.
3. A single generic executor, `Scenario.SpawnUnits(instructions)`, that only calls `CreateUnit` —
   it has no per-scenario knowledge at all.

`spawnsUnits = true` alone spawns nothing: a scenario also needs a matching branch in
`SpawnMatchedScenarioUnits` (`MAP_SCENARIO_SPEC.md` §11). This is a real, load-bearing two-step
opt-in, not a simplification lost in translation.

---

### OPEN — not resolved by this amendment, do not invent a shape

The live Lua does **not** settle a `Params`-side shape for two things this correction had to leave
out rather than guess at:

1. **The per-scenario unit-spawn generator is hand-authored placement code, not data.**
   `BuildSlots5to8Instructions` (the only live generator) derives anchors from live terrain samples
   (`Engine.SampleTerrainHeightFromCell`, `Engine.GetWaterLevel`), runs a deepest-water spiral
   search per army, and lays out a rectangular unit grid — real algorithmic logic, not a short list
   of tunable numbers the way the retired `ScenarioNavalFleet` was. Nothing in the live reference
   suggests a declarative shape that could replace it; whether SanGen should ever try to make
   per-scenario unit spawning author-able as PARAMS data (versus staying hand-written Lua a human
   edits directly, per `MAP_SCENARIO_SPEC.md`'s existing "SanGen owns data, not per-scenario spawn
   code" framing) is an open design question, not decided here. Do not add a `ScenarioUnitSpawn`
   struct, an instruction-list field, or any reuse of the existing `Params::UnitGroup`/
   `UnitTransform` family (§9) for this without a human ruling — none of those was evaluated against
   this use case.
2. **Where the per-scenario dispatch branches and generator functions live under the ratified
   three-file split is unresolved and looks like a real gap, not just an unanswered nice-to-have.**
   `ARCH_15_04`/`MAP_SCENARIO_SPEC.md` §14 describe `<MapName>_Scenarios_Runtime.lua` as *generic,
   identical across every map* (bundled, copied verbatim) and `<MapName>_Scenarios_Data.lua` as
   *pure per-map data tables, never containing algorithm code*. `Scenario.SpawnMatchedScenarioUnits`
   and its per-scenario generator functions (e.g. `BuildSlots5to8Instructions`) are neither: they
   are per-map, per-scenario, AND procedural Lua — a category the three-file split as currently
   ratified has no named home for. The live reference script has not migrated to the three-file
   split at all yet (`MAP_SCENARIO_SPEC.md` §2's divergence table), so this gap has not yet had to
   be resolved in practice. Flagging for a future ARCH ruling before any coder work-order attempts
   to render per-scenario unit-spawn generators from SanGen; not resolved by this amendment.
