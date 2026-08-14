# Work-Order M0-5 — `Spatial_MATH` (radial clearance + Jump-Flood distance field)

*Schema-valid per Constitution §7. Milestone M0 (Foundation). Executor: SanGen Coder.
Status: implemented + verified (both ALL PASS). Completes the MATH group.*

## Title
Radial clearance scoring and the Jump-Flood distance field, decoupled from the DATA
layer.

## Root problem
Placement/scatter needs to size obstacle-free clearances and compute distance-to-
obstacle fields. The old `Sanmath_Spatial.h` (a) duplicated the entire gallop+binary-
search driver across the two clearance scorers, (b) used `short` seed coords (32767
grid cap), (c) copied the whole JFA buffer every pass instead of ping-ponging, (d)
returned an always-zero "variance" (dead API), and (e) `#include`d the DATA-layer
`FloatMask` — a MATH→DATA layer violation (ARCH §3).

## Target files
- `src/math/RadialClearance_MATH.h` + `RadialClearance_MATH_Test.cpp`
- `src/math/JumpFloodDistanceField_MATH.h` + `JumpFloodDistanceField_MATH_Test.cpp`

## Layer & accuracy class
`MATH`. **Exact** grid geometry; `ScoreRadialClearanceStochastic` is the approximate
(cheaper) tier. Operates on **raw `const float*` arrays**, never `FloatMask`.

## Backend policy
CPU scalar. JFA is O(w·h·log(max(w,h))); a threaded/SIMD variant is a later work-order.

## ARCH rules invoked
- §1.1 fully-spelled names; §1.2 `_MATH` suffix; §1.5 ceilings (98 and 83 lines).
- §3 layer purity — MATH takes raw arrays, not the DATA `FloatMask` (fixes the old
  include violation).

## Solution
- **Radial clearance:** one templated `FindLargestClearRadius(minStart, maxSearch,
  isClear)` driver (merges the two duplicated copies), with a Bresenham-perimeter
  predicate (`ScoreRadialClearance`, exact) and an 8-angular-sample predicate
  (`ScoreRadialClearanceStochastic`, deterministic in `(seed, centerX, centerY)` via a
  Teschner-prime position hash). The always-zero variance is dropped from the return.
- **Jump-Flood:** `int` seed coordinates (no 32767 cap), pointer-swap ping-pong (no
  per-pass copy), plus two extra step-1 passes (JFA+2) for exactness. Seeds = out-of-
  band cells or cells whose gradient magnitude exceeds `gradientTolerance`; output
  distance clamped to `maxDistance`.

## Performance estimate (with basis)
Clearance: exact O(radius · log radius), stochastic O(log radius) (*basis: perimeter
size × search probes; cycle-reasoned*). JFA: O(w·h·log(max(w,h))) (*basis: standard
Jump-Flood*).

## Lossy alternative
`ScoreRadialClearanceStochastic` is the built-in lossy clearance tier (8 samples vs the
full perimeter).

## Acceptance test — PASSED
Clearance: on a radius-20 in-band disk, `ScoreRadialClearance` returns 20 and the
stochastic variant 19 (within tolerance); a flat field is bounded at `maxSearchRadius`
(50); the stochastic result is reproducible for identical inputs. JFA: matches a brute-
force nearest-seed reference to **1.9e-6** (float sqrt rounding); seed cells read 0.
**Verified in sandbox: both ALL PASS.**

## Out of scope (explicit)
- OpenMP/SIMD parallelization (callers parallelize; threaded JFA later).
- True-Euclidean vs magnitude gradient tuning for the JFA seed test.
- A spatial hash grid for O(1) marker hit-testing — separate (that is a DATA/UI concern).

## Open (from M0-1, still pending)
Test-file convention not yet ratified in the ARCH — route to ARCH Expert.
