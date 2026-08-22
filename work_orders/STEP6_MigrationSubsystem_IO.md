# Work-Order — Step 6: `SanGenVersion` + the migration subsystem infrastructure
# (no schema cutover yet)

*Constitution §7. Executor: SanGen Coder. First slice of `work_orders/SPEC-4_SanmapSchemaV3_DOCS.md`
(ratified, applied to `SANMAP_FORMAT_SPEC.md`) and `sangen_arch_pack/specs/IO_MIGRATION_SPEC.md`
(the full binding mechanism spec, read in full for this ticket). Deliberately scoped to the
infrastructure only — closes the literal "`SanGenVersion` written but never read" defect
(`IO_MIGRATION_SPEC.md`'s own framing) safely, with ZERO real schema migrations yet. The actual
`GeneralMapSettings`/`HeightmapStack`/`Symmetry`/etc. cutover (SPEC-4 Corrections 2-11) is
explicitly NOT this ticket — see "Why this ticket stops short of the schema cutover."*

## Root problem
`MapExporter_IO.h:75` writes `mapGeneratorDataVersion` (currently `2`) into the nested
`mapGeneratorData.MapGeneratorDataVersion` JSON field, but `MapImporter_Recipe_IO.cpp` has **no
version branch at all** — every document, regardless of its declared version, is read as if it
were the current shape. This is the exact silent-degrade risk Constitution §6 forbids ("a version
mismatch must not silently degrade"). `SANMAP_FORMAT_SPEC.md` Correction 1 (already ratified)
replaces this with a top-level `SanGenVersion` field; `IO_MIGRATION_SPEC.md` (already ratified)
is the full mechanism that makes it an enforced gate instead of a second write-only constant.

## Target files
New:
- `src/io/JsonPrimitives_IO.h` — header-only, `inline` functions, per `IO_MIGRATION_SPEC.md` §5.
- `src/io/JsonPrimitives_IO_Test.cpp` — paired test.
- `src/io/Sanmap_MigrationManifest_IO.h`/`.cpp` — the ordered per-step migration table
  (empty for this ticket — see below) and `kCurrentSanGenVersion`.
- `src/io/Sanmap_MigrationRunner_IO.h`/`.cpp` — version resolution, the walk-forward loop
  (a no-op today, since there are zero steps), and the refusal law.
- `src/io/Sanmap_MigrationRunner_IO_Test.cpp` — paired test.

Modified:
- `src/io/MapImporter_Recipe_IO.h` — `ReadJsonFloat`/`ReadJsonInteger`/`ReadJsonBoolean`/
  `ReadJsonEnumeration`/`ReadJsonText` **move out** to `JsonPrimitives_IO.h` (§5's named
  relocation — this header keeps only the Recipe-domain block-reader declarations after this
  ticket). `ReadJsonFloatVector4` stays (it's Stratum/Recipe-domain-specific shape, not a generic
  primitive — confirm this reading against §5's list before moving it; the spec names five
  functions to relocate, not six).
- `src/io/MapExporter_IO.cpp`/`MapImporter_Layers_IO.cpp`/`MapImporter_Rules_IO.cpp`/every other
  file that `#include`s `MapImporter_Recipe_IO.h` **purely** to reach the five relocated
  functions — repoint those includes to `JsonPrimitives_IO.h` instead. Grep the whole tree for
  each relocated function name to find every call site; do not assume the list above is exhaustive.
- `src/io/MapExporter_IO.h` — write `document["SanGenVersion"] = kCurrentSanGenVersion` at the
  TOP level (sibling of `fileVersion`/`mapVersion`/`name`, NOT nested in `mapGeneratorData` —
  Correction 1 says "top-level", confirmed against `SANMAP_FORMAT_SPEC.md`'s "Entity collections"/
  "Top-level map fields" framing used identically for `armies`/`areas`/etc.). Remove the write of
  `mapGeneratorData.MapGeneratorDataVersion`/the `mapGeneratorDataVersion` constant — `SanGenVersion`
  **replaces** it (Correction 1's exact word), it does not sit alongside it.
- `src/io/MapImporter_IO.cpp` — `ParseSanmapJsonText` calls the new runner on the freshly-parsed
  `document`, BEFORE any block reader (`ReadGeometryJson`, `ReadAreasJson`, etc. — literally the
  first thing after the `document.is_object()` check), per `IO_MIGRATION_SPEC.md` §4.4.

## Layer & accuracy class
IO/BRIDGE. Accuracy class: Exact — version resolution and the refusal law are correctness-critical,
not best-effort.

## Backend policy
CPU only.

## ARCH rules invoked
- `IO_MIGRATION_SPEC.md` in full — this ticket implements §1 (migration unit shape, though zero
  are written yet), §3 (manifest), §4 (runner), §5 (`JsonPrimitives_IO.h`), §6 (refusal law)
  verbatim. Do not deviate from any named function/file/constant in that spec without flagging it
  back rather than improvising.
- `SANMAP_FORMAT_SPEC.md` Correction 1 (`SanGenVersion` field, top-level, replaces
  `MapGeneratorDataVersion`).
- Constitution §6 — the refusal law IS this ticket's core deliverable.
- ARCH_01_05_FileSizeCeilings.md §1.5 (`Type_Aspect_LAYER` file-split pattern) — `IO_MIGRATION_SPEC.md` §1 already confirms
  the migration-unit naming is an instance of this existing law, not a new exception.

## Why this ticket stops short of the schema cutover
`SPEC-4`'s Corrections 2-11 (the actual `GeneralMapSettings`/`HeightmapStack`/`Symmetry`/
`SlopeDefaults`/`Flow`/`Accumulation`/`MarkersStack`/`PropsStack`/`DecalsStack`/`UnitsStack`/
`DetailNormal` sections, plus several genuinely new PARAMS fields like `GlobalGravity` and
`bSlopeUseGlobal`) are a large, multi-file, multi-domain rewrite of nearly every existing IO
read/write path — `IO_MIGRATION_SPEC.md` §7's own "worked example" lists **thirteen** separate
migration files for that one version step alone, and `SPEC-4` itself explicitly excludes any
`src/io/`/`src/params/` code change from its own scope ("Any code change... this work-order
updates the spec pack only"). Attempting the whole cutover as one ticket risks exactly the
half-finished, hard-to-review state this project's whole work-order discipline exists to avoid.

This ticket instead lands the **foundation** the cutover needs, in a state that is immediately
safe and useful on its own: `kCurrentSanGenVersion` is set to **`2`** for this ticket — not `3`.
This retroactively and honestly names TODAY's already-shipped shape (the `mapGeneratorData` blob,
unchanged by this ticket) as "SanGenVersion 2," swaps the write-only legacy constant for a real,
top-level, henceforth-enforced version field, and wires the runner in with an **empty manifest**
(zero migration steps — a resolved version of `2` already equals `kCurrentSanGenVersion`, so the
runner's walk-forward loop is a documented no-op today). This closes the literal defect
(`SanGenVersion` written but never read) without touching a single existing block reader's shape.
**Bumping `kCurrentSanGenVersion` to `3` and writing the thirteen real Correction-2-through-11
migration files is the next ticket(s)**, once this foundation is in place and reviewed.

## Solution

1. **`JsonPrimitives_IO.h`** — relocate verbatim (same signatures, same behavior, just a new
   file/namespace-free-function-location):
   ```cpp
   inline bool ReadJsonFloat(const nlohmann::json& parent, const char* key, float& destination);
   inline bool ReadJsonInteger(const nlohmann::json& parent, const char* key, int& destination);
   inline bool ReadJsonBoolean(const nlohmann::json& parent, const char* key, bool& destination);
   inline bool ReadJsonText(const nlohmann::json& parent, const char* key, std::string& destination);
   inline bool ReadJsonEnumeration(const nlohmann::json& parent, const char* key, int valueCount,
                                   int& destination);
   ```
   Plus the five new transform primitives, exact names/behaviors per `IO_MIGRATION_SPEC.md` §5
   (signatures given there are binding; ergonomic details like default arguments are the Coder's
   call): `RenameKey`, `MoveKey`, `WrapScalarAsVector`, `DefaultIfMissing`, `DeleteKeyIfPresent`.
   All total and idempotent (§5's own requirement) — write a test for idempotency (calling twice
   produces the same result as calling once) for at least `DeleteKeyIfPresent`/`RenameKey`, since
   those are the two most likely to be mis-implemented as non-idempotent.

2. **`Sanmap_MigrationManifest_IO.h`/`.cpp`**:
   ```cpp
   inline constexpr int kCurrentSanGenVersion = 2;   // see "Why this ticket stops short" above —
                                                       // NOT 3. Bumped by the future cutover ticket.
   // Ordered table: for each source version N, the ordered migration-function list and legacy
   // keys to delete for that step. Empty for this ticket — zero steps exist yet.
   // (Concrete container shape — e.g. std::vector<MigrationStep> — is the Coder's call, matching
   // IO_MIGRATION_SPEC.md §3's "sparse by construction" framing; do not pre-build entries for the
   // future V2->V3 step, that's the next ticket's job and belongs in its own commit.)
   ```

3. **`Sanmap_MigrationRunner_IO.h`/`.cpp`** — implements §4 verbatim:
   - Version resolution: read top-level `document["SanGenVersion"]`. If absent, fall back to the
     legacy `document["mapGeneratorData"]["MapGeneratorDataVersion"]` field and **log** that the
     fallback fired (never silent). If **neither** is present, refuse (§6) — do not guess `1`.
   - Refusal law (§6): a resolved version greater than `kCurrentSanGenVersion` refuses outright
     with a clear logged reason. A resolved version equal to `kCurrentSanGenVersion` is a no-op
     pass-through (still runs resolution and the refusal check, calls no migration). A resolved
     version between the fallback and `kCurrentSanGenVersion` would walk the manifest's ordered
     steps — for THIS ticket, with an empty manifest, a resolved version of exactly `2` is the
     only value that can legally reach the pass-through path; anything else this ticket can
     actually encounter in the wild (nothing, since nothing has ever written anything but `2`)
     would just fall through the empty loop unchanged. Do not special-case this — write the loop
     generically per §4.2, it will simply not iterate today.
   - Return type: mirror the existing `MapImportResult{ bSucceeded, warningCount, Log()/Warn() }`
     pattern already live in `src/io/` (§4.5's explicit instruction) rather than inventing a new
     result type or throwing.

4. **Wire into `MapExporter_IO.h`/`.cpp`**: `document["SanGenVersion"] = kCurrentSanGenVersion;`
   at top level, replacing the `mapGeneratorData.MapGeneratorDataVersion` write and the
   `mapGeneratorDataVersion` constant entirely (Correction 1: "replacing", not "alongside").

5. **Wire into `MapImporter_IO.cpp`**: `ParseSanmapJsonText` runs the migration runner on
   `document` immediately after confirming `document.is_object()`, before `ReadJsonFloat(document,
   "height", ...)` or any other read. If the runner refuses, `ParseSanmapJsonText` returns `false`
   with the runner's logged reason — mirror the existing early-return pattern already used for a
   malformed document.

6. **Relocate the five functions and repoint every include** — grep-verify, do not assume the
   file list in "Target files" is exhaustive.

## Explicit out-of-scope
- **All of SPEC-4 Corrections 2-11** — the actual schema cutover, tracked as the next ticket(s).
  Nothing about `GeneralMapSettings`/`HeightmapStack`/`Symmetry`/`SlopeDefaults`/`Flow`/
  `Accumulation`/`MarkersStack`/`PropsStack`/`DecalsStack`/`UnitsStack`/`DetailNormal`/
  `EntityCollections`-merge is touched by this ticket. No new PARAMS field (`GlobalGravity`,
  `bSlopeUseGlobal`, per-rule symmetry override on `GeoLayer`/`Layer`, etc.) is added.
- **Any real migration file** (`<Domain>_Migrate_V2_IO.h/.cpp`) — the manifest stays empty.
- **The V1-dialect legacy-detection sniff** (§6's "a genuinely un-versioned true-v1-dialect
  document... gets its own explicitly-named legacy-detection step") — no evidence such a document
  needs support; not designed or built here.
- **`MASKING_SPEC.md` §1.7 amendment** — Correction 5's territory, not this ticket's.

## Acceptance test
1. `JsonPrimitives_IO_Test.cpp`: each of the five transform primitives, unit-tested independently,
   including the idempotency check named in Solution step 1.
2. `Sanmap_MigrationRunner_IO_Test.cpp`: (a) a document with `SanGenVersion: 2` → passes through,
   no warning logged; (b) a document with no `SanGenVersion` but a legacy
   `mapGeneratorData.MapGeneratorDataVersion: 2` → passes through, exactly one warning logged
   about the fallback; (c) a document with neither field → refused, `bSucceeded == false`,
   reason logged; (d) a document with `SanGenVersion: 99` (newer than current) → refused, reason
   logged, distinct from case (c)'s reason.
3. The existing `MapExporter_IO_Test.exe`/`MapImporter_IO_Test.exe` round-trip suite (every prior
   step's tests) still passes unchanged — the exported document's shape does not change at all
   except the version field's name/location, and every existing round-trip fixture goes through
   export→import, so this is the real regression check that nothing else moved.
4. Full `SanGenV2` build stays clean (aside from the separately-tracked, pre-existing
   `PreviewComposite_Wysiwyg_UI_Test.cpp` failure, not this ticket's to fix).
