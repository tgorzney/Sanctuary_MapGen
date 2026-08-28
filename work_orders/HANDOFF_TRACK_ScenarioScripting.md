# HANDOFF — Map Scenario Scripting track

*Written 2026-08-21 for the consolidation session. This file is the only artifact that survives the
originating session. **No code was written by this track — work-order authoring only.** Verified:
zero scenario/Lua files exist under `src/params/`, `src/io/`, `src/ui/`, `src/sys/`.*

**ARCH restructure acknowledged.** All my tickets were authored against monolithic `ARCH.md §15.x`.
Another session has already rewritten the `ARCH.md` references inside STEP63/64/65/70/74 and
`DESIGN_MapScenarioIO_R1.md` to the new `ARCH_15_NN_*.md` filenames. ⚠️ **Not all my files were
updated** — STEP69/71/72/73/77/78 and `DESIGN_ScenariosTabAndLuaEditor_R1.md` still cite bare
`ARCH.md §15.x`. Section numbers are unchanged and still resolve conceptually, but the filenames are
stale. See §G.7.

---

## A. TRACK IDENTITY

**Domain:** The SanGen Map Scenario system — the game-side Lua scenario-scripting mechanism
(`ARCH_15_*`, `sangen_arch_pack/specs/MAP_SCENARIO_SPEC.md`) plus SanGen's own PARAMS/IO/UI for
authoring and exporting it.

**Claimed numbers:** STEP63, 64, 65, 69, 70, 71, 72, 73, 74, 77, 78.
Plus `BRIEF_ScenarioScriptingRatification.md` (input, authored by another session),
`DESIGN_MapScenarioIO_R1.md`, `DESIGN_ScenariosTabAndLuaEditor_R1.md`.
**Vacated:** STEP75, STEP76 (see §F.4).

---

## B. WORK ORDERS WRITTEN — all 11 complete, no TODO holes

Status vocabulary: **RATIFIED** = grounded in landed ARCH/spec law, ready for a Coder once its
dependencies land. None are DRAFT or STUB.

| File | Layer | Status | Notes |
|---|---|---|---|
| `STEP63_LuaTableWriter_IO.md` | IO | RATIFIED | Header-only Lua-literal primitives. Zero deps. Implementable now. |
| `STEP64_GameInstallLocation_IO.md` | IO (+UI touch) | RATIFIED | `gameInstallRoot`/`scenarioRuntimeOverridePath` on `AppSettings`, `ValidateGameInstallRoot`. Zero deps. Implementable now. ⚠️ Edits 4 pre-existing files. |
| `STEP65_LuaSyntaxCheck_SYS.md` | SYS | RATIFIED | Compile-only validator + **LuaJIT** (not vanilla Lua) vendoring. Zero deps. Implementable now. |
| `STEP69_ParamsScenariosRoundTrip_IO.md` | PARAMS + IO | RATIFIED | `Params::Scenarios` + `.sanmap` round-trip. **Foundational — every other ticket consumes this type.** |
| `STEP70_ScenarioScriptDataLua_IO.md` | IO | RATIFIED | Renders `<MapName>_Scenarios_Data.lua` text. Deps: STEP63 + STEP69. |
| `STEP71_ScenarioScriptExport_IO.md` | IO | RATIFIED | Export orchestrator + `ScenarioExportResult`. Deps: STEP64, STEP70, STEP72. |
| `STEP72_ScenarioRuntimeResource_IO.md` | IO + `resources/lua/` | RATIFIED | Bundled runtime `.lua` (full algorithm port), resolver, CMake staging. Deps: STEP69/70. |
| `STEP73_ScenarioAlloyRosterRender_IO.md` | IO | RATIFIED | Renders `ARMY_ID_TO_NAME`/`KNOWN_ALLOY_MARKERS`. Deps: STEP63/69/70. |
| `STEP74_ScenariosTab_CoreAuthoring_UI.md` | UI | RATIFIED | Scenarios tab: lists, rule authoring, matrix, spawns warning, `maxArmySlotCount`. Dep: STEP69 only. |
| `STEP77_ScenariosLuaEditorAndExportFlow_UI.md` | UI | RATIFIED | `LuaCodeEditor_UI` + `ExportScenarioScript` Files-tab flow. Deps: STEP64/65/71/72/74. |
| `STEP78_ScenarioEditModeCanvas_UI.md` | UI | RATIFIED but **GATED** | Canvas marker editing. **Do not dispatch** — see §D.1. |

