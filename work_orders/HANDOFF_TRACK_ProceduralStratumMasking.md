# HANDOFF — Track: Procedural Stratum Mask Filters

Living tracking document for this feature track, updated as it progresses (unlike a one-time
session-handoff capture — this doc is expected to be re-opened and edited across many sessions).
Started 2026-09-12, human request: generalize the Stratum Tab's single hardcoded slope gate into a
per-stratum chain of procedural filters (slope, height, roughness, more later), each with its own
settings, a blend mode reusing the preview system's vocabulary, and an alpha — composed only within
the stratum they're assigned to. First concrete step requested: compact the Stratum Tab's existing
Enabled/Name UI to match the app's established compact-widget convention.

## A. Track identity

Governs **ARCH §23** (`ARCH_23_ProceduralStratumMaskFilterChain.md`, ratified 2026-09-12), formalizing
`MASKING_SPEC.md`'s own flagged-but-unscoped "v2 mask-of-masks" open item (end of Part 2 of that
spec). Claimed STEP number so far: **STEP268** (independent UI ticket, does not itself require ARCH
ratification — see B). Next free STEP number for anything further in this track: **STEP269**.

## B. Work orders written (exact filenames, status)

- `work_orders/STEP268_StratumTabCompactRow_UI.md` — **SHIPPED**, commit `62d46cb` on `SanGen-v3`.
  Compacted the Stratum Tab's Enabled+Name row (one line, hidden labels, Name gets hint text) and the
  Soil panel's Erodable checkbox; added a `bLabelHidden` parameter to the shared `DrawCheckbox` widget
  (matching the existing convention already on `Combo`/`ColorSwatch`/`TextInput`). Full build clean,
  197/197 tests passing (`Checkbox_UI_Test`/`StratumsTab_UI_Test` included, no regressions). One
  build note: the final app-exe link step didn't run because `SanGenV3App.exe` was open at build
  time (file lock, not a defect) — restart the app to see the change live.
- `work_orders/DESIGN_ProceduralStratumMaskFilters_R1.md` — **RATIFIED into ARCH §23, with two
  corrections** (see D). The Generator Expert's proposed shape for the filter-chain feature: a
  composed `StratumMaskFilter` list on `Params::Stratum`, left-to-right fold chain semantics,
  HeightBand/Roughness filter definitions. Superseded in authority by ARCH §23 where the two differ
  (filter storage is `std::vector<StratumMaskFilter>`, not a fixed `[8]` array +
  count; `bSlopeUseGlobal` lives per-filter, not on the stratum root) — kept on disk as the design's
  origin record, not deleted, per this pack's usual DESIGN-doc-superseded-not-erased convention.

## C. Work orders / consults not yet done (needed, no file yet)

ARCH §23 is ratified — read it (`ARCH_23_ProceduralStratumMaskFilterChain.md` + its `§23.1`-`§23.6`
subsection files), not the superseded parts of `DESIGN_ProceduralStratumMaskFilters_R1.md`, as ground
truth from here on. Sequenced roughly in dependency order:

1. ~~ARCH Expert ratification pass~~ — **DONE**, ARCH §23 (see D).
2. **Format Expert consult** — exact `.sanmap` wire shape for the new per-filter list (now a
   `std::vector`, per §23.1's correction), extending `StratumGenerationSettings` (`MASKING_SPEC.md`
   §1.7 Correction 12). **Unblocked, can start now.**
3. **IO Architecture Expert consult** — migration versioning: old maps have only flat slope-gate
   fields, need to synthesize a single-entry filter list (`SlopeGate`, `bSlopeUseGlobal` per §23.1's
   per-filter correction) on load so existing maps render unchanged post-migration. Depends on (2)
   for the exact wire shape being migrated to.
4. **Compute Optimization Expert consult** — evaluation strategy for the variable-length per-stratum
   filter chain over a `std::vector<StratumMaskFilter>`-shaped PARAMS list feeding a fixed-capacity
   PROC/GPU kernel-config record — §23.1 explicitly routes the real fixed-capacity question here
   rather than answering it at the PARAMS layer. Also: `Math::WindowedHeightVariance`'s
   cost/caching question (§23.3), SIMD layout, GPU kernel shape for a chain whose length varies per
   stratum (`DISPATCH_INTERFACE_SPEC`). **Unblocked, can start now in parallel with (2)/(3).**
