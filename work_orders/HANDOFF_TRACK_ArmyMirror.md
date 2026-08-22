# Handoff — Army Mirror / Migration Dialog / Session Hygiene track

*Written for a consolidation session per its explicit request. This session covered more than
just army-mirror — everything it touched is captured below since this file is the only thing
that survives it.*

## A. Track identity
Primary: **Army-mirror feature** (`STEP75`). Also authored in this session: the **migration
reconciliation dialog** (`STEP26A`/`STEP26B`), three already-shipped usability fixes
(`STEP43`/`STEP44`/`STEP45`), and a newly-discovered, currently-unticketed **army-naming-format
bug**. Also did general session hygiene: cleaned an accidental 1.7GB build-artifact commit,
created branch `SanGen-v3`.

## B. Work orders written
- `work_orders/STEP43_CompactOpenSanmapButton_UI.md` — **RATIFIED**. Implemented, independently
  verified (94/94 tests), uncommitted on `SanGen-v3`. Complete, no holes.
- `work_orders/STEP44_PreviewWindowFitScaling_UI.md` — **RATIFIED**. Implemented (with one
  self-caught correction to the fallback logic), verified 94/94, uncommitted. Complete.
- `work_orders/STEP45_RenameExeToV3_UI.md` — **RATIFIED**. Implemented, verified 94/94,
  uncommitted. Complete.
- `work_orders/STEP26A_MigrationLosslessFlagAndPreview_IO.md` — **DRAFT**, complete, no holes.
  Ready for coder dispatch. Not yet dispatched (holding per "do not execute").
- `work_orders/STEP26B_MigrationReconciliationDialog_UI.md` — **DRAFT**, complete, no holes.
  Depends on STEP26A landing first (sequencing only, not a design gap).
- `work_orders/STEP75_ArmyMirrorSymmetry_UI.md` — **DRAFT**, complete, but **blocked** (see D).
- `work_orders/BRIEF_OptimizedPreviewPipeline.md`, `BRIEF_MarkersTabUI.md`,
  `BRIEF_ScenarioScriptingRatification.md` — these are **design-conversation briefs, not work
  orders**, written earlier this session before discovering ARCH §14/§15/§16 already existed for
  two of the three. `BRIEF_ScenarioScriptingRatification.md` turned out to be the real, successful
  source document for the entire scenario-scripting track (confirmed by that track directly) — not
  redundant, leave it as historical record. The other two (`BRIEF_OptimizedPreviewPipeline.md`,
  `BRIEF_MarkersTabUI.md`) duplicate the preview-overlay-layering and marker-layer-symmetry
  tracks' own already-ratified design work — **recommend retiring/ignoring both**, they should not
  be used to start fresh conversations. (Note: `BRIEF_MarkersTabUI_R2.md` also exists on disk —
  not written by this session; presumably the marker-symmetry track's own revision.)

## C. Work orders not yet written
- **Army-naming-format bug** (no filename claimed yet — candidate `STEP76_ArmyNameFormatGuard_IO.md`,
  layer IO+UI). Scope: `NextArmyName` (`ArmiesTab_UI.h:77`) produces `"Army1"`/`"Army2"` style
  names; the engine requires zero-padded `ARMY_XX`. Verified independently by the scenario track
  (`map-generator-ff`) against real code (`MapExporter_Armies_IO.cpp:75`) and a real shipped file
  (Pandemonium Isthmus uses `ARMY_01`..`ARMY_06`). Breaks: engine slot assignment (alphabetical
  sort), `markers.Spawn.transforms` lookup, and the scenario track's own `ARMY_ID_TO_NAME`
  derivation. **Not written because the human hadn't yet answered whether I take it** (asked,
  pending — see E).

## D. Blocked / in design
- **STEP75 (army-mirror)**: blocked on **(ii) another session** — needs `STEP68`
  (`src/pipeline/SymmetryOrbitQuery_PIPELINE.h`, `Pipeline::BuildWorldSymmetryOrbit`) to exist on
  disk first; owned by the marker-layer-symmetry track. That track confirmed (via cross-session
  message) they will NOT extend their struct for my orientation-rotation need — I'm adding a
  separate small sibling function (`ApplyHalfTurnYaw`) once their file lands. No open design
  question, purely a landing-order dependency.
- **STEP26B**: blocked on **(iv) sequencing**, not a real blocker — needs `STEP26A` implemented
  first, same track, no cross-session dependency.
