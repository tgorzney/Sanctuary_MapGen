# Work-Order M0-9 — `GpuResource_SYS` (GL programs, buffers, async dispatch)

*Schema-valid per Constitution §7. Milestone M0 (Foundation) — SYS group. Executor:
**SanGen Coder in Claude Code** (needs the real OpenGL toolchain — not sandbox-
verifiable). Status: spec ready; NOT yet implemented.*

## Title
The single OpenGL resource manager: compile-once programs, persistent buffers, async
dispatch — the GPU side of the dispatch contract.

## Root problem
GPU compute today is inconsistent and wasteful: `ErosionCompute.cpp` **recompiles the
shader from a hardcoded absolute path and re-uploads every dispatch**, then blocks on
`glMapBuffer` and tears everything down. `TerrainCompute.cpp` hardcodes
`"D:/Projects/.../MarkerCompute.glsl"`. `PreviewRenderer.cpp` does it right (persistent
`s_SSBOs`, `GL_MAP_UNSYNCHRONIZED`) but in the UI layer. There is no shared GL resource
owner. `Dispatch_SYS` routes GPU work to `kernel.RunOnGpu()`, but nothing provides the
managed GL resources that method needs.

## Target files
- `src/sys/GpuResource_SYS.h`
- `src/sys/GpuResource_SYS.cpp`  (split methods across `GpuResource_*_SYS.cpp` if the
  header/impl approaches the §1.5 ceiling)

## Layer & accuracy class
`SYS`. Accuracy N/A (resource plumbing). GPU/GL state lives ONLY here (Constitution §1 /
ARCH §3.2 — never in DATA/PARAMS/UI).

## Backend policy
The GPU backend infrastructure itself. Provides the resources the GPU kernels use; the
CPU/GPU choice is made by `Dispatch_SYS` (M0-8).

## ARCH rules invoked
- §1.1 fully-spelled names; §1.2 `_SYS` suffix; §1.5 ceilings (split impl across files
  as needed).
- ARCH §4 / `DISPATCH_INTERFACE_SPEC §3` — the resource-manager contract.
- §5 portability — keep GL behind this seam; do not leak GL handles upward.

## Solution (model on the GOOD existing pattern, not the bad one)
Provide a `GpuResourceManager` owning:
1. **Compiled programs, cached once.** Compile each compute shader a single time, keyed
   by name; never recompile per dispatch. Resolve shader paths **relative to a
   configured shader directory** (never a hardcoded absolute path — kill the
   `D:/Projects/...` literals). Report compile/link errors clearly (Constitution §6).
2. **Persistent SSBOs / textures**, allocated once and reallocated ONLY on resize —
   follow `PreviewRenderer`'s persistent-`s_SSBOs` + `GL_MAP_UNSYNCHRONIZED` approach,
   NOT erosion's per-dispatch create/destroy. One documented upload/readback path per
   data type (flatten → upload → barrier → optional readback).
3. **Async dispatch.** Use fences (`glFenceSync`/`glClientWaitSync`) instead of a
   blocking `glMapBuffer` on the hot path; the caller polls/awaits completion.
4. **One place for GL extension loading + boilerplate**; named workgroup-size constants
   shared between the GLSL `local_size` and the dispatch math (retire the duplicated
   literals: erosion 256, avalanche/terrain 16², markers 8²).
5. `std430` struct layouts declared once and shared (kill the `LayerPhysics` vs
   `vec4 physics[]` aliasing hazard).

A GPU kernel's `RunOnGpu()` (the method `Dispatch_SYS` calls) uses this manager to bind
its cached program + persistent buffers and dispatch — it never compiles or allocates
inline.

## Performance estimate (with basis)
Eliminates per-dispatch shader recompile (~ms each) and per-dispatch buffer alloc; async
fences remove the pipeline stall from blocking readback (*basis: the current
recompile-every-call is a known multi-millisecond cost; rough-estimate*).

## Lossy alternative
N/A (infrastructure).

## Acceptance test (run in Claude Code with GL)
- A compute program compiles once and is reused across many dispatches (assert the
  compile path runs a single time — e.g. a compile counter).
- SSBOs persist across dispatches and reallocate only when the size changes.
- A round-trip (upload → dispatch a trivial kernel that doubles a buffer → async
  readback) returns correct data with no blocking map on the hot path.
- No hardcoded absolute paths remain; shader dir is configurable.
- Builds clean in MSVC against the project's GL loader.

## Out of scope (explicit)
- Porting the actual erosion/noise/etc. GLSL kernels to use this — that is M3 per stage.
- The CPU/GPU decision (owned by `Dispatch_SYS`, M0-8).
- Vulkan/DX backends (GL only for the standalone target).

## Note to the executor
Read the current `gui/PreviewRenderer.cpp` (the persistent-buffer pattern to emulate),
`core/ErosionCompute.cpp` (the recompile-every-dispatch anti-pattern to remove), and
`core/TerrainCompute.cpp` (the hardcoded shader path to eliminate) before writing.
Match the project's existing GL loader/extension setup. This work-order was authored by
the ARCH-context session but is implemented where GL compiles.
