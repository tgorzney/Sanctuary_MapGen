# DESIGN — `.santp` Footprint / Blueprint Ingestion (R1)

*Authored by the SanGen Format Expert, 2026-08-21. **Design only — no code, no work-order.**
Read-only against `src/**` and read-only against the game install.*

*Grounded against: `sangen_arch_pack/CONSTITUTION.md`, `ARCH_01_05_FileSizeCeilings.md`,
`ARCH_02_LayerDirectoryMap.md`, `ARCH_14_13_OpenItems.md` (item 1),
`ARCH_15_03_ExportOnlyLuaRatified.md`, `ARCH_15_08_ThirdPartyDependencyRuling.md`,
`sangen_arch_pack/specs/{GAMEDATA_LAYOUT_SPEC,UNIT_PROP_MARKER_DATA_SPEC,ASSET_LOADING_SPEC}.md`,
`work_orders/{STEP58_WorldFootprintSizeTable_IO,STEP62_ReclaimPropFilter_PARAMS,STEP64_GameInstallLocation_IO,STEP65_LuaSyntaxCheck_SYS}.md`,
and a **live read of the real Steam Demo install** at
`E:\Games\Steam\steamapps\common\Sanctuary Shattered Sun Demo\` (nothing written there).
`src/` inventory confirmed by direct grep this session.*

*Format template: `work_orders/DESIGN_MapScenarioIO_R1.md`. `ARCH_01_05_FileSizeCeilings.md`
governs **program code**; this document is not program code. Every source file §3 proposes is
sized in §3 and specified to land under the 100-line soft ceiling, one primary type per file.*

---

## 0. Why this document exists

`ARCH_14_13_OpenItems.md` item 1 is still open: *"Real footprint-size source: placeholder-per-domain
now (§14.3); who/when derives real mesh bounds is unscheduled."*

`STEP58_WorldFootprintSizeTable_IO.md` deliberately ships the **data shape plus a two-entry
hand-seeded lookup** and stops, because no Lua reader exists in `src/`. Re-confirmed by grep this
session: `lua_State`, `luaL_`, `LuaTable`, `LuaJIT`, `lua.h`, `sanprop` have **zero hits** anywhere
in `src/`. The five `santp` hits are all `blueprintPath` string literals in IO test fixtures
(`src/io/MapImporter_IO_Test.cpp:999,1021`, `src/io/MapImporter_PropsDecals_IO_Test.cpp:61,88`,
`src/io/MapExporter_BlueprintValidation_IO_Test.cpp:65`) — none parse a template. The only
Lua-adjacent production code is an **unbound function-pointer seam** for SupCom `_save.lua` import
(`src/ui/FilesTab_UI.h:37,82`; `src/ui/FilesTab_Actions_UI.cpp:49` logs *"No SupCom Lua importer is
bound to this build."*) — a different format with no implementation.

This design closes the parsing gap. It does **not** re-open STEP58's data shape, and it does **not**
redesign STEP62.

---

## 1. Verified ground truth (live install, this session)

Everything in this section was read from the real install. Where I could not verify something, §1.6
says so explicitly.

### 1.1 Where the data actually is

| What | Real path (relative to install root) | Count |
|---|---|---|
| Unit templates | `engine/LJ/lua/common/units/unitsTemplates/<tpId>/<tpId>.santp` | 295 dirs, 295 files |
| Engine prop templates | `engine/LJ/lua/common/props/propsTemplates/` | 3 dirs + flat `defaultWreckage.santp` |
| Marker templates | `engine/LJ/lua/common/markers/markerTemplates/` | 6 |
| Projectile templates | `engine/LJ/lua/common/projectiles/projectilesTemplates/` | 62 |
| Environment prop templates | `engine/Sanctuary_Data/Gamedata/Environment.sanpack.unzipped/Environment/<Biome>/Props/**` | 111 `.santp` |
| Loose prop templates | `engine/Sanctuary_Data/Gamedata/Props/`, `.../Gamedata/Pandemonium/` | 17 + 17 `.santp`, 34 `.sanprop` |

**Whole corpus: 546 files, 2,121,061 bytes total, 3,884 B average, 35,845 B largest.** This is a
~2 MB text ingest, not a 2 GB one — decisive for §4's cost argument.

### 1.2 Root-table census — there are FIVE dialects, not two

`UNIT_PROP_MARKER_DATA_SPEC.md` §"Two incompatible prop-template dialects" is correct as far as it
goes, but the *reader* sees five root table names across the `.santp`/`.sanprop` corpus:

| Root table | Count | Carries `footprint`? |
|---|---|---|
| `UnitTemplate` | 283¹ | ✅ yes |
| `propTemplate` (lowercase — Dialect A) | 145 | ✅ yes |
| `PropTemplate` (capital — Dialect B) | 4 | ✅ yes |
| `ProjectileTemplate` | 62 | ❌ **no** |
| `MarkerTemplate` | 6 | ❌ **no** |

¹ *The census regex matched 283 of 295 unit files. I inspected the 12 misses: every one is a real
`UnitTemplate` whose assignment is preceded by an `---@type UnitTemplate` annotation **and a
multi-line comment header** (the naval PLACEHOLDER / DEV-ONLY submarine blocks). All 295 are
`UnitTemplate`. **This is itself a load-bearing finding: the root assignment is NOT reliably on line
1 or 2**, so a line-anchored or regex-based reader is wrong from the start.*

Verified: the **only** files lacking `footprint` are the 62 `ProjectileTemplate` and 6
`MarkerTemplate` files. Every `UnitTemplate` / `propTemplate` / `PropTemplate` file in the install
carries `footprint = {x, y}`. The reader must therefore **filter by root table name, not by
extension** — projectiles and markers share the `.santp` extension and must be skipped, not
error-reported.

### 1.3 Confirmed field shapes

**Unit (`UnitTemplate`)** — e.g. `uca1001.santp`:
```lua
collisionInfo = { centerOffset = {x=0, y=0.6, z=0}, collisionSize = {x=0.8, y=1.25, z=0.8} },
footprint = { x = 1.2, y = 1.2 },
```
185 of 295 unit templates also carry `skirtSize` (structures). Out of scope here; noted so a later
ticket does not rediscover it.

**Dialect A (`propTemplate`, lowercase)** — e.g. `Environment/01_Highlands/Props/edbm0101/edbm0101.santp`:
```lua
collider = { center = {x,y,z}, size = {x,y,z} },
economy = { harvestTime = 3.0, harvest = { alloys = 5.0, plasma = 20.0 } },
footprint = { x = 0.696, y = 0.689 },
tags = { "HARVESTABLE", "FLAMMABLE", "KNOCKDOWNABLE" },
```

**Dialect B (`PropTemplate`, capital)** — `engine/LJ/lua/common/props/propsTemplates/exe0000/exe0000.santp`:
```lua
collisionInfo = { centerOffset = {...}, collisionSize = {...} },
economy = { harvestTime = 10, harvest = { alloys = 10, energy = 100 } },
footprint = { x = 1, y = 1 },
tags = { "HARVESTABLE" },
```

Both prop dialects share `footprint = {x, y}` **identically**, exactly as STEP58 asserts. The
`collider{center,size}` vs `collisionInfo{centerOffset,collisionSize}` split and the
`plasma` vs `energy` harvest-key split are both confirmed real. Harvest-key census: 194 files
mention `plasma`, 4 (the engine-lua Dialect B set) use `energy`.

### 1.4 Reclaim is real, and it is a **template** property

Confirmed live: 104 of 111 Environment `.santp` files carry `HARVESTABLE`. Dialect A carries it in a
top-level `tags = {...}` array **and, redundantly, as an `effects[].tag = "HARVESTABLE"` entry** —
`edbm0101` has both. A reader keying only off `effects[]` would miss Dialect B, which has `tags` and
no `effects` block at all. **Key off top-level `tags`.**

**Relationship to STEP62 — no redesign, no conflict.** STEP62 adds a single internal
`bool bReclaimable` to `Params::PropRule` / `Params::PropInstanceGroup` (wire key `"Reclaimable"`),
and its own Out-of-scope list already says: *"Auto-populating `bReclaimable` from a future blueprint
`tags`/`HARVESTABLE` import … is explicitly undecided here."* This design does not touch STEP62's
field, casing, or partition semantics. It only observes that **the same reader that surfaces
`footprint` surfaces `tags` for free**, and proposes a later, separate ticket (§7, ticket 92) that
*populates* STEP62's existing bool from ingested data. STEP62's simple bool remains correct: SanGen's
internal representation deliberately does not mirror the game's tag/yield shape 1:1.

### 1.5 ⚠️ Traps a naive reader will fall into

1. **`.sanprop` does not imply Lua.** The 17 `Gamedata/Props/*" - Copy".sanprop` files are **JSON,
   not Lua**, with a completely different schema — top-level `alloys`/`plasma`/`reclaimSpeed`/`lods`,
   and **no `footprint` at all**. These are evidently stale pre-migration editor duplicates. This is
   the *entire* explanation for the 17-file gap between the 546-file corpus and the 68 files that
   legitimately lack `footprint`. **A reader must sniff content, never trust the extension.**
2. **tpId collisions are real and already documented.** `UNIT_PROP_MARKER_DATA_SPEC.md:73-77` records
   9 Pandemonium files whose `general.tpId` ≠ filename, including `Cliff_03.sanprop` declaring
   `tpId = "Cliff_02"` — a genuine duplicate key. The `" - Copy"` files compound this by duplicating
   names. A tpId-keyed table **must detect and report collisions, not silently last-write-wins**.
   Note this directly contradicts `WorldFootprintSizeTable::SetFootprint`'s documented
   last-write-wins policy — see Open Question Q7.
3. **`03_Desert` has ZERO template files.** Its 15 Quixel-named prop folders
   (`Nature_Rock_vd5rfiq_4K_3d_ms/`) contain only `.sanmodel`/`.sanmaterial` — no `.santp`, no
   `.sanprop`. `DysonParts`, `Common`, `Skybox`, `Water`, `Winter` are likewise empty of templates.
   **Some props are permanently unreachable by ingestion** and will always fall back to a default.
   Per-biome `.santp` counts: `01_Highlands` 54, `10_WhiteDesert` 18, `Pandemonium` 17, `Dev` 12,
   `02_Evergreen` 6, `04_Baikal` 3, `09_Industrial` 1, all others 0.
4. **`UnitsTemplates.sanpack` is a 22-byte EMPTY zip** (magic `504b0506` = a bare
   End-Of-Central-Directory record, no entries). Unit templates ship **only** as loose Lua under
   `engine/LJ/lua/`. Any design that plans to read units out of a sanpack is reading nothing.
   By contrast `Environment.sanpack` is 1,751,449,309 bytes and genuinely holds the prop templates —
   *and* is simultaneously extracted to `Environment.sanpack.unzipped/Environment/` alongside a
   1.8 GB `Environment.zip`.
5. **Load-bearing shipped misspellings** (`UNIT_PROP_MARKER_DATA_SPEC.md:70-71`): `maxVerrtices`
   (double r), `positonOffset` (missing i). Never "correct" them in a key lookup.

### 1.6 What I could NOT verify

- **Whether the `.sanpack.unzipped/` trees ship with the game or are an artifact of prior manual
  extraction on this machine.** The presence of a sibling `Environment.zip` (1.8 GB) *inside*
  `Environment.sanpack.unzipped/` strongly suggests local extraction, not a shipped layout. **The
  design must not assume the unzipped tree exists** — hence §3's two-source resolution. This is the
  single biggest uncertainty in this document.
- **Whether `Gamedata/Props/` and `Gamedata/Pandemonium/` are duplicates** of
  `Environment/Pandemonium/Props/`. Filenames overlap exactly (`Cliff_01`, `CliffSpireCap_01`, …),
  but I did not byte-compare them. Treated as a collision source, not asserted as duplicates.
- **Whether a non-Demo (full release) install has the same layout.** Only the Demo is present.
- **Whether the game itself ever reads the loose `Gamedata/Props/*.sanprop` files.** Their JSON
  `" - Copy"` siblings suggest a stale working directory, but I did not trace engine load order.

---

## 2. The reader — does LuaJIT reuse hold?

**Verdict: the LuaJIT *vendoring* reuses cleanly and is a major simplification. The
`LuaSyntaxCheck_SYS` *contract* does not reuse at all, and must not be stretched to fit.**

### 2.1 What reuses

`STEP65_LuaSyntaxCheck_SYS.md` and `ARCH_15_08_ThirdPartyDependencyRuling.md` §15.8 already ratify:
- **LuaJIT specifically**, not vanilla Lua — dialect ruling, binding. The engine's tree is rooted at
  `LJ/lua/`; "LJ" is LuaJIT. Templates must be parsed by the grammar that actually loads them.
- **`SYS` as the home layer**, with `IO → SYS` formalized as a legal dependency direction (grounded
  in the pre-existing `src/io/AssetAtlasCache_IO.cpp` → `../sys/ThreadPool_SYS.h` precedent).
- **The CMake vendoring block**, `target_link_libraries(SanGenV2 PRIVATE <luajit>)`, and the
  "no `lua.h` type leaks into the header" discipline (mirroring how `SanpackReader_IO.h` hides
  `miniz`).

**If STEP65 lands first, this design inherits the entire third-party dependency, its build wiring,
and its layer ruling for free.** That is the simplification, and it holds. There is no argument for a
second Lua vendoring, and none for a different dialect.

### 2.2 What does NOT reuse — and precisely why

`LuaSyntaxCheck_SYS`'s contract is **compile-only and never-execute**, stated three times as a hard
safety property, with the killer acceptance test being that `CheckLuaSyntax("while true do end")`
returns in bounded time *because* nothing is ever run.

Footprint ingestion is the opposite operation. A `.santp` is a **Lua assignment statement**, not a
data literal. To obtain `footprint.x` you must **execute** the chunk and then read the resulting
global table. `luaL_loadbuffer` alone yields a compiled function and no values whatsoever.

So: **`CheckLuaSyntax` cannot be called to do this job, and must not be widened to do it.** Widening
it would delete the exact property STEP65's two proof tests exist to defend, for a caller that does
not even need syntax checking. This design proposes a **sibling SYS primitive** sharing the vendored
library and nothing else.

### 2.3 The sandbox, stated as a hard contract

Constitution §6 governs: pre-alpha game data is unreliable, and a modded install is fully untrusted
input. Executing it demands an explicit sandbox. Empirically, **every one of the 546 files in this
install is a pure literal table assignment** — I scanned the whole corpus for `require`, `function`,
`for`, `while`, `dofile`, `loadstring`, `setmetatable`, `math.`, `os.`, `io.`, and string
concatenation, and every single hit was inside a `--` comment. That is a strong argument the sandbox
will never be stressed in practice. **It is not an argument for omitting it** — a mod, a future
patch, or a corrupt file changes this instantly.

Binding constraints for the new primitive:
1. **Zero standard libraries opened.** No `luaL_openlibs`. Categorically no LuaJIT `ffi` — that
   alone is arbitrary native code execution. Inherits §15.8 constraint 2 verbatim.
2. **An instruction-count debug hook** (`lua_sethook` with `LUA_MASKCOUNT`) that aborts the chunk
   past a configured budget. This is what makes executing untrusted text safe; STEP65 achieved the
   same guarantee by never executing at all.
3. **A byte-size cap on the source** before it is ever handed to Lua (Constitution §6's
   "cap file size" applied literally), and a cap on total evaluated table nodes.
4. **`lua_pcall`, never `lua_call`** — a protected call so a runtime error is a returned failure,
   never a longjmp through C++ frames.
5. **A fresh `lua_State` per file**, closed on every exit path including early returns. No shared
   state means no cross-file contamination and no accumulated globals.
6. **Result is copied into an owned plain-C++ tree and the state is closed before returning.** No
   LuaJIT type, and no pointer into Lua-owned memory, outlives the call or crosses the header.

**This is a materially different safety posture from `LuaSyntaxCheck_SYS`, and it needs its own ARCH
sign-off — it is not covered by §15.8, which rules only on the never-execute file.** See Q2.

---

## 3. Scope boundary, layer plan, and file set

### 3.1 What this design covers — and what it does not

**IN SCOPE — "template ingestion":** locating game template files; evaluating them safely; extracting
`footprint`, `tpId`, root-table kind, `tags`, and the collision box; caching the result; producing a
populated `Io::WorldFootprintSizeTable`; degrading gracefully with no install.

**OUT OF SCOPE — the deferred "texture importer"** (`MEMORY.md` `project_texture_importer_scope`):
`.dds` decode, icon/thumbnail atlas building, prop thumbnail *rendering*, stratum texture ingestion,
`.sanmodel` mesh bounds. `ASSET_LOADING_SPEC.md` already owns that surface and
`src/io/AssetAtlasCache_*` already implements much of it. **The one thing the two share is the
game-root location, and that is exactly why §3.2 reuses STEP64 rather than inventing a second
mechanism.**

Note the deliberate narrowing versus `ARCH_14_13` item 1's original phrasing: item 1 says *"real
mesh-derived bounds."* This design derives footprint from the **shipped `footprint` field**, which is
authored ground truth and strictly better than mesh bounds for this purpose. Deriving bounds from
`.sanmodel` geometry is neither needed nor proposed. **Item 1 should be closed by this design's
tickets, with that substitution recorded** — flagged to ARCH in §6.

### 3.2 Install location — reuse STEP64, do not invent

`STEP64_GameInstallLocation_IO.md` already specifies `AppSettings::gameInstallRoot` (durable, JSON
round-tripped, seeded at startup / flushed at shutdown) and a pure
`Io::ValidateGameInstallRoot(candidateRoot) -> {bValid, reason}`. **This design consumes both
unchanged and adds no second install-location mechanism.**

⚠️ **But STEP64's validator, as written, rejects the real install.** It checks
`<root>/engine/LJ/lua` **and** `<root>/Sanctuary_Data/Maps`. On the real install the first exists;
the second **does not** — the real path is `<root>/engine/Sanctuary_Data/Maps`. Confirmed both
directions: `ls <root>/Sanctuary_Data` fails, `ls <root>/engine/Sanctuary_Data/Maps` lists 20+ map
folders. `work_orders/SESSION_HANDOFF_4.md:20` independently records the correct path
(`…\engine\Sanctuary_Data\Maps\Pandemonium Isthmus\…`). The error originates in
`DESIGN_MapScenarioIO_R1.md` §1's table and propagates into STEP64 §4 and its acceptance test.
This is a shipping bug in an already-drafted ticket, not a footprint-ingestion concern — §7 ticket 93
fixes it; §6 flags the doc correction.

This design additionally needs a **third** subpath, `<root>/engine/Sanctuary_Data/Gamedata`, since
that is where the Environment prop templates live. Whether that joins `ValidateGameInstallRoot`'s
required set or is checked separately by the ingester is Q4-adjacent; recommendation is
**separately** — the Map Scenario export leg has no business failing because Gamedata is missing.

### 3.3 Two sources, resolved in order

Because §1.6 leaves genuine doubt that the unzipped trees ship, the ingester must handle both:

| Source | Mechanism | Status in `src/` |
|---|---|---|
| Loose Lua tree (`engine/LJ/lua/common/**`) — units, engine props, markers | plain directory walk | **new** — no directory-walking scanner exists (the only `std::filesystem::directory_iterator` in `src/` is `MapImporter_IO.cpp:56`, unrelated) |
| Unzipped pack tree (`Gamedata/*.sanpack.unzipped/**`) | same directory walk | **new**, same code path |
| `Environment.sanpack` (1.75 GB zip) | **existing `Io::SanpackReader`** | **reuses cleanly** |

The `SanpackReader` reuse is excellent and worth stating plainly: `SanpackReader_IO.h`'s
`ExtractFiltered(filter, limits, outPayloads)` already does *exactly* what is wanted — memory-map
without copying, parse the central directory once, filter by extension, sort by local-header offset,
inflate in a single forward pass, and return per-entry `{bValid, rejectionReason}` instead of
throwing. Filtering `Environment.sanpack` to `.santp`/`.sanprop` pulls **~2 MB of payload out of
1.75 GB** in one sequential pass. This is `ASSET_LOADING_SPEC.md`'s single-pass rule honoured for
free, with zero new zip code.

**Resolution order:** prefer the unzipped tree when present (cheaper, no inflate); fall back to the
`.sanpack` via `SanpackReader`; if neither, that source contributes nothing and is logged. Never
both — double-ingesting is a guaranteed tpId collision storm.

### 3.4 Proposed file set

Naming per `ARCH_01_NamingLaw` (`Type_Aspect_LAYER`, layer tag as suffix, no abbreviations).
Line counts are budgets, all under the 100-line soft ceiling per `ARCH_01_05_FileSizeCeilings.md`,
one primary type per file.

| File | Layer | ~Lines | Primary type / responsibility |
|---|---|---|---|
| `src/sys/LuaTableValue_SYS.h` | SYS | ~70 | `Sys::LuaTableValue` — owned plain-C++ value tree (nil/boolean/number/text/array/table). No LuaJIT type. Header-only. |
| `src/sys/LuaTableEvaluate_SYS.h` | SYS | ~55 | `Sys::LuaTableEvaluateLimits`, `Sys::LuaTableEvaluateResult`, `EvaluateLuaTableSource(sourceText, rootTableName, limits)`. Opaque — no `lua.h`. |
| `src/sys/LuaTableEvaluate_SYS.cpp` | SYS | ~95 | The sandbox. The only TU besides `LuaSyntaxCheck_SYS.cpp` that includes LuaJIT. |
| `src/io/TemplateSourceScan_IO.h/.cpp` | IO | ~50/~90 | `Io::TemplateSourceScan` — resolves the three source roots off `gameInstallRoot`, walks trees / drives `SanpackReader`, yields `{logicalPath, sourceBytes}`. |
| `src/io/TemplateDialect_IO.h/.cpp` | IO | ~60/~95 | `Io::TemplateDialectKind` + `Io::TemplateRecord`. Detects root table by name; extracts `footprint`, `tpId`, `tags`, collision box across all five kinds; skips projectiles/markers by design. |
| `src/io/TemplateIngestCache_IO.h/.cpp` | IO | ~65/~95 | Fingerprinted disk cache — §4.2. |
| `src/io/TemplateIngest_IO.h/.cpp` | IO | ~55/~90 | Orchestrator + `Io::TemplateIngestReport`. The one entry point UI calls. |
| `src/io/WorldFootprintSizeTable_IO.h` | IO | *unchanged* | **STEP58's file, not edited** — see §5. |

Layer homes follow `ARCH_02_LayerDirectoryMap.md` without amendment. The ingested table is **IO, not
DATA** — Constitution §1 defines DATA as *"the struct-of-arrays computed output of generation"*, and
an asset-derived lookup is not generation output. STEP58 already made this call
(`WorldFootprintSizeTable` in `src/io/`, same category as `AssetAtlasCache_*`), and this design
preserves it. See Q3 for the richer-catalog variant.

---

## 4. Data flow, caching, and when ingestion runs

### 4.1 Flow

```
AppSettings::gameInstallRoot  (STEP64, durable)
   └─> TemplateIngest_IO::Ingest(gameInstallRoot, cacheDirectory, threadPool)
         ├─ TemplateIngestCache_IO: fingerprint hit?  ──yes──> load manifest ──┐
         │                                                                     │
         └─no─> TemplateSourceScan_IO   (dir walk | SanpackReader)             │
                  └─> LuaTableEvaluate_SYS  (per file, sandboxed, fan-out)     │
                        └─> TemplateDialect_IO  (root-table branch, extract)   │
                              └─> TemplateIngestCache_IO::SaveToDisk ──────────┤
                                                                               ▼
                                                         Io::WorldFootprintSizeTable (STEP58 shape)
                                                                               │
                                                    Application_AssetBridge_UI ─┘  (STEP51/52's accessor)
```

### 4.2 Caching — model it on `AssetAtlasCache`, do not invent

`src/io/AssetAtlasCache_*` already solves this exact problem well, and the ingest cache should be a
deliberate mirror of it rather than a new scheme:

- **Caller-supplied cache directory.** `AssetAtlasCache` never hardcodes one; it takes
  `cacheDirectory`, sourced from `AppSettings::assetCacheDirectory` via the System tab. Reuse the
  same setting (Q5).
- **Filename keying:** `<sourceStem>_<16-hex FNV-1a of the full absolute source path>` — the path
  digest exists precisely so two same-named sources cannot collide in one shared cache folder
  (`AssetAtlasCache_Fingerprint_IO.cpp:45-51`).
- **`SourceFingerprint {sourcePath, byteSize, modifiedTime, contentHash}` with `Matches()` requiring
  all four.** Default is path + size + mtime; content hash opt-in. For a ~2 MB corpus, **content
  hashing is cheap enough to enable by default** — unlike the 1.75 GB sanpack case where
  `AssetAtlasCache` rightly defaults it off. That is a real difference worth stating, not a
  copy-paste.
- **A format-version stamp checked *before* the fingerprint** (`DiskFormat::formatVersion`), so a
  reader-logic change invalidates every cache without touching source files. Load-bearing: dialect
  handling will change.
- **Bounds-checked reads that fall through to a rebuild** rather than reading out of bounds
  (`ByteCursor`). A corrupt cache is never fatal — Constitution §6.
- **Threading: fan out per-file evaluation over the existing `Sys::ThreadPool`** exactly as
  `AssetAtlasCache::BuildOrLoad(..., workerPool)` fans out decode. Each file gets its own
  `lua_State` (§2.3 constraint 5), so the fan-out is embarrassingly parallel with no synchronization.

**Cost basis (measured, not guessed):** 546 files, 2.12 MB total, 3.9 KB average. A cold ingest is
one directory walk plus 546 sandboxed evaluations of ~4 KB each; a warm start is a single manifest
read of a few tens of KB. Compare `AssetAtlasCache`'s existing budget — it already handles ~37 MB of
DDS across a few hundred files on the same thread pool. This is an order of magnitude *smaller*.
A real benchmark is not warranted before implementation; per Constitution §7 the basis tag is
**direct measurement of corpus size + structural equivalence to an existing shipped path.**

### 4.3 When ingestion runs

**Recommendation: explicit user action, with a cached result reused silently on later launches.**

Never blocking at startup: a 546-file cold ingest behind a splash screen is exactly the "app feels
broken" failure `MEMORY.md`'s `project_realworld_verification_gap` note warns about, and it would run
for users who never open a Props tab. The System tab already hosts the `sanpackPath` and
`assetCacheDirectory` pickers (`src/ui/Application_AssetPanel_UI.cpp:85`,
`src/ui/SystemTab_UI.h:28`); an **"Ingest game templates"** button beside them, showing
last-ingest time and entry count, matches the existing interaction model. On subsequent launches, a
fingerprint-matching cache loads automatically and silently — the user does not press the button
again. See Q4 for the alternatives.

### 4.4 Failure paths (Constitution §6, validate-then-default-then-log)

| Condition | Behaviour |
|---|---|
| `gameInstallRoot` empty | Ingestion is not attempted. Placeholder table stands. UI shows "no game install configured", non-blocking. |
| `gameInstallRoot` set but invalid | `ValidateGameInstallRoot`'s specific `reason` surfaced verbatim. Placeholder stands. |
| Root valid, `Gamedata` absent | Loose-Lua units/props still ingest. Environment props do not. Logged with counts — a **partial** ingest is a success, not a failure. |
| One file fails to evaluate | That file is skipped with its path and the Lua error logged. Ingestion continues. Never aborts the batch. |
| File is JSON, not Lua (the `" - Copy"` case) | Evaluation fails cleanly; skipped and logged as a dialect mismatch, **not** as corruption. Expected, not alarming. |
| Root table is `ProjectileTemplate` / `MarkerTemplate` | Skipped silently by design — counted, not warned. |
| `footprint` absent from an expected kind | Falls back to STEP58's `kDefault*FootprintSize`, logged once per identifier. |
| tpId collision | **Reported, both paths named.** Resolution policy is Q7. |
| Cache corrupt / version mismatch | Silent rebuild. |

**Nothing in this list ever prevents SanGen from launching, generating, or exporting a map.**

---

## 5. Superseding STEP58 without churn

This is the part that must be got right so STEP58 can ship immediately.

**STEP58's `src/io/WorldFootprintSizeTable_IO.h` is not edited by this design at all.** Its shape is
already correct: a `WorldFootprintSizeTable` class with `SetFootprint` / `Resolve` / `Clear` /
`Count`, plus a free `BuildPlaceholderWorldFootprintSizeTable()` returning a seeded instance.

The supersession mechanism is **adding a second producer of the same type**, not replacing the type:

```
STEP58 today:     BuildPlaceholderWorldFootprintSizeTable()  ──> WorldFootprintSizeTable ──> consumers
After ingestion:  BuildPlaceholderWorldFootprintSizeTable()  ──┐
                  TemplateIngest_IO::PopulateFootprintTable()  ─┴> WorldFootprintSizeTable ──> consumers
```

Ordering: seed the placeholder first, then overlay ingested entries. `SetFootprint`'s already-
documented last-write-wins policy makes real data win over placeholders **for free, with no new
policy and no code change in STEP58's file.** Unseeded identifiers keep resolving to
`kDefaultUnitFootprintSize` / `kDefaultPropFootprintSize` / `kDefaultUnknownFootprintSize` via the
existing `tpId` char-1 branch — which is exactly what the 15 `03_Desert` Quixel props and every
template-less prop will always need (§1.5 trap 3). **The placeholder path is not scaffolding to be
deleted; it is the permanent fallback.**

Consumers (`Application_AssetBridge_UI`, STEP52's pairing lookup, STEP53's LOD draw pass) see one
type with one accessor and never learn where the numbers came from. STEP58's own "Verify" step —
*"confirm no `.santp`/Lua parser exists at implementation time; if one has since landed, route back
to ARCH"* — is satisfied by this design being the ARCH-routed answer.

**One real tension to resolve, not paper over:** last-write-wins is correct for
placeholder-then-real, but **wrong** for real-vs-real tpId collisions (§1.5 trap 2). Those are two
different situations sharing one code path. See Q7.

---

## 6. Determinism and offline behaviour

### 6.1 SanGen must remain fully usable with no game install

**What still works with zero game install:** everything. Terrain generation, erosion, flow, masking,
placement, symmetry, the preview, `.sanmap` import/export, scenario authoring. None of these read a
game template. `AppSettings::gameInstallRoot` defaults to empty and every consumer treats empty as
"skip, log, continue" (§4.4).

**What degrades, precisely:** icon LOD sizing in the preview overlay. Every prop and unit icon is
sized from `kDefaultUnitFootprintSize` (2.0 × 2.0) or `kDefaultPropFootprintSize` (4.0 × 4.0) rather
than its true extent. Icons therefore render at a plausible uniform size per domain instead of a
true-to-scale one. A 1.2-unit scout and an 18.4-unit experimental both draw at 2.0. **Nothing is
invisible, nothing is missing, nothing errors** — STEP58's whole "always an explicit stand-in"
discipline exists for this.

This degradation is **cosmetic and bounded**, and that is a deliberate consequence of the accuracy
class, not luck — see §6.2.

### 6.2 Determinism — the load-bearing constraint

STEP58 assigns this table accuracy class **Visual**, because its only declared consumer is STEP53's
screen-space icon LOD sizing.

**That assignment is what makes ingestion safe, and it must be defended explicitly.** Constitution §4
defines the Deterministic sub-mode as *"competitive shared generation from settings+seed, no file
transfer"* — two machines produce bit-identical gameplay-authoritative output from a `.sanmap`'s
PARAMS and a seed alone. Ingested template data is **not** in settings+seed. It comes from a local
game install that may be a different patch, a different branch, or modded. Two users running the same
`.sanmap` with the same seed can legitimately hold different `footprint` values.

**Therefore: ingested game-template data must never feed an Exact-class stage, and must never enter
the Exact chain that Constitution §4 / ARCH_04_DispatchContract.md §4.6 closes over.** Concretely, it must not feed
placement spacing, obstacle distance, collision resolution, passability classification, or anything
under `src/proc/` that contributes to baked output.

This is not hypothetical. `footprint` is *obviously* the right input for prop scatter spacing, and a
future ticket will want it there. **That ticket would silently break determinism.** It must be
blocked by an explicit rule rather than rediscovered. See Q8 — I believe this needs an ARCH ruling
before ticket 89 lands, not after.

The cache does not weaken this: it is keyed on a source fingerprint, so it is deterministic *given an
install*, and no cache can make two different installs agree.

---

## 7. Proposed work-order breakdown

**Enumeration only — none of these tickets is written here.** Numbers 76 and 79–84 are claimed;
highest existing is STEP83. Proposed 85+. Dependency order.

| # | Ticket | Layer | One-line scope |
|---|---|---|---|
| **85** | `LuaTableEvaluate_SYS` + `LuaTableValue_SYS` | **SYS** | Sandboxed LuaJIT table evaluation (zero libs, instruction-count hook, size caps, `lua_pcall` only, fresh state per file) returning an owned plain-C++ value tree; reuses STEP65's vendoring, adds no second dependency. |
| **86** | `TemplateSourceScan_IO` | **IO** | Resolve the three template roots off `gameInstallRoot`; walk loose/unzipped directory trees, and drive the existing `SanpackReader::ExtractFiltered` for `Environment.sanpack`; prefer unzipped, fall back to zip, never both. |
| **87** | `TemplateDialect_IO` | **IO** | Branch on root table name across all five kinds; extract `footprint`/`tpId`/`tags`/collision box for the three that carry them; skip projectiles and markers by design; detect and report tpId collisions. |
| **88** | `TemplateIngestCache_IO` | **IO** | Fingerprinted (path+size+mtime+content-hash) disk cache with a format-version stamp and bounds-checked reads, mirroring `AssetAtlasCache`'s manifest discipline. |
| **89** | `TemplateIngest_IO` orchestrator | **IO** | Compose 86→85→87→88 with `ThreadPool` fan-out; emit `TemplateIngestReport`; populate STEP58's `WorldFootprintSizeTable` as a second producer over the placeholder seed (STEP58's file untouched). |
| **90** | Template-ingest `AppSettings` fields + shell bridge | **IO** | Durable last-ingest state / opt-out toggle on `Io::AppSettings` with the `Application_AppSettings_UI` load/flush wiring, following `STEP19`'s established pattern. |
| **91** | System-tab ingestion controls | **UI** | "Ingest game templates" button beside the existing pack/cache pickers; entry counts, last-ingest time, per-source coverage, and the loud non-blocking offline/partial banners of §4.4. |
| **92** | `bReclaimable` auto-population from ingested `tags` | **PARAMS + IO** | Populate STEP62's existing bool from ingested `HARVESTABLE`, closing STEP62's own deferred out-of-scope item. Does not change STEP62's field, wire key, or partition semantics. |
| **93** | Correct `ValidateGameInstallRoot`'s subpath | **IO** | Fix `<root>/Sanctuary_Data/Maps` → `<root>/engine/Sanctuary_Data/Maps` in STEP64's validator and its acceptance test; as drafted it rejects the real install (§3.2). Independent of everything above — can ship immediately. |

85 → 86 → 87 → 88 → 89 is a strict chain. 90 and 93 are parallel with all of it. 91 depends on 89 and
90. 92 depends on 89 and on STEP62 having landed.

---

## ⚠️ Flagged for the ARCH Expert — spec/doc corrections (I do not edit these)

Per `CLAUDE.md`, the Format Expert never writes `ARCH.md`, any `ARCH_NN_*.md`, or anything under
`sangen_arch_pack/`. Recorded here with exact replacement text so it is not re-derived or drifted.

1. **`GAMEDATA_LAYOUT_SPEC.md` — "Top level" nesting is wrong.**
   Current: *"Each sanpack unzips to `Gamedata/<Name>/<Name>/...` (the internal path repeats the pack
   name)"*, and the top-level listing `Gamedata/ = Audio/, Editor/, Environment/, …`.
   **Verified real layout:** `Gamedata/<Name>.sanpack.unzipped/<Name>/…` —
   e.g. `Environment.sanpack.unzipped/Environment/01_Highlands/…`. The real top level is
   `Audio/, Editor.sanpack, Environment.sanpack, Environment.sanpack.unzipped, Gameplay.sanpack,
   Gameplay.sanpack.unzipped, Pandemonium/, Pandemonium.zip, Projectiles.sanpack, Props/, UI.sanpack,
   UI.sanpack.unzipped, Units.sanpack, UnitsTemplates.sanpack, VFX.sanpack, icons_cache.json`.
   **A precision note on the framing I was given:** the brief describes this as SINGLE- vs
   DOUBLE-nested. Strictly, both are two path components; the error is in the **first component's
   literal name** (`<Name>.sanpack.unzipped`, not `<Name>`), not in the depth. The spec's downstream
   `UI/UI/` and `Units/Units/<tpId>/` shorthands are correspondingly wrong as literal paths, and
   `Units/` in particular is only present as a 1.35 GB **zipped** `Units.sanpack` with no unzipped
   tree at all. Recommend the spec state the pack-relative path (`<Pack>/<Biome>/…`) and name the
   container separately, so it stays true whether the source is the zip or an extracted tree.
2. **`GAMEDATA_LAYOUT_SPEC.md` — `Gamedata/` is not at the install root.** Its real location is
   `<root>/engine/Sanctuary_Data/Gamedata/`. The spec never states a root-relative prefix.
3. **`GAMEDATA_LAYOUT_SPEC.md` — the biome/prop-convention table needs two additions.** `Pandemonium`
   is listed as *"flat `<Name>.sanprop`"*; in the real install `Environment/Pandemonium/Props/` uses
   `<Name>/<Name>.santp` folders (17 of them) and the flat `.sanprop` files live in the separate
   `Gamedata/Props/` and `Gamedata/Pandemonium/` trees. Also worth recording: `03_Desert`,
   `DysonParts`, `Common`, `Skybox`, `Water`, and `Winter` contain **zero** template files, so the
   spec's *"only biomes with a `Props/` folder can contribute"* rule needs sharpening to *"only
   biomes with template files."*
4. **`DESIGN_MapScenarioIO_R1.md` §1 and `STEP64_GameInstallLocation_IO.md` §4 — wrong subpath.**
   `<root>/Sanctuary_Data/Maps` should be `<root>/engine/Sanctuary_Data/Maps`. As drafted,
   `ValidateGameInstallRoot` rejects the real install. `ARCH_15_04_ThreeFileOnDiskShape.md` is cited
   as the source of the two subpaths but contains no `Sanctuary_Data` string, so the error appears to
   originate in the design doc. Ticket 93 fixes the code; the docs need the same correction.
5. **`ARCH_15_08_ThirdPartyDependencyRuling.md` refers to `src/third_party/`, which does not exist.**
   Confirmed: there is no `src/third_party/` directory. Vendored headers currently live in `core/`
   (`FastNoiseLite.h`, `stb_image.h`, `miniz.c/h`), which `CMakeLists.txt` itself flags as an open
   ARCH question (§5.5, `ARCH_07_03_VendoredThirdPartyHeaders.md`). Not blocking, but §15.8's
   "the coder's call" ruling points at a directory that must be created first.
6. **`UNIT_PROP_MARKER_DATA_SPEC.md` counts are low.** It says *"98 prop definitions"* in
   `Environment.sanpack`; the real install has 111 `.santp` under `Environment/` (99 excluding
   `Dev/`), plus 34 more `.sanprop` and 34 `.santp` in the `Gamedata/Props`/`Gamedata/Pandemonium`
   trees. Its Dialect-A/B split is otherwise **accurate and confirmed**. Recommend adding the three
   non-prop root tables (§1.2) and trap 1 (§1.5) so the next reader-author does not rediscover them.
7. **`ARCH_14_13_OpenItems.md` item 1 wording.** Item 1 says *"real **mesh-derived** bounds."* The
   real source is the shipped `footprint = {x,y}` field, which is authored ground truth and strictly
   better for icon sizing than derived mesh bounds. Recommend item 1 be closed by tickets 85–89 with
   that substitution recorded, rather than left open awaiting a mesh-bounds pass nobody needs.

---

## ❓ Open questions — decisions with options

**Q1 — Does `ARCH_15_03`'s "SanGen never parses Lua back" forbid this?**
§15.3 ratifies option (c), export-only, and says *"SanGen never calls into a Lua parser to read this
file, **or any scenario content**, back."* Read literally, that clause is scoped to **scenario**
content, and this design reads **game template** data — a different corpus, a different direction,
and no round-trip. I believe there is no conflict. But the phrasing is close enough that a coder
could reasonably read §15.3 as a blanket prohibition and refuse ticket 85.
*(a)* ARCH confirms §15.3 is scenario-scoped and adds one clarifying sentence. *(b)* ARCH rules
Lua reading is barred outright, and this whole design collapses to Q2 option (b) or is abandoned.
**Recommend (a).** This must be settled before ticket 85 is dispatched.

**Q2 — Sandboxed execution, or a hand-written table-literal reader?**
*(a) Execute in a LuaJIT sandbox* (this design). Correct by construction — it is the same grammar the
engine uses; handles comments, annotations, nested tables, mixed array/hash, and the misspelled keys
without special cases. Cost: executing untrusted text, mitigated by §2.3's six constraints.
*(b) Hand-write a table-literal reader in IO.* No execution, no new dependency if STEP65 slips.
Cost: a new parser to maintain, and it must handle `---@type` annotations, multi-line comment
headers before the root assignment (§1.2 footnote — 12 real files), nested tables, arrays of tables,
trailing commas, and both `{x=..,y=..}` and positional forms. Constitution §6 says validate all
input; a bespoke parser is *more* likely to mis-handle a malformed file than LuaJIT is.
**Recommend (a)**, contingent on Q1 and on STEP65 landing. It is genuinely simpler and genuinely
safer, and the empirical finding that all 546 shipped files are pure literals (§2.3) means the
sandbox is a safety net, not a hot path.

**Q3 — Where does the *richer* catalog live?**
Footprint alone fits STEP58's existing `WorldFootprintSizeTable` in IO. But the same reader surfaces
`tags`, `economy.harvest`, `collisionInfo`/`collider`, and `general.displayName` — enough to feed the
unit catalog and prop index `UNIT_PROP_MARKER_DATA_SPEC.md` §"What SanGen actually needs" calls for.
*(a)* Keep everything in IO as sibling asset-derived tables, matching `AssetAtlasCache_*`.
*(b)* Introduce a DATA-layer catalog. *(c)* Ingest footprint only now; defer the rest to the texture
importer.
**Recommend (a) for footprint+tags now** (tickets 89, 92) **and (c) for the rest.** Constitution §1
defines DATA as generation output, which this is not — so (b) needs an ARCH ruling I would rather not
force. ARCH's call.

**Q4 — When does ingestion run?**
*(a) Explicit user action, cached thereafter* (§4.3 recommendation). *(b) Automatic at startup when
`gameInstallRoot` is valid.* *(c) Lazy on first need.*
**Recommend (a).** (b) makes cold start hostage to a 546-file walk for users who never need it; (c)
puts a multi-hundred-millisecond stall inside a UI frame. (a) matches how `sanpackPath` and
`assetCacheDirectory` already behave. Human's call — it is a UX decision, not a technical one.

**Q5 — Cache location: reuse `assetCacheDirectory`, or a separate setting?**
*(a) Reuse* — one folder the user already picks; `AssetAtlasCache`'s path-digest keying already
prevents filename collisions between unrelated sources. *(b) Separate setting* — independent
clearing, at the cost of a second picker.
**Recommend (a).** `ASSET_LOADING_SPEC.md` already ruled cache location is a user-picked folder;
adding a second picker for a 2 MB cache is not worth the UI surface.

**Q6 — What footprint do template-less props get?**
15 `03_Desert` Quixel props (and every `DysonParts`/`Common` asset) have no template file at all
(§1.5 trap 3) — they can never be ingested.
*(a)* They fall through to STEP58's `kDefaultPropFootprintSize`. But note their folder names carry no
`tpId` code, so the char-1 `'e'` branch does not fire and they land on
`kDefaultUnknownFootprintSize` (2.0) rather than the prop default (4.0) — arguably the wrong
fallback for a rock.
*(b)* Add an explicit "no template exists" marker so the UI can distinguish *not yet ingested* from
*ingestible data does not exist*.
*(c)* Derive bounds from `.sanmodel` — the original §14.3 "mesh-derived" idea, resurfacing only here.
**Recommend (a) plus (b)'s marker**, and explicitly **not (c)** — 15 props do not justify a mesh
parser. Someone should decide whether the fallback for a non-tpId path should be prop-shaped rather
than unknown-shaped.

**Q7 — tpId collision policy — this one genuinely conflicts with STEP58.**
`WorldFootprintSizeTable::SetFootprint` documents **last-write-wins**. That is correct for
placeholder-then-ingested (§5) but wrong for real-vs-real collisions, which are confirmed to exist:
`Cliff_03.sanprop` declares `tpId = "Cliff_02"`, 9 Pandemonium files have `tpId` ≠ filename, and 17
`" - Copy"` files duplicate names in JSON form (§1.5 traps 1–2).
*(a)* First-write-wins for ingested entries, with a deterministic source-priority order
(loose Lua > unzipped tree > sanpack), collisions reported loudly. *(b)* Keep last-write-wins and
only report. *(c)* Reject a colliding identifier entirely and fall back to the default.
**Recommend (a).** It is deterministic, it is reportable, and it keeps STEP58's documented policy
untouched for the placeholder-overlay case by making collision resolution ticket 87's job rather than
the table's. But this is a policy the ARCH Expert should ratify, because it means two producers of
one type follow two different write policies — which is exactly the kind of thing that surprises a
coder later.

**Q8 — May ingested data ever enter the Exact chain?**
§6.2 argues **no**: ingested data is not in settings+seed, so it cannot participate in Constitution
§4's Deterministic sub-mode. But `footprint` is the natural input for prop scatter spacing and
obstacle distance, and a future ticket will reach for it.
*(a)* ARCH rules ingested game-asset data is **permanently Visual-class only**, and any ticket
wanting it in PROC must first move the value into PARAMS (authored, serialized in the `.sanmap`,
transmitted with settings+seed) — which is the mechanism that would actually make it deterministic.
*(b)* Leave it to each future ticket's own accuracy-class review.
**Strongly recommend (a), and recommend it be ruled before ticket 89 lands.** (b) is how this breaks
silently: the failure mode is not a crash but a divergent competitive map, discovered late and hard
to attribute.
