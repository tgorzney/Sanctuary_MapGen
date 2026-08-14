# Work-Order M0-2 — `FloatVector_MATH` (portable 8-lane SIMD float vector)

*Schema-valid per Constitution §7. Milestone M0 (Foundation). Executor: SanGen Coder.
Status: implemented + verified (both backends ALL PASS).*

## Title
Build the portable 8-lane SIMD float-vector primitive `FloatVector`.

## Root problem
Today's `math/Sanmath_SIMD.h` is stub-level — two threshold-compare functions, AVX-
only, no scalar fallback, no arithmetic. The v2 PROC kernels (noise, erosion, masks,
flow) need a real width-consistent SIMD float primitive with arithmetic, FMA, min/max,
reductions and select, plus a portable fallback so the design isn't AVX-locked.

## Target files
- `src/math/FloatVector_MATH.h` — the AVX2 backend + the backend switch.
- `src/math/FloatVector_Scalar_MATH.h` — portable scalar fallback (same API).
- `src/math/FloatVector_MATH_Test.cpp` — acceptance test.

## Layer & accuracy class
`MATH`. **Mixed**: arithmetic, `Minimum`/`Maximum`/`SquareRoot`, `CompareLessOrEqual`,
`Select`, `HorizontalSum` are **Exact**; `FusedMultiplyAdd` and `ReciprocalApproximate`
are **Accurate/Visual** (backends may differ within a stated tolerance — §4).

## Backend policy
Compile-time, not a dispatched runtime stage. `#if defined(__AVX2__)` → 8-lane `__m256`
path; else the scalar `float[8]` fallback. `FusedMultiplyAdd` uses `_mm256_fmadd_ps`
under `__FMA__`, else `mul`+`add`. On MSVC, `/arch:AVX2` defines `__AVX2__`; a plain
build gets the scalar fallback.

## ARCH rules invoked
- §1.1 fully-spelled names (`FloatVector`, `laneCount`, `whenTrue`/`whenFalse`).
- §1.2 `_MATH` suffix, `src/math/`.
- §1.5 ceilings — all three files ≤150 (header 64); the scalar fallback lives in its
  own file so neither exceeds the cap.
- §4 accuracy classes — FMA/reciprocal declared non-Exact.
- §5 portability — the scalar fallback is mandatory, not optional.

## Solution
An 8-lane `FloatVector` wrapping `__m256`: `Broadcast`, `Load`, `Store`, `+ - * /`, and
free functions `Minimum`, `Maximum`, `SquareRoot`, `ReciprocalApproximate`,
`FusedMultiplyAdd`, `CompareLessOrEqual`, `Select` (sign-bit blend), `HorizontalSum`.
The scalar fallback mirrors the exact API and width.

## Performance estimate (with basis)
8-wide AVX2 processes 8 floats per instruction vs 1 for scalar → ~8× arithmetic
throughput on saturated hot loops (*basis: lane width; rough-estimate*).
`_mm256_rcp_ps` is ~11–12-bit accurate (*basis: Intel spec* → Visual class).

## Lossy alternative
`ReciprocalApproximate` is itself the lossy reciprocal (Visual). Exact reciprocal is
`FloatVector::Broadcast(1.0f) / x` (full-precision divide).

## Acceptance test (`FloatVector_MATH_Test.cpp`) — PASSED
Compile+run the same source **twice** (scalar, then `-mavx2 -mfma`); every op checked
against a scalar reference — Exact ops within 1e-6, FMA within 1e-6, reciprocal within
2e-3. Both must print `ALL PASS`. **Verified in sandbox: both backends ALL PASS; header
64 lines (≤150).**

## Out of scope (explicit)
- Intermediate AVX/SSE tiers and AVX-512 — this is AVX2-or-scalar; wider tiers later.
- Integer vectors, masked load/store, gather/scatter — later MATH work-orders.
- Wiring `FloatVector` into the kernels — that is M3 (per-stage).