**Design documents (not tickets):**
- `DESIGN_MapScenarioIO_R1.md` — IO-side design. ⚠️ §1/§2 use superseded filenames
  (`MapExporter_MapScenario_IO`, `<MapName>_Scenarios_Script.lua`, non-prefixed
  `SanGenScenarioRuntime.lua`); STEP70/71/72 carry the corrections inline. Read tickets over design.
- `DESIGN_ScenariosTabAndLuaEditor_R1.md` — UI design. ⚠️ §1's `ScenarioSettings` and §7's
  editor model are superseded; STEP74/77 carry corrections inline. Format-Expert questions 6/7/8
  are answered inline in the file.

---

## C. WORK ORDERS NOT YET WRITTEN

**Only one is known-needed and unwritten in this track:**

| Intended filename | Layer | One-line scope |
|---|---|---|
| *(unclaimed — not mine)* `STEP__NextArmyNameConvention_UI.md` | UI (+IO validation) | Fix `NextArmyName` producing `Army1`-style names; add export-time `ARMY_XX` validation; decide migration for already-exported maps. See §D.2 and §G.5. |

**Deliberately NOT written (out of track, or correctly excluded):**
- Nothing else. The 8-work-order plan from `DESIGN_MapScenarioIO_R1.md` §6 expanded to 11 and is
  complete.

---

## D. BLOCKED / IN DESIGN

**D.1 — `STEP78_ScenarioEditModeCanvas_UI.md` — blocker type (ii), another track.**
Needs **five** unlanded tickets from the preview-overlay/marker tracks, not the one originally
assumed: **STEP47** (`WorldScreenProjection_UI` — world↔screen math), **STEP50** (CSR bucket index),
**STEP51** (`OverlayLayer_UI` data model — the ghost-baseline reuse target), **STEP52** (atlas
pairing), **STEP53** (`MapCanvas_IconLayer_UI` — the icon draw pass). Verified none exist in `src/`.
Minimum to start: STEP47 + STEP51. **This blocks nothing else** — STEP74 + STEP77 deliver a
complete authoring surface without it (flat-list numeric editing instead of canvas dragging).

**D.2 — `NextArmyName` fix — blocker type (iii), human decision on ownership.**
Genuinely unclaimed; confirmed no ticket exists. Adjacent session `map-generator-6d` was offered it
and has it under consideration. Not mine — my track consumes conforming army names but does not own
the Armies tab. See §G.5 for the full bug.

**D.3 — `MatchesScenarioConditions` placement — blocker type (i), ARCH ruling.**
STEP74 places the pure condition evaluator UI-local (`ScenariosTab_UI.h`), on the same footing as
`ArmiesTab_UI.h`'s existing pure helpers. The original design non-bindingly suggested MATH
promotion. Flagged in the ticket for ARCH confirmation. **Non-blocking** — if ARCH rules MATH, it is
a file move, not a redesign.

**D.4 — `maxArmySlotCount` export-time warning home — blocker type (v), unresolved but worked around.**
`ARCH_15_10` point 2 requires the warning. STEP70 §2b recommended STEP71 host it; **STEP71 does not
mention `maxArmySlotCount` anywhere** (verified). STEP74 now surfaces it UI-side as an always-visible
banner, so the requirement is met — but nothing emits it into `ScenarioExportResult::debugLog` as
§15.10's text implies. Needs either a STEP71 amendment or an explicit ruling that UI-only is enough.

