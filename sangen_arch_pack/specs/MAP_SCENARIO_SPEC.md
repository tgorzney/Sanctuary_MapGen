# MAP_SCENARIO_SPEC — the SanGen Map Scenario system

**Status: DEPLOYED and confirmed working live in-game (2026-08-20).** This spec
consolidates that deployment into first-class law: file structure, module contract,
data format, and hard requirements. It supersedes the "what to build" content that
previously lived in `MODDING_SCRIPTING_SPEC.md`'s "Scenario-script file split"
section — that section now holds only the historical investigation trail (the
disproven cross-tree-`Import()` hypothesis and the general `Import()`-mechanics
lessons it produced); read it for *why* this design looks the way it does, read
this spec for *what the law is*. Prerequisite reading: `MODDING_SCRIPTING_SPEC.md`
§"Lua runtime & sandbox" (the `Import()` global-capture rule) and §"Map scripting
(events)" (the `LoadMapData()`/`CreateArmies()`/`RunMapSetup()`/`NewThread` lifecycle)
— both are load-bearing here and not re-derived in this file.

**Reference implementation (ground truth this spec was extracted from — external to
this repo, on the game install, not SanGen source):**
- `LJ/lua/maps/Pandemonium Isthmus/Pandemonium Isthmus_data.lua` (orchestrator)
- `LJ/lua/maps/Pandemonium Isthmus/Pandemonium Isthmus_Scenarios_Script.lua`
  (scenario module — **legacy, single-file shape**; §2 below ratifies the split
  this file's two concerns are divided into going forward)

A future reader without game-install access must treat this spec as the frozen,
authoritative extraction of that pair — do not assume the files are re-readable from
this repo.

---

## 1. Purpose

A map's playable area, per-army spawn position, per-army alloy/mex marker visibility,
and (optionally) a per-army naval fleet are all functions of **lobby composition**
(how many total players, how many human vs. AI, and — separately — *which* army
slots are filled). The Map Scenario system resolves this once, deterministically, at
map load, from a small ordered rule table authored per map. It replaced an earlier
ad hoc if/elseif block that lived directly in `_data.lua` and duplicated logic across
compositions; every composition now goes through the exact same `ApplyScenario()` path.

## 2. The three-file structure (ratified — supersedes the original two-file design)

**Ratified alongside `ARCH_15_MapScenarioSystem.md` §15.3–§15.9** (design option (c): SanGen owns
scenario **data**; it does NOT parse Lua to read it back — export-only). The
original two-file design's `<MapName>_Scenarios_Script.lua` mixed two concerns —
the generic runtime algorithm and the per-map scenario tables — in one hand-authored
file. This ratification splits them into two separate **SanGen-owned** files,
alongside the unchanged hand-authored orchestrator, for **three** files total, all
still colocated in `LJ/lua/maps/<MapName>/` (the colocation/no-cross-tree rule below
is unchanged from the original design):

| File | Role | Owner | Written by SanGen? |
| --- | --- | --- | --- |
| `<MapName>_data.lua` | Orchestrator — reads the lobby, derives `total`/`humanCount`/`aiCount`, calls into the scenario module, wires the result into the map (playable-area resize, naval spawn trigger). **No longer derives `slotPattern` itself — see §15.10 below.** | Hand-authored, map-specific | **Never.** SanGen does not write, generate, or overwrite this file, ever, under any code path. |
| `<MapName>_Scenarios_Runtime.lua` | The generic runtime algorithm — `BuildSlotPattern`, `FindMatchingScenario`, `Scenario.ResolveAndApply`, `Scenario.SpawnNavalFleets` (§3–§5 below; `BuildSlotPattern`'s move into this file is ratified `ARCH_15_10_SlotPatternConstructionMoves.md` §15.10). Identical content across every map that uses the system; a bundled SanGen resource, copied per map on export (a settings-level override path may replace the bundled default — UI-layer design, not fixed here). | SanGen-owned | **Yes, on every export.** Never hand-edited in place — editing happens inside SanGen (a UI-layer concern running in a parallel consult; not designed here). |
| `<MapName>_Scenarios_Data.lua` | The per-map scenario tables — `PATTERN_SCENARIOS`/`COUNT_SCENARIOS`/`DEFAULT_SCENARIO`/`MAX_ARMY_SLOT_COUNT` (the last added by `ARCH_15_10_SlotPatternConstructionMoves.md` §15.10) — rendered from the new `Params::Scenarios` PARAMS type (`ARCH_15_05_ParamsScenariosType.md` §15.5) authored inside SanGen. | SanGen-owned | **Yes, fully regenerated on every export.** Never hand-edited, and never read back by SanGen (design option (c) — no Lua parser). |

- **Naming rationale.** `_Runtime`/`_Data` suffixes (not the legacy `_Script` name)
  make the SanGen-owned pair visually and mechanically distinct from both the
  hand-authored orchestrator and the legacy single-file `_Scenarios_Script.lua` this
  design supersedes — load-bearing for the overwrite-safety mechanism below (§2.1),
  which depends on the generated paths never colliding with a filename a human might
  have hand-authored before this ratification existed.
- **Link mechanism, extended.** `<MapName>_data.lua`'s `Import()` call target moves
  from the legacy `<MapName>_Scenarios_Script.lua` to
  `<MapName>_Scenarios_Runtime.lua` — a **hand-edit**, once per map, part of the
  migration in §2.2. Internally, `<MapName>_Scenarios_Runtime.lua` itself
  `Import()`s `<MapName>_Scenarios_Data.lua` (same folder, same
  libPath-root-relative convention the original colocation rule already requires)
  to obtain the `PATTERN_SCENARIOS`/`COUNT_SCENARIOS`/`DEFAULT_SCENARIO`/
  `MAX_ARMY_SLOT_COUNT` globals it operates over — the generated data file therefore
  declares its tables as file-level **globals**, for the same `Import()`
  global-capture reason §3 already states for the `Scenario` table, never `local`.
  Exact Lua-rendering syntax (table literal formatting, internal variable names) is
  IO-layer/coder-tier, not fixed here.
- **The module API contract (§3) is unchanged by this three-file split** —
  `Scenario.ResolveAndApply`/`Scenario.SpawnNavalFleets` still live in the runtime
  file; only the file's name, and the fact that it no longer carries the scenario
  data tables inline, change. **Amended separately by `ARCH_15_10_SlotPatternConstructionMoves.md` §15.10**, which
  moves slot-pattern construction into this file too and changes
  `Scenario.ResolveAndApply`'s fourth argument — see §3 below, updated accordingly.
- **Location and cross-tree rule: unchanged.** All three files remain colocated in
  the script tree, `LJ/lua/maps/<MapName>/`; the map's asset folder
  (`Sanctuary_Data/Maps/<MapName>/`) still contains no `.lua` files at all —
  cross-tree `Import()` is still impossible (`MODDING_SCRIPTING_SPEC.md`'s
  retraction record).

### 2.1 Overwrite safety — SanGen must never clobber a hand-authored file
Ratified mechanism (`ARCH_15_04_ThreeFileOnDiskShape.md` §15.4):
1. **Filename disjointness (primary defense).** SanGen's exporter writes to exactly
   two paths per map — `<MapName>_Scenarios_Runtime.lua` and
   `<MapName>_Scenarios_Data.lua` — and **never** to `<MapName>_data.lua` or the
   legacy `<MapName>_Scenarios_Script.lua`. Because both SanGen-owned filenames are
   new, introduced by this ratification, no file that predates this design can ever
   occupy one of them by coincidence.
2. **Generated-file header marker.** Both SanGen-owned files begin with a
   machine-checkable banner comment (exact text is IO-layer/coder-tier; must include
   a literal, greppable token, e.g. `-- SANGEN GENERATED FILE — DO NOT HAND-EDIT`)
   identifying the file as SanGen-owned and regenerated on every export.
3. **Loud refusal on an unrecognized occupant.** Before writing either SanGen-owned
   path, the exporter reads whatever file already exists there (if any) and checks
   for the marker token. Present and matching → overwrite proceeds (this is SanGen's
   own prior output). **Absent** → the exporter refuses to write **that file only**,
   logs a loud, specific error naming the exact path and stating that a foreign/
   hand-authored file occupies a SanGen-generated path, and continues exporting
   everything else the map export touches (map assets, the `.sanmap`, any other
   domain) rather than aborting the whole export over one file. This is a
   **write-target safety refusal**, distinct in kind from Constitution §6's
   import-time "a version marker is never grounds to refuse the file" rule — that
   rule governs reading a file of unclear vintage; this governs refusing a
   destructive **write**.

### 2.2 Migration of the live two-file map
The live `Pandemonium Isthmus_Scenarios_Script.lua` is hand-authored today and mixes
both concerns this ratification splits apart (runtime algorithm + per-map scenario
tables). It is **not** at any overwrite risk under §2.1 — its filename never
collides with either SanGen-owned path — but it is also not automatically migrated
by SanGen; migrating an existing map to the three-file design is a one-time
**human** action:
1. Author the map's scenario data inside SanGen (the new `Params::Scenarios` UI, out
   of scope here) from the existing file's `PATTERN_SCENARIOS`/`COUNT_SCENARIOS`/
   `DEFAULT_SCENARIO` tables, preserving `COUNT_SCENARIOS`' authored order exactly
   — order is the match-priority authoring action (`ARCH_15_06_CountScenariosOrdering.md` §15.6). Also author
   `maxArmySlotCount` (`ARCH_15_10_SlotPatternConstructionMoves.md` §15.10) — for a map migrating from a reference file
   whose `BuildSlotPattern` hardcoded `for i = 1, 16`, this is `16`.
2. Export from SanGen once, producing `<MapName>_Scenarios_Runtime.lua` +
   `<MapName>_Scenarios_Data.lua` alongside the untouched legacy file.
3. Hand-edit `<MapName>_data.lua`:
   a. Retarget its `Import()` call from the legacy `_Scenarios_Script.lua` to
      `_Scenarios_Runtime.lua`.
   b. Change the `Scenario.ResolveAndApply` call site from
      `Scenario.ResolveAndApply(total, humanCount, aiCount, slotPattern)` to
      `Scenario.ResolveAndApply(total, humanCount, aiCount, playersInformation)`
      (`ARCH_15_10_SlotPatternConstructionMoves.md` §15.10).
   c. Delete the orchestrator's own `BuildSlotPattern` function and its call
      (`local slotPattern = BuildSlotPattern(playersInformation)`) — dead code
      once the runtime owns the algorithm.
4. The now-orphaned legacy `_Scenarios_Script.lua` is left in place by SanGen —
   never auto-deleted; deleting a hand-authored file is exactly as forbidden as
   overwriting one. Removing it, if desired, is a separate manual cleanup step
   outside SanGen's own write path.

## 3. Module API contract

`Scenario` **MUST be declared as a global table** (`Scenario = {}`, not
`local Scenario = {}`) — per the `Import()` global-capture rule
(`MODDING_SCRIPTING_SPEC.md`, "Lua runtime & sandbox"): `Import()` captures only a
file's global variables via a custom environment table, never a module's `return`
value. A `local`-scoped module silently yields none of its fields through
`Import()`, with no error.

- **`Scenario.ResolveAndApply(total, humanCount, aiCount, playersInformation) -> chosenArea, navyEnabled`**
  (signature changed, `ARCH_15_10_SlotPatternConstructionMoves.md` §15.10 — the fourth argument was `slotPattern`; the
  orchestrator no longer builds it and passes the raw lobby array instead)
  - Called **once, synchronously**, from `_data.lua`'s `LoadMapData()`-time
    `pcall`-wrapped body.
  - **Builds `slotPattern` internally**, via `BuildSlotPattern(playersInformation,
    MAX_ARMY_SLOT_COUNT)` — `BuildSlotPattern` (§2 above; the algorithm the
    reference `_data.lua` used to own) and `MAX_ARMY_SLOT_COUNT` (the rendered
    `Params::Scenarios::maxArmySlotCount`, `ARCH_15_10_SlotPatternConstructionMoves.md` §15.10) both now live in this
    file, so the bound the pattern is built against and the data the pattern is
    matched against come from the same authored source, every export.
  - Finds the matching scenario via the three-tier system (§4), mutates
    `GameInfo.MapData.markers.Spawn.transforms` / `.Alloys.transforms` in place per
    the matched scenario's `alloyMode` (§5), logs the resolution outcome, and
    returns the resolved playable-area rect plus the navy flag.
  - MUST complete before `CreateArmies()` / `RunMapSetup()` read those same tables
    (§6 execution law) — this is the entire reason it runs synchronously inside
    `LoadMapData()` rather than deferred.
- **`Scenario.SpawnNavalFleets(area)`**
  - Deferred, **host-only** unit spawning, invoked only for a matched scenario with
    `navy = true`.
  - MUST run **after** `RunMapSetup()` — it needs `Armies` populated, which does not
    exist yet during `LoadMapData()`. Called from `_data.lua`'s single `NewThread`
    callback, gated `if navyEnabled then`.
  - Each army's spawn attempt is wrapped in its own `pcall` so one army's failure
    does not abort fleet spawning for the rest.
- **`_data.lua` reads the lobby itself**, via `Engine.GetLobbyInformation()`
  (`.playersInformation`), rather than `GameInfo.MapData.armies` (static per-map
  data, identical regardless of lobby size). `total`/`humanCount`/`aiCount` are
  still derived by the orchestrator, on demand, from that array — unchanged by
  `ARCH_15_10_SlotPatternConstructionMoves.md` §15.10, which moves only `slotPattern` construction downstream. The
  orchestrator now passes `playersInformation` itself through unmodified, rather
  than pre-deriving `slotPattern` from it — still no separate transformed/stored
  shape invented anywhere in the chain, just one more consumer (the runtime, not
  the orchestrator) doing the deriving.

## 4. The three-tier matching system

`slotPattern` format: one character per army slot, 1..`maxArmySlotCount` (a map's
own authored max army count, `Params::Scenarios::maxArmySlotCount`, `ARCH_15_10_SlotPatternConstructionMoves.md`
§15.10 — author to the map's real ceiling, not the current lobby UI's exposed
limit, since the UI limit is not guaranteed permanent; default 16, matching the
live reference), `"h"` = human, `"A"` = AI, `"-"` = empty. **Built by the runtime**
(`BuildSlotPattern`, §2/§3 above), not the orchestrator.

- **TIER 1 — `PATTERN_SCENARIOS`.** Exact `slotPattern` string equality
  (`scenario.pattern == slotPattern`). Checked first. For when **which** slots are
  filled matters, not just how many — e.g. 4 humans in slots 1-4 (a compact
  cluster) vs. slots 5-8 (scattered) can be identical under every aggregate count
  yet need different areas/spawns.
- **TIER 2 — `COUNT_SCENARIOS`.** An **ordered array** of records, each carrying
  `match = function(total, humanCount, aiCount, slotPattern) -> boolean`. Checked
  in array order; **the first match wins.** Order is significant and load-bearing:
  a broader predicate placed above a narrower one silently shadows it. (The
  reference implementation deliberately keeps its broad fallback rules —
  `2hRestAI`, `floor169` — after every more specific composition rule for exactly
  this reason.) `FindMatchingScenario` calls each `match` function wrapped in its
  own `pcall`; a throwing match function is swallowed (falls through to the next
  candidate), not fatal to resolution.
- **TIER 3 — `DEFAULT_SCENARIO`.** No `match`/`pattern` field; always matches.
  Last-resort fallback if nothing in tiers 1-2 matched.

## 5. Scenario record shape and `alloyMode` semantics

Fields: `name` (string, log/debug identifier), `match` (tier 2) or `pattern`
(tier 1), `area` (world-space rect `{x, y, width, height}` — `y` is world `z`,
matching the `.sanmap`'s own "areas" format), `navy` (bool), `alloyMode` (one of
the four values below), optional `spawns` (per-army `{x, y, z}` table — see §6,
the hard requirement), optional `alloys` (per-army list of
`{name, x, y, z}` marker overrides), optional `navalFleet` (naval-fleet
composition, meaningful only when `navy` is true — §5.1).

- **`explicit`** — the scenario is authoritative. Every army named in
  `scenario.alloys` has its markers created (if absent) or repositioned. Every army
  present in the map's known-alloy-marker roster that the scenario does **not**
  mention has its markers **deleted**. Fully self-contained; immune to any other
  scenario's data — this is the fix for the exact regression class described in §6.
- **`occupancy`** — no explicit alloy data is trusted from the scenario; instead
  trust the `.sanmap`'s own baked marker positions, and delete markers only for
  armies with **no player** per `slotPattern` (an empty slot).
- **`keepAll`** — no deletion at all. Every known marker stays exactly as the
  `.sanmap` has it baked, even for empty slots (e.g. deliberately leaving extra
  resources available to a lone AI in a 3-slot lobby).
- **`delta`** — wired but **not yet used by any live scenario** (kept "so we don't
  lose the thought," for once a real diff baseline exists). Unlike `explicit`,
  **silence is NOT a delete instruction** — only `scenario.alloys.add` /
  `scenario.alloys.remove` are applied; everything else is left exactly as the
  baseline (or an earlier-applied layer) already set it. Shape:
  `alloys = { add = { ARMY_XX = {{name=, x=, y=, z=}, ...} }, remove = { ARMY_XX = {"MarkerName", ...} } }`.

### 5.1 Naval-fleet composition (ratified — `ARCH_15_05_ParamsScenariosType.md` §15.5, live reference read 2026-08-21)

The reference `SpawnNavalFleets(area)` (§3 above) reads three per-map values, live-read
directly from `Pandemonium Isthmus_Scenarios_Script.lua`:
- `NAVAL_FLEET` — an ordered `{tpId, count}` list (e.g. `{tpId="ucn1001", count=5}`), the
  spawn-batch composition.
- `NAVAL_POND_SIDE_BY_ARMY` — a sparse per-army side assignment (`-1` = west/left pond, `1` =
  east/right pond), read as `NAVAL_POND_SIDE_BY_ARMY[army.name] or 1` — an army absent from
  the table defaults to `1` (east).
- `NAVAL_SIDE_BIAS_DISTANCE` — a single world-units scalar (`90` in the reference), the
  offset applied to the spawn-marker origin before the spiral search begins.

`Params::ScenarioNavalFleet` (`ARCH_15_05_ParamsScenariosType.md` §15.5) shapes exactly these three as per-scenario
PARAMS data — `fleet`, `pondSideByArmy`, `sideBiasDistance` — added to `ScenarioBody`
alongside `navy`, meaningful only when `navy == true`. **Per-scenario, not per-map** — the
live reference's own file-scoping is an artifact of only one scenario having ever used
`navy = true`; `ARCH_15_05_ParamsScenariosType.md` §15.5 rules per-scenario placement correct going forward, so
different navy-enabled scenarios can field different compositions.

**Explicitly NOT part of this shape — algorithm tuning constants, never per-map data:**
`NAVAL_BATCH_SIZE`, `NAVAL_SPIRAL_STEP`, `NAVAL_SPIRAL_MAX_TRIES`, `NAVAL_GAP`,
`NAVAL_DEFAULT_FOOTPRINT`, `NAVAL_GRID_CELL`, `NAVAL_GIVE_UP_AFTER_MISSES` — the reference
file's own header comment already marks these "algorithm tuning constants, NOT per-map
authored data." They stay hardcoded inside `<MapName>_Scenarios_Runtime.lua` (§2 above),
identical across every map, and are never exposed as `Params::Scenarios` fields.

## 6. ⚠️ HARD REQUIREMENT — explicit `spawns` is mandatory for deterministic compositions

**Every scenario that needs deterministic spawn positions MUST declare an explicit
`spawns` table.** A scenario without one silently inherits whatever spawn positions
happen to be baked into the `.sanmap` at that moment.

**Why this is a hard requirement, not a style preference:** the `.sanmap` stores
exactly **one** spawn transform per army name
(`GameInfo.MapData.markers.Spawn.transforms[armyName]`), read unconditionally by
`SpawnInitialUnits` (`common/gameUtils.lua:356-371`) with **no per-composition
branching of its own.** This value is **shared mutable state across every
scenario that does not override it.** Editing it to tune one composition silently
changes spawn behavior for every other scenario that lacks its own explicit
`spawns` entry.

**Live proof (2026-08-20):** a 3-player lobby (armies 1 and 3 human, army 2 AI)
correctly matched the `2h1ai` scenario and correctly applied its `AREA_169`
playable area — but spawned players at the **6-player positions**, because
`2h1ai` declares `area`/`navy`/`alloyMode` and no `spawns`. This was **working
exactly as defined** — the failure was incomplete scenario data, not a bug in the
resolution mechanism. The same class of regression is independently documented,
in the reference implementation's own comments, as having previously broken 1v1,
4-human, and 1-human-3-AI compositions in the same way, each time a shared
baseline was edited for an unrelated composition's testing.

**Rule:** authoring a new `COUNT_SCENARIOS` or `PATTERN_SCENARIOS` entry without a
`spawns` table is only acceptable when that composition is intentionally meant to
inherit the `.sanmap`'s baked default (document that intent in the entry's own
comment) or is explicitly `alloyMode = "occupancy"`/`"keepAll"` pending a future
`spawns` table (as the reference implementation's in-progress `6total` entry does,
flagged in its own comment as "still in progress, no explicit data yet"). Silent
omission — a scenario that looks complete but happens to share the default
baseline by accident — is the exact failure mode above and must not recur.

## 7. Execution / timing law

- `Scenario.ResolveAndApply` runs **synchronously** during `LoadMapData()`,
  **before** `CreateArmies()` and `RunMapSetup()` read the marker tables it mutates.
- Deferred work (anything touching `Armies`, `CreateUnit`, playable-area resize)
  goes inside `NewThread`, which fires after `RunMapSetup()`, on tick 0.
- **Only ONE `NewThread()` call per script is honored** (`MODDING_SCRIPTING_SPEC.md`,
  "Map scripting (events)") — a second, separate call silently never runs. A
  script with more than one host-deferred job (playable-area resize AND naval
  spawn, here) must merge them into a single `NewThread` callback, exactly as the
  reference `_data.lua` does.
- Host-only work (`SetPlayableArea`, unit spawning) is gated behind `IsHost`.
- **The orchestrator's entire body MUST be `pcall`-wrapped.** An uncaught error
  inside `LoadMapData()` aborts it entirely, and `CreateArmies()`/`RunMapSetup()`
  then never run at all — no armies, no units, no props, no markers, no resource
  spots. Confirmed by a live crash, 2026-08-16. The `pcall` failure branch must
  fall back to the map's normal defaults (an empty override, not a re-thrown
  error) so this file can never take the rest of map load down with it.
