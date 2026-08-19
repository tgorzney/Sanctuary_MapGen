# Session Handoff 2 — Import/Export + Schema-v3 + UI Wiring

*Written to let a fresh conversation pick up with zero prior context. Repo:
`D:\Projects\Sanctuary\Map Generator`. Read `CLAUDE.md` first — it's the always-loaded router.
This doc is a snapshot; if anything here conflicts with the actual files, the files win.
Supersedes `work_orders/SESSION_HANDOFF_ImportExport.md` (the session before this one) — that
file's own "recommended next step" is what this whole session executed.*

## 1. What this project is
SanGen v2 — a from-scratch rebuild of a Sanctuary: Shattered Sun map generator, governed by a
single authoritative architecture (`ARCH.md` + `sangen_arch_pack/CONSTITUTION.md` + specs reached
through `sangen_arch_pack/INDEX.md`). Work flows: a domain expert (read-only) proposes → the ARCH
Expert (sole writer of `ARCH.md`/`sangen_arch_pack/`) ratifies → the SanGen Coder implements. No
agent commits to git; the human does. See `CLAUDE.md` for the full expert roster.

**Ground truth for the `.sanmap` format**: `D:\Projects\Sanctuary\Sanmap File Format\` (`SanMap.cs`,
`SanMap.Types.cs`, `Types.cs`). **Do NOT trust** `Sanctuary-Map-Generation-develop\src\` (the
`ExtraneousMapGen` namespace) — confirmed unrelated old tooling. The real engine install is at
`E:\Games\Steam\steamapps\common\Sanctuary Shattered Sun Demo\`.

## 2. What this session did (22 work-orders, `STEP2` through `STEP22`)
Starting from the previous handoff's "nothing implemented except two shipping bug fixes," this
session shipped the full `.sanmap` entity/schema-v3 round-trip end to end:

**Entity domains (PARAMS + IO)**: Armies/Areas (`STEP2`), Markers/Chains (`STEP3`),
Props/Decals — built safe, deliberately not live-wired at first (`STEP4`) — then the
`blueprintPath` validation + confirm-dialog + live wiring that turned it on (`STEP5`).

**Migration foundation**: `SanGenVersion` + the migration subsystem (manifest/runner/
`JsonPrimitives_IO.h`), `kCurrentSanGenVersion = 2`, zero real migrations yet (`STEP6`).

**Known-defect fixes**: `DecalRule` symmetry (was silently getting zero symmetry, `STEP7`); the
Global Symmetry tab's exclusive-choice bug + stale "Radial" label (`STEP8`).

**Full schema-v3 cutover** (`SANMAP_FORMAT_SPEC.md` Corrections 2/3/4/6/7/8/9 — Correction 7 was
shipped by a **different, parallel session**, see §5): `GeneralMapSettings` (`STEP14` — renumbered
from a `STEP13` collision, see §5), `HeightmapStack` (`STEP15`), `Symmetry` global section +
`SymmetryAxis::Radial` reserved bit (`STEP16`), `Flow`/`Accumulation` reserved (`STEP17`),
`DetailNormal` (`STEP18`), global app-settings persistence outside `.sanmap` entirely (`STEP19`).
Also: `Params::Atmosphere` (49 fields, `STEP9`), `Params::SlopeDefaults` + `bSlopeUseGlobal`
mechanism (`STEP10`), the `stratumLayers` appearance export/import fix — a real shipping bug,
textures always exported blank (`STEP11`), `StratumGenerationSettings` (`STEP12`).

**UI wiring**: Armies (`STEP20`) and Areas (`STEP21`) tabs retyped off UI-only presentation types
onto the real PARAMS — found and fixed 3 real bugs in the process (wrong faction labels, a
reorder-desync bug, a name-collision data-loss bug). Props/Decals manual-layer grouping UI
(`STEP22`) — same pattern, built the Decals half from scratch since it never existed.

**Docs**: `SPEC-1_PropFormatCorrections_DOCS.md` applied (prop-pack ground truth corrections).
Two stale defect entries in `INDEX.md`/`PLACEMENT_SCATTER_SPEC.md`/`SANMAP_FORMAT_SPEC.md`
retired once their code landed.

Every ticket has its own `work_orders/STEPn_*.md` file with full rationale — read the specific one
before touching adjacent code, they're detailed and current.

## 3. The established pipeline (follow this exactly)
1. **Research directly** against real code/specs before drafting anything — this repo's specs
   sometimes lag the code or vice versa; verify, don't trust either blindly.
2. **Draft a work-order** (`work_orders/STEPn_Name.md`) — root problem, target files, exact shape,
   explicit out-of-scope, acceptance test. Check the next free `STEPn` number against
   `work_orders/*.md` AND recent `git log` — a parallel session can claim a number (see §5).
3. **Dispatch the relevant domain expert(s)** for review before the coder — ARCH Expert for
   PARAMS/naming/module-boundary conformance, Format Expert for wire-format ground-truth
   verification, IO Architecture Expert for file-split/migration conventions, Generator Expert for
   PROC/pipeline correctness (dirty-hash, stage-consumer questions), UI Expert for widget/tab
   design. **This step has repeatedly caught real bugs before they shipped** — do not skip it for
   anything non-trivial. Fold every finding back into the work-order file itself before dispatch.
4. **Dispatch the SanGen Coder** with a detailed prompt restating the work-order's binding rulings
   (agents don't re-read files unless told to) — always ask it to build AND run the relevant test
   binaries, and to build the FULL project, not just its own new files.
5. **Verify the coder's own report against real files yourself** (grep/read the actual diff) before
   telling the human it's done — this caught several coder self-report inaccuracies this session.
6. **Give the human a short commit-message draft** when asked; never commit or push yourself.

## 4. What's left
- **Hand-placed instance authoring UI** — genuinely undesigned, flagged explicitly and repeatedly
  this session, never attempted: individual `PropTransform`/`DecalTransform`/`MarkerTransform`/
  `Army.groups` rows have zero UI anywhere (place one, edit its position, delete it). This is a
  real new feature (canvas click? manual coordinate entry? a list-add button?) needing its own
  design consult before any work-order.
- **Radial N-fold symmetry — the actual PROC implementation.** `SymmetryAxis::Radial` and
  `radialSymmetryRepeatCount` are reserved (`STEP16`) but `AppendRadialTurns` (the N-way rotation
  orbit generator, generalizing `AppendQuarterTurns` from a hardcoded 3 turns to a designer count)
  does not exist. Building it also reactivates the dormant `symmetryOrbitMaximum = 16` buffer risk
  — check whether the fixed-size `orbit[16]` array needs widening once Radial can combine with
  mirrors (confirmed dormant/safe today only because nothing generates a Radial orbit yet).
- **`Params::SymAlgorithm`** — the exotic-blend enum (Fold/Blur/CrossFade/Superposition/
  Cylinder3D/Torus3D) is explicitly reserved-not-defined (`STEP16` ruling #1) — a future ticket's
  job, default to `Superposition` per ARCH §13 when it lands.
- **The heightfield-symmetry PROC stage itself** — doesn't exist at all; `Placement_Symmetry_PROC`
  mirrors placed ENTITIES, not the terrain field. Real generator-expert design work.
- **`UnitRule::armyIndex`'s design** — the reorder bug is fixed (`STEP20`), but whether positional
  indexing is even the right long-term mechanism (vs. name-based) was never re-litigated, just
  preserved as-is per the wire format's existing commitment.
- **`bUseGpuMarkers`** — has a settings-file home and seeds `placementStage`'s dispatch policy
  (`STEP19`) but no UI checkbox exists yet to actually let a designer toggle it.
- **`Params::GlobalMarkerSettings`** wiring into any UI tab — PARAMS+IO shape only shipped
  (parallel session's `STEP13`), no icon/color/scale picker exists.

## 5. Hygiene notes worth carrying forward
- **A parallel session ran concurrently with this one** and independently shipped
  `work_orders/STEP13_PlacementStacks_IO.md` (`MarkersStack`/`PropsStack`/`DecalsStack`/
  `UnitsStack` + `GlobalMarkerSettings`) and real ARCH.md ratifications (Radial N-fold symmetry
  design, ARCH §13) — discovered mid-session via a `STEP13` filename collision. **Always check
  `git log`/`work_orders/*.md` for what a parallel or prior session may have already shipped
  before assuming a task is unstarted** — this cost real research time to untangle once.
- **The `mapGeneratorData`-gate wiring-order bug is a recurring trap.** Multiple tickets this
  session initially drafted new top-level-key readers to run alongside the gated
  `mapGeneratorData`-derived readers in `MapImporter_IO.cpp`, when they needed to run
  UNCONDITIONALLY, BEFORE that gate (since the new keys are top-level document siblings, not
  nested in `mapGeneratorData`). Caught by review every time it recurred — always double-check
  new top-level-key wiring against this specific mistake.
- **"Relocate a field into a new section" tickets need explicit removal, not just addition** — the
  discipline every schema-v3 ticket followed: delete the old key from the legacy blob in the SAME
  diff as adding the new section, never leave both writing (confirmed via raw-JSON-text grep in
  every acceptance test, not just C++ call-site inspection).
- **Changing a PARAMS default value is higher-risk than it looks** — `STEP16`'s
  `globalSymmetryMask` default change broke 5 separate test fixtures across 5 different files; a
  full audit (not just the first 1-2 obviously-affected tests) was necessary and found 2 more
  breaks the initial pass missed.
- **8 stray git worktrees** were cleaned up in the PRIOR session (before this one) — if new ones
  appear under `.claude/worktrees/agent-*`, same protocol: back up, diff against `main`, only
  remove what's provably superseded.
- User feedback on file (see auto-memory): never launch/interact with the built app directly —
  verify via automated test binaries and code reading only; keep chat replies terse.

## 6. Recommended next step
Pick up with a design consult for hand-placed instance authoring UI (the biggest, most-flagged
remaining gap) OR the Radial N-fold PROC implementation (`AppendRadialTurns` + revisit the
`symmetryOrbitMaximum` buffer question) — both are real, undesigned features needing their own
UI-Expert/Generator-Expert consult before a work-order, same as every other non-trivial item this
session went through. Everything else in §4 is smaller, bounded follow-up work.
