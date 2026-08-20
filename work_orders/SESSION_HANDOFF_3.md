# Session Handoff 3 — Import Robustness, Round-Trip Fidelity, and V2→V3 Migrations

*Written to let a fresh conversation pick up with zero prior context. Repo:
`D:\Projects\Sanctuary\Map Generator`. Read `CLAUDE.md` first — it's the always-loaded router.
This doc is a snapshot; if anything here conflicts with the actual files, the files win.
Supersedes `work_orders/SESSION_HANDOFF_2.md` (the session before this one).*

## 1. What this project is
SanGen v2 — a from-scratch rebuild of a Sanctuary: Shattered Sun map generator, governed by
`ARCH.md` + `sangen_arch_pack/CONSTITUTION.md` + specs reached through `sangen_arch_pack/
INDEX.md`. Work flows: a domain expert (read-only) proposes → the ARCH Expert (sole writer of
`ARCH.md`/`sangen_arch_pack/`) ratifies → the SanGen Coder implements. No agent commits to git;
the human does. See `CLAUDE.md` for the full expert roster.

## 2. What this session did

**Opened with STEP23** (Radial N-fold symmetry PROC implementation, `AppendRadialTurns`) —
shipped, verified, and **already committed** (`be44408`).

**Then pivoted to a much larger initiative, entirely from the human's own bug report**: opening a
real `.sanmap` file (`Pandemonium Isthmus.sanmap`, a real Supreme Commander demo file) got refused
outright. That single bug report grew into 18 more tickets (STEP24–STEP42), all shipped,
independently verified, and **all still uncommitted** as of this handoff.

