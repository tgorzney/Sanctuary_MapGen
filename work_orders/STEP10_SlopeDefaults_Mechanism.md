# Work-Order — Step 10: `Params::SlopeDefaults` + `bSlopeUseGlobal` default/override mechanism

*Constitution §7. Executor: SanGen Coder. Implements the fully-ratified `MASKING_SPEC.md` §1.7
("Settings live in ONE per-stratum type — plus a shared-default layer") and the matching
`SANMAP_FORMAT_SPEC.md` `SlopeDefaults` section (SPEC-4 Correction 5's PARAMS-side mechanism;
the IO side is `StratumGenerationSettings`, Correction 12 — deliberately a SEPARATE, later ticket,
since this one alone already touches PARAMS/PROC/IO and StratumGenerationSettings has its own real
defect to fix that shouldn't be entangled with this mechanism's first landing).*

## Root problem
Per-stratum slope-gate fields (`bSlopeGateEnabled`, `minimumSlopeDegrees`, `maximumSlopeDegrees`,
`slopeFeatherDegreesLow/High`, `bUseSmoothstep`, `bInvertSlopeGate`, `slopeGateStrength`) are the
correct ground truth (`MASKING_SPEC.md` §1.7, already-standing law) — but every stratum must set
all seven independently today; there's no shared-default layer, so a new stratum starts from
`Params::Stratum`'s hardcoded defaults rather than a designer-adjustable global baseline. The
already-ratified fix: a new `bSlopeUseGlobal` flag (default `true`) on `Params::Stratum`, and a
new top-level `SlopeDefaults` record the Mask stage's existing config-flattening step consults
when a stratum has that flag set.

