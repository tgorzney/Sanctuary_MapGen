# Handoff — Preview Overlay Layering Track

Written for the consolidation session per its request. This session's track: markers/props/
units/decals/alloy/reclaim screen-space overlay compositing redesign (the "why do marker icons
scale with zoom" fix, widened to six domains). Co-worked with a peer session ("map-generator-59"
at last contact, name may have rotated since) on the same track — see attribution notes in each
STEP entry below; I did not author all of these myself.

## A. Track identity
**Domain:** Preview overlay layering — `PreviewComposite`/`MapCanvas` screen-space rendering,
`ARCH_14_*` (formerly monolithic `ARCH.md` §14, 13 subsections, now split per the restructuring:
`ARCH_14_PreviewOverlayLayering.md` index + `ARCH_14_01`..`ARCH_14_13`).
**Claimed:** STEP46–59, STEP62. `BRIEF_OptimizedPreviewPipeline.md` (origin brief),
`DESIGN_MarkerPreviewLayering_R1.md` (historical/superseded, do not use),
`DESIGN_MarkerPreviewLayering_R2.md` (authoritative source design, ratified into ARCH §14),
`SEQUENCE_PreviewOverlayLayering.md` (this track's own living planning/status doc — **read this
first**, it is more current than this handoff for day-to-day status).

## B. Work orders written
- **STEP46_SkipUnusedCompositeReadback_UI.md** — **DONE.** Implemented and verified (94/94 tests,
  zero test files edited) by the peer session, **before** the "no execution" directive existed.
  Not touched since. Uncommitted at last check (verify — a lot of git activity happened this
  session, may since have landed).
- **STEP47_WorldScreenProjection_UI.md** — **DRAFT, complete, no TODO holes.** Authored by me.
  World<->screen coordinate projection (`MapCanvasView`/`PreviewComposite` new pure functions).
  Prerequisite for STEP48 and Phase 3.
- **STEP48_MigrateClickPickingToSpatialGrid_UI.md** — **DRAFT, complete but flags one open call
  inside the file itself** (see D). Authored by me. Migrates click-picking off the baked
  `EntityIdBuffer` onto `Picking_UI::PickMarker`+`Data::SpatialGrid`.
- **STEP50–STEP59, STEP62** — authored by the peer session ("map-generator-59"), not me. I have
  **only spot-checked headers/opening sections of each**, not read every file in full — the
  consolidation session should re-verify completeness directly rather than trust my
  characterization. Titles/scope as of last check:
  - STEP50 `ProceduralSubLayerCsrBucketIndex_UI` — DRAFT, ⚠️ see D (stale `recipe.markerRules[i]`
    reference, cross-track).
  - STEP51 `OverlayLayerDataModel_UI` — DRAFT, ⚠️ same cross-track staleness as STEP50.
  - STEP52 `IconAtlasPairingLookup_UI` — DRAFT, not independently re-verified by me.
  - STEP53 `OverlayIconDrawPass_UI` — DRAFT, bundles Phase 3.1+3.2+3.4 (icon draw pass, vertex
    budget/decimation, two-mode LOD). Not independently re-verified by me.
  - STEP54 `ViewLayersToolbarPopup_UI` — DRAFT, not independently re-verified by me.
  - STEP55 `RetireRegenerateButton_UI` — DRAFT, not independently re-verified by me.
  - STEP56 `ManualSubLayerStableId_PARAMS` — DRAFT, implements ARCH §14.13 item 3 "Work-Order A"
    (the `layerId` field ruling I helped close earlier this session).
  - STEP57 `ManualPropsDecalsProcResolution_PROC` — DRAFT, depends on STEP56 landing first
    ("Work-Order B"). Not independently re-verified by me.
  - STEP58 `WorldFootprintSizeTable_IO` — DRAFT. Addresses ARCH §14.13 item 1 (real
    `.santp`/prop-template `footprint` data, no longer a placeholder per Format Expert consult).
  - STEP59 `OverlayVertexGenMicrobenchmark_UI` — DRAFT. Addresses §14.13 item 2 (turns the
    ~400-500k placeholder vertex budget into a measured constant). Needs a built binary to run —
    can't execute pre-build regardless of the "no execution" directive.
  - STEP62 `ReclaimPropFilter_PARAMS` — DRAFT. `bReclaimable` flag partitioning
    `recipe.propRules`/`recipe.props`, resolving the Reclaim-domain gap.

