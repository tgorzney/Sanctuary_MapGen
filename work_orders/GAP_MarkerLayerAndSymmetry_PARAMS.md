# Gap report — for the ARCH Expert (from the `BRIEF_MarkersTabUI.md` UI Expert consult)

Two PARAMS gaps found while designing the manual-markers UI (`STEP49_ManualMarkersUI.md`, buildable
without either). Reporting the gaps precisely, not ratifying them — that's ARCH's call.

## Gap 1 — `Params::MarkerInstanceLayer` + `MarkerTransform::layerIndex` (low-risk, direct parallel)
Exact structural parallel to `PropInstanceLayer`/`DecalInstanceLayer` +
`PropTransform`/`DecalTransform::layerIndex` (`src/params/PropInstance_PARAMS.h`), and shape-
compatible with what `DESIGN_MarkerPreviewLayering_R2.md` already sketched for it (its "Alloy /
Spawns-Armies" row: *"Blocked — no `MarkerInstanceLayer` PARAMS type exists yet... struct already
shaped to split to N once it does"*; ARCH ruling §5 there: `color`/`iconScale` become real
recipe-serialized fields the moment this type exists, same as Props/Decals).

```cpp
struct MarkerInstanceLayer { std::string name; float color[4] = {1.0f,1.0f,1.0f,1.0f}; float iconScale = 1.0f; };

struct MarkerTransform {
    std::string name;
    InstancedTransform transform;
    std::string alias;
    int layerIndex = 0;              // NEW — indexes recipe.markerLayers
};
```
`MapRecipe` gains `std::vector<MarkerInstanceLayer> markerLayers;`, alongside `propLayers`/
`decalLayers`. Out-of-range `layerIndex` uses the same clamp-to-0 convention already ratified in
ARCH §12 — no new import-repair ruling needed.

**Asymmetry vs. Props/Decals, worth naming so it isn't silently inherited**: `PropInstanceLayer`/
`DecalInstanceLayer` names are cosmetic-only. `MarkerInstanceGroup::name`/`MarkerTransform::name`
are **not** — they're real `.sanmap` dictionary keys (`markers[type].transforms[instanceName]`)
and already require uniqueness independent of this gap.

## Gap 2 — per-marker symmetry (needs a field *and* a design decision on its consumer)
v1's "per-marker symmetry" most plausibly meant: toggling a mirror axis on one manual marker also
places its symmetric counterpart(s). `recipe.markers` has **zero PROC consumer today** (confirmed
— no `MarkerInstanceGroup` reference anywhere under `src/proc/`); manual entities are pure
round-trip pass-through, never baked. A mask field alone would silently do nothing.

Proposed field (parallels `MarkerRule`'s existing pair exactly, `src/params/MarkerRule_PARAMS.h:58-64`):
```cpp
struct MarkerTransform {
    ...
    bool bSymmetryUseGlobal = true;   // NEW
    int  symmetryMask       = 0;      // NEW — Params::SymmetryAxis bits
};
```
**Open question this ticket does not resolve — the field's consumer**, three options:
1. A bake-time PROC pass that mutates `recipe.markers` in place before export — no existing PROC
   stage mutates PARAMS today, this would be a new pattern.
2. An export-time IO expansion — `recipe.markers` stays the single source of truth, orbit expands
   only inside `MapExporter`.
3. Live-preview-only decoration, no persisted expansion — cheapest, but a symmetric hand-placed
   marker then never actually ships in the exported map.

Recommend routing to: **ARCH Expert** (module-boundary question — does PARAMS ever get
PROC-mutated in place, y/n) and **Generator Expert** (if option 1 is the answer, since
`Placement_Symmetry_PROC.h`'s `BuildSymmetryOrbit` is the existing analog to reuse). The field's
*presence* is uncontroversial; its *consumer* is not — don't ratify the field without also
answering this.

## Deferred UI work, unblocked once these ratify
- **Phase 2 — Manual Marker Layers** (Gap 1): `MarkersTab_ManualLayers_UI.h/.cpp`, exact
  structural mirror of `PropsTab_Manual_UI.h/.cpp` — `DraggableList<MarkerInstanceLayer>` over
  `recipe.markerLayers`, clamp/renumber repair on delete/reorder, same "Use Group Color" toggle,
  same silent no-notify posture. Adds a Layer `Combo_UI` to `STEP49`'s per-instance editor.
- **Phase 3 — per-marker symmetry controls** (Gap 2): wire the existing
  `DrawPlacementSymmetryAxes(...)` widget (already reused by `MarkerRule`/`PropRule`/`UnitRule`)
  into `STEP49`'s per-instance editor — zero new widget work, just needs the data to exist and
  mean something.
