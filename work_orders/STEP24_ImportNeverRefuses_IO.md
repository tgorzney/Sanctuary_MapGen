# Work-Order — Step 24: `.sanmap` import never refuses — best-effort recovery + Unknown-Import bag

*Constitution §1/§6/§7. Executor: SanGen Coder. **BLOCKED until the ARCH Expert ratifies the
Constitution §6 / `IO_MIGRATION_SPEC.md` §6 amendment this ticket depends on** — see "ARCH
ratification gate" below. This is a human standing directive, not a domain-expert proposal: "This
will override existing ARCH if ARCH says otherwise. This is the new standard." Reconciled through
three read-only expert consults (IO Architecture Expert on the runner/bag mechanism, Format Expert
on real-world file coverage, ARCH Expert on the law itself).*

## ARCH ratification gate — RESOLVED, ratified in two rounds
Round 1 ratified the base never-refuse law. Round 2 (superseding round 1's "no version marker →
resolve to 1, walk forward automatically" clause) ratified the final design: no version marker →
direct-read only, committed, no migration walk, ever — with a separate UI-layer-only preview/apply
feature (out of scope for THIS ticket, see `STEP26` once drafted) that a human may invoke
afterward. Verified directly against the live files:
`sangen_arch_pack/CONSTITUTION.md` §6 and `sangen_arch_pack/specs/IO_MIGRATION_SPEC.md` §3/§6 now
carry the ratified text below. This ticket implements exactly the "ordinary import call" half of
§6 — the always-committed, non-interactive direct-read recovery. It does NOT implement the
preview/selective-apply feature or the `MigrationEntry`/`bIndependentlySelectable` manifest shape
change (also newly ratified in §3) — those belong to a separate ticket once real migration files
exist to preview (see "Critical follow-on, discovered by audit" below).

**Critical follow-on, discovered by a full migration-coverage audit run in parallel with this
ticket's design (not this ticket's job to fix, but blocks the "never lose data" goal until it
does):** zero migration files exist anywhere in `src/io/` today. `kCurrentSanGenVersion` is still
`2`, never bumped to `3` despite `SANMAP_FORMAT_SPEC` Correction 1 saying "Bumped to 3," and every
schema-v3 Correction (2 through 14) shipped as a new-shape-only reader with no legacy-shape
fallback. Consequence: an old `mapGeneratorData`-blob-shaped file declares version 2 (which equals
`kCurrentSanGenVersion`), passes the version gate as a silent no-op, and the new readers find
nothing at the new locations — silently defaulting away the entire heightmap layer stack, all
placement/scatter rules, seed/general settings, symmetry settings, and per-stratum slope-gate
tuning, with no warning logged. This ticket's never-refuse fix does not create this gap and does
not need to fix it (out of scope, see below) — but it doesn't close it either, and the human
should not read "STEP24 shipped" as "old files now round-trip correctly." A separate ticket must
build the actual `<Domain>_Migrate_V2_IO` files per `IO_MIGRATION_SPEC.md` §7's worked example and
bump the version constant.

Below (§0) is the exact ratified text, reproduced for reference.

### §0 — the final ratified text (verbatim, verified directly against the live files)

**`CONSTITUTION.md` §6, final two sentences, as ratified:**
> A `.sanmap`'s declared schema version is external input too, but its value is never grounds to refuse the file: an absent, old, or newer-than-this-build version is always a loud, logged best-effort — the importer migrates what it recognizes and recovers what it does not into a preserved passthrough rather than dropping it, never a silent best-effort and never a flat refusal (`IO_MIGRATION_SPEC`). This does not relax the sentence above it: a file that fails to parse, is not a JSON object, or fails the size/header checks is still refused outright — only a version marker's value stops being refusal-worthy.