- **STEP26A**: not blocked technically. Held only because this session was told not to execute.
- **Army-naming bug (unticketed)**: blocked on **(iii) human decision** — see E.1.
- **ARCH §17 backfill** (STEP26's "9 migrations' actual values" table): originally blocked on
  ARCH.md's pre-split size ceiling — an ARCH Expert dispatch for this literally died mid-write
  with "response exceeded 64000 output token maximum." **This blocker may now be resolved** per
  the consolidation message's claim that ARCH.md has been restructured — worth re-attempting once
  implementation resumes. Until then, the determination this §17 entry would have recorded is
  captured directly inside `STEP26A`'s own text (the 9-migration table), so it's not lost, just
  not yet promoted into the ARCH pack itself.

## E. Human decisions pending
1. **Take the army-naming-format bug as a new ticket, or leave it?** Asked explicitly, no answer
   received before this handoff. If yes: needs both a forward fix (export-time loud non-blocking
   validation per `MigrationEntry`-style discipline, never auto-rename an authored name — the
   scenario track already specified this exact warning shape in their `STEP73` §0, reusable) and a
   migration/repair story for `.sanmap` files already exported with `Army1`-style names (renaming
   alone isn't enough — those names are baked into `armies` JSON keys and referenced by
   `markers.Spawn.transforms`).
2. **The two now-redundant briefs** (`BRIEF_OptimizedPreviewPipeline.md`, `BRIEF_MarkersTabUI.md`)
   — asked whether to rewrite them to point at the existing preview-overlay-layering / marker-
   layer-symmetry tracks instead, or whether the human would rather message those tracks directly.
   Never answered. Low urgency — just don't let a future session start a fresh design conversation
   from either file without checking this first.
3. **Go-ahead to actually dispatch coders** for `STEP26A` and (once `STEP68` lands) `STEP75` — both
   are execution-ready, held only per this session's explicit "do not execute" instruction.

## F. Cross-track dependencies
- `STEP75` → needs `STEP68` (marker-layer-symmetry track) landed first. Already coordinated
  directly with that track (confirmed: no struct extension, I add a separate function).
- `STEP26B` → needs `STEP26A` landed first (same track, internal ordering).
- The would-be army-naming fix would touch `ArmiesTab_UI.h`, `MapExporter_Armies_IO.cpp`,
  probably `MapImporter_Armies_IO.cpp` (import-time warning for legacy files), and interacts with
  the scenario track's `STEP73` (`ARMY_ID_TO_NAME`/`KNOWN_ALLOY_MARKERS` derivation assumes
  conforming names) — coordinate with that track if this is taken on.
- Nothing else currently owed to any other track.

## G. Uncommitted context (exists only in this session's conversation)
- **Session hygiene, already landed in files/git, but the *why* is only here:** an accidental
  1.7GB `build_step40c/` artifact commit got cleaned up (untracked, `.gitignore`'d via new
  `build_*/` pattern, deleted from disk) — see `work_orders/STEP1_ShippingBugFixes.md`-adjacent
  commit history. Branch `SanGen-v3` was created from `SanGen-v2` specifically for this "piece by
  piece" rework phase, per explicit human instruction.
- **The Pandemonium Isthmus import bug was explicitly dropped by the human** ("ignore the import
  bug report") after partial investigation found nothing concrete. This is a deliberate drop, not
  an abandoned thread — a future session should not silently resurrect it as "still open" without
  checking with the human first.
- **`Pandemonium Isthmus.sanmap` formatting**: the scenario track (`map-generator-16`/`ff`
  lineage) reformatted this file via a Python `json.load`/`dump` round-trip during live testing
  (semantically identical, but full whitespace/key-order/float-format change). I independently
  verified zero semantic diff, then restored the original formatting per human request from
  `Pandemonium Isthmus.sanmap.backup-2026-08-20-pre-scenariotest`. The Python-reformatted version
  is preserved at `Pandemonium Isthmus.sanmap.backup-2026-08-21_python-reformatted` — nothing was
  lost either way, but this explains why the file's formatting changed twice in one day if anyone
  investigates later.
- **`IO_MIGRATION_SPEC.md`'s `bLosslessIfSkipped` ruling is real and ratified but sitting
  uncommitted in git** (`git diff sangen_arch_pack/specs/IO_MIGRATION_SPEC.md` shows it) — it was
  written successfully by an ARCH Expert dispatch that then died trying to also write the
  corresponding `ARCH.md` §17 entry (pre-split size ceiling). Treat the spec-file text as
  authoritative law regardless of the missing ARCH-pack entry.
- **Self-correction on record**: I once directly edited `Application_Draw_UI.cpp` myself
  (violating "only the Coder touches code"), caught it within the same turn, reverted it, and
  redispatched the Coder properly. Never committed, no lasting effect — noted only so nobody is
  confused if this pattern is mentioned elsewhere.
- **Multi-session STEP-range map, as last confirmed (session identities churned throughout, this
  is the clearest snapshot available):**
  - Scenario scripting (`ARCH` §15, `MAP_SCENARIO_SPEC.md`) — `STEP63,64,65,69,70,71,72,73,74,77,78`.
    Session lineage: `map-generator-16` → `-ff` (same track, different session names over time).
  - Preview overlay layering (`ARCH` §14) — `STEP46-59,62`. Session lineage:
    `map-generator-0b` → `-17` → `-c8` → `-1f`.
  - Marker layer-scoped symmetry (`ARCH` §16) — `STEP49,60,66,67,68`. Session lineage:
    `map-generator-d7` → `-13` → `-28` → `-2f`.
  - This session (army-mirror + migration dialog + hygiene) — `STEP26A,26B,43,44,45,75`.