- `NewThread` callback errors are already caught safely by the engine's own
  threading system (`common/systems/threads.lua`'s `ResumeThread`) — only the
  synchronous, pre-`NewThread` code needs the explicit `pcall`.

## 8. IO scope ruling — SanGen Import/Export of the scenario file

**Superseded assumption, corrected:** an earlier ratification recorded SanGen
Import/Export of the Scenarios file as in scope, reasoning "SanGen will build
Import/Export for the Scenarios file" — under the assumption, at the time, that the
file would live in the map's **asset folder** (`Sanctuary_Data/Maps/<MapName>/`),
i.e. inside the shippable `.sanmap` package SanGen's `MapImporter_*`/`MapExporter_*`
already read/write. That assumption is now known wrong: the file lives in the
**script tree** (`LJ/lua/maps/<MapName>/`), a location SanGen's importer/exporter
does not address at all today and which is not part of the `.sanmap` package.

**Ruling: still in scope, but reclassified — not a yes/no reversal.** The human's
stated intent ("we will be creating an Import and Export to SanGen for the
Scenarios file") did not hinge on file location, so the scope call stands. What
changed is the **kind** of IO surface this requires:
- It is **not** an additional section inside the existing `.sanmap` JSON package
  (unlike `PropGroups`/`DecalGroups`, `HeightmapStack`, etc.) — it is a **separate
  companion artifact**, a `.lua` text file, written to a **different filesystem
  location** than the map asset export folder (the game install's
  `LJ/lua/maps/<MapName>/`, not `Sanctuary_Data/Maps/<MapName>/`).
- Therefore this is **not** an extension of the existing per-domain
  `MapImporter_<Domain>Stack_IO.cpp` / `MapExporter_*` convention
  (`IO_MIGRATION_SPEC.md` §1) — that convention is for JSON fragments of the one
  `.sanmap` document. A `.lua` companion file at a distinct install-relative path
  is a structurally different IO surface and needs its own convention designed
  from scratch, not shoehorned into the existing per-domain split.
- **This is the SanGen IO Architecture Expert's domain** — how to structure new
  SanGen IO code, including any *new* file-type convention — not this ARCH's. Per
  the existing ruling already recorded in `MODDING_SCRIPTING_SPEC.md`: no code is
  written until a work-order exists and is ratified; the SanGen Coder writes zero
  code without one.
- ~~**❓ Open design question, not resolved by this ratification**~~ **RESOLVED —
  see §8.1 below.**
- This spec (`MAP_SCENARIO_SPEC.md`) continues to govern **what the file is and
  where it lives in the engine's tree(s)** — unchanged in nature by the scope
  reclassification, just no longer hypothetical about SanGen ever touching it.

### 8.1 Resolution (ratified — `ARCH_15_03_ExportOnlyLuaRatified.md` §15.3)
The open question this section originally posed — literal Lua round-trip vs.
SanGen owning only parameterized scenario data — is **settled by the human: option
(c).** SanGen owns scenario **data** (a new `Params::Scenarios` PARAMS type,
`ARCH_15_05_ParamsScenariosType.md` §15.5), persisted in the `.sanmap` as a new SanGen-owned schema-v3
section, and rendered on export into the generated `<MapName>_Scenarios_Data.lua`
(§2 above). SanGen never parses Lua to read scenario data back — export-only, no
Lua parser in the import direction. **Rejected:** literal Lua round-trip (parse and
regenerate the tiered tables verbatim, preserving comments and ordering), and
reading scenario data back out of the `.sanmap` at runtime from Lua (both were
named but deliberately unanswered options in this section's original text).

Two new third-party dependencies this resolution and the parallel UI-authoring
consult require — **ImGuiColorTextEdit** (the runtime `.lua` text-editor widget)
and an **embedded LuaJIT library** (compile-only validation of edited runtime text,
never executed) — are ratified in `ARCH_15_08_ThirdPartyDependencyRuling.md` §15.8, including the hard
never-execute-untrusted-Lua constraint and the layer-dependency correction
(`ARCH_03_ModuleBoundaries.md` §3.1) that lets both `UI` and `IO` reach the validator.

The new PARAMS type's shape/naming ownership, the `.sanmap` section's format-truth
ownership, and the array-order-is-match-priority rule for `COUNT_SCENARIOS` are all
ratified in `ARCH_15_MapScenarioSystem.md` §15.5–§15.7 — not re-derived here.

The engine-whitelist migration path (a future one-line change to `LoadMapData`'s
explicit whitelist that would let the runtime read scenario data directly from
`GameInfo.MapData` and retire the generated `.lua` data file) is recorded as an
**intended future simplification, not current law** — `ARCH_15_09_EngineWhitelistMigrationPath.md` §15.9.

## 9. Cross-references

- `MODDING_SCRIPTING_SPEC.md` — the `Import()` global-capture rule (general law,
  not scenario-specific), the `LoadMapData()`/`CreateArmies()`/`RunMapSetup()`/
  `NewThread` lifecycle, the cross-tree-`Import()` disproof evidence chain (why §2's
  colocation rule is mandatory), and the F1-console reliability caveat.
- `IO_MIGRATION_SPEC.md` §1 — the existing per-domain `.sanmap` JSON IO convention
  this file's `.sanmap`-package IO surface (the new `Scenarios` section, §8.1) may
  extend; the separate companion-`.lua` IO surface (§2/§8) explicitly does **not**
  reuse it (§8).
- `AI_HOSTCLIENT_SPEC.md` — host/client authoritative-vs-presentation split; relevant
  background for the `IsHost`-gating rule in §7 (not re-derived here).
- `ARCH_15_MapScenarioSystem.md` §15 — the binding law this spec's deployment was promoted into,
  including §15.3–§15.9's ratification of the three-file split, the overwrite-safety
  mechanism, the `Params::Scenarios` PARAMS type, and the two new third-party
  dependencies, and §15.10's ratification of moving slot-pattern construction into
  the runtime (`maxArmySlotCount`, the new `Scenario.ResolveAndApply` signature, and
  the third-hardcode ruling on `ApplyScenario`'s `occupancy` loop bound).
