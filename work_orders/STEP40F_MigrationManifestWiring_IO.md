# Work-Order — Step 40F: wire the V2→V3 manifest, bump `kCurrentSanGenVersion` to 3

*Constitution §7. Executor: SanGen Coder. Depends on `STEP40A` through `STEP40E`, all shipped and
verified. Last of 6 dependency-ordered tickets. IO Architecture Expert consult.*

## Root problem
`Sanmap_MigrationManifest_IO.cpp` still returns an empty vector; `kCurrentSanGenVersion` is still
`2`. The 9 real migration functions built by `STEP40B`-`STEP40E` exist and are individually
tested, but nothing wires them into the manifest, so the runner still zero-iterates for every
document today.

## Solution — shape
**The one manifest entry** (`Sanmap_MigrationManifest_IO.cpp`):
```cpp
MigrationStep{
    /*sourceVersion=*/ 2,
    /*migrations=*/ {
        MigrationEntry{ GeneralMapSettings_Migrate_V2, "GeneralMapSettings_Migrate_V2",
            "Relocates Seed/ScaleFeaturesToMapSize/TerrainMinHeight/WorldUnitsPerCell out of "
            "mapGeneratorData into GeneralMapSettings.", /*bIndependentlySelectable=*/ true },
        MigrationEntry{ Symmetry_Migrate_V2, "Symmetry_Migrate_V2",
            "Relocates the 9 legacy global symmetry scalars out of mapGeneratorData into Symmetry.",
            /*bIndependentlySelectable=*/ true },
        MigrationEntry{ Accumulation_Migrate_V2, "Accumulation_Migrate_V2",
            "Reserves the empty Accumulation top-level key.", /*bIndependentlySelectable=*/ true },
        MigrationEntry{ DetailNormal_Migrate_V2, "DetailNormal_Migrate_V2",
            "Relocates DetailNormalMapSize out of mapGeneratorData into DetailNormal.",
            /*bIndependentlySelectable=*/ true },
        MigrationEntry{ Flow_Migrate_V2, "Flow_Migrate_V2",
            "Converts the legacy FlowMapColor array into the {r,g,b,a} object shape under Flow.",
            /*bIndependentlySelectable=*/ false },
        MigrationEntry{ GlobalMarkerSettings_Migrate_V2, "GlobalMarkerSettings_Migrate_V2",
            "Relocates the 9 legacy global marker fields (icons/colors/scales) into GlobalMarkerSettings.",
            /*bIndependentlySelectable=*/ false },
        MigrationEntry{ SlopeDefaults_Migrate_V2, "SlopeDefaults_Migrate_V2",
            "Synthesizes a global SlopeDefaults from each stratum's legacy slope-gate fields and "
            "sets each stratum's SlopeUseGlobal.", /*bIndependentlySelectable=*/ false },
        MigrationEntry{ StratumGenerationSettings_Migrate_V2, "StratumGenerationSettings_Migrate_V2",
            "Relocates each stratum's legacy slope-gate fields verbatim into StratumGenerationSettings, "
            "index-aligned and padded to 9.", /*bIndependentlySelectable=*/ false },
        MigrationEntry{ EntityCollections_Migrate_V2, "EntityCollections_Migrate_V2",
            "Converts each army's legacy Color array into armyColor, and folds the legacy "
            "mapGeneratorData.Aliases dictionary into markers[].alias.",
            /*bIndependentlySelectable=*/ false },
    },
    /*legacyKeysToDelete=*/ { "mapGeneratorData", "MapGeneratorDataVersion", "mapGeneratorDataVersion" }
}
```
**Ordering within the list is load-bearing**: `SlopeDefaults_Migrate_V2` MUST run before
`StratumGenerationSettings_Migrate_V2` (per `STEP40D`'s additive-write discipline — confirm this
exact order is what the array above shows before shipping). All 9 must run before the shared
`legacyKeysToDelete` fires (already guaranteed — deletion is the runner's own step-level behavior,
after every migration in the step's list).

**Bump `kCurrentSanGenVersion` from 2 to 3** in the same file. Update the manifest header's stale
docstring (already partially rewritten by `STEP40A`; finish describing the real, now-populated
state).

## Target files
- `src/io/Sanmap_MigrationManifest_IO.cpp` — populate the one `MigrationStep`, bump the constant.
- `src/io/Sanmap_MigrationManifest_IO.h` — `#include` the 9 new migration headers.
- `src/io/MapImporter_Validation_IO_Test.cpp` — **known regression risk, flagged by the design
  consult**: this file hand-builds several fixtures with the LITERAL `"SanGenVersion": 2` (not the
  symbolic `Io::kCurrentSanGenVersion`, unlike most call sites in `MapImporter_IO_Test.cpp`). Once
  the bump lands, these fixtures stop being "already current" and genuinely walk the new 9-step
  migration for the first time — in particular `CheckHostileValuesFallBackToDefaults` sends a
  hostile `mapGeneratorData` blob through the new step, which now deletes `mapGeneratorData` via
  `legacyKeysToDelete` BEFORE the legacy-gated `ReadGeometryJson`/`ReadStrataSettingsJson` readers
  ever see it (the runner runs before any block reader). The test's assertions may still pass, but
  for the WRONG reason (vacuous defaults from a now-deleted source, not the guard logic the test's
  own comment claims to exercise) — this needs a real look, not a silent pass. Fix or rewrite as
  needed so the test still exercises what it claims to.

## Explicit out-of-scope
- Any change to the 9 migration functions' own logic — they're already shipped and tested
  (`STEP40B`-`STEP40E`); this ticket only wires them in.
- The `Sanmap_MigrationManifest_IO.h` `MigrationEntry` struct itself — `STEP40A`, already shipped.

## Layer & accuracy class
IO only. Accuracy class: Exact.

## Acceptance test
1. A hand-built, hostile-but-plausible V2-shaped document (real `SanGenVersion: 2`, legacy
   `mapGeneratorData` populated across all 9 migrations' source fields) imports and produces the
   exact expected V3-shaped `Params::MapRecipe` — a genuine end-to-end walk, not per-migration
   isolation (those are already covered by `STEP40B`-`E`'s own tests).
2. That same document, after import, re-exports with `SanGenVersion: 3` and no `mapGeneratorData`
   key (matching `STEP36`'s already-shipped never-write-the-blob behavior).
3. A document already at `SanGenVersion: 3` is an unaffected passthrough — zero migration steps
   run, confirmed via a call-count or side-effect check, not just "still imports."
4. `MapImporter_Validation_IO_Test.cpp`'s hostile-value fixtures are confirmed to still exercise
   what they claim to (fixed/rewritten if the design consult's flagged regression is real).
5. Full `SanGenV2` build stays clean; every existing test continues to pass — this is the
   capstone ticket, run the FULL suite, not a subset.