**`IO_MIGRATION_SPEC.md` §6, "Recovery law," as ratified (this ticket implements only the
non-bolded portion — the bolded UI-layer preview/apply feature is explicitly out of scope, its
own future ticket):**
- Newer than `kCurrentSanGenVersion` → never refused. Loud warning logged. No migration steps run
  (nothing forward to migrate to). Block readers read whatever current-shape keys are present;
  anything unrecognized falls to Unknown Import.
- No version marker of any kind → never refused. Loud warning logged. The runner does **not**
  resolve a starting version or walk any migration — the document is handed to the readers exactly
  as found, current-shape keys only. **This direct read (plus the existing legacy
  `mapGeneratorData`-gated readers, plus Unknown Import) is the sole committed result of an
  ordinary import call for this case: no guessing, ever.** *(Out of scope for this ticket: "A
  separate, UI-layer-only feature may preview what treating this document as an assumed version 1
  and walking the full migration chain would find... and let a human selectively apply some or all
  of it... if they do, that second pass's result... replaces the direct-read result entirely."
  This is a future ticket, `STEP26`, gated on real migration files existing to preview.)*
- Old, in-range version (present, less than `kCurrentSanGenVersion`) → unchanged, walk forward per
  §4, loud-logged. Never a refusal case.
- **Unknown Import passthrough.** Any top-level key the manifest's migration steps do not consume
  and that is not one of `SANMAP_FORMAT_SPEC`'s current sections is preserved verbatim under one
  reserved top-level key, `UnknownImport`, instead of being silently dropped. Round-trips:
  `MapExporter_IO` writes it back out unchanged on re-export. A key already consumed by a
  recognized migration or matching a current-schema section is never duplicated into it.
- **Layer ruling.** None of the above relaxes any other validation — a file that fails to parse,
  is not a JSON object, or fails size/header checks is still refused outright. Only a version
  marker's value has stopped being refusal-worthy.

