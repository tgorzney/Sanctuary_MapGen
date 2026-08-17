# IO_MIGRATION_SPEC — the `.sanmap` schema version migration system

Ratifies the migration architecture the human proposed alongside work-order `SPEC-4`
(`work_orders/SPEC-4_SanmapSchemaV3_DOCS.md`), evaluated and formalized here as binding
law. This is the mechanism that makes `SanGenVersion` (`SANMAP_FORMAT_SPEC` Correction 1)
an actually-enforced gate instead of a written-never-read constant — the exact defect
`SPEC-4` found at `MapExporter_IO.h:75` / `MapImporter_Recipe_IO.cpp`. ARCH §1.7 is the
one-paragraph law pointer; this page is the full contract.

**Scope note:** this spec defines the *mechanism* every future `SanGenVersion` bump uses.
It does not enumerate the real V2→V3 migration file list — that is coder-tier work for a
future work-order once `SPEC-4`'s schema is implemented (§9 is illustrative only, and
`SPEC-4` itself is explicit: "any code change to `src/io/*` or `src/params/*`" is out of
its own scope).

---

## 1. The migration unit — one file per (domain, version-step)

- **Naming: `<Domain>_Migrate_V<N>_IO.h/.cpp`.** Migrates a V*N*-shaped JSON fragment
  forward to V*N*+1 shape. `N` is always the **source** version of the step — never a
  range, never a direct N→M jumper. Direct jumpers are forbidden: they combinatorially
  explode (one file per *pair* of versions instead of per *step*) and every one of them
  has to be rewritten on every future bump, which is precisely the maintenance burden
  this design exists to avoid.
- **This is an instance of the ARCH §1.5 `Type_Aspect_LAYER` split-file pattern already
  in use** (`MapImporter_Recipe_IO.cpp`, `AssetAtlasCache_DiskLoad_IO.cpp`): `Domain` is
  the Type, `Migrate_V<N>` is the Aspect, `_IO` is the layer. No new naming exception is
  needed — the existing law already covers this shape.
- **Domain = the SanGen-owned top-level PascalCase section (ARCH §1.6) the migration
  principally produces** — e.g. `HeightmapStack_Migrate_V2_IO` carries the V2
  `mapGeneratorData.GeoLayers` shape forward into the V3 `HeightmapStack` section. A
  migration that does not map onto one section (e.g. a format-native-collection merge,
  `SANMAP_FORMAT_SPEC` Correction 11) is named after the concept it performs, not forced
  into a section name that does not fit (worked example, §9: `EntityCollections_Migrate_V2_IO`).
- **Signature: one free function per file**, operating on the **whole** parsed document
  in place — `void <Domain>_Migrate_V<N>(nlohmann::json& document);`. Declared in the
  `.h`, defined in the `.cpp`. Why whole-document, not a fragment: §2.
- **Paired test: `<Domain>_Migrate_V<N>_IO_Test.cpp`** — a literal OLD-shape JSON fixture
  (a hand-built or real V*N* document fragment) asserting the exact NEW shape after
  calling the migration function alone. Tests one migration against one fixture with one
  exact assertion; it is not a round-trip test and not a test of the runner.
- **Append-only discipline.** Once a migration file's test is green *and* the version
  step it belongs to has shipped (appeared in a build the human accepted), the file is
  **frozen** — never edited again. A defect discovered later in an already-shipped
  migration is fixed by a **new, later** migration step, never by rewriting history: any
  document that already walked through step N→N+1 must keep meaning exactly what that
  step said it meant at the time. This is what "append-only" guarantees in practice, not
  just in intent.
