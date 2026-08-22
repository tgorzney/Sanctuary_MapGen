# Implementation Status — Full Audit, 2026-08-22

*Compiled from 11 parallel research batches (which further sub-delegated into ~35 agents) that
independently verified every M0–M5 and STEP1–45 work order against real `src/` content — never
trusting a ticket's own self-reported status line, since this project has a documented history of
those going stale. Read-only investigation; nothing built, nothing edited, nothing dispatched.
STEP46 onward is tracked separately in `CONSOLIDATION_MASTER.md` (that range is this repo's
active/recent work and its status was already established there).*

## Headline finding

**Every M0–M5 foundation ticket (42/42) and nearly every STEP1–45 ticket (49/51) is already
implemented, tested, and — where checked — committed.** The only two NOT implemented are
`STEP26A`/`STEP26B`, and that is deliberate: held per an explicit prior "do not execute"
instruction, not an oversight. This substantially changes the picture — the real backlog of
undone work is almost entirely in `STEP46` and above (this session's active tracks), not in the
foundation or the early IO/PARAMS/UI-wiring layer.

## How to read this table

- **IMPLEMENTED** — confirmed present in `src/`, matches spec, has real (non-trivial) test coverage.
- **IMPLEMENTED (STALE SELF-REPORT)** — implemented, but the ticket's own header claims otherwise.
- **IMPLEMENTED (SUPERSEDED WORDING)** — implemented; the ticket's *prose* describes an earlier
  design that a later ratification changed, but the ratified successor is what actually shipped.
- **PARTIAL** — real divergence between spec and shipped code, not just wording.
- **NOT IMPLEMENTED** — no evidence in `src/`.

---

## M0 — MATH + SYS foundation (9/9 IMPLEMENTED)

| Ticket | Verdict | Note |
|---|---|---|
| M0-1 Morton | IMPLEMENTED | `src/math/Morton_MATH.h`; old duplicates now forward to it |
| M0-2 Simd | IMPLEMENTED | AVX2 + scalar fallback, identical API |
| M0-3 Trigonometry | IMPLEMENTED | Cephes-style minimax, no libm call |
| M0-4 Reciprocal | IMPLEMENTED | 2-step Newton rsqrt as specified |
| M0-5 Spatial | IMPLEMENTED | Merged radial-clearance duplicate + fixed JFA (int seeds, ping-pong, +2 passes) |
| M0-6 ArenaAllocator | IMPLEMENTED | Move-only, overflow-safe |
| M0-7 ThreadPool | IMPLEMENTED | TSan-clean race fix present |
| M0-8 Dispatch | IMPLEMENTED | Exact 4-step `ResolveBackend` order |
| M0-9 GpuResource | **IMPLEMENTED (STALE SELF-REPORT)** | Header still says "NOT yet implemented" — actually built, tested against a real GL context, already extended by 2 later milestones (M5-0b, M5-5) |

## M1/M2 — Data model + pipeline skeleton (7/7 IMPLEMENTED, 1 superseded)

| Ticket | Verdict | Note |
|---|---|---|
| M1-1 DataCore | IMPLEMENTED | Clean |
| M1-2 LayerAndFields | **PARTIAL (real divergence)** | Ticket specifies one 9-mask family; ship code has two families (`materialProportions`+`surfaceStratumWeights`) plus a `slope` field, per a later ARCH §7.2 ruling — tests pass, but the ticket's literal spec isn't what ships |
| M1-3 LayerStack | IMPLEMENTED | Matches self-report exactly, incl. line counts |
| M1-4 PlacementRules | IMPLEMENTED | File sizes grew (later symmetry/mask-gate additions, superset not divergence) |
| M1-5 MapRecipe | IMPLEMENTED | Clean |
| M1-6 SanmapRoundtrip | **IMPLEMENTED (STALE SELF-REPORT)** | Header says "NOT yet implemented" — round-trip actually landed under STEP2–42, not under this ticket's own name |
| M2-1 GenerationPipeline | IMPLEMENTED | All 6 acceptance cases covered |

## M3 — PROC stages (8/8 IMPLEMENTED)

| Ticket | Verdict | Note |
|---|---|---|
| M3-1 NoiseBlend | **IMPLEMENTED (STALE SELF-REPORT)** | Header says "NOT implemented" — fully built, CPU/GPU parity tested |
| M3-2 Mask | IMPLEMENTED | Single-writer purity confirmed, idempotence tested bit-for-bit |
| M3-3 Erosion | IMPLEMENTED | Old non-atomic float-RMW race explicitly fixed (atomic scatter) |
| M3-4 Thermal | IMPLEMENTED | Historical hardcoded `/2.0` divisor confirmed retired, replaced by `relaxationRate` PARAM |
| M3-5 FlowAccumulation | IMPLEMENTED | Genuine priority-flood + backward topological sweep, not disguised relaxation |
| M3-6 Placement | IMPLEMENTED | Zero `rand()` anywhere; correctly reads `surfaceStratumWeights`, never the retired `materialMasks` |
| M3-7 Bake | **IMPLEMENTED (SUPERSEDED WORDING)** | Ticket text still names `materialMasks`/`maskRemap` as inputs; real code correctly consumes `surfaceStratumWeights` per the M3-8 rework — code is ahead of the ticket's prose |
| M3-8 PipelineIntegration | IMPLEMENTED | Stage order, DATA reshape, rival-remap deletion all confirmed |

## M4 — Preview/picking (7/7 IMPLEMENTED, 1 not yet wired into production)

| Ticket | Verdict | Note |
|---|---|---|
| M4-0a GradientRamp | IMPLEMENTED | Clean |
| M4-0b SpatialGrid | IMPLEMENTED | Confirmed single writer (Placement stage) |
| M4-1 EntityIdBuffer | IMPLEMENTED (still live) | STEP48 will retire its read path; not yet done |
| M4-2 GradientLut | IMPLEMENTED | Correctly lives in `src/ui/`, not `src/proc/` |
| M4-3 PreviewComposite | IMPLEMENTED (since extended) | STEP46 added a readback-gate param, additive |
| M4-4 Picking | **PARTIAL — real gap, already tracked** | `Picking_UI::PickMarker` (the spatial-grid replacement) exists and is tested, but production code (`MapCanvas_UI`, `Application_UI`) still calls the old `PickEntity`/`EntityIdBuffer` path. This is exactly what `STEP48` (already drafted) closes — not a new finding, confirms STEP48 is real and necessary |
| M4-5 PreviewIntegration | IMPLEMENTED | Two-tier dirty flags as specced |

## M5 — UI + assets (11/11 IMPLEMENTED)

| Ticket | Verdict |
|---|---|
| M5-0a WorldUnitsPerCell | IMPLEMENTED |
| M5-0b GpuResource Texture/Shader | IMPLEMENTED |
| M5-0c SlopeField | IMPLEMENTED |
| M5-0d UnifyCellSize | IMPLEMENTED |
| M5-1 CoreInputWidgets | IMPLEMENTED |
| M5-2 ListWidgets | IMPLEMENTED |
| M5-3 RichWidgets | IMPLEMENTED |
| M5-4 AssetPipeline | IMPLEMENTED |
| M5-5 MapCanvas | IMPLEMENTED |
| M5-6 Tabs | IMPLEMENTED |
| M5-7 AppShell | IMPLEMENTED — real `SanGenV3App` executable target confirmed |

## STEP1–9 (9/9 IMPLEMENTED)

All clean matches: shipping bug fixes, Armies/Areas IO, Markers/Chains IO, Props/Decals IO +
validation UI, migration subsystem, decal symmetry fix, global symmetry UI fix, Atmosphere PARAMS
+ IO. One item of note: **STEP6**'s core deliverable (a version-gated "refuse newer/unversioned
documents" law) was later explicitly reversed by **STEP24**'s opposite "never refuse" policy —
STEP6's own acceptance-test prose describing refusal no longer matches shipped behavior. Confirmed
intentional, tracked evolution (STEP24 exists precisely to make this change), not a defect.

## STEP10–19 (10/10 IMPLEMENTED)

SlopeDefaults mechanism, StratumAppearance/StratumGeneration IO, PlacementStacks IO,
GeneralMapSettings IO, HeightmapStack IO, SymmetryGlobalSettings IO, FlowAccumulation-Reserved IO,
DetailNormal IO, AppSettings IO. All confirmed with real round-trip and integration test coverage.
**STEP16**'s one internal ruling (a "dormancy" test proving `BuildSymmetryOrbit` does *not* branch
on Radial) was superseded by **STEP23**'s later, real Radial orbit implementation — forward
evolution, not a defect in STEP16's delivery.