**Also ratified, out of scope for this ticket** (`IO_MIGRATION_SPEC.md` §3): the manifest's
`MigrationStep::migrations` element type is now `MigrationEntry{function, name, description,
bIndependentlySelectable=false}`, not a bare function pointer, with a required paired
self-consistency test for any `bIndependentlySelectable=true` declaration. This ticket does not
touch the manifest shape — it stays correct and compatible whether the manifest has zero entries
(today's actual state) or many; the shape change only matters once real migration files exist.

## Root problem
`src/io/MapImporter_IO.cpp:65-173` (`ParseSanmapJsonText`) calls `RunSanmapMigrations` as the
literal first thing that touches the document (line 80: `if (!RunSanmapMigrations(document,
result)) return false;`). A `false` return aborts the ENTIRE import — before any of the ~20
unconditional top-level readers below it run (markers/props/decals/atmosphere/symmetry/etc.,
which don't depend on migration at all). `Sanmap_MigrationRunner_IO.cpp` returns `false` in two
cases: no version marker present at all, or a version newer than `kCurrentSanGenVersion`. Both are
currently deliberate, ratified law (soon-superseded per §0) — but the Format Expert confirmed by
reading real files that the "no version marker" case is not a hypothetical edge case:
`World_Domination.sanmap` (a real shipped map, `E:\Games\Steam\steamapps\common\Sanctuary
Shattered Sun Demo\...\Maps\World_Domination\`) has a genuine v1 `mapGeneratorData.GeoLayers[]`
shape (`Fractal`/`UseImage`/`Erosion{...}`) with **no `MapGeneratorDataVersion` field anywhere in
the file** — this exact file hard-refuses today. A second real case, `Pandemonium Isthmus.sanmap`,
has 8 sibling crash/backup files a designer would plausibly try to open. Human-reported symptom:
"I tried to open a sanmap file and program refused to open anything unidentifiable."

## Ruled by this ticket (three expert consults, reconciled)

**1. `RunSanmapMigrations` becomes non-fallible (IO Architecture Expert).** Drop the `bool`
return:
```cpp
void RunSanmapMigrations(nlohmann::json& document, MapImportResult& result);
```
`ParseSanmapJsonText`'s line 80 changes from a branching `if` to a bare call — migration becomes
something the document always survives; failure-to-resolve-version becomes a `result.Warn()`
inside the runner, never a caller-visible gate.

**2. Absent version marker — do NOT blind-walk the migration chain as if it were v1 (IO
Architecture Expert + Format Expert, converged independently).** Two reasons, both evidence-backed:
- The migration *transform* primitives (`RenameKey`/`MoveKey`/`WrapScalarAsVector`,
  `JsonPrimitives_IO.h`) are not miss-safe like the `Read*` accessors — they actively rewrite
  keys. Running them against a document of unconfirmed origin risks silently corrupting data a
  plain reader would have recovered correctly (or honestly missed) on its own.
- Format Expert's real-file evidence: an unversioned document (`World_Domination.sanmap`) is
  already correctly recovered TODAY by two pre-existing, working, unconditional code paths —
  (a) the ~20 top-level `Read*Json` calls in `ParseSanmapJsonText` (markers/props/decals/etc,
  format-stable across every real file checked), and (b) the existing legacy-block readers
  (`ReadGeometryJson`/`ReadWaterJson`/`ReadStrataSettingsJson`, `MapImporter_IO.cpp:168-171`),
  which are gated only on `document.contains("mapGeneratorData")` — NOT on version — and already
  tolerate the confirmed-real v1 shape without any code change. Both keep working once the early
  `return false` is removed; neither needs a migration chain to fire.

  Concrete behavior: on absent version, `RunSanmapMigrations` logs a loud warning ("no
  SanGenVersion or legacy version field found; skipping migration, recovering via direct field
  match only") and resolves to a **zero-iteration** state (equivalent to `resolvedVersion =
  kCurrentSanGenVersion`) — the existing forward-walk loop then naturally does nothing, no new
  branch needed. Control falls through to every reader exactly as it does today for a
  current-version document.

**3. Newer-than-current version — same treatment, plus one flagged consequence (IO Architecture
Expert).** Remove the early `return false` at `Sanmap_MigrationRunner_IO.cpp:43-48`; the existing
loop (`for (sourceVersion = resolvedVersion; sourceVersion < kCurrentSanGenVersion; ...)`) already
naturally zero-iterates when `resolvedVersion > kCurrentSanGenVersion` — no other code change
needed there. Log a loud warning ("this map was saved by a newer SanGen (SanGenVersion N);
recovering best-effort — fields this build does not recognize are preserved, not applied"). All
readers then run against the document as-is. **Flagged consequence for the acceptance test:**
`MapExporter_Recipe_IO.cpp:86` always writes `document["SanGenVersion"] = kCurrentSanGenVersion`
on re-export — re-saving a newer-than-supported document silently downgrades its version stamp to
this build's own. Correct under the new best-effort philosophy (this build genuinely doesn't
understand the newer shape and shouldn't claim it still does), but must be an explicit, asserted
acceptance-test case, not a silent side effect discovered later.

**4. The Unknown-Import bag — exact shape (IO Architecture Expert).** Cannot live on
`Params::MapRecipe`: confirmed by grep, `src/params/` has zero `nlohmann::json` includes anywhere
in the tree, and Constitution §1's `IO → {DATA, PARAMS}` one-directional dependency table forbids
pulling `nlohmann/json.hpp` upward into PARAMS. Cannot live on `MapImportResult` either — its one
real consumer (`src/ui/FilesTab_Actions_UI.cpp:35-36`) is a local, transient, discarded value that
doesn't survive to a later export call, and it would force a JSON include into every UI
translation unit touching the Files tab.

New type, new file `src/io/UnknownImportBag_IO.h`:
```cpp
struct UnknownImportBag {
    nlohmann::json unknownTopLevelKeys = nlohmann::json::object();
};
```
Threaded exactly like the existing `Data::MapFields* outFields` pattern — a nullable out-param on
`MapImporter::LoadSanmap`/`ParseSanmapJsonText`, and a nullable in-param on
`MapExporter::BuildSanmapJsonText` (`MapExporter_Recipe_IO.cpp`) — owned by whatever caller
already threads `recipe` across the load-edit-save session (`FilesTab_Actions_UI.cpp:28,58`'s
`RunOpenSanmap`/`RunRecipeExport`).

**Population rule.** After `RunSanmapMigrations` returns (so any legacy key a migration step
deliberately deleted via `DeleteKeyIfPresent`, §3's "legacy top-level keys to delete," is already
physically gone from `document` by construction — no extra bookkeeping needed to distinguish
"deliberately deleted" from "genuinely unknown," pure ordering guarantees it), for every top-level
key in `document` **not** present in a maintained "known top-level keys" allowlist, copy it into
`unknownTopLevelKeys`. The allowlist is the union of:
- (a) every key any `Read*Json` call in `ParseSanmapJsonText` actually reads (ground truth: that
  function's own call list),
- (b) keys `BuildSanmapJsonText` writes but deliberately never reads back — **confirmed today as a
  real, non-empty set**: `fileVersion`, `mapVersion`, `name`, `credits`, `length`,
  `heightmapResolution`, `hasWater`, `waterLevel`, `waterDepth`, `shader`, `heightTransition`,
  `fadeDistance`, `fadeStartDistance` (`MapExporter_Recipe_IO.cpp:81-103`). **Do not silently
  treat this as acceptable** — several of these (`name`, `credits`, `waterLevel`, `waterDepth`)
  look like genuine designer-authored data that SHOULD round-trip, not deliberate write-only
  constants. This ticket allowlists them (so they aren't double-bagged/duplicated — the exporter
  already writes them every time regardless), but does NOT add importers for them — that's
  `STEP25_ExportOnlyFieldAudit_IO.md`'s job, a separate, parallel-safe ticket. Note this
  distinction explicitly in a code comment at the allowlist site so a future reader doesn't
  mistake "allowlisted" for "intentionally write-only forever."
- (c) `SanGenVersion` and `mapGeneratorData` themselves — runner-owned / partially-consumed by the
  existing legacy readers, never wholesale unknown.

Add a paired test (`KnownTopLevelSanmapKeys_IO_Test.cpp` or fold into `MapImporter_IO_Test.cpp`)
asserting every key `BuildSanmapJsonText` writes is present in the allowlist — a future coder
adding a new top-level export key without updating the allowlist should fail loud in CI, not
silently mis-bag it.

**5. `mapGeneratorData` — do NOT bag it wholesale (Format Expert, real-file-confirmed collision
risk).** Format Expert confirmed `mapGeneratorData` is a genuinely overloaded key name —
`SANMAP_FORMAT_SPEC` Correction 1 already documents "two mutually incompatible dialects squatting
on the same key" as prior real history for this exact key. Since it already has a dedicated
(partial) reader path (the existing legacy-block readers), it is in allowlist (c) above and never
gets top-level-bagged. **Explicit, named limitation, out of scope for this ticket:** any field
*inside* `mapGeneratorData` that the legacy readers don't recognize (e.g. the real `ImagePath`
absolute-filesystem-path field Format Expert found at `World_Domination.sanmap` line 3251) is lost
today and stays lost after this ticket — nested/recursive Unknown-Import capture is a real,
harder, separate future extension (per-array-element merge/collision semantics are unresolved),
not silently half-built here. State this limitation in the work-order's completion notes so it
isn't mistaken for full coverage. If a future extension does add nested capture, Format Expert's
warning applies: an absolute local filesystem path like `ImagePath` must be preserved as inert
data only, never re-resolved or treated as a live path.

**6. Exporter re-merge (IO Architecture Expert).** Single new step in
`MapExporter::BuildSanmapJsonText`, inserted **last**, immediately before the final
`document.dump(indent)` — after every other `document[...] = ...` write in the function:
```cpp
if (unknownData != nullptr)
    for (const auto& [key, value] : unknownData->unknownTopLevelKeys.items())
        if (!document.contains(key)) document[key] = value;
```
Collision rule: exporter's own known-domain writers always win; the bag only fills gaps. This is
total and order-independent because it runs strictly last. Self-healing is automatic and needs no
explicit prune step: once a real writer starts setting `document[key]`, the merge's `if` stops
applying the bag's stale copy; once a real reader exists for that key, allowlist (a) grows and the
capture step (point 4) stops bagging it at all.

## Target files
- `src/io/Sanmap_MigrationRunner_IO.h`/`.cpp` — non-fallible signature (ruling 1), remove both
  early `return false` branches (rulings 2/3), add the Unknown-Import capture step (ruling 4/5)
  after the forward-walk loop.
- `src/io/UnknownImportBag_IO.h` — new file, the `UnknownImportBag` type (ruling 4).
- `src/io/MapImporter_IO.h`/`.cpp` — drop the `if` gate on `RunSanmapMigrations`'s result (ruling
  1); add nullable `UnknownImportBag*` out-param to `LoadSanmap`/`ParseSanmapJsonText`.
- `src/io/MapExporter_IO.h`, `MapExporter_Recipe_IO.cpp` — add nullable `const UnknownImportBag*`
  in-param to `BuildSanmapJsonText`; add the re-merge step (ruling 6) as the last write before
  `dump()`.
- `src/ui/FilesTab_Actions_UI.cpp` — thread a `UnknownImportBag` instance through
  `RunOpenSanmap`/`RunRecipeExport` alongside the existing `recipe` reference, per the
  `Data::MapFields*` precedent already established at this call site.
- `src/io/Sanmap_MigrationRunner_IO_Test.cpp` — invert `CheckNoVersionMarkerRefuses` and
  `CheckNewerVersionRefuses` (currently assert `!bAccepted`) into best-effort-accepts-with-warning
  assertions. Do not delete them — rewrite in place, same names if the assertions still describe
  what they test, renamed if the "Refuses" framing is now misleading.
- New test file (or extend `Sanmap_MigrationRunner_IO_Test.cpp`) — a **synthetic, in-repo JSON
  fixture** replicating the Format-Expert-confirmed real shape (no version marker, a
  `mapGeneratorData.GeoLayers[].{Fractal,UseImage,Erosion{...}}` block, top-level `armies`/
  `markers` present) — do NOT take a test dependency on the external game-install path
  (`E:\Games\Steam\...`); build the fixture as a literal string/JSON object in the test file
  itself, sized down to whatever subset actually exercises the behavior.
- `src/io/MapImporter_IO_Test.cpp` — the new `KnownTopLevelSanmapKeys_IO_Test` allowlist-coverage
  test (ruling 4).
- `.claude/agents/sangen-coder.md` — IO Architecture Expert flagged this file should be checked
  for any line citing the old "flat refusal" behavior as binding once ARCH ratifies; grep for
  "refus" and update any stale citation to match the new ratified §0 text verbatim.

## Explicit out-of-scope
- **Nested/recursive Unknown-Import capture** — top-level keys only (ruling 5). A named future
  extension, not this ticket.
- **`STEP25`'s 13 write-only export fields** — allowlisted here so they don't get wrongly bagged,
  but NOT wired with new importers here (ruling 4b). Separate ticket, can run in parallel, does
  not touch any file this ticket touches except `MapImporter_Recipe_IO.cpp`/
  `MapImporter_IO_Test.cpp` for new reads — coordinate ordering if both land close together
  (STEP25 adding readers makes those 13 keys move from allowlist-category-(b) to
  allowlist-category-(a); harmless either order, but don't let both tickets fight over the same
  allowlist comment block simultaneously).
- **A true legacy-fingerprint detection step** (sniffing `Fractal`/`UseImage` to actively RELOCATE
  v1-shaped fields into current-shape sections, vs. today's "leave what the existing legacy
  readers already handle, bag the rest") — explicitly deferred by the ARCH Expert's own drafted
  text: "gets its own explicitly-named legacy-detection step added later — a deliberate, visible
  addition." Not this ticket.
- **Rotation identity-quaternion export defect, `diffuseRemapColor` never-round-trips,
  `importedMaskMode`/`bEnabled` losing their home** — all pre-existing, already-documented gaps
  (`SANMAP_FORMAT_SPEC` "Known gaps," Correction 13) unrelated to the refusal-behavior redesign.
  Separate future tickets.
- **File-size/parse/malformed-JSON refusal** — untouched. Only version-marker-triggered refusal
  changes; a file that fails to parse, isn't a JSON object, or exceeds the size cap is still
  refused outright (ARCH Expert's own drafted text makes this explicit).

## Layer & accuracy class
IO/BRIDGE only. No PARAMS change (ruling 4's layer constraint is itself the point). Accuracy
class: Exact (best-effort recovery is deterministic given the same input document).

## Backend policy
N/A — IO-layer, not compute.

## ARCH rules invoked
- Constitution §6 / `IO_MIGRATION_SPEC.md` §6, as amended per §0 — **must be ratified before
  dispatch**.
- Constitution §1 (layer dependency table) — the Unknown-Import bag's IO-only homing (ruling 4).
- Constitution §7 — this work-order cannot be schema-valid ("ARCH rules invoked" must exist)
  until §0 lands.

## Acceptance test
1. A document with a top-level `SanGenVersion` newer than `kCurrentSanGenVersion` imports
   successfully (no refusal), logs a loud warning, recovers every field a current-shape document
   would, and any genuinely-unrecognized top-level key lands in the Unknown-Import bag.
2. A document with NO version marker at all (synthetic fixture matching the real
   `World_Domination.sanmap` shape, ruling 2/target-files) imports successfully, logs a loud
   warning, recovers everything the existing unconditional + legacy-block readers already handle
   today, and does NOT have the migration transform chain applied to it (assert no `RenameKey`/
   `MoveKey`-driven relocation happened — e.g. a field that a real migration step would have moved
   stays in its original legacy location, proving the chain was skipped, not silently run).
3. Re-exporting either case 1 or case 2's loaded recipe writes `SanGenVersion =
   kCurrentSanGenVersion` (the documented downgrade-on-resave consequence, ruling 3) — assert this
   explicitly, not just that export succeeds.
4. A synthetic document with one genuinely-unrecognized top-level key (not in the allowlist, not a
   legacy key any migration would delete) round-trips: import captures it into the bag, export
   re-emits it byte-for-byte under the same key, UNLESS a known-domain writer already wrote that
   key (collision case: known writer wins, bag copy is dropped).
5. A document where a migration step deliberately deletes a legacy key (any existing V-to-V+1 step
   with a `legacyKeysToDelete` entry, or a synthetic one if none is shipped yet) confirms that
   deleted key does NOT appear in the Unknown-Import bag and does NOT reappear on export.
6. `KnownTopLevelSanmapKeys_IO_Test`: every key `BuildSanmapJsonText` writes is present in the
   allowlist (fails loud if a future coder adds an export key without updating it).
7. Both rewritten `Sanmap_MigrationRunner_IO_Test.cpp` cases (formerly `CheckNoVersionMarkerRefuses`/
   `CheckNewerVersionRefuses`) now assert best-effort-accepts-with-warning, not refusal.
8. Full `SanGenV2` build stays clean; every existing test continues to pass, including
   `MapImporter_IO_Test.cpp` and `MapExporter_IO_Test.cpp`.

## Completion-notes requirement
State plainly, in the coder's completion report, the two named limitations this ticket
deliberately does not close: (a) nested/recursive unknown-field capture inside a known top-level
key (e.g. `mapGeneratorData`'s own unrecognized sub-fields) is still lost, and (b) the 13
`STEP25`-flagged export-only fields are allowlisted but not yet import-wired. Both are intentional
scope cuts, not oversights — but the human's "no data loss unless deprecated" goal isn't fully met
until both close.
