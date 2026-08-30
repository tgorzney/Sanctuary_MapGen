# DESIGN — Navmesh Blocker Mesh Ingestion (R1)

*Authored by the SanGen Format Expert, 2026-08-30. **Design only — no code, no work-order.**
Read-only against `src/**` and read-only against the game install.*

*Grounded against: `sangen_arch_pack/CONSTITUTION.md`, `ARCH_01_05_FileSizeCeilings.md`,
`ARCH_02_LayerDirectoryMap.md`, `ARCH_04_DispatchContract.md` §4.6, `ARCH_18_SantpFootprintIngestion.md`
+ `ARCH_18_01_SandboxedExecutionPrimitive.md` + `ARCH_18_02_IngestedDataDeterminism.md` +
`ARCH_18_03_CatalogDataOwnership.md`, `ARCH_22_NavmapModifierBlockers.md` + `ARCH_22_09_OwnershipScopeRuling.md`,
`sangen_arch_pack/specs/{NAVMAP_MODIFIER_BLOCKER_SPEC,UNIT_PROP_MARKER_DATA_SPEC,ASSET_LOADING_SPEC,
DETERMINISM_SPEC}.md`, `work_orders/DESIGN_SantpFootprintIngestion_R1.md` (sibling design, tickets
85-92/96 of which have since **shipped as real code** — re-confirmed by direct read this session, not
assumed from the design doc alone), and a live read of the real Steam Demo install at
`E:\Games\Steam\steamapps\common\Sanctuary Shattered Sun Demo\` (nothing written there). `src/`
inventory confirmed by direct grep/read this session.*

---

## 0. Why this document exists

The human wants navmesh-modifier-blocker rectangles derived automatically from a placed prop's real
3D mesh intersected against the water-level plane, replacing the current hand-painted-mask workflow
(`NAVMAP_MODIFIER_BLOCKER_SPEC.md` §7/§7.1). This document designs **only the mesh-ingestion slice**:
(1) a safe `.sanmodel` binary reader, (2) LOD0/highest-fidelity path resolution from a prop's
blueprint, (3) per-instance world transform sourcing, and (4) a determinism/accuracy-class reasoning
note. The plane-intersection/rasterization geometry and the UI are each a different expert's parallel
design against this layer's output contract, defined precisely in §5.

**Load-bearing scope correction versus the brief's own framing of prior art:** the sibling
`.santp`/`.sanprop` ingestion design (`DESIGN_SantpFootprintIngestion_R1.md`) is **not just designed —
its tickets 85 through 92, plus 96, have shipped as real code**, confirmed by direct read this
session: `src/sys/LuaTableEvaluate_SYS.{h,cpp}`, `src/sys/LuaTableValue_SYS.h`,
`src/io/TemplateSourceScan_IO.{h,cpp}`, `src/io/TemplateDialect_IO.{h,cpp}`,
`src/io/TemplateIngestCache_IO.{h,cpp}`, `src/io/TemplateIngest_IO.{h,cpp}`,
`src/io/FootprintBakeFingerprint_PARAMS.h` (sic — actually `src/params/`),
`src/io/FootprintBakeStaleness_IO.h`, `src/io/PropReclaimableBake_IO.{h,cpp}` all exist and are wired
into `src/params/ScatterRule_PARAMS.h` (`PropRule::baseFootprintWidth/Depth`,
`footprintBakeFingerprint`). This changes the shape of this design substantially versus what the brief
implied: most of the *file-resolution* plumbing (game-install root, loose/unzipped/sanpack source
priority, sandboxed Lua evaluation, per-template disk caching) already exists and is reused, not
redesigned. What is genuinely new is: (a) a binary `.sanmodel` reader (no such reader, sandboxed or
otherwise, exists anywhere in `src/`), (b) `visuals.lods[]` model-path extraction (explicitly **out of
scope** in the shipped `TemplateDialect_IO.h`'s own header comment — see §6 flag 4), and (c) the
per-instance transform/identity plumbing described in §4.

---

## 1. Verified ground truth (live install + live `.sanmodel` reader source, this session)

### 1.1 `.sanmodel` binary layout — recorded from a third-party source, not re-derived

The byte layout below is taken from the human's own verified read of the real open-source Blender
importer/exporter (`github.com/GlowingShadow/sanmodel-blender`, `sanmodel.py`/
`sanmodel_importer.py`/`sanmodel_exporter.py`, read/write symmetric). **I did not independently read
that third-party source this session — I record it here as supplied ground truth, honestly labeled as
a third-party reverse-engineered Blender plugin's source, not official documentation**, and cross-check
it against real `.sanmodel` bytes read from the live install (§1.2) wherever possible.

```
<name-string, NUL-terminated>
then 9 fixed segments, each: [count][count * N floats], little-endian throughout:
  0 vertices   N=3   (x,y,z)
  1 normals    N=3
  2 tangents   N=4   (xyz + handedness sign)
  3 uv1        N=2
  4 uv2        N=2   (repurposed as boneweights when skinned)
  5 uv3        N=2
  6 colors     N=4   (RGBA)
  7 indices    N=3   (1 triangle; int32 BIT PATTERN packed into a float slot)
  8 bindposes  N=16  (4x4 skinning matrices; count = bone count; ONLY present if skinned)
