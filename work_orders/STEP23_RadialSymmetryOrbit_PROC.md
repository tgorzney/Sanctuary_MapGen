# Work-Order — Step 23: `AppendRadialTurns` — the N-way rotation orbit generator for `SymmetryAxis::Radial`

*Constitution §2/§3/§6. Executor: SanGen Coder. Implements the PROC half of ARCH.md §13
(Radial N-fold symmetry) that §13 itself explicitly deferred: "`Radial`'s own orbit-generation
function... is new PROC work for a future coder work-order — not designed here," plus both of
§13's "Two defects recorded this session, not fixed here" (`DecalRule`'s gap is already closed,
STEP7; the `symmetryOrbitMaximum` buffer-overflow risk is defect 2 and IS this ticket's job,
since this ticket is what makes the overflow reachable). Ratified by three read-only expert
consults obtained for this ticket (Generator Expert on the rotation math, Compute Optimization
Expert on buffer sizing, ARCH Expert on a module-boundary question the Compute Optimization
Expert routed onward) — see "Ruled by this ticket" below.*

## Root problem
`Params::SymmetryAxis::Radial = 1 << 4` (`Symmetry_PARAMS.h`) and its companion
`radialSymmetryRepeatCount` (default `3`, already shipped on `MapRecipe`/`MarkerRule`/
`PropRule`/`DecalRule`/`UnitRule`/`GeoLayer`/`Layer`, STEP16) are reserved, zero-consumer fields.
`BuildSymmetryOrbit` (`Placement_Symmetry_PROC.h`) has exactly 4 explicit branches
(`MirrorAcrossX`/`MirrorAcrossZ`/`RotateHalfTurn`/`QuarterTurns`) and silently produces no clones
for a mask with only `Radial` set — confirmed live by
`Placement_Symmetry_PROC_Test.cpp:147-157`'s dormancy test. Designers cannot get N-fold radial
symmetry (e.g. an 8-player pinwheel start) today; the bit and count exist in every save file
format already but do nothing.

## Ruled by this ticket (three expert consults, reconciled)

**1. Rotation math (Generator Expert).** NOT a generalization of `AppendQuarterTurns`'s iterative
`(x,y) -> (-y,x)` swap trick — that trick is only exact because 90 degrees composes losslessly
via sign-flip/swap; no integer step does that for arbitrary N. `AppendRadialTurns` computes each
turn's angle **directly from the untransformed source offset** (not by chaining/composing a
repeated small rotation — chaining would compound float rounding linearly with turn count):
```cpp
// Same outer shape as AppendQuarterTurns/AppendTransformedSet: capture sourceCount BEFORE
// appending, then rotate every point already in the orbit (not just the original candidate) —
// this is what makes Radial compose correctly with prior bits (MirrorAcrossX/MirrorAcrossZ/
// QuarterTurns) when it runs last, and what ruling 6's 16N worst-case sizing assumes.
inline int AppendRadialTurns(SymmetryOrbitPoint* points, int count, int maximumPoints,
                             int radialSymmetryRepeatCount, float extent, float epsilon) {
    const int turnCount = radialSymmetryRepeatCount > 1 ? radialSymmetryRepeatCount : 1; // ruling 2 floor
    const int sourceCount = count;
    const float center = extent * 0.5f;
    for (int index = 0; index < sourceCount; ++index) {
        const SymmetryOrbitPoint source = points[index];
        const float offsetX = source.positionX - center;
        const float offsetY = source.positionY - center;
        for (int turn = 1; turn < turnCount; ++turn) {
            const float angle = static_cast<float>(turn) * (2.0f * symmetryPi / static_cast<float>(turnCount));
            const float cosine = Math::Cosine(angle);
            const float sine   = Math::Sine(angle);
            SymmetryOrbitPoint clone = source;
            clone.positionX = center + (offsetX * cosine - offsetY * sine);
            clone.positionY = center + (offsetX * sine   + offsetY * cosine);
            clone.yawOffsetRadians = source.yawOffsetRadians + angle;   // yawScale unchanged: rotation, not mirror
            count = AppendPoint(points, count, maximumPoints, clone, epsilon);
        }
    }
    return count;
}
```
using `Math::Sine`/`Math::Cosine` from `Trigonometry_MATH.h` (never `std::sin`/`std::cos` —
determinism, same discipline `TangentFromDegrees` in `Placement_RuleBuild_PROC.h` already
follows; requires adding `#include "../math/Trigonometry_MATH.h"` to
`Placement_Symmetry_PROC.h`, which today only includes `Symmetry_PARAMS.h`). Each turn's angle is
computed **directly from the untransformed source offset**, not by chaining/composing a repeated
small rotation — chaining would compound float rounding linearly with turn count. One clone per
`turn = 1 .. turnCount - 1` (`turnCount - 1` additional clones per source point, `turnCount` total
members including the untouched source, for a `Radial`-only mask), appended through the shared
`AppendPoint` so duplicate detection and the `maximumPoints` bound apply uniformly — this is what
makes `Radial` correctly compose with prior bits (ARCH §13's "already structurally supported"
claim depends on this exact pattern, confirmed against real code by the Generator Expert's review
of this ticket).

