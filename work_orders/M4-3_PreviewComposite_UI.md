# Work-Order M4-3 — `PreviewComposite_UI` (WYSIWYG composite)

*Constitution §7. Milestone M4. **BATCH 2 (parallel, after Batch 1).** Depends on M4-1 +
M4-2 headers (read-only include) + existing `MapFields_DATA` / `GpuResource_SYS`. Own
files, independent of M4-4. Executor: SanGen Coder (GL).*

## Title
Composite the preview image by **sampling the bake** — never re-simulating.

## Root problem
`PREVIEW_COMPOSITING_SPEC` + ARCH_03_ModuleBoundaries.md §3.2: the legacy `PreviewRenderer` shadow-reimplements
slope/flow/marker filtering in its own shaders → "preview truth ≠ bake truth." v2 preview
must colorize + composite the already-baked fields and nothing more.

## Target files
- `src/ui/PreviewComposite_UI.h` / `.cpp` / `.glsl` (+ `_Test.cpp` where testable).

## Layer & accuracy
`UI`. **Visual**. GPU via `GpuResource_SYS` (no private GL pipeline, no hardcoded paths).

## Solution
One compute shader composites into a `GL_RGBA8` image by **sampling** `MapFields`:
height shading; `surfaceStratumWeights` splat × stratum preview colors; flow / accumulation
/ water colorized through the M4-2 LUTs. It **writes the `EntityIdBuffer`** (M4-1) per
pixel while shading. It **reads** the baked fields — it never recomputes slope, re-filters
markers, or re-runs any sim (that is the whole point). Pass ordering: clear → per-field
passes → overlay → entity-id. Resolution/quality is a tweakable (§8): scrub fast, escalate
on idle.

## Acceptance
Given a known synthetic `MapFields`, the composite colors match expected values for a spot
cell (height ramp + splat + a LUT-colorized field); the `EntityIdBuffer` is written for
entity pixels and `emptySentinel` elsewhere; no sim/slope recompute exists in the shader
(code review + a test that changing a sim input without re-baking does NOT change the
composite beyond what the baked field carries); GPU via the shared resource manager;
builds clean.

## Out of scope
Picking readback (M4-4); the dirty-flag wiring + app loop (M4-5).
