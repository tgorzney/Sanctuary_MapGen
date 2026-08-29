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