**2. Degenerate/out-of-range turn count, two independent layers (Generator Expert +
Compute Optimization Expert, reconciled — see ruling 6 below for why both are in THIS ticket):**
- **PROC (mandatory):** before the loop, floor the local turn count defensively:
  `const int turnCount = radialSymmetryRepeatCount > 1 ? radialSymmetryRepeatCount : 1;` — this
  alone makes `N <= 1` produce zero additional clones with no special-case branch, and protects
  `BuildSymmetryOrbit`/`AppendRadialTurns` as pure, directly-testable functions (bypassable by any
  test or future caller, same posture `TangentFromDegrees` already takes for its own inputs).
  Additionally, clamp `turnCount` against `maximumPoints` before the loop starts (mirroring
  `MakeCandidateGridLayout`'s `gridSide` clamp in `Placement_Scatter_PROC.cpp` — a silent,
  defensive clamp at the point of consumption, not a substitute for the IO-level clamp below) —
  this closes ARCH §13 Defect 2's actual hazard (`AppendPoint`'s existing per-point cap prevents
  memory corruption but still silently truncates the orbit with zero diagnostic; this ticket
  should not leave that dangling since it is the ticket that makes the overflow reachable).
- **PARAMS/IO boundary (mandatory, folded into this ticket — see ruling 6):** clamp
  `radialSymmetryRepeatCount` to `[2, 12]` at every read site. `N < 2` is not a rotation
  (identity only); `12` is a policy ceiling, generous for RTS-scale radial team symmetry (no
  existing player/army-count constant in this codebase to derive it from — checked, no match).
  The PROC floor above is explicitly NOT a substitute for this — it protects the function from
  malformed calls, this protects designers from pathological data.

**3. Signature change (Generator Expert).** `BuildSymmetryOrbit` gains a new parameter
immediately after `symmetryMask` (its natural flat-sibling pairing, matching the PARAMS
convention already used across all 7 owning types):
```cpp
inline int BuildSymmetryOrbit(int symmetryMask, int radialSymmetryRepeatCount, float extent,
                              float positionX, float positionY, float duplicateEpsilon,
                              SymmetryOrbitPoint* outPoints, int maximumPoints)
```
Append the `Radial` branch **last** in the bit-check sequence (after `QuarterTurns`), matching the
existing convention that bit-check order tracks ascending bit value. This does not forbid a
designer from setting `Radial` together with `QuarterTurns`/`RotateHalfTurn` at once (a reachable
per-rule OR-able misconfiguration, not this ticket's to forbid — PLACEMENT_SCATTER_SPEC's ratified
addendum already states every set bit composes independently, ARCH-ratified, not ours to
relitigate) — add a defined-behavior test for that combination (no crash/corruption, epsilon-dedup
applies) as a secondary case, distinct from the primary `Radial + MirrorAcrossX` acceptance case
below.

