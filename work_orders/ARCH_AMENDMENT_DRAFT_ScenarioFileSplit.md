# DRAFT — UNRATIFIED — for the ARCH Expert's dedicated setup conversation

**Status: UNRATIFIED DRAFT.** Produced 2026-09-03 by an advisory (read-only) consult on the SanGen
ARCH Expert's behalf — this document is **not** itself authoritative and binds nothing. It exists so
the human can carry a precise, ready-to-ratify text into the ARCH Expert's own dedicated setup
conversation. Nothing in `ARCH.md`, `ARCH_15_04_ThreeFileOnDiskShape.md`,
`ARCH_15_05_ParamsScenariosType.md`, or `sangen_arch_pack/` has been touched to produce this file.
Until ratified there, treat every clause below as a proposal, not law.

Full background/reasoning trail (not repeated here): the advisory consult that produced this draft,
recorded in the coordinator's session log; `MAP_SCENARIO_SPEC.md` §14; `ARCH_15_05_ParamsScenariosType.md`'s
"OPEN" section (item 2); `resources/lua/SanGenScenarioRuntime.lua:302-306`.

---

## Proposed amendment to §15.4 (`ARCH_15_04_ThreeFileOnDiskShape.md`)

Resolves `ARCH_15_05`'s OPEN item 2. Ratifies a fourth, open-ended on-disk file category alongside
the existing three, and amends the overwrite-safety mechanism and the "generic, identical across
every map" framing of `_Scenarios_Runtime.lua` to accommodate it.

### 15.4 (amended) — Four-category on-disk shape + amended overwrite safety

**The three-file list in the current §15.4 becomes a four-category list.** Categories 1–3 are
unchanged from the existing ratification; category 4 is new:

1. `<MapName>_data.lua` — hand-authored orchestrator. **Never written by SanGen, under any code
   path.** *(unchanged)*
2. `<MapName>_Scenarios_Runtime.lua` — the generic runtime algorithm
   (`FindMatchingScenario`/`ResolveAndApply`/the unit-spawn executor), **plus, as of this
   amendment, the generic per-scenario unit-spawn dispatcher** (below) **and any Lua helper
   function that is genuinely universal across every map's scenarios** (e.g. a spiral-grid
   position generator, a unit-grid append helper, a degrees-to-radians conversion — the kind of
   thing that has no per-map or per-scenario content of its own). SanGen-owned: a bundled
   resource, copied per map on every export. Content is byte-identical across every map, exactly
   as already ratified — this amendment adds to what belongs inside this one file, it does not
   relax the "identical across every map" property.
3. `<MapName>_Scenarios_Data.lua` — the per-map scenario tables, rendered from `Params::Scenarios`
   on every export. SanGen-owned, never hand-edited, never read back. *(unchanged)*
4. **NEW — `<MapName>_Scenarios_<ScenarioName>.lua`, one per scenario that opts into
   `spawnsUnits`, and optionally `<MapName>_Scenarios_<ScenarioName>_<FunctionName>.lua` when a
   single scenario's generator function alone would not fit the ceiling below.** Hand-authored,
   per-map, per-scenario, procedural Lua — the category `ARCH_15_05`'s OPEN item 2 identified as
   having "no named home." **Never written by SanGen beyond an initial create-if-missing scaffold**
   (see "Overwrite safety," point 3 below) — this is the same authorship posture as
   `<MapName>_data.lua`, not the same posture as categories 2/3.
   - Colocated in the same directory as the other three, `LJ/lua/maps/<MapName>/` — no change to
     that constraint; cross-tree `Import` remains impossible (`MAP_UNIT_SPAWNING_SPEC.md` §3).
   - Each file must expose its generator as a **global** function named
     `GenerateScenarioUnits(area)`, returning the flat `{armyIndex, templateIdentifier, x, y, z}`
     instruction array `Scenario.SpawnUnits` already consumes (`MAP_SCENARIO_SPEC.md` §11) — the
     same global-exposure requirement every other file in this system already follows
     (`SanGenScenarioRuntime.lua:19-23`), for the same `Import()`-only-captures-globals reason.
   - When further split one-function-per-file (the file-size ceiling below forcing it), the
     `<ScenarioName>.lua` file itself stays the one that defines `GenerateScenarioUnits` and
     becomes a thin composer: it `Import()`s its own `_<FunctionName>.lua` siblings and captures
     their globals into locals at its own top level, exactly the idiom
     `SanGenScenarioRuntime.lua:37-43` already uses to capture `ScenarioData`'s fields. No new
     `Import()` mechanic is introduced — this is the existing idiom applied one layer deeper.

