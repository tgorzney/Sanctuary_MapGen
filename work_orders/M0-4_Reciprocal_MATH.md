# Work-Order M0-4 — `Reciprocal_MATH` (fast reciprocal-square-root)

*Schema-valid per Constitution §7. Milestone M0 (Foundation). Executor: SanGen Coder.
Status: implemented + verified (ALL PASS).*

## Title
Fast reciprocal-square-root (bit-hack + Newton) with Accurate/Visual tiers, plus exact
scalar reciprocal.

## Root problem
Normalization and gradient math need `1/sqrt(x)` in hot loops; `sqrt` + divide is
expensive, and the current MATH library has no fast path. The old `Sanmath_FastMath.h`
is a misnomer (its `FastInv` is an exact divide, and nothing is actually approximated).

## Target files
- `src/math/Reciprocal_MATH.h`
- `src/math/Reciprocal_MATH_Test.cpp`

## Layer & accuracy class
`MATH`. Tiered (§4): `ReciprocalSquareRoot` **Accurate** (~1e-6), 
`ReciprocalSquareRootApproximate` **Visual** (~1.7e-3), `Reciprocal` **Exact**.
Deterministic (pure float + integer ops, no libm).

## Backend policy
CPU scalar primitive. The SIMD Visual-class approximate reciprocal already lives in
`FloatVector_MATH::ReciprocalApproximate` (hardware `rcpps`); not duplicated here.

## ARCH rules invoked
- §1.1 fully-spelled names (`ReciprocalSquareRoot`, `estimate`, no abbreviations).
- §1.2 `_MATH` suffix; §1.5 ceiling (44-line header); §4 accuracy tiers declared;
  §5 portability (no libm).

## Solution / key decision
`ReciprocalSquareRoot`: classic `0x5f3759df` bit-hack seed + two Newton steps
(`y*(1.5 - half*y*y)`); the Approximate variant uses one step. **`Reciprocal` is
deliberately `1.0f/value` (Exact), not a bit-hack** — on modern hardware a scalar
divide is as fast as the bit-hack seed+Newton *and* fully accurate, so approximating
it scalar-side would be a pessimization (max-performance ethos, Constitution §3).
Approximate reciprocal only pays off vectorized, where `FloatVector` already provides
it.

## Performance estimate (with basis)
rsqrt Accurate ≈ bit ops + 2×(2 mul + 1 sub); faster than `sqrtf` + `divss` on hot
loops (*basis: op count / well-known result; rough-estimate*). Visual is roughly half
that. `Reciprocal` is one `divss`.

## Lossy alternative
`ReciprocalSquareRootApproximate` (Visual, one Newton step) is the built-in lossy tier
for preview-class use.

## Acceptance test (`Reciprocal_MATH_Test.cpp`) — PASSED
Over [1e-3, 1e4]: rsqrt Accurate max relative ≤ 5e-6 (measured 4.7e-6); rsqrt Visual
≤ 2e-3 (measured 1.75e-3); `Reciprocal(x)` **bit-identical** to `1.0f/x`. **Verified in
sandbox: ALL PASS.**

## Out of scope (explicit)
- `Exponential`/`Logarithm`/`Power` — later transcendental work-orders.
- A `FloatVector`-vectorized rsqrt with one Newton refinement over `rsqrtps` — later
  (pairs with M0-2).
- `Spatial_MATH` (clearance + JFA) — next MATH work-order.