**4. `ScatterRuleConfiguration`/GPU struct — do NOT touch it (ARCH Expert ruling, binding).**
`DISPATCH_INTERFACE_SPEC` §4's field-mirroring policy protects against divergent declarations of
what claims to be **the same struct/binding** aliasing each other with different strides — it does
not mandate mirroring a field the GPU kernel structurally can never consume.
`Placement_PROC.glsl`'s gate kernel "deliberately does NOT scatter" (its own header comment);
orbit generation is CPU-only forever by design, not by current omission
(`PLACEMENT_SCATTER_SPEC` "CPU vs GPU"). Binding precedent already in this codebase for exactly
this shape: `PlacementStage::ruleTemplateIdentifiers` (`Placement_PROC.h:104`,
`std::vector<Data::TemplateIdentifier>`) is a **parallel array**, built in lockstep with
`ruleConfigurations` inside `BuildRuleConfigurations()` (pushed at the same call sites as
`configurations.push_back(configuration)` in each `Append*Rules` function,
`Placement_Rules_PROC.cpp`), consumed CPU-only by index in `EmitInstance`
(`Placement_Emit_PROC.cpp:67`, `ruleTemplateIdentifiers[configurationIndex]`) — never mirrored
into the GLSL twin. `ScatterRuleConfiguration` is uploaded **raw**
(`&configuration, sizeof(ScatterRuleConfiguration)`, `Placement_Gpu_PROC.cpp:107-109`) — the
"mirror exactly" invariant protects that literal upload payload's byte layout; a parallel
`std::vector<int>` never enters that path, so it creates no alias/stride hazard. Add:
```cpp
std::vector<int> ruleRadialSymmetryRepeatCounts;   // PlacementStage member, sibling of
                                                    // ruleTemplateIdentifiers, Placement_PROC.h
```
`ScatterRuleConfiguration` (`Placement_Kernel_PROC.h`) and its GLSL twin
(`Placement_PROC.glsl:14-27`) get **no new field, no padding, no size change** — `sizeof(...)`
stays 32 scalars/128 bytes.

**5. Resolving the effective count — a rule's own reading of STEP16 ruling #2, not covered by
either consult explicitly.** STEP16 ratified `radialSymmetryRepeatCount` as a flat sibling of
`symmetryMask` specifically so "a local override with `bSymmetryUseGlobal = false` but no local
count would otherwise silently inherit the global N, defeating the point of a local override" —
meaning the SAME `bUseGlobal` switch that picks between `rule.symmetryMask`/`recipe.
globalSymmetryMask` (via the existing `ResolveSymmetryMask` helper, `Placement_RuleBuild_PROC.h`)
must equivalently pick between `rule.radialSymmetryRepeatCount`/`recipe.radialSymmetryRepeatCount`.
Add a sibling helper, same file:
```cpp
inline int ResolveRadialSymmetryRepeatCount(bool bUseGlobal, int ruleCount, int globalCount) {
    return bUseGlobal ? globalCount : ruleCount;
}
```
Call it at all 4 sites in `Placement_Rules_PROC.cpp` (`AppendMarkerRules`/`AppendPropRules`/
`AppendUnitRules`/`AppendDecalRules`) immediately beside each existing `ResolveSymmetryMask` call,
pushing the result into `ruleRadialSymmetryRepeatCounts` at the same point each function already
pushes into `identifiers` (`identifiers.push_back(...)`). `BuildRuleConfigurations()` clears the
new vector alongside `ruleTemplateIdentifiers.clear()` and threads it through all 4 `Append*Rules`
calls as a new `std::vector<int>&` output parameter (same pattern as the existing `identifiers`
parameter).

**6. Buffer sizing (Compute Optimization Expert).** Every orbit-family bit multiplies the running
point count: `MirrorAcrossX`x2, `MirrorAcrossZ`x2, `QuarterTurns`x4 (`RotateHalfTurn`'s x2 doesn't
grow the worst case further — it produces the same 180-degree point `QuarterTurns` already emits,
caught by `IsDuplicatePoint`). Worst case = `N x 4 x 2 x 2 = 16N`. At the ratified `N_max = 12`
(ruling 2): `16 x 12 = 192`. Raise `Params::symmetryOrbitMaximum` (`Symmetry_PARAMS.h:27`) from
`16` to **`256`** (192 rounded up to one page / power-of-two headroom against future symmetry
families — rough-estimate sizing, not benchmarked; see the acceptance test's throughput check
below for the number that actually matters). Add a comment at the constant naming the
relationship explicitly: `symmetryOrbitMaximum` must stay `>= 16 * radialSymmetryRepeatCountMaximum`
so a future ceiling change doesn't silently reopen this defect. `orbit[Params::symmetryOrbitMaximum]`
in `Placement_Accept_PROC.cpp:33` stays a **stack array** (declared once per `AcceptCandidates`
call, outside the per-candidate loop, not per-candidate) — 256 x 16 bytes = 4KB is negligible and
stays L1-resident; do not convert it to a heap/arena allocation for this size.
`Placement_Symmetry_PROC_Test.cpp:152`'s `radialOrbit[Params::symmetryOrbitMaximum]` already reads
the constant symbolically and inherits the new size for free.

