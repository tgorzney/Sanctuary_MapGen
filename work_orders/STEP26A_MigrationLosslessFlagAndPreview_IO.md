# STEP26A — `bLosslessIfSkipped` manifest field + migration preview/selective-apply functions

**Layer:** IO/BRIDGE. **Domain:** the migration manifest + a new preview surface.
**Consulted:** SanGen IO Architecture Expert, SanGen ARCH Expert (this session — ratified in
`sangen_arch_pack/specs/IO_MIGRATION_SPEC.md` §3/§6, currently uncommitted in the working tree;
the corresponding `ARCH.md` §17 summary entry could not be written — the file has grown past the
ARCH Expert's safe rewrite size and is pending a split into per-section files. The spec text is
the actual ratified law; treat it as authoritative even without the ARCH.md entry).

## Problem
`STEP26` (a UI dialog to preview/selectively-apply the no-version-marker migration walk) was
designed but never built. Redesigning it surfaced a real correctness gap: `MigrationEntry::
bIndependentlySelectable` (`src/io/Sanmap_MigrationManifest_IO.h`) only answers "safe to run out
of order" — not "safe to omit without losing data." Auditing all 9 shipped migrations against the
real reader architecture (`MapImporter_IO.h`, `MapImporter_ParseDocument_IO.cpp`): block readers
are current-shape-only by law; only Geometry/Water/StrataSettings have a legacy-
`mapGeneratorData`-gated fallback reader, and none of the 9 migrations target those three domains.
`mapGeneratorData` also no longer survives export (STEP36). So skipping any migration that isn't
pure key-reservation silently orphans its data, permanently, after one export cycle.