## STEP20–29 (9/11 IMPLEMENTED; STEP26A/26B NOT IMPLEMENTED — deliberate)

Armies/Areas/Props-Decals Manual Layers UI wiring, Radial Symmetry Orbit PROC, Import-Never-Refuses,
MapName/Credits IO, Water Top-Level Import, Unknown Import Nesting, Exporter Recipe Split — all
confirmed implemented and tested. **STEP26A** (migration lossless-flag) and **STEP26B**
(reconciliation dialog, depends on 26A) are drafted, complete, but **not implemented** — this
matches the ArmyMirror track's own handoff record exactly: held per an earlier explicit
"do not execute" instruction, not forgotten.

## STEP30–39 (10/10 IMPLEMENTED)

All exporter/importer file splits (LegacyBlobFieldHoming, ExporterRecipeOrchestrator,
ExporterIOHeaderSplit, SymmetryOrbitSplit, ApplicationAssetBridgeSplit, ImporterParseDocumentSplit)
confirmed to have actually landed at the real target filenames the tickets specified — not just
"functionality exists somewhere." **STEP36**'s legacy-blob deletion was independently verified by
filesystem absence, not just grep. StratumAppearanceRoundtrip, MapGeneratorDataWarningWording, and
BlueprintValidationGate (later tightened by STEP39 from log-only to an actual refuse-unless-
acknowledged gate) all confirmed.

