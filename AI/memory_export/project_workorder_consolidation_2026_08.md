---
name: project-workorder-consolidation-2026-08
description: "State of the SanGen work-order consolidation as of 2026-08-22 — five sessions merged, full implementation audit done, citation sweep done."
metadata: 
  node_type: memory
  type: project
  originSessionId: 79343935-b349-47c3-92aa-7dd2aa8ae463
  modified: 2026-08-22T05:02:51.080Z
---

On 2026-08-21/22, five parallel sessions working different SanGen tracks (preview overlay
layering, marker layer-symmetry, scenario scripting, army-mirror/migration-dialog, and a
duplicate preview-compositing track) were consolidated into one session ahead of an eventual
work-order execution phase. The user's explicit process: **work-order authoring only, no code
execution, until a separate future go-ahead** — one exception was made for `STEP95` (a small,
fully-verified, isolated symmetry-mask bug fix) at the user's explicit request.

**Key artifacts, both in `work_orders/`:**
- `CONSOLIDATION_MASTER.md` — the index over everything found/decided/authored during
  consolidation: human rulings (army naming = always `ARMY_XX`, machine-owned, never
  human-settable; footprint data may influence generation only via a `PARAMS` bake, never a live
  `PROC` read; etc.), cross-track dependencies, and defects found while authoring.
- `IMPLEMENTATION_STATUS.md` — a full audit of every M0–M5 and STEP1–45 ticket (93 total) against
  real `src/` content, never trusting a ticket's own status line. Headline finding: **91/93 are
  already implemented, tested, and mostly committed** — only `STEP26A`/`STEP26B` are not, and
  that's deliberate (held per an earlier "do not execute" instruction). The real undone backlog is
  almost entirely `STEP46` and above. Also documents a follow-up citation sweep (78 files, 231
  stale `ARCH.md §N` citations rewritten to the post-restructuring `ARCH_NN_*.md §N` form) and the
  handful of genuine content-drift findings it surfaced.
- `STEP97_AlloySpawnsArmiesManualSubLayers_UI.md` — a correction ticket for a real conflict found
  between `STEP51`'s (unbuilt) design and current ARCH law (`ARCH_14_02_DataModel.md` §14.2). Has
  3 unbuilt prerequisites (STEP51, STEP60, STEP66) plus an open ARCH routing question.

**Status as of this memory's writing:** work-order authoring and a full accuracy audit are both
complete and independently re-verified (23 fresh, adversarial re-checks, zero new errors found
beyond one the assistant caught and corrected itself — see [[feedback_verify_own_synthesis_claims]]).
Next step is execution-order planning across the full ticket set, on the human's go-ahead, in a
future session. The five original consolidated sessions have not yet been deleted — human wants
to wait until all work orders are created *and* confirmed accurate first.

**Update 2026-08-22 — parallel-execution conflict map built.** `work_orders/EXECUTION_CONFLICT_MAP.md`
grouped the 43-ticket unbuilt backlog (`STEP26A`/`STEP26B` + `STEP46`–`STEP97`) into 6 parallel-safe
waves, using 8 parallel Explore agents for inventory extraction + 8 more for adversarial verification
of every same-file touch the tickets didn't cross-reference themselves. Found one real defect (not
just an ordering issue): `STEP75`'s current text instructs the coder to hand-set `army.name`, which
violates `STEP76`'s "name is machine-owned" ruling once STEP76 lands — needs a ticket amendment,
land STEP76 first. Also identified 4 structural "hotspot" files (`MapRecipe_PARAMS.h`, the
`AppendEntityDomainsJson`/`ParseEntityDomainsJson` assembly functions, `Application_UI.cpp`/`.h`,
`MapCanvas_UI.h`/`.cpp`) that nearly every domain ticket touches additively — a standing rule for
future dispatch (never two hotspot-touching tickets to two coders in the same wave without a
rebase step), not a one-time fix. STEP96/STEP97 remain not-yet-schedulable (blocked on unwritten
prerequisite tickets / an open ARCH ruling).