**D.5 — STEP74's positional army-ID assumption vs STEP73's alphabetical sort — blocker type (v).**
`ArmiesExceedingSlotCount` (STEP74) assumes army ID == 1-based position in `recipe.armies`.
STEP73 §0 subsequently established the engine derives slot order by **alphabetical `Army::name`
sort**. Both are flagged in-file. These must be reconciled when both land — a map whose declaration
order differs from alphabetical order will name the wrong armies in the warning. **Not yet
reconciled.**

**D.6 — Bundled-vs-override Lua staleness detection — blocker type (v).**
STEP77 ships a plain "differs from bundled" banner. A real "forked from version X, bundled now at Y"
mechanism needs a stored baseline (e.g. a hash beside `scenarioRuntimeOverridePath`) that no
ratified field provides. Flagged, not designed.

---

## E. HUMAN DECISIONS PENDING

**E.1 — Who owns the `NextArmyName` fix?**
Options: (a) the adjacent army-mirror session (`map-generator-6d`) absorbs it — they found it,
STEP76 is free; (b) this track takes it; (c) a fresh ticket for whoever picks up the Armies tab.
**Recommendation: (a).** No decision received.

**E.2 — Migration policy for maps already exported with `Army1`-style names.**
Options: (a) loud import-time warning only, never auto-rename; (b) an import-time repair pass that
renames and rewrites `markers.Spawn.transforms` keys; (c) do nothing, treat pre-fix exports as
disposable. **Consequential** — (b) touches shipped `.sanmap` files. No decision received.

**E.3 — Should the live `Pandemonium Isthmus` map's incomplete scenarios be hand-fixed now, or
wait for the Scenarios tab (STEP74)?**
`2h1ai`, `6total`, `2hRestAI`, `floor169` all lack explicit `spawns` and therefore inherit the
`.sanmap` baseline — the live bug the human observed (3-player lobby matched correctly, applied the
right area, spawned at another composition's positions). Fixing in Lua now is ~20 lines; waiting
means the map stays wrong for those compositions. **Raised twice, no decision received.**

**E.4 — Is UI-only sufficient for the `maxArmySlotCount` warning (D.4), or must STEP71 also log it?**

**E.5 — `MatchesScenarioConditions` placement (D.3): UI-local or MATH?**

---

## F. CROSS-TRACK DEPENDENCIES

**F.1 — OWED TO ME (blocking STEP78 only):** STEP47, STEP50, STEP51, STEP52, STEP53 from the
preview-overlay / marker-symmetry tracks. Ordering: STEP47 + STEP51 before STEP78 can start;
STEP52 + STEP53 before it can finish.

**F.2 — OWED TO ME (blocking nothing, informational):** the `NextArmyName` fix (§D.2). My track
*consumes* conforming `ARMY_XX` names — STEP73's `ARMY_ID_TO_NAME` derivation and every
`armyName`-keyed scenario field assume them. If SanGen keeps emitting `Army1`, scenario export
produces data the engine cannot match. **Ordering: the fix should land before any SanGen-authored
map with scenarios is shipped**, but not before any of my tickets are implemented.

**F.3 — OWED BY ME:** nothing. No other track consumes my outputs.

**F.4 — Numbering history (so nobody re-collides):** I originally took STEP75/76; `map-generator-6d`
had claimed STEP75 for `STEP75_ArmyMirrorSymmetry_UI.md` one minute earlier. **I renumbered mine to
77/78** and fixed all cross-references. **STEP76 is FREE** — I vacated it and never used it.

**F.5 — Internal ordering within my track:**
```
STEP63, STEP64, STEP65   (zero deps — implementable in any order, in parallel)
STEP69                   (foundational — everything below needs Params::Scenarios)
  ├── STEP70   (needs 63 + 69)
  │     └── STEP73   (needs 63 + 69 + 70)
  ├── STEP72   (needs 69 + 70)
  │     └── STEP71   (needs 64 + 70 + 72)
  └── STEP74   (needs 69 only)
        └── STEP77   (needs 64 + 65 + 71 + 72 + 74)
              └── STEP78   (GATED on STEP47/50/51/52/53)
```

---

## G. UNCOMMITTED CONTEXT — facts that exist only in conversation

**G.1 — Cross-tree `Import()` does NOT work. Live-proven, and it reversed a ratified claim.**
A `.lua` in `Sanctuary_Data/Maps/<Map>/` **cannot** be `Import()`ed from `LJ/lua/`. Proven in-game
by calling `Engine.FileExists(libPath.."/"..path)` — the exact string `import.lua` builds internally
— with a `../../Sanctuary_Data/...` path: returns **false**. This answers
`Sanctuary_Map_System_Rework.md` §12 open question 6 **negatively**.
⚠️ An earlier "2 BigBots spawned" test was recorded as confirming the opposite; it was a **false
positive** and the spec was corrected. The human's original design intent (`_data.lua` stays in
`LJ/lua/`, scenario file in the map folder) is therefore **impossible** — both files must live in
`LJ/lua/maps/<MapName>/`.

