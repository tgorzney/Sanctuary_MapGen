# Branching Event Tree — Dev Brief

**What we want:** a map event can, when it fires, arm a set of *child* events —
each with its own trigger (a delay, an area, a stat threshold, whatever) —
and each child can arm its own children, to arbitrary depth. Each branch's
timing is relative to when its parent fired ("its own timeline"), not the
match clock. Example: `raiders_arrive` fires → arms `raiders_camp_built`
(30s later) → which arms `raiders_second_wave` (AreaEnter, 60s window) →
which arms two more branches, etc.

**What this brief is:** how this falls almost entirely out of machinery
Sanctuary's Lua event system already has (per `Sanctuary_Map_System_Rework.md`
§8.2–8.3), one new field to add to the existing declarative event schema, a
TGUE-inspired addition for replication/debugging, and the open questions
devs need to settle before building. No code is being changed by this
document — it's a proposal.

---

## 1. What already exists — don't rebuild it

| Capability | Where | Notes |
|---|---|---|
| Declarative event array (`id`, `enabled`, `once`, `on`, `run`) | `map.json` `events` channel, rework doc §8.3 | Pure data. Gated once at POPULATE; disabled = never instantiated, costs nothing. |
| Event bus (pub/sub, dense int ids, no `pairs`, no per-publish alloc) | rework doc §8.2(a) | Existing sources: `UnitDestroyed`, `ArmyStatChanged`, `VolumeEnter`, etc. |
| Timer wheel (`ctx.After(delay, fn)`) | rework doc §8.2(b) | Replaces coroutine-per-timer. ~5ns/tick idle regardless of timer count. |
| Area triggers = colliders | rework doc §8.2(c) | Zero cost until a unit crosses the boundary — engine does the broadphase. |
| Army-stat / distance watchers | rework doc §8.2(d)/(e) | Incremental, not polling. |
| Action scripts (`scripts/events/<name>.lua`, `Actions.<Name>(ctx, args)`) | rework doc §8.3 | Only lua that executes, only at RUNTIME. |
| Seeded, deterministic RNG for map-time random choices | rework doc §7 (shatter placement) | `SeededRng(hash(mapId, options, matchId))`, host rolls, client never rolls. |
| Validator: reference closure, no dependency cycles | rework doc §7.5 item 2 | Already checks "every event script exists... no dependency cycles" — extend, don't replace. |

**The core mechanism (event bus + timer wheel + colliders, all O(1) idle) already supports cascading.** An action function can already call `ctx.After(...)` or subscribe to something new. What's missing is a *declarative* way to express a whole cascade as map data, so SanGen (or a hand-authoring map maker) can define one without writing bespoke Lua control flow per branch.

---

## 2. The root problem (not the algorithm)

The actual need isn't "a tree data structure." It's: **author a cascade of
consequences, where each consequence has its own trigger and its own
relative clock, without hand-writing nested callbacks per map, and without
adding runtime cost for branches that never fire.**

That's a general shape — raid escalation, disaster cascades, scripted
mission chains — not a one-off. It should be one small, reused mechanism.

---

## 3. Proposed shape: one new field, not a new subsystem

Add `parent` to the existing event schema (rework doc §8.3):

```lua
events = {
  { id = "raiders_arrive",
    enabled = { option = "north_pass_ambush" },
    once    = true,
    on      = { type = "AreaEnter", area = "NorthPass",
                filter = { armies = "players", tags = "Tags.MOBILE - Tags.SCOUT", minCount = 3 } },
    run     = { action = "SpawnAmbush", args = { group = "North_Raiders", army = "Neutral_Raiders" } } },

  -- NEW: a child. Same schema, one new key.
  { id      = "raiders_camp_built",
    parent  = "raiders_arrive",              -- <- does not arm until the parent fires
    once    = true,
    on      = { type = "Timer", delay = 300 },   -- 300 ticks AFTER the parent fired, not match start
    run     = { action = "SpawnCamp", args = { group = "North_Raiders_Camp" } } },

  { id      = "raiders_second_wave",
    parent  = "raiders_camp_built",
    once    = true,
    on      = { type = "AreaEnter", area = "NorthPass",
                filter = { armies = "players", minCount = 1 } },
    run     = { action = "SpawnAmbush", args = { group = "North_Raiders_Wave2", army = "Neutral_Raiders" } } },
}
```

**Two independent gates, both must be true before a node is instantiated (subscribed/timer-armed/collider-created):**

1. **Config-enabled** — existing mechanism, unchanged.
2. **Parent-fired** — new. A node with a `parent` does nothing at POPULATE except register on a `NodeFired` internal bus channel, keyed by parent id. When the parent's `run.action` completes, it publishes `NodeFired(parent_id)`; every waiting child then runs its *own* POPULATE-time instantiation step (arm its timer / subscribe its area / etc.) at that moment, not at match start.