## STEP40–45 (11/11 IMPLEMENTED)

The full V2→V3 migration chain (STEP40A–F: foundation, trivial relocations, color-conversion,
stratum cross-domain, entity-collections, manifest wiring) confirmed against the real shipped
`Sanmap_MigrationManifest_IO.cpp` — cross-checked consistent with the ratified
`ARCH_17_MigrationValuesRegistry.md`. PostMigrationImportGaps and ImporterRecipeHeaderTrim
confirmed. **STEP43/44/45 (CompactOpenSanmapButton, PreviewWindowFitScaling, RenameExeToV3)
are implemented AND committed** — `git status` clean, tracked, part of commit `18c5154`.

**⚠️ Correction to something stated earlier in this conversation:** `HANDOFF_TRACK_ArmyMirror.md`
claimed STEP43/44/45 were "implemented, independently verified, uncommitted." The "uncommitted"
half is false and was relayed without independent verification earlier in this session. They are
already committed. Not a defect in the code — a stale claim in a handoff doc, now corrected here.

---

## Consolidated list of every real (non-cosmetic) finding from this audit

1. **M0-9, M1-6, M3-1** — self-reported "NOT implemented," actually implemented. Stale status
   lines only; no action needed on the code, but the headers should eventually be corrected so a
   future reader doesn't waste time re-verifying.
2. **M1-2** — genuine spec/shipped divergence (single mask family → two families + slope), driven
   by a later ratified ARCH ruling. Ship code is correct; the ticket text is outdated.
3. **M3-7** — ticket prose names stale pre-M3-8 inputs; shipped code is correct and ahead of the
   ticket's own text.
4. **STEP6** — acceptance-test prose contradicts shipped behavior because STEP24 deliberately
   reversed STEP6's refusal law. Intentional, documented evolution.
5. **STEP16** — one internal ruling superseded by STEP23's later real implementation. Not a defect.
6. **STEP26A/STEP26B** — the only genuinely unimplemented tickets in this entire 93-ticket sweep.
   Deliberately held.
7. **M4-4 / STEP48** — confirms a gap this session already knew about and already has a ticket for:
   `PickMarker` exists and is tested, but production UI code hasn't been switched onto it yet.
8. **STEP43/44/45** — a handoff doc's "uncommitted" claim is false; they're already committed.

## What this means for next steps

The foundation (M0–M5) and the early IO/PARAMS/UI-wiring layer (STEP1–45, minus the two
deliberately-held tickets) do not need further authoring, dispatch, or verification work — they
are done. The real remaining backlog is:
- **STEP26A/26B** — ready to dispatch whenever the human lifts the "do not execute" hold.
- **STEP46 and above** — this session's active tracks (preview overlay layering, marker
  layer-symmetry, scenario scripting, army-mirror, and the newly-authored STEP76–96), whose status
  is already tracked in `CONSOLIDATION_MASTER.md`. STEP46 and STEP95 are the only two of these
  confirmed implemented so far.

The ARCH-currency / "does every ticket cite correct information" audit the human separately asked
for should now focus on the STEP46+ range and the not-yet-implemented tickets — auditing the
citations of 91 already-shipped, already-tested tickets for wording staleness is low value; their
code is the ground truth now, not their prose.

---

## Addendum — citation & content-drift sweep, 2026-08-22

*Follow-up pass covering all 78 work-order files (M0–M5, STEP1–96, DESIGN/BRIEF docs) that still
cited ARCH sections in the pre-restructuring bare form (`ARCH §N` / `ARCH.md §N`) instead of the
current `ARCH_NN_*.md §N` filename form. `ARCH.md` itself guarantees section numbers are permanent
and never renumbered, so these were not factually broken — but each was re-verified against the
CURRENT content of its cited section, not just reformatted, specifically to catch cases where a
ticket's claim no longer matches a section that was amended after the ticket was written.*