### The arc
1. **Never-refuse import** (`STEP24`) — the importer used to hard-refuse any `.sanmap` with no
   version marker or a newer-than-supported one. This required amending `CONSTITUTION.md` §6 and
   `IO_MIGRATION_SPEC.md` §3/§6 (the human ratified this directly, in their own separate ARCH
   Expert conversation — the ARCH Expert refused to let the orchestrating session apply it
   directly, insisting on the human's own words). New law: absent/old/newer version markers are
   never grounds to refuse; unrecognized top-level data is preserved under one reserved
   `UnknownImport` key (nested, not merged flat — `STEP28` corrected an initial flat
   implementation) so re-export never silently drops data this build doesn't understand.
2. **Round-trip completeness fixes** (`STEP25`, `STEP27`, `STEP30`, `STEP37`) — map name/credits,
   water settings, `terrainMaxHeight` full precision, `Stratum.ImportedMaskMode`/`.Enabled`,
   `Water.deepWaterDepthMinimum`, and `Stratum.appearance.{name,environmentName,materialName}` all
   now round-trip; previously several were write-only (exported but never read back) or entirely
   unwired.
3. **Legacy `mapGeneratorData` blob retired from export** (`STEP36`) — once every live field in it
   had a real top-level home (`STEP30`/`STEP11`/`STEP27`), the human authorized deleting the
   EXPORT side entirely. The IMPORT side's gated legacy readers are deliberately UNCHANGED — real
   old files (`World_Domination.sanmap`, `Pandemonium Isthmus.sanmap`, confirmed to exist on disk)
   still need them.
4. **File-architecture cleanup** (`STEP29`, `STEP31`–`STEP35`, `STEP42`) — 6 files that grew past
   ARCH §1.5's 150-line ceiling during the above work got split into proper orchestrators +
   domain-specific files. This was explicitly requested by the human mid-session ("this should be
   an orchestrator... give me a plan... check with all experts") and is now fully closed.
5. **Small correctness fixes found along the way**: `STEP38` (a warning message that became
   misleading once `mapGeneratorData` stopped being written), `STEP39` (moved blueprint-path
   validation from a UI-button-only gate into the exporter itself, since the human is planning
   more export entry points), `STEP41` (two real gaps `STEP40F` surfaced: geometry validation
   guards that stopped running once the legacy blob was gone, and a migration that padded output
   even with zero source data).
6. **The actual V2→V3 migration chain** (`STEP40A`–`STEP40F`, 6 tickets) — the session's biggest
   finding: **zero migration files had ever been built**, despite the mechanism shipping back in
   `STEP6` (a prior session). `kCurrentSanGenVersion` was stuck at 2. This is now fully wired: 9
   real migration functions, a new `ConvertColorArrayToRgbaObject` primitive, the
   already-ratified-but-never-built `MigrationEntry` manifest struct, and the version bump to 3.
   **Four originally-considered migrations were explicitly SKIPPED, not deferred** — the human
   confirmed no real file with an actual `SanGenVersion: 2` stamp has ever existed (this project
   jumped from the v2 mechanism to the v3 schema before ever shipping a build in between), so the
   placement-rule-stack migrations (Markers/Props/Decals/UnitsStack) and a global-scalar broadcast
   would have been pure guessing with no real data to validate against. The heightmap/`GeoLayers`
   migration was also skipped for a related reason — the one real legacy sample found uses a
   completely different, incompatible noise-generation model than the current system; that's a
   design question, not a migration.

### Every ticket has its own `work_orders/STEPn_*.md` file — read the specific one before
touching adjacent code. They are detailed, current, and were each independently verified (full
solo rebuild + full `ctest` run, not just trusting the coder's own report) before being reported
done.

## 3. Current state — READ THIS FIRST
- **All of STEP24–STEP42 is uncommitted.** `git status --short` shows ~34 modified files, ~30
  new files, and this handoff plus every `STEPn` work-order doc as untracked.
- **Full build is clean, full test suite is green: 94/94 tests pass**, independently verified via
  a solo `cmake -S . -B build` regenerate + `MSBuild ALL_BUILD.vcxproj` + `ctest -C Debug` (not a
  concurrent coder's own claim) as of the end of this session.
- `sangen_arch_pack/CONSTITUTION.md` and `sangen_arch_pack/specs/IO_MIGRATION_SPEC.md` are both
  amended (two ratification rounds) — read their current text directly, don't trust old cached
  knowledge of them.
- The human's most recent commit (`be44408`, STEP23 only) has an accidentally-mangled commit
  message (the literal `git add`/`git commit` shell command text landed as the message instead of
  the intended one) — the human was told and said to leave it, diff is fine.
- **That commit is also not pushed** — `git status -sb` showed `ahead 1` of `origin/SanGen-v2` as
  of this session; confirm current push state before assuming otherwise.

## 4. What's left / open items
- **Nothing is committed from STEP24 onward.** The human has not yet been given a commit plan for
  this batch (unlike STEP23, which they committed themselves, if imperfectly). This is probably
  the single most important next step — a huge amount of verified, working code is sitting only
  on disk.
- **`Pandemonium Isthmus.sanmap` has still never actually been imported** through the fixed
  pipeline — the human said "wait" early on and it was never revisited. Given `STEP24`+`STEP36`+
  the full migration chain are all done now, this real-file smoke test is genuinely ready to run.
- **`STEP26`** (the UI-layer "preview what migrating would find, let a human selectively apply"
  reconciliation dialog) was designed in detail (IO Architecture Expert + UI Expert consults, full
  interaction design) but never built — it was explicitly gated on real migrations existing to
  preview, which is no longer true as of `STEP40F`. If the human wants it, the design work is
  already done; it just needs a work-order written from the existing consult notes in this
  conversation's history (not preserved in any file — if this context is gone, redesign from
  scratch or ask the human what they remember wanting).
- **Rotation coordinate-flip** (Props/Decals/Markers/Armies quaternions are never flipped, unlike
  position, which does get the coordinate flip) — the human explicitly ruled: do NOT flip it;
  they'll verify empirically by placing units in the map preview, then playing the map, and
  comparing facing direction. No code change wanted until that verification happens.
- **Minor, low-priority, human already told**: `Flow_Migrate_V2`/`GlobalMarkerSettings_Migrate_V2`
  carry comments suggesting `bIndependentlySelectable = true` is warranted, but the wired manifest
  (`STEP40F`) keeps them `false`, matching the earlier design ruling. Not a bug, just a stale
  comment; no action requested.
- **A confirmed, not-yet-ticketed gap**: `Sanmap_KnownTopLevelKeys_IO.cpp`'s allowlist correctly
  prevents known-writer fields from being mis-bagged into `UnknownImport`, but nested/recursive
  unknown-field capture (an unrecognized field INSIDE a known top-level key, e.g. inside
  `mapGeneratorData` itself) was explicitly ruled out of scope in `STEP24` and never revisited.
- **A third, unrelated `Stratums[]` JSON dialect** was discovered mid-research (real file
  `World_Domination.sanmap` uses lowerCamelCase keys matching neither of the two dialects
  `SANMAP_FORMAT_SPEC` documents) — flagged, never fixed, no ticket written. `ReadStrataSettingsJson`
  doesn't recognize it at all.

## 5. Hygiene notes worth carrying forward
- **This session dispatched many coder agents in parallel against the same shared `build/`
  directory.** This caused real, repeated symptoms: transient "Not Run" test results, a `.pdb`
  lock contention error, one coder's edit to a shared file being found reverted mid-session (later
  self-corrected). None of these caused actual data loss, but every parallel-coder claim of "full
  build passed" needs independent re-verification once ALL parallel work in a batch has landed —
  this was done every time this session and caught nothing new, but don't skip it.
- **Before any independent verification build, run `cmake -S . -B build` first** — several coders
  added new source files and `CMakeLists.txt` entries; a stale CMake cache silently misses them.
- **The established pipeline, followed for all 19 tickets**: research directly against real
  code/specs → draft a work-order (`work_orders/STEPn_*.md`) → dispatch the relevant domain
  expert(s) for a read-only design consult BEFORE writing the ticket (this caught real errors
  multiple times — wrong field names, a wrong group-theory buffer-size calculation, a
  misunderstood JSON key nesting level) → dispatch the SanGen Coder with a prompt restating every
  binding ruling → **verify the coder's own report against real files and a fresh independent
  build+test yourself** before telling the human it's done (this also caught real problems
  multiple times, including two cases where a coder found and fixed a bug the work-order itself
  didn't anticipate, and correctly flagged the deviation rather than silently applying it).
- **User feedback on file (see auto-memory)**: keep chat replies short and concrete; no internal
  narration or process talk in responses (state conclusions, don't narrate the path to them); mark
  questions with `❓` and real issues/blockers with `⚠️` so they're visually unmistakable; never
  manually test the UI — verify via automated test binaries and code reading only.
- **A parallel/concurrent session pattern recurred from the prior handoff too** — always check
  `git log`/`work_orders/*.md`/`git status` for surprises before assuming a clean starting state.

## 6. Recommended next step
Give the human a commit plan for STEP24–STEP42 (large; likely needs splitting into a few logical
commits given how entangled some files are across tickets — this was already discussed once this
session, see the conversation history if still available, or just propose fresh). Once committed
and pushed, the two live open threads are: actually importing `Pandemonium Isthmus.sanmap` as a
real-world smoke test, and deciding whether to build `STEP26`'s already-designed reconciliation
dialog now that real migrations exist for it to preview.