This ticket bundles the small mechanical half of Defect 2's IO-side fix (ruling 2's `[2, 12]`
clamp) alongside the PROC work, since ARCH assigns Defect 2's resolution to whichever ticket makes
the overflow reachable — that's this one. Add `ReadJsonIntegerClamped(parent, key, minimum,
maximum, destination)` to `JsonPrimitives_IO.h` (new primitive). Its behavior is NOT the same as
`ReadJsonEnumeration`'s — `ReadJsonEnumeration` rejects an out-of-range ordinal and leaves
`destination` at its prior/default value; `ReadJsonIntegerClamped` must instead read the value and
CLAMP it into `[minimum, maximum]`, overwriting `destination` with the clamped result (so a saved
`500` imports as `12`, not as whatever `radialSymmetryRepeatCount`'s struct default happened to
be). The only thing shared with `ReadJsonEnumeration`'s idiom is the "silent, no logging" posture —
no logging subsystem exists in `src/sys` to make this literally "loud" (Constitution §6);
human-facing surfacing belongs on the not-yet-drafted radial-count UI widget ticket, note this gap
in a comment rather than inventing a logging primitive here. Use it at all 7 existing
`RadialSymmetryRepeatCount` read sites: `MapImporter_MarkersStack_IO.cpp`,
`MapImporter_PropsStack_IO.cpp`, `MapImporter_DecalsStack_IO.cpp`, `MapImporter_UnitsStack_IO.cpp`,
`MapImporter_HeightmapStack_IO.cpp` (both the `GeoLayer` and `Layer` blocks), and
`MapImporter_Symmetry_IO.cpp` (the global `MapRecipe::radialSymmetryRepeatCount`). Name the `[2,
12]` range once, as named constants beside `symmetryOrbitMaximum` in `Symmetry_PARAMS.h`
(`radialSymmetryRepeatCountMinimum = 2`, `radialSymmetryRepeatCountMaximum = 12`) — single source
of truth for IO, this ticket's PROC clamp, and the future UI slider.

**7. GeoLayer/Layer's `radialSymmetryRepeatCount` stays untouched by PROC.** Those two types'
field exists for the not-yet-built heightfield-symmetry PROC stage (`Placement_Symmetry_PROC`
mirrors placed ENTITIES, not the terrain field — separate, undesigned generator work, unchanged
by this ticket). Only the IO-level `[2, 12]` clamp from ruling 6 touches their read sites; no PROC
consumer changes for them here.

## Target files
- `src/proc/Placement_Symmetry_PROC.h` — add `#include "../math/Trigonometry_MATH.h"` (today it
  only includes `Symmetry_PARAMS.h`; `Math::Sine`/`Math::Cosine` are not visible through the
  existing include chain and this file will not compile without it). Add `AppendRadialTurns`
  (ruling 1/2) in `SymmetryDetail`, alongside `AppendQuarterTurns`. Change `BuildSymmetryOrbit`'s
  signature (ruling 3) and add the `Radial` branch, last in the bit-check sequence.
- `src/params/Symmetry_PARAMS.h` — raise `symmetryOrbitMaximum` to `256` with the relationship
  comment (ruling 6); add `radialSymmetryRepeatCountMinimum = 2` /
  `radialSymmetryRepeatCountMaximum = 12` named constants; remove/replace the now-stale "dormant,
  not dangerous... `BuildSymmetryOrbit` has no branch for this bit yet" comment on the `Radial`
  bit — it now has a branch.
- `src/proc/Placement_PROC.h` — add `std::vector<int> ruleRadialSymmetryRepeatCounts;` member
  (ruling 4), sibling of `ruleTemplateIdentifiers`.
- `src/proc/Placement_RuleBuild_PROC.h` — add `ResolveRadialSymmetryRepeatCount` (ruling 5),
  sibling of `ResolveSymmetryMask`.
- `src/proc/Placement_Rules_PROC.cpp` — thread the new parallel array through all 4
  `Append*Rules` functions and `BuildRuleConfigurations()` (ruling 4/5).
- `src/proc/Placement_Accept_PROC.cpp` — pass
  `ruleRadialSymmetryRepeatCounts[configurationIndex]` into `BuildSymmetryOrbit` (ruling 3/4).