**G.2 — `Import()` ignores a module's `return` entirely; it captures only GLOBALS.**
From `common/systems/import.lua`'s own doc comment. A module written `local X = {} … return X`
yields a table with **none** of its fields, silently, with no error. This cost a full live debugging
session before being found. Now first-class law in `MODDING_SCRIPTING_SPEC.md`'s "Lua runtime &
sandbox" section, and the reason STEP70/72 test for `Scenario = {}` and forbid `local`.

**G.3 — F1 in-game console is unreliable in this build.**
`MODDING_SCRIPTING_SPEC.md` recommends it as the primary diagnostic. During live testing it showed
**nothing** while confirmed-running code executed; `game_logs/*.txt` stayed empty too. All live
verification in this track was done by **spawning countable in-game objects** (BigBots, then alloy
markers in distinctive straight rows) as a visual signal channel. Recorded in the spec as ⚠️
unresolved.

**G.4 — The live `Pandemonium Isthmus` map is currently DEPLOYED with the orchestrator/scenario
split and working.**
`LJ/lua/maps/Pandemonium Isthmus/` holds `Pandemonium Isthmus_data.lua` (orchestrator) and
`Pandemonium Isthmus_Scenarios_Script.lua` (all scenario content), linked by
`Import("maps/Pandemonium Isthmus/Pandemonium Isthmus_Scenarios_Script.lua").Scenario`. Confirmed
working in a live 1v1. Backups on disk: `..._data.lua.backup-2026-08-20-presplit` (pre-split
single-file), `..._Scenarios_Script.lua.full-backup-2026-08-20`.
⚠️ **This live filename (`_Scenarios_Script.lua`) is the LEGACY name.** The ratified design uses
`_Scenarios_Data.lua` + `_Scenarios_Runtime.lua`. STEP71 §2.2 and `MAP_SCENARIO_SPEC.md` §2.2 handle
this via filename disjointness — SanGen never touches the legacy file. Migration is a one-time human
edit per map.

**G.5 — `NextArmyName` produces non-conforming army names. Real, shipped, unclaimed bug.**
`src/ui/ArmiesTab_UI.h:77` → `NextUniqueLabel("Army", count)` → `Army1`, `Army2`.
`src/io/MapExporter_Armies_IO.cpp:75` → `armies[army.name] = armyJson;` — the name becomes the
`.sanmap` JSON key verbatim, no normalization. Real maps use `ARMY_01`…`ARMY_06` (verified in the
shipped Pandemonium `.sanmap`).
**Breaks three things:** (1) engine slot assignment (alphabetical sort, wrong prefix AND no padding
— `Army1`/`Army10`/`Army2`); (2) `markers.Spawn.transforms` lookup (keyed by army name);
(3) the entire scenario system (everything keys off `armyName`).
**Origin:** `STEP20_ArmiesTab_UI_Wiring.md` explicitly left it open — *"either is a mechanical Coder
choice"* — so the Coder used the house `Area`/`NewArea` style. Nobody knew the format was
load-bearing until 2026-08-21.

