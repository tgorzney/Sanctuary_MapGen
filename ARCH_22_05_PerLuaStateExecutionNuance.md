[← ARCH index](ARCH.md) · [§22 ARCH_22_NavmapModifierBlockers](ARCH_22_NavmapModifierBlockers.md) · SanGen ARCH §22.5. **Only the ARCH Expert writes this file.**

### 22.5 Per-Lua-state execution nuance — distinct from `MAP_UNIT_SPAWNING_SPEC` §2's hazard

**Recorded as load-bearing debugging law — confirmed misdiagnosed live, twice, by two different
Claude sessions, before being correctly re-derived.** Full detail: `NAVMAP_MODIFIER_BLOCKER_SPEC.md`
§5.

**Ruled: this is a genuinely different phenomenon from `MAP_UNIT_SPAWNING_SPEC.md` §2's
double-execution hazard, on a different axis, and must not be conflated with it.** §2 documents
`<map>_data.lua` executing twice **within one host Lua state** because two host-side `Import()`
callers spell the same path differently (a cache-miss re-execution). This section documents a
separate axis — **Lua state count**: confirmed by direct read of `script.lua`, `IsHost`/`IsClient`
are mutually exclusive **within** one Lua state, but a solo/listen-server match runs the per-map
`_data.lua` chunk in **two separate Lua states** (`InitLobby` calls `LoadMapData` once under `if
IsHost` and again in the `else` branch) — so the whole chunk, including any blocker-spawning
function in its shared `NewThread`, executes **once per state**, and each state's own single call
correctly takes the branch appropriate to its own role.

**Ruled diagnostic guidance:** an apparent "runs twice" observation for blocker-spawning code in a
solo/listen-server test should first be checked against this per-state explanation before assuming
§2's cache-miss hazard applies — the two are independent and can co-occur, but are not the same
bug and do not share the same fix.
