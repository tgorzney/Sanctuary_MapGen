# Work-Order M0-3 — `Trigonometry_MATH` (portable deterministic sine/cosine)

*Schema-valid per Constitution §7. Milestone M0 (Foundation). Executor: SanGen Coder.
Status: implemented + verified (ALL PASS).*

## Title
Portable, deterministic single-precision `Sine` / `Cosine`.

## Root problem
`DETERMINISM_SPEC` needs transcendentals that are **bit-identical across machines**;
`std::sin`/`std::cos` differ in their last bits across libms/compilers, so the
cross-machine shared-generation path cannot use them. The current MATH library has no
trig at all. SanGen needs its own portable minimax sine/cosine.

## Target files
- `src/math/Trigonometry_MATH.h`
- `src/math/Trigonometry_MATH_Test.cpp`

## Layer & accuracy class
`MATH`. **Accurate** (~1e-7 near the origin; ~1e-5 at |radians|≈200 as float range
reduction widens). **Deterministic**: pure float + int ops, no libm call — bit-
identical across machines when the deterministic build disables fast-math
reassociation/contraction (`DETERMINISM_SPEC`).

## Backend policy
CPU scalar primitive (a `FloatVector`-vectorized variant is a later work-order). Not a
dispatched runtime stage.

## ARCH rules invoked
- §1.1 fully-spelled names (`Sine`, `Cosine`, `reducedAngle`, `octant`).
- §1.2 `_MATH` suffix, `src/math/`.
- §1.5 ceiling — header 70 lines (≤150).
- §4 accuracy class declared; §5 portability (no libm dependency).

## Solution
Cephes-style extended-precision range reduction — pi/4 split into three parts
(`quarterPiHigh/Mid/Low`) for accurate modular arithmetic — plus minimax sine/cosine
polynomials selected by octant. All operations are float/int; no `std::` transcendental
is called.

## Performance estimate (with basis)
~a dozen float multiply-adds plus one float→int and a couple of branches; comparable to
or faster than a libm `sinf`, and unlike libm it is deterministic (*basis: operation
count; rough-estimate*).

## Lossy alternative
None required. If maximum accuracy is needed on the non-deterministic path, callers may
use `double`/libm — but that path is not portable-deterministic and must not be used in
the shared-generation bake.

## Acceptance test (`Trigonometry_MATH_Test.cpp`) — PASSED
Vs `std::sin`/`std::cos` over a ±200 sweep: max error ≤ 1.2e-5 (near-origin measured
2.5e-7); Pythagorean identity `sin²+cos²−1` ≤ 1e-5 (measured 1.4e-7); anchor values at
0, π/2, π, 3π/2 within 2e-6; file ≤150 lines. **Verified in sandbox: ALL PASS.**

## Out of scope (explicit)
- `Tangent`/`ArcTangent2`/`ArcSine`/`ArcCosine`, and `Exponential`/`Logarithm`/`Power`
  — later transcendental work-orders.
- A `FloatVector`-vectorized `Sine`/`Cosine` — later (pairs with M0-2).
- Fast reciprocal / reciprocal-square-root — next work-order (M0-4 group).

## Open (from M0-1, still pending)
Test-file convention not yet ratified in the ARCH (`*_MATH_Test.cpp` used as proposal)
— route to ARCH Expert.