**G.6 — `ARMY_XX` zero-padding is load-bearing, confirmed by the human.**
The engine assigns lobby slots by sorting army names **alphabetically** (`common/gameUtils.lua`'s
`CreateArmies()` correlating `armyID` against `mapStartSlotIndex`). Because it is a *string* sort,
padding is functional: `ARMY_01`…`ARMY_10` sorts correctly; `ARMY_1`/`ARMY_2`/`ARMY_10` sorts as
1, 10, 2 — correct to 9 armies, **silently wrong from 10 onward**. Maps support 16 slots.
`Params::Army::name` has **no format constraint today**. Recorded in
`.claude/agents/sangen-format-expert.md`.

**G.7 — Stale `ARCH.md` references in my files after the split.**
Another session updated STEP63/64/65/70/74 and `DESIGN_MapScenarioIO_R1.md` to `ARCH_15_NN_*.md`
filenames. **Still citing bare `ARCH.md §15.x`:** STEP69, STEP71, STEP72, STEP73, STEP77, STEP78,
`DESIGN_ScenariosTabAndLuaEditor_R1.md`. Section numbers are unchanged, so nothing is *wrong* —
only the filenames are stale. Worth a mechanical sweep.

**G.8 — The `.sanmap` tolerates unrecognized top-level sections. Live-verified.**
Injected `{"SanGenScenariosTest":{"magic":12345}}` into the real Pandemonium `.sanmap`, launched a
real 1v1: loaded and played normally. `LoadMapData` builds `GameInfo.MapData` from an **explicit
whitelist** (`props, decals, areas, armies, markers, chains, groups`) and silently drops everything
else. This is why `SANMAP_FORMAT_SPEC` Correction 17's `Scenarios` section is safe, and why the
generated data `.lua` is needed as the transport to Lua today (`ARCH_15_09` records the future
one-line engine change that would retire it).

**G.9 — A `.sanmap` reformatting incident, resolved.**
I rewrote the Pandemonium `.sanmap` via Python `json.load`/`json.dump(indent=4)` during G.8's test.
Semantically identical but fully reformatted (key order, whitespace, float style). Session
`map-generator-56` restored the original formatting from my backup; I re-verified it (valid JSON,
288 alloy transforms, spawns intact). My reformatted copy is preserved as
`...sanmap.backup-2026-08-21_python-reformatted`.
⚠️ **The human wants the original `.sanmap` formatting preserved as the reference SanGen exports
should eventually match.** Nothing currently specifies exporter output formatting (key order, float
style, indentation) — **that is an unwritten requirement with no ticket.** Blocker type (iv).

**G.10 — Window-close hang, unexplained.**
While a 543-line `.lua` sat in `Sanctuary_Data/Maps/Pandemonium Isthmus/`, the game hung on window
close and needed a force-quit; replacing it with a 9-line stub stopped the hang. Both files were
failing to load via `Import()` at the time, so the mechanism is unexplained — possibly an
asset-folder scanner reacting to a large `.lua` in the map asset tree. Recorded in
`MODDING_SCRIPTING_SPEC.md` as ⚠️ unverified. No `.lua` now lives in that folder.

**G.11 — `.claude/agents/sangen-coder.md` was never updated for this track.**
`DESIGN_MapScenarioIO_R1.md`'s closing bullet specifies a briefing addition (the `ScenarioScript_*_IO`
convention is export-only and composes `LuaTableWriter_IO`, never `JsonPrimitives_IO`; never a
`<Domain>_Migrate_V<N>_IO` candidate). STEP72 adds a second: `resources/lua/` mirrors `shaders/` as
the shipped-resource precedent, so no new ARCH layer is needed next time. **Neither applied** —
verified zero matches for `Scenario`/`LuaTableWriter`/`resources/lua` in that charter.

