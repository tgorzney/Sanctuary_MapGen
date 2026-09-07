[← ARCH index](ARCH.md) · [§15 ARCH_15_MapScenarioSystem](ARCH_15_MapScenarioSystem.md) · SanGen ARCH §15.4. **Only the ARCH Expert writes this file.**

### 15.4 Three-file on-disk shape + overwrite safety (ratifies `MAP_SCENARIO_SPEC.md` §2/§2.1/§2.2)

Full detail: `MAP_SCENARIO_SPEC.md` §2–§2.2. Binding summary:

- **Three files, all colocated in `LJ/lua/maps/<MapName>/`**, superseding the original two-file
  shape named at the top of §15:
  1. `<MapName>_data.lua` — hand-authored orchestrator. **Never written by SanGen, under any
     code path.**
  2. `<MapName>_Scenarios_Runtime.lua` — the generic runtime algorithm
     (`FindMatchingScenario`/`ResolveAndApply`/the unit-spawn executor). SanGen-owned: a bundled
     resource, **copied** per map on every export (a settings-level override path may substitute
     a designer-chosen file for the bundled default — UI-layer design, not fixed here).
     **Corrected 2026-08-28:** the live reference's naval-only `SpawnNavalFleets` this bullet
     originally cited no longer exists — replaced by the generic `SpawnMatchedScenarioUnits`/
     `SpawnUnits` pair (`ARCH_15_05` "RETIRED 2026-08-28" note). That same note also flags an open
     gap this bullet's "generic, identical across every map" framing does not yet resolve:
     `SpawnMatchedScenarioUnits`'s per-scenario dispatch branches and generator functions are
     per-map content, not generic runtime content — where they belong under this three-file split
     is unresolved, not decided here.
  3. `<MapName>_Scenarios_Data.lua` — the per-map scenario tables, **rendered** from
     `Params::Scenarios` (§15.5) on every export. SanGen-owned, never hand-edited, never read
     back.
- **Overwrite safety — three-part mechanism, binding on the exporter:**
  1. **Filename disjointness.** SanGen writes only to paths 2 and 3 above, never to path 1 or the
     legacy `<MapName>_Scenarios_Script.lua` — both new filenames are introduced by this
     ratification, so no pre-existing hand-authored file can occupy them by coincidence.
  2. **Generated-file header marker.** Both SanGen-owned files open with a machine-checkable
     banner token identifying them as SanGen-generated.
  3. **Loud, file-scoped refusal.** Before overwriting either SanGen-owned path, the exporter
     checks for its own marker. Absent (a foreign file occupies a generated path) → refuse to
     write **that file only**, log a specific loud error naming the path, and continue exporting
     everything else the map export touches. This is a write-target safety refusal, not an
     import-time version-tolerance question — it does not fall under, and does not relax,
     Constitution §6's "a version marker is never grounds to refuse the file."
- **The live `Pandemonium Isthmus_Scenarios_Script.lua` is hand-authored and in active use
  today. It is never at overwrite risk** under this design — its filename collides with neither
  SanGen-owned path. It is **not** automatically split or migrated by SanGen. Migrating an
  existing map is a one-time **human** action (`MAP_SCENARIO_SPEC.md` §2.2): author its scenario
  data inside SanGen preserving `COUNT_SCENARIOS`' order, export once, then hand-edit `_data.lua`'s
  `Import()` target from the legacy filename to `_Scenarios_Runtime.lua`. SanGen never deletes
  the orphaned legacy file — exactly as forbidden as overwriting one.

---

### AMENDED 2026-09-03 — category 4, generic lazy dispatch, third overwrite-safety class (resolves `ARCH_15_05`'s OPEN item 2)

**Four categories, not three.** Categories 1–3 above are unchanged. New:

4. `<MapName>_Scenarios_<ScenarioName>.lua`, one per scenario with `spawnsUnits == true`;
   splittable to `<MapName>_Scenarios_<ScenarioName>_<FunctionName>.lua` when a single generator
   function alone would not fit the ceiling below. Hand-authored, per-map, per-scenario Lua — the
   category `ARCH_15_05`'s OPEN item 2 found no home for. **Never written by SanGen beyond a
   create-if-missing scaffold** (overwrite safety below) — the same authorship posture as
   category 1, not categories 2/3. Colocated in `LJ/lua/maps/<MapName>/`; no cross-tree `Import`
   (`MAP_UNIT_SPAWNING_SPEC.md` §3). Each file exposes a **global** `GenerateScenarioUnits(area)`
   returning the flat `{armyIndex, templateIdentifier, x, y, z}` array `Scenario.SpawnUnits`
   consumes (`MAP_SCENARIO_SPEC.md` §11) — the same global-exposure idiom every file in this
   system already uses. When split one-function-per-file, the `<ScenarioName>.lua` file stays the
   one defining `GenerateScenarioUnits`, `Import()`-ing its `_<FunctionName>.lua` siblings and
   capturing their globals into locals at its own top level — the same idiom
   `SanGenScenarioRuntime.lua:37-43` already uses for `ScenarioData`, applied one layer deeper. No
   new `Import()` mechanic.