**This is why "own timeline" falls out for free:** a child's `Timer` delay is counted from the tick it was armed, and it's armed at the tick its parent fired — not the tick the map started. No new clock, no per-branch tick loop. Same mechanism as today's `ctx.After(1, fn)` used inside an action (rework doc §8.3's `raiders.lua` example), just moved into the declarative layer so it doesn't have to be hand-written per branch.

**Root has no `parent` key at all** — same as every event today. A tree is just: events with no parent are roots; events with a parent are children; you can have as many trees as you want in one map, and unrelated trees are unrelated data (no shared root object required).

---

## 4. Why flat, not nested

Do **not** represent this as nested Lua tables (`children = { {...}, {...} }`).
Two independent reasons:

1. **§6.4(d) already flags nested-table depth as a real risk** — a deep
   nested structure can blow the C stack on `table.merged`/`table.deepCopy`/
   `json.encode`, all of which recurse. A flat array with a `parent` id
   string sidesteps this entirely — it's the same shape as today's `events`
   array, just with one more field, and every existing traversal over
   `events` (validator, POPULATE loop) stays a flat `ipairs`, not a
   recursive walk.
2. **Arbitrary depth costs nothing extra either way** — an unarmed 50-deep
   chain is 50 idle table entries until node 1 fires. Depth is not a
   performance question, it's a *representation* question, and flat is the
   one that doesn't risk a stack overflow on a big cascade.

This also matches the DOD pattern TGUE uses for keyframed/dependent chains
(`Timeline_PROC` / `DataStreamSequencer_PROC` — parent-child transforms
sorted and processed as flat arrays, never recursive OOP — see
`TGUE_Reference/`). Different engine, same reason: flat + parent-index beats
nested + recursion for anything that can get deep.

---

## 5. TGUE-inspired addition: a fire ledger, for replication and debugging

This part is a genuine borrow from TGUE's `TGUE_EventStream` (`TGUE_Reference/TGUE_EVENTSTREAM_SPEC.md`) — its "Delta Array" is an event-sourcing ledger of tiny records (`[Time, Target, ActionID, Payload]`) instead of snapshots, replayable to reconstruct state at any point ("Time Scrubbing").

Sanctuary doesn't need full time-scrubbing (Phase 5's CRC/hash desync check
already covers "did we desync"), but the **same shape solves two real
problems for this feature specifically:**

**(a) Replication.** Rework doc §7.2 already settled "host resolves, client
obeys" for config — the same principle applies here. A branching cascade
involves engine-broadphase timing (`VolumeEnter`) and RNG (branch
selection, §6 below), both of which the client must not re-derive itself.
So: every time a node fires, the host publishes one small record —

```lua
{ tick = now, nodeId = "raiders_camp_built", instanceId = 1 }
```

broadcast the same way the shatter brief's `ShatterFired` message works
(rework doc §7, "host decides, host broadcasts, client only renders"). The
client never evaluates triggers for cascade nodes at all — it just plays
`run.action` locally when told a node fired. This is strictly less client
work than today's `AreaEnter` handling, and it removes an entire class of
host/client cascade-desync bugs before they can exist.

**(b) Debug / QA replay.** The ledger is also just a match log: replaying
it against the static tree definition (the `events` array) tells you
exactly which branch path a given match took, without re-simulating
anything. Useful for "why did the second wave never fire" bug reports —
this is the direct, practical version of TGUE's Time Scrubbing idea, sized
to what this feature actually needs rather than a full state-reconstruction
system.

This ledger is small — one entry per node *fire*, not per tick, and most
nodes fire zero or one time per match.

---

## 6. Optional: stochastic branch selection

A node can have more than one child. Nothing above requires only one child
per parent — the event bus fans out to every subscriber for
`NodeFired(parent_id)`, same as any other bus event.

If map authors want *variety* (not every match plays every branch), gate a
child's arming on a probability roll using the map's existing seeded RNG
(rework doc §7 — `SeededRng(hash(mapId, options, matchId))`, host rolls, client
never rolls, deterministic by construction). This is the same
"lossy/stochastic alternative" pattern TGUE's expert format calls out
separately from the accurate solution (`TGUE_Reference/feature_request_event_sourced_state.md`
and neighboring docs) — full determinism stays the default; a weighted roll
is an opt-in per node, not a new subsystem.

---

## 7. Validation — extends §7.5, doesn't replace it

Add to the existing author-time hard-fail checklist (rework doc §7.5 item 2,
"reference closure... no dependency cycles"):

- `parent` id must exist in the same `events` array.
- No cycles — a node cannot be its own ancestor. (Same check class already
  required for the flat array; a parent pointer is just one more edge to
  walk.)