- `src/io/JsonPrimitives_IO.h` — add `ReadJsonIntegerClamped` (ruling 6).
- `src/io/MapImporter_MarkersStack_IO.cpp`, `_PropsStack_IO.cpp`, `_DecalsStack_IO.cpp`,
  `_UnitsStack_IO.cpp`, `_HeightmapStack_IO.cpp`, `_Symmetry_IO.cpp` — switch their existing
  `RadialSymmetryRepeatCount` read call to `ReadJsonIntegerClamped` with the new named range
  (ruling 6). No exporter changes — export is unaffected by an import-time clamp.
- `src/proc/Placement_Symmetry_PROC_Test.cpp` — rewrite the dormancy test (lines 147-157) into
  real coverage (acceptance test below).
- `src/io/MapImporter_IO_Test.cpp` — confirm/extend coverage for the new clamp behavior at the
  `Symmetry` global section's `RadialSymmetryRepeatCount` read (already has a symmetry test
  surface per the grep of this file).

## Explicit out-of-scope
- **`ScatterRuleConfiguration`/GLSL struct changes** — ruled out entirely (ruling 4). Do not add
  any field or padding to either struct.
- **The radial-count UI widget** — no slider/spinner exists for `radialSymmetryRepeatCount` on any
  tab; a separate, not-yet-drafted UI ticket (ARCH §13's own framing).
- **`SymmetryAxisOption::Radial`'s stale `QuarterTurns` stand-in mapping**
  (`SymmetryTab_UI.h`/`SymmetryAxisMaskOfOption`) — explicitly flagged for the UI Expert by ARCH
  §13, not this PROC ticket. Designers cannot reach the real `Radial` bit through the exclusive
  Symmetry tab checkbox until that UI ticket lands; per-rule OR-able checkboxes
  (`PlacementRuleSections_UI.h`) may already be able to set the raw bit if they expose it — verify
  during implementation but do not fix any UI gap found, just note it.
- **Forbidding `Radial` + `QuarterTurns`/`RotateHalfTurn` co-occurrence** — already answered "no,
  they compose" by the ratified `PLACEMENT_SCATTER_SPEC` addendum; not ours to relitigate.
- **The heightfield-symmetry PROC stage** (mirroring the terrain field itself, not entities) —
  separate, undesigned generator work (ruling 7).
- **A SYS logging primitive for a literally "loud" clamp** — no logging subsystem exists in
  `src/sys`; the IO clamp is silent, matching `ReadJsonEnumeration`'s existing idiom.
- **O(n^2) dedup redesign** — `AppendPoint`'s `IsDuplicatePoint` linear scan becomes ~36,864
  comparisons per candidate at the N=12-plus-both-mirrors worst case (Compute Optimization Expert
  estimate). The acceptance test's throughput check (below) is what determines whether this needs
  a follow-up; do not preemptively redesign the dedup mechanism in this ticket.

## Layer & accuracy class
PROC (new orbit-generation branch) + PARAMS (buffer constant, clamp-range constants) + IO (clamp
at 7 read sites). Accuracy class: Exact (placement stays CPU-authoritative, ARCH §4.2).

## Backend policy
CPU only — matches STEP16's existing ruling. `ScatterRuleConfiguration`/GPU gate kernel:
unchanged (ruling 4).

## ARCH rules invoked
- `ARCH.md` §13 — binding, as extended by this ticket's three expert consults (rotation math,
  buffer sizing, GPU-struct module boundary).