5. **UI Expert consult (second one — distinct from the STEP268 consult)** — authoring UI for the
   ordered per-stratum filter list itself (type/blend-mode/alpha controls, add/remove/reorder),
   likely reusing `DraggableList`/`TreeListWidget_UI<T>`. This is also where the slope gate (and its
   new siblings) finally get an authoring surface at all — `StratumsTab_UI.h` SCOPE NOTE 4 has
   flagged the slope fields as UI-less since before this track started. **Unblocked, can start now**
   — §23.1/§23.5 already pin the per-filter field shapes it needs to draw controls for.
6. **Coder work-orders** (PARAMS shape per §23.1/§23.5, `Math::BlendMode`/`Math::CombineChannel`
   relocation per §23.2, `Math::WindowedHeightVariance` promotion per §23.3, PROC/Mask-stage chain
   evaluator, IO round-trip + migration, UI authoring panel) — numbered STEP269+ once (2)-(5) land.
   Not written yet; do not guess their split here, let the UI/Compute consults decide the natural
   file boundaries (ARCH §1.5 file-size ceilings, §3 module boundaries).
7. **Housekeeping, non-blocking** (`ARCH_23_06`'s own flagged item): `MASKING_SPEC.md` Part 1 (§1.2,
   §1.7, and the Part 2 "v2 mask-of-masks" closing note) is now superseded by ARCH §23 but not yet
   rewritten — that content update is Generator-Expert-drafted, ARCH-Expert-written, per this pack's
   usual division of labor. Can happen any time, does not block (2)-(6).

## D. Consults already done this session (2026-09-12)

- **UI Expert**, on the STEP268 scope only: recommended checkbox-first/name-second one-line layout,
  the `bLabelHidden`+tooltip approach for `DrawCheckbox` (rather than moving "Enabled" into the
  collapsing-header row, which would conflate a static label with a live toggle), hint-text (not a
  hidden-label glyph) for the Name field consistent with `ArmiesTab_RowLayout_UI`'s identical-shape
  precedent, and flagged the Soil panel's Scalars rows as a separate future layout pass (not bundled
  into STEP268).
- **Generator Expert**, on the mask-filter-chain feature itself: produced the `maskFilters` PARAMS
  shape, the left-to-right fold chain semantics, the HeightBand/Roughness filter definitions, and the
  §4 routing list now captured in `DESIGN_ProceduralStratumMaskFilters_R1.md`.
- **Peer coordination**: checked `ListAgents` and the `.claude/worktrees/` state before drafting
  STEP268 (its target files — `Checkbox_UI.h/.cpp`, `StratumsTab_Material_UI.cpp`,
  `StratumsTab_Soil_UI.cpp` — showed up as present in an unrelated worktree checkout, which is normal
  worktree behavior, not evidence of a live edit). Messaged the one other active peer session
  (`map-generator-3d`) directly; confirmed no overlap (its current work is a Scenario/Spawn-marker
  redesign, untouched by this track).
- **ARCH Expert**, ratifying `DESIGN_ProceduralStratumMaskFilters_R1.md` as ARCH §23: ratified the
  routing items as proposed (`Math::BlendMode`/`Math::CombineChannel` relocation out of the UI-only
  header, `Math::WindowedHeightVariance` promotion out of Placement-private ownership,
  `ImportedMaskMode` staying a fixed trailing step). Corrected two items: filter storage is
  `std::vector<StratumMaskFilter>` (not a fixed `[8]` array+count — every other ordered/composable
  PARAMS list in this codebase is a vector, and the per-cell kernel already consumes a
  config-flattened POD record, so the container choice has zero per-cell cost); `bSlopeUseGlobal`
  moves onto each filter rather than the stratum root, since a stratum can now hold more than one
  `SlopeGate`-type filter. Closed both of this section's open questions itself rather than blocking
  on the human (see below) and flagged one non-blocking housekeeping item (C.7).

## E. Open questions for the human — resolved by ARCH §23, recorded for the record

Both of this section's original questions were closed by the ARCH ratification (D) rather than
needing a human answer:
- The filter-count ceiling question is now moot — `maskFilters` is an unbounded `std::vector`
  (§23.1); any real fixed-capacity ceiling belongs to the PROC/GPU kernel-config layer, routed to the
  Compute Optimization Expert consult (C.4).
- `RoughnessSettings`'/`HeightBandSettings`' tunables are pinned in §23.5 by mirroring the existing
  `SlopeGateSettings` window/feather/smoothstep/invert/strength shape, plus a units-pinning rule for
  height (game units, converted once at config-flattening, mirroring `MASKING_SPEC.md` §1.8) to
  pre-empt a repeat of the slope gate's historical unit-ambiguity defect.

No open questions remain blocking for this track right now.
