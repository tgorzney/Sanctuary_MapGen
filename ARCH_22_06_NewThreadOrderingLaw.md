[← ARCH index](ARCH.md) · [§22 ARCH_22_NavmapModifierBlockers](ARCH_22_NavmapModifierBlockers.md) · SanGen ARCH §22.6. **Only the ARCH Expert writes this file.**

### 22.6 Ordering law inside the shared `NewThread` — extends, does not replace, existing law

Navmap-blocker work shares the **same single `NewThread`** `MAP_UNIT_SPAWNING_SPEC.md` §4 and
`MAP_SCENARIO_SPEC.md` §3.1 already govern (only one `NewThread` per script is honored). This
ruling adds two ordering constraints on top of that existing law, each paid for by a real
regression this session. Full detail and the concrete resolved order:
`NAVMAP_MODIFIER_BLOCKER_SPEC.md` §6.

1. **Must run after both host-side `SetPlayableArea` calls** (the throwaway nudge and the real
   area) — never before; a prefab instantiated against the throwaway rectangle is a plausible
   culling target. This one is a correctness constraint, not an error-propagation one — `pcall`
   (below) does not substitute for it.
2. **Should run early relative to any other diagnostic/experimental code added later** — before
   anything not yet proven safe, because `NewThread` callback errors are silently swallowed and
   `Log`/`Warn` do not function in this build, so an earlier throw cancels everything after it with
   zero trace.

**Ruled: the concrete order that satisfies both** — `(a)` host-only `SetPlayableArea` ×2, `(b)`
the blocker-spawn call (runs in both Lua states, §22.5), `(c)` any other diagnostic/experimental
work, `(d)` host-only scenario unit spawning.

⚠️ **This corrects the relative position implied by `MAP_UNIT_SPAWNING_SPEC.md` §4's own
illustrative snippet and `MAP_SCENARIO_SPEC.md` §3.1's documented live order**, both of which show
prefab/navmesh work running **last**, after unit spawning. `NAVMAP_MODIFIER_BLOCKER_SPEC.md` §6
records this explicitly as this session's current, reported state of Pandemonium Isthmus's own
`_data.lua` — not independently re-read line-by-line by the ARCH Expert this pass — and is
cross-referenced from both of those sections so neither is silently trusted as still-current by a
future reader.

**Ruled, binding — ordering alone is not sufficient, and got proven insufficient live
(`NAVMAP_MODIFIER_BLOCKER_SPEC.md` §6.1):** every blocker-spawning call in `(b)`/`(c)`/`(d)` that
can throw must be independently `pcall`'d, not merely placed in a safe position. Ordering only
reduces the *chance* something upstream fails first; `pcall` removes the *consequence* if the
blocker call itself throws for any reason — including a bug unrelated to ordering. On Pandemonium
Isthmus, multiple rounds of live re-ordering each fixed a real ordering problem without fixing the
actual failure, because the real cause was a scripted table-rewrite that silently deleted an
adjacent `Import("common/navmapModifiers.lua")` line, leaving `NavmapModifiers` a nil global — an
`Import`-omission bug, not an ordering bug. **Once every risky call is individually `pcall`-wrapped,
the only ordering constraint from this subsection that remains genuinely load-bearing is point 1
above** (playable area must be final before instantiation) — that one governs which world-state the
prefab sees, not error propagation, so `pcall` cannot fix it. Full failure-mode writeup and the
"ordering checks out is not sufficient evidence" caution: `NAVMAP_MODIFIER_BLOCKER_SPEC.md` §6.1.
