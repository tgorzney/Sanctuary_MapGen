# Handoff — Preview Overlay Compositing track

## A. Track identity
Preview overlay compositing/layering: taking markers/props/units/decals/reclaim off the shared
baked GPU composite texture and turning them into a dynamic, screen-space overlay-layer system
(the "why do icons scale with zoom" fix, widened into a six-domain compositor). Numbers claimed:
`STEP46`, `STEP50`–`STEP59`, `STEP62`. `STEP47`/`STEP48`/the original `SEQUENCE_PreviewOverlayLayering.md`
already existed on disk before this session started — likely authored by `map-generator-b8` (who
confirmed working "the preview-overlay-layering redesign, STEP46-57") or a predecessor of that
session; I extended/maintained them but did not originate them. `STEP60`/`STEP61` were claimed and
then discarded this session (see D/G below) — do not resurrect those numbers for this track,
ownership of that ground transferred to the marker-layer-symmetry track.

**⚠️ ARCH restructuring**: `ARCH.md` is now a thin index; this track's ratified law lives in
`ARCH_14_PreviewOverlayLayering.md` + `ARCH_14_01`...`ARCH_14_13` (confirmed present on disk).
Every work-order below cites `ARCH.md §14.X` in its prose — those citations now point at
relocated-but-unchanged content (`ARCH_14_0X_*.md`), not stale content. Nobody has gone through
and rewritten the citations to the new filenames; low priority, cosmetic, content is correct.

## B. Work orders written
All under `work_orders/`. "Complete" = no TODO placeholders, every file/line citation verified
against real source by the drafting agent.

| File | Status | Complete? |
|---|---|---|
| `STEP46_SkipUnusedCompositeReadback_UI.md` | **RATIFIED, IMPLEMENTED** | Yes — built, `ctest -C Debug` 94/94 green, zero test edits, **committed to git** (confirmed in `git log`, part of an earlier bulk commit, not a standalone one — I did not commit it myself). |
| `STEP47_WorldScreenProjection_UI.md` | DRAFT (not authored by me) | Appears complete; I have not audited it line-by-line since I didn't write it. |
| `STEP48_MigrateClickPickingToSpatialGrid_UI.md` | DRAFT (not authored by me) | **Now complete** — I just patched its one open ❓ (MapCanvas/PreviewComposite ownership) with the ARCH ruling obtained this session; see G. |
| `STEP50_ProceduralSubLayerCsrBucketIndex_UI.md` | DRAFT | Complete, with an explicit flagged ⚠️ dependency (see D) — not a hole, a documented open dependency. |
| `STEP51_OverlayLayerDataModel_UI.md` | DRAFT | Complete, same flagged ⚠️ dependency as STEP50. |
| `STEP52_IconAtlasPairingLookup_UI.md` | DRAFT | Complete. |
| `STEP53_OverlayIconDrawPass_UI.md` | DRAFT | Complete. This is the core deliverable — bundles Phase 3.1/3.2/3.4 (draw pass + vertex budget/decimation + LOD switch) into one ticket by design. |
| `STEP54_ViewLayersToolbarPopup_UI.md` | DRAFT | Complete. |
| `STEP55_RetireRegenerateButton_UI.md` | DRAFT | Complete. |
| `STEP56_ManualSubLayerStableId_PARAMS.md` | DRAFT | Complete. |
| `STEP57_ManualPropsDecalsProcResolution_PROC.md` | DRAFT | Complete, depends on STEP56. |
| `STEP58_WorldFootprintSizeTable_IO.md` | DRAFT | Complete, ships a placeholder/manual-entry table on purpose — real `.santp` ingestion is explicitly out of scope (separate, unscoped texture-importer track). |
| `STEP59_OverlayVertexGenMicrobenchmark_UI.md` | DRAFT | Complete, gated on STEP53 being **implemented**, not just drafted (can't benchmark a binary that doesn't exist). |
| `STEP62_ReclaimPropFilter_PARAMS.md` | DRAFT | Complete. |
| `SEQUENCE_PreviewOverlayLayering.md` | Living tracking doc, not a work order | Kept current through this session; source of truth for this track's phase/dependency ordering. |
| `DESIGN_MarkerPreviewLayering_R2.md` | RATIFIED (by `ARCH_14_*`) | The design doc `ARCH_14` ratifies. Complete as a historical record; do not re-derive design from it, `ARCH_14_*` is the live source of truth now. |
| `DESIGN_MarkerPreviewLayering_R1.md` | SUPERSEDED | Historical only. `ARCH_14_PreviewOverlayLayering.md`'s own intro explicitly says "do not consult as current." |

**Deleted this session, not present on disk**: `STEP60_MarkerInstanceLayer_PARAMS.md`,
`STEP61_ManualMarkerSymmetryAuthoring_UI.md`. Do not recreate under these numbers for this track —
see D/F/G.