- **File location: flat in `src/io/`**, no `migrations/` subfolder. This matches the
  layer's existing convention (`AssetAtlasCache_*`, `SanpackReader_*`, `MapImporter_*`
  all sit flat, distinguished by name prefix, not by nesting) and avoids inventing an
  unratified grouping tier the pack elsewhere explicitly declines to add without cause
  (`LAYER_SYSTEM_SPEC`'s rejection of a nesting tier is the same instinct). If file count
  ever makes this genuinely unmanageable, that is its own future ARCH ruling — not a
  default assumed here.

## 2. Cross-domain compensation — the runner mutates the WHOLE document

A version step routinely needs one domain's migration to read fields that still live in
**another** domain's old location. The load-bearing example: `SlopeDefaults_Migrate_V2`
must read each stratum's V2 per-stratum slope fields — still present in the V2 document,
nowhere else — to synthesize a sane global `SlopeDefaults` default (`MASKING_SPEC` §1.7,
`SANMAP_FORMAT_SPEC` Correction 5). It cannot operate on a `SlopeDefaults`-shaped slice,
because in V2 no such slice exists.

**Therefore every migration function receives the entire `nlohmann::json` document**, not
an isolated fragment — deliberately, not as a shortcut. Two rules keep that wide
capability disciplined:

1. **A migration function writes only within the section(s) implied by its own domain
   name** — its own top-level key, plus any format-native collection member fields its
   correction explicitly names (e.g. `armyColor`/`alias` inside `armies[key]`). It may
   **read** anywhere in the document.
2. **A migration function never re-reads a key an earlier migration in the same step
   already deleted.** Ordering inside a step's manifest entry is therefore load-bearing
   law, not a convenience — see §3.

## 3. The manifest — the one file touched on every version bump

**`Sanmap_MigrationManifest_IO.h/.cpp`.** One ordered table: for each source version `N`
present, an **ordered list** of the migration functions that fire carrying a document
from `N` to `N+1`, plus (optionally) the legacy top-level keys to delete once every
migration in that step has run — deletion uses `DeleteKeyIfPresent` (§5), and it happens
**after** every migration that still needs to read the old location has executed.

- **Sparse by construction.** Most version steps touch a handful of domains, not all of
  them. The table has exactly as many entries per step as that step needs — never a fixed
  N×M grid with mostly-empty cells.
- **This is the only file a coder edits to wire a new version step.** The migration
  files, their tests, and any new primitive one of them needs are pure additions — new
  files, zero edits to anything pre-existing except this one manifest.
- **No self-registration / auto-discovery.** A migration function is wired in by one
  explicit, visible line in this file — no static-initializer registry, no macro that
  appends to a global list at load time. Static-init order across translation units is
  undefined-order risk (a migration silently absent from a build because its registration
  object never got linked in is a genuinely hard failure to diagnose), and an
  auto-discovered list is exactly the kind of implicit magic the pack's AI-legibility
  principle exists to forbid. One line a coder — or a reviewing AI — can read top to
  bottom beats a mechanism that requires trusting the linker.
- **`kCurrentSanGenVersion` lives here, as one constant** — `highest manifest step + 1`.
  The exporter (`MapExporter_IO`) writes `SanGenVersion` from this **same** constant,
  never a second independently-maintained number. This closes the exact defect the whole
  subsystem exists to fix: today `mapGeneratorDataVersion` is a write-only literal at
  `MapExporter_IO.h:75` with no reader anywhere. One source of truth for "what version
  does this build produce" removes the class of bug where the writer and the (future)
  reader silently drift apart.

## 4. The runner

**`Sanmap_MigrationRunner_IO.h/.cpp`.** Owns:

1. **Version resolution.** Reads `SanGenVersion` if present. If absent, falls back to the
   historical predecessor field `MapGeneratorDataVersion`/`mapGeneratorDataVersion`
   (`SanGenVersion` "replaces" it, `SANMAP_FORMAT_SPEC` Correction 1) and **logs that it
   did so** — a fallback is never silent (Constitution §6). If **neither** field is
   present, the runner refuses outright (§6) rather than guessing version 1.
2. **Walking forward.** From the resolved version to `kCurrentSanGenVersion`, for each
   step: run that step's migration list in the manifest's declared order, delete that
   step's declared legacy keys, then set `document["SanGenVersion"] = N + 1` before
   moving to the next step. **Writing the version field is the runner's job**, never an
   individual migration's — every step leaves the document honestly stamped, whether or
   not further steps follow.
3. **Current-version passthrough.** A document already at `kCurrentSanGenVersion` is a
   no-op: the runner still performs version resolution and the refusal check (§6), but
   calls no migration.
4. **Runs before any domain block reader.** `MapImporter` invokes the runner on the freshly
   parsed JSON **before** any `Read<Domain>Json` block reader executes. Consequence: block
   readers (`ReadGeometryJson`, `ReadLayerStackJson`, …) are written against exactly the
   **current** schema shape and **never contain a version branch of their own** — all
   historical-shape knowledge lives in the migration files, in one place, and every reader
   downstream always faces the one shape the current `SANMAP_FORMAT_SPEC` defines.
5. **Return contract mirrors the existing pattern already live in `src/io/`**
   (`MapImportResult{ bSucceeded, warningCount, Log()/Warn() }`, `MapImporter_IO.h`) — not
   an exception, a result the caller inspects. IO already validates-then-defaults-then-logs
   (Constitution §6); the runner is one more instance of that pattern, not a new one. The
   exact reuse-vs-own-type call is coder-tier (§8).

## 5. The shared primitive toolkit — `JsonPrimitives_IO.h`

**Homing confirmed: IO/BRIDGE-scoped only, one file, never a whole-program `Utils.h`.**
JSON manipulation happens nowhere else in the tree — `UI` edits `PARAMS` structs in
memory and never touches a document (ARCH §3.2, "UI never simulates"; the DATA/PARAMS
split means there is no document for UI to reach for), and no other layer may depend on
`IO` at all (ARCH §3.1's dependency table). A JSON toolkit therefore has exactly one
honest home. The reasoning is the same the pack already uses twice:
- `WidgetHelpers_UI.h` — "the shared, imgui-free core of the universal widget library...
  one implementation, one look, DRY" — scoped to its own layer, not a cross-layer grab-bag.
- `src/math/`'s one-file-per-concept convention (`Morton_MATH.h`, `Reciprocal_MATH.h`) —
  small, pure, header-only, no `MathUtils.h`.

**Filename ratified: `JsonPrimitives_IO.h`** (header-only, `inline` functions — no
`.cpp`, matching the same header-only-pure-function precedent both citations above
already establish). Paired `JsonPrimitives_IO_Test.cpp`.

One deliberate change from the human's literal suggestion: **"Primitives," not
"Helpers."** Every function here is a base-case JSON transform every migration composes
from (rename / move / wrap / default / delete / typed-read) — "primitive" states what
these functions *are* (the smallest transform unit a migration is built from); "helper"
is the word `WidgetHelpers_UI.h` already uses for a different kind of thing (interaction/
clamping math for widgets). Keeping the two words distinct keeps a search for precedent
from conflating the two files' contents. The suffix law itself (`_IO`) was already
correct in the proposal and is unchanged.

**Contents:**

- **Relocated (fixes the live mis-homing defect the human flagged):** `ReadJsonFloat`,
  `ReadJsonInteger`, `ReadJsonBoolean`, `ReadJsonEnumeration` move verbatim out of
  `MapImporter_Recipe_IO.h` — today's mis-home: one domain's header, cross-included by
  `MapImporter_IO.cpp`, `MapImporter_Layers_IO.cpp`, and `MapImporter_Rules_IO.cpp` purely
  to reach a category of function that has nothing to do with the Recipe domain
  specifically. **`ReadJsonText` moves too**, though the launching task named only four of
  the five — it lives in the exact same "typed accessors every reader is built from" block
  for the exact same reason; leaving it behind recreates the identical defect for the next
  domain that needs a string field.
- **New transform primitives**, for migrations to compose (names and behaviors below are
  binding; exact signature ergonomics are the coder work-order's call):
  - `RenameKey(json& parent, const char* oldKey, const char* newKey)` — moves a value to a
    new key in the **same** object; no-op if `oldKey` is absent.
  - `MoveKey(json& sourceParent, const char* sourceKey, json& destinationParent, const char* destinationKey)`
    — moves a value **across** objects; the cross-domain primitive (pulling a field out of
    the legacy `mapGeneratorData` blob into a new top-level section); no-op if `sourceKey`
    is absent.
  - `WrapScalarAsVector(json& parent, const char* key)` — replaces a scalar at `key` with a
    single-element array containing it; the exact tool for SPEC-4 Correction 7's global→
    per-layer cardinality changes (`HydroMultiplier`/`ReclaimDensity`/`MexDensity`/
    `SpawnPointCount`: one old global scalar copied onto every `MarkerRule` instance).
  - `DefaultIfMissing(json& parent, const char* key, json defaultValue)` — sets `key` only
    if absent; never overwrites a present value.
  - `DeleteKeyIfPresent(json& parent, const char* key)` — erases `key` if present, no-op
    otherwise. The primitive the manifest's legacy-key cleanup (§3) is built from.
- **All transform primitives are total and idempotent** (a second call is always safe) —
  the same no-crash, no-surprise contract the existing `ReadJson*` accessors already keep
  (Constitution §6), extended from "read" to "transform."

## 6. Refusal law (Constitution §6, made concrete for this subsystem)

- `SanGenVersion` (or its resolved legacy predecessor, §4.1) **newer** than
  `kCurrentSanGenVersion` → the runner refuses the import outright, with a clear logged
  reason ("this map was saved by a newer SanGen and cannot be opened"). Never a
  best-effort partial parse, never silently dropping fields it does not recognize.
- **No version marker of any kind** (neither `SanGenVersion` nor its legacy predecessor
  field present) → refuse; do not guess a starting version. A genuinely un-versioned
  true-v1-dialect document, if one is ever found in the wild and needs support, gets its
  own explicitly-named legacy-detection step added to the runner's version-resolution
  logic (sniffing a v1-only fingerprint field, e.g. `Fractal`/`UseImage`) — a deliberate,
  visible addition, never a silent default-to-1.
- These two rules are this subsystem's entire discharge of Constitution §6 ("a version
  mismatch must not silently degrade") for the `.sanmap` importer.

## 7. Worked example (V2 → V3, illustrative — NOT a coder-tier prescription)

Shows the shape the manifest entry for `SPEC-4`'s own version bump would take, once a
future work-order builds it. This is an example of the **pattern**, not a ratified file
list — the exact set of migration files, their ordering, and whether any `SPEC-4`
correction needs splitting across more than one file is decided by that future
work-order, informed by this spec plus the ratified `SANMAP_FORMAT_SPEC`.

```
Step 2 -> 3 (illustrative):
  GeneralMapSettings_Migrate_V2_IO     // Correction 2: pulls Seed / ScaleFeaturesToMapSize /
                                        // TerrainMinHeight / WorldUnitsPerCell out of mapGeneratorData
  HeightmapStack_Migrate_V2_IO         // Correction 3: GeoLayers -> HeightmapStack, folds in
                                        // SimulationGrouping
  Symmetry_Migrate_V2_IO               // Correction 4: global symmetry fields -> Symmetry
  SlopeDefaults_Migrate_V2_IO          // Correction 5: READS each stratum's old per-stratum slope
                                        // fields (cross-domain read; does not delete them) to
                                        // synthesize SlopeDefaults; sets bSlopeUseGlobal = false
                                        // wherever a stratum's old values disagree with the
                                        // synthesized default, true otherwise
  Flow_Migrate_V2_IO                   // Correction 6: reserves Flow (FlowMapColor lands here)
  Accumulation_Migrate_V2_IO           // Correction 6: reserves Accumulation (empty)
  MarkersStack_Migrate_V2_IO           // Correction 7: wraps MarkerRule[]; WrapScalarAsVector's
                                        // the four former global scalars onto every rule
  PropsStack_Migrate_V2_IO             // Correction 7
  DecalsStack_Migrate_V2_IO            // Correction 7
  UnitsStack_Migrate_V2_IO             // Correction 7 (UnitRule already exported ad hoc; wraps it)
  DetailNormal_Migrate_V2_IO           // Correction 8: reserves DetailNormalMapSize
  EntityCollections_Migrate_V2_IO      // Correction 11: armies[]/markers[] gain armyColor/alias;
                                        // deletes the old global Aliases block

Legacy keys deleted after the above (this step's manifest entry):
  mapGeneratorData, MapGeneratorDataVersion, mapGeneratorDataVersion
```

`SlopeDefaults_Migrate_V2_IO` is the load-bearing demonstration of §2's cross-domain
rule: the one migration in this step whose correctness depends on reading fields that
live under a *different* domain's old location — and it must run before
`mapGeneratorData` is deleted, which is exactly why ordering inside a step is law, not
convenience.

## 8. What this spec does not decide (deliberately)

- The literal C++ result-type ergonomics of `Sanmap_MigrationRunner_IO` (whether it reuses
  `MapImportResult` verbatim or defines its own) — coder-tier, constrained only by §4.5's
  "mirrors the existing pattern" requirement.
- The real migration file list for V2→V3 — §7 is illustrative; the actual list is a future
  work-order's job, built against this spec plus the ratified `SANMAP_FORMAT_SPEC`.
- Whether a true pre-`mapGeneratorDataVersion` v1 dialect needs its own active migration
  step — flagged in §6, not designed here; no evidence yet that such a document needs
  active support rather than a clean refusal.