**231 citations fixed across 78 files.** Constitution `§N` citations (a separate, never-split
document) were correctly left untouched throughout — verified file by file, not assumed.

### Real findings (not cosmetic)

1. **⚠️ STEP51 vs. current `ARCH_14_02_DataModel.md` §14.2 — RESOLVED, but with a correction to
   this document's own earlier claim.** This section originally called `STEP51_OverlayLayerDataModel_UI.md`
   "already shipped, confirmed IMPLEMENTED" — **that was wrong, asserted without verification, and
   is withdrawn.** `STEP51` is only ever named in the STEP46+ range, which the Phase 1 build-status
   audit above explicitly did NOT cover (see the file's own opening note). A later agent verified
   directly against `src/`: zero matches for `OverlayLayer_UI`/`SeedMarkerDomains` anywhere in the
   codebase, empty `git log --all` on every local branch. **STEP51 is DRAFTED, not built** — matching
   `SEQUENCE_PreviewOverlayLayering.md`'s own (correct) status, which contradicted this document.
   So the real situation is milder than first framed: STEP51's design (Alloy/SpawnsArmies get zero
   Manual sub-layers) is now superseded by `ARCH_14_02_DataModel.md` §14.2's later ruling
   (`Params::MarkerInstanceLayer` now exists; these domains should map to
   `recipe.markerLayers[i]`), but no shipped code is out of compliance — only an unbuilt ticket's
   design is stale. The human ruled current ARCH law wins. Correction ticket:
   `work_orders/STEP97_AlloySpawnsArmiesManualSubLayers_UI.md`, which found the fix has **three**
   unbuilt prerequisites, not one — `STEP51` itself, `STEP60` (`MarkerInstanceLayer`/
   `recipe.markerLayers`), and `STEP66` (`markerRules`→`markerRuleLayers`) all verified absent from
   `src/` — plus a genuine open ARCH question: `Params::MarkerInstanceLayer` (§16.1) carries no
   category field, so unlike Props/Decals there is no principled way for a manual layer entry to
   route to Alloy vs. SpawnsArmies specifically. Routed to the ARCH Expert rather than invented.
2. **M3-2 / M3-8 vs. `ARCH_07_02` §7.2 item 5.** Both tickets describe a per-stratum `Remap_s()` step
   happening once inside Mask. §7.2 item 5 has since explicitly withdrawn that claim — there is no
   remap step anywhere in generation; `surfaceStratumWeights[s]` is `Merge(...)`'s output unmodified.
   Shipped code is already correct (confirmed IMPLEMENTED, clean, earlier in this audit) — this is
   pure ticket-prose staleness with zero code impact, flagged in both files.
3. **M4-5 vs. `ARCH_06_RebuildOrder.md` §6.1.** Cites §6.1 for "a cheap visual tweak must not trigger
   a full regen" — §6.1 is actually a PROC-stage completion checklist and says nothing about this.
   The real citation should be `ARCH_14_08_DirtyFlagTiers.md` §14.8. Flagged, not corrected (a
   section-number change, not a filename fix, is outside this pass's authorized scope).
4. **`DESIGN_MapScenarioIO_R1.md` vs. §1.5.** Cites §1.5 for "naming law"; §1.5 is actually file-size
   ceilings. The real naming-law sections are §1.2/§1.7. Likely a wrong section number from the
   original author, not ARCH drift — same reasoning as #3, flagged not corrected.
5. **`DESIGN_MarkerPreviewLayering_R1.md`** — footnoted only, since the whole document is marked
   historical/superseded: its "constant-screen-size icons are correct" claim is superseded by the
   live `ARCH_14_03_IconRenderingLod.md`'s two-mode LOD ruling.
6. **`STEP52_IconAtlasPairingLookup_UI.md`** carries several `ARCH.md:1107-1135`-style line-number
   citations from the old monolithic file — a different citation form than this pass targeted, now
   meaningless since `ARCH.md` is a 139-line index with no ruling text at those lines. Not fixed;
   worth a small follow-up cleanup.

### Confirmed clean (no drift) across this entire 78-file sweep
The remaining ~225 fixed citations, spanning every M0–M5/STEP1–45/STEP46–96/DESIGN track, were each
individually checked against the live content of their target `ARCH_NN_*.md` file and found
consistent — including cross-track confirmations already known from earlier in this session (STEP50/
STEP51's `ruleIndex` flag now cites STEP79's CONFIRMED verdict by name; `ARCH_16_10`'s routing
summary already incorporates STEP79's own correction verbatim).