```

**Critical gotcha (binding on the reader's implementation, per the brief's own emphasis):** the
per-segment `count` field, and every value in segment 7 (indices), are the raw bit pattern of an
`int32` sitting in a 4-byte float slot — `reinterpret_float_to_int = struct.unpack("<i",
struct.pack("<f", value))` in the source. **This must be a bit-reinterpret (`memcpy`/`std::bit_cast`),
never a numeric cast**, or every segment boundary and every triangle index silently corrupts.

**Coordinate convention:** `.sanmodel` stores **Unity-style Y-up, local/model space**
(`vecSanmodelToBlender = (v[0], v[2], v[1])`, "swap y and z axis to match unity") — consistent with the
engine's own `EngineClasses.float3(x, 0, z)` horizontal convention `NAVMAP_MODIFIER_BLOCKER_SPEC.md`
already documents. **Local space only** — the world transform is a separate concern, §4.

### 1.2 Confirmed real `.sanmodel` bytes match the name-string prefix

I opened a real file directly: `edbm0101_lod0.sanmodel` (01_Highlands) begins with the literal ASCII
bytes `edbm0101_lod0` followed by binary — matching the layout's own name-string-first claim. I did
not decode further (no reader exists to validate against); this is a weak but real corroboration, not
proof of the full 9-segment layout.

### 1.3 `visuals.lods[]` — confirmed shape, confirmed ordering rule, confirmed dialect gap

Read directly from real `propTemplate` (Dialect A) files:

**Single-LOD case** (`edbm0101.santp`):
```lua
lods = {
    { distance = 150.0, shadowCastingMode = 1,
      model = "Environment/01_Highlands/Props/edbm0101/edbm0101_lod0.sanmodel",
      material = "Environment/01_Highlands/Props/edbm0101/edbm0101_lod0.sanmaterial" }
}
```

**5-LOD case** (`edmm0204.santp`, 02_Evergreen), confirming the ordering rule:
```lua
lods = {
    { distance = 10.0,  model = ".../edmm0204_lod0.sanmodel", ... },
    { distance = 20.0,  model = ".../edmm0204_lod1.sanmodel", ... },
    { distance = 40.0,  model = ".../edmm0204_lod2.sanmodel", ... },
    { distance = 120.0, model = ".../edmm0204_lod3.sanmodel", ... },
    { distance = 600.0, model = ".../edmm0204_lod4.sanmodel", ... }
}
```

**LOD0/highest-fidelity selection rule, confirmed by direct read, not assumed:** array index 0,
minimum `distance` value, and the `_lod0` filename suffix all agree in every real file I read. The
**defensive rule to implement** (Constitution §6 — never trust file structure blindly) is **the
`lods[]` entry with the minimum `distance` value**, not "index 0" — a malformed or modded file could
plausibly ship the array out of order or the filename suffix could disagree with position; selecting
by minimum distance is self-correcting against both and costs nothing extra (a single linear scan over
what is typically 1-5 entries).

**⚠️ Confirmed real defect, load-bearing for path resolution (§4): folder name and model filename stem
can disagree.** `01_Highlands/Props/edmm0101/` contains `edms0103_lod0.sanmodel` through
`edms0103_lod4.sanmodel` — the **folder** is `edmm0101` but every model file inside it is named
`edms0103_*`. This directly confirms the brief's warning: **never synthesize a model path from a
tpId** (`<tpId>/<tpId>_lod0.sanmodel`); always read the literal `model` string from `visuals.lods[]`.

**⚠️ Confirmed real dialect gap — Dialect B props have NO model path at all.** Read directly,
`exe0000.santp` (engine-lua `PropTemplate`, capital P):
```lua
visuals = {
    mesh = { isWreckage = false, lod0distance = 33, lod1distance = 160, lod2distance = 300 }
}
```
No `lods[]` array, no `model`/`material` field of any kind — matching `UNIT_PROP_MARKER_DATA_SPEC.md`'s
existing "no model/material paths" note for Dialect B exactly. **A placed instance whose blueprintPath
resolves to one of the 4 Dialect-B templates (`exe0000`-`exe0002`, `defaultWreckage` — all engine-lua
test/default props, never real map content per the sibling design's own census) cannot be
mesh-ingested by this feature at all.** This must be a graceful, logged skip (Constitution §6), never
a hard failure — and it is expected to affect ~0 real instances on any authored map, since these are
test-only templates.

**Scope boundary this feature does NOT need to resolve:** unit templates (`UnitTemplate`) already have
their own native, engine-side navmesh-blocking mechanism (`skirtSize`, structures block all layers
except Air — `NAVMAP_MODIFIER_BLOCKER_SPEC.md` §2). This feature is **props only**, matching the
human's own framing throughout. Whether `UnitTemplate`'s `visuals` carries a `lods[]`-shaped array was
not checked — irrelevant to this design's scope.

### 1.4 Two populations of placed prop instances, with two different identity-resolution paths — a real, load-bearing finding

Read directly: `src/data/PlacementInstances_DATA.h`, `src/data/PlacementInstance_DATA.h`,
`src/proc/Placement_Manual_PROC.cpp`, `src/proc/Placement_Emit_PROC.cpp` (grep), `src/params/PropInstance_PARAMS.h`,
`src/params/InstancedTransform_PARAMS.h`.

`Data::PlacementInstances` (SoA, layer DATA) is the single resolved buffer both procedural and manual
props end up in, and it already carries exactly the world transform this feature needs
(`positionX/Y/Z`, `rotationX/Y/Z/W` quaternion, `scaleX/Y/Z` — absolute world/game units, per its own
header comment), plus `manualLayerId` (`-1` for procedural, a real layer id for manual) and
`bCollidable`. **But the identity field behaves differently per population:**

- **Procedural instances** (`manualLayerId == -1`): `templateIdentifier` (`Data::TemplateIdentifier`,
  a fixed 7-char game tpId) is populated by `Placement_Emit_PROC.cpp:67`
  (`instance.templateIdentifier = ruleTemplateIdentifiers[configurationIndex]`). This is real, usable
  identity.
- **Manual instances** (`manualLayerId != -1`): read `Placement_Manual_PROC.cpp`'s `MakeManualInstance`
  directly — it copies `position*/rotation*/scale*` from `Params::InstancedTransform` and **never
  touches `instance.templateIdentifier` at all**, leaving it at its all-zero struct default. **Every
  manually-placed prop's `Data::PlacementInstances::templateIdentifier` is empty/meaningless.** The
  file's own header comment ("straight 1:1 copy-through... no field is computed or reinterpreted") and
  its ARCH citation (`ARCH_14_13_OpenItems.md` item 3, "WORK-ORDER B", Ruling 3) confirm this is
  deliberate design, not an oversight — manual authoring's real identity lives in **PARAMS**
  (`Params::PropInstanceGroup::blueprintPath`, `src/params/PropInstance_PARAMS.h`), which is a
  **literal, already-fully-qualified `.santp` path** (e.g.
  `"Environment/01_Highlands/Props/edbm0149/edbm0149.santp"`), one group per distinct blueprint, an
  array of `PropTransform` per group.

**Design consequence, stated as a finding, not invented:** this feature must **not** try to recover
manual-instance identity from `Data::PlacementInstances` (it isn't there) and must **not** try to
re-zip `results.props` positionally back against `recipe.props[group][transform]` (a fragile,
undocumented ordering contract that a future PROC change could silently break). The two populations
should be resolved by **two separate walks**:
1. **Procedural** — walk `Data::PlacementInstances` where `manualLayerId == -1`; identity = `templateIdentifier` (tpId);
   world transform = the DATA columns directly (no PARAMS equivalent exists for procedural positions —
   PROC computes them, DATA is the only place they live).
2. **Manual** — walk `Params::MapRecipe::props` (`PropInstanceGroup::blueprintPath` +
   `PropTransform::transform`) **directly**, never through DATA for this population. This is simpler
   (identity and transform are already co-located in one PARAMS struct, no lookup needed) and more
   robust (PARAMS is the authored source of truth; DATA is a regenerated copy of it for this
   population specifically).

**A second, independent finding worth flagging now rather than letting the intersection expert
rediscover it (§6 flag 3):** `Data::PlacementInstance::bCollidable` is documented "collidable props are
gameplay" and is populated for procedural instances from `Params::ScatterTransform::bCollidable`
(`Placement_RuleBuild_PROC.h:51`) — but `Placement_Manual_PROC.cpp`'s `MakeManualInstance` **never sets
it**, so it defaults `false` for every manually-placed prop. If the (out-of-scope-for-me)
intersection/rasterization stage intends to gate "which instances get a blocker" on `bCollidable`, that
signal **does not exist today for manually-authored props at all** — every hand-placed prop currently
reads as non-collidable. Whether blocker-eligibility should even be `bCollidable`-gated (versus simply
"every resolvable prop instance gets intersected, and the water-plane test itself is the real filter")
is that other expert's call — I flag the gap, not the resolution.

### 1.5 What already ships and reuses cleanly

- `src/sys/LuaTableEvaluate_SYS.{h,cpp}` + `LuaTableValue_SYS.h` — the sandboxed LuaJIT evaluator
  (ARCH §18.1: zero libs, instruction-count hook, size caps, `lua_pcall` only, fresh state per file,
  owned-tree result). **Directly reusable, unmodified**, to evaluate a *specific already-resolved*
  `.santp` file's text for its `visuals.lods[]` table (§4.2).
- `src/io/SanpackReader_IO.h` — `ExtractFiltered(filter, limits, outPayloads)` already returns raw
  `std::vector<unsigned char>` payloads (not just text) via `SanpackPayload::bytes` — **directly
  reusable, unmodified**, for extracting a `.sanmodel` binary out of `Environment.sanpack` without
  writing any new zip code.
- `src/io/TemplateIngest_IO.h`'s `TemplateIngestReport::FindByTemplateIdentifier(tpId)` already returns
  a `TemplateFootprintRecord` whose `sourceFingerprint.sourcePath` is the resolved `.santp` **logical
  path** (either a real filesystem path, or a sanpack-entry logical path in the shipped
  `"<sanpackPath>!<entryName>"` form — confirmed by reading `TemplateSourceScan_IO.h`'s own doc
  comment). **This closes the entire procedural-instance `.santp` resolution problem for free** — no
  new source-resolution code needed for that population; just call the already-shipped, already-cached
  ingestion report.
- `Params::AppSettings::gameInstallRoot` (STEP64) + `Io::ValidateGameInstallRoot` — reused unchanged,
  same as the sibling design.

### 1.6 What I could NOT verify this session

- **Whether a `Environment.sanpack` zip entry's name is spelled identically to a `visuals.lods[]`
  `model` path's pack-relative string** (e.g. is the entry literally
  `"Environment/01_Highlands/Props/edbm0101/edbm0101_lod0.sanmodel"`?). I did not open
  `Environment.sanpack`'s central directory to confirm this. `TemplateSourceScan_IO`'s own resolution
  for `.santp` sources gives strong circumstantial confidence this convention holds (it already
  round-trips `.santp` entries this way), but I did not independently confirm it for `.sanmodel`
  specifically. **The mesh-fetch design (§4.2) must confirm this via `SanpackReader::DirectoryEntries()`
  at implementation time before trusting it.**
- **Exact `.sanmodel` file sizes.** I read one file's first bytes only (`Read`'s line-based tool does
  not report byte size for binary content). `ASSET_LOADING_SPEC.md` calls prop `.sanmodel`+`.dds`
  folders "heavy 3D assets, multi-MB" in aggregate but does not give a per-mesh figure. The reader's
  size cap (§3, Constitution §6) should be a conservative, generous constant (tens of MB) pending a
  real measurement at implementation time — do not guess a tight cap from this document.
- **Whether any real *environment* prop is skinned.** I greped every `Environment/**/*.santp` for a
  non-empty `skeleton` field and got **zero matches** — every environment prop's `visuals.skeleton` is
  `""`. Combined with the human's own scope boundary (static, non-skinned props only), this suggests
  the skinned-mesh code path (§3's segment-8 handling) is a defensive completeness measure, not
  something the real corpus will exercise — mirroring the sibling design's own "empirically all
  literal, still not an argument for omitting the safety" reasoning (§2.3 there). I did not check
  `Gamedata/Props`/`Gamedata/Pandemonium`'s loose `.sanprop` files or the engine-lua Dialect B set for
  skeletons.
- **Whether `Params::Water_PARAMS.h` (confirmed to exist, not read in depth) is the right water-level
  source for the intersection stage.** Out of my scope — noted only so the other expert has a pointer.

---

## 2. Scope boundary

**IN SCOPE (this document):**
1. A safe, bounds-checked `.sanmodel` binary reader (SYS-layer primitive).
2. LOD0/highest-fidelity model-path resolution from an already-resolved prop `.santp`.
3. Per-instance world transform + identity sourcing — reusing existing `Data::PlacementInstances` /
   `Params::MapRecipe::props` shapes, inventing nothing new.
4. The determinism/accuracy-class reasoning for the resulting baked artifact (§5), flagged for ARCH
   confirmation rather than asserted.
5. The output contract (§4.4) this layer hands to the plane-intersection/rasterization stage.

**OUT OF SCOPE (explicitly, per the dispatching brief):**
- The plane-intersection/rasterization geometry itself (triangle-vs-plane clip, rectangle
  decomposition) — a different expert's parallel design consuming this layer's output.
- The UI (a "Compute Navmesh Blockers" trigger, progress/coverage reporting, etc.) — a third expert's
  scope, though §4.3 notes the natural trigger-point precedent (mirrors the shipped "Ingest game
  templates" button, `STEP91_TemplateIngestionControls_UI.md`).
- `.sanmaterial` parsing — never needed; this feature is collision geometry only, never rendering.
- Skinned-mesh animation/posing (segment 8 bindposes, segment 4 boneweights) — read past
  (bounds-checked) but never interpreted; rest-pose geometry only, per the human's explicit scope
  boundary (§3).
- Unit meshes — units already have a native blocking mechanism (`skirtSize`); this is props only
  (§1.3).
- Widening `LuaSyntaxCheck_SYS` or `LuaTableEvaluate_SYS`'s own sandbox contract — both are reused
  unmodified, per ARCH §18.1's own "shares a library and nothing else" ruling, which this design does
  not touch or reopen.

---

## 3. The `.sanmodel` reader — layer, contract, file set

**Layer: SYS.** This is pure, bounds-checked binary parsing — not Lua execution, so none of ARCH
§18.1's sandbox machinery (instruction-count hook, `lua_pcall`, per-file `lua_State`) applies; it is a
plain buffer-in/struct-out primitive, the same footing as a `.dds` header sniff
(`AssetAtlasCache_IO`'s own validation step) but for a proprietary mesh format instead. Mirrors the
brief's own suggested precedent: a sibling primitive to `LuaTableEvaluate_SYS`, sharing the
"runtime-primitive, reachable from IO" footing `ARCH_18_01` already established for that primitive,
without sharing any of its actual machinery (no LuaJIT involved at all here).

**Constitution §6 still applies in full** — this is untrusted, third-party/modded asset data: cap the
input byte size before parsing begins, bounds-check every read (never trust a `count` field to stay
inside the buffer), never assert/throw on malformed input, return a `bSucceeded=false` result with a
diagnostic instead.

**Proposed file set** (naming per `ARCH_01_NamingLaw`; line counts are budgets under `ARCH_01_05`'s
soft-100/hard-150 ceiling, one primary type per file):

| File | Layer | ~Lines | Contract |
|---|---|---|---|
| `src/sys/SanmodelMesh_SYS.h` | SYS | ~40 | `Sys::SanmodelMesh` — owned plain-C++ result: `std::vector<float>` positions (x,y,z triples, local space, Y-up as stored), `std::vector<std::uint32_t>` triangle indices (already bit-reinterpreted, 3 per triangle), `bool bWasSkinned` (segment 8 was present — rest-pose geometry only, per §2's scope boundary). **Deliberately omits normals/tangents/UVs/colors/bindposes** — the plane-intersection consumer needs positions+indices only; see Open Question Q1 on whether normals should be included too. |
| `src/sys/SanmodelRead_SYS.h` | SYS | ~30 | `Sys::SanmodelReadLimits{maximumByteSize, maximumVertexCount, maximumTriangleCount}`, `Sys::SanmodelReadResult{bSucceeded, diagnosticMessage, mesh}`, `ReadSanmodelMesh(const unsigned char* bytes, std::size_t byteCount, const SanmodelReadLimits& limits)`. Pure, stateless, thread-safe (no shared state), safe to fan out over `Sys::ThreadPool` exactly like `LuaTableEvaluate_SYS` (§4.2's caching note). |
| `src/sys/SanmodelRead_SYS.cpp` | SYS | ~95-110 | The byte-cursor parser. A small internal `ByteCursor` (bounds-checked `ReadFloat`/`ReadFloatAsBitReinterpretedInt32`/`Skip(count*byteSize)`, every call checked against the remaining buffer length before advancing) reads the name string, then walks all 9 segments in order, populating `mesh.positions`/`mesh.indices` from segments 0/7 and **skipping** (bounds-checked, not blindly trusted) segments 1-6, detecting segment 8's presence for `bWasSkinned`. Any bounds violation aborts with `bSucceeded=false` and a diagnostic naming which segment failed — never a crash, never a truncated silent result. May need a second `.cpp` split (`SanmodelRead_ByteCursor_SYS.h`, header-only) if the segment-walk function threatens the 40-line function cap — the natural split point is "one small function per segment kind," which also makes the bit-reinterpret gotcha visually isolated in its own 5-10 line function rather than buried in a larger loop. |

**The bit-reinterpret gotcha, restated as an implementation-binding contract:** the `count` field
prefacing every segment, and every value inside segment 7 (indices), are read with
`std::bit_cast<std::int32_t>(rawFloatBits)` (or an equivalent `memcpy`) — **never** `static_cast<int>(floatValue)`
or `std::round`. A single wrong cast here silently corrupts every downstream segment boundary and every
triangle index without any observable error, exactly the failure mode the brief's ground-truth section
called out. This sentence should be copied verbatim into the eventual work-order's acceptance test:
construct a synthetic buffer with a known small mesh, verify the reader reproduces the exact index
values byte-for-byte, not merely "some plausible index."

---

## 4. Resolution flow — LOD0 path, transform sourcing, and the mesh cache

### 4.1 Overview

```
Per placed prop instance (procedural: Data::PlacementInstances; manual: Params::MapRecipe::props)
  │
  ├─ identity: tpId (procedural) or literal blueprintPath (manual) — §1.4
  │
  ▼