**Dispatch mechanism — RULED, resolving `ARCH_15_05`'s OPEN item 2. The dispatcher is generic and
lives entirely inside `_Scenarios_Runtime.lua` (category 2); it is never an if/elseif chain, and it
is never edited per map or per scenario.** Concretely:

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

- **Ruled: lazy, not eager.** The path is built and `Import()`'d **only for the one scenario that
  actually matched** at `Scenario.ResolveAndApply` time (`currentMatchedScenarioName`, already
  captured at `SanGenScenarioRuntime.lua:261`). SanGen/the runtime never `Import()`s every
  authored scenario's generator file up front, never builds a name→function table eagerly, and
  never enumerates the full `Scenarios` set at dispatch time. This is a deliberate, binding
  performance ruling, not an implementation detail left open: load cost for this mechanism is
  **O(1) extra file(s) touched per map load** (the matched scenario's own file, plus whatever it
  itself `Import()`s), independent of how many scenarios the map author has authored in total —
  never O(N authored scenarios).
- A missing file for a scenario with `spawnsUnits == false` is the expected, silent, common case
  (no scenario needing no unit spawn ever gets a generator file) — not a collision, not an error,
  not logged.
- A missing file for a scenario with `spawnsUnits == true` is a real authoring gap; `pcall`
  swallows the `Import()` failure per this file's existing "everything in the deferred thread is
  independently `pcall`'d" law (`MAP_SCENARIO_SPEC.md` §3.1), so it degrades to "no units spawned"
  rather than aborting the thread — consistent with, not a new exception to, the existing ordering
  law.

**Universal-helper placement — RULED.** Helpers that are genuinely reusable across *any* map's
scenarios (the proposal's own examples: a spiral-grid position generator, a unit-grid append
helper, a degrees-to-radians conversion) are **folded into `_Scenarios_Runtime.lua` itself**
(category 2) as additional global functions — they are not a new file. Rationale: "reusable across
any map's scenarios, identical content every time" is exactly Runtime.lua's own existing
definition; inventing a fifth SanGen-owned bundled-and-copied file for content that already fits an
existing category's stated role adds a file with no corresponding new *kind* of content. **Fallback
only:** if folding all universal helpers into Runtime.lua would push that file over the ceiling
ruled for it (a ceiling decision this amendment does not make — see "File-size ceiling" below), a
second bundled, byte-identical, copied-per-map-on-export file, `<MapName>_Scenarios_Helpers.lua`
sourced from a new bundled resource `SanGenScenarioHelpers.lua` (mirroring
`ScenarioScript_RuntimeResource_IO.h`'s existing resolve/copy mechanism for Runtime.lua), is
introduced — never a per-map, per-scenario helper file.

**Overwrite safety — amended from a fixed literal-filename list to a pattern.** The existing
mechanism (banner marker + loud file-scoped refusal) is unchanged for categories 2–3. What changes:

1. **Filename disjointness, restated as a pattern.** SanGen writes:
   - Categories 2 and 3 (`_Scenarios_Runtime.lua`, `_Scenarios_Data.lua`) — always regenerated on
     every export, exactly as already ratified.
   - Category 4 files (`_Scenarios_<ScenarioName>.lua`, `_Scenarios_<ScenarioName>_<FunctionName>.lua`)
     — **written exactly once, only when absent** (see point 3). SanGen never writes category 1
     (`_data.lua`) or the legacy `_Scenarios_Script.lua`, unchanged.
   - The collision check for categories 2–3 therefore stays a literal two-name check; the
     protection for category 4 is a **glob/pattern match** (`<MapName>_Scenarios_*.lua` excluding
     the two literal category-2/3 names) used only to decide "does a file already occupy this
     scenario's generator slot," never to decide whether to overwrite it — category 4 is never
     overwritten once it exists, regardless of content, marker, or absence of one.
2. **Generated-file header marker — category 4 gets its own, weaker banner, not the "DO NOT
   HAND-EDIT" one.** A scaffold created under point 3 below opens with a distinct marker (e.g.
   `-- SANGEN-CREATED STARTING POINT -- freely hand-edit -- never regenerated`), signaling
   create-once-then-hands-off, not create-then-regenerate. This is a genuinely different
   overwrite-safety class from either existing one (see point 3).
