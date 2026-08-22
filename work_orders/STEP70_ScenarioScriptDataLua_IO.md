# STEP70 — `ScenarioScript_DataLua_IO`: `Params::Scenarios` → `<MapName>_Scenarios_Data.lua` text

**Layer:** IO. **Domain:** Map Scenario Lua-rendering leg (export-only, `ARCH_15_03_ExportOnlyLuaRatified.md` §15.3), NOT the
`.sanmap` JSON leg. **Sequence:** Map Scenario IO track, `work_orders/DESIGN_MapScenarioIO_R1.md`
§6, Work-Order 5 of 8. Depends on **STEP63** (`LuaTableWriter_IO` — the only rendering primitives
this ticket may use) and **STEP69** (`Params::Scenarios`/`MapRecipe::scenarios` — the type this
ticket renders FROM). Optional test-only dependency on **STEP65** (`Sys::CheckLuaSyntax`, a
self-check of this ticket's own output). Unblocks **STEP71** (`ScenarioScript_Export_IO`, WO7),
which composes `BuildScenarioDataLuaText` as one of its three inputs.

## ⚠️ Naming correction — this ticket supersedes two stale names, follow the ratified spec verbatim

`DESIGN_MapScenarioIO_R1.md` §1/§2 (authored before `MAP_SCENARIO_SPEC.md` §2 was ratified) names
this file's output `<MapName>_Scenarios_Script.lua` and describes the *data* file itself declaring
the global `Scenario` table and importing a non-map-prefixed `SanGenScenarioRuntime.lua`. **Both
are stale.** The ratified `MAP_SCENARIO_SPEC.md` §2 / `ARCH_15_04_ThreeFileOnDiskShape.md` §15.4 three-file table supersedes
them:

| Ratified name | Owner | Role |
|---|---|---|
| `<MapName>_Scenarios_Data.lua` | **this ticket** | Declares `PATTERN_SCENARIOS`/`COUNT_SCENARIOS`/`DEFAULT_SCENARIO` as **globals**. Nothing else. |
| `<MapName>_Scenarios_Runtime.lua` | WO6 (not this ticket) | The generic algorithm. **Declares the global `Scenario` table** (`ResolveAndApply`/`SpawnNavalFleets`) and internally `Import()`s this ticket's `_Scenarios_Data.lua` to obtain the three tables above (`MAP_SCENARIO_SPEC.md` §2, "Link mechanism, extended"). Map-name-prefixed after all — the DESIGN doc's non-prefix exception did not survive ratification; its own flagged "must land in an ARCH/spec amendment" open item is retired by this: there is no exception to land. |

**Correction applied rather than silently followed:** the instruction "the generated file must
expose global `Scenario`" is true of the *live-debugging lesson* (global, not `local`, or `Import()`
silently yields nothing) but was aimed at the wrong file — under the ratified three-file split,
`Scenario` is the **Runtime** file's global (WO6), not this file's. This ticket's file exposes
**`PATTERN_SCENARIOS`/`COUNT_SCENARIOS`/`DEFAULT_SCENARIO`** as globals instead, for the identical
reason (`MAP_SCENARIO_SPEC.md` §2: the generated data file declares its tables as file-level
globals, never `local`, because it is the Runtime file's `Import()` that must capture them). The
acceptance test below targets the correct three names.

## Root problem
`ARCH_15_03_ExportOnlyLuaRatified.md` §15.3 ratifies export-only Lua rendering: SanGen owns `Params::Scenarios` (STEP69) and
renders `<MapName>_Scenarios_Data.lua` text on export, never parsing Lua back. No renderer from
`Params::Scenarios` to Lua text exists anywhere in `src/` today — confirmed, `grep -r
"Scenarios_Data" src/` and `grep -r "PATTERN_SCENARIOS" src/` both empty.

**This ticket's scope, precisely:** the pure, disk-free text builder only — one function, taking a
`Params::MapRecipe` and returning a complete `std::string`. It touches no filesystem (that's
STEP71's job) and knows nothing about the game-install path, the runtime file, or overwrite safety.
Mirrors `MapExporter_IO::BuildSanmapJsonText`'s shape exactly: pure builder in, whole-document text
out, round-trip-testable with no disk I/O.

## Fix

### 1. New file: `src/io/ScenarioScript_DataLua_IO.h`
```cpp
// ScenarioScript_DataLua_IO.h — renders Params::Scenarios (STEP69, `ARCH_15_05_ParamsScenariosType.md` §15.5) into the text of
// the generated <MapName>_Scenarios_Data.lua companion file (`ARCH_15_04_ThreeFileOnDiskShape.md` §15.4, MAP_SCENARIO_SPEC.md
// §2). Layer: IO. Pure, disk-free -- mirrors MapExporter_IO::BuildSanmapJsonText's own shape:
// PARAMS in, complete document text out, no filesystem touched here (that's ScenarioScript_Export_IO,
// STEP71). Composes ONLY LuaTableWriter_IO.h's generic primitives (STEP63) -- never JsonPrimitives_IO,
// never hand-rolled Lua string concatenation outside those primitives (DESIGN_MapScenarioIO_R1.md §1).
//
// Export-only, one-directional (`ARCH_15_03_ExportOnlyLuaRatified.md` §15.3): there is no matching "read this back" function
// anywhere in this file or its .cpp, and there never will be -- do not add one.
#pragma once
#include <string>

namespace SanmapGen {
namespace Params { struct MapRecipe; }
namespace Io {

// The exact literal every SanGen-owned scenario Lua file (this one AND the WO6 Runtime file) opens
// with (`ARCH_15_04_ThreeFileOnDiskShape.md` §15.4 point 2, MAP_SCENARIO_SPEC.md §2.1). SINGLE SOURCE OF TRUTH: STEP71
// (ScenarioScript_Export_IO)'s overwrite-safety check compares an existing file's first line
// against this exact string -- never re-typed as a second literal there. Whoever authors WO6
// (ScenarioScript_RuntimeResource_IO) MUST open the bundled runtime resource's own source text with
// this exact line too -- flagged here for that future ticket.
//
// ASCII-only by deliberate choice (plain "--" rather than an em-dash) -- maximizes grep-safety and
// avoids any encoding ambiguity inside a Lua source comment that different tools/editors may
// re-save under different encodings.
inline constexpr const char* kScenarioGeneratedFileBannerLine =
    "-- GENERATED BY SANGEN -- DO NOT HAND-EDIT (regenerated on every export)";

// recipe.scenarios -> the complete text of <MapName>_Scenarios_Data.lua, including the banner line
// above and the trailing global-table declarations. Pure and total -- never throws, never touches
// the filesystem. Takes the whole MapRecipe (not just Params::Scenarios) for the same reason
// BuildScenariosJson (STEP69) does: recipe.geometry.mapSize is needed for the position Z-flip (§3
// below), and recipe.mapName seeds the file's own header comment.
std::string BuildScenarioDataLuaText(const Params::MapRecipe& recipe);

} // namespace Io
} // namespace SanmapGen
```

### 2. New file: `src/io/ScenarioScript_DataLua_IO.cpp` — structure

Anonymous-namespace private helpers, composing `LuaTableWriter_IO.h`'s primitives exclusively:

- `AppendScenarioBodyFields(std::string& out, int indentLevel, const Params::ScenarioBody& body,
  int mapSize)` — appends the 10 `ScenarioBody` fields as key=value/nested-table lines **inside an
  already-opened table** (the caller opens/closes the outer `{ ... }`; this only fills it). Field
  order mirrors STEP69 §6's JSON emission order for direct cross-reference. Lua keys are
  **lowerCamelCase mirroring the C++ member names** (`name`, `area`, `navy`, `alloyMode`, `spawns`,
  `alloys`, `alloysToAdd`, `alloysToRemove`, `authoringNote`, `navalFleet`) — **not** the JSON's
  PascalCase spellings. Deliberate, IO-Architecture-owned call: `MAP_SCENARIO_SPEC.md` §2 leaves
  "exact Lua-rendering syntax... internal variable names" as coder/IO-tier, and Lua carries none of
  `SANMAP_FORMAT_SPEC.md` §1.6's JSON-casing law.
  - `name` → `AppendKeyValueLine(out, indentLevel, "name", QuotedLuaString(body.name))`.
  - `area` → `OpenTable(out, indentLevel, "area")`; 4 `AppendKeyValueLine` calls
    (`x=area.originX, y=area.originZ, width=area.width, height=area.length` — **the exact same
    field mapping `BuildAreasJson`/STEP69 §6 already use**, do not invent a different one);
    `CloseTable(out, indentLevel, true)`.
  - `navy` → `AppendKeyValueLine(..., RenderLuaBoolean(body.navy))`.
  - `alloyMode` → `AppendKeyValueLine(..., QuotedLuaString(kScenarioAlloyModeSpellings[
    static_cast<int>(body.alloyMode)]))` — reuses **STEP69's own `kScenarioAlloyModeSpellings`
    array verbatim** (`explicit`/`occupancy`/`keepAll`/`delta`), duplicated as a domain-local
    `constexpr` in this `.cpp` per STEP69 §5's "each file owns its own copy" precedent — do not
    `#include` `MapExporter_Scenarios_IO.cpp`.
  - `spawns`/`alloys`/`alloysToAdd` → `AppendArrayOfTables`, one flat row per element:
    `armyName = "<name>", x = <positionX>, y = <positionY>, z = <FLIPPED positionZ>`;
    `alloys`/`alloysToAdd` rows additionally carry `markerName`. **⚠️ coordinate flip on `z` only —
    see §3.**
  - `alloysToRemove` → `AppendArrayOfTables`, row = `armyName = "<name>", markerName = "<name>"`
    (no position — `ScenarioAlloyRemoval` carries none).
  - `authoringNote` → `AppendKeyValueLine(..., QuotedLuaString(body.authoringNote))` — real string
    data, never rendered as a `--` Lua comment (ARCH_15_05_ParamsScenariosType.md §15.5: "as real data now that this is no
    longer hand-authored Lua text").
  - `navalFleet` → `OpenTable(out, indentLevel, "navalFleet")`; inside: `fleet` via
    `AppendArrayOfTables` (row = `templateIdentifier = "<id>", count = <RenderLuaNumber(int)>`),
    `pondSideByArmy` via `AppendArrayOfTables` (row = `armyName = "<name>", side =
    <RenderLuaNumber(static_cast<int>(side))>` — **raw `-1`/`1`, never `QuotedLuaString`/
    enum-spelling lookup** — mirrors STEP69 §4's "do NOT use `ReadJsonEnumerationText` for `Side`"
    ruling, applied symmetrically on the write side), `sideBiasDistance` via
    `AppendKeyValueLine(..., RenderLuaNumber(body.navalFleet.sideBiasDistance))` (a `float`, so the
    `RenderLuaNumber(float)` overload); `CloseTable(..., indentLevel, true)`. **Always emitted, even
    when `navy == false`** — matches STEP69 §6's "NavalFleet: always emitted" rule; do not
    special-case it away.

- `BuildPatternScenariosTable(const std::vector<Params::PatternScenario>&, int mapSize) ->
  std::string` — `OpenTable(out, 0, "PATTERN_SCENARIOS")`; per element: open an anonymous nested
  table at indent 1 (`OpenTable(out, 1, "")`), `AppendKeyValueLine(out, 1, "pattern",
  QuotedLuaString(entry.slotPattern))` (Lua key `pattern`, matching `MAP_SCENARIO_SPEC.md` §4's
  `scenario.pattern`, **not** `slotPattern`), `AppendScenarioBodyFields(out, 1, entry.body,
  mapSize)`, `CloseTable(out, 1, true)`; `CloseTable(out, 0, false)`.
  **`AppendArrayOfTables` (STEP63) is NOT used here** — it assumes a flat, single-line row body;
  `ScenarioBody` is multi-field/nested (sub-table `area`, several sub-arrays, nested `navalFleet`),
  so each element is opened/closed manually with `OpenTable`/`CloseTable`, still composing nothing
  but STEP63's primitives.

- `BuildCountScenariosTable(const std::vector<Params::CountScenario>&, int mapSize) -> std::string`
  — same shape, except each element additionally gets a `conditions` field **before**
  `AppendScenarioBodyFields`: `AppendArrayOfTables(out, 2, "conditions", <one row per condition>)`
  where each row is `field = "<spelling>", comparator = "<spelling>", value = <RenderLuaNumber(int)>`.
  **⚠️ `field`/`comparator` string spellings are STEP69's own JSON spellings, reused verbatim**
  (`Total`/`HumanCount`/`AiCount`; `Equal`/`NotEqual`/`GreaterThan`/`GreaterOrEqual`/`LessThan`/
  `LessOrEqual`) — deliberately **one shared vocabulary** between the `.sanmap`'s JSON persistence
  and the rendered Lua, rather than a second translation table; WO6's runtime-side condition
  dispatcher keys off these exact strings.

  **The runtime consumes declarative condition TABLES (`field`/`comparator`/`value` triples), never
  a rendered Lua boolean expression or closure.** `MAP_SCENARIO_SPEC.md` §4's `match = function(...)`
  closure shape is the *legacy* hand-authored contract; the new SanGen-rendered contract is data,
  consistent with `ARCH_15_03_ExportOnlyLuaRatified.md` §15.3 and with `Params::ScenarioCountCondition`'s own
  `{field, comparator, value}` shape being a direct 1:1 source — no interpretation step on the C++
  side at all.
  **⚠️ Conditions within one `CountScenario` are conjunction-only (AND'd)** — `ARCH_15_05_ParamsScenariosType.md` §15.5's
  ruling; the emitted `conditions` array carries no OR/grouping structure, matching
  `Params::CountScenario::conditions`' flat `std::vector` shape exactly.

- `BuildDefaultScenarioTable(const Params::ScenarioBody&, int mapSize) -> std::string` — same
  element-body rendering, but a single anonymous table (`OpenTable(out, 0, "DEFAULT_SCENARIO")`,
  `AppendScenarioBodyFields(out, 1, body, mapSize)`, `CloseTable(out, 0, false)`) — never wrapped in
  an outer array; `DEFAULT_SCENARIO` is one record, not a list (`ARCH_15_06_CountScenariosOrdering.md` §15.6: only
  `countScenarios` carries an ordering requirement).

- `BuildScenarioDataLuaText(const Params::MapRecipe& recipe)` — the public entry point. Composes, in
  this exact order:
  1. `kScenarioGeneratedFileBannerLine` + `"\n"`.
  2. One informational comment line naming `recipe.mapName` and the source section
     (`"-- Source: <mapName>.sanmap \"Scenarios\" section (see MAP_SCENARIO_SPEC.md).\n\n"`) —
     cosmetic, never machine-checked, never re-parsed.
  3. **`MAX_ARMY_SLOT_COUNT = <recipe.scenarios.maxArmySlotCount>`** as a bare global assignment
     (`ARCH_15_10_SlotPatternConstructionMoves.md` §15.10 point 2) — `AppendKeyValueLine` is not used (that emits a trailing comma
     for a table member; this is a file-level global statement). Emit
     `"MAX_ARMY_SLOT_COUNT = " + RenderLuaNumber(recipe.scenarios.maxArmySlotCount) + "\n\n"`.
     Global, never `local` — the runtime `Import()`s this file and captures only globals, the
     same capture rule the three tables below depend on.
  4. `BuildPatternScenariosTable(recipe.scenarios.patternScenarios, recipe.geometry.mapSize)`.
  5. `BuildCountScenariosTable(recipe.scenarios.countScenarios, recipe.geometry.mapSize)` — **⚠️
     load-bearing: iterate in the `std::vector`'s own order, one `for` loop, no staging container of
     any kind** (`ARCH_15_06_CountScenariosOrdering.md` §15.6 — see acceptance test 1).
  6. `BuildDefaultScenarioTable(recipe.scenarios.defaultScenario, recipe.geometry.mapSize)`.
  Returns the concatenation as one `std::string`.

### 2b. ⚠️ Export-time validation — `maxArmySlotCount` vs. the authored army roster

`ARCH_15_10_SlotPatternConstructionMoves.md` §15.10 point 2 requires a **loud, logged, non-blocking** warning when
`recipe.scenarios.maxArmySlotCount < recipe.armies.size()`: at least one authored army can then
never appear in any `slotPattern` the runtime builds (`BuildSlotPattern` only marks
`player.armyID <= MAX_ARMY_SLOT_COUNT`), which is silent gameplay breakage, not a cosmetic
mismatch.

**Never clamp, never auto-raise the value, never refuse the export.** Silently overriding
authored data is the failure mode Constitution §6 forbids in the other direction. The warning
must **name the specific armies** that exceed the configured slot count, not just report a count
mismatch — the human needs to know which ones to act on.

⚠️ **Placement problem the coder must resolve, flagged not decided:** `BuildScenarioDataLuaText`
as specified in §1 returns a bare `std::string` and has nowhere to put a warning. Options: (a)
add an out-parameter (`std::vector<std::string>& outWarnings`), (b) return a small result struct,
or (c) let STEP71's orchestrator perform this validation instead, since it already owns a
`ScenarioExportResult` with a `debugLog`. **(c) is the recommended default** — it keeps this
ticket's builder pure and disk-free (its whole design premise, §1) and puts the warning where the
export result already surfaces. If (c) is chosen, this section becomes a STEP71 requirement and
this ticket only needs the acceptance test dropped; say which was chosen in the implementation.

### 3. ⚠️ Coordinate flip on `Position.z` — implement WITH it, same as STEP69 §9

`Params::ScenarioSpawn`/`Params::ScenarioAlloyOverride`'s `positionX/Y/Z` are the SAME in-memory
values STEP69's `BuildScenariosJson` reads — this ticket renders from `recipe.scenarios` directly
(not from parsed JSON), so it independently needs the identical flip STEP69 ruled on, for the
identical reason: every other `InstancedTransform`-shaped position field flips `z` on export.
**Apply it here too, independently — do not assume STEP69's JSON-side flip "covers" this separate
Lua-rendering path; they are two different render targets from the same source values.**

```cpp
// ⚠️ ATTENTION — COORDINATE FLIP UNCONFIRMED FOR SCENARIOS (Lua-rendering leg).
// Applies the SAME `mapSize - z - 1` flip STEP69's MapExporter_Scenarios_IO.cpp applies to the
// .sanmap JSON leg, for the same reason (every other InstancedTransform-shaped position field
// flips z on export). NOT independently ratified for this Lua-text leg by ARCH_15_05_ParamsScenariosType.md §15.5 or
// MAP_SCENARIO_SPEC.md — chosen for consistency with STEP69's own ruling, per the human's
// 2026-08-21 decision to implement now and verify later.
// IF SCENARIO SPAWNS/ALLOYS APPEAR MIRRORED ALONG Z IN-GAME, THIS IS THE FIRST PLACE TO LOOK —
// alongside the matching comment in MapExporter_Scenarios_IO.cpp (STEP69). Round-trip tests
// CANNOT catch a wrong choice here (there is no import path for this file at all, ARCH_15_03_ExportOnlyLuaRatified.md §15.3) —
// only in-game verification will.
```

`x`/`y` render unflipped (`RenderLuaNumber(positionX)`, `RenderLuaNumber(positionY)`); `z` renders
`RenderLuaNumber(static_cast<float>(mapSize) - positionZ - 1.0f)`.

## Files touched
- NEW `src/io/ScenarioScript_DataLua_IO.h` — the code block in §1 above, verbatim.
- NEW `src/io/ScenarioScript_DataLua_IO.cpp` — per §2/§3.
- NEW `src/io/ScenarioScript_DataLua_IO_Test.cpp`.
- `CMakeLists.txt` — one new `add_sangen_test(ScenarioScript_DataLua_IO_Test
  src/io/ScenarioScript_DataLua_IO_Test.cpp)` near the other Map-Scenario-track tests
  (`LuaTableWriter_IO_Test`, `Scenarios_IO_Test`). **No `nlohmann_json` link needed** (touches no
  JSON type). No extra LuaJIT link line needed even though the test calls `Sys::CheckLuaSyntax`
  (STEP65's precedent: the test uses only the `std::string`-in/`LuaSyntaxCheckResult`-out contract,
  never a LuaJIT symbol — transitively resolved via `SanGenV2`).

## Backend policy
N/A — pure CPU-side string building, called at most once per map export (not per-frame). No compute
dispatch, no SIMD, no GPU handle; does not touch `Dispatch_SYS`.

## ARCH rules invoked
- `ARCH_15_03_ExportOnlyLuaRatified.md` §15.3 — export-only rendering; this ticket is the render step.
- `ARCH_15_04_ThreeFileOnDiskShape.md` §15.4 point 2/3 — the banner-marker contract this output must satisfy so STEP71's
  overwrite-safety check can recognize it.
- `ARCH_15_05_ParamsScenariosType.md` §15.5 — the `Params::Scenarios` shape rendered here, verbatim (STEP69's type).
- `ARCH_15_06_CountScenariosOrdering.md` §15.6 — `countScenarios` array order is load-bearing.
- `MAP_SCENARIO_SPEC.md` §2 ("Link mechanism, extended") — the corrected file-role split this
  ticket's scope is grounded in.
- `MAP_SCENARIO_SPEC.md` §4 — the three-tier vocabulary (`pattern`/`conditions`) rendered here.
- STEP63 (`LuaTableWriter_IO`) — every primitive composed; none duplicated or hand-rolled.
- STEP69 — the type and the three string-spelling vocabularies reused verbatim, never reinvented.
- Constitution §6 — total, never-throwing render; an empty `Params::Scenarios{}` still renders a
  complete, syntactically valid file, never a partial/malformed document.

## Explicit out-of-scope
- **Any filesystem write, path resolution, or overwrite-safety check** — STEP71, which consumes this
  ticket's output as one of its three inputs.
- **`<MapName>_Scenarios_Runtime.lua`'s content, the bundled resource, or its own banner line** —
  WO6, not yet authored. This ticket only *defines* the shared banner constant that WO6's bundled
  resource text must also open with.
- **The `Scenario` global table, `ResolveAndApply`/`SpawnNavalFleets` wiring** — WO6's file, per the
  naming correction; this ticket's output never declares `Scenario`.
- **A Lua parser / round-trip read path** — `ARCH_15_03_ExportOnlyLuaRatified.md` §15.3 rules this out permanently, not deferred.
- **`GameInstallLocation_IO`, `LuaSyntaxCheck_SYS`** — STEP64/STEP65, already-authored tickets this
  one only optionally consumes (test-only, for STEP65).

## Acceptance test
New `src/io/ScenarioScript_DataLua_IO_Test.cpp` (registered in `CMakeLists.txt`):

1. **⚠️ Load-bearing — `COUNT_SCENARIOS` array order is rendered verbatim (`ARCH_15_06_CountScenariosOrdering.md` §15.6).**
   Fixture with 3 `CountScenario` entries named `"Zulu"`, `"Alpha"`, `"Mike"` (deliberately
   non-alphabetical) — assert `output.find("\"Zulu\"") < output.find("\"Alpha\"") <
   output.find("\"Mike\"")` (substring-position order proves emission order, not just presence).
2. **Global declaration, not `local`.** Rendered text contains `"PATTERN_SCENARIOS = {"`,
   `"COUNT_SCENARIOS = {"`, `"DEFAULT_SCENARIO = {"` and contains **none** of
   `"local PATTERN_SCENARIOS"`, `"local COUNT_SCENARIOS"`, `"local DEFAULT_SCENARIO"`. Also assert
   the text does **not** contain `"Scenario = {}"` or `"Scenario.ResolveAndApply"` — proves this
   file does not declare the `Scenario` global (WO6's job).
3. **Banner first line exact match.** `output.substr(0, strlen(kScenarioGeneratedFileBannerLine)) ==
   kScenarioGeneratedFileBannerLine`, and the banner is literally the first line.
4. **`alloyMode` renders all four spellings.** Four fixtures, one per `ScenarioAlloyMode`, each
   renders the exact quoted string (`"explicit"`/`"occupancy"`/`"keepAll"`/`"delta"`).
5. **Coordinate flip, deterministic.** `mapSize = 512`, `positionZ = 100.0f` on a `spawns` entry →
   the row contains `"z = 411"` (`512 - 100 - 1`) and not `"z = 100"`; `x`/`y` render unflipped.
6. **`conditions` spellings match STEP69's tables.** A `CountScenario` with 3 conditions spanning
   all 3 `ScenarioCountField` values and ≥3 distinct `ScenarioComparator` values renders the exact
   spelling strings (`"Total"`/`"HumanCount"`/`"AiCount"`, `"GreaterOrEqual"`, etc.).
7. **`navalFleet` always emitted.** A `navy == false` scenario still renders a `navalFleet = {`
   block with `fleet = {`/`pondSideByArmy = {`/`sideBiasDistance =` all present.
8. **`ScenarioNavalPondSide` renders as a raw signed integer, never a quoted string.** A `West`
   (`-1`) entry renders `"side = -1"`, not `"side = \"West\""`.
9. **Empty `Params::Scenarios{}` still renders a complete, non-empty file.** `PATTERN_SCENARIOS =
   {\n}` / `COUNT_SCENARIOS = {\n}` (empty tables, never omitted keys), `DEFAULT_SCENARIO = {` with
   struct defaults (`alloyMode == Occupancy` → `"occupancy"`), and `MAX_ARMY_SLOT_COUNT = 16`
   (the §15.10 default).
9b. **`MAX_ARMY_SLOT_COUNT` renders as a bare global, before the three tables.** A fixture with
   `maxArmySlotCount = 8` renders the exact substring `"MAX_ARMY_SLOT_COUNT = 8"`; its position in
   the output is **before** `output.find("PATTERN_SCENARIOS")`; and the text contains no
   `"local MAX_ARMY_SLOT_COUNT"` and no trailing comma after the value (it is a global statement,
   not a table member).
10. **Self-check via STEP65.** Full text of a fixture covering every field/tier/enum value passed to
    `Sys::CheckLuaSyntax` returns `bSucceeded == true` — proves the renderer's own output is
    syntactically valid Lua. `#include "../sys/LuaSyntaxCheck_SYS.h"` in the **test file only**;
    production code never depends on `SYS`.
11. Full solo rebuild + `ctest -C Debug`: previously-passing suite stays green; the new target passes.

## Verify
- New `src/io/ScenarioScript_DataLua_IO_Test.cpp` passes (all 11 assertions).
- Full solo rebuild + `ctest -C Debug`: 100% pass, zero pre-existing test files edited or broken.
