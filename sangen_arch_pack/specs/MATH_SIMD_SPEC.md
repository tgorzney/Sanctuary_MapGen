# MATH_SIMD_SPEC — the core math library (SIMD, fast-math, Morton, spatial)

Source: `src/math/*_MATH.h` (each paired with a standalone `*_MATH_Test.cpp`
acceptance test). Namespace `SanmapGen::Math`. This is the shared MATH layer
(Constitution §1, ARCH §3.1: depends on nothing, everything else may depend on
it) every kernel builds on. **This supersedes the old, now-legacy
`core/math/Sanmath_*.h` stub family** (`Sanmath_SIMD.h`, `Sanmath_FastMath.h`,
`Sanmath_Morton.h`, `Sanmath_Spatial.h`) — see "Relationship to `core/math/`"
below; do not confuse the two when reading old work-orders or comments that
predate the split.

## Naming law
One file per primitive family, `<Topic>_MATH.h`, PascalCase topic, no
abbreviation (Constitution §1.8/§1.9 lineage). A scalar-only fallback that a
SIMD header conditionally includes gets its own sibling file,
`<Topic>_Scalar_MATH.h` (today: `FloatVector_Scalar_MATH.h`), never inlined
into the SIMD header's `#else` branch as a second implementation to keep in
sync. Every header carries a standalone, compile-and-run
`<Topic>_MATH_Test.cpp` acceptance test (no test framework dependency) as a
sibling in the same directory — this is the enforced convention for new MATH
files, not optional polish.

## The current file set (real, in active use)
All nine headers below are genuinely substantial (not stubs) and have live
callers in `src/proc/` and `src/ui/` today (confirmed by direct grep — at
least `NoiseBlend_Prepare_PROC.cpp`, `NoiseBlend_PROC.cpp`,
`NoiseBlend_Blend_PROC.cpp`, `Placement_SymmetryOrbit_PROC.h`,
`Placement_RuleBuild_PROC.h`, `Placement_Fields_PROC.cpp`,
`Placement_Transform_PROC.h`, `Placement_Metrics_PROC.cpp`,
`Thermal_Kernel_PROC.h`, `Erosion_Rain_PROC.cpp`, and
`MarkersTab_BundleNodeBody_UI.cpp`). Read this list before writing a new
primitive — it exists to stop a future coder duplicating something already
here.

### `FloatVector_MATH.h` / `FloatVector_Scalar_MATH.h` — portable 8-lane SIMD float vector
The width-agnostic SIMD abstraction every wide kernel is written against.
`FloatVector_MATH.h` is an AVX2 backend (`__m256`, gated on `__AVX2__`) that
`#include`s `FloatVector_Scalar_MATH.h` instead when no SIMD backend is
available — same public API, same 8-lane width, on both paths, so calling
code never branches on backend. Provides: `Broadcast`/`Load`/`Store`,
`+ - * /`, `Minimum`/`Maximum`/`SquareRoot`, `ReciprocalApproximate` (Visual —
raw `rcpps`/scalar-divide, no Newton refinement), `FusedMultiplyAdd` (uses
`_mm256_fmadd_ps` under `__FMA__`, falls back to mul+add), `CompareLessOrEqual`
+ `Select` (branchless `blendv`-style masked select), `HorizontalSum`. This is
the "portable SIMD abstraction" and "FMA/masked-select/horizontal-reduction"
capability the old spec's v2 target list (§1 below) called for — it exists
now.

### `Trigonometry_MATH.h` — deterministic minimax sine/cosine
`Sine(radians)`/`Cosine(radians)`, cephes-style extended-precision range
reduction (three-part `pi/4` split) plus minimax polynomials — no
`std::sin`/`std::cos`, no libm call, so results are bit-identical across
machines/compilers. Accuracy class **Accurate** (~1e-7 near the origin,
degrading for very large `|radians|`). This is the portable deterministic
transcendental `DETERMINISM_SPEC` requires, and the direct replacement for
every ad hoc `std::cos`/hardcoded-`3.14159265f` call the old stub-era code had
scattered around.

### `Reciprocal_MATH.h` — fast reciprocal-square-root and exact reciprocal
Three functions, each accuracy-tagged: `ReciprocalSquareRootApproximate`
(Visual, one Newton step after the classic `0x5f3759df` bit-hack seed, ~1.7e-3
relative) and `ReciprocalSquareRoot` (Accurate, two Newton steps, ~1e-6
relative) for `1/sqrt(x)`; `Reciprocal(x)` (Exact — a scalar divide is stated
by design as no slower than a bit-hack approximation on modern hardware, so
approximating it scalar-side would be a pessimization — the Visual-class
reciprocal lives on the SIMD path instead, `FloatVector::ReciprocalApproximate`
above). This is the real, accuracy-tagged rsqrt the old `Sanmath_FastMath.h`
only pretended to be (its `FastInv` was an exact divide with an unused rcp
comment).

