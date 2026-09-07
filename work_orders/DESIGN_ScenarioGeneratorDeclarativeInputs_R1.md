# DESIGN — Declarative "Inputs" for Per-Scenario Unit-Spawn Generators (R1)

**Revision 2 (2026-09-07)** — corrects revision 1 on two points, per direct human correction:
1. Revision 1 grounded "what resists declarativity" in the live reference file's battleship/naval
   deepest-water anchor search (`FindDeepestWaterNear`/`FindFleetAnchorForArmy`). **That mechanic is
   deprecated** — it is no longer a live design target, and every reference to it as a grounding
   example is removed below. This also removes the "some things genuinely resist declarativity"
   claim revision 1 built on that example — nothing in the still-relevant part of the live
   generator has been shown to resist a declarative shape.
2. Revision 1 mis-modeled "select a custom script" as sharing the SAME SanGen-managed file/path a
   declarative mode would also render into, which forced a whole "which write posture wins"
   sub-design (regenerate vs. hands-off, a mode-switch danger, a new overwrite-safety class). **The
   human's actual model is simpler and doesn't have this problem**: Declarative mode and Custom
   Script mode target two independent things — Declarative mode's output is a file SanGen owns and
   regenerates; Custom Script mode is a **reference to a separate, always human-owned file** the
   author selects or writes directly (reusing the already-shipped Lua script editor). SanGen never
   writes into the referenced file, so there is no shared-file conflict, no ambiguity, and no
   dangerous mode-switch — §4 is rewritten accordingly.

*Analysis consult. NOTHING in this document is ratified. No PARAMS/ARCH/IO type is invented here,
only sketched-and-flagged, per Constitution §7 / `ARCH_08_04_CoderScopeLaw.md` §8.4. No code,
`.sanmap`, or `.lua` file has been touched to produce this — read-only research only.*

