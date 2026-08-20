# Work-Order — Step 40B: the 4 trivial V2→V3 field-relocation migrations

*Constitution §7. Executor: SanGen Coder. Depends on `STEP40A` (must be merged/verified first —
needs `MigrationEntry` to exist). Format Expert + IO Architecture Expert consults, field lists
corrected against real shipped-code comments (not the original launching ticket's guesses).*

## Root problem
4 top-level V3 sections have real legacy data sitting in `mapGeneratorData` with no migration
path: `GeneralMapSettings`, `Symmetry`, `Accumulation`, `DetailNormal`. Each is a pure, zero-risk
flat-key relocation — same field meaning, different location, all `bIndependentlySelectable=true`
(no cross-migration ordering dependency, no shared source with any sibling migration).

**Corrected field lists (IO Architecture Expert caught 2 errors in the original research,
verified against real shipped-code comments — use these, not any earlier draft):**
- `GeneralMapSettings_Migrate_V2`: `Seed`, `ScaleFeaturesToMapSize`, `TerrainMinHeight`,
  `WorldUnitsPerCell` — 4 fields (NOT `GlobalGravity`, which `MapExporter_GeneralMapSettings_IO.cpp`
  and `SANMAP_FORMAT_SPEC.md` Correction 2 both confirm is a genuinely NEW field with zero legacy
  source; leave it untouched, its PARAMS default already applies).
- `Symmetry_Migrate_V2`: `GlobalSymmetryMask`, `SnapImperfectSymmetry`,
  `SymmetryDetectionTolerance`, `SymSuperpositionBlend`, `SymmetryBlurRadius`, `CrossFadeWidth`,
  `CylinderZScale`, `TorusMajorRadius`, `TorusMinorRadius` — 9 fields (NOT `SymAlgorithm`, which
  `MapExporter_Symmetry_IO.cpp` and the spec both confirm doesn't exist anywhere in `src/` —
  explicitly out of scope, STEP16 ruling #1; also NOT `RadialSymmetryRepeatCount`, genuinely new,
  ARCH §13, defaults to 3 already).
- `Accumulation_Migrate_V2`: no legacy fields — reserves the empty `Accumulation` top-level key.
- `DetailNormal_Migrate_V2`: `DetailNormalMapSize` — 1 field.

## Solution — shape
4 new files, `<Domain>_Migrate_V2_IO.h/.cpp`, each `void <Domain>_Migrate_V2(nlohmann::json&
document);` per `IO_MIGRATION_SPEC.md` §1's signature law. All fields relocate via `MoveKey`
(`JsonPrimitives_IO.h`) — same key name both sides in every case (confirmed above), so no
`RenameKey` is needed, only `MoveKey(document["mapGeneratorData"], "<Key>", document["<Section>"],
"<Key>")` (create `document["<Section>"]` as an object first if not already present — check how
sibling already-shipped migrations, if any exist as precedent in this codebase's PROC/PARAMS
migration examples, or just construct it directly).

`Accumulation_Migrate_V2` writes `document["Accumulation"] = nlohmann::json::object();` if not
already present (reservation only, `DefaultIfMissing` is the right primitive here).

**Paired tests** (`IO_MIGRATION_SPEC.md` §1): `<Domain>_Migrate_V2_IO_Test.cpp` per file — a
literal OLD-shape JSON fixture (hand-built, matching the confirmed real legacy key names above)
asserting the exact NEW shape after calling the migration function alone. Not a round-trip test,
not a runner test — one migration, one fixture, one exact assertion.

## Target files
- New `src/io/GeneralMapSettings_Migrate_V2_IO.h/.cpp` + `_Test.cpp`.
- New `src/io/Symmetry_Migrate_V2_IO.h/.cpp` + `_Test.cpp`.
- New `src/io/Accumulation_Migrate_V2_IO.h/.cpp` + `_Test.cpp`.
- New `src/io/DetailNormal_Migrate_V2_IO.h/.cpp` + `_Test.cpp`.

## Explicit out-of-scope
- Wiring these into the manifest — `STEP40F`, dispatched last after all of B–E ship.
- Deleting `mapGeneratorData`/legacy version keys — that's the manifest's `legacyKeysToDelete`,
  fired by the runner after a whole step completes, not any individual migration's job.
- Any other migration (`Flow`/`GlobalMarkerSettings`/`SlopeDefaults`/`StratumGenerationSettings`/
  `EntityCollections`) — separate tickets (`STEP40C`/`D`/`E`).

## Layer & accuracy class
IO only. Accuracy class: Exact.

## Acceptance test
1. Each of the 4 migration functions, called alone against its own hand-built OLD-shape fixture,
   produces exactly the documented NEW shape — nothing more, nothing less.
2. Calling each migration function on a document that DOESN'T have the relevant
   `mapGeneratorData` fields at all is a safe no-op (total/idempotent, per `JsonPrimitives_IO.h`'s
   own contract for its primitives).
3. Full `SanGenV2` build stays clean; every existing test continues to pass.
