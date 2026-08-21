# SanGen ARCH — the authoritative v2 architecture

The binding architecture for the SanGen v2 rebuild. Authored and ratified with the
human by the SanGen ARCH Expert. **Only the ARCH Expert writes this file.** It
resolves the Constitution's **(TBD)** items and executes the opening hit-list. Read
`sangen_arch_pack/CONSTITUTION.md` (always-loaded law) first; load specs from
`sangen_arch_pack/INDEX.md` as needed.

## Opening hit-list (the v2 mandate)
1. Reconcile the two data-model families — dead `core/data/*` + `GenParams_*` vs live
   `params/Params_*`.
2. Dismember the `GenerationParams` god object; evict GPU/GL state from DATA.
3. Unify the CPU/GPU twins behind one dispatch interface; retire rival toggles.
4. Eliminate the preview's shadow reimplementation of the sim (WYSIWYG).

---

## 1. Naming law (Constitution §2, resolved)

### 1.1 Literal, fully-spelled names — no abbreviations
Every file, type, method, variable, and parameter uses **complete descriptive
words**. No abbreviations, no truncations, no single-letter names.
- `centerX / centerY` (not `cx/cy`), `deltaX / deltaY` (not `dx/dy`),
  `frequency` (not `freq`), `config` (not `cfg`), `blueprintPath` (not `bp`),
  `radiusSeed` (not `rSeed`).
- **Only exceptions:** file extensions (`.sanmap`, `.dds`, `.glsl`); identifiers the
  file format or game dictates — `.sanmap` JSON keys, stratum names, and the
  format-derived PARAMS fields §1.8 governs — verbatim so import/export round-trips
  (§1.8 also lists its own named exceptions, e.g. `tpId` → `templateIdentifier`, the
  spelling actually used everywhere in `src/`); and the universally-standard hardware
  acronyms `Cpu` and `Gpu`. Our own code around them spells fully.
- **Booleans keep the `b` prefix** (`bNeedsMapUpdate`) — retained precedent; the word
  after it is still fully spelled.
- **A name must state the quantity, not the role.** A field holding a physical
  proportion is not called a "mask"; a field holding a visibility weight is not called
  a "proportion". (This rule is written in blood — see §7.2.)

### 1.2 Layer tag is a SUFFIX (TGUE convention)
A file's **suffix** declares its Constitution §1 layer, and it must match the file's
folder (§2). Descriptive-name first, layer last:

| Suffix | Layer | Example files |
| --- | --- | --- |
| `_MATH` | MATH / SIMD | `Vector_MATH.h`, `Noise_MATH.h`, `Morton_MATH.h`, `Spatial_MATH.h` |
| `_DATA` | DATA — computed output | `Heightfield_DATA.h`, `MapFields_DATA.h`, `FlowMap_DATA.h`, `Props_DATA.h` |
| `_PARAMS` | PARAMS — adjustable settings | `Layers_PARAMS.h`, `MarkerRules_PARAMS.h`, `Geometry_PARAMS.h`, `Enums_PARAMS.h` |
| `_PROC` | PROC processors | `NoiseBlend_PROC.cpp`, `Erosion_PROC.cpp`, `Mask_PROC.cpp`, `Placement_PROC.cpp` |
| `_PIPELINE` | PIPELINE (conductor) | `Generation_PIPELINE.cpp`, `StageGraph_PIPELINE.h`, `DirtyHash_PIPELINE.h` |
| `_IO` | IO / BRIDGE | `SanmapImport_IO.cpp`, `SanmapExport_IO.cpp`, `SanpackReader_IO.cpp` |
| `_UI` | UI | `MaterialTab_UI.cpp`, `RangeSliderWidget_UI.h`, `MapCanvas_UI.cpp` |
| `_SYS` | SYS (runtime primitives) | `ArenaAllocator_SYS.h`, `ThreadPool_SYS.h`, `Dispatch_SYS.h`, `Log_SYS.h` |

This replaces the old prefixes (`Gen_`, `Tab_`, `Widget_`, `Params_`, `Sanmath_`).

### 1.3 Math naming — domain + optimization variant
Math files are `<Domain>_MATH` (the word "San" is dropped — it is just math). The
**optimized / accuracy variant is a function suffix**, mirroring TGUE
(`Transform_SIMD` vs `Transform_Lossy_SIMD`) and the Constitution §4 classes:
- `_SIMD` — the vectorized accurate path.
- `_Lossy` (or `_Visual`) — the fast lossy twin (Visual class).
- Scalar/portable/deterministic variant carries no SIMD suffix (or `_Portable` when a
  name must disambiguate for `DETERMINISM_SPEC`).

### 1.4 CPU / GPU kernels pair by shared base name
A processor's CPU and GPU implementations live in the **same folder** and share a
**base name** so they sort adjacent and read as an obvious pair:
- `Erosion_PROC.cpp` — CPU (accuracy path).
- `Erosion_PROC.glsl` — GPU kernel (speed path).
The `.glsl` extension marks the GPU side; no separate layer suffix — it is owned by
its `_PROC` twin. Divergence between the pair is visible at a glance (Constitution §4,
`DISPATCH_INTERFACE_SPEC`).

### 1.5 File-size ceilings
- **Soft 100 lines / hard 150 lines** per file. One **primary type per file**.
- **Functions ≤ 40 lines.**
- Rationale: any edit reads and rewrites the whole file, so file size is the per-edit
  token cost and the mis-match risk — smaller is strictly better. Nothing forces a
  large file: functions are capped, and a large class splits its method definitions
  across multiple `.cpp` files (`Type_Aspect_PROC.cpp`) behind one small header.
- Exceeding a ceiling requires a **documented work-order exception** (Constitution §7)
  — a deliberate ratchet, never silent drift.

### 1.6 `.sanmap` top-level key casing — game-native vs SanGen-owned (ratifies work-order SPEC-4 Correction 0)
The `.sanmap` format already had a live, useful split, formalized here as binding
naming law: **camelCase top-level key = game-native field** (`width`, `armies`,
`markers`, …); **PascalCase top-level key = SanGen-owned section**
(`GeneralMapSettings`, `HeightmapStack`, `Symmetry`, `SlopeDefaults`, `Flow`,
`Accumulation`, `MarkersStack`, `PropsStack`, `DecalsStack`, `UnitsStack`,
`DetailNormal`, `SanGenVersion`, `PropGroups`, `DecalGroups` — the ratified schema
v3, `SANMAP_FORMAT_SPEC`).

- Every SanGen-owned top-level key is **single-token PascalCase, no spaces** —
  `GeneralMapSettings`, never `"General Map Settings"`.
- A field **merged into an existing format-native collection** (e.g. `armies[key]`)
  stays **lowerCamelCase** to match its siblings — `armyColor`, `alias`, never
  `"Army Color"`. It does not become a new SanGen section just because SanGen added
  it; it is a sibling field inside the format's own dictionary.
- This governs `.sanmap` **top-level JSON keys and format-collection member keys
  only**. It does not change §1.1 (identifiers the format/game dictates stay
  verbatim) or the C++ naming law inside `src/`; a PARAMS type's C++ member name
  still follows §1.1/§1.8 (`camelCase`/`b` prefix, and §1.8's data-kind rule for
  format-derived fields) regardless of how the same value is spelled at the JSON
  top level.

### 1.7 IO migration file naming — schema version steps (ratifies `IO_MIGRATION_SPEC`)
A `.sanmap` schema version bump (`SanGenVersion`, `SANMAP_FORMAT_SPEC`) is carried
forward by one small file per (domain, version-step): `<Domain>_Migrate_V<N>_IO.h/.cpp`,
migrating a V*N*-shaped JSON fragment to V*N*+1 shape — an instance of the §1.5
`Type_Aspect_LAYER` split-file pattern (`Domain` = Type, `Migrate_V<N>` = Aspect), never
a direct N→M jumper. Each migration is paired with a literal-fixture
`<Domain>_Migrate_V<N>_IO_Test.cpp` and, once green and shipped, is **append-only** —
never edited again. Exactly one file, `Sanmap_MigrationManifest_IO`, is touched to wire a
new version step's ordered migration list; everything else (migrations, tests, any new
JSON-transform primitive) is pure addition. **No self-registration** — static-init order
is a real failure mode and an explicit manifest line beats implicit discovery
(AI-legibility). Full contract, the runner, and the shared `JsonPrimitives_IO.h`
toolkit: `IO_MIGRATION_SPEC`.

### 1.8 PARAMS field naming for format-derived types — governed by data KIND, not by key presence
Ratified alongside `ENTITY_AUTHORING_PARAMS_SPEC` (`Params::Army`/`UnitGroup`/`UnitTransform`/
`MapArea`). A PARAMS field's naming is governed by **what kind of data it is**, not by whether a
`.sanmap` format key of a similar name happens to exist.

- **Pass-through, human-authored, verbatim entity data.** No PROC stage computes or
  reinterprets the value — round-trip fidelity is the field's entire purpose. Such a field uses
  **the format's own spelling by default**, converted only for case (`camelCase`) and the §1.1
  `b`-boolean prefix. Governs `Army`, `UnitGroup`, `UnitTransform`, `MapArea`
  (`ENTITY_AUTHORING_PARAMS_SPEC`) and any future type in that same hand-authored-entity family.
- **SanGen's own generative recipe/setting.** A PROC stage computes, derives, or reshapes the
  value, so the field keeps **SanGen's own descriptive name** even where a format key of similar
  meaning exists — already the practice (`Params::Water::waterLevelMaximum` vs. the format's
  `waterLevel`; `Params::Geometry::worldUnitsPerCell`, which has no format analog at all) and does
  not change here.
- **Named exceptions inside the pass-through bucket** — verbatim would collide with an established
  SanGen quantity, or is too generic to stay AI-legible:
  - **`Area.height` → `length`.** The format's `height` here means Z-extent/depth, not
    elevation — "height" is otherwise universally elevation in this codebase.
  - **`Area.x`/`Area.y` → `originX`/`originZ`.** The format's 2D texture-space origin vs.
    SanGen's `positionX/Y/Z` world convention, where `y` reads as elevation.
  - **`Army.faction` keeps the word but becomes `enum class Faction`**, not a raw `int` —
    matches the existing `MarkerCategory`/`MarkerPriority` pattern of retyping a format-style
    category int at the JSON boundary (`MarkerRule_PARAMS.h`).
  - **`tpId`/`tpid` → `templateIdentifier`.** Already the established spelling everywhere it is
    actually used as a C++ member (`ScatterTransform_PARAMS.h`, `PlacementInstance_DATA.h`).
    This **supersedes** §1.1's naming of `tpId` itself as the verbatim exception — the literal
    spelling `tpId` has never actually shipped as a C++ member anywhere in `src/`.
