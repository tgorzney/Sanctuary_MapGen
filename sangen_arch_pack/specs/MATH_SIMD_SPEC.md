# MATH_SIMD_SPEC — the core math library (SIMD, fast-math, Morton, spatial)

Source: `core/math/Sanmath_SIMD.h`, `Sanmath_FastMath.h`, `Sanmath_Morton.h`,
`Sanmath_Spatial.h`. Namespace `SanmapGen::Math`. This is the shared MATH layer
(Constitution §1) every kernel builds on. **Reality check: today it is stub-level** —
each file is tiny and most of the advertised capability does not exist yet. This
spec records what's really there and prescribes the v2 library that
`OPTIMIZATION_PILLARS`, `DISPATCH_INTERFACE_SPEC`, and `DETERMINISM_SPEC` assume.

## Current state (measured — all four files are ~18–252 lines)

### SIMD — `Sanmath_SIMD.h` (18 lines)
Two functions, both AVX threshold compares, no arithmetic kernels:
`CheckThreshold8_AVX(__m256, float)` → movemask of lanes `<= threshold`;
`CheckThreshold8_AVX_Mask(__m256, float)` → raw compare mask for downstream
predication. AVX only (`_mm256_cmp_ps _CMP_LE_OQ`, `_mm256_movemask_ps`). **No FMA,
no reductions, no `blendv`, no load/store helpers, no scalar fallback, no `#ifdef`
guard** — including this on a non-AVX/ARM target fails to compile.

### FastMath — `Sanmath_FastMath.h` (19 lines)
Nothing is actually approximated. `FastInv(x)` = exact `1.0f/x` (the rcp intrinsic
is only a comment). `GetSlopeSquaredThreshold(deg)` = `(1/cos²)−1` via `std::cos`,
with guards `>=89.9f`/`cos<=0.0001f → 9999999.0f`. The name is aspirational: no
rsqrt bit-hack, no minimax poly, no LUT, no sin/cos/exp/log/pow/atan2, no documented
error class.

### Morton — `Sanmath_Morton.h` (35 lines)
`EncodeMorton2D`/`DecodeMorton2D` — classic magic-number bit-interleave
(`0x55555555`→`0x33333333`→`0x0f0f0f0f`… Part1By1/Compact1By1). Portable and
deterministic, 16 bits/axis. **2D only; no 3D, no BMI2 `pdep/pext`, no tile/
block-linear helpers.** (This same interleave is **triplicated** elsewhere —
`Gen_Noise.h` and inline in `TerrainGenerator.cpp` — a §2 naming-law violation.)

### Spatial — `Sanmath_Spatial.h` (252 lines) — the only substantial file
- `ScoreRadialClearance(mask, cx, cy, minH, maxH, tol, maxR, minStartR=1)` —
  Bresenham-perimeter clearance, exponential gallop + binary search, O(R log R).
- `ScoreRadialClearance_Stochastic(..., seed=12345)` — 8 angular samples via
  `cos/sin`, position-hashed RNG `seed ^ r*19349663 ^ cx*73856093 ^ cy*83492791`
  (Teschner primes), O(log R).
- `ComputeJFADistanceField(mask, minH, maxH, tol, maxR)` — Jump-Flood distance
  field, `#pragma omp parallel for` (3 regions), O(w·h·log max(w,h)); `short Coord`
  caps grids at 32767; full `buffer = nextBuffer` copy per pass (no pointer swap).
Both scorers always return `0.0f` for the variance element ("simplified for speed")
— the second half of that API is dead.

## Issues (the gap to close)
- **Not portable** (Constitution §5): AVX with no guard/fallback; the deterministic
  and ARM paths `DETERMINISM_SPEC`/portability need do not exist.
- **No real fast-math**: the pillars (`OPTIMIZATION_PILLARS`) assume minimax
  transcendentals, bit-hack rsqrt, reciprocal-multiply, FMA — none are implemented.
  `FastInv` is a misnomer.
- **No accuracy-class contract**: nothing states ULP/error bounds, so the Exact/
  Accurate/Visual classes (Constitution §4) have no math to stand on.
- **Duplication**: Morton triplicated; the two clearance scorers duplicate the whole
  gallop+binary-search driver (should be templated on the perimeter predicate).
- **Hardcoded constants**: pi `3.14159265f` (three copies), sentinels `9999999.0f`/
  `99999999.0f`, `89.9f`, `0.0001f`, seed `12345`, Teschner primes inline.
- **Dead API**: variance return always 0.
- **Missing breadth**: no 3D Morton / block-linear (tiled-Z) helpers, no SoA/AoSoA
  vector types, no horizontal reductions — all named in the pillars.

## v2 target library (what the pillars require this to become)
1. **Portable SIMD abstraction** — a width-agnostic vector type with AVX2/AVX/SSE and
   a scalar fallback selected at compile time (and an ARM/NEON path allowed, since
   portability must not limit the design — Constitution §5). Real kernels: FMA,
   masked/branchless select (`blendv`), horizontal reductions, load/store, gather.
2. **Real fast-math with declared accuracy classes** — minimax-polynomial
   sin/cos/exp/log/pow/atan2, bit-hack + one-Newton rsqrt, reciprocal-multiply; each
   tagged Exact/Accurate/Visual with a stated error bound. The **portable
   deterministic** variants (identical bits on every machine) live here — they are
   the transcendentals `DETERMINISM_SPEC` mandates and double as an optimization
   pillar.
3. **One Morton module** — 2D + 3D encode/decode, BMI2 `pdep/pext` fast path with a
   magic-number fallback, plus **block-linear / tiled-Z** helpers; delete the two
   duplicate copies (§2).
4. **Spatial** — template the clearance driver on its predicate; pointer-swap JFA;
   widen `Coord` past 32767; compute the variance or drop it from the signature;
   OpenMP with explicit scheduling. Hoist all magic constants to named, tweakable
   config (§8).
5. **SoA/AoSoA vector types** the data-oriented pipeline (`OPTIMIZATION_PILLARS`,
   `PLACEMENT_SCATTER_SPEC`) is written against.

## Ties
MATH layer (Constitution §1) under everything; realizes `OPTIMIZATION_PILLARS`
(SIMD/FMA/reciprocal/LUT/Morton) and supplies `DETERMINISM_SPEC` its portable
transcendentals; CPU/GPU kernels share these primitives via
`DISPATCH_INTERFACE_SPEC`.
