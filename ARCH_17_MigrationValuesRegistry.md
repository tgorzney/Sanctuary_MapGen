[← ARCH index](ARCH.md) · SanGen ARCH §17. Part of the ratified v2 architecture; the Constitution (`sangen_arch_pack/CONSTITUTION.md`) and this file's preamble in `ARCH.md` bind alongside it. **Only the ARCH Expert writes this file.**

## 17. `bLosslessIfSkipped` values for the 9 shipped `SanGenVersion` 2→3 migrations (ARCH ruling, ratifies `IO_MIGRATION_SPEC.md` §3, backfills `work_orders/STEP26A_MigrationLosslessFlagAndPreview_IO.md`)

**Backfill note.** `IO_MIGRATION_SPEC.md` §3 and `STEP26A_MigrationLosslessFlagAndPreview_IO.md` both
cite this section as the authoritative record of the per-migration `bLosslessIfSkipped` audit;
neither could actually be written into `ARCH.md` at ratification time because the file had grown
monolithic and past the ARCH Expert's safe single-file rewrite size (recorded in `ARCH.md`'s "Known
gap" note and in STEP26A's own header). `ARCH.md` is now split into per-section `ARCH_NN_*.md` files,
which removes that blocker — this section is that retry. The audit's content and values are
unchanged from STEP26A's own text; this is a transcription into binding ARCH law, not a re-audit.
Confirmed against the shipped manifest (`src/io/Sanmap_MigrationManifest_IO.cpp`) at time of writing:
the 9 migrations, their names, and their current `bIndependentlySelectable` values match this
section's table exactly — `bLosslessIfSkipped` itself is not yet a field on `MigrationEntry` in the
shipped header (`src/io/Sanmap_MigrationManifest_IO.h`), i.e. `STEP26A` is still an undispatched
work-order; this ruling is what that ticket's Part 2 implements when dispatched.

### The rule this table exists to apply

`IO_MIGRATION_SPEC.md` §3 distinguishes two independent, orthogonal properties on a `MigrationEntry`:
`bIndependentlySelectable` (safe to run out of order relative to its siblings — an **ordering**
question) and `bLosslessIfSkipped` (safe to omit entirely without losing data — a **data-safety**
question). A selective-apply dialog (`IO_MIGRATION_SPEC.md` §6, the STEP26 feature) may only offer a
genuine "skip this" checkbox where **both** are `true`. Declaring `bLosslessIfSkipped = true` requires
verifying — against the real reader architecture, not the migration's own transform — that every
field the migration would relocate remains recoverable by some current-shape block reader or an
existing legacy-`mapGeneratorData`-gated fallback reader when this migration alone is skipped and
every sibling in its step still runs.

### The audit

Read directly against `MapImporter_IO.h`/`MapImporter_ParseDocument_IO.cpp`: block readers are
current-shape-only by law; only Geometry/Water/StrataSettings carry a legacy-`mapGeneratorData`-gated
fallback reader, and **none of the 9 shipped migrations target those three domains.**
`mapGeneratorData` also no longer survives export (STEP36) — so skipping any migration that is not
pure key-reservation would silently orphan its data, permanently, after one export cycle. Only
**`Accumulation_Migrate_V2`** is pure key-reservation (it reserves an empty `Accumulation` top-level
key; there is nothing to relocate) — the trivial lossless case `IO_MIGRATION_SPEC.md` §3 names by
example. Every other migration relocates real data with no fallback reader:

| Migration | `bIndependentlySelectable`¹ | `bLosslessIfSkipped` | Why |
|---|---|---|---|
| `Accumulation_Migrate_V2` | `true` | **`true`** | Pure key reservation — nothing to relocate. |
| `DetailNormal_Migrate_V2` | `true` | `false` | Relocates real fields, no fallback reader for `DetailNormal`. |
| `EntityCollections_Migrate_V2` | `false` | `false` | Relocates entity data, no fallback reader. |
| `Flow_Migrate_V2` | `false` | `false` | Relocates real fields, no fallback reader for `Flow`. |
| `GeneralMapSettings_Migrate_V2` | `true` | `false` | Relocates real fields, no fallback reader. |
| `GlobalMarkerSettings_Migrate_V2` | `false` | `false` | Relocates real fields, no fallback reader. |
| `SlopeDefaults_Migrate_V2` | `false` | `false` | Relocates real fields, no fallback reader. |
| `StratumGenerationSettings_Migrate_V2` | `false` | `false` | Relocates real fields, no fallback reader. |
| `Symmetry_Migrate_V2` | `true` | `false` | Relocates real fields, no fallback reader for `Symmetry`. |

¹ `bIndependentlySelectable` values are the already-shipped ground truth, confirmed directly against
`src/io/Sanmap_MigrationManifest_IO.cpp` (not re-derived here) — four migrations carry it today:
`GeneralMapSettings_Migrate_V2`, `Symmetry_Migrate_V2`, `Accumulation_Migrate_V2`, and
`DetailNormal_Migrate_V2`. **`bIndependentlySelectable` and `bLosslessIfSkipped` are independent
properties, and their values are expected to disagree** — three of the four
`bIndependentlySelectable == true` entries are still `bLosslessIfSkipped == false` in this table, and
that is correct, not a defect to reconcile. A future implementer must not "fix" one flag to match the
other.

### Test obligations this ruling creates

Per `IO_MIGRATION_SPEC.md` §3's paired-test discipline: only `Accumulation_Migrate_V2` needs a new
test when `STEP26A` is implemented — a trivial one, asserting the step's other 8 migrations still
produce a valid document with `Accumulation` absent (the spec's own "nothing to carry" allowance for
a pure key-reservation case). The other 8 need no new test — `false` is the safe default, and the spec
requires a justification test only for a `true` declaration. Each `false` declaration site should
carry a one-line comment stating the matching reason from the table above, matching this file's
existing citation-heavy convention.

### Scope note

This section rules on values only — it does not itself add the `bLosslessIfSkipped` field to
`MigrationEntry`, wire the preview/selective-apply functions (`Sanmap_MigrationPreview_IO`), or add
`MapImportResult::bNoVersionMarkerFound`. Those remain `STEP26A_MigrationLosslessFlagAndPreview_IO.md`'s
implementation work, unblocked by this ruling and cross-referencing it in place of the placeholder
note its own header currently carries.