Grounded in `ARCH_15_05_ParamsScenariosType.md`'s OPEN item 1 (lines 302-318), `ARCH_15_04_
ThreeFileOnDiskShape.md`'s category-4 amendment, `MAP_SCENARIO_SPEC.md` §11/§11.1/§11.2/§14, and
the already-shipped per-map "Runtime Script (advanced)" override system (STEP77:
`LuaCodeEditor_UI`, `ScenariosTab_RuntimeScript_UI.cpp`, `ScenarioScript_RuntimeResource_IO.*`) —
the existing, working precedent for "a human selects/writes a Lua file, SanGen references it
without owning its content," which this document leans on directly.

**Confirmed against real code:** `src/params/Scenario_PARAMS.h` has no `SlotRangeOccupiedCount`
field yet, and `src/io/ScenarioScript_Export_IO.cpp` has no `GenerateScenarioUnits`/scaffold/
banner logic at all. Category 4 (`ARCH_15_04`, amended 2026-09-03) is ratified law but **not yet
implemented** — this document's proposals sit on top of a category-4 mechanism that itself does
not exist in `src/` today. Any work-order that follows from this design must build category 4
first, or alongside.

## 0. The human's direction, restated

Per-scenario unit spawning gets exactly two authoring choices:
1. **Declarative inputs** — UI-authored fields (unit type, count, formation, etc.), SanGen renders
   and owns the resulting Lua.
2. **Custom script** — the author selects (or writes, via the already-shipped in-app Lua editor) a
   script file; SanGen stores a reference to it and calls its entry point at spawn time. SanGen
   never generates or owns that file's content.

This is intentionally simple, and directly mirrors a mechanism that already ships today at the
map-wide level (the Runtime Script override, STEP77) — just applied at the per-scenario,
unit-spawning-specific granularity instead of the whole map's algorithm file.

## 1. What a declarative vocabulary would need to express

Derived from the still-relevant, non-deprecated parts of the live reference generator
(`Pandemonium Isthmus_Scenarios_Script.lua`'s `BuildSlots5to8Instructions` and `AppendUnitGrid`) and
from `MAP_SCENARIO_SPEC.md` §12's own worked example (`Build1v1Slots3and4Instructions`).

### 1.1 Unit type / tpId selection — declarative
A bare `std::string templateIdentifier` field with a tpId picker. No open question.

### 1.2 Count — declarative
A bare `int count` field. No open question.

### 1.3 Formation/layout shape — declarative, one shape has live evidence
`AppendUnitGrid` (:627-658) is a rectangular grid: `columns = ceil(sqrt(count))`, `rows =
ceil(count/columns)`, laid out from a computed start point at fixed `spacing`. Every parameter
(`count`, `spacing`) is already a scalar the live code passes in — a `GridLayout { spacing }` mode
covers it exactly. No other formation (ring, spiral, line) has live evidence; recommend the
formation vocabulary start as `{ Grid }` only, extended later if/when a second real scenario
demonstrates a different shape.

### 1.4 Anchor position — declarative
`MAP_SCENARIO_SPEC.md` §12's worked example anchors a grid directly on the matched army's own
`Spawn` marker plus a fixed offset — no search, no iteration, purely an authored `(offsetX,
offsetZ)` pair added to a value already resolvable from `GameInfo.MapData.markers.Spawn`. This is
fully declarative with no caveats.

### 1.5 Altitude — declarative as a mode, with one small caveat
Two live behaviors: spawn at `waterLevel` (`Engine.GetWaterLevel()`, a single zero-argument
runtime query), or spawn at `sampledGroundHeight + offset` (one `Engine.SampleTerrainHeightFromCell`
call). Both are cheap, single-call, non-iterative engine queries — an enum `{ AtWaterLevel,
AboveTerrainAtAnchor(offset) }` is a clean declarative surface. The only caveat: like every field
here, the actual number isn't known until Lua runtime (no live engine access at SanGen export
time), so a small shared Lua helper still issues the real call — the field is authored data, the
value is resolved live. This is a normal, unremarkable consequence of the whole system's existing
design (every scenario field already works this way — see `MAP_SCENARIO_SPEC.md` §3's execution
timing), not a blocker.

### 1.6 Water requirement — declarative, already exists as a flag in the live code
`AppendUnitGrid` already exposes a boolean `requiresWater` parameter (nudging individual grid
points via `FindNearbyWaterSpot`, a small, bounded, first-hit spiral — not the deprecated
deepest-water search). This is a clean, already-parameterized boolean field.

**Net effect of removing the deprecated example:** nothing identified in the still-relevant parts
of this system currently resists a declarative shape. If a future scenario needs real procedural
placement logic that doesn't fit the grid/anchor/altitude vocabulary above, that's exactly what
Custom Script mode is for — but that's a general safety valve, not evidence that any *specific*
part of today's live design needs it.

## 2. The "custom script" mode — concrete shape (simplified from revision 1)

**One new field**, following this file family's established "flat sibling fields, only the ones
matching the record's own mode are meaningful" idiom (`ScenarioBody`'s `alloys`/`alloysToAdd`/
`alloysToRemove` already do this). Illustrative sketch only, **not ratified**:

```cpp
// ILLUSTRATIVE ONLY — NOT RATIFIED. Naming, placement, and default are all open.
enum class ScenarioUnitSpawnMode { CustomScript, Declarative };

struct ScenarioBody {
    // ... existing fields unchanged ...
    ScenarioUnitSpawnMode unitSpawnMode = ScenarioUnitSpawnMode::CustomScript;  // meaningful only
                                                                                  // when spawnsUnits
                                                                                  // == true

    // Meaningful only when unitSpawnMode == CustomScript:
    std::string customUnitSpawnScriptPath;   // reference to a human-owned .lua file; SanGen
                                              // never writes its content, only Import()s it.

    // Meaningful only when unitSpawnMode == Declarative:
    // std::vector<ScenarioUnitFormation> declarativeUnitFormations;  -- see §3
};
```

`CustomScript` as the default, with an empty `customUnitSpawnScriptPath`, preserves every
currently-authored scenario's behavior unchanged (no path selected = nothing spawns, same as
`spawnsUnits == true` with no dispatch today).

**Authoring surface — reuse what already ships, don't invent a new editor.** STEP77's
`LuaCodeEditor_UI` + `ScenariosTab_RuntimeScript_UI.cpp` already implement exactly the mechanic
needed: pick or write a Lua file, syntax-check it live via the already-vendored LuaJIT
(`ARCH_15_08`), save it, and have SanGen reference/copy it on export without ever trying to parse
or merge its content. The per-scenario Custom Script field should reuse this same widget, scoped to
one scenario's unit-spawn entry point instead of the whole map's runtime algorithm — a new call
site into existing infrastructure, not new infrastructure.

## 3. A concrete declarative mode — "Grid Spawn Near Army's Own Spawn Marker"

Grounded in `MAP_SCENARIO_SPEC.md` §12's own worked example — a land-unit grid anchored on an
army's own `Spawn` marker.

**Illustrative PARAMS sketch — NOT ratified:**

```cpp
// ILLUSTRATIVE ONLY — NOT RATIFIED.
enum class ScenarioUnitAltitudeMode { WaterLevel, AboveTerrainAtAnchor };

