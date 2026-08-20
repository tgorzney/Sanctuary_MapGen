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
  (scenario module)

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

## 2. The two-file structure

- **`<MapName>_data.lua` — orchestrator.** Reads the lobby, derives
  `total`/`humanCount`/`aiCount` and the `slotPattern`, calls into the scenario
  module, and wires the result into the map (playable area resize, naval spawn
  trigger). **Holds no scenario content of its own** — no scenario definitions, no
  `alloyMode` logic, no spawn-position tables.
- **`<MapName>_Scenarios_Script.lua` — scenario module.** Every scenario definition
  (all three tiers, §4), all `alloyMode` application logic (§5), and all naval-fleet
  spawning logic (§3).
- **Naming: both files are map-name-prefixed**, following the engine's existing
  per-map convention (`<MapName>_data.lua`, `<MapName>_data_debug.lua`,
  `<MapName>_info.lua`) — a bare identically-named file repeated in every map folder
  would reproduce the filename-pairing fragility already keyed on the sanmap
  filename (not `data.name`) elsewhere in the map-loading system.
- **Location: COLOCATED in the script tree, `LJ/lua/maps/<MapName>/`.** Both files
  live in the same folder. **Not split across trees** — the map's asset folder
  (`Sanctuary_Data/Maps/<MapName>/`, where the `.sanmap`/Props/Textures live)
  contains no `.lua` files at all. This is mandatory, not a style preference:
  cross-tree `Import()` (script tree → asset tree) is impossible — disproven live
  in-game (see `MODDING_SCRIPTING_SPEC.md`'s retraction record for the evidence
  chain). If `_data.lua` ever relocates, both files move together, in the same
  commit.
- **Link mechanism:** `Import("maps/<MapName>/<MapName>_Scenarios_Script.lua").Scenario`,
  called from `_data.lua`. The path is **libPath-root-relative** (the `LJ/lua`
  root), the same convention every other `Import()` call site in the engine uses
  (`Import("common/gameUtils.lua")`) — never a path relative to the calling file.

## 3. Module API contract

`Scenario` **MUST be declared as a global table** (`Scenario = {}`, not
`local Scenario = {}`) — per the `Import()` global-capture rule
(`MODDING_SCRIPTING_SPEC.md`, "Lua runtime & sandbox"): `Import()` captures only a
file's global variables via a custom environment table, never a module's `return`
value. A `local`-scoped module silently yields none of its fields through
`Import()`, with no error.

- **`Scenario.ResolveAndApply(total, humanCount, aiCount, slotPattern) -> chosenArea, navyEnabled`**
  - Called **once, synchronously**, from `_data.lua`'s `LoadMapData()`-time
    `pcall`-wrapped body.
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
  data, identical regardless of lobby size). `total`/`humanCount`/`aiCount` and
  `slotPattern` are all derived on demand from that **same** official array — no
  separate transformed/stored shape invented on top of it.

## 4. The three-tier matching system

`slotPattern` format: one character per army slot, 1..16 (a map's own max army
count — author to the map's real ceiling, not the current lobby UI's exposed
limit, since the UI limit is not guaranteed permanent), `"h"` = human,
`"A"` = AI, `"-"` = empty.

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
`{name, x, y, z}` marker overrides).

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
- **❓ Open design question, not resolved by this ratification** (flag to the IO
  Architecture Expert / human when this becomes live work): does SanGen know, or
  need to be told, the game install's `LJ/lua` root at export time (a second
  export destination beyond the map asset folder)? Does SanGen literally
  round-trip the Lua text (parse/regenerate the tiered scenario tables verbatim,
  preserving hand-authored comments and ordering-significant array structure —
  hard, given Lua is not JSON and TIER 2 ordering is semantically load-bearing per
  §4), or does SanGen instead own only the *parameterized scenario data* (as a
  PARAMS/JSON structure) and render that into the `.lua` module text on export,
  never reading the `.lua` back in on import? The latter avoids a Lua parser
  entirely and fits SanGen's existing PARAMS→IO write direction better, but is not
  decided here — this spec states only that the capability remains in scope and
  names the design question, per instruction not to invent the answer.
- This spec (`MAP_SCENARIO_SPEC.md`) continues to govern **what the file is and
  where it lives in the engine's tree(s)** — unchanged in nature by the scope
  reclassification, just no longer hypothetical about SanGen ever touching it.

## 9. Cross-references

- `MODDING_SCRIPTING_SPEC.md` — the `Import()` global-capture rule (general law,
  not scenario-specific), the `LoadMapData()`/`CreateArmies()`/`RunMapSetup()`/
  `NewThread` lifecycle, the cross-tree-`Import()` disproof evidence chain (why §2's
  colocation rule is mandatory), and the F1-console reliability caveat.
- `IO_MIGRATION_SPEC.md` §1 — the existing per-domain `.sanmap` JSON IO convention
  this file's IO surface explicitly does **not** reuse (§8).
- `AI_HOSTCLIENT_SPEC.md` — host/client authoritative-vs-presentation split; relevant
  background for the `IsHost`-gating rule in §7 (not re-derived here).