## Ratified fix (`IO_MIGRATION_SPEC.md` §3, already written — read it in full before implementing)
A second, orthogonal, author-declared field: `bLosslessIfSkipped`, defaulting `false`. A dialog
may only offer a genuine "skip this" checkbox for an entry where **both**
`bIndependentlySelectable == true` **and** `bLosslessIfSkipped == true`. Declaring `true` requires
a paired test (same discipline as `bIndependentlySelectable`'s own): an assertion in the
migration's own `<Domain>_Migrate_V<N>_IO_Test.cpp` proving the data survives via some other
reader when this migration alone is skipped and every sibling in its step still runs.

## Part 1 — the manifest field (mechanical, same shape as `bIndependentlySelectable`)
```cpp
// Sanmap_MigrationManifest_IO.h
struct MigrationEntry {
    MigrationFunction function;
    const char*       name;
    const char*       description;
    bool              bIndependentlySelectable = false;
    bool              bLosslessIfSkipped       = false;   // NEW
};
```

## Part 2 — the 9 shipped migrations' actual values (the audit this ticket performs, since
`ARCH.md` §17 couldn't be written to record it — this table IS the authoritative determination,
made per `IO_MIGRATION_SPEC.md` §3's test criteria)
Only **`Accumulation_Migrate_V2`** is pure key-reservation (reserves an empty `Accumulation`
top-level key, nothing to relocate) — the trivial lossless case the spec names by example.
Every other migration relocates real data with no fallback reader:

| Migration | `bLosslessIfSkipped` | Why |
|---|---|---|
| `Accumulation_Migrate_V2` | `true` | Pure key reservation — nothing to relocate. |
| `DetailNormal_Migrate_V2` | `false` | Relocates real fields, no fallback reader for `DetailNormal`. |
| `EntityCollections_Migrate_V2` | `false` | Relocates entity data, no fallback reader. |
| `Flow_Migrate_V2` | `false` | Relocates real fields, no fallback reader for `Flow`. |
| `GeneralMapSettings_Migrate_V2` | `false` | Relocates real fields, no fallback reader. |
| `GlobalMarkerSettings_Migrate_V2` | `false` | Relocates real fields, no fallback reader. |
| `SlopeDefaults_Migrate_V2` | `false` | Relocates real fields, no fallback reader. |
| `StratumGenerationSettings_Migrate_V2` | `false` | Relocates real fields, no fallback reader. |
| `Symmetry_Migrate_V2` | `false` | Relocates real fields, no fallback reader for `Symmetry`. |

Only `Accumulation_Migrate_V2` needs the paired test added (a trivial one, since there's nothing
to verify recovers — assert the step's other migrations still produce a valid document with
`Accumulation` absent, per the spec's own "nothing to carry" allowance). The other 8 need no new
test — `false` is the safe default, no justification-test required by the spec for `false`. Add a
one-line comment at each `false` declaration site stating the same one-sentence reason as the
table above, matching this codebase's existing citation-heavy convention (every field elsewhere
in this file already carries this level of reasoning).

**⚠️ If any of the 4 migrations currently marked `bIndependentlySelectable = true`
(`GeneralMapSettings`, `Symmetry`, `DetailNormal`, and one more — confirm the current 4 against
`Sanmap_MigrationManifest_IO.cpp` directly, don't trust this list from memory) also default to
`bLosslessIfSkipped = false` per this table, that's expected and correct — `bIndependentlySelectable`
and `bLosslessIfSkipped` are independent properties per the ratified rule. Do not "fix" one to
match the other.**

## Part 3 — preview/apply functions (new file pair, IO Architecture Expert's design)
New `src/io/Sanmap_MigrationPreview_IO.h`/`.cpp` (sibling to `Sanmap_MigrationManifest_IO`/
`Sanmap_MigrationRunner_IO` — migration-system machinery, not a per-domain fragment):

```cpp
struct MigrationPreviewEntry {
    const char*     name;
    const char*     description;
    bool            bIndependentlySelectable;
    bool            bLosslessIfSkipped;
    bool            bWouldChangeDocument;
    nlohmann::json  diffPatch;   // json::diff(before, after) for this entry alone
};
struct MigrationPreviewStep {
    int                                 sourceVersion;
    std::vector<MigrationPreviewEntry>  entries;
    std::vector<const char*>            legacyKeysToDelete;   // informational mirror only
};
struct MigrationPreviewReport {
    int                                assumedStartingVersion = 1;
    std::vector<MigrationPreviewStep> steps;
};

// Non-mutating. Run each MigrationEntry::function against a COPY of the document, diff via
// nlohmann::json::diff (already available, no new diff primitive). Caller must have already
// confirmed via MapImportResult that no version marker was found before calling this.
MigrationPreviewReport PreviewSanmapMigrationWalk(const nlohmann::json& document);

// Mutates `document` in place per the human's selection. `selectedNames` are the
// MigrationEntry::name values opted IN. Every bIndependentlySelectable == false entry in a step
// runs unconditionally whenever any entry of that step is selected — full-step semantics for
// non-independent entries, unchanged. Only writes document["SanGenVersion"] = sourceVersion + 1
// for a step where EVERY entry (independent and not) was selected; a partial application leaves
// SanGenVersion untouched and does not proceed into any further step (a later step's migrations
// may assume the prior step's shape is complete). legacyKeysToDelete fires only on full-step
// application, per the existing law this ticket does not change.
void ApplySelectedSanmapMigrations(nlohmann::json& document,
                                   const std::vector<std::string>& selectedNames,
                                   MapImportResult& result);
```

Do **not** add a dry-run flag to `RunSanmapMigrations` itself — it's the non-fallible, always-runs,
always-committed automatic path (`MapImporter_ParseDocument_IO.cpp:127`); conflating it with an
on-demand preview risks the preview path becoming reachable from the automatic one. Keep them
fully separate.

## Part 4 — one new `MapImportResult` field
`bool bNoVersionMarkerFound = false;` (`MapImporter_IO.h`), set alongside the existing
`Warn(...)` call in `Sanmap_MigrationRunner_IO.cpp`'s no-marker branch. This is what STEP26B's
"Check for Migrations…" button gates on.

## Files touched
- `src/io/Sanmap_MigrationManifest_IO.h` — `bLosslessIfSkipped` field
- `src/io/Sanmap_MigrationManifest_IO.cpp` — the 9 explicit values + comments
- `src/io/Sanmap_MigrationPreview_IO.h`/`.cpp` — new
- `src/io/MapImporter_IO.h` — `MapImportResult::bNoVersionMarkerFound`
- `src/io/Sanmap_MigrationRunner_IO.cpp` — set the new field
- `src/io/Accumulation_Migrate_V2_IO_Test.cpp` — the one new paired test

## Verify
Full solo rebuild + `ctest -C Debug`, full suite green. New test: the paired
`bLosslessIfSkipped` assertion on `Accumulation_Migrate_V2`. New test for `PreviewSanmapMigrationWalk`/
`ApplySelectedSanmapMigrations`: a synthetic no-marker document, preview reports all 9 entries
with correct diffs, selective apply with only `Accumulation_Migrate_V2` selected leaves every
other migration's target key absent and `SanGenVersion` unset (partial-apply, no version bump);
full selection produces `SanGenVersion == 3` and all target keys present.