3. **A third overwrite-safety class: create-only-if-missing, then permanently hands-off.**
   Existing law recognizes two classes: "always regenerate, refuse on foreign-marker collision"
   (categories 2–3) and "never write, ever, under any code path" (category 1). Category 4 is
   neither: on export, for every scenario with `spawnsUnits == true` whose expected generator file
   does not yet exist, SanGen writes a minimal scaffold (`GenerateScenarioUnits(area) return {} end`
   under the point-2 marker above) **once**; on every subsequent export, if the file already
   exists — with the scaffold marker, with a human's own edits, with no marker at all, in any
   state — SanGen **never touches it again**. This is simpler and safer than trying to distinguish
   "still the untouched stub" from "a human has started editing it," and mirrors category 1's own
   "keep backups, SanGen will not help you here again" posture, one notch softer (SanGen gives you
   a starting point once, then gets out of the way permanently).
4. **Loud, file-scoped logging is unchanged in spirit:** creating a category-4 scaffold is logged
   exactly as visibly as writing categories 2–3; skipping an already-present category-4 file is a
   quiet, expected no-op (not a warning) — this is the common case on every export after the first.

**File-size ceiling for category 4 — NEW, scoped narrowly.** Per the human's proposal: soft 100 /
hard 150 lines per category-4 file, functions ≤ 40 lines, mirroring `ARCH_01_05_FileSizeCeilings.md`'s
C++ convention by analogy (the same AI-legibility/edit-blast-radius rationale transfers to whoever
edits these files by hand — today a human map author, potentially a future SanGen-hosted
`LuaCodeEditor_UI` session, `ARCH_15_08_ThirdPartyDependencyRuling.md`). A scenario's generator
function must be split one-file-per-function (the point-4-of-§15.4 "further split" rule above) the
moment a single function alone would not fit.

**This ceiling explicitly does NOT apply retroactively to `_Scenarios_Runtime.lua` (category 2) or
the `_Scenarios_Data.lua` renderer's output (category 3).** `_Scenarios_Runtime.lua`
(`resources/lua/SanGenScenarioRuntime.lua`) is already ratified, already shipping, and is 309 lines
— over double the proposed hard ceiling — because it was written and ratified before this amendment
existed and under `ARCH_15_01_LayerClassification.md`'s framing that this Lua system sits entirely
outside the Constitution's layer stack. Retrofitting Runtime.lua/Data.lua's own sizing (including
the "fold universal helpers into Runtime.lua" ruling above, which grows it further) is a separate,
independently-decided future question — not silently forced by this amendment. If Runtime.lua's
size becomes a live concern, that is its own future ratchet under Constitution §7, not an automatic
consequence of ratifying category 4's ceiling.

---

## Proposed addendum to §15.5 (`ARCH_15_05_ParamsScenariosType.md`)

**New validation rule on `ScenarioBody::name` — ADDED by this amendment, consequence of the §15.4
dispatch ruling above.**

`name` (`ScenarioBody::name`, `Scenario_PARAMS.h`) has, until now, been documentation/log-identifier
data only — read for `Log`/`Warn` text and, in the pre-amendment live reference, as an in-file
`if/elseif` string compare (`MAP_SCENARIO_SPEC.md` §11). **Under the §15.4 dispatch ruling above,
`name` becomes a literal filesystem path component** — it is string-formatted directly into the
`Import()` path SanGen and the runtime both use to locate a scenario's generator file
(`<MapName>_Scenarios_<ScenarioName>.lua`). This is a real new load-bearing role, not cosmetic, and
needs validation it did not need before:

