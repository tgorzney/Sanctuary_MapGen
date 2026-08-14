# Work-Order M0-1 — `Morton_MATH` (portable Z-order module)

*Schema-valid per Constitution §7. Milestone M0 (Foundation). Author: (Compute
Optimization Expert). Executor: SanGen Coder. Status: ready.*

## Title
Build the single portable Morton (Z-order) math module, `Morton_MATH`.

## Root problem
Morton encode/decode is **triplicated** — `math/Sanmath_Morton.h`, again in
`gen/Gen_Noise.h`, and inline in `TerrainGenerator.cpp` — is **2D-only**, and uses the
**magic-number** interleave with no BMI2 fast path, no 3D, and no block-linear/tiled-Z
helper. That violates the naming law (one definition, §2) and leaves the DATA layer
without the tiled-Z addressing the optimization pillars assume.

## Target files
- **Create** `src/math/Morton_MATH.h` (the module).
- **Create** `src/math/Morton_MATH_Test.cpp` (the acceptance test).
- **Delete** the three duplicate copies (in `Sanmath_Morton.h`, `Gen_Noise.h`, and the
  inline copy in `TerrainGenerator.cpp`); repoint their callers at `Morton_MATH`.

## Layer & accuracy class
`MATH`. **Exact** — pure integer bit operations, bit-exact by construction.

## Backend policy
Foundational CPU MATH primitive — **not a dispatched stage** (no `DispatchPolicy`).
Compile-time backend: **BMI2 `_pdep_u32`/`_pext_u32`** under `#if defined(__BMI2__)`,
else a **portable magic-number fallback** (Constitution §5 — portability must not gate
the design, but the fallback must exist).

## ARCH rules invoked
- §1.1 fully-spelled names, no abbreviations (`interleaveX`, not `ix`).
- §1.2 `_MATH` suffix, file lives in `src/math/`.
- §1.5 ceilings: file ≤150 lines, functions ≤40, one primary concern.
- §2 one definition — this deletes the triplication.
- Constitution §5 — BMI2 fast path + portable fallback, selected at compile time.

## Solution
In `namespace SanmapGen::Math`, provide fully-spelled:
- `EncodeMorton2D(x, y)` / `DecodeMorton2D(code, x, y)` — existing 2D behavior, kept
  bit-compatible with the current magic-number result.
- `EncodeMorton3D(x, y, z)` / `DecodeMorton3D(...)` — new (Part1By2 / Compact1By2).
- `BlockLinearIndex(x, y, tileWidthLog2)` — tiled-Z address helper for DATA layout.
Each has a BMI2 path (`_pdep_u32`/`_pext_u32`) and a magic-number fallback behind one
compile-time switch; both paths must be bit-identical.

## Performance estimate (with basis)
BMI2 `pdep`/`pext` ≈ 3-cycle latency, 1/clock throughput on Intel Haswell+
(*basis: published `pdep` latency; rough-estimate*). Magic-number fallback ≈ 5 shift+
and pairs per axis, ~10–20 cycles (*basis: cycle-counted the existing shift chain*).
Expected ~4–6× on the BMI2 path; both bit-identical.

## Lossy alternative
None — exact integer math, no lossy variant.

## Acceptance test (`Morton_MATH_Test.cpp`)
1. **Round-trip identity**: `Decode(Encode(x,y)) == (x,y)` across the full 16-bit-per-
   axis 2D domain (sampled) and the 10-bit-per-axis 3D domain (sampled).
2. **Backend parity**: BMI2 path and magic-number fallback produce **identical codes**
   across the sampled domain (compile both, compare).
3. **2D compatibility**: new `EncodeMorton2D` == the old `Sanmath_Morton` result for a
   sampled set (no silent layout change).
4. **Block-linear**: `BlockLinearIndex` maps a spot-checked set of `(x,y)` into the
   correct tile + in-tile offset.
5. **Cleanup**: the three duplicate copies are gone and the project still links.
6. Files within §1.5 ceilings.

## Out of scope (explicit)
- SIMD/batch Morton (vectorized encode of an array) — a later MATH work-order.
- Actually re-laying-out DATA buffers in tiled-Z order — that is the DATA-layout
  work-order (M1); this only provides the addressing function.
- Spatial hashing / clearance scoring — separate (`Spatial_MATH`).

## Note routed to ARCH Expert
No **test-file location/naming convention** is in the ARCH yet (this uses
`*_MATH_Test.cpp` beside the module as a proposal). Ratify a testing convention before
M0-2 so it is consistent across the rebuild.