struct ScenarioUnitFormation {
    std::string templateIdentifier;         // tpId, e.g. "ucl4004"
    int         count          = 0;
    float       spacing        = 0.0f;      // AppendUnitGrid's `spacing`
    float       anchorOffsetX  = 0.0f;      // offset from the army's own Spawn marker
    float       anchorOffsetZ  = 0.0f;
    bool        requiresWater  = false;     // per-grid-point FindNearbyWaterSpot nudge
    ScenarioUnitAltitudeMode altitudeMode = ScenarioUnitAltitudeMode::WaterLevel;
    float       altitudeAboveTerrain = 0.0f;  // meaningful only for AboveTerrainAtAnchor
};

// ScenarioBody gains (meaningful only when unitSpawnMode == Declarative):
// std::vector<ScenarioUnitFormation> declarativeUnitFormations;
```

**Render target.** The category-4 file's `GenerateScenarioUnits(area)` body gets real generated
Lua, calling into a small set of bundled helpers (`ARCH_15_04`'s already-ratified "universal
helpers" home, folded into `_Scenarios_Runtime.lua`):

```lua
-- Illustrative generated content, NOT ratified wording.
function GenerateScenarioUnits(area)
    local instructions = {}
    for armyIndex, army in pairs(Armies) do
        if not (army.lobbyOptions and army.lobbyOptions.isEmptySlot) then
            local anchorX, anchorZ = ScenarioHelpers.ResolveArmySpawnAnchor(army.name, 30, 0)
            if anchorX then
                ScenarioHelpers.AppendUnitGrid(instructions, armyIndex, "ucl4004",
                    anchorX, ScenarioHelpers.WaterLevel(), anchorZ, 8, 6, false)
            end
        end
    end
    return instructions
