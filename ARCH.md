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
  file format or game dictates (`tpId`, `.sanmap` JSON keys, stratum names) — verbatim
  so import/export round-trips; and the universally-standard hardware acronyms `Cpu`
  and `Gpu`. Our own code around them spells fully.
- **Booleans keep the `b` prefix** (`bNeedsMapUpdate`) — retained precedent; the word
  after it is still fully spelled.

### 1.2 Layer tag is a SUFFIX (TGUE convention)
A file's **suffix** declares its Constitution §1 layer, and it must match the file's
folder (§2). Descriptive-name first, layer last:

| Suffix | Layer | Example files |
| --- | --- | --- |
| `_MATH` | MATH / SIMD | `Vector_MATH.h`, `Noise_MATH.h`, `Morton_MATH.h`, `Spatial_MATH.h` |
| `_DATA` | DATA — computed output | `Heightfield_DATA.h`, `MaterialMasks_DATA.h`, `FlowMap_DATA.h`, `Props_DATA.h` |
| `_PARAMS` | PARAMS — adjustable settings | `Layers_PARAMS.h`, `MarkerRules_PARAMS.h`, `Geometry_PARAMS.h`, `Enums_PARAMS.h` |
| `_PROC` | PROC processors | `Noise_PROC.cpp`, `Erosion_PROC.cpp`, `Mask_PROC.cpp`, `Placement_PROC.cpp` |
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

---

## 2. Layer → directory map (Constitution §1/§2)

One directory per layer; the folder and the file suffix always agree, so a misplaced
file is visible instantly. The `core/` + `gui/` split and the dead `core/data/` are
retired.

