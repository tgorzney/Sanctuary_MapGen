# STEP78 — Scenario Edit Mode: interactive canvas marker editing (GATED, not dispatchable yet)

**Layer:** UI. **Domain:** `MapCanvas`, the ARCH_14_PreviewOverlayLayering.md §14 screen-space overlay stack, `ScenariosTab_UI`.
**Sequence:** Map Scenario track, Work-Order 8 of 8, UI leg, part 3 of 3 — the interactive-canvas
piece `DESIGN_ScenariosTabAndLuaEditor_R1.md` §6 calls "the core ask."

## ⚠️ BLOCKED — do not dispatch to the Coder until every prerequisite below lands

This ticket is written now (full design, ready to implement) so it can be reviewed alongside
STEP74/75, but its dependency chain is **materially wider than the single risk originally named**
(`STEP47_WorldScreenProjection_UI`). Re-checked against the current tree:

- **STEP47** (`WorldScreenProjection_UI`) — **DRAFTED, not landed** (confirmed:
  `MapCanvasView::ProjectPreviewPixelToRegionLocal` does not exist in `src/ui/MapCanvasView_UI.h`).
  Supplies the world↔screen math this ticket's drag/hit-test needs in both directions.
- **STEP50** (per-layer CSR bucket index over `Data::PlacementInstances`) — needed to locate the
  real baked Alloy/Spawns-Armies instances the ghost baseline reads. Not confirmed landed.
- **STEP51** (`OverlayLayer_UI`/`OverlayDomainKind_UI`/`overlayLayers`) — the ghost baseline
  explicitly reuses "the ARCH_14_PreviewOverlayLayering.md §14 baseline Alloy/SpawnsArmies overlay layers... as a ghost
  baseline"; that reuse target must exist first. Confirmed not in `src/`.
- **STEP52** (atlas pairing lookup) / **STEP53** (`MapCanvas_IconLayer_UI`, the icon draw pass) —
  the rendering machinery this ticket's icons are drawn through. Confirmed not in `src/`.

**None of STEP47/50/51/52/53 are landed.** This ticket cannot be usefully started until at minimum
STEP47 + STEP51 land (world↔screen math + the overlay stack to ghost against); STEP52/53 are needed
for actual icon rendering once interaction logic exists. **Do not dispatch before confirming all
four have landed** — that confirmation is this ticket's own gate, not assumed here.

**This gating does NOT block STEP74 or STEP77** — neither touches `MapCanvas`, `OverlayLayer_UI`, or
any world↔screen math; both are buildable, testable, and shippable today. A designer can fully
author scenario data (lists, rules, matrix, spawns/alloys as flat-list rows, the Lua editor, export)
with STEP74+75 alone. This ticket only upgrades spawn/alloy-position authoring from "flat list of
float fields" to "drag on the map."

## Root problem
`Params::ScenarioBody::spawns`/`alloys`/`alloysToAdd`/`alloysToRemove` hold world-space positions;
STEP74 ships only a flat-list numeric editor for them (honest, but low-fidelity — no visual feedback
against the actual baked map). `DESIGN_ScenariosTabAndLuaEditor_R1.md` §6 designed a dedicated draw
pass reusing the ARCH_14_PreviewOverlayLayering.md §14 overlay renderer as a ghost baseline; nothing in `src/` implements any
part of it.

## Fix — carried forward from the design's §6, re-verified against the current ARCH

The original §6 content is **still accurate** against `ARCH_14_PreviewOverlayLayering.md` §14/§15.5 as ratified — re-checked
line by line, no corrections required (unlike §1/§3/§7, which needed the fixes STEP74/75 apply).
Restated as binding scope, with type names updated to §15.5's ratified
`ScenarioBody`/`ScenarioSpawn`/`ScenarioAlloyOverride` (the design's
`Scenario`/`ScenarioArmySpawn`/`ScenarioArmyAlloys` are retired, per STEP74):

- **Not the generic `OverlayLayer_UI` machinery** — a dedicated, transient, single-scenario draw
  pass, because (a) it is not a stackable View-toolbar layer, (b) its source is neither
  `Data::PlacementInstances` nor a `recipe.*Layer` array (it is the OPEN scenario's own
  `spawns`/`alloys`/`alloysToAdd`/`alloysToRemove` plus the baseline overlay as ghost context),
  (c) it needs per-state visuals no generic layer carries. **Does** reuse §14.9's primitives (bulk
  vertex writes, the atlas, `MapCanvasView`'s projection once STEP47 lands) and the baseline
  Alloy/SpawnsArmies overlay layers, desaturated, as ghost context.
- **Cardinality is tens of entries** — explicitly does not need `Data::SpatialGrid`/`Picking_UI`
  O(1) machinery; a linear screen-rect hit test is correct. Flagged so nobody over-engineers it.