## C. Work orders not yet written (this track)
| Intended filename | Scope | Layer |
|---|---|---|
| (unnamed, Phase 1.4) `STEPxx_RetireEntityIdBufferWritePath_UI.md` | Remove `Data::EntityIdBuffer`'s GPU/CPU write pass in `PreviewComposite`, its GPU buffer allocation, and STEP46's now-orphaned entity-id readback line, once STEP48 is confirmed to be the buffer's last consumer. | UI (+ touches SYS-owned GPU buffer teardown) |
| (unnamed, Phase 5.3) `STEPxx_ManualSubLayersInViewToolbar_UI.md` | Wire manual Props/Decals sub-layers (from STEP56/57) into the View toolbar's "Overlays" section rows. | UI |
| (unnamed) `STEPxx_ReclaimFilterWiring.md` — possibly not its own file, see D | Wire `bReclaimable` (STEP62) as the actual Props-vs-Reclaim filter predicate into STEP50's CSR bucket build and/or STEP53's draw pass. | DATA or UI, small |
| Not scoped at all — no filename proposed | Real `.santp` Lua footprint/blueprint ingestion (STEP58 currently ships a placeholder). This is really the texture-importer track's problem, not this track's — see G for what's known. | IO (+ needs a Lua reader that doesn't exist anywhere in `src/` today) |

