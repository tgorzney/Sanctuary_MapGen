# Execution Plan — parallel batch + sequential order for the unbuilt backlog

*Rebuilt from scratch 2026-09-08, superseding the 2026-08-22 version of this document in full.
That version's entire scope (`STEP26A`, `STEP26B`, `STEP46`–`STEP97`) has since shipped — verified
by direct commit-history cross-reference, not assumed. This rebuild is a full re-audit of every
work order on disk (~200 files) against current `src/` (HEAD, clean tree) using 5 independent
research passes, following this project's own standing rule: never trust a ticket's — or a prior
audit's — self-reported status.*

## 0. Headline finding

**Of the ~200 files in `work_orders/`, only 4 were real, unbuilt, dispatchable coder tickets.**
Everything else is either already shipped (the overwhelming majority — confirmed against actual
`src/` content, not commit messages, which are frequently silent about which STEP number they
implement), historical/superseded planning material, or not yet ratified into a ticket at all
(the Assembly track). See §4 for the full accounting.

**Update 2026-09-08, later same day — Wave 1 dispatched, mixed result:**
- **STEP250 — SHIPPED**, commit `e3e8641`. Full solo rebuild clean, `ArmiesTab_UI_Test`/
  `Combo_UI_Test` both pass. Landed 2 extra files beyond its own "Files touched" list
  (`ArmiesTab_RowLayout_UI.h/.cpp`) — the ticket's own §2 anticipated this exact split if the
  edited file landed over the ARCH §1.5 size ceiling, which it did (196 lines pre-split). Both new
  files are UI-only, zero overlap with the Scenario cluster.