## C. Work orders not yet written
- **Phase 1.4 — retire `Data::EntityIdBuffer`'s write path.** No filename claimed yet (would be
  STEP-numbered by whoever picks it up; my track's numbers stopped at 62, next free per other
  tracks' claims was 70+ as of last contact — check current state, this moves fast). **Layer:
  UI/SYS.** Scope: remove the entity-id GPU/CPU compositing pass in `PreviewComposite`
  (`PreviewComposite_Cpu_UI.cpp`, `PreviewComposite_UI.glsl`), its GPU buffer allocation
  (`PreviewComposite_GpuBuffers_UI.cpp`), and STEP46's now-orphaned entity-id readback line. See D
  for why this isn't written yet.

That's the only genuinely unwritten ticket in this track — everything else in
`SEQUENCE_PreviewOverlayLayering.md`'s Phases 0-5 has a STEP file.

## D. Blocked / in design
- **Phase 1.4 (above):** (v) sequencing dependency, not a design question — cannot be safely
  scoped until STEP48 actually lands and is confirmed to be `Data::EntityIdBuffer`'s last
  consumer. Drafting it now would risk describing a retirement that isn't actually safe yet.
- **STEP48's internal open call — RESOLVED as of this handoff being written** (the file changed on
  disk mid-write, someone else's ARCH ruling landed in real time). Ruled: `MapCanvas` gets a
  `const PreviewComposite*` member (`SetPreviewComposite()`), injected the same way it already
  takes `Sys::GpuResourceManager*` — both are `UI` layer, so this is an intra-layer edge, not a
  dependency-table violation. The rejected alternative (`Application` re-pushing derived numbers
  via callback) would've created a second copy of live baked state to keep in sync — the exact
  anti-pattern the ticket's own Problem section already cites. STEP48 is now fully closed, no
  remaining open calls. **No longer a blocker on anything.**
- **STEP50/STEP51 (peer-authored):** (ii) waiting on another track. Both reference
  `recipe.markerRules[i]` as a flat array in their code snippets. The marker-layer-symmetry track
  (ARCH §16, now `ARCH_16_*`) restructured this to nested `markerRuleLayers` — confirmed directly
  by the peer author ("map-generator-59") mid-session, who said they'd rework both once that
  track's migration (STEP66, that track's numbering) landed. **Unknown whether that rework has
  actually happened as of this handoff** — the consolidation session should check STEP50/51's
  current text against `ARCH_16_01`'s ratified shape before treating either as implementation-
  ready.
- **§14.13 items 1/2 (footprint source, vertex budget):** (v) stated but answerable only by
  running STEP58/STEP59 — not blocked on discussion, blocked on execution the "no execution"
  directive correctly defers.

