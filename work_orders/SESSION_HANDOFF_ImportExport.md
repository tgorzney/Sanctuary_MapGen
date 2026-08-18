# Session Handoff — Import/Export & PARAMS Completion

*Written to let a fresh conversation pick up with zero prior context. Repo:
`D:\Projects\Sanctuary\Map Generator`. Read `CLAUDE.md` first — it's the always-loaded
router. This doc is a snapshot; if anything here conflicts with the actual files, the
files win.*

## 1. What this project is

SanGen v2 — a from-scratch rebuild of a Sanctuary: Shattered Sun map generator, governed
by a single authoritative architecture (`ARCH.md` + `sangen_arch_pack/CONSTITUTION.md` +
specs reached through `sangen_arch_pack/INDEX.md`). Work flows through a fixed pipeline:
a domain expert (read-only) proposes → the **ARCH Expert** (sole writer of `ARCH.md`/
`sangen_arch_pack/`) ratifies → the **SanGen Coder** implements. No agent commits to git;
the human does. Nine expert subagents exist in `.claude/agents/*.md` — see `CLAUDE.md`'s
"Experts" section for the full roster and when to use each.

**Ground truth for the `.sanmap` format**: `D:\Projects\Sanctuary\Sanmap File Format\`
(`SanMap.cs`, `SanMap.Types.cs`, `Types.cs`) — the `EM.Map` namespace. **Do NOT trust**
`Sanctuary-Map-Generation-develop\src\` (the `ExtraneousMapGen` namespace) — confirmed
unrelated, non-authoritative old tooling from a different project. The real game/engine
install is at `E:\Games\Steam\steamapps\common\Sanctuary Shattered Sun Demo\`.

## 2. What this whole effort has been about

Replacing v1's `mapGeneratorData` blob (a single JSON key holding SanGen's entire
generator state, ~60% duplicate of the format's own fields) with independently-versioned,
top-level, format-sibling sections — schema v3. Alongside that, closing the "recipe is
rules-only, format is instances-only" gap: giving manually-authored entity data (armies,
areas, markers, props, decals, chains) a real PARAMS home for the first time.

## 3. Fully ratified — ready for a coder work-order, nothing blocking

All of this is written into `ARCH.md` and `sangen_arch_pack/specs/`. None of it is
implemented in `src/` yet except where noted in §4.

| Domain | Spec | Notes |
|---|---|---|
| Schema v3 shape (`GeneralMapSettings`, `HeightmapStack`, `Symmetry`, `SlopeDefaults`, `Flow`, `Accumulation`, `MarkersStack`/`PropsStack`/`DecalsStack`/`UnitsStack`, `DetailNormal`) | `work_orders/SPEC-4_SanmapSchemaV3_DOCS.md`, `SANMAP_FORMAT_SPEC.md` | `PerformanceSettings` was ruled OUT of the file entirely — global app-settings only, no per-map storage |
| `SanGenVersion` + migration architecture | `sangen_arch_pack/specs/IO_MIGRATION_SPEC.md` | Runner/manifest/primitives law; no actual migrations needed yet (nothing to migrate *from*) |
| `Army`/`UnitGroup`/`UnitTransform`/`MapArea` (manually-placed armies/areas) | `ENTITY_AUTHORING_PARAMS_SPEC.md` (1st session) | |
| `Params::Atmosphere` (49 fields, 8 sub-structs) | `ATMOSPHERE_PARAMS_SPEC.md` | |
| `Params::GlobalMarkerSettings` | `SANMAP_FORMAT_SPEC.md` Correction 7 | |
| `StratumGenerationSettings` (soil physics + slope-gate override) + `stratumLayers` appearance export/import fix | `SANMAP_FORMAT_SPEC.md` Corrections 12/13 | Appearance importer doesn't exist at all yet — real gap, not just incomplete |
| `MarkerInstanceGroup`/`MarkerTransform`, `PropInstanceGroup`, `DecalInstanceGroup`, `MarkerChain`/`ChainMarker`, shared `InstancedTransform` base | `ENTITY_AUTHORING_PARAMS_SPEC.md` (3rd session) | The "no PARAMS home for baked instances" gap, closed |
| Props/decals manual layering (`layerIndex` field + `PropGroups`/`DecalGroups`) | `ARCH.md` §12, `ENTITY_AUTHORING_PARAMS_SPEC.md` (4th revision) | |
| Radial N-fold symmetry | `ARCH.md` §13, `SANMAP_FORMAT_SPEC.md` Correction 4 amendment | Default axis = Point, default blend = Superposition (once `SymAlgorithm` exists) |
| Naming law | `ARCH.md` §1.8 | Pass-through/authored data → format spelling; generative settings → SanGen's own names. Named exceptions documented inline. |
| Two shipping export bugs | Fixed in code, see §4 | `maskRemapMin/Max` → real `Vector4`; `height` → rounds to int |

## 4. Actually implemented in `src/` (small list — check `git log`/`git status` for current truth)

1. Step 1 shipping bug fixes (`work_orders/STEP1_ShippingBugFixes.md`) — `maskRemapMin/Max`
   widened, `height` rounds to int.
2. The full maskRemap saga: `Params::Stratum::maskRemapMinimum/Maximum` widened to `float[4]`,
   ruled as pure appearance pass-through (NOT a Mask-stage input — this overturned a prior
   ARCH ruling), Mask kernel's remap step removed entirely (6 files), `StratumsTab_Appearance_UI.cpp`
   updated to 8 independent per-channel rows. Full `SanGenV2` build confirmed clean, all
   tests passing, CPU/GPU parity verified.

**Everything else in §3 is designed and ratified but not wired into any `.cpp`/`.h` file.**

## 5. Known defects — flagged, not fixed, need their own coder work-order

1. **`DecalRule` has no symmetry override at all** — missing the `bSymmetryUseGlobal`/
   `symmetryMask` pair, AND `AppendDecalRules` never calls `ResolveSymmetryMask` — decals get
   **zero symmetry in the running code today**, not just a missing field. Recorded in
   `sangen_arch_pack/INDEX.md`'s "Standing recorded defects" list.
2. **Symmetry-clone buffer overflow risk** — `Params::symmetryOrbitMaximum = 16` backs a fixed
   `SymmetryOrbitPoint orbit[16]` stack array. A high radial count combined with mirrors can
   need 32+ slots; currently silently drops the excess rather than erroring.
3. **Global Symmetry tab UI is exclusive-choice**, can't combine axes, while the per-rule
   symmetry UI already correctly allows combinations. The tab's existing "Radial" checkbox
   is currently a stale stand-in mapped to `QuarterTurns`.
4. **`stratumLayers`' `maskRemapMin`/`Max` broadcast/collapse question is now moot** — it's a
   real 4-component field now, nothing to broadcast/collapse.
5. Pre-existing, not new this session: **rotation is unimplemented in the exporter** (identity
   quaternions written for every entity), and **props export is currently disabled** in the
   existing `MapExporter_*` code.

## 6. Explicitly deferred — needs real design before any coder work-order, not just wiring

1. **Heightfield symmetry PROC stage** — doesn't exist in v2 at all. Fully researched (v1's
   actual mechanism confirmed via code read: symmetrizes at noise-generation time, then
   re-symmetrizes per-layer immediately after each layer's full erosion pass — not per-step,
   not deferred to the end — plus an optional final seam-blur for the Blur algorithm; low-level
   erosion solvers have zero symmetry awareness, it's purely an orchestration-layer concern).
   v2 has decided NOTHING yet about how it will do this. Generator Expert + ARCH territory.
2. **`Flow`/`Accumulation` real simulation design** — the human flagged existing sim math as
   possibly wrong; deferred entirely. Can ship as empty reserved JSON keys for now (already
   decided), doesn't block IO work, but the real algorithm is unstarted.
3. **Props/decals `blueprintPath` validation mechanism** — mandatory before any export (a bad
   path aborts the rest of map load in-game per real evidence), but the actual validation
   mechanism was explicitly left to IO Architecture Expert, not yet designed.
4. **`Params::UnitRule::armyIndex`'s relationship to the new `recipe.armies` list** — left as
   an open integration question for whoever writes the entity-export work-order.
5. **Live-engine `armies[x].groups` consumption** — real finding that the live engine may not
   read this the way the editor format assumes (uses a separate `_data.lua` registry instead).
   Explicitly ruled out of scope: round-trip the official format faithfully regardless.

## 7. Hygiene / process notes worth carrying forward

- **8 stray git worktrees** (`.claude/worktrees/agent-*`) were found and removed this session
  — all confirmed stale/superseded via real diffing against `main` before deletion, backed up
  first to the scratchpad. If new ones appear, same protocol: back up, diff against `main`,
  only remove what's provably superseded.
- **Always verify agent self-reports before relaying them** — this session caught a case where
  an ARCH dispatch's "already applied" framing didn't match git history, and this file itself
  exists partly because a background dispatch died mid-write (verified clean before re-running,
  no corruption — but always check `git status`/`git diff` after any write-enabled dispatch,
  don't just trust the completion summary).
- **`CLAUDE.md` has a 3-step agent-pack consistency audit procedure** (added this session) —
  run it after any ratification that could make an agent charter stale.
- Two small `.claude/agents/*.md` staleness fixes were applied directly by the orchestrating
  session (not by the ARCH Expert — agent files are ordinary project config, not
  `sangen_arch_pack/`'s exclusive domain, but an agent still won't edit its own or a sibling's
  file from a relayed instruction — that stays a human/main-session action).

## 8. Recommended next step

Batch coder work-orders for everything in §3, in the dependency order already established
this session (schema-key integration mostly funnels through `MapExporter/MapImporter_Recipe_IO`
and `MapImporter_IO.cpp`, so most of it is sequential, not parallel — see the "parallel vs
sequential schedule" reasoning already done for the original 9-item list, extend it for the
newer ratifications). Start with whatever's most valuable to see working end-to-end first —
Armies+Areas or the Markers/Props/Decals/Chains family are the most structurally significant.