- **STEP252 — NOT SHIPPED, made zero code changes, routed back.** The dispatched coder found a
  real, unscoped gap: retiring `Params::ScenarioSpawn` (the ticket's whole point) breaks a live,
  compiled subsystem neither the ticket nor `ARCH_15_12`/`ARCH_15_13` ever mention —
  `MapCanvas_ScenarioEditMode_*` (canvas drag-to-place spawns) and `ScenariosTab_SpawnsWarning_UI.cpp`/
  `ScenariosTab_UI.h`'s mandatory-spawns acknowledgment banner, both of which directly construct/
  mutate `Params::ScenarioSpawn`/`body.spawns`. Deciding how canvas-drag spawn placement should work
  against the new shared `spawnId` pool is a design call outside a coder's authority — this needs
  either a scope amendment to STEP252 (extending it to cover those files with an explicit design)
  or a split-off follow-up ticket, and possibly an ARCH Expert consult if the canvas-drag semantics
  need new law. **Not resolved by this session; working tree left untouched.**
- **STEP253 — SHIPPED**, commit `4181001`, dispatched independently once STEP252 stalled (the
  sequencing rule below was to avoid a merge collision with STEP252's edits, which never
  materialized since STEP252 touched nothing). Full solution build clean; 183/185 tests passed,
  the only 2 failures being the pre-existing, unrelated baseline failures called out below — no
  regression. One correction the dispatched coder found and applied: the ticket cited
  `MapImporter_ScenarioRecord_IO.cpp` for the JSON reader, but that logic actually lives in
  `MapImporter_Scenarios_IO.cpp` (a file split that postdates the ticket's prose) — edited the real
  location. Also flagged, not fixed (pre-existing, not this ticket's fault): `MapExporter_IO.cpp`
  (158→168 lines) and `ScenariosTab_UI.h` (175→186 lines) were already over the ARCH §1.5 hard
  ceiling before this ticket touched them.
- **STEP251 — still BLOCKED**, now on STEP252's unresolved scope gap specifically (previously
  blocked on STEP252 landing at all; the blocker has changed shape, not cleared).
- **Two pre-existing, unrelated test failures confirmed real** (fail even on a clean baseline with
  none of this session's changes applied): `MapCanvas_UI_Test` (a manual-marker drag-release
  regression) and `MapExporter_Formatting_IO_Test` (a `MarkerLinks`/`decals`/`props` JSON
  key-order byte-fixture mismatch, almost certainly from the MarkerLink chain's IO changes not
  updating this fixture). Neither has a work order. Worth a follow-up ticket.

## 1. THE REAL BACKLOG — 4 tickets identified, 2 shipped, 1 blocked-on-scope, 1 blocked-on-that

| Ticket | Verdict | Files touched |
|---|---|---|
| `STEP250_ArmiesTabCompactSingleLineRow_UI.md` | **SHIPPED** (`e3e8641`) | `src/ui/Combo_UI.h`, `src/ui/Combo_UI.cpp`, `src/ui/ArmiesTab_UI.cpp`, `src/ui/ArmiesTab_RowLayout_UI.h`/`.cpp` (new, size-ceiling split) |
| `STEP253_ScenarioSlotRangeCondition_PARAMS_IO_UI.md` | **SHIPPED** (`4181001`) | `Scenario_PARAMS.h`, `MapExporter_Scenarios_IO.cpp`, `MapImporter_Scenarios_IO.cpp` (not `MapImporter_ScenarioRecord_IO.cpp` as originally cited), `ScenarioScript_DataLua_IO.cpp`, `SanGenScenarioRuntime.lua`, `ScenariosTab_MatchRules_UI.cpp` + 2 new split files + new `ScenarioSlotRangeValidation_IO.h/.cpp` |
| `STEP252_ScenarioSpawnIdentityImplementation_PARAMS_IO_UI.md` | **BLOCKED — needs a scope amendment or ARCH ruling**, not dispatchable as written; zero code changes made | Ticket's own list, plus (newly discovered, unscoped) `MapCanvas_ScenarioEditMode_*` (5 files) and `ScenariosTab_SpawnsWarning_UI.cpp`/`ScenariosTab_UI.h` |
| `STEP251_ScenarioCategoryFourExport_IO.md` | **BLOCKED** on STEP252 | New files, plus reads STEP252's `spawnIds` shape |

Every other STEP number from 98 through 249, plus the STEP150–153 and STEP200–236 ranges, plus the
full 13-ticket MarkerLink correction chain (STEP237–249), is **already shipped** — confirmed by
direct code inspection, not commit-message grep alone (several shipped without citing their STEP
number in the commit message at all, e.g. STEP227/228/230/234–236 all landed inside larger
uncited commits). See §4 for the per-ticket evidence table.

## 2. Batching — parallel wave, then a forced-sequential tail

### Wave 1 — dispatch simultaneously, zero file overlap

`STEP250` and `STEP252` share no files whatsoever (Armies-tab row layout vs. Scenario PARAMS/IO/Lua
runtime) — safe for two coders in parallel, each in an isolated worktree.

### Sequential tail — after STEP252 lands

1. **STEP253** — no *functional* dependency on STEP252 (spawn identity vs. count-condition shape
   are logically independent features), but both land in the exact same 6-file cluster
   (`Scenario_PARAMS.h`, the Scenario IO pair, `ScenarioScript_DataLua_IO.cpp`,
   `SanGenScenarioRuntime.lua`, `ScenariosTab_MatchRules_UI.cpp`). STEP253's own text explicitly
   flags this as a same-file merge-collision risk, not a data dependency — run it only after
   STEP252 has landed and merged, to avoid two coders fighting over the same regions of the same
   files at once.
2. **STEP251** — genuinely blocked: it consumes STEP252's `spawnIds` pool shape directly, and its
   own §9 says to re-verify its text against whatever actually ships before starting. Dispatch
   last.

Net order: **{STEP250 ∥ STEP252} → STEP253 → STEP251.**

### A note on peer coordination

An active peer session's worktree (`.claude/worktrees/agent-a4d614cf830e42c4d`, observed live
during this audit) is mid-edit on `MarkerLayerBundle`-adjacent files — a different domain from all
4 tickets above (no shared files), so no coordination is required before dispatching this batch.
Still, per `CLAUDE.md`'s commit protocol, whoever picks up any of these 4 tickets should re-check
`ListAgents`/active sessions immediately before touching each file, not just once at ticket start.

## 3. One real, unticketed defect found during this audit

**`MarkersTab_UI.cpp:295-296` still hardcodes a 3-entry Type-section loop** instead of the
ratified dynamic `DrawMarkerTypeSections` enumeration over live `markerTypeName` values
(`ARCH_19_13`/`§19.14`). This is already recorded as a Standing Recorded Defect in
`sangen_arch_pack/INDEX.md:237-250`, explicitly out of scope for every Link-chain ticket that
touched the surrounding code. It has no work order. Not blocking anything in §1's batch — flagged
here so it isn't lost, and so a future ticket-authoring session (UI Expert) picks it up.

Two other non-coder loose ends, neither blocking §1's batch:
- **`FORMAT_SPEC_UPDATE_DRAFT_MapScenarioSpec.md`** — a Format Expert doc-sync draft against
  `MAP_SCENARIO_SPEC.md`/`INDEX.md`. Confirmed still genuinely unapplied. Docs-only, no `src/`
  impact, no coder needed — just an ARCH-Expert-writes-docs task whenever picked up.
- **`PHASE_A_ScenarioDataMigration_PandemoniumIsthmus.md`** — Rev.2, dated 2026-09-04, still
  current: a one-time hand-edit to the live `Pandemonium Isthmus` map files, not a coder ticket at
  all (no `src/` change). Awaiting manual application by the human.

## 4. Full accounting — every other cluster, verdict + evidence

*(Condensed from 5 independent research passes, each of which read every ticket in full against
the live tree before rendering a verdict. Full per-file citations live in this session's research;
summarized here to keep this document a usable index rather than a second audit trail.)*

### 4.1 Area / canvas / mask cluster — ALL SHIPPED
`STEP204` (naval retirement), `STEP210` (area canvas gesture — shipped, then legitimately
rewritten in place by `STEP212`/`STEP219`; treat `STEP210`'s own file as historical, its code
blocks show the pre-rewrite shape), `STEP216`–`STEP220` (tier-gated uploads, 16-color palette, GPU
compose benchmark, real per-frame recomposite + watchdog, imported-mask version-hash fix) are all
confirmed live in `src/`, tests included.

### 4.2 Selection / marquee cluster — ALL SHIPPED
`STEP227` (Z-order + size-sort), `STEP228` (overlay panel gate), `STEP230` (marquee Ctrl-toggle /
Shift-union), `STEP232`/`STEP233` (shift-range anchor + canvas/list selection-set sync — both
landed together in commit `752e282`), `STEP234`/`STEP235`/`STEP236` (global Delete key, +Group/
+Layer move-selection, `DrawDialCompact`) all confirmed live with registered tests. The last three
shipped bundled inside the large MarkerLink-correction commit (`8235f03`) without their STEP
numbers in the commit *message* — only as in-code `// STEPnnn` comments — which is why a
message-only grep misses them; always verify against code.

### 4.3 MarkerLink chain (STEP237–249) — ALL SHIPPED, fully self-consistent
All 13 tickets landed in one commit (`8235f03`, "Correct Markers-Tab Link mechanic"), which
created the ARCH files (`ARCH_19_28`–`33`, `ARCH_21_09`), the work orders, and the implementation
together — the "original" and "correction" rounds were never separately committed. Every
`depends on STEPnnn (done)` annotation inside these tickets checks out against real code. No
mismatches, no unbuilt pieces, no ticket left dangling. `ARCH_19_29`'s retracted sentence is
correctly struck through with a pointer to `§19.33`.

### 4.4 Scenario / Armies cluster — see §1 (the only cluster with real remaining work)
The four ARCH draft files (`ARCH_AMENDMENT_DRAFT_ScenarioSpawnId.md`,
`ARCH_CORRECTION_DRAFT_ScenarioSpawnIdCaseInsensitivity.md`,
`ARCH_AMENDMENT_DRAFT_ScenarioSlotRangeCondition.md`, `DESIGN_ScenarioSlotRangeCondition_R1.md`)
are all ratified and folded into `ARCH_15_05`/`ARCH_15_12`/`ARCH_15_13` — historical records now,
correctly self-marked. One correction: the case-insensitivity draft's own proposed replacement
text (uppercase-fold) is itself wrong; the actually-ratified `§15.12` text (lowercase `tolower`) is
correct, and `STEP252`'s ticket text already matches the correct ratified version, not the draft's
error — no action needed, just don't resurrect the draft's text as if it were current.

### 4.5 Not yet ratified — excluded from this batch entirely
- **Assembly** (`BRIEF_Assembly_R1.md` / `DESIGN_Assembly_R1.md`) — still genuinely design-phase.
  `sangen_arch_pack/INDEX.md` confirms no dedicated Assembly ARCH section has been ratified;
  Assembly's groundwork (rigid-rotate math, "no members list" rule, `assemblyIdentifier` scalar)
  was absorbed piecemeal into the separately-ratified `ARCH_19_MarkerLayerBundle.md` track, but
  the Assembly feature itself has zero dispatchable ticket. Not part of this plan.
- **`DESIGN_ScenarioGeneratorDeclarativeInputs_R1.md`** — Rev.2, dated 2026-09-07 (the newest
  design doc in the repo). No ticket has been derived from it yet. Watch for ratification; not
  yet actionable.

### 4.6 Confirmed historical / superseded (no action needed beyond what §3 already covers)
`CONSOLIDATION_MASTER.md`, `IMPLEMENTATION_STATUS.md`, this document's own pre-2026-09-08 content,
`SESSION_HANDOFF_2/3/4.md`, `SESSION_HANDOFF_ImportExport.md`, `TABREBUILD.md`,
`TAB_REBUILD_PLAN.md`, `HANDOFF_TRACK_ArmyMirror.md`, `HANDOFF_TRACK_MarkerLayerSymmetry.md`,
`HANDOFF_TRACK_PreviewCompositing.md`, `HANDOFF_TRACK_PreviewOverlayLayering.md`,
`HANDOFF_TRACK_ScenarioScripting.md`, `SEQUENCE_PreviewOverlayLayering.md`, `PARITY_BACKLOG.md`,
`RECIPE_PARITY_BACKLOG.md`, `IO_PARITY_REPORT.md`, `GAP_MarkerLayerAndSymmetry_PARAMS.md`,
`B2_ParityFields.md`, `DESIGN_MarkerGroupLayerRestructure_R1.md` +
`BRIEF_MarkerGroupLayerRestructure_R1.md`, `DESIGN_MarkerLayerSymmetry_R1.md` (self-superseded by
R2), `DESIGN_MarkerLayerSymmetry_R2.md`, `DESIGN_MarkerPreviewLayering_R1.md` (self-superseded by
R2) + `DESIGN_MarkerPreviewLayering_R2.md`, `DESIGN_MarkerTypeSectionsAndInstanceSelection_R1.md` +
`BRIEF_MarkerTypeSectionsAndInstanceSelection_R1.md`, `BRIEF_MarkersTabUI_R2.md`,
`BRIEF_MarkersUICorrectionRound2_R1.md` + `DESIGN_MarkersUICorrectionRound2_R1.md`,
`DESIGN_MapScenarioIO_R1.md`, `DESIGN_ScenariosTabAndLuaEditor_R1.md`,
`DESIGN_SantpFootprintIngestion_R1.md` (its proposed STEP85–92/94/96/97 all independently confirmed
shipped by commit cross-reference during this audit), `REFERENCE_UnitSpawning_VerifiedRecipe.md`
(already correctly self-headered SUPERSEDED), and all three `BUGFIX_*.md` files
(`OverlayVisibilityAndPropIconFallback_R1` → commit `a73ad87`, `SlopeTabUICorrection_R1` → commit
`d9654cd`, `UniversalCoordinateConversionAndDragRewrite_UI` → commit `18019b7`, each shipped).
`BRIEF_ScenarioScriptingRatification.md` is the one exception in this list that stays **CURRENT**
by explicit prior ruling (`CONSOLIDATION_MASTER.md` H8) — it remains the scenario track's source
document, not superseded.

`BRIEF_OptimizedPreviewPipeline.md` and `BRIEF_MarkersTabUI.md` were missing the SUPERSEDED header
`CONSOLIDATION_MASTER.md` ruled they should get back in 2026-08-21 (ruling never executed) — fixed
in place during this audit. `SPEC-1_PropFormatCorrections_DOCS.md` and
`SPEC-4_SanmapSchemaV3_DOCS.md` both had stale "corrections NOT yet applied" status lines despite
both sets of corrections being verified live in `sangen_arch_pack/specs/` — also fixed in place.

## 5. Method

5 independent research agents, each read every ticket in its cluster in full, then read every
file/line/type each ticket cited against the current `src/` tree (HEAD, clean working tree) —
never trusting a ticket's own status line or a commit message's silence, matching the standard
`IMPLEMENTATION_STATUS.md` set in the prior audit. Cross-referenced against `sangen_arch_pack/`
for every cited ARCH section. Peer sessions were notified before this document was written; one
confirmed no conflicting in-flight work on `work_orders/`.