## E. Human decisions pending
None outstanding specific to this track. Every ARCH-level open question this track generated
(blendMode reuse, Decals data source, manual sub-layer stable-id shape) was resolved via expert
consult + ARCH ratification earlier in this session — see G for the trail if it matters later.
The one open call in D (STEP48's `MapCanvas`/`PreviewComposite` coupling) is an architecture-
boundary judgment call suitable for the ARCH Expert, not necessarily the human, but flagging since
nobody has ruled on it yet.

## F. Cross-track dependencies
- **I owe nothing blocking to another track.** Phases 0-4 are fully self-contained. Phase 5
  (STEP56/57, manual Props/Decals sub-layers) is also self-contained PARAMS/PROC work, no
  external dependency.
- **I am owed a landing from the marker-layer-symmetry track**: STEP50/51 cannot be finalized
  until that track's `markerRuleLayers` restructuring (their STEP66) is confirmed stable. Not a
  hard blocker on drafting (both files already exist) — a blocker on trusting them as
  implementation-ready.
- **Informational, not a dependency**: the marker-layer-symmetry track built
  `SymmetryOrbitQuery_PIPELINE.h` (domain-agnostic orbit math) which I pointed a separate,
  unrelated "army-mirror" track at, since their 180°-rotation need looked like a natural reuse
  candidate. They ultimately chose a separate small PIPELINE function instead of extending it —
  resolved between those two tracks directly, does not touch this track.
- **ARCH restructuring citation risk**: every STEP file in this track (mine and the peer's) cites
  section numbers as `ARCH.md §14.x`. Under the new `ARCH_NN_*.md` split, those citations should
  still resolve correctly by number (`ARCH_14_01` etc. preserve the same subsection numbering per
  the restructuring's own file list), but **this was not verified by me** — the consolidation
  session should confirm `ARCH_14_PreviewOverlayLayering.md`'s content matches what every STEP
  file in this track assumes before treating any of them as dispatch-ready.

## G. Uncommitted context — true, important, exists only in this conversation
- **A near-miss earlier this session, worth the record**: I initially overwrote
  `work_orders/DESIGN_MarkerPreviewLayering_R1.md` via the `Write` tool without reading its
  existing content first (violated normal read-before-write practice). Since that file was never
  committed to git, whatever was originally in it (if different from what's there now) is not
  recoverable. Practical impact was low — `DESIGN_MarkerPreviewLayering_R2.md` (a separate,
  already-existing, more complete file) was the actual authoritative source and is what
  `ARCH_14_*` was built from — but flagging the incident itself since nobody else would know it
  happened.
- **A related, larger near-miss**: shortly after, an ARCH-ratification pass (mine, dispatched to
  the ARCH Expert subagent) wrote a new §14 into the then-monolithic `ARCH.md` based on the
  now-known-wrong R1 document. That was caught (by the human, mid-session) before it caused
  lasting damage, reverted via `git checkout` (both `ARCH.md` and `PREVIEW_COMPOSITING_SPEC.md`
  were tracked/committed at that point, so the revert was clean and lossless), and re-authored
  correctly from R2. The commit trail if anyone wants to audit it: commit `7f89429` ("Amend §14
  overlay-layering ratification: resolve blendMode and Decals gaps") and `a6c90f7` ("ARCH and
  Workorders") both predate the `ARCH_NN_*.md` restructuring and contain the monolithic-era
  history.
- **I did not independently re-read STEP50–59/62 in full** before this handoff — my
  characterization of their completeness in section B is based on headers/spot-checks and the
  peer author's own self-report, not a line-by-line review. Flagging explicitly per this
  handoff's own instruction not to overstate confidence.
- **Session-identity instability observed all session**: the same underlying peer sessions
  appeared to me under different `ListAgents` names at different points in this one conversation
  (observed chain: "map-generator-f7" -> "map-generator-16"; "map-generator-0f" -> possibly
  "map-generator-13" per another peer's reference; "map-generator-59" -> possibly "map-generator-
  2f"; "map-generator-56" -> "map-generator-6d"). Peer self-reports sometimes showed awareness of
  their own prior name, sometimes didn't (looked like fresh context). If the consolidation session
  is also talking to these same peers, **do not assume a new `ListAgents` name is a new,
  uninformed session** — but also don't assume message history definitely carried over either;
  I saw both behaviors happen.
- **Other tracks' STEP-number claims as of last contact** (informational, may already be stale by
  the time this is read): scenario scripting = STEP63-65, 69-74 (`ARCH` old-§15, now `ARCH_15_*`);
  marker layer-symmetry = STEP49, 60, 66-68 (`ARCH` old-§16, now `ARCH_16_*`); army-mirror =
  STEP75 (new, small, unrelated track discovered late in this conversation). STEP26 was confirmed
  a genuine pre-existing gap (an old migration-reconciliation-dialog ticket that was apparently
  never actually written), unrelated to any active track, safe to claim.
- **This track is not fully done and not yet closable** in the sense of "nothing left" — Phase 1.4
  and the STEP50/51 cross-track rework are real open items, both correctly sequenced rather than
  drafted prematurely. Everything else (Phases 0, 2, 3, 4, 5 as designed) has a STEP file.
