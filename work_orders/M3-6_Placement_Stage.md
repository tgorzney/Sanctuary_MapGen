# Work-Order M3-6 — Placement stage (`Placement_PROC`), CPU (+ GPU preview) 

*Constitution §7. Milestone M3. Executor: SanGen Coder. Independent of other M3 stages
(own files) — parallel-safe with worktrees.*

## Title
Seeded scatter of markers / props / units from rules.

## Root problem
`PLACEMENT_SCATTER_SPEC`: today placement is three divergent mechanisms, `Gen_Placement`
is unimplemented, scatter uses non-seeded `rand()`, and props are AoS-mislabeled-SoA.
Build one seeded scatter module on the M0 spatial primitives.

## Target files
`src/proc/Placement_PROC.h` / `.cpp` / `_Test.cpp` (+ optional `.glsl` for the preview
density gate).

## Layer & accuracy
`PROC`. Output **Exact** (Cpu — spacing/markers, gameplay-authoritative); Preview may use
a Gpu density gate (Visual).

## Inputs / outputs
In: `MapFields.heightfield` + derived slope + `materialMasks`, marker/prop/decal rules,
seed. Out: resolved `Data::Markers`/`Props`/`Units` instance arrays (real SoA).

## Contract
- Seeded, position-hashed Poisson-disk / blue-noise scatter (no `rand()`); consume
  `RadialClearance_MATH` + `JumpFloodDistanceField_MATH` (M0-5) for clearance/exclusion.
- Per-rule gates (slope/height/biome/mask), scale/rotation/align ranges, symmetry (owned
  here, deterministic via `SymmetryId`). Markers must stay **AI-analyzable**
  (`AI_HOSTCLIENT_SPEC`): valid reachable spawns/alloys/expansions.
- Real SoA instance buffers with full round-trip fields (tpId, rotation, biome,
  collision). Deterministic in (seed, rule, position).
- Dispatch/register as a pipeline stage (mostly CPU).

## Acceptance
Same seed → identical placement (determinism); Poisson spacing respected (no two closer
than the rule min); gates honored; symmetry clones aligned; a basic AI-analyzability
check (spawns reachable) passes; builds clean.

## Out of scope
The AI-analyzability *validation pass* proper (later, `AI_HOSTCLIENT_SPEC`); bake.