Resolve to a .santp SOURCE (reuse, never reinvent):
  procedural → TemplateIngestReport::FindByTemplateIdentifier(tpId)::sourceFingerprint.sourcePath
  manual     → the blueprintPath IS already the source (no lookup needed)
  │
  ▼
Fetch .santp bytes + evaluate (reuse Sys::EvaluateLuaTableSource, ticket 85, unmodified)
  │
  ▼
Extract visuals.lods[] → select minimum-distance entry → pack-relative .sanmodel path   [NEW, §4.2]
  │
  ▼
Resolve pack-relative path → raw bytes (unzipped tree, else Environment.sanpack via SanpackReader)  [NEW, §4.2]
  │
  ▼
Sys::ReadSanmodelMesh(bytes) → Sys::SanmodelMesh (local space)                          [§3]
  │
  ▼
CACHE keyed by resolved .sanmodel path (many instances share one template — §4.3)
  │
  ▼
Handoff (§4.4): { Sys::SanmodelMesh (shared, per-template) } × { per-instance world transform }
  → the plane-intersection expert's stage
```

### 4.2 The two genuinely new IO pieces

**`visuals.lods[]` extraction — RESOLVED by ARCH: option (a), the additive sibling file, is now
BINDING (`ARCH_22_11_MeshIngestionShape.md` point 2); option (b) is rejected.**
The shipped `TemplateDialect_IO.h`'s own header comment states an explicit OUT OF SCOPE list
(`economy.harvest`, `collisionInfo`/`collider`, `general.displayName`) per `ARCH_18_03_CatalogDataOwnership.md`
§18.3 — `visuals`/mesh paths were never named either way in that ruling; ARCH confirmed (re-reading
§18.3 directly) that nothing in it technically forbids extending `TemplateRecord`, but ruled that
doing so anyway would reopen a shipped type's scope for one feature's convenience — exactly the class
of scope creep §18.3 exists to prevent even where its text is silent. §18.3 itself is not amended by
this ruling.

**RULED (a): a new `src/io/TemplateVisualLod_IO.h/.cpp`** that takes an already-evaluated
`Sys::LuaTableEvaluateResult` (the SAME shipped type `TemplateDialect_IO.cpp` consumes), re-detects
the root table, and extracts the minimum-`distance` `lods[]` entry, returning
`{bFound, modelPath, diagnosticMessage}` — never touching the shipped, already-ARCH-ratified
`Io::TemplateRecord`/`ParseTemplateSource`.

**(b) Extend `TemplateRecord` — REJECTED.** Adding `visualLodModelPath` directly to the shipped
`Io::TemplateRecord`/`ParseTemplateSource` would have been more efficient (reuses the disk cache for
free) but is ruled out precisely because it touches a shipped type's already-closed scope for one
feature's convenience.

**Also ruled, binding (not left to the coder's discretion): the shared root-table helper.**
`TemplateDialect_IO.cpp`'s anonymous-namespace-private `DetectRootTable` becomes a shared
`Io::DetectTemplateRootTable(const Sys::LuaTableValue& globals, TemplateDialectKind& outKind) ->
const Sys::LuaTableValue*`, reused by both `TemplateDialect_IO.cpp` and the new sibling file — see
§6 flag 5 correction below.

**Pack-relative path → bytes** (new either way — no shipped primitive does this today;
`TemplateSourceScan_IO` only *walks* whole `.santp`/`.sanprop` corpora, it does not resolve one
specific already-known relative path): a new `src/io/PackAssetResolve_IO.h/.cpp`,
`Io::ResolvePackRelativeAssetBytes(gameInstallRoot, packRelativePath, safetyLimits) ->
{bSucceeded, bytes, diagnosticMessage}`. Tries `<Gamedata>/Environment.sanpack.unzipped/<packRelativePath>`
first (cheap, no inflate — same "prefer unzipped" priority `TemplateSourceScan_IO`'s own doc comment
already establishes generally); falls back to `SanpackReader::ExtractFiltered` over
`Environment.sanpack` with the entry name **assumed** equal to `packRelativePath` — **flagged as
unverified, §1.6**; the coder must confirm this against `SanpackReader::DirectoryEntries()` before
relying on it. Generalizes cleanly beyond `.sanmodel` (same shape would resolve `.sanmaterial`/`.dds`
later, though this ticket only exercises the `.sanmodel` case) — a genuinely reusable small primitive,
not a one-off.

### 4.3 Per-template mesh cache — many instances, one mesh

A map can place hundreds of instances of one rock/tree template. The mesh should be read **once per
distinct resolved `.sanmodel` path**, not once per instance. A small `Io::PropMeshCache` (an
`unordered_map<std::string /*resolved path*/, Sys::SanmodelMesh>`, populated lazily as distinct
templates are encountered during a bake run) sits above the resolution chain in §4.1 — this is a
run-scoped, in-memory cache, not a disk cache like `TemplateIngestCache_IO`'s (the corpus this feature
actually touches per bake is "however many distinct prop templates are placed on this one map," almost
always a few dozen, not the whole ~500-file `.sanmodel` corpus — a disk cache is very likely
unwarranted; see Open Question Q3).

### 4.4 Output contract handed to the plane-intersection stage — the seam this design ends at

```
Per distinct resolved template:  const Sys::SanmodelMesh&      (local space, shared, cache-owned)
Per placed instance:              { position, rotation quaternion, scale }
                                   — sourced per §1.4: Data::PlacementInstances columns (procedural)
                                     or Params::InstancedTransform (manual), NEVER a new representation