**Dispatch — generic, lazy, lives entirely inside `_Scenarios_Runtime.lua` (category 2); never
an if/elseif chain, never edited per map or per scenario:**

```lua
function Scenario.SpawnMatchedScenarioUnits(area)
    if not currentMatchedScenarioName then return end
    local path = string.format("maps/%s/%s_Scenarios_%s.lua",
        currentMapName, currentMapName, currentMatchedScenarioName)
    local importOk, generatorModule = pcall(Import, path)
    if not importOk or not generatorModule or not generatorModule.GenerateScenarioUnits then
        return  -- no such file: this scenario did not opt into unit spawning -- not an error
    end
    local buildOk, instructions = pcall(generatorModule.GenerateScenarioUnits, area)
    if buildOk and instructions then
        Scenario.SpawnUnits(instructions)
    end
end
```

Binding: `Import()`'d **only for the one matched scenario** (`currentMatchedScenarioName`,
`SanGenScenarioRuntime.lua:261`) — never eager, never a name→function table built up front, never
an enumeration of the full authored `Scenarios` set. Load cost is **O(1) files touched per map
load**, independent of how many scenarios are authored. A missing file for a `spawnsUnits ==
false` scenario is the silent, expected common case. A missing file for `spawnsUnits == true` is a
real authoring gap; `pcall` degrades it to "no units spawned" rather than aborting the thread —
consistent with the existing per-call `pcall` ordering law (`MAP_SCENARIO_SPEC.md` §3.1), not a
new exception to it.

**Universal helpers fold into `_Scenarios_Runtime.lua`** (category 2) as additional global
functions — not a new file. "Reusable across any map's scenarios, identical content every time"
is Runtime.lua's own existing definition; a fifth SanGen-owned bundled file would add no new kind
of content. Fallback only, if this would push Runtime.lua over a future ceiling ruled for it (not
ruled here): a second bundled, byte-identical, copied-per-export file,
`<MapName>_Scenarios_Helpers.lua` (bundled source `SanGenScenarioHelpers.lua`, mirroring
`ScenarioScript_RuntimeResource_IO.h`'s existing copy mechanism) — never a per-map or
per-scenario helper file.

**Overwrite safety gains a third class.** Categories 2–3 keep the existing "always regenerate,
refuse on foreign-marker collision" mechanism, unchanged. Category 1 keeps "never write, ever,
under any code path." Category 4 is neither:
- The collision check for categories 2–3 stays a literal two-name check. Category 4 uses a
  **glob/pattern match** (`<MapName>_Scenarios_*.lua`, excluding the two category-2/3 literal
  names) only to detect "does a file already occupy this scenario's generator slot" — never to
  decide whether to overwrite it. Category 4 is never overwritten once it exists, in any state
  (scaffold marker present, hand-edited, no marker at all).
- On export, for every `spawnsUnits == true` scenario whose expected generator file does not yet
  exist, SanGen writes a minimal scaffold (`GenerateScenarioUnits(area) return {} end`) under a
  distinct, weaker banner (`-- SANGEN-CREATED STARTING POINT -- freely hand-edit -- never
  regenerated`), **once**. On every subsequent export, if the file already exists in any state,
  SanGen never touches it again — simpler and safer than distinguishing "still the untouched
  stub" from "a human started editing it." One notch softer than category 1's "keep backups,
  SanGen won't help you here again" posture: SanGen gives a starting point once, then permanently
  gets out of the way.
- Creating a category-4 scaffold is logged exactly as loudly as writing categories 2–3; skipping
  an already-present category-4 file is a quiet, expected no-op — the common case on every export
  after the first.

**File-size ceiling — category 4 only, explicitly NOT retroactive.** Soft 100 / hard 150 lines per
category-4 file, functions ≤ 40 lines, mirroring `ARCH_01_05_FileSizeCeilings.md` §1.5 by analogy.
A generator function that would not fit must split one-file-per-function (above). This ceiling
does **not** apply to `_Scenarios_Runtime.lua` (category 2 — already ratified, already shipping,
309 lines, written before this amendment existed) or to `_Scenarios_Data.lua`'s renderer output
(category 3). Retrofitting either file's own sizing is a separate, independently-decided future
ratchet under Constitution §7 — not a silent consequence of ratifying category 4's ceiling.

**Resolves `ARCH_15_05`'s OPEN item 2** (where per-scenario dispatch branches and generator
functions live under the ratified file split). **OPEN item 1** (whether generator logic ever
becomes declarative `PARAMS` data) is unaffected and remains open — category 4 stays hand-authored
Lua, exactly as item 1 already assumed as the default posture.

