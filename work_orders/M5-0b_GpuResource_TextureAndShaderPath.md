# Work-Order M5-0b — `GpuResource_SYS`: GL texture primitive + shader search path

*Constitution §7. Milestone M5 prerequisites. **Parallel-safe** (own file:
`GpuResource_SYS`), disjoint from M5-0a/M5-0c. **Gates M5 display.** Executor: SanGen
Coder (GL).*

## Root problem
Two M4 follow-ups, both in `GpuResource_SYS`:
1. It has no GL **image/texture** primitive — M4's "GL_RGBA8 image" is a packed SSBO.
   On-screen display (MapCanvas, M5) needs a real sampleable/blittable GL texture.
2. Its shader loader resolves **one** directory, so M4-5 had to stage every `.glsl` into a
   single output dir — breaking the `.cpp`/`.glsl` co-location rule (ARCH §1.4).

## Target files
- `src/sys/GpuResource_SYS.*` (+ test where feasible).

## Layer & accuracy
`SYS`. GPU/GL owned here only.

## Solution
1. Add a managed **GL texture** resource (create/resize/upload/bind, RGBA8 + the formats
   the composite/canvas need), owned + lifecycle-managed like the existing programs/SSBOs
   (opaque handle, GL types stay inside the SYS seam, ARCH §5). The composite can then
   write a real texture the canvas samples.
2. Give the shader loader a **configurable search path** (ordered list of directories);
   resolve a shader by scanning the path. Keeps `.glsl` beside their `_PROC`/`_UI` twins;
   CMake registers the path instead of copying files into one dir.

## Acceptance
A texture is created/resized/uploaded and read back correctly (round-trip a small RGBA8
image); a shader resolves from any directory on the configured path (put two shaders in
two dirs, both load); no hardcoded absolute paths; builds clean.

## Out of scope
Repointing the composite to emit a texture instead of an SSBO (a small M5 follow-up once
the primitive exists) — note it; MapCanvas itself (M5).
