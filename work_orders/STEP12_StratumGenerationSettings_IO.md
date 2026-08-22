# Work-Order — Step 12: `StratumGenerationSettings` — soil physics + slope-gate relocation

*Constitution §7. Executor: SanGen Coder. Implements `SANMAP_FORMAT_SPEC.md` Correction 12
verbatim. Now fully unblocked: `bSlopeUseGlobal` (Step 10) and the `stratumLayers` appearance fix
(Step 11) are both already shipped — the two things this correction depends on existing.*

## Root problem
`Params::Stratum::soilPhysics` (6 fields) has been **write-only-to-nothing since the type was
created** — nothing in `src/io/` serializes it at all. The 8 slope-gate fields (including the new
`bSlopeUseGlobal`) currently live ONLY in the doomed legacy `mapGeneratorData.Stratums` blob
(`BuildStratumJson`/`ReadStrataSettingsJson`, `MapExporter_Layers_IO.cpp`/`MapImporter_Recipe_IO.cpp`
— confirmed by direct read). Correction 12 gives soil physics its first real IO surface and
**relocates** (not duplicates) the 8 slope-gate keys to a new top-level `StratumGenerationSettings`
array, index-aligned with `stratumLayers[9]`.

## Target files
- `src/io/MapExporter_StratumGeneration_IO.cpp` (new) — `BuildStratumGenerationSettingsJson`.
- `src/io/MapImporter_StratumGeneration_IO.cpp` (new) — `ReadStratumGenerationSettingsJson`.
- `src/io/MapExporter_Layers_IO.cpp` — `BuildStratumJson`: **remove** the 8 relocated keys
  (`SlopeGateEnabled`, `MinimumSlopeDegrees`, `MaximumSlopeDegrees`, `SlopeFeatherDegreesLow`,
  `SlopeFeatherDegreesHigh`, `UseSmoothstep`, `InvertSlopeGate`, `SlopeGateStrength`) — this is a
  MOVE, per the spec's own text ("only the container these keys live in changes"), not a
  duplication. **Keep** `ImportedMaskMode`, `MaskRemapMinimum`/`Maximum`, `Enabled`, `TintRed/
  Green/Blue`, `TileCount` exactly as they are (unrelated fields, not part of this correction,
  still legitimately duplicated with `stratumLayers` until a future cleanup — not this ticket's
  job to resolve that overlap).
- `src/io/MapImporter_Recipe_IO.cpp` — `ReadStrataSettingsJson`: **remove** the matching 8 reads.
  **Do not** revert Step 11's grow-and-merge fix (`outRecipe.strata.resize`/merge-in-place) — that
  stays exactly as Step 11 left it; this ticket only removes 8 specific field reads from within it.
- `src/io/MapExporter_Recipe_IO.h`/`.cpp`, `MapImporter_Recipe_IO.h`, `MapImporter_IO.cpp` — wire
  the new builder/reader, top-level, unconditional, before the `mapGeneratorData` gate on import —
  same tier as `stratumLayers` (Step 11), `SlopeDefaults` (Step 10), and every other top-level-key
  ticket this session.

## Layer & accuracy class
PARAMS (no new type — see below) + IO/BRIDGE. Accuracy class: Exact.

## Backend policy
CPU only.

## ARCH rules invoked
- `SANMAP_FORMAT_SPEC.md` Correction 12 — binding, implement verbatim.
- `MASKING_SPEC.md` §1.7 — the `bSlopeUseGlobal` mechanism this correction gives an IO home to
  (already shipped, Step 10 — this ticket does not touch the PROC/hash side again).
- ARCH_07_01_ParamsPerStratum.md §7.1 — **no new C++ type.** Every field is already a member of the single `Params::Stratum`
  (`soilPhysics` sub-struct + the 8 slope-gate fields directly on `Stratum`). This ticket is purely
  a new IO surface for fields that already have a PARAMS home — it must not invent a rival
  per-stratum settings type or a rival top-level array.

