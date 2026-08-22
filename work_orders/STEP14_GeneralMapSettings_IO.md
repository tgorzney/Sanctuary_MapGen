# Work-Order — Step 14: `GeneralMapSettings` — schema-v3 Correction 2

*Renamed from "Step 13" — that number was already claimed by
`work_orders/STEP13_PlacementStacks_IO.md` (MarkersStack/PropsStack/DecalsStack/UnitsStack +
GlobalMarkerSettings), shipped by a separate parallel session, discovered mid-review here.*

*Constitution §7. Executor: SanGen Coder. Implements `SANMAP_FORMAT_SPEC.md` Correction 2
verbatim. First slice of the broader schema-v3 cutover (SPEC-4) touched directly — deliberately
narrow: relocates exactly the 4 named fields plus one genuinely new field, nothing else.*

## Root problem
`Seed`, `ScaleFeaturesToMapSize`, `TerrainMinHeight`, `WorldUnitsPerCell` currently live nested
inside the doomed `mapGeneratorData` blob (`BuildMapGeneratorDataJson`, `MapExporter_Recipe_IO.cpp`
lines 57,60-61 — confirmed by direct read). Correction 2 relocates these four to a new top-level
`GeneralMapSettings` section. `GlobalGravity` has no PARAMS field at all today — it's tab-local UI
state (`HeightmapTab_UI.h:70`, explicitly marked "not serialized" in that file's own SCOPE NOTE 2)
that bulk-writes every stratum's `ErosionLayerSettings::gravity` on demand; it is NOT a rival
second store for that per-stratum value (the per-layer slider edits the same field), it's simply
never persisted, so it resets to `4.0f` every session.

## Target files
New: `src/io/MapExporter_GeneralMapSettings_IO.cpp` / `MapImporter_GeneralMapSettings_IO.cpp`.

Modified:
- `src/params/Geometry_PARAMS.h` — **no new field here.** `GlobalGravity` is NOT geometry (per
  `HeightmapTab_UI.h`'s own SCOPE NOTE 2: gravity is per-stratum, this is a UI-convenience mirror
  of the last bulk-set value, not a geometry concept) — it gets its own small home (see below).
- **Where `GlobalGravity` lives**: add a new, minimal `Params::GeneralMapSettings` struct
  (`src/params/GeneralMapSettings_PARAMS.h`) holding exactly one field:
  `float globalGravity = 4.0f;` (default matches `HeightmapTabState::globalGravity`'s current
  hardcoded default, `HeightmapTab_UI.h:70`). This is a genuinely new, minimal aggregator — not a
  rival store for per-stratum gravity, purely a persisted mirror of the UI's last bulk-set value
  (same framing `SANMAP_FORMAT_SPEC.md` Correction 2 itself uses: "a genuine new-field addition...
  not a relocation").
- `src/params/MapRecipe_PARAMS.h` — add `GeneralMapSettings generalMapSettings;` as a flat sibling
  of `water`/`atmosphere`/`slopeDefaults`.
- `src/io/MapExporter_Recipe_IO.cpp` — `BuildMapGeneratorDataJson`: **remove** the `Seed`,
  `ScaleFeaturesToMapSize`, `TerrainMinHeight`, `WorldUnitsPerCell` lines (relocation, not
  duplication — same discipline as Steps 10/12). **Leave `MapSize`, `TerrainMaxHeight`,
  `GlobalSymmetryMask`, `SimulationGrouping`, `GeoLayers`, `Stratums`, `Water`, `PlacementRules`
  exactly as they are** — none of those are part of Correction 2; they belong to other,
  not-yet-implemented corrections (3, 4) or stay in the legacy blob for now.
- `src/io/MapImporter_Recipe_IO.cpp` — `ReadGeometryJson`: **remove** the matching 4 reads (it
  currently reads `MapSize`/`Seed`/`TerrainMinHeight`/`TerrainMaxHeight`/`ScaleFeaturesToMapSize`/
  `WorldUnitsPerCell`/`GlobalSymmetryMask` from `generatorData` — remove only the 4 relocated ones;
  `MapSize`, `TerrainMaxHeight`, `GlobalSymmetryMask` stay exactly as they are, read from
  `generatorData` exactly as today). **ARCH Expert findings, both load-bearing:**
  1. **Leave the clamp/`Warn` block at the end of `ReadGeometryJson` (lines ~31-44) completely
     untouched, do not delete it alongside the 4 relocated fields.** It's a cross-field invariant
     check (`TerrainMinHeight` clamped against `TerrainMaxHeight`; `WorldUnitsPerCell` positivity
     clamp) that stays correct post-relocation ONLY because the new top-level `GeneralMapSettings`
     reader runs unconditionally BEFORE `ReadGeometryJson` (same ordering every top-level-key
     ticket this session established) — so by the time this clamp block runs, `terrainMinHeight`/
     `worldUnitsPerCell` are already set from the new reader, and `TerrainMaxHeight` is set in the
     same function just above. Deleting this block because it's "field-adjacent" to the 4 relocated
     reads would silently reopen a `Geometry::IsValid()` violation path on import.
  2. **`Seed`'s current read is not a bare copy — replicate its negative-value guard verbatim** in
     the new `ReadGeneralMapSettingsJson`: `int seed = ...; if (ReadJsonInteger(...)) geometry.seed
     = seed > 0 ? static_cast<unsigned int>(seed) : 0u;` (see the existing code in `ReadGeometryJson`
     for the exact pattern) — do not naively read `Seed` as a plain unsigned integer.
- `src/io/MapExporter_Recipe_IO.h`/`MapImporter_Recipe_IO.h`, `MapImporter_IO.cpp` — wire the new
  builder/reader, top-level, unconditional, before the `mapGeneratorData` gate on import — same
  tier as every other top-level-key ticket this session.

## Layer & accuracy class
PARAMS + IO/BRIDGE. Accuracy class: Exact.

## Backend policy
CPU only. No PROC/PIPELINE change — `globalGravity` remains a UI-convenience bulk-write trigger,
not a new generation input; this ticket does not touch `ApplyGlobalGravityToErosion` or any
stratum's actual per-layer `gravity` field.

## ARCH rules invoked
- `SANMAP_FORMAT_SPEC.md` Correction 2 — binding, implement verbatim.
- ARCH_01_06_SanmapKeyCasing.md §1.6 (PascalCase top-level SanGen-owned key) — `GeneralMapSettings`.
- Constitution §8 — `globalGravity` becoming a real, persisted setting (rather than resetting
  every session) is exactly the kind of gap this principle exists to close.

## Solution — shape
```
GeneralMapSettings: {
    Seed                   (int)    <- geometry.seed
    ScaleFeaturesToMapSize (bool)   <- geometry.bScaleFeaturesToMapSize
    GlobalGravity          (float)  <- generalMapSettings.globalGravity   // NEW
    TerrainMinHeight       (float)  <- geometry.terrainMinHeight
    WorldUnitsPerCell      (float)  <- geometry.worldUnitsPerCell
}
```
PascalCase, `b`-prefix dropped, matching every other SanGen-owned top-level section's established
internal-field-casing convention (same as `StratumGenerationSettings`, `SlopeDefaults`).

## Explicit out-of-scope
- **`MapSize`/`TerrainMaxHeight`/`GlobalSymmetryMask`/`SimulationGrouping`/`GeoLayers`/`Stratums`/
  `Water`/`PlacementRules`** — untouched, still live in `mapGeneratorData` exactly as today; each
  belongs to a separate, not-yet-implemented correction or is genuinely unrelated to Correction 2.
- **Any change to `ApplyGlobalGravityToErosion` or per-stratum `gravity`** — this ticket only gives
  the UI's bulk-set value a durable home; it does not change what drives generation.
- **UI wiring** (`HeightmapTab_UI.h`'s `globalGravity` field reading from/writing to
  `recipe.generalMapSettings.globalGravity` instead of its own hardcoded default) — separate,
  already-tracked follow-up (session task "UI wiring for shipped PARAMS types"), same exclusion
  every PARAMS ticket this session has had.
- **The rest of schema-v3** (Corrections 3/4/6/7/8/9) — separate tickets.

## Acceptance test
A `Params::MapRecipe` with non-default `geometry.seed`/`bScaleFeaturesToMapSize`/
`terrainMinHeight`/`worldUnitsPerCell` and non-default `generalMapSettings.globalGravity` survives
export→import exactly through `GeneralMapSettings`. Confirm (via raw JSON text inspection in the
test, not just C++ values) that the 4 relocated keys no longer appear under
`document["mapGeneratorData"]` after this ticket — relocation, not duplication. Confirm `MapSize`/
`TerrainMaxHeight`/`GlobalSymmetryMask` still round-trip through the legacy blob exactly as before
(regression guard). Test a negative seed value round-trips correctly (the guard clamps to 0, not
a raw negative-to-unsigned wraparound). Test that `TerrainMinHeight`/`TerrainMaxHeight`'s band
invariant is still enforced correctly post-relocation (a document with `TerrainMinHeight` above
`TerrainMaxHeight` gets clamped one unit below it, exactly as today). Full `SanGenV2` build stays
clean; every existing test continues to pass.