end
```

Every piece of this — the grid layout, the occupied-army iteration idiom, the army-marker anchor,
the altitude modes, the water-nudge flag — is already a parameterized scalar in the live reference
generator (§1). No part of this first declarative mode has an identified gap.

## 4. Custom Script mode has no file-ownership conflict (corrects revision 1's error)

Revision 1 assumed Declarative and Custom Script modes both target the same SanGen-managed
category-4 path, which forced a "who gets to write this file" conflict and a dangerous mode-switch
scenario. **That assumption was wrong.** The two modes target different things entirely:

- **Declarative mode**: SanGen renders `GenerateScenarioUnits(area)` into the scenario's own
  generated file, and — like `_Scenarios_Data.lua` — regenerates it every export. This file is
  always SanGen-owned; a human never hand-edits it (if they want to, that's what Custom Script mode
  is for).
- **Custom Script mode**: `customUnitSpawnScriptPath` (§2) references a *separate* file the human
  selected or authored via the reused script editor (§2). SanGen `Import()`s it at runtime and
  calls its entry point; SanGen's export step never writes to it, full stop — the same posture the
  already-shipped Runtime Script override already has today (`ScenarioScript_RuntimeResource_IO`
  resolves and copies an override file on export; it does not generate or merge its content).

**Mode switching is safe in both directions**, because switching modes only changes which field
SanGen reads (`declarativeUnitFormations` vs. `customUnitSpawnScriptPath`) — it never causes SanGen
to write over a human-authored file. Switching from Custom Script to Declarative simply stops using
the referenced script (which stays on disk, untouched, exactly as the human left it) and starts
rendering the Declarative-mode file instead. Switching back re-references the same script path,
unchanged. No confirmation gate, no destructive-write risk, no new overwrite-safety class needed —
revision 1's §4 in full is retracted.

**The only real design point left here:** if a human picks Custom Script mode with no file selected
yet, does the UI offer a "create new" path that scaffolds a minimal starting script via the reused
editor widget (mirroring what STEP77's Runtime Script override UI already offers for a first-time
override)? Recommend yes, reusing that existing affordance verbatim rather than inventing a second
one — flagged for the UI Expert, not ruled here.

## 5. Recommendation — minimum viable vs. explicitly deferred

**Minimum viable, if ratified:**
1. One new field, `unitSpawnMode`, on `ScenarioBody`, defaulting to `CustomScript` — zero behavior
   change for every currently-authored scenario.
2. One new field, `customUnitSpawnScriptPath`, meaningful only in `CustomScript` mode — a reference
   SanGen stores and `Import()`s, never writes to (beyond an optional one-time "create new" scaffold
   reusing STEP77's existing editor affordance).
3. Exactly **one** declarative formation shape: rectangular grid, anchored on the matched army's own
   `Spawn` marker plus a fixed offset, optional water requirement, altitude = waterLevel or
   terrain+offset (§3) — the shape `MAP_SCENARIO_SPEC.md` §12's own worked example already
   demonstrates.
4. A small number of shared, bundled Lua helpers (grid layout, occupied-army iteration, the water
   nudge) folded into the already-ratified "universal helpers" home (`ARCH_15_04`).
5. The per-scenario Custom Script field reuses the already-shipped `LuaCodeEditor_UI` widget
   (STEP77) rather than a new editor surface.

**Explicitly deferred / out of scope for a first ratification:**
- Any formation beyond rectangular grid (ring, spiral, line) — zero live evidence.
- Any general node-graph / visual-scripting authoring surface — the hybrid "declarative fields +
  escape-hatch custom script" model is the whole point, not a general system.
- Any procedural placement algorithm (search/scan-style logic) inside SanGen's own bundled Lua —
  not currently justified by any live, non-deprecated example; Custom Script mode is the answer for
  this class of need if and when it arises.

## ❓ Open questions for the ARCH Expert

1. **Field shape for the mode discriminator** — a flat sibling enum on `ScenarioBody` (§2's
   sketch), consistent with `alloyMode`'s existing precedent?
2. **Which file category Declarative-mode content belongs to.** It's rendered, not hand-authored —
   philosophically closer to category 3 (`_Scenarios_Data.lua`) than category 4. Should it live at
   category 4's path (since it's still per-scenario, keyed by name) with category 3's *regenerate*
   posture, or does it warrant being named as its own category even though nothing about its
   physical location changes?
3. **Formation-to-army filtering.** Does a first declarative formation need its own army filter, or
   is "applies to every army the matched scenario is relevant to" sufficient for an MVP?
4. **Where the shared Lua helpers live** — directly inside `_Scenarios_Runtime.lua` (already named
   as the fallback home for universal helpers), or a separate bundled helpers file?
5. **Extending `LuaCodeEditor_UI`'s existing selection/creation affordance to a per-scenario
   scope** — is this purely a UI Expert implementation detail, or does referencing a
   scenario-scoped script file (rather than the one map-wide Runtime Script override) need its own
   ARCH-level file-location/naming ruling (e.g. where such human-authored scripts live on disk
   relative to the four already-ratified categories)?

## Reuse candidate assessed and rejected: `Params::Army`/`UnitGroup`/`UnitTransform`

Read `src/params/Army_PARAMS.h` and `sangen_arch_pack/specs/ENTITY_AUTHORING_PARAMS_SPEC.md`
(lines 260-360) directly, since `ARCH_15_05`'s OPEN item 1 explicitly names this family as
considered-but-unevaluated. Verdict: **poor fit, not recommended.** That family round-trips
*baked, absolute, design-time-authored* coordinates through the `.sanmap`'s `armies` dictionary —
positions a human places once on the canvas and that stay fixed. Scenario unit-spawn positions are
the opposite: computed live at game load from runtime terrain samples and the current lobby's army
markers, and vary per composition. There is no coordinate to bake at export time, and the Scenario
system's Lua-rendering pipeline has no wire home for `UnitGroup`/`UnitTransform`'s shape today.
