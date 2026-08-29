# STEP204 — Retire the naval-fleet scenario feature across PARAMS / IO / UI; make Scenarios import+export correct

**Layers:** PARAMS, IO, UI. **Domain:** `Params::Scenarios` shape and its `.sanmap` + Lua round-trip.
**Sequence:** Map Scenario track. **Authorized by the human, 2026-08-28:** *"Fix SanGen so it
correctly imports and exports Scenarios. Old sanmap with the navy thing should be dropped (it is
deprecated)."*

## 0. Why

The 2026-08-27 rewrite of the live map script deleted `Scenario.SpawnNavalFleets` and every `NAVAL_*`
constant, replacing them with a generic `spawnsUnits`-gated path.
`ARCH_15_05_ParamsScenariosType.md` §15.5 was amended 2026-08-28 to match: it **retires**
`ScenarioNavalFleet`, `ScenarioNavalFleetEntry`, `ScenarioNavalPondSide`,
`ScenarioNavalPondAssignment`, `ScenarioBody::navalFleet` and `ScenarioBody::navy`, replacing the
opt-in with `ScenarioBody::spawnsUnits` (bool).

SanGen's own code never followed. 14 shipped files still carry the retired shape, including a whole
UI panel that edits a structure nothing reads. Left as-is, SanGen exports Lua that the live runtime
cannot read: it emits `navy`/`navalFleet`, the runtime looks for `spawnsUnits`, so every scenario
resolves falsy and unit spawning is **silently** off.

## 1. Human rulings that settle the design questions

- **Legacy data is DROPPED, not migrated.** Per the human: the `navy`/`NavalFleet` content of any
  existing `.sanmap` is deprecated. The importer must **silently discard** those keys, not error, not
  warn-and-halt, not translate them into `spawnsUnits`. A `.sanmap` written before this change loads
  cleanly with `spawnsUnits` defaulting to `false`.
- **`spawnsUnits` is a plain bool.** No fleet composition, no pond assignment, no placement data in
  PARAMS.

## 2. ⚠️ What this ticket does NOT do

`ARCH_15_05` §15.5 leaves two questions OPEN and this ticket resolves neither:

1. Whether per-scenario unit-spawn *placement* ever becomes declarative PARAMS data.
2. Where `Scenario.SpawnMatchedScenarioUnits` and its per-scenario generator functions live under the
   ratified three-file split.

**Do not invent an answer to either.** Where the UI previously offered a fleet editor, remove it —
do not design a replacement. This ticket is a retirement plus a correct `spawnsUnits` round-trip,
nothing more.

## 3. PARAMS — `src/params/Scenario_PARAMS.h` (13 refs)

- **DELETE** the types `ScenarioNavalFleet`, `ScenarioNavalFleetEntry`, `ScenarioNavalPondSide`,
  `ScenarioNavalPondAssignment` and every member using them.
- **DELETE** `ScenarioBody::navy` and `ScenarioBody::navalFleet`.
- **ADD** `bool spawnsUnits = false;` to `ScenarioBody`, defaulting false.
- Field naming follows `ARCH_01_08_ParamsFieldNamingByKind.md`; `.sanmap` key casing follows
  `ARCH_01_06_SanmapKeyCasing.md` (PascalCase → `SpawnsUnits`).

## 4. IO — export

`src/io/MapExporter_Scenarios_IO.cpp` (7 refs)
- Stop emitting the `NavalFleet` object entirely.
- Emit `"SpawnsUnits"` as a bool on every scenario record, **always present** (not omitted when false)
  — matching how the other scalar scenario fields are emitted today.

`src/io/ScenarioScript_DataLua_IO.cpp` (12 refs)
- Delete the `navalFleet` sub-builder outright. Do **not** port it to a renamed key.
- Render `spawnsUnits = true/false`. ⚠️ The Lua key spelling is fixed by the runtime, which reads
  `matchedScenario.spawnsUnits` — this is not a free IO-tier naming choice.

`src/io/LuaTableWriter_IO.h` (1 ref) — remove the naval mention; likely a comment only.

## 5. IO — import (the drop rule)

`src/io/MapImporter_ScenarioRecord_IO.cpp` (13 refs)
- Read `"SpawnsUnits"` into `ScenarioBody::spawnsUnits`. **Absent key ⇒ `false`**, no error — that is
  what every pre-STEP204 `.sanmap` will look like.