### `Morton_MATH.h` — 2D + 3D Morton (Z-order) encode/decode + tiled block-linear index
One definition for the whole project (replacing the Morton logic that used to
be triplicated across `Gen_Noise.h`, `TerrainGenerator.cpp`, and the old
`Sanmath_Morton.h`). `EncodeMorton2D`/`DecodeMorton2D` (16 bits/axis) and
`EncodeMorton3D`/`DecodeMorton3D` (10 bits/axis) each have a BMI2
`pdep`/`pext` fast path (`#if defined(__BMI2__)`) with a portable
magic-number bit-interleave fallback (`MortonDetail::SpreadEveryOtherBit`
etc.) always compiled and available to tests for backend-parity checks.
`BlockLinearIndex(x, y, surfaceWidth, tileSizeLog2)` adds the tiled-Z /
block-linear surface index the old spec asked for: square tiles laid out
row-major, cells within a tile Morton-ordered. Accuracy class Exact (pure
integer bit ops).

### `RadialClearance_MATH.h` — largest obstacle-free radius around a cell
Depends only on `Trigonometry_MATH.h`. One shared gallop+binary-search driver,
`RadialClearanceDetail::FindLargestClearRadius`, templated on a
`PredicateIsClear(radius)` callable — replacing the old spec's two duplicated
driver copies. Two public entry points share it: `ScoreRadialClearance`
(exact Bresenham-perimeter check — every perimeter cell must be in-bounds,
within `[minHeight, maxHeight]`, and within `heightTolerance` of the center's
height) and `ScoreRadialClearanceStochastic` (8 jittered angular samples per
radius via `Trigonometry_MATH`'s `Sine`/`Cosine`, deterministic per
`(seed, centerX, centerY)` through a Teschner-prime position hash). Operates
on a raw `const float* heightField` — deliberately not the DATA-layer
`FloatMask` type, which would make MATH depend on DATA (ARCH §3.1). Used by
placement/scatter to size clearances around a candidate point. **Note: the
old spec's "variance element always returns 0.0f, a dead half of the API" —
this rewritten version has no variance return at all; that dead surface was
dropped, not carried forward.**

### `JumpFloodDistanceField_MATH.h` — distance-to-nearest-obstacle via Jump Flooding
`ComputeJumpFloodDistanceField(heightField, width, height, minHeight,
maxHeight, gradientTolerance, maxDistance, outDistance)` — for every cell, the
Euclidean distance to the nearest "seed" cell (outside the height band, or
where the local gradient magnitude exceeds `gradientTolerance`), clamped to
`maxDistance`. O(w·h·log(max(w,h))). Fixes vs. the old `Sanmath_Spatial.h`
version: `int` seed coordinates (no `short`/32767-cell cap), true
pointer-swap read/write buffer ping-pong (no full-buffer copy per pass), and
two extra step-1 relaxation passes (JFA+2) for exactness. Also raw-array
based, same DATA-independence reasoning as `RadialClearance_MATH.h`. Used by
placement to keep scatter away from boundaries/obstacles.

### `HeightOcclusion_MATH.h` — top-down occlusion weighting for `MaterialProportions`
Three small pure functions realizing `MASKING_SPEC` Part 2's "height mask
(top-down occlusion)" formula verbatim: `OrderOcclusionWindow` (orders a
blend window and guarantees it non-empty via a caller-supplied
`separationEpsilon`, never hardcoded here), `OcclusionAlpha` (`thickness *
contrast`, hard-clamped into the ordered window, times opacity — hard clamp
only, no smoothstep/feather/invert), `OcclusionContribution` (caps a
computed alpha by whatever visibility is still unclaimed). Accuracy class
Accurate — the expression is written to be identical on CPU and the GLSL
twin (`occlusionAlpha` in `NoiseBlend_Shape_PROC.glsl`), so `PROC`'s CPU/GPU
backends agree bit-for-bit-in-intent, not just within a loose tolerance.
`NoiseBlend_PROC` is `MaterialProportions`'s single declared writer (ARCH
§7.2) and this is that writer's shared math.

