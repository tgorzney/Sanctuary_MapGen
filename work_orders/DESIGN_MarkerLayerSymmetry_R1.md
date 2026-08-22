# Design Output — Markers Tab UI + Layer-Scoped Symmetry, Round 1

**⚠️ Correction from the human, post-round-1 (not yet folded into the sections below):** no fixed
anchor. Any member of a symmetry group is draggable; dragging ANY one recomputes all others from
its new position, acting as the source. §2/§3's "anchor vs. sibling, siblings not draggable" text
is superseded.

**Generator Expert verification of that correction (factual, not design):**
- **Math is correct and safe for every legal bit combination.** Regenerating the orbit from an
  arbitrary existing member reproduces the same physical point set (proven via group theory —
  every combo is a fixed-center reflection/rotation group, right-multiplication is a bijection —
  and cross-checked against `Placement_Symmetry_PROC_Test.cpp`'s hardest cases). No combination
  needs to be blocked/disallowed in the UI on correctness grounds.
- ⚠️ **Real blocker, unresolved**: `BuildSymmetryOrbit` is stateless — it has no concept of "this
  output point used to be instance C." Regenerating from a dragged member produces brand-new
  coordinates for every other member with **no built-in correspondence** to which existing
  `MarkerTransform` should receive which new position. No nearest-point or index-based scheme
  exists today, and naive nearest-point matching is provably unreliable for non-abelian combos
  (`QuarterTurns`/`Radial` + a mirror) since a drag can rotate the whole point cloud. **This means
  per-sibling identity (alias, army assignment) cannot be reliably preserved across an
  arbitrary-member drag without new design work** — not an implementation detail, a real open
  question for whoever designs the drag mechanism.
- ⚠️ **Orbit size can change mid-drag.** Orbit cardinality is a function of (mask, exact position),
  not just mask — a point landing exactly on a mirror axis or the map center has a smaller
  stabilizer and collapses to a smaller orbit; moving off-axis grows it back. A drag gesture may
  need to spawn/destroy group members live, not just reposition them — with the same identity
  question above applying to newly-spawned members.
- Combo test-coverage gaps found (not correctness bugs, just unverified): isolated `MirrorAcrossX`/
  `MirrorAcrossZ` alone, isolated `QuarterTurns` alone, mirror+`QuarterTurns` without `Radial`, and
  the on-axis mirror degenerate case. Worth adding tests, not a design blocker.

Consult of `BRIEF_MarkersTabUI_R2.md`. SanGen UI Expert only this round. Not yet ratified — no
coder-dispatchable ticket exists until ARCH (and Format/Generator/IO Architecture Experts) rule
on the items in §4. Supersedes `GAP_MarkerLayerAndSymmetry_PARAMS.md` Gap 2's three-option menu.
`STEP49_ManualMarkersUI.md` stays valid for its own scope (alias/position/spawn→army/delete); its
deferred layerIndex/symmetry items are superseded by this document.

## 0. Premise correction
v2 today has **no layer tier at all** on the procedural side — `MapRecipe::markerRules` is a flat
vector, `MarkerRule` already carries its own `bSymmetryUseGlobal`/`symmetryMask` and is already
live-exported per-rule (`MapExporter_MarkersStack_IO.cpp`). This design isn't restoring v1's tier
— it's introducing a new tier into already-shipped v2 PARAMS/IO and removing 3 fields from an
already-field-complete type. `PropRule`/`DecalRule`/`UnitRule` carry the identical 3-field group
— not touched this round, but the shape below generalizes to them later without rework.

## 1. Central data-shape decision
**Two arrays, not one unified type — sharing one composed symmetry struct.** Matches the existing
ratified split for Props/Decals (Correction 7's procedural Stacks vs. Correction 14's manual
Groups are explicitly different concepts). Manual markers also have a hard format-mandated
`markers[type][instanceName]` dictionary axis that Layer can't replace — Layer must be a
cross-cutting index-tag there, same as `PropTransform::layerIndex` already is.

```cpp
// Symmetry_PARAMS.h — new
struct SymmetrySetting {
    bool bSymmetryUseGlobal = true;
    int  symmetryMask       = 0;
    int  radialSymmetryRepeatCount = 3;
};

// procedural side
struct MarkerRuleLayer {
    std::string name;
    bool bEnabled = true;
    bool bHidden  = false;
    SymmetrySetting symmetry;
    std::vector<MarkerRule> rules;
};
// MarkerRule loses bSymmetryUseGlobal/symmetryMask/radialSymmetryRepeatCount — moved up a tier

// manual side — extends round 1's Gap 1 proposal
struct MarkerInstanceLayer {
    std::string name;
    float color[4] = {1,1,1,1};
    float iconScale = 1.0f;
    SymmetrySetting symmetry;   // NEW this round
};
```
`MapRecipe::markerRules` → `markerRuleLayers`; new `MapRecipe::markerLayers`. Widget composition
(outer/inner `DraggableList`) already exists — `LayersTab_UI.cpp`/`LayerEditor_UI.cpp` already run
this exact two-tier pattern for `HeightmapStack`. Nothing new to build in the widget layer.

## 2. Symmetry-consumer mechanism for manual markers
Materialize at authoring-time (not bake-time PROC mutation, not export-time expansion, not
preview-only — all three of round 1's options have real flaws once the setting is layer-scoped;
see full reasoning in the consult transcript). Mechanism:

- Adding a marker resolves the target layer's effective mask, calls the existing
  `BuildSymmetryOrbit(...)` once, inserts one real `MarkerTransform` per orbit point into
  `MarkerInstanceGroup.transforms`. Point 0 = **anchor** (`bSymmetryAnchor = true`), rest =
  **derived siblings**, all sharing a new `symmetryGroupId`.
- Dragging the **anchor** re-runs `BuildSymmetryOrbit` and writes results onto existing siblings
  by stable `symmetryOrbitIndex` — never touches their name/alias/army-assignment.
- **Siblings are not independently draggable/editable-by-position** — tab sliders disabled,
  canvas drag refused. (v1 let any sibling be dragged and recomputed the rest via a single-axis
  `if/else if` chain that silently mishandled composed masks — confirmed live bug, not repeated.)
- A layer's symmetry-setting change resizes every governed anchor's sibling set to match the new
  orbit point count (v1 never did this — flagged as an open sub-decision on exact diff-vs-recreate
  behavior).
- **"Break Symmetry Link"** (new, v1 never had it) — sets `symmetryGroupId = 0` on every member,
  detaching them at their current positions.
- Deleting an anchor cascades to its siblings (generalizes v1's Spawn-only cascade to every type).

⚠️ **Spawn/Army wrinkle, unresolved**: v1 auto-created a new `Army` per symmetric spawn sibling.
Whether this design does the same or leaves siblings' army-assignment manual is a real open call
— routed to Format/Generator Expert, not decided here.

No IO-layer change needed for correctness beyond §4's new fields — siblings are real
`MarkerTransform` entries the moment they're created, so export/import already round-trips them.

## 3. Tab + canvas interaction
**Tab**: Procedural Rule Layers (outer `DraggableList<MarkerRuleLayer>`, symmetry axes drawn once
per layer instead of per rule; inner `DraggableList<MarkerRule>` unchanged otherwise) → Manual
Marker Layers (new, mirrors `PropsTab_Manual_UI` exactly, plus the same symmetry-axes control) →
Manual Markers roster (STEP49's shape, plus a now-load-bearing Layer picker, disabled position
sliders on siblings, "Break Symmetry Link") → Placed Markers (existing read-only list, unchanged).

**Canvas**: left-click selects (procedural markers select/highlight only, never drag — DATA has
one writing stage; manual anchor/ungrouped markers drag, siblings refused). Right-click a marker →
Delete (cascades)/Break Link/Assign to Layer. Right-click empty canvas with a manual layer selected
→ "Add Marker Here" (materializes anchor+siblings per §2). Hit-test via the existing O(1)
EntityIDBuffer/spatial-grid, not a per-marker screen-rect scan.

## 4. Flagged for ARCH (and Format/Generator/IO Architecture Experts) — nothing here is ratified
1. New PARAMS types: `SymmetrySetting`, `MarkerRuleLayer` (breaking field removal from
   already-live `MarkerRule`), `MarkerInstanceLayer` (extends Gap 1). Recipe field renames/adds.
2. New `MarkerTransform` fields: `layerIndex` (Gap 1, unchanged), `symmetryGroupId`,
   `bSymmetryAnchor`, `symmetryOrbitIndex` (new). Explicitly no per-instance symmetry mask —
   instances carry only linkage.
3. ⚠️ **Module-boundary question**: `BuildSymmetryOrbit`/`ResolveSymmetryMask` currently live in
   PROC, which UI doesn't legally reach directly. Recommend relocating to MATH (pure functions,
   zero behavior change for existing PROC callers) so both UI and IO can call them legally —
   ARCH's call, relocate vs. grant a narrower exception.
4. `SANMAP_FORMAT_SPEC` Correction 7 amendment — one tier only, not the full deferred Group-of-
   Layers hierarchy: `MarkersStack: [ N x { Name, Enabled, Hidden, SymmetryUseGlobal, SymmetryMask,
   RadialSymmetryRepeatCount, <rules array> } ]`. Exact nested-key name is Format Expert's call.
5. New top-level `MarkerGroups` wire key (parallels Correction 14's `PropGroups`/`DecalGroups`).
6. New merged fields on `markers[type].transforms[name]`: `layerIndex` (low-risk, Correction 14
   precedent) + `symmetryGroupId`/`bSymmetryAnchor`/`symmetryOrbitIndex` (needs explicit
   ratification — new fields on an already-ratified verbatim wire object).
7. Migration needed: `MarkerRule`'s per-rule symmetry fields are live in exported files today;
   moving them up a tier is a real schema change — IO Architecture Expert territory.
8. Naming (`MarkerRuleLayer` vs `MarkerInstanceLayer`) is this consult's recommendation, not a
   ratification.

## Spawn/Army resolution (human decision, post-round-1)
Confirmed against `MAP_SCENARIO_SPEC.md` (ratified, deployed law) and split into two distinct
concepts that must not be conflated:

- **Sanmap Spawns (baseline)** — the `.sanmap`'s own `markers.Spawn.transforms[armyName]`, 1:1
  with the Army roster (`recipe.armies`). This is what spawns if no Scenario exists, and its
  count IS the map's max player count. **This is the only Spawn concept in scope for the marker
  work in this document** — symmetric spawn siblings auto-create a matching Army each (v1's
  behavior, now confirmed format-correct: an unmatched Spawn marker is orphaned, nothing reads it).
- **Scenario Spawns (override)** — the `spawns` table inside a `Scenario` record
  (`MAP_SCENARIO_SPEC.md` §5/§6), a per-lobby-composition override living in the separate
  `<MapName>_Scenarios_Script.lua` file, not the `.sanmap`. **Not buildable yet** — SanGen doesn't
  import/export that file at all today (§8's open IO-Architecture-Expert question, unresolved).
- **Human's requirement for later**: once Scenario Spawns become editable, they must be visually
  distinct on the canvas from baseline Army Spawns (different icon/color) — noted now so the icon/
  category scheme this round designs doesn't need rework when that feature lands. Not designed in
  this round; flagged for whoever eventually scopes the Scenario-file IO + its own marker overlay.

## Who else this touches
- **Generator Expert**: `Placement_Rules_PROC.cpp`'s marker symmetry-resolution call site needs to
  walk the new layer tier instead of reading `rule.*` directly — small, mechanical once ratified.
- **Format Expert**: §4 items 4-6, plus the Spawn/Army sibling question in §2.
- **IO Architecture Expert**: the migration path in §4 item 7.

No coder-dispatchable ticket this round.
