# Work-Order — Step 40C: `Flow`/`GlobalMarkerSettings` V2→V3 migrations (color conversion)

*Constitution §7. Executor: SanGen Coder. Depends on `STEP40A` (needs `ConvertColorArrayToRgbaObject`
to exist). IO Architecture Expert consult.*

## Root problem
`mapGeneratorData.FlowMapColor` and `mapGeneratorData.{MarkerColorAlloy,MarkerColorPlasma,
MarkerColorSpawn}` are legacy 4-element `[r,g,b,a]` arrays. Every current V3 color field uses the
`{r,g,b,a}` object shape — a plain `MoveKey` would relocate the array unchanged, which the current
readers (expecting an object) would then silently fail to parse. Both `Flow.FlowMapColor` and
`GlobalMarkerSettings`'s 3 color fields need `STEP40A`'s new `ConvertColorArrayToRgbaObject`
primitive, not `MoveKey` alone.

## Solution — shape
**`Flow_Migrate_V2`**: `MoveKey(mapGeneratorData, "FlowMapColor", document["Flow"], "FlowMapColor")`
then `ConvertColorArrayToRgbaObject(document["Flow"], "FlowMapColor")` (move first, convert at the
destination — or convert-then-move, either order is safe since both primitives are total/no-op-safe;
pick whichever reads more naturally and document why).

**`GlobalMarkerSettings_Migrate_V2`**: relocates 9 fields total —
`GlobalIconAlloy`/`GlobalIconPlasma`/`GlobalIconSpawn` (strings, plain `MoveKey`),
`MarkerColorAlloy`/`MarkerColorPlasma`/`MarkerColorSpawn` (arrays, `MoveKey` +
`ConvertColorArrayToRgbaObject` each), `MarkerScaleAlloy`/`MarkerScalePlasma`/`MarkerScaleSpawn`
(scalars, plain `MoveKey`) — all into `document["GlobalMarkerSettings"]`.

Both `bIndependentlySelectable = true` once this ticket's own paired tests prove the primitive
composition is correct — no cross-migration ordering dependency with anything else in the step.

## Target files
- New `src/io/Flow_Migrate_V2_IO.h/.cpp` + `_Test.cpp`.
- New `src/io/GlobalMarkerSettings_Migrate_V2_IO.h/.cpp` + `_Test.cpp`.

## Explicit out-of-scope
- Wiring into the manifest — `STEP40F`.
- Any other migration — separate tickets.

## Layer & accuracy class
IO only. Accuracy class: Exact.

## Acceptance test
1. `Flow_Migrate_V2`, given a hand-built fixture with `mapGeneratorData.FlowMapColor = [0.2, 0.4,
   0.6, 1.0]`, produces `document["Flow"]["FlowMapColor"] = {"r":0.2,"g":0.4,"b":0.6,"a":1.0}`.
2. `GlobalMarkerSettings_Migrate_V2`, given a fixture with all 9 legacy fields populated (colors as
   4-element arrays), produces the correct `GlobalMarkerSettings` object with all 9 fields, colors
   correctly converted to objects.
3. Both migrations are safe no-ops on a document missing the relevant legacy fields.
4. A short (fewer than 4 elements) color array pads correctly per `ConvertColorArrayToRgbaObject`'s
   documented default-fill behavior — at least one test case exercises this.
5. Full `SanGenV2` build stays clean; every existing test continues to pass.
