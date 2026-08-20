# Work-Order — Step 40E: `EntityCollections` V2→V3 migration (army color + alias fold-in)

*Constitution §7. Executor: SanGen Coder. Depends on `STEP40A`. IO Architecture Expert consult.
Isolated as its own ticket — the launching research flagged this as the highest-bespoke-risk
migration: a per-key dictionary join plus a cross-container search, not composable from existing
primitives at all. Deserves undiluted review.*

## Root problem
Two pieces of legacy data have no current migration path:
1. `mapGeneratorData.Armies[key].Color` — a legacy `[r,g,b,a]` array, per army.
2. The legacy TOP-LEVEL `Aliases` dictionary (`aliasName → markerTransformName`, confirmed
   present with ~96 entries in the one real file checked this session) — an inverted index that
   needs to be folded INTO the marker data it points at, not just relocated as-is.

## Solution — shape
**Army color**: for each key in `mapGeneratorData.Armies`, `ConvertColorArrayToRgbaObject` the
`Color` field and move it to `document["armies"][key]["armyColor"]` (top-level `armies`, the
format-native collection — NOT the legacy `mapGeneratorData.Armies`, a different object; confirm
both exist and are correctly distinguished before writing code, they are easy to conflate by name
alone).

**Alias fold-in** (the genuinely bespoke part): for each `(aliasName, markerTransformName)` pair in
the legacy top-level `Aliases` dict, search every marker-type collection's transforms (confirmed
scope: whichever top-level collections hold named marker transforms — determine the exact set by
reading `MapImporter_Markers_IO.cpp`/`MapExporter_Markers_IO.cpp` and any sibling marker-family
collections, do not assume only one) for a transform whose own name equals `markerTransformName`,
and set that transform's `alias` field to `aliasName`. This is a per-key dictionary join across
containers — write it as a straightforward nested loop, no primitive composition attempt needed
(the IO Architecture Expert confirmed this isn't composable from `JsonPrimitives_IO.h`'s existing
primitives, a bespoke function is correct here, not a gap to fill with a new generic primitive).

Delete the legacy `Aliases` block only as part of the WHOLE STEP's shared `legacyKeysToDelete`
(already includes it) — this migration itself only reads it, never deletes it directly (matching
every other migration's read-only-until-shared-delete discipline).

`bIndependentlySelectable = false` — no sibling in the step reads/writes `armies`/marker aliases,
but the migration's own internal complexity (two unrelated sub-tasks in one function) makes a
confident independence claim premature; leave it whole-step-only for this first ship.

## Target files
- New `src/io/EntityCollections_Migrate_V2_IO.h/.cpp` + `_Test.cpp`.

## Explicit out-of-scope
- Wiring into the manifest — `STEP40F`.
- Any other migration — separate tickets.

## Layer & accuracy class
IO only. Accuracy class: Exact.

## Acceptance test
1. A fixture with 2+ armies, each with a legacy `Color` array, produces the correct `armyColor`
   object on each corresponding top-level `armies[key]` entry.
2. A fixture with a legacy `Aliases` dict pointing at real marker transform names correctly sets
   `alias` on the matching transforms across at least 2 different marker-type collections (not
   just one, to prove the search isn't scoped too narrowly).
3. An alias pointing at a transform name that DOESN'T exist anywhere is a safe no-op for that one
   entry (does not crash, does not create a phantom transform) — log it if `MapImportResult`-style
   logging is available in this context, otherwise silently skip.
4. A document with no legacy `Armies`/`Aliases` data at all is a safe no-op.
5. Full `SanGenV2` build stays clean; every existing test continues to pass.
