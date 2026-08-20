# Design Brief — Ratify the Scenario Scripting file structure

*For a dedicated conversation consulting the SanGen ARCH Expert and the SanGen Format Expert.
Read `CLAUDE.md` first. Previous attempts at this consult failed because the experts were given
no grounding in the source material — this brief exists to fix that. Read every source file
listed below before consulting either expert; do not summarize from this brief alone.*

## ⚠️ Scope question to resolve FIRST, before either expert consult
The source material below (`Map Format Debate/`) describes the **Sanctuary: Shattered Sun
game's own Lua scripting/modding system** (`engine/LJ/lua/`) — this is the GAME ENGINE's
codebase, not SanGen's own C++ map-generator tool. SanGen's ARCH governs SanGen's own source
tree (`src/`), not the game engine's Lua runtime. So the first thing to settle, and state
explicitly to the ARCH Expert:

**Is this in SanGen's own domain at all?** Two possibilities:
1. SanGen's own export pipeline (`src/io/MapExporter_*`) is expected to GENERATE or manage
   these Lua files as part of a map package export — in which case this genuinely is SanGen IO
   territory, and the ARCH/Format Expert ratification is real and load-bearing for SanGen's own
   code.
2. This is a game-engine-side initiative the human is tracking in this repo for reference/
   convenience only, and SanGen's own tool has no direct code that touches it — in which case
   "ratification" means recording it as reference law for future SanGen IO work to build toward,
   not something that changes any SanGen C++ file today.

Don't assume either way — ask the ARCH Expert to state which, and get the human's confirmation
if the expert can't tell from the ARCH alone.

## Source material — read all three before consulting anyone
1. **`Map Format Debate/Sanctuary_Map_System_Rework.md`** (1413 lines) — the full design. Most
   relevant sections for this specific ratification:
   - §3.1 "Layout" — proposed map package structure, including `scripts/map.lua` ("Optional
     per-map script. Fills the MapPopulate void.") and the note "Why the data tree, not
     `LJ/lua/maps/`" (the SAME file-location tension this ratification is about).
   - §6.3 "Layer order and conflict resolution" — `_data.lua` listed as "legacy overlay,
     `table.merged` as today — additive-only is fine HERE because nothing is beneath it" —
     i.e. `_data.lua` is being KEPT, not replaced, which matches the human's "orchestrator" framing.
   - §7.1 "The new host timeline" — step 6a (`_data.lua` merge, "kept for compat") and step 10a
     (`[NEW]` load `scripts/map.lua`; fire `OnResolved`) — this is the closest existing precedent
     to the human's `_data.lua` (orchestrator) + `Scenarios_Script.lua` (code) split.
   - §11 "Implementation plan" — the Tier 1 / Tier 2 / Tier 3 table the human calls "the tiered
     system." Tier 1 = ~60 lines, no engine change, live-bug fixes. Tier 2 = the one engine
     dependency (`gameOptions` field). Tier 3 = the full rework (resolve seam, manifest loader,
     anchor system, event bus, etc.). **The scenario-scripting file split this brief is about is
     NOT itself listed as a numbered Tier item** — it's a simplification/alternate-naming of
     Tier 3 item 10's "resolve seam" + the `scripts/map.lua` concept from §3.1, scoped down to
     just the file-structure piece. State this relationship explicitly to the ARCH Expert rather
     than letting it assume the ratification covers the whole Tier 3.
2. **`Map Format Debate/Sanctuary_PerMapScripting_DevBrief.md`** — the original, smaller proposal
   this grew from: a per-map script hook via `Import()`, confirming `_data.lua`'s real role
   (§2: "a companion `<mapDataName>_data.lua` file" read by `SpawnGroup`/`SpawnGroupUnit`).
3. **`session_findings_2026-08-17_unit_spawning.md`** — LIVE-VERIFIED findings about the real
   file today. Critical: **§1, "Pandemonium Isthmus has two `_data.lua` copies — only one is
   real."** `Sanctuary_Data\Maps\Pandemonium Isthmus\_data.lua` (next to the `.sanmap`) is a
   DECOY; the real one the engine loads is `LJ\lua\maps\Pandemonium Isthmus\_data.lua`. This is
   exactly the file-location trap the human's "devs will likely move `_data` to the map folder"
   comment is anticipating — read this section closely, it's the ground truth for where the real
   file lives today vs. where it's expected to move.
4. **`sangen_arch_pack/specs/MODDING_SCRIPTING_SPEC.md`** — SanGen's own existing spec for this
   area (marked "only partially read so far"). If this ratification is real SanGen law, it
   likely amends or extends this file, not a new standalone spec. Read its current "Map
   scripting (events)" section — it already documents the `_data.lua` dual-path trap and the
   `Import()` mechanism this ratification's `_data.lua` → `Scenarios_Script.lua` link would use.

## The exact ratification ask, verbatim from the human
> "The original `_data` lua file, should be an orchestrator main file, and we should have a
> separate `Scenarios_Script.lua` file where we will place all the scenario code and the data
> file will reference/link it. This file will go in same folder as `_data`, however, the devs
> will likely move the `_data` to the map folder, and the Scenario file would go with it."

Decompose that into the three concrete things to ratify:
1. **`_data.lua` becomes an orchestrator/main file only** — no scenario code lives in it
   directly anymore; it references/links out.
2. **A new sibling file, `Scenarios_Script.lua`**, holds all the actual scenario code (events,
   spawns — the content `session_findings_2026-08-17_unit_spawning.md` §5 shows already living
   inline in the real `_data.lua` today). The link mechanism is presumably `Import()`
   (`MODDING_SCRIPTING_SPEC.md`'s documented convention) — confirm this is right, don't assume.
3. **Colocation rule**: `Scenarios_Script.lua` starts beside `_data.lua` (today:
   `LJ/lua/maps/<MapName>/`). If/when `_data.lua` relocates to the map's asset folder
   (`Sanctuary_Data/Maps/<MapName>/` — the direction §3.1 of the rework doc points, and the
   direction the *decoy* copy `session_findings` found already sits in), `Scenarios_Script.lua`
   moves with it — the pair is never split across the two folders.

## What "ratify" and "formalize" mean for each expert
- **ARCH Expert**: rule on the scope question above first. If in-scope, ratify the file-structure
  law (the three points above) as real SanGen-relevant architecture — where this lives in
  `sangen_arch_pack/` (likely `MODDING_SCRIPTING_SPEC.md`), and whether SanGen's own IO layer
  needs a work-order to actually emit/manage these two files on export (a real code change) or
  whether this stays reference-only for now.
- **Format Expert**: formalize what the `.sanmap`/map-package format truth should say about this
  pair — naming (`Scenarios_Script.lua` exact filename), the reference/link mechanism, and
  **explicitly note the possible future file-location change** (both files potentially moving
  from `LJ/lua/maps/<MapName>/` to `Sanctuary_Data/Maps/<MapName>/`) as a known-pending, not-yet-
  final relocation — the format truth should be written to tolerate that move, not assume
  today's location is permanent.

## Response style (carry forward)
Terse, ❓ for questions, ⚠️ for problems, no narration. See `work_orders/SESSION_HANDOFF_4.md`
§8 for the full house rule if more detail is needed.
