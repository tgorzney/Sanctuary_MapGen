# Session Handoff 4 — Real-World Failure, New Workflow, Preview Scaling Priority

*Written to let a fresh conversation pick up with zero prior context. Repo:
`D:\Projects\Sanctuary\Map Generator`. Read `CLAUDE.md` first — it's the always-loaded router.
This doc is a snapshot; if anything here conflicts with the actual files, the files win.
Supersedes `work_orders/SESSION_HANDOFF_3.md`.*

## 1. ⚠️ Read this before trusting anything else in this repo's history

The previous session shipped STEP24–42 (19 tickets), reported "94/94 tests passing,
independently verified." The human then **ran the actual built app themselves** and it is
"a mess, nowhere near usable." They also tried importing a real file and **it did not import
correctly** (see §2). A green automated test suite is unit-level correctness, not proof the app
works. Do not report anything "done" or "fixed" on test-suite results alone — the human is now
the source of truth on whether a feature actually works, and prior "verified" claims in older
handoff docs should be re-checked, not assumed. See memory `project_realworld_verification_gap`.

## 2. ⚠️ Open bug — real-file import still broken

File: `E:\Games\Steam\steamapps\common\Sanctuary Shattered Sun Demo\engine\Sanctuary_Data\Maps\Pandemonium Isthmus\Pandemonium Isthmus.sanmap`

This is the exact file that originally motivated STEP24's "never-refuse import" work. Despite
STEP24 (never-refuse) and the full STEP40A–F V2→V3 migration chain shipping, importing this file
"did not import correctly" per the human's own test — no further detail captured yet. **First
task for the new session**: reproduce this with the real file, get a precise failure description
(crash? silent data loss? wrong values? refuses again?), then trace it with real code reading —
don't trust old assumptions about what STEP24/STEP40 fixed.

Ground truth for the `.sanmap` format if the importer's behavior is in question:
`D:\Projects\Sanctuary\Sanmap File Format\` (`SanMap.cs`, `SanMap.Types.cs`, `Types.cs`, the
`EM.Map` namespace) — **not** `Sanctuary-Map-Generation-develop\src\`, which is confirmed
unrelated old tooling. Real game install: `E:\Games\Steam\steamapps\common\Sanctuary Shattered
Sun Demo\`.

## 3. New workflow — the human wants tight, incremental control now

Explicit instruction: after a week of batched multi-ticket sessions producing an app that
doesn't actually work end-to-end, the human wants to go **piece by piece, action by action**
from here forward. Don't batch many tickets into one big unverified sweep — smaller steps, more
visible checkpoints, confirm real behavior before moving to the next piece.

## 4. Priority #1 — preview panel scaling bug

The map preview should **always scale proportionally to fill its window/panel**. Currently it
stays a fixed size regardless of window size. This is the first concrete task for the new
session. Relevant files to start from (not yet investigated this session):
`src/ui/MapCanvas_UI.h`, `src/ui/MapCanvas_UI.cpp`, `src/ui/MapCanvas_Draw_UI.cpp`,
`src/ui/MapCanvasView_UI.h`, `src/ui/PreviewComposite_UI.h`,
`src/ui/PreviewComposite_Kernel_UI.h`, `src/ui/PreviewComposite_Settings_UI.h`. Follow the
normal pipeline: research → work-order → domain expert consult (likely SanGen UI Expert, maybe
UI Optimization Expert if it's a performance-relevant resize path) → coder → verify.

## 5. Paused thread — army-mirror feature (not yet ticketed)

Human's ask: Army1/3/5 (odd, by position in `recipe.armies`) get duplicated onto Army2/4/6
(even), renamed, with every duplicated unit rotated 180° about map center (position +
orientation). One-time duplicate action, not a persistent link.

Two read-only domain-expert consults already ran and returned solid, detailed findings — reuse
these, don't re-research from scratch:

**Format Expert findings:**
- `Params::Geometry` (`src/params/Geometry_PARAMS.h:11,32`) has `mapSize` (cells) and
  `worldUnitsPerCell` (default 1.0). Two conventions for "world extent" coexist in shipped code
  and disagree when `worldUnitsPerCell != 1.0`: entity-position IO flip
  (`MapExporter_Armies_IO.cpp:21-26`, `MapImporter_Armies_IO.cpp:42-51`, same in Markers/Decals/
  Props IO) uses `mapSize` directly; scatter/UI world-extent (`TerrainTab_UI.cpp:56`,
  `Placement_Emit_PROC.cpp:60-61`) uses `mapSize * worldUnitsPerCell`. No existing "map center"
  helper for entity positions anywhere (grepped clean).
- **Unresolved question for ARCH**: should the new mirror's map-center be `mapSize / 2`
  (matches the actual entity-position IO convention — the ground truth for these exact fields)
  or `(mapSize * worldUnitsPerCell) / 2` (matches the scatter convention)? They only diverge
  when `worldUnitsPerCell != 1.0`.
- Quaternion composition is **not new math** — `QuaternionMultiply` and a half-turn-quaternion
  construction pattern already exist in `src/proc/Placement_Transform_PROC.h:31-38,74`. 180°
  about Y is the exact literal `(0, 1, 0, 0)` — hardcode it, don't compute via trig.

**UI Expert findings:**
- Per-army button (not one global button) inside `DrawArmySettings`
  (`src/ui/ArmiesTab_UI.cpp:78-105`), visible only when the selected army is even-0-indexed
  (odd-numbered: Army1/3/5) **and** a successor row exists.
- Pair generically by consecutive position (`sourceIndex`, `sourceIndex+1`), not hardcoded to
  3 pairs — matches the existing `DraggableList`'s arbitrary add/remove/reorder.
- Gate behind the existing `ConfirmDialog_UI.h` widget — this overwrites the target army's
  existing hand-placed units, so it's destructive and needs confirmation.
- New pure helper in `ArmiesTab_UI.h`, same style as `DropUnitRulesForRemovedArmy` — recurse
  `UnitGroup.groups` + `UnitGroup.units`.
- Do **not** call `previewDriver->NotifyParametersChanged()` — zero PIPELINE code reads
  `Army.groups` today (grepped clean), matches existing SCOPE NOTE 1's posture for Army's other
  fields.
- Reuse the existing `bArmiesMoved`-triggered `MakeNamesUnique` repair for the rename, not
  `NextArmyName`/`NextUniqueLabel` (wrong tool — that seeds brand-new rows, not renames).

**Still needed before writing the work-order**: an ARCH Expert ruling on (1) the map-center
formula question above, and (2) whether a UI-layer header (`ArmiesTab_UI.h`) is allowed to
`#include` a PROC header (`Placement_Transform_PROC.h`) to reuse `QuaternionMultiply`, or
whether that violates layer boundaries and the math needs duplicating/promoting to a shared
layer instead. This consult was queued but not yet run — do it before drafting the ticket.