### `RigidTransformPivot_MATH.h` — rigid rotate-around-pivot, pure scalars
`RotatePointAroundPivot`, `MultiplyQuaternions` (Hamilton product, same
operand order as `Placement_Transform_PROC.h`'s `QuaternionMultiply`), and
`YawQuaternion` (world-Y-axis rotation, same construction as
`Placement_Transform_PROC.h:74-77`) — zero `Params::` types in any signature,
so it is legally `MATH` under the general placement rule (ARCH §3.5): any
function with a `Params::`-typed parameter must live in `PARAMS` instead,
never here. This is the one shared rotate primitive `ARCH_19_08` requires
Bundle's move/rotate and Assembly's rotate to both call, rather than
maintaining two copies.

## Relationship to `core/math/`
`core/math/Sanmath_SIMD.h`, `Sanmath_FastMath.h`, `Sanmath_Morton.h`, and
`Sanmath_Spatial.h` **still exist on disk, unchanged, and are not dead** —
`core/TerrainGenerator.cpp`, `core/gen/Gen_FlowAndAccumulation.cpp`,
`core/gen/Gen_Mask_Slope.cpp`, and `core/gen/Gen_Marker_Procedural.cpp` (the
old, pre-`src/`-rebuild generation pipeline named as greenfield/legacy by the
ARCH opening hit-list item 1) still `#include` them and remain their real,
compiling dependents. They are **legacy, not current law**: stub-level (same
content the original version of this spec described — two AVX threshold
compares with no fallback guard, an unapproximated "fast" inverse, 2D-only
Morton, a non-pointer-swap JFA), superseded in every real capability by the
`src/math/*_MATH.h` files above, and out of scope for new code. **New work
targets `src/math/`, never `core/math/`.** Do not delete or edit
`core/math/Sanmath_*.h` on the strength of this spec alone — it still has
live callers in `core/`, and removing it is a `core/`-pipeline retirement
question (opening hit-list item 1), not a MATH-layer one; that removal is a
coder work-order gated on the old `core/` pipeline's own retirement, not
something this spec triggers by itself.

## What the old spec's "v2 target library" wish list asked for, and its status now
The prior version of this spec was written when `core/math/` was the only
math library and prescribed a v2 target. Recording what shipped, since a
reader may still hold the old mental model:
1. **Portable SIMD abstraction (AVX2 + scalar fallback, FMA, masked select,
   horizontal reductions, load/store)** — shipped: `FloatVector_MATH.h` /
   `FloatVector_Scalar_MATH.h`. Still open: no AVX-512, no gather, no
   ARM/NEON backend (a third file, `FloatVector_Neon_MATH.h`, following the
   same "same API, different backend, selected at compile time" pattern,
   would close this without touching the other two).
2. **Real fast-math with declared accuracy classes** — shipped for
   sin/cos (`Trigonometry_MATH.h`) and rsqrt/reciprocal (`Reciprocal_MATH.h`),
   each carrying an explicit Exact/Accurate/Visual tag and a stated error
   bound, per Constitution §4. Still open: no exp/log/pow/atan2 minimax
   polynomials yet — add them here, as `<Name>_MATH.h` siblings, if/when a
   caller needs one; do not let a caller hand-roll its own.
3. **One Morton module, 2D + 3D, BMI2 fast path + fallback, block-linear
   helpers, triplication deleted** — shipped in full: `Morton_MATH.h`.
4. **Spatial: templated clearance driver, pointer-swap JFA, wider seed
   coordinates, magic constants named** — shipped, split into two focused
   files instead of one `Sanmath_Spatial.h`: `RadialClearance_MATH.h` and
   `JumpFloodDistanceField_MATH.h`. The variance-return dead API was dropped
   entirely rather than fixed.
5. **SoA/AoSoA vector types for the data-oriented pipeline** — partially
   addressed: `FloatVector_MATH.h` is the SIMD lane primitive such types are
   built from, but no higher-level SoA/AoSoA container type lives in
   `src/math/` yet. Still open — see `OPTIMIZATION_PILLARS.md` /
   `PLACEMENT_SCATTER_SPEC.md` for where that data-oriented shape is
   consumed today.

Two files named in the old target list are genuinely new since: the
top-down-occlusion math (`HeightOcclusion_MATH.h`) and the rigid
rotate-around-pivot math (`RigidTransformPivot_MATH.h`) — neither was
anticipated by the original wish list; both arrived from real feature work
(`MASKING_SPEC`'s height-mask formula and `ARCH §19.8`'s Bundle/Assembly
rotate unification, respectively) and are recorded above as first-class
members of the library, not gap-fill.

## Ties
MATH layer (Constitution §1, ARCH §3.1) under everything; realizes
`OPTIMIZATION_PILLARS` (SIMD/FMA/reciprocal/Morton); supplies
`DETERMINISM_SPEC` its portable transcendentals (`Trigonometry_MATH.h`,
`Reciprocal_MATH.h`); `RadialClearance_MATH.h`/`JumpFloodDistanceField_MATH.h`
back `PLACEMENT_SCATTER_SPEC`; `HeightOcclusion_MATH.h` backs `MASKING_SPEC`'s
height-mask formula and is `NoiseBlend_PROC`'s shared math (ARCH §7.2);
`RigidTransformPivot_MATH.h` backs `ARCH §19.8`'s Bundle/Assembly rotate
unification; CPU/GPU kernels share these primitives via
`DISPATCH_INTERFACE_SPEC`; the general MATH-vs-PARAMS-vs-PROC placement rule
for any new pure function is `ARCH §3.5`.
