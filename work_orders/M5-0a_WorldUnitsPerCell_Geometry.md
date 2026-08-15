# Work-Order M5-0a — move `worldUnitsPerCell` into `Geometry_PARAMS`

*Constitution §7. Milestone M5 prerequisites. Small cleanup (ARCH follow-up). Sequential
with M5-0c (both touch Placement); can run before it. Executor: SanGen Coder.*

## Root problem
`worldUnitsPerCell` currently has no PARAMS home and is read from
`Proc::PlacementConstants` — but it is fundamental **map geometry** (world units per
heightfield cell), used well beyond placement. Wrong owner.

## Target files
- `src/params/Geometry_PARAMS.h` — add the field.
- `src/proc/…PlacementConstants…` — stop owning it; read from `Geometry`.
- Any other reader — repoint to `Geometry.worldUnitsPerCell`.

## Layer & accuracy
`PARAMS`. No behavior change — a relocation.

## Solution
Add `float worldUnitsPerCell` to `Params::Geometry` (with the current default value).
Remove it from `PlacementConstants`; every reader takes it from the `Geometry` on the
`MapRecipe`. If it is derivable (`worldExtent / mapSize`), keep the explicit field for now
and note the derivation — do not change semantics in this work-order.

## Acceptance
Builds clean; the value read by placement is identical to before (same default); no
behavior change; `PlacementConstants` no longer declares it. Existing tests pass.

## Out of scope
Slope field (M5-0c); any placement math change.