- **State-distinguishing visuals** (never rely on identical screen position — two markers can
  coincide exactly):

  | State | Visual |
  |---|---|
  | Spawn, no override (no explicit row for this army) | hollow/dashed icon + ⚠ badge at baseline position |
  | Spawn, explicit override (a `ScenarioSpawn` row exists) | solid filled icon, even if numerically identical to baseline |
  | Alloy, kept | baseline icon, normal color |
  | Alloy, deleted (occupancy empty slot, or omitted under `explicit`) | greyed + strike-through/X |
  | Alloy, added (`explicit` list or `alloysToAdd`) | baseline-style icon in an "added" tint |
  | Alloy, `alloysToRemove` entry | ghost icon with red X — distinct from `explicit`'s plain omission |

- **Per-army grouping**: tinted by `Params::Army::armyColor`. Zero new PARAMS. A legend strip lists
  army name/color.
- **"Preview As" control**: `occupancy`/`keepAll` only make sense against a concrete composition — a
  scratch slot-pattern editor (reusing STEP74 §3's toggle row) defaults to the scenario's own
  pattern if Tier 1, else a synthesized composition satisfying the Tier-2 predicate
  (`MatchesScenarioConditions`, STEP74). UI-session scratch state only, never written to the recipe.
- **Interaction**:
  - Left-drag a solid (explicit) spawn: moves it, writes to the matching `ScenarioSpawn` row.
  - Left-drag a hollow (inherited) spawn: **first drag materializes it** — seeds a new
    `ScenarioSpawn` from the real baseline position (resolvable here since the ghost layer reads
    real baked instances — closing STEP74 §4's flagged "zeroed seed" gap), continues the drag live.
    Operationalizes the mandatory-spawns rule at the point of highest leverage: touching the canvas
    can't leave a scenario silently inheriting.
  - Right-click a baseline alloy → "Remove for this scenario" (appends a `ScenarioAlloyRemoval` when
    `alloyMode == Delta`, or removes/omits per the active mode's semantics). **Disabled +
    tooltipped, not silently no-op**, when `alloyMode == KeepAll`.
  - Right-click empty canvas near an army's territory → "Add Alloy Marker for [Army]".
- **Mode entry/exit**: explicit opt-in toggle on the open scenario's detail panel (STEP74's
  `ScenariosTab_Detail_UI.cpp`), default off — browsing the list must never hijack the canvas. Takes
  exclusive canvas-interaction ownership while active; auto-exits when the panel closes or the
  toggle flips off.

## Files touched (provisional — finalize against STEP47/50-53's actual shipped signatures)
- NEW `src/ui/MapCanvas_ScenarioEditMode_UI.h`/`.cpp` (+ split siblings per ARCH_01_05_FileSizeCeilings.md §1.5 as needed).
- EDIT `src/ui/ScenariosTab_Detail_UI.cpp` (STEP74) — the mode-entry toggle.
- EDIT `src/ui/MapCanvas_UI.h`/`.cpp` — exclusive-interaction-ownership wiring while active.

## Backend policy
CPU-side imgui/`ImDrawList` interaction only, reusing §14.9's already-mandated bulk-vertex-write
primitives for the icon draws (no new backend decision beyond what STEP53 establishes).

## ARCH rules invoked
- `ARCH_14_PreviewOverlayLayering.md` §14 (the overlay layering redesign the ghost baseline sits on) and §14.9 (bulk vertex
  writes, atlas bucketing — reused, not reinvented).
- `ARCH_15_05_ParamsScenariosType.md` §15.5 — the real `ScenarioBody`/`ScenarioSpawn`/`ScenarioAlloyOverride`/
  `ScenarioAlloyRemoval` shapes written to.
- `MAP_SCENARIO_SPEC.md` §6 — the mandatory-spawns rule the "materialize on first drag" interaction
  directly operationalizes.

## Explicit out-of-scope
- **Everything STEP47/50/51/52/53 themselves ship** — consumed once landed, never substituted.
- **List management, rule authoring, the Lua editor, export flow** — STEP74/75, complete without this.

## Acceptance test (provisional — cannot be finalized until STEP47/51/53 land)
1. The six-state visual table above: each renders visually distinctly (screenshot-diff or
   draw-call-inspection, per whatever convention STEP53 establishes for its own icon draw pass).
2. Left-drag materialization: dragging a hollow spawn icon produces exactly one new `ScenarioSpawn`
   seeded from the **real** baseline position (not zeroed), and the icon renders solid next frame.
3. `alloyMode == KeepAll` → the "Remove for this scenario" item is present but disabled, with a
   tooltip explaining why (never silently absent, never a silent no-op click).
4. Mode toggle off (or panel close) → the canvas returns exclusive-interaction ownership to the
   normal picker/selection path, verified by a state-ownership test, not visual inspection.
5. Full solo rebuild + `ctest -C Debug`: previously-passing suite stays green.

## Verify
- Re-run the "confirm all prerequisites landed" gate at the top **before** dispatch — this is the
  actual precondition for this ticket existing as anything other than a design record.
- Once implemented: new acceptance tests pass; full solo rebuild + `ctest -C Debug` 100% pass, zero
  pre-existing test files edited or broken.
