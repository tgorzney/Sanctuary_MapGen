# Work-Order M1-4 — marker/prop/decal rule + water `PARAMS`

*Constitution §7. Milestone M1 (Data model). Status: implemented + verified (ALL PASS).*

## Root problem
The recipe's placement + water settings, split out of the god object (settings only;
scatter math stays in PROC — PLACEMENT_SCATTER_SPEC).

## Target files
- `src/params/MarkerRule_PARAMS.h` — marker rule + `MarkerPriority` / `FocusGradient`.
- `src/params/ScatterRule_PARAMS.h` — `PropRule` + `DecalRule`.
- `src/params/Water_PARAMS.h` — water level + deep-water depth.
- `src/params/PlacementRules_PARAMS_Test.cpp`.

## Layer & accuracy
`PARAMS` (settings). No behavior.

## Solution
Faithful gate/spacing/quantity/focus/symmetry fields on `MarkerRule`; the shared
slope/height-gate + density shape on `PropRule`/`DecalRule`; water surface + depth
thresholds. Placement enums kept local to the marker file (placement-scoped, not
cross-cutting).

## Acceptance — PASSED
Defaults for marker (maxSlope 89.9, count 4, LargestArea, None focus, global symmetry),
prop, decal, water. **ALL PASS**; files 46/27/16 lines.

## Out of scope / deferred
- `Atmosphere_PARAMS` — deferred until the real `GenParams_Atmosphere` field set is read
  (avoid inventing fields).
- The top-level `MapRecipe` aggregate (geometry + stack + rules + water + atmosphere +
  dispatch) and the `.sanmap` round-trip — next M1 work-orders.
