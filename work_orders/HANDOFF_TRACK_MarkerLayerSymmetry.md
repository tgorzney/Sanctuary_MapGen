# HANDOFF — Track: Marker Layer-Scoped Symmetry

Written for the consolidation session per the human's handoff request. This session will be
deleted after capture — treat this file as the complete record.

## A. Track identity
Marker layer-scoped symmetry (procedural rule layers + manual instance layers), unifying how
symmetry is set for both auto-generated and hand-placed markers. Governs `ARCH_16_MarkerLayerSymmetry.md`
+ `ARCH_16_01` through `ARCH_16_10` (10 subsections, all ratified). Claimed numbers: **STEP49,
STEP60 (amended, not original author), STEP66, STEP67, STEP68**. STEP61 was claimed by another
session, retired/deleted by me per the human's direction once ARCH §16 ratified (see B).

## B. Work orders written (exact filenames, status, completeness)
- `work_orders/STEP49_ManualMarkersUI.md` — **DRAFT, complete**, no TODO holes. Manual-marker
  editor (alias/position/spawn→army/delete). No PARAMS gap. Ready for coder dispatch on the
  human's go-ahead.
- `work_orders/STEP60_MarkerInstanceLayer_PARAMS.md` — **DRAFT, complete**. Originally authored by
  a different session (not me) for Gap 1 only (`MarkerInstanceLayer` + `layerId`, no symmetry). I
  amended it this session to add the `Params::SymmetrySetting symmetry` field + its JSON
  round-trip, per ARCH §16's ratified shape, and retired the note that used to defer symmetry to
  STEP61. No holes.
- `work_orders/STEP66_MarkerRuleLayer_PARAMS.md` — **DRAFT, complete**. Procedural counterpart to
  STEP60: new `MarkerRuleLayer`/`SymmetrySetting` types, `MapRecipe::markerRules` →
  `markerRuleLayers`, `MarkersStack` wire rewrite to the nested `Rules`-key shape (Correction 15).
  Explicitly states `src/proc/` will not compile until the PROC consumer ticket (C.1 below) lands
  — not a hole, a stated sequencing fact.
- `work_orders/STEP67_MarkersStackSymmetryMigration_IO.md` — **DRAFT, complete**. V3→V4 migration
  bridging old flat `MarkersStack` files into STEP66's new shape. Full grouping algorithm
  specified (contiguous-run-by-matching-triplet — provably lossless, no "which setting wins"
  step). **Depends on STEP66 landing first** (stated in the ticket).
- `work_orders/STEP68_MarkerSymmetryLinkage_PIPELINE.md` — **DRAFT, one open naming item, not a
  content hole**. Adds `symmetryGroupIdentifier` to `MarkerTransform` + a new, deliberately
  domain-agnostic PIPELINE passthrough (`BuildWorldSymmetryOrbit`/`WorldSymmetryOrbitPoint`,
  working file name `SymmetryOrbitQuery_PIPELINE.h`) wrapping `Proc::BuildSymmetryOrbit` so UI code
  can legally reach the mirror math (`UI → PIPELINE → PROC`). Renamed away from an earlier
  marker-specific name after the human questioned it — now correctly generic (usable by
  Props/Decals/Units later without a duplicate wrapper). Only the exact filename needs a quick
  confirm before dispatch (see D/E).
