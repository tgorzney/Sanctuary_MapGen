# Work-Order M1-5 — `MapRecipe_PARAMS` (the full settings aggregate)

*Constitution §7. Milestone M1 (Data model). Status: implemented + verified (ALL PASS).*

## Root problem
Complete the PARAMS split: one aggregate holding all the editable settings for a map —
the thing `mapGeneratorData` serializes and the deterministic mode transmits.

## Target files
- `src/params/MapRecipe_PARAMS.h` (+ test).

## Layer & accuracy
`PARAMS` (settings / the recipe).

## Solution / key decision
`MapRecipe` aggregates `geometry`, `layerStack`, marker/prop/decal rules, `water`, and
the global symmetry mask. **Excludes dispatch/backend** — those are execution concerns,
not reproducible-recipe content (backend choice must not change the map). `IsValid()`
delegates to geometry.

## Acceptance — PASSED
Composition: set geometry/seed, add a GeoLayer with 2 layers + 3 marker rules + 1 prop
rule; `GetFlatLayers()` works through the recipe; validity flips with geometry. **ALL
PASS under ASan+UBSan**; 30-line header.

## Out of scope
`Atmosphere_PARAMS` (deferred). The `.sanmap` / `mapGeneratorData` round-trip through IO
— the next work-order (M1-6), the big one.
