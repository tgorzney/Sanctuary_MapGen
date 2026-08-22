# STEP67 — `MarkersStack` symmetry-tier migration (V3 → V4)

**Layer:** IO/BRIDGE (migration). **Domain:** `Sanmap_MigrationManifest_IO`, new
`MarkersStack_Migrate_V3_IO`. **Sequence:** depends on `STEP66_MarkerRuleLayer_PARAMS.md` landing
first — this migration bridges old-shape (`STEP66`-era) `.sanmap` files into the new-shape reader
`STEP66` ships; it is not the shape change itself. Design authored by the IO Architecture Expert
consult this session (`ARCH_16_06_MigrationRouting.md` §16.6); this ticket transcribes that ruling into dispatchable form.

## Root problem
`kCurrentSanGenVersion = 3` today. Every `.sanmap` exported before `STEP66` lands has
`MarkersStack` as a flat array with per-rule `SymmetryUseGlobal`/`SymmetryMask`/
`RadialSymmetryRepeatCount`. Once `STEP66`'s importer expects the new layer-wrapped shape, an old
file needs a bridge or it silently loses/misreads its marker rules on import.

## Target files
- `src/io/MarkersStack_Migrate_V3_IO.h`/`.cpp` (new)
- `src/io/MarkersStack_Migrate_V3_IO_Test.cpp` (new)
- `src/io/Sanmap_MigrationManifest_IO.h` — bump `kCurrentSanGenVersion` 3 → 4, update its comment
- `src/io/Sanmap_MigrationManifest_IO.cpp` — register the new step

## Layer & accuracy class
IO/BRIDGE. Accuracy class: Exact.

## Backend policy
CPU only.

## ARCH rules invoked
- `IO_MIGRATION_SPEC.md` §1-7 — the `<Domain>_Migrate_V<N>_IO` convention, followed exactly.
- Constitution §6 — never silently degrade data; this migration is designed to lose nothing (see
  grouping algorithm below), the stronger discharge of that rule.

## Solution — shape

**Signature**: `void MarkersStack_Migrate_V3(nlohmann::json& document);` — matches the established
contract exactly (`Symmetry_Migrate_V2_IO.h` etc.).

**Manifest wiring** (`Sanmap_MigrationManifest_IO.cpp`): append
`MigrationStep{ /*sourceVersion=*/3, migrations = { MigrationEntry{ MarkersStack_Migrate_V3,
"MarkersStack_Migrate_V3", "<description>", /*bIndependentlySelectable=*/true } },
legacyKeysToDelete = {} }`. `legacyKeysToDelete` stays empty — `MarkersStack` reshapes in place,
nothing moves to a different top-level key. `bIndependentlySelectable = true` is correct: this
migration reads/writes exclusively within its own `MarkersStack` key, no cross-domain read (unlike
`SlopeDefaults_Migrate_V2`, correctly `false`).

**Grouping algorithm — eliminates the lossy-collapse risk by construction, not by picking a
winner and logging a warning:**

Walk the old flat `MarkersStack` array left to right. Start a new layer whenever `i == 0` or the
current rule's **effective** `(SymmetryUseGlobal, SymmetryMask, RadialSymmetryRepeatCount)` triplet
differs from the running group's. Each maximal contiguous run of rules sharing an identical
effective triplet becomes exactly one `MarkerRuleLayer`. A rule is never folded into a group unless
it already agrees with that group's setting — there is no "which one wins" step, ever, because
nothing is ever discarded.

"Effective," not raw JSON value: read each field with the same typed accessors the live reader
already uses (`ReadJsonBoolean`, `ReadJsonInteger`, `ReadJsonIntegerClamped(...,
radialSymmetryRepeatCountMinimum, radialSymmetryRepeatCountMaximum, ...)`) against a scratch
triplet seeded with `MarkerRule`'s own struct defaults (`bSymmetryUseGlobal = true`,
`symmetryMask = 0`, `radialSymmetryRepeatCount = 3`) before comparing. Consequences:
- A rule missing a key entirely compares equal to a neighbor that explicitly carries that field's
  default value — no spurious group-splitting from a hand-edited/partial file.
- An out-of-range value is written onto the layer already clamped — the same normalization the
  current importer already applies, not new lossy behavior.

**Per-layer output:**
- `Name`: synthesized, `"Migrated Layer <N>"`, 1-indexed in output order (old rules carry no name
  field to relocate from).
- `Enabled`/`Hidden`: **left unset** — the rewritten importer's own struct defaults govern on read,
  same precedent `DetailNormal_Migrate_V2`/`Accumulation_Migrate_V2` already set for genuinely-new
  fields with no legacy source.
- `SymmetryUseGlobal`/`SymmetryMask`/`RadialSymmetryRepeatCount`: the group's one shared triplet.
- `Rules`: the run's member rule objects, in original order, each with `SymmetryUseGlobal`/
  `SymmetryMask`/`RadialSymmetryRepeatCount` deleted (`DeleteKeyIfPresent`), every other field
  untouched.

**Order preservation**: concatenating every output layer's rules in layer order reproduces the
exact original array order — no rule is ever reordered relative to another, only grouped. Merging
non-adjacent groups sharing an identical triplet (cosmetically tidier) is explicitly **not** done —
it would change flattened rule-execution order, which this ticket does not assume is
order-independent for `Placement_Rules_PROC.cpp`'s priority/overlap resolution. Not a future
enhancement to build without a Generator Expert order-independence sign-off first.

**Real, expected UX consequence, not a bug**: an old file with alternating per-rule symmetry
settings (`A,B,A,B`) migrates into 4 single-rule layers, not 2 — every rule keeps its exact
original setting and position; nothing is lost.

## Explicit out-of-scope
- **Non-adjacent same-triplet cosmetic merging** — flagged above, needs a separate Generator
  Expert ruling first.
- **`STEP66` itself** — this ticket assumes the new PARAMS shape/importer already exist; it does
  not implement them.
- **Any change to `MarkersStack_Migrate_V3`'s sibling migrations** (`GeneralMapSettings`,
  `DetailNormal`, `Symmetry`, etc.) — untouched.

## Acceptance test
1. All rules share one triplet → one layer, all rules, correct lifted triplet.
2. Two contiguous groups (A,A,B,B) → two layers in order, correct membership/triplet each.
3. Alternating (A,B,A) → three single-rule layers, original order preserved end-to-end.
4. Missing-key rule compares equal to an explicit-default neighbor (no spurious split).
5. Out-of-range value normalizes/clamps on the layer, not passed through raw.
6. Post-migration, no rule object retains any of the 3 removed keys.
7. Every untouched rule field (`Category`, `MinSlope`, `HydroMultiplier`, …) survives verbatim.
8. Empty/missing `MarkersStack` → total no-op.
9. Second call on already-V4-shaped output → safe no-op (idempotency, same posture every existing
   migration test asserts).
10. `bIndependentlySelectable` isolation assertion — running this migration alone reproduces the
    full-step result (trivially true here, but the convention requires the explicit test).

Full `SanGenV2` build stays clean; every existing test continues to pass.