- **A `Dictionary<string, X>` becomes `std::vector<X>` with the dictionary key folded in as a
  `name` field on `X`.** Not new design — the existing choice for `Area`
  (`AreasTab_List_UI.h`'s `MapAreaRectangle`); applied one level deeper for `Army.groups` and
  `UnitGroup.units`/`UnitGroup.groups` (`ENTITY_AUTHORING_PARAMS_SPEC`). A PARAMS-shape
  consequence of the naming decision above, not a separate rule.
- **A format-native object gains a small SanGen field by direct injection when the field is
  genuinely novel information with no competing home** (`armyColor`, `alias`, and — ARCH §12 —
  `PropTransform`/`DecalTransform::layerIndex`); **a separate SanGen-owned array is used only
  when the metadata is richer than a scalar AND no format-native group container already exists
  to hold it** (`PropInstanceLayer`/`DecalInstanceLayer`, ARCH §12). This is one consistent rule
  applied per-case, not two competing philosophies — it does not reopen `armyColor`/`alias`/
  `MarkerTransform::alias`, all already-settled instances of the first branch.

---

## 2. Layer → directory map (Constitution §1/§2)

One directory per layer, the folder and the file suffix always agree, so a misplaced
file is visible instantly. The `core/` + `gui/` split and the dead `core/data/` are
retired.

```
src/
  math/     *_MATH                 pure stateless math (SIMD, noise, vector, morton, spatial)
  data/     *_DATA                 computed SoA output (heightfield, masks, flow, resolved prop/marker/unit instances)
  params/   *_PARAMS               adjustable settings / the recipe (layer stack, rules, constants, seed) — serializes to the schema v3 PascalCase sections (§1.6, `SANMAP_FORMAT_SPEC`)
  proc/     *_PROC  +  *.glsl      processors; each CPU .cpp paired with its GPU .glsl
  pipeline/ *_PIPELINE             the conductor — dirty-hash DAG, stage order, backend policy
  io/       *_IO                   .sanmap / SupCom import-export, sanpack reader — the platform seam
  ui/       *_UI                   imgui-bypass tabs + widgets, 100k-entity preview
  sys/      *_SYS                  runtime primitives — threads, allocation, GPU resources, dispatch mechanism, logging
```

- **DATA vs PARAMS are separate folders** — computed output (`data/`) never mixes with
  the adjustable settings/recipe (`params/`). Input (PARAMS) vs output (DATA).
- **GPU lives beside its CPU twin** in `proc/`, not a separate `gpu/` tree.
- A large class's split files stay in their layer folder (`PreviewRenderer_*_UI.cpp`).

---

## 3. Module boundaries & ownership (Constitution §1, resolved)

### 3.1 Dependency direction (downward only, no cycles)
| Layer | May depend on | Never |
| --- | --- | --- |
| `MATH` | (nothing) | any other layer |
| `PARAMS` | (nothing) | any other layer |
| `DATA` | `MATH` | GPU/GL handles; PROC/UI |
| `PROC` | `DATA`, `PARAMS`, `MATH` | UI; owning a backend choice |
| `PIPELINE` | `PROC`, `DATA`, `PARAMS`, `MATH`, `SYS` | UI; drawing |
| `IO` | `DATA`, `PARAMS`, `MATH` | simulating; PROC |
| `SYS` | `DATA`, `PARAMS`, `MATH` | knowing the pipeline shape |
| `UI` | `PIPELINE`, `DATA`, `PARAMS`, `SYS` | sim logic; touching PROC directly |

The canonical call chain: **`UI → PIPELINE → PROC → SYS`** (with PROC/PIPELINE reading
`DATA`/`PARAMS` and using `MATH`).

### 3.2 Hard rules (fall out of the direction)
- **GPU/GL handles live only in `SYS`** (and the `.glsl` kernels) — never `DATA`,
  never `PARAMS`. (Fixes the current `GenerationParams` holding GL state.)
- **UI never simulates** — it sets params, trips dirty flags, and asks `PIPELINE` to
  regenerate; it composites and *samples* baked results, never recomputes them.
  (Kills the preview shadow-sim and the `Widget_MapCanvas` spawning god-widget.)
- **PROC never draws; IO never simulates; PARAMS holds no logic.**
- **No layer knows the pipeline shape except `PIPELINE`** — stage order and the
  dirty-hash DAG live in exactly one place.
- **A PROC stage is a pure function of its declared inputs** (§3.4). No stage
  read-modify-writes a field it does not own.

### 3.3 Ownership of the moving parts
- **`PIPELINE` owns generation orchestration** — the dirty-hash dependency DAG, the
  PROC stage order (§7.4: noise/blend → erosion → thermal → flow/accumulation → mask →
  placement → bake), and resolving the per-stage backend/accuracy policy (Preview vs
  Output, Constitution §4). It calls `Dispatch_SYS` to run each stage. Replaces the
  `TerrainGenerator` god + the `main.cpp` regen loop.
- **`SYS` owns the runtime** — `Dispatch_SYS` (router mechanism: run kernel K on
  backend B), `GpuResource_SYS` (programs compiled once, persistent buffers, async
  fences), `ThreadPool_SYS`, `ArenaAllocator_SYS`, `Log_SYS`. Knows *how* to run a
  kernel, not *which* stages exist.
- **`PROC` owns the kernels** — one math source per stage, CPU `.cpp` + GPU `.glsl`
  pair; declares its inputs/outputs (so `PIPELINE` can build the DAG and the two-tier
  dirty flags) and its accuracy class; requests a backend, never selects one.
- **`DATA`/`PARAMS` own output/settings** — DATA = plain SoA computed arrays; PARAMS =
  the adjustable recipe (serializes to the schema v3 PascalCase sections, §1.6). No
  behavior, no GPU handles.
- **`IO` owns the format seam** — `.sanmap` schema v3 round-trip (`SANMAP_FORMAT_SPEC`,
  `IO_MIGRATION_SPEC`), sanpack ingestion, asset validation (Constitution §6). The
  swappable port boundary (§5).
- **`UI` owns presentation** — tabs, the universal widget library, the preview
  composite; reads baked results and the resident atlas, writes params.

### 3.4 Stage purity & single-writer rule (new — the M3 lesson)
Two rules that make the dirty-hash DAG sound. Both are binding on every PROC stage.

1. **Single writer per DATA field.** Every `*_DATA` field has exactly **one** producing
   stage. Other stages read it. `PIPELINE` derives the DAG edges from these declared
   input/output sets, so an undeclared write is a silent DAG lie.
2. **Stages are idempotent and re-runnable.** A stage is a pure function
   `outputs = f(inputs, params)`. It **never** reads a field, transforms it, and writes
   it back to the same field, because the dirty-hash conductor is entitled to re-run a
   single dirty stage without re-running its upstream — an in-place read-modify-write
   then applies its transform twice. If a stage transforms a field, the result goes into
   a **different** field.
   - *Sole exception:* a **sim** stage may own a field and evolve it in place (erosion
     owns the heightfield and the material proportions) — because the sim is the field's
     single writer and PIPELINE re-runs a sim from its upstream snapshot, never from the
     sim's own previous output. The exception must be declared in the stage's spec.

---

## 4. Dispatch contract (Constitution §4, resolved)

Replaces every ad-hoc `UseGPUx` bool. `PIPELINE` sets a `DispatchPolicy` per stage;
`Dispatch_SYS` reads it and runs the kernel on the resolved backend.

### 4.1 The policy object
```
enum class ComputeBackend    { Cpu, Gpu, Automatic }
enum class GenerationContext { Preview, Output }
enum class AccuracyClass     { Exact, Accurate, Visual }

struct DispatchPolicy {
    ComputeBackend previewBackend;
    ComputeBackend outputBackend;
    AccuracyClass  previewAccuracy;
    AccuracyClass  outputAccuracy;
    bool           bDeterministic;   // forces Cpu + portable transcendentals + ordered reductions
}
```
`Cpu`/`Gpu` are kept as standard acronyms (naming §1.1).

### 4.2 Per-stage defaults (Preview → Output)
| Stage | Preview | Output |
| --- | --- | --- |
| Noise · Blend · Mask · Thermal | Gpu / Visual | Cpu / Accurate |
| Erosion · Flow / Accumulation | Gpu / Visual | **Cpu / Exact** (shapes terrain + pathing) |
| Placement | Cpu / Accurate | **Cpu / Exact** (spacing, markers) |
| Bake · Albedo · preview color | Gpu / Visual | Gpu / Visual (decorative, determinism-exempt) |

These are defaults; §8 tweakability lets any stage be overridden per project. In the
Output context they are further constrained by §4.6.

### 4.3 Backend resolution (how `Dispatch_SYS` picks)
1. `bDeterministic` set and the stage is in the **Exact chain** (§4.6) → **Cpu**,
   portable transcendental + ordered-reduction path (`DETERMINISM_SPEC`).
2. else the stage's `previewBackend`/`outputBackend` for the active context.
3. else the global backend setting.
4. `Automatic` → fastest **legal** backend for the declared accuracy class given data
   residency (no needless Cpu↔Gpu copies). Same accuracy class ⇒ backends must agree
   within that class's tolerance; a backend that cannot meet the class is not legal.

### 4.4 Idle escalation (Preview context)
During interaction the preview runs its fast path (Gpu/Visual). When input goes idle,
`PIPELINE` re-runs the affected stages at **Output** accuracy and swaps the result in —
so scrubbing stays fast but the settled image is truth. WYSIWYG holds because the
preview *samples* that bake; it never re-simulates (§3.2). Idle escalation is only
sound because stages are re-runnable (§3.4.2).

### 4.5 Determinism scope
`bDeterministic` makes only the **Exact-class, gameplay-authoritative** outputs
bit-identical across machines (heightmap incl. erosion, flow, placement/markers,
collidable props). Visual-class outputs stay on the fast Gpu path and may differ per
machine. Experimental until the cross-machine bit-exact gate passes (`DETERMINISM_SPEC`).

### 4.6 Exact-chain closure (Output context only)
An Exact result cannot be produced from an input that was computed non-reproducibly.
Therefore, in the **Output** context:

> The **Exact chain** is the transitive set of stages whose output is read — directly or
> through intermediate stages — by any stage declared **Exact**. Every stage in the Exact
> chain is dispatched on the backend and code path its Exact consumer requires. When
> `bDeterministic` is set, the **whole Exact chain** runs Cpu / portable / ordered — not
> only the stages whose own declared class is Exact.

- A stage's declared `AccuracyClass` still describes **its own product**. Exact-chain
  membership is a **dispatch** property that `PIPELINE` computes from the DAG; no stage
  computes it, and no stage hardcodes it.
- **Visual-only consumers never pull a producer into the chain.** Bake reads everything
  and is Visual, so it constrains nothing.
- **Preview has no Exact chain.** The preview product is Visual by definition and every
  stage takes its fast path; the guarantee is honored on the §4.4 idle escalation to
  Output.
- Practical effect on §4.2: in Output, everything except Bake is already on `Cpu`, so
  this rule costs **no backend changes** — it only tightens the class label (and thus the
  code path selected under `bDeterministic`) for Noise/Blend, Mask, and Thermal, which
  all feed Exact consumers.
- An edge may be exempted (producer stays lax for one specific consumer) only via a
  documented **work-order exception** (Constitution §7) proving that consumer's Exact
  decision does not depend on that input's exactness.

---

## 5. God-object dismemberment (hit-list #1–2)

Applying §3 boundaries and the input/output split (settings → `PARAMS`, computed arrays
→ `DATA`). Four offenders and where their pieces land.

### 5.1 `GenerationParams` → typed modules
- **settings → `PARAMS`:** `Layers_PARAMS` (the editable layer stack), `Stratum_PARAMS`,
  `MarkerRules_PARAMS`, `PropRules_PARAMS`, `Water_PARAMS`, `Atmosphere_PARAMS`,
  `Geometry_PARAMS` (dimensions + seed), `ErosionFlow_PARAMS`, `Symmetry_PARAMS`,
  `Environment_PARAMS`, `Enums_PARAMS`.
- **computed arrays → `DATA`:** `Heightfield_DATA`, `BlendedMap_DATA`,
  `MapFields_DATA` (heightfield, flow, accumulation, `materialProportions`,
  `surfaceStratumWeights` — §7.2), `Markers_DATA`, `Props_DATA`, `Units_DATA`,
  `Areas_DATA`, `EntityIdBuffer_DATA`, `SpatialGrid_DATA`, cached noise.
- **dispatch toggles → `DispatchPolicy`** (PIPELINE sets, SYS reads).
- **dirty flags + hashes → `DirtyHash_PIPELINE`.**
- **GPU buffers / GL handles → `GpuResource_SYS`.**

### 5.2 `NoiseLayer` (~90 fields) → `Layers_PARAMS`
Keeps only identity + noise + blend + stratum-index. Evicted: image-bake state → a bake
concern in `PROC`; per-layer erosion → `ErosionFlow_PARAMS`; placement fields
(`AvoidWater`, `NearCliffs`, blueprint) → `PropRules_PARAMS`; physics tags → material/
stratum physics. Its computed noise output → the `DATA` cache.

### 5.3 `Widget_MapCanvas` (~720 lines) → `MapCanvas_UI`
Keeps draw + input only. Evicted: triangle height-interpolation → `Interpolation_MATH`;
unit-grid / symmetry **spawning + army creation** → `Placement_PROC` (UI only requests
it via `PIPELINE`); picking → reads `EntityIdBuffer_DATA` produced by `SYS`.

### 5.4 `PreviewRenderer` (~300 lines) → `PreviewComposite_UI`
Keeps pass ordering only. Evicted: GL load / shader compile / SSBO packing →
`GpuResource_SYS`; gradient LUT bake → **`GradientLut_UI`** (a UI-layer colorization
helper — **not** a PROC stage; corrected in §8.1); **sim/rule re-filtering → deleted**
(samples the bake, §3.2); picking readback → `Picking_UI`.

### 5.5 Retire outright
`TerrainGenerator` god + the `main.cpp` regen loop → `Generation_PIPELINE`. The dead
`core/data/*` + `GenParams_*` duplicate family → deleted (hit-list #1). Every hardcoded
GPU constant (erosion `0.3`, thermal `/2.0`) → a `PARAMS` field (Constitution §8).

---

## 6. v2 rebuild order (dependency-ordered milestones)

Bottom-up along §3.1; each milestone independently testable.

- **M0 — Foundation** (no deps): the real `MATH` library (portable SIMD abstraction,
  minimax transcendentals with declared accuracy classes, 2D/3D Morton + block-linear,
  spatial) + `SYS` primitives (`ArenaAllocator_SYS`, `ThreadPool_SYS`, `Log_SYS`,
  `GpuResource_SYS`, `Dispatch_SYS` router). `MATH_SIMD_SPEC`, `DISPATCH_INTERFACE_SPEC`.
- **M1 — Data model** (hit-list #1): define `*_DATA` (computed) + `*_PARAMS` (settings)
  replacing `GenerationParams`; delete dead `core/data/*` + `GenParams_*`;
  `.sanmap` schema v3 round-trip through `IO` (`SANMAP_FORMAT_SPEC`, `IO_MIGRATION_SPEC`).
- **M2 — Dispatch + PIPELINE skeleton** (hit-list #3): `DispatchPolicy`, `Dispatch_SYS`
  resolution, `Generation_PIPELINE` (DAG + dirty-hash). **Vertical slice on one stage
  (noise), both backends**, to prove the whole spine before fanning out.
- **M3 — PROC stages**, in pipeline order (§7.4): Noise/Blend → Erosion → Thermal →
  Flow/Accumulation → Mask → Placement → Bake. **Each stage built as a complete CPU +
  GPU pair and parity-checked together — a stage is "done" only when both backends
  produce in-class-equivalent results.** Not all-CPU-then-GPU; finish the pair, then the
  next stage.
- **M4 — Preview / WYSIWYG** (hit-list #4): `PreviewComposite_UI` samples the bake
  (shadow-sim deleted), `Picking_UI`, two-tier dirty flags derived from the DAG.
- **M5 — UI**: universal imgui-bypass widget library, tabs, `MapCanvas_UI`, and the
  asset pipeline (sanpack ingest → atlas → disk cache; `ASSET_LOADING_SPEC`).
- **M6 — Advanced / optional**: determinism mode + cross-machine bit-exact gate
  (`DETERMINISM_SPEC`); the persistent thickness stack + true surface-exposure
  derivation (§7.5); then future sim types (fluvial/glacial/snow-melt,
  `FUTURE_SIM_TYPES_SPEC`); then AI-analyzability validation + host/client
  (`AI_HOSTCLIENT_SPEC`).

### 6.1 Definition of done (per PROC stage)
A stage is complete only when: CPU **and** GPU implemented and parity-verified within the
stage's accuracy class; wired into `PIPELINE` + `Dispatch_SYS` (no rival toggle); all its
constants exposed as `PARAMS` (§8); its declared inputs/outputs satisfy the §3.4 purity
and single-writer rules; files within the §1.5 ceilings; and its work-order acceptance
test (Constitution §7) passes.

---

## 7. M3 design resolutions (ARCH rulings)

Questions surfaced by the M3 stage coders, ruled here so they are binding.

### 7.1 Where the remaining PARAMS live — one settings type per stratum
`ErosionFlow_PARAMS.h` and `Stratum_PARAMS.h` live in `src/params/` with the `_PARAMS`
suffix, like every other setting.

**There is exactly ONE per-stratum settings type: `Params::Stratum`, in
`src/params/Stratum_PARAMS.h`.** Everything a stratum is configured with — the mask
slope gate, the stored-mask merge mode, the bake/appearance settings (albedo source,
tint, tiling, the material's own mask-texture remap window — §7.2 item 5), and the soil
physics — is reached through it.

- **No rival per-stratum settings type.** A stage takes `const std::vector<Params::Stratum>&`
  (or a span of it). It may **not** take its own private per-stratum array. Two rival arrays
  must be kept in sync by hand, and the same field appears twice — which is exactly how the
  double-remap code defect below was created.
- **Composition is allowed; rival top-level types are not.** `Params::Stratum` aggregates
  small named sub-structs, and a sub-struct may be split into its own `_PARAMS` header
  when the §1.5 ceiling forces it. Such a header is a **member** file of `Stratum_PARAMS.h`,
  never a settings type a stage reaches independently.
- The imported mask **pixel data** (the TGA contents) is not settings: it is a
  `Data::FloatField` in `src/data/` (loaded input, not part of the recipe). Rule:
  *modes/thresholds → PARAMS; loaded pixels → DATA.*

**Standing violations to clear (current tree, M3-2 rework):**
1. `src/params/StratumMask_PARAMS.h` is a rival top-level per-stratum settings type — it
   must fold into `Params::Stratum`.
2. That same file holds `std::vector<float> importedMaskData` — loaded pixels inside
   PARAMS, a second breach of the rule above. The pixels move to a `Data::FloatField`.
3. `Proc::StratumBakeSource` in `src/proc/Bake_Kernel_PROC.h` is a *third* per-stratum
   settings surface living in PROC. Its settings fields fold into `Params::Stratum`; only
   the flattened GPU-layout record (`StratumKernelConfiguration`) stays in PROC.

### 7.2 Material proportion vs surface weight — the two fields (RATIFIED)
*Confirmed by the project owner. This section is settled law; the previous
"dev confirmation needed" flag is cleared.*

**Ruling: there are two per-stratum fields, not one, and they mean different things.**

| DATA field | Meaning | Single writer (§3.4.1) | Read by |
| --- | --- | --- | --- |
| `materialProportions[0..8]` | **Physical.** How much of each stratum is present in the column, per cell. Sums to 1 where the column is non-empty. | the sim stages (NoiseBlend seeds it; Erosion/Thermal evolve and renormalize it) | the sims; the Mask stage |
| `surfaceStratumWeights[0..8]` | **Visible.** The resolved 0..1 weight of each stratum at the surface after gating and stored-art merge. This is what the eye and the game shader see. | the **Mask stage**, exclusively | Placement (stratum gate), Bake (composite + stratum TGA export), preview |

Consequences, all binding:

1. **Erosion owns and renormalizes `materialProportions`** when it moves material between
   strata. That was always correct and is unchanged.
2. **The slope/visibility gate is never pre-baked into `materialProportions`.** Doing so
   lets a renormalizing sim undo the gate, and makes the Mask stage a non-idempotent
   read-modify-write (§3.4.2).
3. **The Mask stage never writes `materialProportions`.** It reads them (plus the
   heightfield, for slope), and writes `surfaceStratumWeights`. It is therefore a pure,
   re-runnable function — a mask-parameter change may re-run Mask alone.
4. **The Mask stage performs the combine itself**, and emits one resolved field directly —
   there is no separate per-stratum remap step after the merge:
   ```
   gate_s        = SlopeGateWeight(slopeGradient, stratum_s)          // 0..1
   procedural_s  = materialProportions[s] * gate_s
   surfaceStratumWeights[s] = Merge(procedural_s, storedArt_s, importedMaskMode_s)
                                                          // final value. The merge's own
                                                          // output-clamp window is the only
                                                          // rescale applied — see item 5.
   ```
   **This overrides the earlier "Mask emits a bare `visibilityWeight`, Bake multiplies"
   mechanism**, for a hard technical reason: `ImportedMaskMode` is **not** a multiplicative
   gate. `StaticOverride` *replaces* the value with the artist's art and `ProceduralStart`
   *adds* to it; neither is expressible as a single multiplicative weight applied later
   (the override would require `visibility = art / proportion`, which is degenerate wherever
   the proportion is zero). The merge must therefore happen where both operands are in
   hand — inside Mask. The architectural property the original ruling was protecting
   (proportion never destroyed by the gate) is preserved in full by the *separate output
   field*, which is the part that actually mattered.
5. **CORRECTED — there is no per-stratum surface-weight remap in the Mask stage, or
   anywhere in SanGen generation.** This item originally read "the remap happens exactly
   once, in the Mask stage." That claim is **wrong** and is withdrawn — ruled by the
   Generator Expert after independently verifying the evidence directly against the real
   code (not a summary). Nothing else in this section changes: the field split, item 4's
   merge, the stage-order rulings (§7.4), and Bake consuming `surfaceStratumWeights`
   verbatim all stand exactly as ratified.

   `merged_s` from item 4's formula **is** `surfaceStratumWeights[s]`, unmodified. The
   merge's own output-clamp window (`MergeStoredMask`'s `[maskMinimum, maskMaximum]`,
   defaulting `[0,1]`) is a generic safety clamp on the merge result — it is a *different*
   mechanism from, and must not be confused with, the field discussed below.

   `Params::Stratum::maskRemapMinimum`/`maskRemapMaximum` is **per-stratum material/
   appearance pass-through data**, not a Mask-stage input or output. Confirmed by:
   - **Struct placement in the real C# format** (`SanMap.Types.cs`, ground truth): the
     field sits beside `diffuseRemap`/`tileSize`/`normalScale` — the shader-appearance
     fields — never near anything visibility-related.
   - **v1 never computed with it.** Its one touch point,
     `PreviewRenderer.cpp:426-427,505`, read channel `[0]` of stratum 0 only and applied
     it as a single global preview-shader contrast uniform — an ad-hoc debug knob, never
     wired to the real export/bake path, and code v2 explicitly supersedes.
   - **The real per-stratum visibility mechanism is the wholly separate `stratums_1_4.tga`/
     `stratums_5_8.tga` splat-weight export**, which `surfaceStratumWeights` feeds
     directly and verbatim.
   - `StratumAppearance_PARAMS.h`'s own existing scope note already buckets
     `maskRemapMinimum`/`maskRemapMaximum` as appearance data "no generation stage reads
     yet."

   The field is consumed only by the **game's own renderer**, against the stratum's own
   composite/"mask" texture (`StratumAppearance::compositeTexturePath`) — a real texture
   asset, distinct from the `stratums_1_4/5_8.tga` files the Mask stage produces. **No
   SanGen generation stage reads or writes it today.** Bake still consumes
   `surfaceStratumWeights` verbatim — that conclusion is unchanged — but the *reason* Bake
   has no remap of its own changes: it is not "the one remap lives upstream in Mask
   instead," it is "there is no per-stratum surface-weight remap anywhere in SanGen
   generation." Bake's former kernel fields for this
   (`StratumBakeSource::maskRemapMinimum/Maximum`,
   `StratumKernelConfiguration::maskRemapMinimum/maskRemapRangeReciprocal`,
   `RemapMaskWeight`) stay deleted; `StratumKernelConfiguration` keeps the two now-unused
   scalar slots as explicit padding so its std430 stride holds at a 16-byte multiple
   (`DISPATCH_INTERFACE_SPEC` §4) — this was, and remains, the correct code shape, only the
   stated reason for it has changed.

   *Field placement (the Generator Expert leaves this open; not load-bearing either way):*
   `maskRemapMinimum`/`maskRemapMaximum` **stay direct members of `Params::Stratum`**
   rather than moving into `StratumAppearance` — there is no behavioral reason to churn the
   file, and `StratumAppearance_PARAMS.h`'s existing "NOT DUPLICATED HERE" note already
   documents the split for a reader who lands there first.

   A future work-order may eventually wire this field to a real SanGen consumer — most
   plausibly composite/mask-texture processing inside Bake (`MASKING_SPEC` §1.6). That
   consumer does not exist yet and is not designed here; until then the field stays pure
   round-trip data.
6. **Placement's stratum gate reads `surfaceStratumWeights`,** not `materialProportions`.
   "Scatter trees where grass shows" is a *visibility* statement, and WYSIWYG (hit-list #4)
   requires props to follow what the preview shows. This places Mask upstream of Placement
   (§7.4) and puts Mask in the Output Exact chain (§4.6).
7. **Seeding proportions from an imported `.sanmap` is an IO concern, not a Mask concern.**
   The stratum TGAs on disk are surface weights; when a map is imported, `IO` seeds
   `materialProportions` from them as the best available approximation and records that it
   did so. `ImportedMaskMode` in the Mask stage governs only the `surfaceStratumWeights`
   output. IO loads a field; it does not simulate (§3.2).
8. **Rename, do not overload.** The field previously called `materialMasks` is renamed
   `materialProportions` throughout (§1.1: a name states the quantity, not the role).
   The word "mask" is reserved for the on-disk `.sanmap` stratum art and for the
   `surfaceStratumWeights` that produce it. `SANMAP_FORMAT_SPEC` is unaffected — the
   on-disk masks are, and always were, surface weights.
9. **Clarifications (M3 mask-rework coder judgment calls, ratified).** Two decisions the
   rework made within the spirit of §7.2 but not previously named:
   - **`strata` (the per-stratum `Params::Stratum[]`) lives on `MapRecipe`**, not as an
     assembler side-vector. Stratum settings are PARAMS (§7.1) and must round-trip in the
     `.sanmap`; `MapRecipe` is the PARAMS aggregate, so they belong on it.
   - **Placement's `DominantStratumIndex` (biome tag) is computed from
     `surfaceStratumWeights`, not `materialProportions`.** Placement is the
     visibility-consistent consumer (§7.2.6, "scatter where it shows"); its biome tag must
     match the rendered surface for WYSIWYG, and Placement reads exactly one stratum field.
     (If a future gameplay consumer needs the *physical* dominant material, it reads
     proportions directly — a separate consumer, not a change here.)
10. **Amendment — `maskRemapMinimum`/`maskRemapMaximum` are genuine 4-component fields,
    not a scalar** (ratified in a dedicated `Params::Stratum`-IO session; confirmed against
    the C# ground truth `SanMap.Types.cs` — `Stratum.maskRemapMin`/`maskRemapMax` are both
    real `Vector4`, defaulting to `(0,0,0,0)`/`(1,1,1,1)` — and against a real shipped map).
    `Params::Stratum::maskRemapMinimum`/`maskRemapMaximum` (`src/params/Stratum_PARAMS.h`)
    currently collapse this to a single `float` each, losing real format data for a
    pass-through field — the same fidelity principle §1.8 already states for hand-authored
    entity data, applied here to a format-native per-stratum field.
    - **Superseded framing note (added by the item 5 correction above).** This item's own
      framing below — "where the remap runs," "one remap site" — predates and is superseded
      by item 5's correction: there is no remap site at all, in Mask or anywhere else in
      SanGen generation. The part of this amendment that remains binding, unaffected by that
      correction, is purely the **field's shape**: `maskRemapMinimum`/`maskRemapMaximum` is a
      4-component `Vector4` pass-through field, not a scalar, exactly as the format types it
      — independent of whether, or where, any stage ever consumes it.
    - **This does NOT reopen §7.1's "no rival per-stratum settings type."** That ruling is
      about not inventing a **second per-stratum settings type** that duplicates
      `Params::Stratum` (the double-remap code defect was two rival *arrays*, not one field
      with the wrong width). Widening one field's own shape on the *same* single
      `Params::Stratum` struct is not that — it corrects the field to match what the format
      actually carries, regardless of consumption.
    - **Shape:** `float maskRemapMinimum[kStratumColorChannelCount]` /
      `float maskRemapMaximum[kStratumColorChannelCount]` — the same 4-wide convention
      `StratumAppearance::diffuseRemapColor`/`farColorRemapColor` already use
      (`kStratumColorChannelCount`, defined in `StratumAppearance_PARAMS.h`, which
      `Stratum_PARAMS.h` already includes — reused rather than a second magic `4` or a new
      constant). Defaults become `{0,0,0,0}` / `{1,1,1,1}` — the same numeric values the
      scalar fields already carried, now per-channel, matching the C# defaults exactly.
    - **Presentation is separate from shape.** The Stratum tab may expose this as up to 4
      per-channel inputs (`StratumsTab_Appearance_UI.cpp`'s `DrawMaskRemapWindow`) — a UI
      decision, not a PARAMS one; not designed here.
    - **Shape only, not wiring.** Widening the field, updating its `IO` read/write (see
      `SANMAP_FORMAT_SPEC` Correction 13), and updating its own in-code comment are separate
      coder work. **CLOSED by the item 5 correction above:** the earlier open question of
      "how does the Mask kernel's single-scalar-per-cell surface weight consume a
      4-component remap window" no longer applies — the Mask kernel does not consume
      `maskRemapMinimum`/`maskRemapMaximum` at all, and no coder needs to stop and report on
      it for that reason. (A coder still stops and reports before inventing any *new*
      consumer for this field — §8.4 — but the specific open question this amendment
      originally flagged is resolved, not merely deferred.)

### 7.3 Vendored third-party headers
Third-party vendored code (`FastNoiseLite.h`, `miniz`, `stb_*`) does **not** belong in a
layer folder and is **exempt from the naming law**. It lives in **`src/third_party/`**
with its upstream names/style unchanged; our code includes it as
`third_party/<Header>`. This keeps the layer folders pure (our code, our naming) and
vendored code clearly quarantined.

### 7.4 Pipeline stage order (binding on M3-8)
```
NoiseBlend → Erosion → Thermal → FlowAccumulation → Mask → Placement → Bake
```
This **supersedes** the order M3-8 currently registers (`NoiseBlend → Mask → Erosion → …`)
and the informal order quoted in earlier drafts of §3.3 / §6.

Why Mask moves after the sims:
- **The gate must be evaluated on the final slope.** Gating on the pre-erosion heightfield
  gates against terrain that no longer exists — the visible strata would not follow the
  eroded landform. Semantic requirement, independent of §7.2.
- **The proportions the gate multiplies must be the post-sim proportions.** Erosion and
  Thermal move material between strata; the visible surface must reflect where the material
  ended up.
- **It removes the ordering hazard entirely.** With Mask last among the field stages, no
  renormalizing stage runs after it, so the gate cannot be undone even by a future sim
  inserted into the chain.

Placement of Mask relative to `FlowAccumulation`: `FlowAccumulation` reads the heightfield
and writes only `flow` / `accumulation` — it does not modify height, so Mask could legally
precede it. Mask is nevertheless placed **after** it, at zero cost, so that `accumulation`
is available as a gate input (e.g. "no grass in the river channel") when §8 tweakability
adds it. Mask must come **before** Placement (§7.2.6) and Bake.

Any later insertion of a sim stage (`FUTURE_SIM_TYPES_SPEC`) goes **before** Mask, in the
sim block. This is a standing rule, not a one-off.

### 7.5 Triaged follow-ups — NOT in scope for the M3-2/M3-8 rework
Two defects were raised alongside §7.2. Both are real; neither is resolvable inside the
Mask/stage-order rework, and neither blocks it. They are recorded here so they are not
lost and so no coder "fixes" them opportunistically.

**(a) Volume fraction is not surface exposure.** `Erosion_Field_PROC.cpp::WriteThicknessToFields`
writes `ticks / totalTicks` — a **volume fraction** — while `LAYER_SYSTEM_SPEC` defines the
exported stratum mask as **surface exposure** ("topmost layer with thickness > 0, soft
blend"). These are different quantities: thin topsoil over deep bedrock reads ~0% under
volume fraction but should visually cover the surface.
*Reframing under §7.2:* this is **no longer a defect in Erosion.** Volume fraction is
exactly what `materialProportions` is now defined to mean, so Erosion's write-back is
correct. What is *missing* is the volume → surface-exposure derivation, and its home is the
Mask stage (the stage that turns physical proportion into visible weight).

**(b) The thickness stack is not persistent DATA.** Each sim stage reconstructs
`thickness = height × proportion` on entry and collapses back to a proportion on exit,
which discards buried stratigraphy across stage boundaries. `FUTURE_SIM_TYPES_SPEC` expects
sims to consume ordered thickness columns as their native representation.

**These two are one problem.** Surface exposure cannot be derived from proportions alone,
because a proportion vector carries no stratigraphic **order** — you cannot know which
stratum is on top. (a) is therefore blocked on (b): the ordered thickness column must be
persistent DATA before true surface exposure can be computed.

**Ruling — deferred to its own ARCH ruling and work order, in M6 (§6):**
- Do **not** attempt either fix in the M3-2/M3-8 rework.
- Until then, the Mask stage consumes `materialProportions` as its exposure approximation,
  and this approximation is documented at the call site as such.
- **The seam is safe to build against now.** When the thickness stack lands, Mask's input
  changes from "the proportion field" to "the surface-exposure field derived from the
  stack" — same shape (9 × `FloatField`, 0..1), same consumer, same kernel. The Mask
  kernel does not change; only its input binding does.
- Anyone raising this again: it needs a DATA-shape ruling (ordered thickness columns:
  layout, fixed-point width, memory cost at 4096², and which stage owns the stack across
  stage boundaries), not a patch.

---

## 8. M4 design resolutions (ARCH rulings)

Questions surfaced while dispatching the M4 (Preview / WYSIWYG) work-orders, ruled here
so they are binding. §8.1 **corrects** a token in §5.4; §8.2–§8.3 create the two missing
supporting types; §8.4 states the standing scope law those two questions exposed.

### 8.1 The gradient LUT bake is `UI`, not `PROC` (corrects §5.4)
**Ruling: the color-ramp LUT bake lives at `src/ui/GradientLut_UI.h/.cpp`.
`Gradient_PROC` is retired as a name and never existed as a file.**

§5.4's original "gradient LUT bake → `Gradient_PROC`" was written before §6.1 and §7.4
existed, and it is wrong under both:

- **PROC has a definition of done that a color ramp cannot satisfy.** §6.1 requires every
  PROC unit to be a CPU + GPU pair, parity-checked, wired into `PIPELINE` + `Dispatch_SYS`,
  with a declared accuracy class and declared DATA inputs/outputs. A 256-entry color LUT
  has no GPU twin worth writing, no DAG node, and produces no DATA field any stage reads.
- **It is not a pipeline stage.** §7.4 enumerates the stage order exhaustively; a color
  ramp is not in it and must not be smuggled in.
- **It is presentation, by the layer definitions.** Constitution §1: PROC is *applied
  processors* (terrain synthesis, erosion, masking, placement); UI *"composites/samples
  baked results."* Turning a designer-chosen ramp into a sampled table is colorization —
  the definitional UI job.
- **§3.2's "UI never simulates" is not violated.** That rule forbids the UI re-deriving a
  *simulated quantity* (slope, flow, rule filtering) — the shadow-sim, hit-list #4. A LUT
  is built from PARAMS alone; it reads no DATA field and duplicates no stage. Building a
  presentation resource from settings is not simulating.
- **Direction check (§3.1).** UI → PARAMS is legal and downward. The reverse would not be:
  nothing in PROC may include a `_UI` header, so if a future **bake/export** path ever
  needs to sample a designer ramp, the LUT builder moves to `MATH` (a pure ramp→table
  function, `GradientLut_MATH`) and UI calls it there. It does **not** become PROC. As of
  today no PROC stage consumes a color ramp (Bake composites stratum art, not ramps), so
  `UI` is correct and is the least-privilege home.

`PreviewComposite_UI` (M4-3) owns the GPU upload of the baked LUT via `GpuResource_SYS`;
`GradientLut_UI` stays pure CPU and sandbox-testable with no GL.

### 8.2 `GradientRamp_PARAMS` — the missing v2 settings type
A color ramp is an **adjustable setting**, so it is PARAMS, and `src/` may never include a
`core/` header. The type therefore has to exist in the v2 tree before M4-2 can compile.

**Ruling: `src/params/GradientRamp_PARAMS.h`, type `Params::GradientRamp` (with its member
`Params::GradientStop`).** Naming follows §1.1/§7.1 precedent (`Params::Water`,
`Params::Stratum`): the namespace already says "settings", so the type states the
**quantity** — `GradientRamp` — and the legacy role-word name `GradientSettings` is
**not** carried over. M4-2's signature becomes
`BakeGradientLut(const Params::GradientRamp&, int resolution)`.

Binding shape decisions (they are ARCH rulings, not coder preference):
- `stops` is a `std::vector<GradientStop>`; `GradientStop` holds `location` + `color[4]`.
- **`location` is normalized 0..1** along the ramp. The legacy field was "0.0 to 100.0
  (or mapped to degrees 0–90)" — a domain-dependent scale baked into the settings type,
  which is exactly the "name/quantity ambiguity" §1.1 forbids. Domain mapping (slope
  degrees, flow range, height range) belongs to the **consumer**, which normalizes its own
  domain before sampling the LUT. Verified safe: gradients are **not** part of any
  SanGen-owned schema v3 section (no `Gradient` key anywhere in `SANMAP_FORMAT_SPEC`), so
  no round-trip breaks.
- `bSmoothInterpolation` keeps the `b` prefix (§1.1) and selects smoothstep vs linear.
- One ramp **per colorized field** — slope, flow, accumulation, height, water each own
  theirs. The legacy "accumulation reuses the flow gradient" aliasing is retired
  (`PREVIEW_COMPOSITING_SPEC`).
- Resolution is a tweakable (Constitution §8), defaulted to 256, not hardcoded at the
  call site.

### 8.3 `SpatialGrid_DATA` vs `Placement_SpacingGrid_PROC` — two different structures
**Ruling: they are unrelated and both stay. `Placement_SpacingGrid_PROC`'s header comment
is correct** and is not to be "reconciled" with §5.1.

| | `Proc::SpacingGrid` (`src/proc/Placement_SpacingGrid_PROC.h`) | `Data::SpatialGrid` (`src/data/SpatialGrid_DATA.h`, new) |
| --- | --- | --- |
| Purpose | Poisson **min-spacing rejection** during scatter | **Hit-test** acceleration for the UI cursor |
| Cell size | the rule's spacing radius (varies per rule) | the map divided into a fixed chunk count |
| Lifetime | transient, inside one Placement run | persistent DATA, survives to the UI |
| Contents | accepted candidate positions | indices into the resolved instance arrays |
| Layer | PROC (a scatter accelerator) | DATA (a computed index) |

`SpatialGrid_DATA` is the §5.1 replacement for the v1 `GenerationParams::MarkerSpatialGrid`
(32×32 `MarkerChunk`s). M4-4's `PickMarker` takes **`const Data::SpatialGrid&`**; the
legacy name `MarkerSpatialGrid` does not appear anywhere in `src/`.

Binding shape decisions:
- **Indices, not string keys.** The v1 chunk stored `std::vector<std::string> MarkerKeys`;
  v2 stores `std::int32_t` indices into `Data::PlacementInstances` (the resolved SoA).
  A pick returns an index; the caller reads the SoA columns it needs. String keys in a hot
  hit-test path are a cache-coherence defect, and `PlacementInstances` is already the SoA
  of record.
- **Flat CSR buckets, not `vector<vector<>>`** — `bucketStart[cellCount + 1]` +
  `instanceIndex[total]`, two contiguous arrays. Same reason the instance buffer is a real
  SoA: one allocation, one cache line per chunk query.
- **The cell hash lives on the DATA type** as a pure accessor
  (`CellIndexAt(worldX, worldY)`), so the builder and the picker share exactly one
  world→cell mapping. Two copies of that arithmetic is how a picker silently drifts from
  its index.
- **Chunk resolution is a tweakable** (Constitution §8), defaulted to 32, carried on the
  grid — not a hardcoded 32 in two places as in v1.
- **Single writer (§3.4.1): `Generation_PIPELINE`, immediately after the Placement stage.**
  The grid is a derived index over `PlacementInstances`, not a new physical quantity, so it
  needs no PROC stage of its own; the container exposes a mechanical `Build(...)` exactly as
  `EntityIdBuffer_DATA` exposes `Set(...)`, and PIPELINE is its only caller. `Picking_UI`
  is strictly a reader.

### 8.4 Scope law — a coder never invents a missing type
Standing rule, generalized from §8.2/§8.3 (both were caught only because the dispatcher
diffed the work-orders against the tree):

> **A work-order's target-file list is exhaustive.** A coder that discovers it needs a type
> which does not exist **stops and reports**; it does not create that type in a folder its
> work-order does not name, and it does not substitute a legacy `core/` type to get a
> build. A missing type is a missing work-order.

Rationale: a type created as a side effect of another task gets its shape from whatever the
one caller happened to need, in whatever layer that caller lived — which is precisely how
the v1 duplicate `StratumSettings` families (hit-list #1) came to exist. New types get a
work-order and, where the shape is not obvious, an ARCH ruling (§8.2, §8.3 are those
rulings).

---

## 9. `Params::Army` / `UnitGroup` / `UnitTransform` / `MapArea` (ARCH ruling, ratifies `ENTITY_AUTHORING_PARAMS_SPEC`)

Fills the gap standing since M1: `MapRecipe` has always held procedural placement RULES
(`MarkerRule`/`PropRule`/`DecalRule`/`UnitRule`, §7.1-adjacent, `PLACEMENT_SCATTER_SPEC`) but
had no PARAMS home for **manually-placed, human-authored** entity data — pre-placed army
units and named areas — even though `.sanmap` has real, live sections for both (`armies`,
`areas`) and v1 round-tripped both. Confirmed by `work_orders/RECIPE_PARITY_BACKLOG.md` Tier 1
and the standing §8.4-compliant scope notes in `ArmiesTab_UI.h` / `AreasTab_UI.h`.

- **Naming derivation:** §1.8 (new naming law this ruling also adds).
- **Full field lists, the recursive-tree structural ruling, the `legacyTypeTag`
  passthrough ruling, and the live-engine out-of-scope note:** `ENTITY_AUTHORING_PARAMS_SPEC`.
- **These are pass-through types, not procedural rules** — no PROC stage computes or
  reinterprets any field on `Army`/`UnitGroup`/`UnitTransform`/`MapArea`; they exist purely
  for round-trip fidelity through `IO` and direct authoring through `UI`. They are therefore
  distinct from, and additive to, `Params::UnitRule` (which remains the procedural scatter
  rule for armies) — both are legal producers into the same `armies[]` roster.
- **Shape only, not wiring.** This ruling and its spec fix the C++ shape of the four new
  types. Adding `std::vector<Army> armies;` / `std::vector<MapArea> areas;` to
  `MapRecipe_PARAMS.h`, the matching `IO` round-trip, and retiring the two UI scope notes
  are a separate coder work-order (`ENTITY_AUTHORING_PARAMS_SPEC` "Where these land").

---

## 10. `Params::Atmosphere` (ARCH ruling, ratifies `ATMOSPHERE_PARAMS_SPEC`)

Realizes the `Atmosphere_PARAMS` placeholder §5.1 already named in the `GenerationParams`
dismemberment list, by promoting the already-field-complete, 49-field `Ui::AtmosphereSettings`
(`src/ui/AtmosphereSettings_UI.h`) to a real recipe type. Confirmed against
`SANMAP_FORMAT_SPEC` "Top-level map fields" (Lighting / Background-fog / Global-wind) and the
legacy exporter (`Export_Metadata.cpp:149-221`): every field is an existing, live, camelCase,
format-native `.sanmap` key (or its already-established legible expansion, e.g. `sunRA` →
`sunRightAscension`) — **not** a new SanGen-owned schema-v3 section, so §1.6 does not apply and
no `IO_MIGRATION_SPEC` version gate is needed (the keys already round-trip today; this only
gives them a `Params::` home).

- **Shape:** one `Params::Atmosphere` aggregator (`sun`, `skylight`, `exposureSkybox`,
  `legacyFog`, `backgroundFog`, `heightFog`, `linearFog`, `globalWind`) composed of 8 named
  sub-structs — composition per §7.1 ("composition is allowed; rival top-level types are not"),
  the same pattern as `Stratum`/`StratumAppearance`/`StratumSoilPhysics`.
- **File split — RULED: all 8 sub-structs get their own `_PARAMS` header, none merged into
  the aggregator**, even the 2-3 field ones (`AtmosphereSkylight_PARAMS.h`,
  `AtmosphereGlobalWind_PARAMS.h`). `Water_PARAMS.h` (4 fields, its own file) is standing
  precedent that a small struct still gets its own file in this codebase; uniform
  one-struct-per-file beats an asymmetric "some inlined, some not" layout. Full reasoning:
  `ATMOSPHERE_PARAMS_SPEC`.
- **The one retype:** `skyboxIntensityModeIndex` (raw `int`) becomes
  `skyboxIntensityMode : SkyboxIntensityMode`
  (`enum class SkyboxIntensityMode { Exposure, Lux, Multiplier }`, added to
  `GenerationEnums_PARAMS.h`) — a §1.8 "retype a format-style category int" call, same
  precedent as `Army::faction`/`MarkerRule::category`. The `Index` suffix is dropped on
  retype, matching how `faction`/`category` are named (not `factionIndex`/`categoryIndex`).
  Every other field name is copied **verbatim** from `AtmosphereSettings_UI.h` — this is the
  only rename in the whole promotion.
- **Not promoted:** the UI-only dropdown label array (`skyboxIntensityModeLabels`) and the
  slot-addressing helpers (`AtmosphereColorAt`/`AtmosphereVectorAt`/`AtmosphereTextAt` and
  their slot enums) are `Ui::AtmosphereSettings`-specific widget-table plumbing, not settings
  — they stay in `UI` (Constitution §1) and are not ported.
- **Shape only, not wiring.** `MapRecipe_PARAMS.h` gaining `Atmosphere atmosphere;` (flat
  sibling of `water`), the `GenerationEnums_PARAMS.h` edit, the matching `IO` round-trip
  against the already-live `mapdef["sun*"]`/`["skylight*"]`/`["skybox*"]`/`["fog*"]`/
  `["background*"]`/`["heightFog*"]`/`["linearFog*"]`/`["windSpeed"/"windDirection"]` keys,
  and retiring `AtmosphereSettings_UI.h`'s promotion scope note are a separate coder
  work-order (`ATMOSPHERE_PARAMS_SPEC` "Where these land").

## 11. `Params::GlobalMarkerSettings` (ARCH ruling, completes `SANMAP_FORMAT_SPEC` Correction 7)

Fills the C++-shape gap in the already-ratified `.sanmap` `GlobalMarkerSettings` sub-key
(`SANMAP_FORMAT_SPEC` Correction 7, `PLACEMENT_SCATTER_SPEC` "IO wrapping") — map-wide default
icon/color/scale for the three resource marker kinds (Alloy/Plasma/Spawn), distinct in scope
from any single `Params::MarkerRule` (the same global-vs-per-rule distinction as
`Symmetry`/`SlopeDefaults` vs. their per-rule overrides). Confirmed against the legacy
reference `core/Parameters.h:79-87`.

- **New standalone file, `GlobalMarkerSettings_PARAMS.h`, sibling of `MarkerRule_PARAMS.h`**
  — not a member of `MarkerRule`, because it is map-scoped, not per-rule.
- **Shape:**
  ```cpp
  struct GlobalMarkerSettings {
      std::string iconNameAlloy  = "Alloy";
      std::string iconNamePlasma = "Plasma";
      std::string iconNameSpawn  = "Spawn";
      float colorAlloy[4]  = {0.8f, 0.8f, 0.2f, 1.0f};
      float colorPlasma[4] = {0.2f, 0.8f, 0.8f, 1.0f};
      float colorSpawn[4]  = {0.8f, 0.2f, 0.2f, 1.0f};
      float scaleAlloy  = 0.17f;
      float scalePlasma = 0.17f;
      float scaleSpawn  = 0.17f;
  };
  ```
- **Naming:** icon fields are `iconName*`, not `icon*Path` — they are atlas-manifest name
  keys (`name → { page, uv-rect }`, `ASSET_LOADING_SPEC`), not file paths. Color/scale fields
  drop the redundant `Marker` prefix the legacy globals (`MarkerColorAlloy`,
  `MarkerScaleAlloy`, …) carried — the type's own name already scopes them, and the bare
  `color*`/`scale*` reads cleanly as `Params::GlobalMarkerSettings::colorAlloy`.
- **`Plasma` = Energy, a real planned resource type** (already ruled, `SANMAP_FORMAT_SPEC`
  Correction 7) — not the v1 invention `IO_PARITY_REPORT.md` Decision #5 flagged; keep all
  three Plasma-named fields.
- **Shape only, not wiring.** `MapRecipe_PARAMS.h` gaining
  `GlobalMarkerSettings globalMarkerSettings;` (flat sibling of `markerRules`, for now — the
  future `MarkersStack` Group/Layer wrapper, `PLACEMENT_SCATTER_SPEC`/`SANMAP_FORMAT_SPEC`
  Correction 7, may fold this inside it later; not designed here) and the matching `IO`
  round-trip are a separate coder work-order.

## 12. Manual-layer authoring for props/decals — `layerIndex` + `PropGroups`/`DecalGroups` (ARCH ruling, revises `ENTITY_AUTHORING_PARAMS_SPEC`)

Fills the gap `PropsTab_Manual_UI.h`'s SCOPE NOTE 1 named: v1's manual prop GROUPS (hand-placed
props kept as named groups after import) had no `_PARAMS` home — `MapRecipe` carried scatter
RULES only. **Ratifies Option B, direct field injection, over the alternative of contiguous
index-ranges in a separate key.**

- **Decisive rejection of the range-based alternative.** A contiguous index-range key (e.g.
  "layer 0 owns `transforms[0..40)`") has a real silent-corruption failure mode: if any external
  tool (hand-editing, or the real Unity map editor — both supported workflows for this format)
  reorders an entry in `transforms[]` without changing the count, the ranges stay internally
  self-consistent while silently misattributing instances to the wrong layer, undetectable by any
  validation. `layerIndex` traveling directly with the instance has no such failure mode — external
  reordering cannot desync it. Cost is trivial (~1 MB even at Forge's 63.5k prop instances,
  `SANMAP_FORMAT_SPEC`'s 23-map survey) against the file's actual bulk, which is textures, not
  JSON.
- **`PropTransform`/`DecalTransform` become real, named wrapper types**, superseding
  `ENTITY_AUTHORING_PARAMS_SPEC`'s earlier "props/decals need no wrapper type" ruling — that
  ruling's premise (zero fields beyond `InstancedTransform`) no longer holds:
  ```cpp
  struct PropTransform  { InstancedTransform transform; int layerIndex = 0; };
  struct DecalTransform { InstancedTransform transform; int layerIndex = 0; };
  struct PropInstanceGroup  { std::string blueprintPath; std::vector<PropTransform>  transforms; };
  struct DecalInstanceGroup { std::string blueprintPath; std::vector<DecalTransform> transforms; };
  ```
- **`layerIndex` does NOT go on shared `InstancedTransform`.** It would leak onto
  `MarkerTransform`'s composed member and every future consumer of the base that has no concept
  of a manual layer. JSON key `layerIndex`, lowerCamelCase, merged directly into the existing
  transform object — the same rule already governing `armyColor`/`alias` (§1.6 Correction 0),
  §1.8's format-derived-field data-KIND rule.
- **Separate layer-metadata array**, one per domain, new top-level schema-v3 PascalCase keys
  `PropGroups`/`DecalGroups` (`SANMAP_FORMAT_SPEC` Correction 14):
  ```cpp
  struct PropInstanceLayer  { std::string name; float color[4]; float iconScale = 1.0f; };
  struct DecalInstanceLayer { std::string name; float color[4]; float iconScale = 1.0f; };
  ```
  Not `PropLayers`/`DecalLayers` — `PropsStack`/`DecalsStack`'s Group→Layer(rule) procedural
  hierarchy (`SANMAP_FORMAT_SPEC` Correction 7) already uses "Layer" for something unrelated (a
  rule inside a procedural Stack); reusing the word here would collide two different concepts.
  `ManualPropGroup` is also the already-live identifier in `src/ui/PropsTab_Manual_UI.h`, so
  `PropGroups`/`DecalGroups` picks up an existing name rather than inventing one.
- **Import validation:** `layerIndex` out of range against the corresponding `PropGroups`/
  `DecalGroups` array size is a loud, logged clamp to `0` (Constitution §6), never a hard refusal
  — this is authoring-convenience metadata, not gameplay-authoritative data. A missing
  `layerIndex` key on an older/foreign file degrades for free to `0` (the field's own default).
- **General principle, binding beyond this ratification (also recorded in §1.8):** a
  format-native object gains a small SanGen field by **direct injection** when the field is
  genuinely novel information with no competing home (`armyColor`, `alias`, `layerIndex`); a
  **separate SanGen-owned array** is used only when the metadata is richer than a scalar AND no
  format-native group container already exists to hold it (`PropInstanceLayer`/
  `DecalInstanceLayer`). One consistent rule, applied per case — it does not reopen `armyColor`/
  `alias`/`MarkerTransform::alias`, all already-settled instances of the first branch.
- **Shape only, not wiring.** `MapRecipe_PARAMS.h` gaining `std::vector<PropInstanceLayer>
  propLayers;` / `std::vector<DecalInstanceLayer> decalLayers;`, the matching `IO` round-trip, and
  reconciling `Ui::ManualPropGroup` (`PropsTab_Manual_UI.h`) against this new durable
  `Params::PropInstanceLayer` home are separate coder/UI work-orders — not designed here. Full
  detail: `ENTITY_AUTHORING_PARAMS_SPEC`.

## 13. Radial N-fold symmetry — `SymmetryAxis::Radial` + `radialSymmetryRepeatCount` (ARCH ruling, amends `Symmetry_PARAMS.h`, `SANMAP_FORMAT_SPEC` Correction 4)

- **New bit:** `constexpr int Radial = 1 << 4;` in the `SymmetryAxis` namespace
  (`src/params/Symmetry_PARAMS.h`). Confirmed by direct code read
  (`src/proc/Placement_Symmetry_PROC.h`'s `BuildSymmetryOrbit`) that every set bit in the mask is
  already composed independently in sequence (`MirrorAcrossX` → `MirrorAcrossZ` →
  `RotateHalfTurn` → `QuarterTurns`, each via its own `AppendTransformedSet`/`AppendQuarterTurns`
  call) — arbitrary combination of `Radial` with the existing bits is therefore already
  structurally supported by the orbit builder's shape; **no PROC combination-logic change is
  needed for this ratification.** `Radial`'s own orbit-generation function (the N-way analog of
  the existing `AppendQuarterTurns` helper, generalized from its hardcoded 3 turns to a
  designer-chosen turn count) is new PROC work for a future coder work-order — not designed here.
- **Companion count field**, a flat sibling wherever `symmetryMask` already lives (NOT a wrapper
  struct — matches the existing `bSymmetryUseGlobal`/`symmetryMask` flat-sibling convention):
  ```cpp
  int radialSymmetryRepeatCount = 3;
  ```
  Each independently-overridable mask needs its own `N`: `MapRecipe::globalSymmetryMask`,
  `MarkerRule::symmetryMask`, `PropRule::symmetryMask`, `UnitRule::symmetryMask`, and — once
  Defect 1 below is fixed — `DecalRule::symmetryMask`, plus the future `HeightmapStack`
  `GeoLayer`/`Layer` override (`SANMAP_FORMAT_SPEC` Correction 3). A local override with
  `bSymmetryUseGlobal = false` but no local count would otherwise silently inherit the global `N`,
  defeating the point of a local override.
- **JSON key `RadialSymmetryRepeatCount`, PascalCase** — matches the confirmed-live
  `SymmetryMask` key convention in `MapExporter_Rules_IO.cpp`. Lands in `SANMAP_FORMAT_SPEC`
  Correction 4's `Symmetry` global-section field list beside `GlobalSymmetryMask`, and as a
  per-rule sibling of `SymmetryMask` on each `MarkersStack`/`PropsStack`/`DecalsStack`/
  `UnitsStack` rule entry.
- **Default axis change:** `MapRecipe::globalSymmetryMask`'s default becomes
  `SymmetryAxis::RotateHalfTurn` (was `SymmetryAxis::None`, `MapRecipe_PARAMS.h:31`) — the
  existing "Point" bit; no new bit needed for this default.
- **Default blend — forward-attached requirement on a standing reservation, not built now.**
  `Params::SymAlgorithm` does not exist in `src/` yet (confirmed zero matches) — it remains
  `SANMAP_FORMAT_SPEC` Correction 4's own reserved, deferred field. Whichever future work-order
  defines `Params::SymAlgorithm{Fold, Blur, CrossFade, Superposition, Cylinder3D, Torus3D, ...}`
  **must default it to `Superposition`.** Recorded here and in `SANMAP_FORMAT_SPEC` Correction 4
  so the requirement is not lost between now and that work-order.

**Two defects recorded this session, not fixed here** (out-of-scope code gaps for a future coder
work-order; full detail in `SANMAP_FORMAT_SPEC` Correction 4 and `PLACEMENT_SCATTER_SPEC`'s
"Known issues" addendum):
1. **`DecalRule` has no `bSymmetryUseGlobal`/`symmetryMask` pair at all**
   (`src/params/ScatterRule_PARAMS.h`) — contradicted `SANMAP_FORMAT_SPEC` Correction 4's prior
   claim that the pattern was "already live and tested on `MarkerRule`/`PropRule`/`DecalRule`";
   that claim was factually wrong for `DecalRule` and is corrected in this same session.
   `AppendDecalRules` (`src/proc/Placement_Rules_PROC.cpp`) also never calls `ResolveSymmetryMask`
   for decals, so decals currently generate with **no symmetry at all**, not even the global
   default — a real functional gap, not merely a missing field.
2. **Symmetry-clone buffer overflow risk.** `Params::symmetryOrbitMaximum = 16`
   (`Symmetry_PARAMS.h`) backs a fixed-size stack array (`SymmetryOrbitPoint orbit[16]`,
   `src/proc/Placement_Accept_PROC.cpp:33`) sized for the old maximum combination (mirror X ×
   mirror Z × quarter turns). A designer-chosen `radialSymmetryRepeatCount` combined with mirrors
   can now exceed 16 (e.g. 8-fold × MirrorX × MirrorZ → up to 32), and the buffer **silently drops
   excess clones** rather than erroring — a real correctness gap this ratification creates by
   making a larger orbit reachable from PARAMS/UI. Raising the cap and/or adding a loud validated
   clamp on the designer-facing `N` (Constitution §6) is PROC/buffer-sizing work for a future
   Compute Optimization Expert or Generator Expert work-order — not sized here.

**UI finding, not ARCH's or this ratification's to fix — flagged for the UI Expert.**
`src/ui/SymmetryTab_UI.h`'s existing `SymmetryAxisOption::Radial` option (part of the plan's five
exclusive choices — Point/X/Z/XY/Radial) currently maps to `Params::SymmetryAxis::QuarterTurns`
(confirmed by code read, `SymmetryAxisMaskOfOption`) — a stand-in used because no true N-fold bit
existed yet. **This mapping is now stale**: it must be remapped to the real
`SymmetryAxis::Radial` bit this ratification adds, or the tab's "Radial" checkbox will keep
producing 4-fold `QuarterTurns` instead of the designer's chosen N-fold repeat. This is separate
from, and additional to, the already-known finding that `SymmetryTab_UI.h`'s exclusive-checkbox
row (unlike `PlacementRuleSections_UI.h`'s per-rule OR-able tick boxes) cannot express combined
axes at all — both are UI-layer reconciliation work for the UI Expert, not decided here.

## 14. Preview overlay layering — six-domain screen-space compositor (ARCH ruling, ratifies `work_orders/DESIGN_MarkerPreviewLayering_R2.md`)

Closes and widens the hit-list #4 gap `PREVIEW_COMPOSITING_SPEC` already recorded ("Decals never
composited"): today `Props`/`Units`/`Decals`, all resolved in `Data::PlacementResults`, **never
reach the canvas at all** — a materially bigger gap than an earlier, superseded round
(`work_orders/DESIGN_MarkerPreviewLayering_R1.md`, historical only, do not consult as current)
assumed under a markers-only framing. This ruling covers **six** dynamic overlay domains — Alloy,
Spawns/Armies, Units, Props, Reclaim (not in-game yet; slot reserved), Decals — kept open to
adding more without a code-shape change.

### 14.1 Module boundary and the DATA-vs-PARAMS split
`OverlayLayer_UI`/`overlayLayers` is `UI`, the same precedent as `PreviewCompositeSettings::
fieldLayers` (`PreviewComposite_Settings_UI.h`) — session-only presentation, not
recipe-serialized (§14.5). Two kinds of sub-layer, never conflated:
- **Procedural sub-layers** reuse the existing DATA columns (`PlacementInstance_DATA.h`'s
  `ruleIndex`/`category`) — no new DATA field.
- **Manual sub-layers** never touch DATA at all — they read `Params::MapRecipe` pass-through
  arrays (`PropInstanceLayer`/`DecalInstanceLayer` §12, `Army.groups` §9,
  `ENTITY_AUTHORING_PARAMS_SPEC`) directly, filtered by their own existing identity field.

GPU-resident overlay draw state (vertex buffers, atlas bindings) routes through the existing
`GpuResource_SYS` (`DISPATCH_INTERFACE_SPEC` §3) — a UI-owned GL pipeline is a named v1 defect
class (§3.2, §5.4) and must not reappear here.

### 14.2 Data model (binding shape)
```cpp
enum class OverlayDomainKind_UI   { Alloy, SpawnsArmies, Units, Props, Reclaim, Decals }; // open/additive
enum class OverlaySubLayerKind_UI { Manual, ProceduralRule };

struct OverlaySubLayerRef_UI { OverlaySubLayerKind_UI kind; int index; bool bEnabled = true; };

struct OverlayLayer_UI {
    std::string name;
    OverlayDomainKind_UI domainKind;
    bool bEnabled = true;
    float opacity = 1.0f;                             // layer-wide alpha multiplier, folded into
                                                        // each instance's tint alpha at draw time —
                                                        // replaces blendMode, §14.13 item 5 (closed)
    std::vector<OverlaySubLayerRef_UI> subLayers;     // any mix/count of Manual + ProceduralRule
    float thumbnailLodThresholdPixels = 5.0f;         // §14.3
    // color[4]/iconScale intentionally NOT always here — §14.5
};
std::vector<OverlayLayer_UI> overlayLayers;           // vector order = Z order, View-toolbar stack
```
A layer's drawn set is the union of every `bEnabled` sub-layer's resolved instances; one opacity
multiplier applies to the whole layer — **not** `Ui::PreviewBlendMode` (UI Expert verdict,
§14.13 item 5, closed: `Ui::PreviewBlendMode` is a two-operand GPU raster-compositing enum wired
into the GPU composite shader as integer defines — meaningless for a textured-quad icon draw
under ImGui's one global blend equation, and a per-layer blend-equation switch would break the
bulk-batched-vertex-write model §14.9 mandates). Reorder/add/remove never touches a fixed enum or
switch statement — this indirection is the entire point of the sub-layer shape.

Sub-layer → data mapping (binding; not to be re-derived per domain in a work-order):

| Domain | Manual sub-layers | Procedural sub-layers |
| --- | --- | --- |
| Props | `recipe.propLayers[i]` (`PropInstanceLayer`) | `recipe.propRules[i]` |
| Decals | `recipe.decalLayers[i]` (`DecalInstanceLayer`) | `recipe.decalRules[i]` |
| Units | one sub-layer per top-level `Army.groups[name]` — **flat**, §14.4 | `recipe.unitRules[i]` |
| Alloy / Spawns-Armies | ⚠️ blocked — no `MarkerInstanceLayer` PARAMS type exists yet (`work_orders/BRIEF_MarkersTabUI.md`); single undifferentiated Manual bucket until it lands. The struct shape above already splits to N once it does. | `recipe.markerRules[i]`, filtered by `category` (Spawn vs. rest), §14.6 |
| Reclaim | n/a — no data yet | n/a — no rule type yet; slot reserved, zero cost until it ships |

Sub-layer authoring (add/remove/toggle) lives in each domain's own tab (Props/Decals/Armies/
Markers) — never the View toolbar, which only orders/blends/hides whole `OverlayLayer_UI`s.

### 14.3 Icon rendering — two-mode LOD, not constant-screen-size-only
R1's framing — markers are always constant-screen-size icons — is **wrong and is retired.** Each
layer switches between two draw modes at its own `thumbnailLodThresholdPixels` (default 5px,
tunable, Constitution §8):
1. **Thumbnail mode** (zoomed in enough) — the entity's raster thumbnail at its true
   world-footprint size: `screenSize = (baseFootprint * instance.scale) / worldUnitsPerCell *
   pixelsPerCell * view.ZoomScale()`. Scales with zoom, by design.
2. **Strategic icon mode** — when thumbnail mode would render below the threshold, switch to a
   fixed-size symbolic icon. Constant screen pixels below threshold — this is the only mode R1's
   retired assumption ever actually covered, now correctly scoped to this mode alone.

Real, currently-unsolved gaps, recorded so no coder papers over them with an invented default:
- **No world-footprint-size data exists anywhere in the codebase today** (`InstancedTransform`
  carries a scale *multiplier*, not an absolute size) — needs a new `templateIdentifier ->
  baseFootprintWidth/Depth` table, IO-layer, asset-derived not PARAMS-authored. Buildable now with
  a placeholder default per domain; real mesh-derived bounds are separately-scoped later work
  (§14.13 item 1).
- Today's prop thumbnail (`AssetAtlasCache_PropThumbnail_IO.cpp`) is a placeholder flat-shaded
  stand-in derived from a digest of the model bytes, not a real rendered view — its own header
  already says so; unchanged and out of scope here.
- A strategic icon per entity type is **new authored visual content**, not a second render of
  existing data. **Decided: bespoke per blueprint** — every `templateIdentifier` gets its own
  authored strategic icon, not a generic one-glyph-per-domain fallback. This is real
  authoring/asset-pipeline work, out of this ruling's scope, not a rendering detail.
- `IconAtlasEntry`/`IconAtlasManifest` (`IconGridWidget_UI.h`) stays one `iconId` -> one UV rect;
  do not widen it — its only other consumer, the icon-picker grid, wants exactly one slot. Add a
  **separate pairing lookup** the overlay renderer consumes: `templateIdentifier ->
  {thumbnailIconId, strategicIconId}`, each id still resolving through the existing single-slot
  manifest unchanged. The widget's own header already names this exact seam as anticipated.

### 14.4 Nested `UnitGroup` addressing is flat
Top-level `Army.groups[name]` is one sub-layer; nested `UnitGroup.groups` draw as part of their
top-level parent, never separately addressable. This keeps `OverlaySubLayerRef_UI` uniform across
every domain (no domain-specific recursive-index special case) and avoids the "flattened
pre-order index into a mutable recursive tree" corruption class `ENTITY_AUTHORING_PARAMS_SPEC`
already ruled against elsewhere. Confirmed (Format Expert) this mirrors the official `.sanmap`
format's own `Army.groups`/`UnitGroup.units`/`.groups` tree 1:1 — not a SanGen invention.
Recursive addressing is a legitimate future ask; it needs its own ratification if actually
requested later.

### 14.5 View-stack state — split by field, not one blanket policy
- **Order / `bEnabled` / opacity:** session-only UI presentation — same policy already
  governing `PreviewCompositeSettings` (v1's serialized `PreviewLayers` was already a named
  defect to replace, not evidence v2 must re-serialize).
- **`color`/`iconScale`:** **not** a blanket UI-only field. Where a domain already owns a
  recipe-serialized layer-metadata record — Props/Decals (`PropInstanceLayer`/
  `DecalInstanceLayer`, §12) — `OverlayLayer_UI` reads/writes that record directly. No shadow
  copy, no second source of truth. Where no such PARAMS record exists yet (Alloy/SpawnsArmies,
  Units), these stay UI-session defaults until a future ratification gives them a real home —
  mirror the shipped Props/Decals pattern, do not invent a new one now.

### 14.6 `OverlayDomainKind_UI` vs `MarkerCategory`/`PlacementResults` — sits alongside, changes neither
`Alloy`/`SpawnsArmies` re-slice the existing `markers` buffer by its existing `category` column.
A UI enum may re-slice an existing DATA collection by its own field without the DATA shape
changing — zero blast radius on `MarkersTab_Rules_UI.h` or marker import/export. ⚠️ Domain-kind
is **asymmetric** versus DATA buckets — it splits markers 2 ways but maps Props/Units/Decals 1:1
— a coder must not assume `domain == DATA-bucket identity`.

### 14.7 View toolbar — replaces "Regenerate," one popup / two non-crossing sections
"View" opens a click-to-open popup (not hover — hover-close would fight a drag-reorder gesture),
`ImGui::BeginPopup("ViewLayersPopup")` rendering two independent `DraggableList` calls (the same
widget `LayersTab` already uses for GeoLayers) separated by a static section label:
- **"Terrain (composited)"** — `PreviewCompositeSettings::fieldLayers`.
- **"Overlays (screen-space)"** — `overlayLayers`.

Terrain rows carry their own blend-mode `Combo_UI` (`PreviewCompositeSettings::fieldLayers`'s
real GPU blend-equation switch into the composite shader — unchanged by this ruling). Overlay
rows carry an **opacity slider** instead (§14.2, §14.13 item 5, closed) — there is no
per-overlay-layer blend-equation switch; every overlay layer shares ImGui's one global blend
equation. Reorder is real *within* each section; **a row cannot cross sections** — true
interleaving (a marker rendering "under" a terrain layer) is rejected outright: it is not
renderable without either re-baking markers into the texture (the exact bug this whole redesign
kills) or rebuilding `PreviewComposite` into an interleaved multi-target compositor, and a control
that *looks* interleaved but isn't would violate the WYSIWYG law by showing an order that is not
the real render order. Mechanism: the two `DraggableList` renders use different drag-payload
identifiers so cross-section drops structurally fail to match — no new validation code needed. No
new widget; straight reuse.

**"Regenerate" is retired from the primary toolbar.** `Pipeline::PreviewDriver` already
auto-derives refresh tier from parameter hashes (`NotifyParametersChanged()`); a manual full-regen
button is the exact anti-pattern that system exists to replace. `MapCanvas::
RequestRegeneration()` and `PreviewDriver::RequestMapUpdate()` are currently **two rival trigger
paths** — per hit-list #3's "retire rival toggles" (applied here to a UI-level trigger, not a
compute backend), these **must collapse to one call path.** Keep exactly one debug/System-panel
affordance calling `RequestMapUpdate()` directly, for the one legitimate manual case
`PreviewDriver`'s own docstring already names ("a change no parameter hash can see: a resize, a
recipe reload, new stratum art") — not on the View toolbar.

⚠️ **R2 self-inconsistency found in the source document, flagged rather than silently resolved.**
R2's own "ARCH rulings (this round)" item 4 reads the fieldLayers/overlayLayers unification
question as still-open ("not ratifiable as scoped... route back to UI Expert"). A later section of
the *same document* ("View toolbar") states the UI Expert's dedicated pass already ran and records
the two-section/no-crossing design as "**Confirmed by human**." R2's own "Consolidated ❓ open
items" list (item 1) was never updated to drop this after that later resolution — it still reads
"sent back to UI Expert for a dedicated pass (in progress)." This ARCH ruling treats the later,
explicitly human-confirmed text as authoritative (the strongest ratification signal anywhere in
the document) and treats the two-section/no-crossing design above as **closed law**, not open —
but the inconsistency itself is recorded here rather than quietly picked one way.

### 14.8 Dirty-flag tiers — four, not two (extends `PREVIEW_COMPOSITING_SPEC`'s existing two-tier model)
| Tier | Trigger | Cost |
| --- | --- | --- |
| A — Full regen | Sim/recipe param changed | Unchanged (PROC) |
| B — Full recomposite | Terrain/water/stratum layer setting changed | Unchanged pass sequence. Cost is resolution-dependent, not one number — sub-ms-to-low-ms credible at the 512² default, plausibly several-to-10ms+ at the 8192² cap (256× the pixel work). **Rough-estimate; must be benchmarked at both, never shipped as one range** (Constitution §7 basis-tag law). |
| C — Screen-space redraw | Every overlay layer, every frame: pan/zoom/hover/visibility/opacity/reorder | Zero GPU recompute, per-layer culled, bounded by the §14.9 cross-layer budget |
| C2 — Interaction-scoped redraw (new) | Active drag/edit on a marker or group | Cache non-selected instances' generated vertex+draw-command bytes once at gesture-start (CPU bytes, not a GPU texture/FBO), replay via memcpy each frame, regenerate live only the selection. Invalidates on pan/zoom/selection-change/layer-setting-change mid-gesture. |

Reorder/opacity changes in the overlay View stack are O(layerCount), never O(instances) — opacity
is a per-vertex tint-alpha multiply, already covered by the C2 table's "layer-setting-change"
trigger above. **§14.13 item 5's resolution closes the open question this paragraph previously
flagged:** overlay layers carry `opacity`, not a per-layer blend-mode enum (§14.2), so every
overlay shares ImGui's one global blend equation and there is no divergent per-vertex
color-encoding (premultiplied vs. straight alpha) risk to confirm. The thumbnail-vs-strategic swap
still needs its own C2 invalidation check, independent of opacity. LOD threshold crossing during
zoom needs no new invalidation rule of its own: zoom already invalidates C2's cache
unconditionally.

### 14.9 Rendering/performance — mandatory in the first work-order, not deferrable
- **Bulk vertex writes only.** "Batched icon quads" means one bulk `ImDrawList::PrimReserve` +
  raw vertex/index writes per layer, **not** N individual `ImDrawList::AddImage()` calls —
  per-call overhead at 600k markers could plausibly cost 30–60ms, larger than the entire frame
  budget, independent of the transform math. This is a stated work-order requirement, not left to
  a coder's default imgui usage.
- **Cross-layer visible-vertex budget with automatic decimation** (screen-cell clustering, then
  priority-cap fallback) — mandatory in the first work-order, not deferrable. Rough-estimate
  placeholder default ~400,000–500,000 instances before decimation kicks in (derived from a
  3ms-of-16ms frame-budget target) — **explicitly a placeholder pending a real microbenchmark**
  (SIMD-transform, bulk-write, and naive-`AddImage` timed separately at N ∈ {100k, 300k, 600k},
  both 0%-culled and ~5%-visible, on real dev hardware, before this number becomes a ratified
  constant per Constitution §7/§12 basis-tag law), must ship as a named tweakable setting
  (Constitution §8), never a literal.
- **Atlas page bucketing is required.** Thumbnails for many distinct prop templates can legally
  scatter across many atlas pages (general bin-packed atlas, no same-page guarantee) — drawing in
  raw visit order risks draw-call count regressing toward O(pages touched) instead of O(layers).
  Fix: accumulate each visible instance's quad into a per-page bucket during vertex-gen, flush one
  draw command per non-empty bucket — bounds draw calls to O(pages touched this frame) regardless
  of visit order. Strategic-icon mode is naturally safe here (small fixed low-cardinality icon
  set) — put it on one dedicated always-resident page.
- Reuse the existing resident icon atlas (`Ui::IconAtlasManifest`) — already shared by
  Markers/Armies/Props pickers, already proven at 10k+ scale via `ImGuiListClipper`-style
  virtualization.
- Per-layer AABB early-out + per-layer `Data::SpatialGrid` (§8.3) for view-window culling. ⚠️ The
  grid gives **zero help** fully-zoomed-out (everything visible, every bucket queried) — that
  case is genuinely O(N); the cross-layer budget above is what bounds it, not the grid.
- **Layer-id column: do not physically resort `PlacementInstances` by layer.** Reuse the existing
  `ruleIndex`/`category` columns (`Data::PlacementInstances`, `PlacementResults_DATA.h`) via a CSR
  bucket index built once (same lifecycle point as `Data::SpatialGrid`'s build, right after
  Placement, §8.3) — per-layer flat index arrays, cached, rebuilt only when that layer's own
  sub-layer membership changes, not every frame. **Procedural Decals use this identical scheme,
  confirmed** (§14.13 item 4, closed): `Data::PlacementResults::decals` is the same
  `Data::PlacementInstances` SoA type with the same `ruleIndex`/`category` columns
  (`Placement_PROC.cpp:64` `CollectionFor(3)`, `Placement_Rules_PROC.cpp:104-138`
  `AppendDecalRules`, `Placement_Kernel_PROC.h:52` collection index 3) — no special-case needed.
  One of R2's own open items still bears directly on this scheme and is **not** resolved here —
  §14.13 item 3 (manual sub-layer stable-id; see its sharpened problem statement).

### 14.10 GPU color-texture readback bug (recorded, separate narrow fix, lands first)
`ComposeOnGpu()` (`PreviewComposite_Gpu_UI.cpp:78-81`) unconditionally reads back the full color
texture even on the GPU-resident hot path where nothing downstream consumes it (confirmed:
`Application_UI.cpp` only consumes it `if (!composite.LastRunUsedGpu())`) — up to 256MB wasted
PCIe transfer plus a blocking wait at the 8192² cap, every recompose. The entity-id buffer
readback on the same lines is **not** dead — `MapCanvas_UI.cpp` click-picking reads it
unconditionally on both backends. Fix scope: gate only the color-texture readback on
`!bLastRunUsedGpu`; leave entity-id readback as-is. Independent of, and should land before, the
overlay redesign — a narrow, already-diagnosed defect that compounds with every future Tier B
trigger.

### 14.11 Determinism
Presentation-only — same Visual-class exemption `OPTIMIZATION_PILLARS.md` pillar 15 already
grants GPU-resident preview compositing (Constitution §4, `DETERMINISM_SPEC`). **Binding
guardrail:** any future screen-space decimation/clustering may only affect what is *drawn* — it
must never mutate or discard `PlacementInstances`, and must never feed back into export/bake. A
"helpful" LOD optimization silently becoming a second, non-deterministic placement decision is the
exact failure mode this sentence forbids.

### 14.12 Naming
`OverlayLayer_UI` / `OverlayDomainKind_UI` / `OverlaySubLayerKind_UI` / `OverlaySubLayerRef_UI` —
`_UI` suffix per §1.2, reflected throughout this section.

### 14.13 Open items — status as of this ratification (closed items marked)
R2's own "Consolidated ❓ open items" list, carried forward. Items 4 and 5 were closed this
session by direct expert consult; items 1-3 remain open. A coder or future ARCH pass must not
treat items 1-3 as settled by this ruling:
1. ⚠️ **Real footprint-size source:** placeholder-per-domain now (§14.3); who/when derives real
   mesh bounds is unscheduled.
2. ⚠️ **Cross-layer visible-vertex budget default and Tier B per-resolution costs** (§14.8-14.9):
   need the real benchmark named in §14.9, not the reasoned placeholders in this ruling.
3. ⚠️ **Manual sub-layer stable-id — a real DATA-shape work item, not a small column ask (open,
   sharpened this session).** Generator Expert consult found a two-part gap, bigger than §14.9
   originally assumed:
   - (a) `PropInstanceLayer`/`DecalInstanceLayer` (`src/params/PropInstance_PARAMS.h:30-31`)
     carry only `name`/`color[4]`/`iconScale` — no id field. The only backward reference,
     `layerIndex` on `PropTransform`/`DecalTransform` (`PropInstance_PARAMS.h:19-20`), is a
     **plain vector position, not a stable identity** — `RenumberPropLayerIndicesForReorder`
     renumbers it on drag-reorder and `ClampPropLayerIndicesForRemovedLayer` clamps it on delete
     (`src/ui/PropsTab_Manual_UI.cpp:43-67`, citing prior "STEP22 ruling #5").
   - (b) `Data::PlacementInstances` — the resolved runtime SoA §14.9's CSR scheme buckets
     against — **has no `layerIndex`-equivalent column at all** (confirmed,
     `PropsTab_Manual_UI.cpp:103`): no way exists today to correlate a resolved instance back to
     which manual layer authored it.
   - Needs: (a) a stable id added to `PropInstanceLayer`/`DecalInstanceLayer` that survives
     reorder/delete, distinct from the existing reorder-renumbered `layerIndex`; and (b) a new
     correlation column on `Data::PlacementInstances` (or a side table) recording which manual
     layer produced each resolved instance.
   - Separately, related: manual/authored decals (`Params::DecalInstanceGroup`/`DecalTransform`,
     `recipe.decals`) are pure round-trip-JSON PARAMS today, "NOT yet live-wired into
     `BuildSanmapJsonText`/`ParseSanmapJsonText`" (`MapRecipe_PARAMS.h:103-104`) — they do not
     resolve into `results.decals` at all, unlike the procedural path item 4 confirms.
   - Stays open. This is a real DATA-shape work item for a future work-order; not resolved here.
4. ✅ **CLOSED — Decals data source.** Confirmed (Generator Expert): procedural Decals already
   resolve into `Data::PlacementResults::decals` (`PlacementResults_DATA.h:11-15`), the identical
   `Data::PlacementInstances` SoA type with identical `ruleIndex`/`category` columns
   markers/props/units use (`Placement_PROC.cpp:64` `CollectionFor(3)`,
   `Placement_Rules_PROC.cpp:104-138` `AppendDecalRules`, `Placement_Kernel_PROC.h:52` collection
   index 0=markers/1=props/2=units/3=decals). No compositor currently reads them (confirmed: no
   `Decal` reference anywhere in `PreviewComposite_*`) — the gap is purely a missing draw-pass
   consumer, not a DATA-shape mismatch. `PREVIEW_COMPOSITING_SPEC`'s prior "Decals never
   composited" framing is corrected accordingly. The §14.9 CSR-bucket/`SpatialGrid` scheme applies
   to procedural Decals exactly as written, no special-case needed. Applies **only** to procedural
   decals (`recipe.decalRules`) — manual decals are the separate, still-open gap recorded in
   item 3.
5. ✅ **CLOSED — `OverlayLayer_UI::blendMode` retired; replaced by `opacity: float`.** UI Expert
   verdict: `Ui::PreviewBlendMode` (`src/ui/PreviewComposite_Settings_UI.h:26`) is a two-operand
   GPU raster-compositing enum (`Replace`/`AlphaBlend`/`Add`/`Multiply`/`Maximum`/`Minimum`) wired
   into the GPU composite shader as integer defines (`PreviewComposite_GpuProgram_UI.cpp:43-48`)
   — meaningless for an `ImDrawList::AddImage` icon draw (a textured quad with per-instance
   vertex-color tint under ImGui's one global blend equation). Forcing a per-layer blend-equation
   switch would require a custom render callback per overlay layer, breaking the bulk-batched-
   vertex-write model §14.9 mandates and turning the "zero GPU recompute" screen-space tier into a
   shader-state-change cost. §14.2's struct now carries `opacity: float` (0-1 layer-wide alpha
   multiplier, folded into each instance's tint alpha at draw time) in place of `blendMode`. A
   future additive-glow overlay kind is a narrow per-layer-kind flag, not a shared blend-mode
   enum — not designed in now.

R2's own open item 1 (fieldLayers/overlayLayers unification) is **not** carried forward on this
list — §14.7 above rules it closed, and records the R2 self-inconsistency that made this call
non-trivial.

## 15. The SanGen Map Scenario system — formalized as first-class law (ratifies `MAP_SCENARIO_SPEC.md`)

The game's per-army spawn position, alloy/mex marker visibility, and playable-area resolution —
for every player-count/composition a lobby can produce — is deterministically resolved once, at
map load, by the SanGen Map Scenario system: an `<MapName>_data.lua` orchestrator paired with an
`<MapName>_Scenarios_Script.lua` scenario module, both colocated in the engine's script tree
(`LJ/lua/maps/<MapName>/`), linked at runtime via
`Import("maps/<MapName>/<MapName>_Scenarios_Script.lua").Scenario`. **Status: DEPLOYED and
confirmed working live in-game (2026-08-20).** This ruling promotes that deployment to binding
ARCH law. Full contract — the two-file split, the module API (`ResolveAndApply`/
`SpawnNavalFleets`), the three-tier `PATTERN_SCENARIOS`/`COUNT_SCENARIOS`/`DEFAULT_SCENARIO`
matching system, the four `alloyMode` semantics (`explicit`/`occupancy`/`keepAll`/`delta`), the
§6 hard requirement that a scenario needing deterministic spawns must declare an explicit
`spawns` table, and the execution/timing law — lives entirely in `MAP_SCENARIO_SPEC.md`; it is
not re-derived here.

### 15.1 Layer classification
This system is **game-side Lua, not SanGen C++** — it runs inside the engine's own script
sandbox at map-load time, on the player's machine, not inside any SanGen process. It therefore
does not occupy a slot in the Constitution §1 `MATH`/`DATA`/`PARAMS`/`PROC`/`PIPELINE`/`IO`/
`UI`/`SYS` layer stack at all — those layers describe SanGen's own binary, and this is map
content the binary produces (or, per §15.2, may in future consume), not code SanGen executes.
Its prerequisite law (the `Import()` global-capture rule, the `LoadMapData()`/`CreateArmies()`/
`RunMapSetup()`/`NewThread` lifecycle) is itself game-engine law, not SanGen architecture, and is
recorded in `MODDING_SCRIPTING_SPEC.md` for that reason — a spec that documents the game's
scripting contract SanGen must respect, distinct from a spec governing SanGen's own module shape.

### 15.2 IO scope ruling — corrects an earlier assumption, does not reverse it
An earlier ratification recorded SanGen Import/Export of the Scenarios file as in scope, under
the assumption that the file would live in the map's **asset folder**
(`Sanctuary_Data/Maps/<MapName>/`) — i.e. inside the shippable `.sanmap` package SanGen's
`MapImporter_*`/`MapExporter_*` already read/write, an extension of the existing per-domain
`.sanmap` JSON convention (§1.6-adjacent). That assumption is now known wrong: the file lives in
the engine's **script tree** (`LJ/lua/maps/<MapName>/`, §15/§2 of `MAP_SCENARIO_SPEC.md`), a
location SanGen's importer/exporter does not address today and which is not part of the `.sanmap`
package at all.

**Ruling: still in scope, reclassified — not a yes/no reversal.** The scope call stands; what
changes is the *kind* of IO surface required. It is **not** an additional section inside the
existing `.sanmap` JSON document (unlike `PropGroups`/`DecalGroups`, `HeightmapStack`, etc.) — it
is a **separate companion artifact**, a `.lua` text file, at a **structurally distinct
filesystem location** from the map asset export folder. It therefore does **not** extend the
existing per-domain `MapImporter_<Domain>Stack_IO.cpp`/`MapExporter_*` convention
(`IO_MIGRATION_SPEC.md` §1) — that convention is scoped to JSON fragments of the one `.sanmap`
document — and needs its own convention, designed from scratch.

**Ownership: the SanGen IO Architecture Expert's domain**, not this ARCH's — how to structure the
new SanGen IO code, including any new file-type convention. Per the existing law already recorded
in `MODDING_SCRIPTING_SPEC.md`: no code is written until a work-order exists and is ratified; the
SanGen Coder writes zero code without one.

**❓ Open design question, not resolved by this ratification** — flag to the IO Architecture
Expert / human when this becomes live work: does SanGen literally round-trip the Lua text (parse
and regenerate the tiered scenario tables verbatim, preserving hand-authored comments and the
semantically-load-bearing `COUNT_SCENARIOS` ordering — hard, since Lua is not JSON), or does
SanGen instead own only the *parameterized scenario data* (a PARAMS/JSON structure) and render
that into `.lua` module text on export-only, never reading the `.lua` back in on import? The
latter avoids a Lua parser entirely and fits SanGen's existing PARAMS→IO write direction better,
but the choice is not decided here. Full detail and the "does SanGen know the game install's
`LJ/lua` root at export time" sub-question: `MAP_SCENARIO_SPEC.md` §8.