- `work_orders/GAP_MarkerLayerAndSymmetry_PARAMS.md` — **RATIFIED/historical**. The original gap
  report (Gap 1 = STEP60's origin, Gap 2 = what became the DESIGN_R1/R2 → ARCH §16 track). Superseded
  in authority by ARCH §16 but kept as the origin record, not deleted.
- `work_orders/BRIEF_MarkersTabUI_R2.md` — **closed/historical**. The design brief that produced
  DESIGN_MarkerLayerSymmetry_R1/R2. No further action needed on it.
- `work_orders/DESIGN_MarkerLayerSymmetry_R1.md` — **RATIFIED into ARCH §16**. R2 explicitly
  supersedes its §2/§3/§4-item-2/§4-item-6; everything else in R1 still stands.
- `work_orders/DESIGN_MarkerLayerSymmetry_R2.md` — **RATIFIED into ARCH §16**, the final/current
  design. Resolves the any-member-drag identity problem (gesture-start position matching, proven
  correct by the Generator Expert, not a heuristic), the Spawn/Army shrink rule (orphan, never
  auto-delete), and the Sanmap-Spawn-vs-Scenario-Spawn scope boundary.
- **`work_orders/STEP61_ManualMarkerSymmetryAuthoring_UI.md` — DELETED**, not on disk. Authored by
  another session as a smaller, session-only "Place Symmetric" tool; its own text named
  DESIGN_MarkerLayerSymmetry_R1/R2 as its supersession condition. That design is now ratified, so
  I deleted it per the human's explicit instruction rather than leave a stale ticket.

## C. Work orders not yet written (needed, no file yet)
1. **PROC consumer update** (intended name: next free STEP number at draft time — numbers are
   contested across tracks right now, check current max first). Layer: PROC. Scope: make
   `src/proc/Placement_Rules_PROC.cpp` and `src/proc/Placement_Hash_PROC.cpp` read the new
   `recipe.markerRuleLayers` two-level shape instead of the old flat `recipe.markerRules`. **Full
   exact shape already scoped by a Generator Expert consult this session — see G, it is not
   written into any file and would be lost otherwise.**
2. **The actual drag-and-follow UI** (manual marker symmetry: drag any group member, others
   recompute live). Layer: UI. Implements `DESIGN_MarkerLayerSymmetry_R2.md` §1/§2 in real code —
   gesture-start position matching, live recompute during drag, cardinality-change handling
   (ghost-preview during drag, commit only at mouse-up), Spawn-group resize-via-drag refusal. See D
   for why this isn't written yet.
3. **UI tab wiring for the new layer-level symmetry controls.** Layer: UI. `MarkersTab_Rules_UI.h`/
   `.cpp` currently call `DrawPlacementSymmetryAxes` **per rule**; once STEP66 lands that call needs
   to move to the layer level (`MarkerRuleLayer.symmetry`) to keep compiling meaningfully. STEP66's
   own out-of-scope section flags this as a known gap with no ticket yet.
4. **Manual Marker Layers tab** (`MarkersTab_ManualLayers_UI.h`/`.cpp`, Phase 2 of the original gap
   report). Layer: UI. `DraggableList<MarkerInstanceLayer>` block — add/reorder/toggle, symmetry
   axes control, the layerIndex clamp/renumber repair on delete/reorder (mirrors
   `PropsTab_Manual_UI.h`/`.cpp` exactly). STEP60 explicitly deferred this.
5. **Layer picker on STEP49's per-instance editor.** Layer: UI. Small addition — a `Combo_UI` over
   `recipe.markerLayers`, deferred alongside item 4 since it needs that layer list to exist as a
   real UI concept first.
6. **Export-time validation: warn on an Army with no matching Spawn marker.** Layer: IO. Ruled in
   scope by the Format Expert (per-Army scan, not per-group — subsumes "missing Spawn group
   entirely" for free) but never built; STEP49's own out-of-scope note first flagged the need.

## D. Blocked / in design
- **STEP68 filename** — (i) waiting on an ARCH ruling. One-line confirm of
  `SymmetryOrbitQuery_PIPELINE.h` vs. an alternative. Everything else in that ticket is settled.
- **C.2 (drag-and-follow UI)** — (ii) waiting on other sessions' work landing as real code, not
  just drafts: `STEP49` (mine, drafted not built), `STEP47`/`STEP48` (preview-overlay track's
  world↔screen projection + spatial-grid click picking — both fully drafted, not built; STEP48 has
  its own small open architecture question flagged in its own file). Writing C.2 before those exist
  in code would describe UI with nothing real to attach to.
- **C.1 (PROC consumer)** — not actually blocked, just not transcribed into a STEP file yet. Fully
  scoped already (see G). Should land together with or immediately after STEP66 — STEP66 states
  `src/proc/` won't compile without it.
- **C.3 (UI tab rewiring)** — no blocker beyond STEP66 landing first; ready to draft whenever
  someone picks it up.
- **C.4/C.5 (Manual Marker Layers tab, layer picker)** — (v) unresolved only in the sense of not
  yet prioritized; no external blocker.
- **C.6 (export-time Spawn/Army warning)** — (iv) no spec/ticket written yet; low priority, purely
  a UX-polish validation, not correctness-critical (an orphaned Army is already a legal, tolerated
  state per ARCH §16.8 — no data corruption risk).
- **STEP67's "non-adjacent same-triplet cosmetic merge"** — explicitly NOT to be built — (ii)
  needs a Generator Expert order-independence sign-off first (would change flattened rule-execution
  order, unverified whether `Placement_Rules_PROC.cpp`'s priority/overlap resolution tolerates
  that). Not scheduled, just flagged as a possible future enhancement.

## E. Human decisions pending
1. **STEP68's connector-file name** — confirm `SymmetryOrbitQuery_PIPELINE.h` or provide an
   alternative. Low stakes, doesn't block writing/reading the ticket, only its coder dispatch.
2. **Bundle C.1 (PROC consumer) with STEP66 into one dispatch, or keep sequenced as two tickets?**
   My draft assumed two (matches this session's convention of one-concern-per-ticket), but they're
   tightly coupled (STEP66 doesn't compile without C.1) — the human may prefer one combined ticket.
3. **Should C.2 (drag-and-follow UI) be drafted now anyway**, ahead of STEP47/48/49 landing in
   code, so it's ready the instant they do — or wait until they're real? I chose not to draft it
   prematurely (nothing concrete to cite line numbers against yet, same reasoning STEP61's original
   author used for STEP49 dependency) — but this is a judgment call, not a hard rule.

## F. Cross-track dependencies
- **Owed to other tracks:** nothing currently blocking another track on my output.
- **Needed from the preview-overlay track:** `STEP47`/`STEP48` must land as real code (not just
  drafts) before C.2 can be written.
- **Needed from within my own track:** `STEP49` must land as real code before C.2 can be written.
- **Internal ordering:** `STEP66` before `STEP67` and before `C.1`. `STEP60`/`STEP68` are
  independent of `STEP66`/`STEP67` and of each other — any order.
- No dependency the other direction found — nothing in the preview-overlay or scenario tracks
  appeared to need my track's output as of this session's last check.
- ⚠️ **STEP-number collision already found and resolved this session, worth the consolidator's
  awareness:** a "Correction 15" numbering collision between my `SANMAP_FORMAT_SPEC.md` work
  (`MarkersStack`) and the scenario track's staged `Scenarios` correction — the scenario track
  already renumbered theirs to Correction 17. Also observed (not mine to resolve): a live STEP72
  collision between an army-mirror ticket and scenario-track tickets, renumbered to STEP75 by that
  session mid-conversation. Numbers have been volatile across all tracks this session — the
  consolidator should treat any STEP number below the current true max as unverified until
  re-checked against disk.

## G. Uncommitted context — real content that exists only in this conversation
**The PROC consumer ticket's (C.1) exact shape — the single most important thing to preserve here,
scoped by a live Generator Expert consult this session, never written to a file:**

Today (`Placement_Rules_PROC.cpp`, `AppendMarkerRules`): a flat single loop over
`recipe.markerRules`, where the loop's flat `index` doubles as both array position and the
`ruleIndex` fed into `MakeRuleSeed`/`MakeCommonConfiguration` for seed decorrelation — critically,
this counter still advances even for rules skipped by the `!bEnabled && !bHidden` gate, since it's
inside a normal `for`, not incremented conditionally.

Required after-shape: a two-level walk (outer over `recipe.markerRuleLayers`, inner over
`layer.rules`), with **one flat counter threaded through both loops** to exactly preserve today's
seed-decorrelation numbering — losing this would silently reseed every marker rule following a
skip, a real determinism regression, not a style choice. Symmetry resolved from
`layer.symmetry.bSymmetryUseGlobal`/`.symmetryMask`/`.radialSymmetryRepeatCount` instead of
`rule.*`. Suppression becomes `(!layer.bEnabled && !layer.bHidden) || (!rule.bEnabled &&
!rule.bHidden)` — **the human has since ruled `bEnabled` is a real generation gate** (not
UI-only), captured in STEP66's header, so this OR-combination is now confirmed correct, not just
inferred from structural symmetry as the Generator Expert originally flagged it.

`Placement_RuleBuild_PROC.h` (`MakeCommonConfiguration`/`ResolveSymmetryMask`/
`ResolveRadialSymmetryRepeatCount`) needs **zero changes** — already generic/pure; only the
caller's field sourcing moves.

**Second real PROC-layer consumer, found by the Generator Expert, missing from ARCH's own routing
list — do not skip it:** `src/proc/Placement_Hash_PROC.cpp`'s `PlacementStage::ComputeParameterHash()`
independently reads `rule.bSymmetryUseGlobal`/`.symmetryMask` directly (`HashMarkerRule`, its own
top-level loop over `recipe.markerRules`). This must migrate in lockstep with
`Placement_Rules_PROC.cpp` or the dirty-hash silently stops reacting to marker-symmetry-layer edits
— a stale-preview bug, not a compile error, so it's easy to miss. After-shape: a new
`HashMarkerRuleLayer` hashing `layer.bEnabled`/`.bHidden`/`.symmetry.*` once, then each
`layer.rules[i]` via a trimmed `HashMarkerRule` (same as today minus the two symmetry-hash lines),
top-level loop nested over `recipe.markerRuleLayers`/`layer.rules`. `HashGateCore` (shared
template) is untouched — it only reads gate-core fields, none of which moved.
`HashPropRule`/`HashUnitRule`/`HashDecalRule` are correctly untouched (their domains keep the flat
per-rule shape).

**Test fixtures confirmed (by direct grep this session) to need mechanical updates or the build
breaks** — not logic bugs, just real consumers of the old shape:
- `src/proc/Placement_Symmetry_PROC_Test.cpp` — constructs `MarkerRule` and sets the 3 symmetry
  fields directly, pushes to `recipe.markerRules`. This is the file exercising the exact
  local-override-vs-global bug (STEP16) the symmetry system guards against — needs real care
  rewriting, not a mechanical find-replace, since it asserts specific resolved values.
- `src/proc/Placement_Gpu_PROC_Test.cpp`, `src/proc/Placement_PROC_Test.cpp` — simpler, one
  `MarkerRule` pushed into `recipe.markerRules` each.
- `src/pipeline/GenerationAssembler_TestScene_PIPELINE.h` — the M3-8 end-to-end fixture; builds a
  spawn `MarkerRule` relying on its default `bSymmetryUseGlobal == true` with an explicit comment
  tying that default to an exact-count assertion elsewhere.
- `src/params/MapRecipe_PARAMS_Test.cpp` (`recipe.markerRules.resize(3)`) — PARAMS-territory,
  STEP66's own domain, not the PROC ticket's, but will also break and needs fixing in the same
  landing.

Confirmed via grep this session: no other file under `src/proc/`/`src/pipeline/` touches
`recipe.markerRules`/`MarkerRule`'s symmetry fields — the two files above are the complete set of
real PROC-layer consumers, contrary to ARCH §16.10's routing list naming only one.

**Everything else material to this track (the drag-identity math proof, the Spawn/Army
orphan-not-delete reasoning, the Sanmap-vs-Scenario-Spawn boundary, the STEP68 naming correction)
is already fully captured in DESIGN_MarkerLayerSymmetry_R1.md/_R2.md and the STEP files themselves
— no further loss risk there.**

## H. Correction (post-handoff)
`STEP60_MarkerInstanceLayer_PARAMS.md` was reported missing from disk by the consolidation session.
Verified: true — the file did not exist at time of report (`ls`/`git status` both confirmed empty).
Root cause: the preview-overlay-compositing track deleted it during a cross-track ownership
transfer (their own `HANDOFF_TRACK_PreviewCompositing.md` §B and `SEQUENCE_PreviewOverlayLayering.md:59`
record this), without my session being notified at the time.

This was **not** a lost-content situation — my amendment (the `symmetry` field, its JSON
export/import, the updated out-of-scope note) survived in this session's own conversation
transcript in full, alongside the complete original-author content from my initial full read of
the file. Re-authored `STEP60_MarkerInstanceLayer_PARAMS.md` from that transcript, byte-for-byte,
not from `ARCH_16_01_NewParamsShapes.md` alone — no `⚠️ RECONSTRUCTED — original lost` marker was
needed anywhere in the re-created file, since nothing was actually guessed or paraphrased.

File is back on disk now, DRAFT, complete, same status as recorded in §B above.