```

This design does **not** apply the transform (no world-space triangle soup is produced here) — per the
brief's own instruction, transform application + plane intersection belongs to the parallel expert
consuming this contract. The two pieces (mesh, transform) are handed off separately because one mesh
serves many transforms; combining them here would force a copy of the mesh per instance for no reason.

---

## 5. Determinism / accuracy-class reasoning — flagged for ARCH, not asserted

**This is genuinely different from the sibling footprint-ingestion case, and I believe the difference
resolves cleanly, but I am flagging the reasoning for ARCH to confirm rather than ruling on it myself**
(this is exactly the kind of Constitution §4 boundary call ARCH_18_02 itself was written to settle, and
that ruling's own mechanism is the load-bearing precedent below).

**The footprint precedent, as actually shipped (not just designed):** ARCH_18_02 ruled that ingested,
install-local template data may **never** be read live by `PROC`/generation (two users on different
game installs must never silently diverge), and permitted it to influence generation **only** via a
discrete, human-triggered, one-shot bake into an ordinary `Params::` field
(`PropRule::baseFootprintWidth/Depth` + `FootprintBakeFingerprint` for staleness detection) — this is
real, shipped code (`src/params/FootprintBakeFingerprint_PARAMS.h`, `src/io/FootprintBakeStaleness_IO.h`),
not a paper design. Once baked, the value is ordinary recipe data: transmitted with settings+seed,
reproducible with no game install at all, indistinguishable from any other designer-typed constant from
PROC's point of view.

**Why the blocker-rectangle case is not identical, and why that plausibly matters less here, not more:**
`DETERMINISM_SPEC.md` explicitly classifies "collidable props and reclaim (gameplay-relevant)" as
requiring bit-exact cross-machine agreement in the optional shared-generation mode — and a navmesh
blocker is arguably even more directly pathing-authoritative than a prop's own collision box. If this
feature's mesh-intersection computation were ever run **live**, on a peer machine, as part of PROC/
generation, it would inherit the exact same cross-install-divergence risk ARCH_18_02 ruled against for
footprint (a different game patch/mod could ship a different mesh, producing a different rectangle
list from an identical `.sanmap`+seed).

**But the brief is explicit that this is not how the feature is meant to work:** the computation runs
**once**, on the map author's own machine, as a **human-triggered export-time bake** — "baked into
static exported Lua data at map-authoring/export time... like the existing hand-authored `BLOCKER_RECTS`
tables... not recomputed live by each player's engine." Per `NAVMAP_MODIFIER_BLOCKER_SPEC.md` §1/§7,
the runtime consumer (the game engine, on every player's machine) has no mesh-intersection code at
all — it only ever reads static rectangle literals out of a Lua table. **No player's engine, and no
peer's SanGen instance in shared-gen mode either — provided the bake happens before any such transfer —
ever re-executes this computation.** This is structurally the same "author-machine-only, one-shot,
export-baked" pattern the Map Scenario system's own `ARCH_15_MapScenarioSystem.md` §15 already
ratifies for its companion `_Scenarios_Data.lua` file: SanGen owns the data, computes it once at
export, and the engine (and every peer) only ever reads the already-baked artifact.

**RESOLVED by ARCH (`ARCH_22_13_BakedArtifactStorageAndDeterminism.md` point 1):** storage is an
ordinary `Params::` field, never a companion export-only `.lua` artifact — option (a), the fork's
first branch. This mirrors `ARCH_18_02_IngestedDataDeterminism.md`'s already-shipped footprint-bake
mechanism exactly; no third mechanism is invented. Because `Params::` is definitionally the
"settings" `DETERMINISM_SPEC.md`'s "the host sends only settings+seed" already transports, a
`Params::`-resident rectangle list travels bit-identically to every shared-generation peer
automatically, as ordinary recipe payload — not because it is independently recomputed and happens to
agree, but because it is literally the same transported bytes, exactly like any other already-baked
`Params::` scalar. This closes the question without opening a new IO Architecture Expert shared-
generation transfer-contract exception; the companion-`.lua` alternative, which would have needed
exactly that exception, is rejected.

**The Exact/Deterministic-chain question, also RESOLVED (same ruling, point 2):** this feature sits
entirely outside Constitution §4's Deterministic sub-mode / `DETERMINISM_SPEC.md`'s bit-exact
cross-machine regeneration bar — settled, not merely "arguably outside," and structural rather than
conditional. ARCH's reasoning: the mesh-intersection computation never runs inside the live PROC
regeneration DAG (`RegisterStages()`), because the new one-shot PIPELINE bake responsibility
(point 3 of the same ruling — see the §6 flag 6 correction below) categorically bars it from ever
being part of what any peer machine "regenerates from settings+seed." What each peer receives is the
already-computed `Params::`-resident rectangle list, transported, never recomputed.
`DETERMINISM_SPEC.md`'s bit-exact bar governs values independently regenerated by more than one
machine; this value is never independently regenerated by more than one machine (only the author's),
so the bar does not apply — not because the value is exempt or decorative (it plainly is not, per
this document's own framing above), but because its correctness mechanism is **transport**, not
recomputation. `DETERMINISM_SPEC.md` itself has been amended with a short clarifying paragraph
recording that some gameplay-authoritative-adjacent `Params::` fields (baked footprint per §18.2;
navmesh blocker rectangles per this ruling) achieve cross-machine parity by transport, never
independent per-machine recomputation, and must never be wired into a live-regenerated PROC stage.

**Also ruled (point 3): a new PIPELINE responsibility class named for this bake** —
`NavmeshBlockerBake_PIPELINE.h/.cpp`, never wired into `RegisterStages()`, never auto-re-run, invoked
only by an explicit human action, running strictly after Placement. This is the concrete mechanism
that makes the "author-machine-only, one-shot, export-baked" pattern recommended below binding rather
than aspirational; see `ARCH_22_13_BakedArtifactStorageAndDeterminism.md` for the full shape (this is
PIPELINE/UI-layer detail outside this document's own IO-layer scope, recorded here only because it is
the mechanism that makes the determinism ruling above hold).

**What I am confident enough to state as a recommendation, not merely a flag:** whichever artifact
ARCH picks, the **mechanism** should mirror ARCH_18_02's shipped shape exactly — a discrete,
human-triggered bake action (never implicit inside Generate/export), a staleness fingerprint over the
actual external input (here: the resolved `.sanmodel` file(s) consumed, via the same
`{sourcePath, byteSize, modifiedTime, contentHash}` shape `FootprintBakeFingerprint`/`SourceFingerprint`
already use), and the baked result remaining an ordinary, re-editable value after baking (Constitution
§8), never a read-only mirror of the mesh. This reuses a proven, shipped pattern rather than inventing
a third one.

---

## 6. ⚠️ Flagged for the ARCH Expert — I do not edit these

Per `CLAUDE.md`, the Format Expert never writes `ARCH.md`, any `ARCH_NN_*.md`, or anything under
`sangen_arch_pack/`.

1. **RESOLVED by ARCH — corrected directly, not merely recommended.** `UNIT_PROP_MARKER_DATA_SPEC.md`'s
   "The `.san*` proprietary format family" section has been updated in place (2026-08-30) with an
   explicit exception clause: *"...does not need to parse `.sanmodel` bodies for ordinary map
   generation, only the templates/decals. (2026-08-30, corrected — the navmesh-blocker-generation
   feature is a real exception, not covered by the sentence above.) SanGen parses `.sanmodel` mesh
   bodies for the navmesh-blocker-generation feature only — rest-pose geometry, LOD0/highest-fidelity
   only, static/non-skinned props only — see `ARCH_22_11_MeshIngestionShape.md`'s mesh-ingestion
   ratification. It still does not parse `.sanvfx`/`.sananimation`/`.sanmaterial` bodies for any
   purpose."* No further action needed from this document.
2. **RESOLVED by ARCH.** `ARCH_22_09_OwnershipScopeRuling.md` §22.9's required design consult is now
   satisfied — this document, together with `DESIGN_NavmeshBlockerMaskGeneration_R1.md` (Generator
   Expert), `DESIGN_NavmeshBlockerGeometryMath_R1.md` (Compute Optimization Expert), and
   `DESIGN_NavmeshTab_UI_R1.md` (UI Expert), were together ratified as that consult in
   `ARCH_22_10_MeshIngestionOwnershipRuling.md`. §22.9's own text stands as historical, not
   retracted — the hand-authored Lua half (§22.1-§22.8) is unaffected; the new SanGen-owned pipeline
   is purely additive, producing a resolved rectangle list that still feeds into, and is consumed by,
   the unmodified §22.3/§22.4 Lua-authoring techniques. **No new top-level section was opened** — §22
   is extended in place with `ARCH_22_10` through `ARCH_22_17`, with the layer-membership ruling
   ("whether/how mesh-ingestion enters the layer stack") recorded in a table in §22.10: `SYS` gets the
   `.sanmodel` binary reader, `IO` gets pack-asset resolve/visual-LOD extraction/the run-scoped mesh
   cache, `MATH`/`PROC`/`PARAMS`/`PIPELINE`/`UI` each get their own named capability, ruled section by
   section in `ARCH_22_11` through `ARCH_22_17`.
3. **RESOLVED by ARCH — ruled and closed, not merely routed onward.** A real interface gap, found not
   invented: manually-placed props have no `bCollidable`-equivalent signal. §1.4's second finding —
   `Placement_Manual_PROC.cpp`'s `MakeManualInstance` never sets `instance.bCollidable`, so every
   hand-placed prop defaults `false`. `ARCH_22_11_MeshIngestionShape.md` point 5 confirms (by direct
   read this session, `PropInstance_PARAMS.h`/`Placement_Manual_PROC.cpp`) this is a genuinely missing
   PARAMS field, not a wiring omission — `Params::PropInstanceGroup`/`PropTransform` carries no
   collidable-equivalent field at all today.

   **RULED:** add `bool bCollidable = false;` to `Params::PropInstanceGroup`, wire key `"Collidable"`,
   group-level granularity — the exact same shape/precedent as the already-shipped `bReclaimable`
   field on the same struct (additive, no `SanGenVersion` bump, mechanical IO round-trip mirroring
   `bReclaimable`'s existing exporter/importer handling). `Placement_Manual_PROC.cpp::
   ResolveManualPropsAndDecals` is authorized to populate `instance.bCollidable = group.bCollidable;`
   in the same loop that already resolves `instance.manualLayerId` per group. Dispatched as a
   prerequisite to (or folded into) ticket 227 (`NavmeshBlocker_PlaneSlice_PROC`) — without it,
   mask-generation would silently produce zero output for every manually-placed prop. This also
   answers this document's own open question ("should blocker-eligibility even be `bCollidable`-gated
   at all?") by removing the reason to ask it differently — see
   `ARCH_22_12_MaskGenerationAlgorithmAndScope.md` for the filter ruling itself.
4. **`ARCH_18_03_CatalogDataOwnership.md` §18.3's deferred-field list does not mention `visuals`/mesh
   paths either way.** §4.2's fork (a) vs (b) needs an explicit ARCH answer: is extending the shipped,
   ARCH-ratified `Io::TemplateRecord`/`ParseTemplateSource` to also carry a LOD0 model path within
   §18.3's existing closed scope, or does it need a fresh, explicit nod because §18.3 never considered
   mesh paths at all? I recommend option (a), the additive sibling file, specifically to avoid needing
   this decided before the ticket can be dispatched — but ARCH should still record which reading of
   §18.3 is correct so a future reader isn't left guessing.
5. **RESOLVED by ARCH — ruled binding, not left to the coder's discretion.**
   `ARCH_22_11_MeshIngestionShape.md` point 2 ("Also ruled") confirms: `TemplateDialect_IO.cpp`'s
   anonymous-namespace-private `DetectRootTable` (the five-dialect root-table branch) becomes a
   shared `Io::DetectTemplateRootTable(const Sys::LuaTableValue& globals, TemplateDialectKind&
   outKind) -> const Sys::LuaTableValue*`, reused by both `TemplateDialect_IO.cpp` and the new
   `TemplateVisualLod_IO` sibling file (§4.2). ARCH's own reasoning: a five-line branch duplicated
   across two files in the same module is exactly what `ARCH_01_05_FileSizeCeilings.md`'s naming/DRY
   discipline prefers named once — this document's own flag correctly identified the gap; ARCH
   confirms it as binding rather than leaving it optional.
6. **RESOLVED by ARCH (`ARCH_22_13_BakedArtifactStorageAndDeterminism.md`).** Storage is an ordinary
   `Params::` field (option (a)); the companion export-only `.lua` artifact option is rejected, so no
   IO Architecture Expert shared-generation transfer-contract exception is needed. The
   Exact/Deterministic-chain question is settled: this feature sits entirely outside
   `DETERMINISM_SPEC.md`'s bit-exact regeneration bar because the value is transported, never
   independently recomputed on more than one machine — enforced structurally by a new one-shot,
   human-triggered PIPELINE bake responsibility that is never wired into `RegisterStages()`.
   `DETERMINISM_SPEC.md` itself has been amended with a clarifying paragraph recording this class of
   field. See §5's correction for the full recorded reasoning.

---

## ❓ Open questions — decisions with options

**Q1 — Does `Sys::SanmodelMesh` need normals, or positions+indices only?**
§3 proposes positions+indices only, on the reasoning that a water-plane intersection only needs raw
triangle geometry. *(a)* Positions+indices only (this design) — smallest memory footprint, matches the
stated consumer need exactly. *(b)* Also carry normals (segment 1) — costs one more
`std::vector<float>` per mesh, but lets the intersection stage sanity-check triangle winding/orientation
without re-deriving it from the vertex order, and could matter if the intersection expert wants to
distinguish "prop pokes up through water" from "prop mesh is inverted/degenerate." **Recommend (a)
unless the intersection expert's own design asks for (b)** — this is genuinely an interface decision
for that parallel design, not mine to force; flagging it here so it is not independently rediscovered
mid-implementation.

**Q2 — `visuals.lods[]` extraction: additive sibling file, or extend the shipped `TemplateRecord`?**
Detailed in §4.2 and flagged to ARCH in §6 flag 4. **Recommend (a), the additive sibling file** —
avoids any question of reopening ARCH §18.3's already-closed scope, at the cost of one small (five-line)
duplicated root-table branch. If ARCH confirms §18.3 never considered mesh paths and is comfortable
extending `TemplateRecord`, (b) is strictly more efficient (reuses the disk cache for free) and I would
not object to it.

**Q3 — RESOLVED by ARCH: option (a), in-memory, confirmed.**
`ARCH_22_11_MeshIngestionShape.md` point 4 rules: per-template mesh cache is in-memory, run-scoped
only. No disk cache. ARCH's reasoning mirrors this document's own: Constitution §7's basis-tag
discipline argues against pre-building caching infrastructure for a cost that has not been shown to
be real yet, the same posture this pack already took for the sibling footprint-ingestion cache before
real corpus numbers existed.

**Q4 — RESOLVED by ARCH.** `ARCH_22_13_BakedArtifactStorageAndDeterminism.md` point 1 rules option
(a): a new `Params::` field, transmitted inside the `.sanmap`'s settings+seed payload exactly as
ARCH_18_02's baked footprint scalar is — trivially bit-identical across shared-gen peers, zero new
transfer-contract question. Option (b), the companion exported `.lua` artifact, is rejected. See §5's
correction for the full ruling, including the accompanying Exact/Deterministic-chain resolution and
the new PIPELINE bake responsibility class that makes it structural.

**Q5 — Should mesh-ingestion for this feature be a fully independent human-triggered action from the
existing "Ingest game templates" button (STEP91), or a natural extension/second mode of it?**
Not designed here (UI is a different expert's scope), but worth naming: the existing button already
establishes the "explicit user action, cached thereafter" precedent (`ARCH_18` family). Whether
"Compute Navmesh Blockers" reuses that same trigger point, or is its own separate action (since it
needs a resolved `Data::PlacementInstances`/`Params::MapRecipe::props` — i.e., a generated map already
exists — unlike template ingestion which only needs a valid game install), is left to that UI-scoping
expert.