- **Silently ignore** any `"NavalFleet"`, `"Navy"`, `"PondSide"` or `"PondAssignment"` key. Do not
  translate, do not warn to the user, do not fail the load. Per §1 this data is deprecated and
  dropped.
- ⚠️ Confirm the unknown-key policy of the surrounding importer before relying on it. If the
  `Scenarios` reader has a strict "unknown key is an error" mode, the retired keys must be added to an
  explicit ignore list rather than silently falling through — a strict reader would otherwise reject
  every existing map. Check, and state which path you took.

## 6. UI

- **DELETE the file** `src/ui/ScenariosTab_DetailNaval_UI.cpp`. The build uses `GLOB_RECURSE` over
  `src/` (`CMakeLists.txt:177`), so no CMake edit is needed — verify that after deletion.
- `src/ui/ScenariosTab_UI.h` (2 refs) — remove the declaration(s) of the naval detail panel.
- `src/ui/ScenariosTab_Detail_UI.cpp` (3 refs) — remove the call into the naval panel; replace the
  `navy` checkbox with a `spawnsUnits` checkbox.
- `src/ui/ScenariosTab_DetailAlloys_UI.cpp` (5 refs) — remove naval references; this file's own alloy
  responsibility is unchanged.
- ⚠️ The `spawnsUnits` checkbox needs a caption making the **two-step opt-in** explicit: setting it
  true does nothing on its own, because a matching dispatch branch must also exist in the map's Lua.
  Do **not** attempt to validate that such a branch exists — that is OPEN item 2's territory.

## 7. Bundled runtime — `resources/lua/SanGenScenarioRuntime.lua` (33 refs)

Apply STEP72's §2 correction in full:
- Delete `local currentNavalFleet` and the entire naval block (`NAVAL_*` constants, every `Naval*`
  helper, `Scenario.SpawnNavalFleets`).
- `ResolveAndApply` reads `matchedScenario.spawnsUnits`; log line says `spawnsUnits=%s`.
- Add the generic `Scenario.SpawnUnits(instructions)` executor exactly as STEP72 §2 specifies —
  checking **both** `ok` and `unit`.
- ⚠️ **Do NOT add `Scenario.SpawnMatchedScenarioUnits`, not even a no-op stub.** Blocked, OPEN item 2.
- ⚠️ **Fix the carried-forward bug while you are here:** the deleted naval code contained an
  **unguarded** `army.lobbyOptions.isEmptySlot`. `lobbyOptions` is nil on some army entries and an
  unguarded dereference throws inside a `NewThread` callback, where the error is swallowed. Any
  surviving occurrence must read `army.lobbyOptions and army.lobbyOptions.isEmptySlot`, treating nil
  as **OCCUPIED**. See `MAP_UNIT_SPAWNING_SPEC.md` §5.

## 8. Tests and fixtures

- `src/io/MapImporter_Scenarios_IO_Test.cpp`, `src/io/ScenarioScript_DataLua_IO_Test.cpp`,
  `src/io/ScenarioScript_RuntimeResource_IO_Test.cpp` — update to the new shape.
- `src/io/testdata/FormattingReference.sanmap` — remove the `NavalFleet` content.
- **Required new coverage:**
  1. Round-trip `spawnsUnits = true` and `spawnsUnits = false` through `.sanmap` export→import.
  2. **A legacy fixture** containing `NavalFleet`/`Navy` keys imports cleanly, drops them, and yields
     `spawnsUnits == false`. This is the human's explicit ruling and must be pinned by a test.
  3. Negative assertions that no output contains `NavalFleet`, `navy`, `NAVAL_` or
     `SpawnNavalFleets` — proving removal, not mere supplementation.
  4. `ScenarioScript_DataLua_IO` renders `spawnsUnits` in both states.

## 9. Verify

- Full solo rebuild + `ctest -C Debug`: 100% pass.
- `grep -rn "navy\|navalFleet\|NavalFleet\|NavalPond\|NAVAL_\|SpawnNavalFleets" src/ resources/`
  returns **only** comments that describe the retirement historically — no live code, no live data.
- Confirm `ScenariosTab_DetailNaval_UI.cpp` is gone and the build still configures and links.

## 10. Out of scope

- Anything touching the two ARCH OPEN questions (§2).
- `Scenario.SpawnMatchedScenarioUnits` or any per-scenario generator, in any form.
- The `ARMY_ID_TO_NAME`/`KNOWN_ALLOY_MARKERS` renderer gap — separate, pre-existing.
- Any file under the live game install. This ticket touches the SanGen repo only.
