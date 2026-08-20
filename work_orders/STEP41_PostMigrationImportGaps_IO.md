# Work-Order — Step 41: fix 2 import gaps `STEP40F` found but correctly left out of scope

*Constitution §6. Executor: SanGen Coder. Two independent, unrelated fixes found while
investigating test regressions during `STEP40F`'s capstone verification.*

## Item 1 — geometry validation guards never run for new-format files

### Root problem
`ReadGeometryJson` (`MapImporter_Recipe_IO.cpp:11-48`) is called ONLY from inside the
`mapGeneratorData`-gated block (`MapImporter_ParseDocument_IO.cpp`). It contains 3 real validation
guards that have nothing to do with the legacy blob itself — they clamp `geometry.terrainMaxHeight`/
`terrainMinHeight`/`worldUnitsPerCell` into a sane band, and the function's own comment already
notes they're "correct post-relocation ONLY because `ReadGeneralMapSettingsJson` already ran" (i.e.
they were already written to depend on the NEW unconditional reader, not the legacy one, for their
actual input values — only their *execution* stayed gated). Since `STEP36` stopped writing the
legacy blob, these guards now NEVER run for any current-format file — a hand-edited or corrupted
`GeneralMapSettings.TerrainMaxHeight`/`TerrainMinHeight`/`WorldUnitsPerCell` value sails through
unclamped on every normal import today.

### Solution — shape
Extract the 3 guards (current lines 36-47) into a new, small, unconditionally-called function —
e.g. `ClampGeometryBand(Params::Geometry& geometry, MapImportResult& result)` — and call it from
`MapImporter_ParseDocument_IO.cpp`'s `ParseSimulationDomainsJson`, immediately after
`GeneralMapSettings` is read (same ordering dependency the original comment already documented,
just moved to where it can actually always fire). `ReadGeometryJson` keeps only the legacy
`MapSize`/`TerrainMaxHeight` re-reads that still make sense inside the gated block (or drop them
too if a quick check shows they're now fully redundant with `width`/`GeneralMapSettings.
TerrainMaxHeight` — verify, don't assume, before removing).

### Target files
- `src/io/MapImporter_Recipe_IO.cpp` — extract the 3 guards out of `ReadGeometryJson`.
- `src/io/MapImporter_Recipe_IO.h` — declare the new function.
- `src/io/MapImporter_ParseDocument_IO.cpp` — call it unconditionally in `ParseSimulationDomainsJson`.
- Tests: a document with an out-of-band `TerrainMaxHeight`/`TerrainMinHeight`/`WorldUnitsPerCell`
  in `GeneralMapSettings` and NO `mapGeneratorData` block must still get clamped correctly.

## Item 2 — `StratumGenerationSettings_Migrate_V2` pads even with zero source data

### Root problem
`StratumGenerationSettings_Migrate_V2_IO.cpp` unconditionally pads `document
["StratumGenerationSettings"]` to exactly 9 entries, even when `mapGeneratorData.Stratums` is
completely absent or empty — unlike its sibling `SlopeDefaults_Migrate_V2`, which correctly
short-circuits with no write at all when `Stratums` is empty (`SlopeDefaults_Migrate_V2_IO.cpp:
123-127`). This means ANY document that walks the V2→V3 migration step — even one with no stratum
data whatsoever — ends up with a spurious 9-entry `StratumGenerationSettings` array, which then
trips `ReadStratumGenerationSettingsJson`'s cardinality check against `stratumLayers`'s real length.

### Solution — shape
Mirror the sibling's exact short-circuit: if `mapGeneratorData` is absent/not an object, OR
`Stratums` is absent/not an array/empty, return immediately with NO write to
`document["StratumGenerationSettings"]` at all — not even the padding array.

### Target files
- `src/io/StratumGenerationSettings_Migrate_V2_IO.cpp` — add the early-return short-circuit,
  matching `SlopeDefaults_Migrate_V2_IO.cpp`'s exact pattern.
- `src/io/StratumGenerationSettings_Migrate_V2_IO_Test.cpp` — add a test asserting no
  `StratumGenerationSettings` key at all is produced for an N=0 input; this also means
  `MapImporter_Validation_IO_Test.cpp`'s `CheckAPartialDocumentRecoversWhatItHas` fixture (patched
  by `STEP40F` to expect `warningCount == 1` because of THIS exact bug) should revert to its
  original, correct expectation once this is fixed — check and update that assertion back.

## Explicit out-of-scope
- Any other migration or reader logic — both items are narrowly scoped to exactly what `STEP40F`
  found and flagged.

## Layer & accuracy class
IO only. Accuracy class: Exact.

## Acceptance test
1. A document with no `mapGeneratorData` block and an out-of-band `GeneralMapSettings.
   TerrainMaxHeight` (e.g. 0 or negative) imports with the value correctly clamped to 1, matching
   the exact behavior the old gated guard used to provide for legacy-blob documents.
2. Same for `TerrainMinHeight` sitting above the ceiling, and `WorldUnitsPerCell` being
   non-positive — both clamp correctly on a no-legacy-blob document.
3. A minimal V2-shaped document with `SanGenVersion: 2` and NO `Stratums` data at all migrates
   with zero `StratumGenerationSettings` key produced, and consequently zero spurious cardinality
   warning from `ReadStratumGenerationSettingsJson`.
4. `MapImporter_Validation_IO_Test.cpp`'s `CheckAPartialDocumentRecoversWhatItHas` reverts to its
   pre-`STEP40F` warning-count expectation (confirm what that was) once item 3 is fixed.
5. Full `SanGenV2` build stays clean; every existing test continues to pass.