- **Safe-filename charset.** `name` must be non-empty and match a restricted, portable charset —
  provisionally `^[A-Za-z][A-Za-z0-9_]*$` (must start with a letter; letters/digits/underscore only;
  no path separators, no spaces, no punctuation Windows or the game's own filesystem layer could
  treat specially). Existing live-reference names (`slots5to8AnyFilled`, `4human`, `2h1ai`,
  `floor169`) satisfy this pattern already; `1v1` does **not** (leading digit) and would need
  renaming (e.g. `oneVsOne`) under this rule — flagged here explicitly since it is a real, named
  live example that fails the proposed charset, not a hypothetical.
- **Case-insensitive uniqueness across the whole `Scenarios` set.** `name` must be unique — checked
  case-insensitively — across `patternScenarios`, `countScenarios`, and `defaultScenario` combined
  within one `Params::Scenarios`. Rationale: Windows (and this system's target filesystem) folds
  case for file lookup, so `"FooBar"` and `"foobar"` would silently resolve to the same
  `Import()` path today with no error — a strictly worse, newly-introduced failure mode this
  amendment must close, not merely inherit. (Prior to this amendment, duplicate/differently-cased
  `name` values were merely a confusing log/debug identifier collision, never a functional
  collision — this addendum is what makes uniqueness load-bearing.)
- **Enforcement point and posture:** validated at the same layer/points `ARCH_15_10`'s
  `maxArmySlotCount` and this file's own `areaName` staleness checks are validated (UI-authoring
  time and export time), loud-logged on violation, **never silently renamed, truncated, or
  deduplicated by SanGen** — matching Constitution §6's "loud, logged, never silent" posture and
  this exact file family's own established idiom (`ARCH_15_05`'s existing stale-`areaName`-reference
  handling). A violating export is not blocked outright (consistent with this file's existing
  "never a flat refusal" posture elsewhere) but the specific scenario's category-4 file write is
  refused with a named, specific error, exactly as an unrecognized-collision refusal already works
  for categories 2–3 under §15.4 — this is a new instance of an existing refusal shape, not a new
  refusal mechanism.
- **No other field on `ScenarioBody`/`PatternScenario`/`CountScenario` changes.** This addendum is
  additive validation only; the binding C++ shape in the existing §15.5 code block is unchanged.

---

## Resolves

**`ARCH_15_05`'s OPEN item 2** ("where per-scenario dispatch branches and generator functions live
under the ratified three-file split is unresolved") is resolved by the §15.4 amendment above: they
live in the new category-4 files, dispatched generically and lazily from inside the unchanged
category-2 `_Scenarios_Runtime.lua`. **OPEN item 1** (whether generator logic ever becomes
declarative PARAMS data) is **not** addressed and remains open — this amendment keeps category-4
content hand-authored Lua, exactly as OPEN item 1 already assumed as the default posture.

## Scope for implementation (pointer only, not a work order)

Once ratified, the coder-facing work order should cover, at minimum:
- `ScenarioScript_Export_IO` (`src/io/ScenarioScript_Export_IO.cpp/.h`): add the third
  overwrite-safety class (create-scaffold-if-missing, then never touch) for category-4 files, and
  the pattern-based (not literal-list) collision check described above.
- `resources/lua/SanGenScenarioRuntime.lua`: add the generic dynamic-dispatch
  `Scenario.SpawnMatchedScenarioUnits` body shown above, and fold in whichever universal helpers
  (`GetSpiralGridXZ`, `AppendUnitGrid`, `DegreesToRadians`-equivalents) are confirmed genuinely
  map-agnostic from the live reference.
- `Scenario_PARAMS.h` / the Scenarios-tab UI: add the `name` charset + case-insensitive-uniqueness
  validation from the §15.5 addendum.
- No changes to `MapImporter_ScenarioRecord_IO.cpp` or any read-back path — this amendment adds no
  new "read Lua back" capability anywhere, consistent with `ARCH_15_03`.