## Solution — shape
```
StratumGenerationSettings: [ 9 × {
    // Soil physics (Params::Stratum::soilPhysics, 6 fields — NEW writes, nothing serializes these today):
    Hardness            (float)  <- soilPhysics.hardness
    Friction            (float)  <- soilPhysics.friction
    Cohesion            (float)  <- soilPhysics.cohesion
    CapacityMultiplier  (float)  <- soilPhysics.capacityMultiplier
    AbsorptionRate      (float)  <- soilPhysics.absorptionRate
    Erodable            (bool)   <- soilPhysics.bErodable   // "b" dropped, this section's own casing rule

    // Slope-gate (Params::Stratum, 1 NEW + 7 RELOCATED verbatim from the legacy blob):
    SlopeUseGlobal          (bool)   <- bSlopeUseGlobal            // NEW
    SlopeGateEnabled        (bool)   <- bSlopeGateEnabled          // relocated, verbatim key
    MinimumSlopeDegrees     (float)  <- minimumSlopeDegrees        // relocated, verbatim key
    MaximumSlopeDegrees     (float)  <- maximumSlopeDegrees        // relocated, verbatim key
    SlopeFeatherDegreesLow  (float)  <- slopeFeatherDegreesLow     // relocated, verbatim key
    SlopeFeatherDegreesHigh (float)  <- slopeFeatherDegreesHigh    // relocated, verbatim key
    UseSmoothstep           (bool)   <- bUseSmoothstep             // relocated, verbatim key
    InvertSlopeGate         (bool)   <- bInvertSlopeGate           // relocated, verbatim key
    SlopeGateStrength       (float)  <- slopeGateStrength          // relocated, verbatim key
} ]
```
Casing: PascalCase, `b`-prefix dropped — matches this section's own established convention
(`GeneralMapSettings`, the legacy `BuildStratumJson`'s own keys) — this is a SanGen-owned array's
internal field spelling, unconstrained by ARCH_01_06_SanmapKeyCasing.md §1.6 (which governs top-level KEYS and format-native
COLLECTION MEMBERS, not a SanGen section's own internal shape).

**Cardinality:** always write exactly `sanmapStratumCount` entries (the shared constant,
`MapExporter_IO.h` — do not hardcode a literal `9`, same as `BuildStratumLayersJson` already does),
padding past `recipe.strata.size()` with `Params::Stratum()` defaults. On import, compare
`StratumGenerationSettings`'s actual array length against `stratumLayers`'s actual array length
(the two SanGen-owned arrays, to EACH OTHER — not each independently against `sanmapStratumCount`;
Correction 12's own wording is "a length mismatch between `stratumLayers` and
`StratumGenerationSettings`") and log a loud warning on mismatch — never silent truncation, never
a hard refusal. **`ReadStratumGenerationSettingsJson` must be grow-only, merge-in-place, never
clear-and-rebuild** — same pattern `ReadStratumLayersJson` (Step 11) already established, since it
runs at the same top-level tier and must not truncate whatever a sibling reader already wrote onto
`outRecipe.strata`.

## Explicit out-of-scope
- **Any new PARAMS type.** `bSlopeUseGlobal` already exists (Step 10). `StratumSoilPhysics`
  already exists. This ticket adds zero new C++ types.
- **The `ImportedMaskMode`/`Enabled`/`Tint*`/`MaskRemap*`/`TileCount` overlap between the legacy
  `mapGeneratorData.Stratums` blob and `stratumLayers`** — pre-existing, unrelated duplication;
  not this ticket's job to resolve. This ticket only removes the 8 SLOPE-GATE keys from the legacy
  blob (a genuine relocation, not a cleanup of the other overlap).
- **`importedMaskMode`/`bEnabled`'s missing eventual format home** once the legacy blob is deleted
  — still an open follow-up flagged elsewhere (Step 11's out-of-scope list), not resolved here.
- **Deleting the legacy `mapGeneratorData.Stratums` blob itself** — it still carries 5 fields this
  correction doesn't touch (`ImportedMaskMode`, `MaskRemapMin/Max`, `Enabled`, `Tint*`, `TileCount`);
  the blob stays alive, just smaller.

## Acceptance test
A `Params::Stratum` with non-default `soilPhysics` (all 6 fields) and non-default slope-gate
fields (including `bSlopeUseGlobal = false`, to prove the override path round-trips too, not just
the default path) survives export→import exactly through `StratumGenerationSettings`. Confirm the
8 relocated keys no longer appear anywhere under `document["mapGeneratorData"]["Stratums"]` in an
exported document (grep the JSON text in the test, not just the C++ call sites) — the whole point
of "relocated, not duplicated." Confirm the 5 untouched legacy keys (`ImportedMaskMode`,
`MaskRemapMin/Max`, `Enabled`, `Tint*`, `TileCount`) still round-trip through the legacy blob
exactly as before this ticket (regression guard — Step 11's grow-and-merge fix must still work
correctly once 8 fewer keys flow through it). Cardinality-mismatch warning behavior tested the
same way Step 11's was. Full `SanGenV2` build stays clean; every existing test continues to pass.