**G.12 — Charters updated by this track (already applied, for the record):**
`sangen-format-expert.md` (Map Scenario pointer + the `ARMY_XX` law, G.6),
`sangen-io-architecture-expert.md` (design question settled → option (c), pointers to §15.4–15.7),
`sangen-generator-expert.md` (generated markers are subject to post-load scenario mutation).

**G.14 — CONFIRMED LIVE: the exact working method for spawning units from a per-map script.**
Proven in-game this session (BigBots visibly appeared), and the same pattern the live map's
naval-fleet feature already uses successfully:
```lua
-- inside the single NewThread callback (Armies is NOT populated during LoadMapData)
for armyIndex, army in pairs(Armies) do
    local x, z = 1024, 1024
    local errCode, height = Engine.SampleTerrainHeightFromCell(
        EngineClasses.int2(math.floor(x), math.floor(z)))
    if errCode ~= EngineErrorCode.Success then height = 100 end
    pcall(CreateUnit, armyIndex, "ucl4004", EngineClasses.float3(x, height, z))
end
```
- `CreateUnit(armyIndex, tpId, float3)` — confirmed working. `ucl4004` = Chosen T4 BigBot.
- `armyIndex` comes from iterating the engine's own `Armies` table, never a derived/assumed index.
- `Engine.SampleTerrainHeightFromCell(int2)` returns **two** values: `(errorCode, height)`.
- ⚠️ **Units outside the active playable area are CULLED** — models *and* strategic icons. Early
  diagnostics placed at `z=1224`/`z=1284` (outside `AREA_169`'s z-range 824–1224) rendered nothing
  and were misread as "the code didn't run." Always spawn inside the active area when using units
  as a diagnostic signal.
- ⚠️ **`army.lobbyOptions.isEmptySlot` unguarded is a CONFIRMED LIVE BUG — fixed 2026-08-28.**
  `lobbyOptions` is nil on at least some army entries (an **AI army** is the confirmed case), so
  the unguarded dereference **throws**. Because the caller wraps the spawn in `pcall` and `Warn()`
  goes to the unreliable F1 console (G.3), the throw is swallowed and **zero units spawn with no
  visible error anywhere.**
  **Reproduced live:** 1v1 with a HUMAN in slot 5 and an AI in slot 6 → correct `AREA_FULL`
  playable area (set before the spawn call, never touches `lobbyOptions`) and correct alloy
  handling (`ApplyScenario`'s occupancy branch keys off `slotPattern`) — but **zero units**.
  **Latent for a long time:** the old `SpawnNavalFleets` used the identical unguarded check, but
  `navy`/`spawnsUnits` was only ever true on the all-human `4human` scenario, so no AI army
  reached it until `slots5to8AnyFilled` shipped.
  **Fix (applied to the live file AND STEP72):**
  `local bIsEmptySlot = army.lobbyOptions and army.lobbyOptions.isEmptySlot`
  nil `lobbyOptions` ⇒ treat as **OCCUPIED** (an AI slot IS filled). Never treat a missing options
  table as an empty slot — that silently skips real players.
  Also prefer per-army `pcall` isolation over one wrapping `pcall`, so a single bad entry cannot
  abort the whole loop silently.

**Separate, also confirmed:** alloy markers can be created by writing into
`GameInfo.MapData.markers.Alloys.transforms` **before** `RunMapSetup` (a plain table insert with
`rotation`/`scale`/`position` sub-tables). That is how every diagnostic marker row in this session
was produced, and it is a synchronous `LoadMapData`-time mechanism, not a `NewThread` one.

**G.13 — `resources/lua/` needs no new ARCH layer. Settled.**
The repo already has a top-level `shaders/` tree holding `.glsl`, staged beside the executable at
build time, entirely outside `src/`. ARCH §2's layer map governs `src/` subdirectories only.
`resources/lua/` is the same category. STEP72 Part 3 mirrors the shader CMake staging verbatim.
