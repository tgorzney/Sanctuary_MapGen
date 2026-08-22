[← ARCH index](ARCH.md) · SanGen ARCH §3. Part of the ratified v2 architecture; the Constitution (`sangen_arch_pack/CONSTITUTION.md`) and this file's preamble in `ARCH.md` bind alongside it. **Only the ARCH Expert writes this file.**

## 3. Module boundaries & ownership (Constitution §1, resolved)

### 3.1 Dependency direction (downward only, no cycles)
| Layer | May depend on | Never |
| --- | --- | --- |
| `MATH` | (nothing) | any other layer |
| `PARAMS` | (nothing) | any other layer |
| `DATA` | `MATH` | GPU/GL handles; PROC/UI |
| `PROC` | `DATA`, `PARAMS`, `MATH` | UI; owning a backend choice |
| `PIPELINE` | `PROC`, `DATA`, `PARAMS`, `MATH`, `SYS` | UI; drawing |
| `IO` | `DATA`, `PARAMS`, `MATH`, `SYS`¹ | simulating; PROC |
| `SYS` | `DATA`, `PARAMS`, `MATH` | knowing the pipeline shape |
| `UI` | `PIPELINE`, `DATA`, `PARAMS`, `SYS` | sim logic; touching PROC directly |

The canonical call chain: **`UI → PIPELINE → PROC → SYS`** (with PROC/PIPELINE reading
`DATA`/`PARAMS` and using `MATH`).

¹ **Correction, §15.8.** `IO`'s dependency on `SYS` was added by the §15.8 ratification
(`LuaSyntaxCheck_SYS`, the embedded-Lua validator both `IO` and `UI` must reach) — but it
formalizes a pre-existing real-code fact, not a new liberty: `src/io/AssetAtlasCache_IO.cpp`
already `#include`s `../sys/ThreadPool_SYS.h`, so `IO → SYS` was already true in the shipped
tree, undeclared here until now.

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
  `TerrainGenerator` god + the `main.cpp` regen loop. **It also owns the narrow, stateless
  "query passthrough" surface (ARCH §16.3) that lets `UI` legally reach a handful of pure
  PROC math functions without a DAG node** — a second, lighter kind of PIPELINE
  responsibility, distinct from stage orchestration, first named by that ruling.
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