```
src/
  math/     *_MATH                 pure stateless math (SIMD, noise, vector, morton, spatial)
  data/     *_DATA                 computed SoA output (heightfield, masks, flow, resolved prop/marker/unit instances)
  params/   *_PARAMS               adjustable settings / the recipe (layer stack, rules, constants, seed) — serializes to mapGeneratorData
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

### 3.3 Ownership of the moving parts
- **`PIPELINE` owns generation orchestration** — the dirty-hash dependency DAG, the
  PROC stage order (noise → blend → mask → erosion → thermal → flow → placement →
  bake), and resolving the per-stage backend/accuracy policy (Preview vs Output,
  Constitution §4). It calls `Dispatch_SYS` to run each stage. Replaces the
  `TerrainGenerator` god + the `main.cpp` regen loop.
- **`SYS` owns the runtime** — `Dispatch_SYS` (router mechanism: run kernel K on
  backend B), `GpuResource_SYS` (programs compiled once, persistent buffers, async
  fences), `ThreadPool_SYS`, `ArenaAllocator_SYS`, `Log_SYS`. Knows *how* to run a
  kernel, not *which* stages exist.
- **`PROC` owns the kernels** — one math source per stage, CPU `.cpp` + GPU `.glsl`
  pair; declares its inputs/outputs (so `PIPELINE` can build the DAG and the two-tier
  dirty flags) and its accuracy class; requests a backend, never selects one.
- **`DATA`/`PARAMS` own output/settings** — DATA = plain SoA computed arrays; PARAMS =
  the adjustable recipe (serializes to `mapGeneratorData`). No behavior, no GPU handles.
- **`IO` owns the format seam** — `.sanmap` + `mapGeneratorData` round-trip, sanpack
  ingestion, asset validation (Constitution §6). The swappable port boundary (§5).
- **`UI` owns presentation** — tabs, the universal widget library, the preview
  composite; reads baked results and the resident atlas, writes params.

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

These are defaults; §8 tweakability lets any stage be overridden per project.

### 4.3 Backend resolution (how `Dispatch_SYS` picks)
1. `bDeterministic` set and the stage is Exact-class → **Cpu**, portable transcendental
   + ordered-reduction path (`DETERMINISM_SPEC`). (Visual stages ignore it.)
2. else the stage's `previewBackend`/`outputBackend` for the active context.
3. else the global backend setting.
4. `Automatic` → fastest **legal** backend for the declared accuracy class given data
   residency (no needless Cpu↔Gpu copies). Same accuracy class ⇒ backends must agree
   within that class's tolerance; a backend that cannot meet the class is not legal.

### 4.4 Idle escalation (Preview context)
During interaction the preview runs its fast path (Gpu/Visual). When input goes idle,
`PIPELINE` re-runs the affected stages at **Output** accuracy and swaps the result in —
so scrubbing stays fast but the settled image is truth. WYSIWYG holds because the
preview *samples* that bake; it never re-simulates (§3.2).

### 4.5 Determinism scope
`bDeterministic` makes only the **Exact-class, gameplay-authoritative** outputs
bit-identical across machines (heightmap incl. erosion, flow, placement/markers,
collidable props). Visual-class outputs stay on the fast Gpu path and may differ per
machine. Experimental until the cross-machine bit-exact gate passes (`DETERMINISM_SPEC`).

---

## 5. God-object dismemberment (hit-list #1–2)

Applying §3 boundaries and the input/output split (settings → `PARAMS`, computed arrays
→ `DATA`). Four offenders and where their pieces land.

### 5.1 `GenerationParams` → typed modules
- **settings → `PARAMS`:** `Layers_PARAMS` (the editable layer stack), `Stratums_PARAMS`,
  `MarkerRules_PARAMS`, `PropRules_PARAMS`, `Water_PARAMS`, `Atmosphere_PARAMS`,
  `Geometry_PARAMS` (dimensions + seed), `ErosionFlow_PARAMS`, `Symmetry_PARAMS`,
  `Environment_PARAMS`, `Enums_PARAMS`.
- **computed arrays → `DATA`:** `Heightfield_DATA`, `BlendedMap_DATA`,
  `MaterialMasks_DATA`, `FlowMap_DATA`, `AccumulationMap_DATA`, `Markers_DATA`,
  `Props_DATA`, `Units_DATA`, `Areas_DATA`, `EntityIdBuffer_DATA`, `SpatialGrid_DATA`,
  cached noise.
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
`GpuResource_SYS`; gradient LUT bake → `Gradient_PROC`; **sim/rule re-filtering →
deleted** (samples the bake, §3.2); picking readback → `Picking_UI`.

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
  `mapGeneratorData` round-trip through `IO` (`SANMAP_FORMAT_SPEC`).
- **M2 — Dispatch + PIPELINE skeleton** (hit-list #3): `DispatchPolicy`, `Dispatch_SYS`
  resolution, `Generation_PIPELINE` (DAG + dirty-hash). **Vertical slice on one stage
  (noise), both backends**, to prove the whole spine before fanning out.
- **M3 — PROC stages**: Noise/Blend → Mask → Erosion → Thermal → Flow/Accumulation →
  Placement → Bake. **Each stage built as a complete CPU + GPU pair and parity-checked
  together — a stage is "done" only when both backends produce in-class-equivalent
  results.** Not all-CPU-then-GPU; finish the pair, then the next stage.
- **M4 — Preview / WYSIWYG** (hit-list #4): `PreviewComposite_UI` samples the bake
  (shadow-sim deleted), `Picking_UI`, two-tier dirty flags derived from the DAG.
- **M5 — UI**: universal imgui-bypass widget library, tabs, `MapCanvas_UI`, and the
  asset pipeline (sanpack ingest → atlas → disk cache; `ASSET_LOADING_SPEC`).
- **M6 — Advanced / optional**: determinism mode + cross-machine bit-exact gate
  (`DETERMINISM_SPEC`); then future sim types (fluvial/glacial/snow-melt,
  `FUTURE_SIM_TYPES_SPEC`); then AI-analyzability validation + host/client
  (`AI_HOSTCLIENT_SPEC`).

### 6.1 Definition of done (per PROC stage)
A stage is complete only when: CPU **and** GPU implemented and parity-verified within the
stage's accuracy class; wired into `PIPELINE` + `Dispatch_SYS` (no rival toggle); all its
constants exposed as `PARAMS` (§8); files within the §1.5 ceilings; and its work-order
acceptance test (Constitution §7) passes.