## D. Blocked / in design
None of this track's blockers are ARCH-ruling/expert/human-decision blockers — everything design-
relevant that this track controls has been ratified (`ARCH_14_01`–`13`). The blockers are all
**implementation-sequencing** (a prerequisite ticket must be *coded*, not just drafted, before the
next one can be correctly written or dispatched) or **cross-track** (waiting on a different
session's ticket to land):

- **Phase 1.4** (EntityIdBuffer retirement): blocked on STEP48 being implemented and confirmed to
  have zero remaining `EntityIdBuffer` readers. Nothing to design — mechanical once STEP48 lands.
- **Phase 5.3** (View-toolbar manual sub-layer wiring): blocked on STEP56 + STEP57 being
  implemented. Nothing to design.
- **STEP59** (microbenchmark): blocked on STEP53 being implemented — needs a real binary to time.
- **STEP53** itself: depends on STEP47, STEP50, STEP51, STEP52 all being implemented first (stated
  in STEP53's own Sequence line).
- **STEP50 and STEP51's `markers`-domain code paths**: **cross-track blocker.** A different,
  more-advanced session (marker-layer-symmetry track, apparently `map-generator-0f`) ratified
  `ARCH_16_*` which renames `Params::MapRecipe::markerRules` → `markerRuleLayers` (nested:
  `MarkerRuleLayer.rules`, not a flat array) — see `STEP66_MarkerRuleLayer_PARAMS.md` (not this
  track's file, do not edit it). I patched STEP50/51's markers-domain code to assume `ruleIndex`
  stays a flat/global index over the layer-concatenated sequence, flagged explicitly ⚠️ **as an
  unconfirmed assumption** in both files — the actual authority is `STEP66`'s own **not-yet-drafted**
  `Placement_Rules_PROC.cpp` consumer ticket (mentioned in `STEP66`'s "Explicit out-of-scope"
  section as a separate, undrafted follow-up). Until that consumer ticket exists and either
  confirms or contradicts the flat-index assumption, STEP50/51's `markers` bucket-index code should
  be treated as provisional. Props/units/decals portions of both tickets are unaffected and can
  dispatch independent of this.
- **Reclaim filter wiring** (item 3 in C): not blocked, just not drafted — open scoping call
  (own ticket vs. amend STEP50/53 in place) rather than a design gap.
- **Real `.santp` ingestion** (item 4 in C): blocked on (iv), a spec that doesn't exist — the whole
  texture-importer/Gamedata-folder-picker feature has never had a design session. STEP58
  deliberately ships a placeholder and stops rather than block on this.

## E. Human decisions pending
None outstanding for this track as of this handoff. Everything that was a live human decision this
session got resolved in-conversation (marker taxonomy, LOD icon two-mode design, layer-unification
two-section-no-crossing, Reclaim-as-Props-filter, Regenerate-button retirement, STEP60/61
discard-vs-patch). The two open ⚠️ items in D (marker `ruleIndex` scheme, Reclaim filter's own-
ticket-vs-amendment) are technical/sequencing calls a coder or the consolidating session can make
when the time comes, not decisions that need the human specifically.

## F. Cross-track dependencies
- **I depend on** the marker-layer-symmetry track's `STEP66` (`markerRules`→`markerRuleLayers`)
  landing, and its own not-yet-drafted PROC-consumer follow-up, before STEP50/51's `markers`-domain
  code is confirmed correct (see D). I owe that track nothing in return on this specific point.
- **I do NOT own** `MarkerInstanceLayer` PARAMS + a `symmetry` field (was this track's `STEP60`) —
  ratified shape lives in `ARCH_16_01_NewParamsShapes.md`; ownership transferred to the marker-
  symmetry track this session. If nobody has drafted a `MarkerInstanceLayer`-specific coder ticket
  by the time this handoff is read, that's a gap in **their** track, not mine — flag it there, not
  here.
- **I do NOT own** the "Place Symmetric" manual-marker authoring tool (was this track's `STEP61`) —
  same transfer. `STEP68` (their file) explicitly names it out-of-scope and still unscheduled by
  anyone as of the last check.
- **Ordering constraint**: `STEP66` (their track) should land before `STEP50`/`STEP51`'s
  `markers`-domain portions are dispatched, but Props/Units/Decals portions of those two tickets
  have no such constraint and can go first.
- No other cross-track dependencies known. The Scenario/Lua-editor track (`ARCH_15_*`) and whatever
  owns `STEP63`–`STEP65`/`STEP69`–`STEP75` are unrelated domains with no known ordering constraint
  against this track.

## G. Uncommitted context — true and important, lives only in this conversation until now
- **Multi-session discovery timeline**: this session did its own design work (3 rounds with UI
  Expert/UI Optimization Expert/Compute Optimization Expert/ARCH Expert/Format Expert/Generator
  Expert, producing `DESIGN_MarkerPreviewLayering_R2.md`) *before* discovering that a parallel,
  more-advanced ARCH-authoring session had already ratified `ARCH_14` (from the same R2 doc — i.e.
  this session's own design output was picked up and ratified by a different session's ARCH-
  authoring conversation, not this one) and was already producing `STEP47`/`STEP48`. Mid-session,
  it further discovered the marker-symmetry track (`ARCH_16`) had independently solved the same
  Gap1/Gap2 problem this session got an informal advisory ARCH-subagent ruling on — that advisory
  ruling (not written to any ratified ARCH file, just reported in chat) is **superseded** by
  `ARCH_16` and should not be treated as law by anyone who finds it quoted in old chat transcripts.
- **Exact reason STEP60/61 were discarded, not merged**: this session's own advisory consult
  produced a simpler `MarkerInstanceLayer` (no symmetry field, no `MarkerRuleLayer` procedural
  counterpart) and a marker-specific PIPELINE wrapper file name
  (`MarkerSymmetryOrbit_PIPELINE.h`/`BuildMarkerSymmetryOrbit`) that turned out to duplicate the
  other track's `STEP68` (`SymmetryOrbitQuery_PIPELINE.h`/`BuildWorldSymmetryOrbit`, deliberately
  domain-agnostic so Props/Decals/Units can reuse it later — a materially better design than this
  session's marker-specific one). The human's explicit instruction was discard-and-defer-to-the-
  -other-track rather than reconcile-and-merge — recorded here so nobody re-derives a merge later
  and re-does work that was deliberately dropped.
- **Real game-data findings** (Format Expert consult this session, grounded against the real Steam
  Demo install, not spec-derived): confirmed real fields for future footprint/reclaim work —
  `footprint = {x, y}` (ground-plane extent) on both unit templates (`.santp` Lua,
  `unitsTemplates/<id>/<id>.santp`) and props (both prop-template dialects); `collisionInfo`/
  `collider` for the 3D bounding box; Reclaim is real in-game data via `tags` containing
  `"HARVESTABLE"` + an `economy.harvest{alloys, plasma|energy}` yield table — **not** a boolean
  flag (SanGen's own `bReclaimable` in STEP62 is a deliberately simpler internal representation,
  not a 1:1 mirror of this). These are captured in STEP58/STEP62's own text already.
- **NOT yet captured anywhere else — new finding, write it down here**: the real Steam Demo
  install's sanpacks unzip to `Gamedata/<Name>.sanpack.unzipped/<Name>/...` (single-nested, e.g.
  `Environment.sanpack.unzipped\Environment\01_Highlands\...`), **not** the double-nested
  `Gamedata/<Name>/<Name>/...` layout `sangen_arch_pack/specs/GAMEDATA_LAYOUT_SPEC.md`'s "Top
  level" section currently describes. Nobody has filed a correction ticket for this spec
  inaccuracy. Relevant whenever the texture-importer track actually gets scoped.
- **NOT yet captured anywhere else — doc bug found while drafting STEP59**: `ARCH_14_09` (rendering/
  performance) and `sangen_arch_pack/specs/OPTIMIZATION_PILLARS.md` both cite a nonexistent
  Constitution "§12" for the basis-tag/benchmark law — the Constitution only has 8 sections; the
  real basis-tag section is §7. This is flagged inside `STEP59`'s own file text but no one has
  actually gone and fixed the stale cross-reference in `ARCH_14_09`/`OPTIMIZATION_PILLARS.md`
  themselves. Small, easy, unassigned.
- **Peer-session coordination this session did, for continuity**: exchanged status messages with
  three peer sessions — `map-generator-b8` (confirmed their `STEP46-57` self-description was
  imprecise, actually `STEP46` is this track's own and `STEP47/48` predate this session), a marker-
  symmetry-track session referred to as `map-generator-0f` (via `map-generator-b8`, never talked to
  directly — they're the ones who deleted `STEP61`), and `map-generator-6d` (claimed `STEP75`,
  unrelated track, no dependency either direction). None of these exchanges contain track-critical
  design content beyond what's already written into this handoff and `SEQUENCE_PreviewOverlayLayering.md`.