- A node with `once = false` and children: needs an explicit decision (see
  Open Question 1 below) before this can even be validated — the rule
  differs depending on the answer.

---

## 8. Open questions for devs to settle

**1. Can a parent fire more than once?** If `once = false`, does each fire
spawn an independent branch instance (multiple concurrent `raiders_second_wave`
timers in flight), or does re-firing the parent do nothing if a branch is
already live? If instances can be concurrent, the fire ledger needs an
`instanceId`, not just a `nodeId` (already sketched above, but the semantics
need deciding, not assuming).

**2. Cancellation.** If a branch's precondition stops making sense mid-match
(e.g. the target area gets disabled by a later config layer, or the spawning
army is defeated), do already-armed children need explicit teardown
(unsubscribe / disarm timer / delete collider)? Or is "let it fire into a
no-op" acceptable? Rework doc §8.1's phase boundaries don't cover this case
because it's a RUNTIME-only concern — CONFIGURE/POPULATE are already over by
then.

**3. Author-time vs runtime tree growth.** Is the tree closed at CONFIGURE
(entirely static data, fully known before the match starts — what this brief
assumes throughout), or can an action script add *new* nodes at RUNTIME
(e.g. "spawn a random 3-level cascade")? This is the single biggest fork in
the design:
   - **Static tree:** SanGen can visualize, lint, and validate the whole
     tree at author time. Depth/cycle checks are exhaustive. This is what
     §3–§7 above assume.
   - **Dynamic tree:** far more flexible for authors, but the fire ledger
     becomes load-bearing for *understanding* a match (nothing is knowable
     ahead of time), and author-time validation can only check the
     generator code, not its output.

   Recommend starting **static-only** (matches CONFIGURE's existing "pure,
   no world access" contract, rework doc §8.1) and revisiting dynamic growth
   as a v2 if a real use case needs it.

**4. Depth/breadth sanity limit.** Structurally unbounded (§4), but infinite
in practice is a footgun for map authors, not the engine. Recommend a
soft author-time lint warning (e.g. "this chain is 40 nodes deep — are you
sure?") rather than a hard cap.

---

## 9. Summary

| # | Item | Status |
|---|---|---|
| 1 | `parent` field on the existing `events` schema | Proposed — one field, no new subsystem |
| 2 | Parent-fired gate via existing event bus (`NodeFired` channel) | Proposed |
| 3 | Relative timing via existing timer wheel (`ctx.After`) | Already exists — just needs to be armed at the right tick |
| 4 | Flat array, not nested tree | Proposed — avoids §6.4(d)'s stack-depth risk |
| 5 | Fire ledger (host broadcasts, client obeys) | Proposed, TGUE-inspired — closes a replication-desync class |
| 6 | Stochastic branch selection | Optional, reuses existing seeded RNG |
| 7 | Validator: cycle check on `parent` | Extends existing §7.5 rule, same check class |
| 8 | Four open questions (§8) | **Needs a decision before implementation scoping** |

**Engine changes required: none.** Everything above is Lua + one new data
field, built entirely on primitives the rework doc already established.

---

## Appendix: where the ledger/timeline idea came from

Copied into `Map Generator/TGUE_Reference/` for reference (source: TGUE
project, `D:\Projects\UE5 Projects\TGUE\sys_docs\`):

- `TGUE_EVENTSTREAM_SPEC.md` — Delta Array (event-sourcing ledger) + Time
  Scrubbing (replay-to-microsecond). Source for §5's fire ledger.
- `TGUE_PLUGIN_API_SPEC.md` — "Event Sourcing Input": external code mutates
  state only by appending to the stream, never by direct write. Same
  principle as "client never evaluates cascade triggers, only plays back
  what the host published" in §5(a).
- `feature_request_event_sourced_state.md` — event-driven engine callbacks
  instead of polling. Validates that Sanctuary's existing `VolumeEnter`/
  event-bus approach (rework doc §8.2) is already the right shape; nothing
  to change there.
- `feature_request_temporal_decoupling.md` — localized sub-stepping /
  per-region relative clocks. Conceptual parent of §3's "child timing is
  relative to when its parent fired," even though the concrete mechanism
  (timer wheel with a later arm-tick) is simpler than TGUE's version because
  Sanctuary doesn't need a separate sub-tick rate.
- `TGUE_MASTER_FLOW.md` — not directly used here; included for context on
  how TGUE seals/replicates state per frame, which is the same *shape* of
  problem as §5(a)'s replication concern, solved differently per engine.

This brief is Sanctuary/Lua-specific and does not port any TGUE code or
API — only the underlying ideas (event-sourced ledger, relative/local
timing, event-driven-not-polled triggers), adapted to primitives that
already exist in `engine/LJ/lua`.