Given §2's real-file-import bug is now more urgent, this thread should probably wait unless the
human says otherwise.

## 6. STEP26 reconciliation dialog

The human asked "real migrations exist?" — **yes**: STEP40A–F shipped 9 real V2→V3 migration
functions (`Accumulation_Migrate_V2_IO`, `DetailNormal_Migrate_V2_IO`,
`EntityCollections_Migrate_V2_IO`, `Flow_Migrate_V2_IO`, `GeneralMapSettings_Migrate_V2_IO`,
`GlobalMarkerSettings_Migrate_V2_IO`, `SlopeDefaults_Migrate_V2_IO`,
`StratumGenerationSettings_Migrate_V2_IO`, `Symmetry_Migrate_V2_IO`, all in `src/io/`),
`kCurrentSanGenVersion` bumped to 3, wired through `Sanmap_MigrationManifest_IO`/
`Sanmap_MigrationRunner_IO`. STEP26 (a UI dialog previewing what a migration would find/change,
letting the human selectively apply it) was designed in detail in a prior session but never
built — still the human's call whether to build it, and given §2, probably comes after the real
import bug is actually fixed and confirmed working on the real file.

## 7. Git/repo hygiene (resolved this session, carry forward)

- STEP24–42 is committed and pushed (`9f74991` + earlier). Nothing left uncommitted from that
  batch.
- A stray `build_step40c/` parallel-build directory got accidentally `git add`-ed into that
  commit (1,542 build-log files, none of the real binaries — those were already gitignored).
  Fixed: untracked from git, deleted from disk (freed 1.7GB), and `.gitignore` now has
  `build_*/` so any future ad-hoc build directory can't repeat this.
- Real `.git` folder is small (~3.6MB) — the accidental commit was never a history-bloat
  problem, just a working-tree one. No history rewrite was needed or done.
- Before any independent verification build, run `cmake -S . -B build` first (new source files
  need a fresh cache).

## 8. Response style — the human was explicit about this, follow it exactly

- Only output text the human actually needs to read. No internal narration, no "let me..."
  framing, no restating what a tool call did.
- Mark every question with `❓` and every problem/blocker/discrepancy with `⚠️` — never bury
  either inside a paragraph of status text. Plain unprefixed text for normal statements/results.
- Keep responses short and concise by default. Save real structure (tables, headers, multi-
  section breakdowns) for actual documents (work-order files, this handoff), not chat replies.
- Never personally launch/click through the app to test — automated test binaries + code
  reading only (see §1: this is *necessary* but demonstrably *not sufficient*, so don't oversell
  a green test run as proof something works).