- `DISPATCH_INTERFACE_SPEC` §4 — the ARCH Expert's ruling on this ticket clarifies (does not
  amend) that its field-mirroring clause governs shared-buffer struct integrity, not blanket
  mirroring of CPU-only data; `ruleTemplateIdentifiers` is the standing precedent. Flagged back to
  the ARCH pack for a future documentation pass (not this ticket's to write).
- Constitution §6 — the designer-facing `[2, 12]` clamp on `radialSymmetryRepeatCount`.
- Constitution §2/§3 — parallel-array shape matches the codebase's own established convention
  for CPU-only per-rule data (`ruleTemplateIdentifiers`), minimal blast radius.

## Acceptance test
In `Placement_Symmetry_PROC_Test.cpp`, replacing the dormancy test:
1. `Radial(N)` alone (`radialSymmetryRepeatCount = N` for at least two distinct N, e.g. 4 and 12
   like an 4-way and 12-way pinwheel) at a non-center position produces exactly `N` orbit members,
   each at the correct rotated angle from the source (spot-check via distance-from-center and
   angular delta, not just count).
2. `Radial(N) | MirrorAcrossX` produces exactly `2N` members, no missing/duplicate pairs
   (Generator Expert's primary composition case).
3. `Radial(N) | QuarterTurns` (the double-rotation misconfiguration) does not crash or corrupt —
   defined behavior only, epsilon-dedup applies; no exact-count assertion required.
4. A candidate exactly at map center (`extent * 0.5`, `extent * 0.5`) with `Radial(12)` set
   collapses to orbit count 1 via existing epsilon-dedup (named regression test, not a defect —
   Generator Expert's center-degeneracy note).
5. `Radial(12) | MirrorAcrossX | MirrorAcrossZ | QuarterTurns` — ruling 6's actual documented
   worst case (`16 x 12 = 192` members, NOT `Radial(12) | MirrorAcrossX | MirrorAcrossZ` alone,
   which is only `12 x 2 x 2 = 48`) — fills correctly into the raised `256` buffer with no
   truncation, confirming the widened cap actually covers the case it was sized for.
6. `ReadJsonIntegerClamped` round-trips an in-range value exactly and clamps an out-of-range
   `RadialSymmetryRepeatCount` (e.g. `0`, `1`, `500`, negative) into `[2, 12]` — OVERWRITING
   `destination` with the clamped value, not leaving it at a prior/default — on import, verified
   at minimum at the `Symmetry` global section and one per-rule stack (raw-JSON-text or re-import
   check, not just call-site inspection — same discipline every prior IO ticket this session used).
7. Direct, IO-bypassing calls into `BuildSymmetryOrbit`/`AppendRadialTurns` (mirroring how the old
   dormancy test called `Proc::BuildSymmetryOrbit` directly) exercising ruling 2's PROC-level
   defenses specifically: `radialSymmetryRepeatCount = 0` and `= 1` each produce zero additional
   clones (the floor engages with no special-case branch), and a call with an absurdly large count
   (e.g. `100000`) against a small `maximumPoints` does not overrun the buffer — the internal
   clamp against `maximumPoints` engages before the append loop runs. This is the defensive layer
   ARCH's Defect 2 exists to close; it must be tested directly, not only via the IO clamp in test 6
   (which a broken PROC implementation could pass while still being exploitable by any direct
   caller).
8. End-to-end (not just unit-level) local-override coverage: run the FULL pipeline
   (`BuildRuleConfigurations -> AcceptCandidates -> BuildSymmetryOrbit`) with a rule's
   `bSymmetryUseGlobal = false` and a rule-local `radialSymmetryRepeatCount` that DIFFERS from
   `recipe.radialSymmetryRepeatCount`, and assert the placed orbit reflects the LOCAL count, not
   the global one — this is the exact bug STEP16 flagged ("local override... silently inherit the
   global N"); a unit test of `ResolveRadialSymmetryRepeatCount` alone cannot catch a broken
   array-threading implementation between `BuildRuleConfigurations` and `AcceptCandidates`.
9. A throughput check (can be a simple wall-clock or iteration-count assertion, not a formal
   benchmark harness) comparing `AcceptCandidates` cost at a typical config (N~4, no mirrors) vs.
   the N=12-plus-both-mirrors-plus-QuarterTurns worst case (test 5's setup) — record the result in
   the ticket's completion notes; if the regression is severe, flag it as a new follow-up ticket
   rather than silently accepting it (Compute Optimization Expert's explicit ask — do not
   hand-wave this).
10. `ResolveRadialSymmetryRepeatCount` unit-level coverage: a rule with `bSymmetryUseGlobal = true`
    and a distinct local `radialSymmetryRepeatCount` uses the GLOBAL count, not its own (mirrors an
    existing `ResolveSymmetryMask` test if one exists — check `Placement_RuleBuild_PROC_Test.cpp`
    or equivalent; add one if it doesn't).

Full `SanGenV2` build stays clean; every existing test continues to pass, including
`Placement_PROC_Test.cpp`, `Placement_Gpu_PROC_Test.cpp` (parity — confirm `sizeof
(ScatterRuleConfiguration)` is genuinely unchanged, ruling 4), and `MapImporter_IO_Test.cpp`.