## Target files
New: `src/params/SlopeDefaults_PARAMS.h` (the 7 fields, as shared defaults — same shape as
`Params::Stratum`'s own 7 slope-gate fields, verbatim names). `src/io/MapExporter_SlopeDefaults_IO.cpp`/
`MapImporter_SlopeDefaults_IO.cpp` (top-level `document["SlopeDefaults"]`, one flat object, siblings
of `armies`/`atmosphere`/etc. — NOT nested in `mapGeneratorData`).

Modified:
- `src/params/Stratum_PARAMS.h` — add `bool bSlopeUseGlobal = true;`.
- `src/params/MapRecipe_PARAMS.h` — add `SlopeDefaults slopeDefaults;`.
- `src/proc/Mask_PROC.h` — `MaskStage`'s constructor gains a `const Params::SlopeDefaults&
  slopeDefaults` parameter and matching member (same reference-member pattern as `strata`).
  `ComputeParameterHash()`'s doc comment says it hashes "every per-stratum mask setting" — verify
  whether `slopeDefaults` needs to be folded into that hash (see "PROC correctness" below; this is
  the one part of this ticket most likely to have a subtle correctness bug if done by pattern-
  matching alone — get a Generator Expert ruling before implementing, not just for architecture
  cleanliness but for whether a `SlopeDefaults`-only edit currently triggers a Mask re-run at all).
- `src/proc/Mask_Prepare_PROC.cpp` — `ConfigureSlopeGate`: when `stratum.bSlopeUseGlobal` is true,
  read the 7 gate fields from `slopeDefaults` instead of `stratum`'s own; when false, read from
  `stratum` exactly as today. Do not change `ConfigureSlopeGate`'s other behavior (the tangent
  conversion, the feather-reciprocal math) — only which SOURCE record the 7 raw values come from.
- Every call site that constructs `MaskStage(...)` (find via grep — likely one, in the pipeline
  orchestrator) — pass `recipe.slopeDefaults`.
- `src/io/MapExporter_Recipe_IO.h`/`.cpp`, `MapImporter_Recipe_IO.h`, `MapImporter_IO.cpp` — wire
  the new builder/reader in, top-level, unconditional, same tier as every prior top-level-key
  ticket this session (before the `mapGeneratorData` gate on import).

## Layer & accuracy class
PARAMS + PROC + IO/BRIDGE. Accuracy class: Exact for the IO round-trip; the PROC resolution
(which source a stratum's gate values come from) must be bit-identical to reading the stratum's
own fields directly when `bSlopeUseGlobal == false` — i.e. this ticket must not change ANY
existing behavior for a stratum that opts out of the shared default.

## Backend policy
CPU/GPU parity unaffected in principle (the config-flattening step already runs once, host-side,
before either backend — same as today); confirm this holds once `slopeDefaults` is threaded in.

## ARCH rules invoked
- `MASKING_SPEC.md` §1.7 — binding mechanism, implement verbatim: default-vs-override is a
  **config source** decision at flattening time, not a PROC/kernel change. The per-stratum kernel
  itself must remain completely unaware whether a value came from a default or an override.
- ARCH_07_01_ParamsPerStratum.md §7.1 ("no rival settings type") — `SlopeDefaults` is a single global record the flattening
  step reads alongside each stratum, not a per-stratum type any stage reaches independently.
- ARCH_03_ModuleBoundaries.md §3.4 (one writing stage per DATA field; stage-hash correctness) — see the flagged hash
  question above; this is the one place this ticket could introduce a real dirty-flag bug if the
  hash isn't updated to match the new dependency.

## PROC correctness — RULED by the Generator Expert (implement exactly, do not re-derive)
Confirmed real bug: `MaskStage::ComputeParameterHash()`'s `HashStratumSettings` (`Mask_PROC.cpp`)
hashes each `Params::Stratum`'s raw fields directly (`bSlopeGateEnabled`, `minimumSlopeDegrees`,
`maximumSlopeDegrees`, feathers, `slopeGateStrength`, `bUseSmoothstep`, `bInvertSlopeGate`,
`importedMaskMode`) — nothing derived, nothing else. A `recipe.slopeDefaults` edit touching zero
per-stratum fields would NOT change this hash today, so a stratum with `bSlopeUseGlobal == true`
would silently show no preview change until an unrelated stratum edit happens to perturb the hash.

**Ruling (implement exactly):**
1. Add a new `HashSlopeDefaults(seed, const Params::SlopeDefaults&)` mirroring
   `HashStratumSettings`'s float/bool hashing pattern for the same 7 fields, called ONCE in
   `ComputeParameterHash()` (alongside the existing `HashConstants` call) — unconditionally, not
   gated on whether any stratum currently has `bSlopeUseGlobal == true`.
2. Add one `HashInteger(seed, stratum.bSlopeUseGlobal ? 1 : 0)` line inside the existing
   per-stratum `HashStratumSettings` loop.
   Reasoning (matches this file's own established style, not a fresh preference): `HashConstants`
   already unconditionally hashes every constant regardless of whether every stratum's settings
   make it "currently live" (e.g. `smoothstepShoulder/Scale` hash in even for strata with
   `bUseSmoothstep == false`); `HashStratumSettings` already hashes slope fields even when
   `bSlopeGateEnabled == false`. This file consistently chooses "hash unconditionally, a missed
   mix is a correctness bug, an extra mix only costs a harmless recompute" — the conditional
   alternative (only hash `slopeDefaults` if some stratum uses it) buys nothing at runtime anyway,
   since `PrepareRun()` re-flattens all 9 stratum slots unconditionally whenever the stage runs at
   all; there's no partial per-stratum re-run to protect by being more precise.

**Hard requirement, not left to acceptance-test discovery:** `Params::SlopeDefaults`'s own field
defaults MUST exactly match `Params::Stratum`'s current hardcoded defaults, verbatim
(`Stratum_PARAMS.h`): `bSlopeGateEnabled = false`, `minimumSlopeDegrees = 0.0f`,
`maximumSlopeDegrees = 90.0f`, `slopeFeatherDegreesLow = 0.0f`, `slopeFeatherDegreesHigh = 0.0f`,
`bUseSmoothstep = false`, `bInvertSlopeGate = false`, `slopeGateStrength = 1.0f`. Since
`bSlopeUseGlobal` defaults to `true`, every existing stratum that never explicitly sets these 7
fields will now silently resolve against `slopeDefaults` instead of its own hardcoded field
defaults — if the two default sets drift even slightly, every currently-passing Mask test and
every existing recipe changes its resolved gate output on first load. Copy the literal values,
do not re-derive or "improve" them.

**`MaskStage` construction — 11 call sites, not "likely one"; all mechanical, add a
`Params::SlopeDefaults slopeDefaults` and pass it, no architectural conflict (same reference-
member lifetime pattern already used for `strata`; `recipe.slopeDefaults` is a sibling field with
the same recipe-lifetime guarantee):**
- Production: `src/pipeline/GenerationAssembler_PIPELINE.cpp:18`.
- Tests: `Mask_DirtyHash_PROC_Test.cpp:33` (**an aggregate-member init-list, not a bare
  `MaskStage(...)` call — grepping for the literal string `MaskStage(` will MISS this site; grep
  for the bare type name `MaskStage` instead**), `Mask_Merge_PROC_Test.cpp:42,127`,
  `Mask_Parity_PROC_Test.cpp:105,119` (`cpuStage`/`gpuStage`), `Mask_Slope_PROC_Test.cpp:48`,
  `Mask_Purity_PROC_Test.cpp:72,94,123`, `Mask_WorldScale_PROC_Test.cpp:35,68`.

No CPU/GPU parity concern: `ConfigureSlopeGate` runs host-side inside `PrepareRun()`, strictly
before either backend's kernel runs; `slopeDefaults` is consumed identically to how `stratum`'s
own fields are consumed today, never reaching a kernel or shader directly.

## Explicit out-of-scope
- **`StratumGenerationSettings` (Correction 12's IO shape, plus the 6 soil-physics fields and the
  currently-write-only-to-nothing `Params::StratumSoilPhysics`)** — separate, later ticket. This
  one only lands the mechanism (`bSlopeUseGlobal` + `SlopeDefaults`) and its own minimal top-level
  IO section; it does not build the combined 9-entry `StratumGenerationSettings` array that will
  eventually carry both this mechanism's fields AND soil physics together.
- **The `stratumLayers` appearance import/export completion** (Correction 13 — the real,
  currently-shipping bug where albedo/normal/mask texture paths always export as empty strings,
  and no importer exists at all) — unrelated defect, separate ticket, do not fix opportunistically
  here even though it's adjacent code.
- **UI wiring** — no `SlopeDefaults` editor tab/section is built; `Params::Stratum`'s own slope-gate
  UI (wherever it's drawn today) is not touched or given a `bSlopeUseGlobal` checkbox in this
  ticket. Same UI-wiring exclusion every prior PARAMS ticket this session has had.
- **`StratumMask_PARAMS.h`'s standing "rival `StratumSettings` type" cleanup** (ARCH hit-list #1) —
  unrelated pre-existing violation, not this ticket's job.

## Acceptance test
A `Params::Stratum` with `bSlopeUseGlobal = true` and a populated `Params::MapRecipe::
slopeDefaults` produces the identical `MaskStratumConfiguration` as the same stratum with
`bSlopeUseGlobal = false` and its own 7 fields manually set to match `slopeDefaults`'s values —
i.e. the resolution is a pure substitution, verified by direct comparison, not just "it compiles."
`SlopeDefaults` round-trips through export→import exactly. Whatever the Generator Expert rules on
the hash question is directly tested: an edit to `slopeDefaults` alone (no per-stratum field
touched) changes `ComputeParameterHash()`'s result for any stratum with `bSlopeUseGlobal == true`,
and does NOT change it for a stratum with `bSlopeUseGlobal == false`. Full `SanGenV2` build stays
clean; every existing Mask-stage test continues to pass unchanged (this ticket must not alter
output for any recipe that never sets `bSlopeUseGlobal` away from its own explicit per-stratum
values today — though note the DEFAULT is now `true`, so a freshly-constructed `Params::Stratum`
with no explicit slope-gate values set will now resolve against `slopeDefaults` instead of its own
hardcoded field defaults; confirm this default-value change doesn't silently alter any EXISTING
test's expected output that relied on `Params::Stratum`'s previous defaults).
