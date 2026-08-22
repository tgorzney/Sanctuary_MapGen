# Work-Order — Step 33: split `Placement_Symmetry_PROC.h` under the ARCH_01_05_FileSizeCeilings.md §1.5 ceiling

*Constitution §7. Executor: SanGen Coder. Generator Expert consult.*

## Root problem
`src/proc/Placement_Symmetry_PROC.h` is 167 lines, over the 150-line hard ceiling (grew past it
during `STEP23_RadialSymmetryOrbit_PROC.md`).

## Ruled by this ticket
**Two-file split**, mirroring the existing `Placement_Transform_PROC.h`/`Placement_Gate_PROC.h`
dependency-file precedent already live in this same `Placement_*` family (header-only both sides,
no `.cpp`, `inline` functions — this is the established convention for small free-function
primitive libraries in `src/proc/`, not a special case).

- **`src/proc/Placement_Symmetry_PROC.h`** (kept, trimmed) — keeps ONLY `BuildSymmetryOrbit`, the
  public entry point every external caller uses. Adds `#include "Placement_SymmetryOrbit_PROC.h"`.
  Est. ~49 lines.
- **New `src/proc/Placement_SymmetryOrbit_PROC.h`** — moves `symmetryPi`, `struct
  SymmetryOrbitPoint`, and the entire `SymmetryDetail` namespace verbatim (`ApplyOrbitTransform`,
  `IsDuplicatePoint`, `AppendPoint`, `AppendTransformedSet`, `AppendQuarterTurns`,
  `AppendRadialTurns`). Add `#include "../math/Trigonometry_MATH.h"` (needed by
  `AppendRadialTurns`). Est. ~137 lines.

**Zero-ripple confirmed**: no external caller references `SymmetryDetail::` directly — every
caller only touches `SymmetryOrbitPoint`/`BuildSymmetryOrbit`, both re-exposed transitively once
the kept file includes the new one. No other file's `#include` needs to change. No new test file
needed — `Placement_Symmetry_PROC_Test.cpp` continues to cover the moved code through the existing
include chain.

**Confirmed not needed / explicit out-of-scope:**
- A three-way split isolating `AppendRadialTurns` alone — defensible but not required (the
  two-file split already lands well inside the ceiling with margin); do not do this now.
- Unifying `AppendQuarterTurns`/`AppendRadialTurns`'s shared "capture sourceCount, transform
  existing points" shape into one generic function — deliberately NOT DRY for a real reason:
  collapsing `AppendQuarterTurns` into `AppendRadialTurns(turnCount=4)` would replace exact
  swap-based 90° rotation with trig calls, a real determinism risk (`Math::Cosine`/`Sine` at
  π/2 boundaries may not be bit-exact 0/±1 the way integer swaps are) and a needless perf
  regression for the cheapest transform family. The current per-family duplication is correct;
  do not refactor it.

## Target files
- `src/proc/Placement_Symmetry_PROC.h` — trim to `BuildSymmetryOrbit` + include.
- New `src/proc/Placement_SymmetryOrbit_PROC.h` — the moved primitives.

## Layer & accuracy class
PROC only, pure refactor — zero behavior/output change. Accuracy class: Exact.

## Acceptance test
1. Both files land under 150 lines; every individual function stays under the 40-line cap
   (already true pre-split — confirm it stays true post-split).
2. `Placement_Symmetry_PROC_Test.cpp` and every other existing test (`Placement_PROC_Test`,
   `Placement_Gpu_PROC_Test`) continues to pass unchanged — this is a pure header relocation, no
   behavior difference is possible if the move is done correctly.
3. Full `SanGenV2` build stays clean.
