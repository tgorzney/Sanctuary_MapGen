[← ARCH index](ARCH.md) · [§16 ARCH_16_MarkerLayerSymmetry](ARCH_16_MarkerLayerSymmetry.md) · SanGen ARCH §16.3. **Only the ARCH Expert writes this file.**

### 16.3 Module boundary — solved via the existing legal `UI → PIPELINE → PROC` chain; no MATH relocation, no new UI→PROC exception
The design's item 3 recommended relocating `BuildSymmetryOrbit`/`ResolveSymmetryMask`
(`src/proc/Placement_Symmetry_PROC.h`, `Placement_SymmetryOrbit_PROC.h`,
`Placement_RuleBuild_PROC.h`) to `MATH` so `UI`'s new drag/add-marker interactions could call the
same orbit math PROC uses, avoiding the v1 duplicated-mirror-math bug the design correctly wants
to prevent. **Ruled: reject the MATH relocation for the mask-dispatching entry points; no
relocation is needed at all** — read together, direct code inspection this session found a
narrower, fully law-conformant answer.

- **Why relocating `BuildSymmetryOrbit` itself to MATH would violate the Constitution, not just
  the dependency table.** `BuildSymmetryOrbit` (`Placement_Symmetry_PROC.h`) directly branches on
  `Params::SymmetryAxis::MirrorAcrossX`/`MirrorAcrossZ`/`RotateHalfTurn`/`QuarterTurns`/`Radial` —
  a real, load-bearing `#include "../params/Symmetry_PARAMS.h"` dependency, confirmed by direct
  code read. Constitution §1 assigns "symmetry" to PARAMS's own bailiwick by name ("the
  adjustable settings (the recipe): ... symmetry"), and ARCH §3.1's dependency table states MATH
  "may depend on (nothing)" — no exception exists, unlike `PARAMS`→(nothing) for the same reason.
  Moving this function into MATH would make MATH depend on PARAMS, which is not a technicality to
  route around; it contradicts both the Constitution's own layer charter and the dependency table
  in the same stroke. Likewise `ResolveSymmetryMask`/`ResolveRadialSymmetryRepeatCount`
  (`Placement_RuleBuild_PROC.h`) are pure `bool`-branch resolvers over PARAMS values — same
  problem, same ruling.
- **The lower orbit primitives are, separately, already pure MATH today — confirmed, not
  assumed.** `Placement_SymmetryOrbit_PROC.h`'s `SymmetryOrbitPoint`, `ApplyOrbitTransform`,
  `IsDuplicatePoint`, `AppendPoint`, `AppendTransformedSet`, `AppendQuarterTurns`,
  `AppendRadialTurns` reference **zero** `Params::` symbols anywhere in the file (confirmed by
  direct code read) — their one `#include "../params/Symmetry_PARAMS.h"` is dead/incidental, not
  load-bearing. **A future, separately-scoped cleanup MAY relocate this file's content verbatim
  to a new `Symmetry_MATH.h`** (dropping the unused include) with zero behavior change for
  existing PROC callers — this is a legitimate optional hygiene move, but it is **not required**
  to solve this ratification's module-boundary problem (next bullet), so it is not mandated here.
- **The actual fix: `UI` already has a fully legal path to this PROC code, via `PIPELINE`.**
  ARCH §3.1's canonical call chain is **`UI → PIPELINE → PROC → SYS`**, and `PIPELINE` is
  explicitly allowed to depend on `PROC`. `UI` cannot call `Placement_Symmetry_PROC.h` directly
  (`UI`'s "Never" column: "touching PROC directly") — but nothing stops `PIPELINE` from exposing a
  **thin, stateless query passthrough** that wraps `BuildSymmetryOrbit`/`ResolveSymmetryMask`/
  `ResolveRadialSymmetryRepeatCount` verbatim, for `UI` to call. This costs zero relocation, zero
  behavior change to any existing PROC/`Symmetry_PARAMS.h` code, and guarantees `UI`'s drag/add
  preview and PROC's real scatter pass call the **exact same function** — the one property that
  actually matters for avoiding v1's duplicated-math bug (a duplicated bit-dispatch loop would
  have been the real risk the design's item 3 was worried about; this eliminates that duplication
  entirely rather than trading it for a law-violating relocation).
- **New law, named explicitly (Constitution's ARCH charter: "propose the precise rule to add,
  don't invent it silently").** `PIPELINE` gains a second, lighter kind of responsibility beyond
  §3.3's "generation orchestration" — a **stateless query passthrough**: a narrow, explicitly-named
  forwarding function with **no DAG node, no dirty-hash involvement, no stage registration, and no
  DATA read/write** — purely a legal doorway for `UI` to reach a handful of pure PROC math
  functions the same way PROC itself calls them. §3.3 is amended above to name this. Binding
  scope, so it cannot be used as a backdoor for heavier UI→PROC needs later:
  1. **Applies only to functions that are already, independently, pure** — no DATA dependency, no
     side effects, no GL/SYS resource ownership, re-callable every frame with no state carried
     between calls. `BuildSymmetryOrbit`/`ResolveSymmetryMask`/`ResolveRadialSymmetryRepeatCount`
     qualify by direct inspection (their only inputs are PARAMS values and plain scalars).
  2. **Lives in its own tiny `PIPELINE` file** (e.g. `SymmetryQuery_PIPELINE.h`, following the
     existing narrow-single-purpose `PIPELINE` file precedent — `PreviewDriver_PIPELINE.h` is the
     model to match in size/shape), not bolted onto `Generation_PIPELINE`/`GenerationAssembler`,
     which own the real DAG.
  3. **Never becomes a second dispatch mechanism.** It does not choose CPU/GPU, does not consult
     `DispatchPolicy`, and does not run on a worker thread — it is a direct, synchronous call,
     exactly matching how `UI`'s own C2 interaction-scoped redraw tier (§14.8) already expects a
     per-frame, zero-DAG-cost query during an active drag gesture. This ratification names §14.8
     as its own first real consumer.
  4. **A future request to route a heavier/stateful PROC capability through this same pattern
     needs its own ARCH ruling** — this ratification authorizes the pattern for exactly the three
     named pure functions above, not as a general "UI may ask PIPELINE to forward anything"
     liberty.

