# Work-Order — Step 49: Manual Markers editor (alias / position / spawn→army / delete)

*Constitution §7. Executor: SanGen Coder. From the SanGen UI Expert's consult on
`BRIEF_MarkersTabUI.md`. No PARAMS gap — buildable now. Two related gaps (marker layers,
per-marker symmetry) are deferred — see `work_orders/GAP_MarkerLayerAndSymmetry_PARAMS.md`.*

## Root problem
`recipe.markers` (`Params::MarkerInstanceGroup`/`MarkerTransform`) is real, ratified PARAMS
content — but nothing lets a designer edit it. `MarkersTab_Placed_UI.h`'s read-only list only
shows the Placement stage's *resolved* output, not the hand-authored roster. Two scope-note
comments claiming "no PARAMS home exists" (`MarkersTab_Placed_UI.h`, `MarkersTab_UI.h` item 2)
are now **stale** — `ENTITY_AUTHORING_PARAMS_SPEC`'s third session already ratified this type.
Retire both comments as part of this ticket (factual correction, not a design call).

## Target files
- `src/ui/MarkersTab_Manual_UI.h`/`.cpp` (new) — mirrors `AreasTab_UI.h`/`.cpp`'s shape (a small,
  name-keyed, manually-authored vector with select→edit), not `PropsTab_Manual_UI.h` (whose list
  is read-only preview of a different buffer).
- `src/ui/MarkersTab_UI.cpp` — wire `DrawManualMarkers(recipe.markers, recipe.armies,
  state.manual)` into `DrawMarkersTab`, alongside the existing globals/rule-stack/placed-list
  calls. Add the new state member to `MarkersTabState`.
- `src/ui/MarkersTab_Placed_UI.h`, `src/ui/MarkersTab_UI.h` — delete the two stale "no PARAMS
  home" scope-note comments.

## Layer & accuracy class
UI. Accuracy class: Visual.

## Backend policy
N/A (no compute).

## ARCH rules invoked
- Constitution §1 — UI sets PARAMS, never runs generation math (relevant to why per-marker
  symmetry is explicitly NOT built here — see out-of-scope).
- `SANMAP_FORMAT_SPEC.md` — `Spawn`/`Alloys` fixed group-name constants; `Spawn` group's inner
  dictionary keyed by army name (confirmed live in-game, `session_findings_2026-08-17_unit_spawning.md`).
- ARCH_12_ManualPropDecalLayers.md §12 pattern (Props/Decals manual-layer precedent) — reused for widget shape only; this
  ticket adds no new PARAMS type itself.

## Solution — shape
**Group stack** (`DraggableList<Params::MarkerInstanceGroup>`, same shape as `DrawAreaList`):
- Row label = `group.name` (fallback `"Marker Type"`).
- "Add Marker Type": `Combo_UI` seeded from the existing `markerCategoryLabels[kMarkerCategoryCount]`
  (`MarkersTab_Rules_UI.h:22` — reuse, don't duplicate). Seed `bResource = true` when the pick is
  `Alloys`, else `false` (editable after).
- Selected group fields: `name` (TextInput, same `nameRules` as `AreasTab_UI.cpp:96-98`),
  `bResource` (Checkbox).
- `MakeNamesUnique(recipe.markers)` after add/rename — **not cosmetic**: `.sanmap` `markers` is a
  dictionary keyed by this field, unlike Props/Decals' cosmetic layer names.
- Delete: standard `DraggableListSignalKind::Delete`, no protected-row guard — a missing `Spawn`
  group degrades to "that army gets no commander" (soft engine behavior, not a hard requirement).

**Instance list** within the selected group (`DraggableList<Params::MarkerTransform>` — hand-
placed counts are tens, not tens of thousands):
- Row label = `alias` if non-empty else `name`. Add/Remove buttons (`DrawRuleListButtons` pattern).

**Selected instance editor:**
- **Alias** — `DrawTextInput("Alias", transform.alias, ...)`. Free text, no uniqueness requirement.
- **Name** — if the parent group's `name == "Spawn"`: `Combo_UI` over `recipe.armies[i].name`
  (or `.alias`), committing the pick into `transform.name`. Otherwise a plain `TextInput`, same
  `nameRules`, `MakeNamesUnique` scoped **per-group** (`group.transforms`), not global — the wire
  format's inner dictionary key only needs uniqueness within its own outer group.
- **Position** — three `DrawSliderScalar` calls over `transform.transform.positionX/Y/Z`. X/Z:
  reuse the `AreaOriginSliderRange(mapSize)`-shaped continuous-bounds reasoning (one map-width
  slack each side), not integer-stepped. Y: no existing world-elevation-range constant exists
  anywhere in `src/params/` — pick a sensible placeholder range as a named Constitution §8
  setting, not a literal. No terrain-height snapping (out of scope, no stage height-samples
  manual markers today).
- **Delete** — erase from `group.transforms`.
- **No rotation/scale, no layerIndex control** — round-trips whatever import set, or defaults;
  not in v1's named feature list, not invented here.

**Dirty-flag posture:** `recipe.markers` feeds no PROC stage today (confirmed — no
`MarkerInstanceGroup` reference anywhere under `src/proc/`). This block does **not** call
`NotifyPlacementChange`/touch `PreviewDriver` — same silent posture as `PropsTab_Manual_UI.h`
SCOPE NOTE 1. Once the (separate, later) marker-preview-layering work ships, these edits become
live for free, by that design's own "manual sub-layers read `MapRecipe` directly" ruling — zero
change needed here when that lands.

## Explicit out-of-scope
- **`layerIndex` / manual marker layers** — blocked on `Params::MarkerInstanceLayer` not existing
  yet (Gap 1, `GAP_MarkerLayerAndSymmetry_PARAMS.md`). No layer `Combo_UI` in this ticket.
- **Per-marker symmetry** — blocked on both a new field AND an undecided consumer (Gap 2, same
  gap doc) — adding the field alone would silently do nothing, so it's not added here either.
- **Rotation/scale editing** on manual markers.
- **Terrain-height snapping / any height-sampling** of manual marker positions.
- **Export-time validation** (e.g. warn on missing `Spawn` group) — flagged as a possible future
  ticket, same class as the existing `blueprintPath` warn-dialog, not built here.

## Acceptance test
Add a marker group via the tab, confirm it appears with a unique name (colliding names get
repaired). Add an instance, edit alias/position, confirm round-trip through export/import
unchanged. Add a `Spawn` group, confirm the name field becomes an army-picker `Combo_UI` and
writing it sets `transform.name` to the chosen army's name. Delete an instance and a group,
confirm `recipe.markers` shrinks correctly with no dangling state. Confirm `NotifyPlacementChange`/
`PreviewDriver` are never touched by this tab (grep the new files). Full `SanGenV2` build stays
clean; every existing test continues to pass.
