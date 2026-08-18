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
`DetailNormal`, `SanGenVersion` — the ratified schema v3, `SANMAP_FORMAT_SPEC`).

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

- **Pass-through, human-authored, verbatim entity data.** No PROC stage computes or reinterprets
  the value — round-trip fidelity is the field's entire purpose. Such a field uses **the format's
  own spelling by default**, converted only for case (`camelCase`) and the §1.1 `b`-boolean
  prefix. Governs `Army`, `UnitGroup`, `UnitTransform`, `MapArea`
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
