[← ARCH index](ARCH.md) · [§15 ARCH_15_MapScenarioSystem](ARCH_15_MapScenarioSystem.md) · SanGen ARCH §15.10. **Only the ARCH Expert writes this file.**

### 15.10 Slot-pattern construction moves into the runtime; `maxArmySlotCount` becomes authored data (ratifies the human's construction-code-belongs-in-universal-mod-code decision; amends `MAP_SCENARIO_SPEC.md` §2/§3/§4)

**The problem this closes.** The live reference held three independent hardcoded numbers with
nothing cross-checking them: 4 authored armies (`ARMY_ID_TO_NAME`/`KNOWN_ALLOY_MARKERS`),
`BuildSlotPattern`'s `for i = 1, 16` in `_data.lua`, and `ApplyScenario`'s own
`for armyId = 1, 4` occupancy-deletion loop in `_Scenarios_Script.lua`. Worse, `BuildSlotPattern`
lived in the one file SanGen is forbidden to write (`_data.lua`, §15.4 point 1) — a
SanGen-authored slot-pattern length disagreeing with a hand-kept literal there makes TIER 1
(§15's exact `slotPattern` string-equality match, `MAP_SCENARIO_SPEC.md` §4) silently never
match: a length mismatch with zero diagnostic.

**Decision (ratified 2026-08-21, human's own framing):** slot-pattern construction is universal,
never-varies-per-map algorithm — exactly the same kind of content
`FindMatchingScenario`/`Scenario.ResolveAndApply` already are (§15.4) — so it moves out of the
hand-authored orchestrator and into `<MapName>_Scenarios_Runtime.lua`, alongside them.
*(Corrected 2026-08-28: this bullet originally also cited `Scenario.SpawnNavalFleets` as a peer
example — that function no longer exists in the live reference; see `ARCH_15_05`'s "RETIRED
2026-08-28" note. The argument itself — slot-pattern construction belongs beside the other
generic, per-map-invariant runtime functions — is unaffected by that function's removal.)*

**1. New module API contract** (amends `MAP_SCENARIO_SPEC.md` §3, and its §2.2 migration steps):
- `Scenario.ResolveAndApply(total, humanCount, aiCount, playersInformation)` — the fourth
  argument changes from a pre-built `slotPattern` string to the raw lobby array
  (`Engine.GetLobbyInformation().playersInformation`, unmodified). `total`/`humanCount`/
  `aiCount` are unchanged — still derived by the orchestrator itself, on demand, from that
  same array (§15's own no-separate-stored-shape discipline; only slot-pattern derivation
  moves).
- `Scenario.ResolveAndApply` builds `slotPattern` internally, via a new runtime-local
  `BuildSlotPattern(playersInformation, MAX_ARMY_SLOT_COUNT)` — the algorithm ports verbatim
  from the reference `_data.lua`, with its hardcoded `16` replaced by the rendered
  `MAX_ARMY_SLOT_COUNT` global (point 2 below).
- **Migration hand-edit, once per existing map** (`_data.lua`, not SanGen-written — human
  action, `MAP_SCENARIO_SPEC.md` §2.2):
  a. Retarget `Import()` from the legacy `_Scenarios_Script.lua` to `_Scenarios_Runtime.lua`.
  b. Change the call site from
     `Scenario.ResolveAndApply(total, humanCount, aiCount, slotPattern)` to
     `Scenario.ResolveAndApply(total, humanCount, aiCount, playersInformation)`.
  c. Delete the orchestrator's own `BuildSlotPattern` function and its call
     (`local slotPattern = BuildSlotPattern(playersInformation)`) — dead code once the
     runtime owns the algorithm.

**2. `maxArmySlotCount` — new authored field on `Params::Scenarios`** (amends §15.5's shape):
```cpp
struct Scenarios {
    std::vector<PatternScenario> patternScenarios;   // TIER 1 — pattern, §4
    std::vector<CountScenario>   countScenarios;      // TIER 2 — ORDER IS LOAD-BEARING, §15.6
    ScenarioBody                 defaultScenario;     // TIER 3 — always matches, exactly one
    int                          maxArmySlotCount = 16;  // NEW — governs BuildSlotPattern's
                                                         // slot count / slotPattern string
                                                         // length, rendered MAX_ARMY_SLOT_COUNT
};
```
- **Top-level, map-wide — a sibling of `patternScenarios`/`countScenarios`/`defaultScenario`,
  not per-scenario.** A single `slotPattern` format governs every `PatternScenario` entry in
  the same map (TIER 1 exact-match requires every authored pattern to share one length); there
  is no per-scenario notion of "how many army slots this map has."
- **Default: 16, matching the live reference.** Legal range: integer ≥ 1, and — see point 4
  below — never silently allowed to be less than the authored army roster driving the occupancy
  deletion loop (point 3). **No fixed upper bound is imposed by this ruling.** `maxArmySlotCount`
  is deliberately the map's own real ceiling, not the current lobby UI's exposed limit (reportedly
  8 today) — the UI limit is not guaranteed permanent, and a smaller ceiling than the true
  engine limit would silently truncate valid slot occupancy off the end of the pattern
  (`MAP_SCENARIO_SPEC.md` §4 already states this framing; this ruling is the field that makes it
  real authored data instead of a hardcoded literal).
- **Rendered as the Lua global `MAX_ARMY_SLOT_COUNT`** inside `<MapName>_Scenarios_Data.lua`,
  alongside `PATTERN_SCENARIOS`/`COUNT_SCENARIOS`/`DEFAULT_SCENARIO` (§15.4 point 2's render
  step; exact Lua syntax is IO-layer/coder-tier, unchanged from that ruling).
- **`maxArmySlotCount` < authored army count is a loud, logged, non-blocking export-time
  validation warning — never a silent truncation and never an auto-clamp.** (Constitution's
  "loud, logged" validate-input posture, applied here.) A slot count smaller than the map's own
  authored army roster means at least one authored army can never appear in any `slotPattern`
  the runtime builds (`BuildSlotPattern` only marks `player.armyID <= MAX_ARMY_SLOT_COUNT`) —
  real, silent gameplay breakage, not a cosmetic mismatch. The exporter must warn by name (which
  army/armies exceed the configured slot count) and still complete the export — this is
  authoring guidance the human can act on, not a destructive-write case like §15.4's overwrite
  refusal, so it does not block. The exporter must never invent a larger `maxArmySlotCount` on
  the author's behalf; silently overriding authored data is exactly the failure mode Constitution
  §6 forbids in the other direction (never fabricate data the human didn't author).

**3. The third hardcode — `ApplyScenario`'s occupancy-branch `for armyId = 1, 4` loop.** This
bound is **not** `maxArmySlotCount` (point 2 is deliberately allowed to exceed the authored
army count, §4 — reusing it here would index the per-map alloy-marker roster
(`ARMY_ID_TO_NAME`/`KNOWN_ALLOY_MARKERS`) out of bounds, and `ipairs(nil)` on a missing
`KNOWN_ALLOY_MARKERS[armyName]` entry is a hard Lua runtime error, not a harmless no-op). It
derives from **the authored army roster itself** — the same per-map data already driving
`ARMY_ID_TO_NAME`/`KNOWN_ALLOY_MARKERS`'s rendering (ultimately `MapRecipe::armies`, §9). Ruled
fix, ported into `<MapName>_Scenarios_Runtime.lua` alongside `ApplyScenario`'s move: iterate the
roster table directly — `for armyId, armyName in pairs(ARMY_ID_TO_NAME) do` — rather than a
hardcoded numeric range. This removes the third hardcoded literal outright instead of replacing
it with a fourth authored number; no new `Params::Scenarios` field is needed for it.
- ⚠️ **Scope note:** `KNOWN_ALLOY_MARKERS`/`ARMY_ID_TO_NAME`'s own per-map rendering shape (how
  the alloy-marker-name-per-army roster becomes Lua data) is not otherwise ratified anywhere in
  this pack today — it is IO/Format-Expert-owned rendering detail (same render step as point 2),
  out of scope for this ruling beyond the loop-construct fix above.

**4. Follow-on Format correction.** `SANMAP_FORMAT_SPEC.md` Correction 17 (the `Scenarios`
`.sanmap` section, §15.7) gains the `MaxArmySlotCount` key — **the Format Expert's call**, per
§15.7's existing three-way split; not specified here beyond naming that it is needed.

**5. Downstream work-order deltas (list only — not edited here):**
- `work_orders/STEP69_ParamsScenariosRoundTrip_IO.md` — transcribes `Params::Scenarios`
  (currently `patternScenarios`/`countScenarios`/`defaultScenario` only); gains
  `maxArmySlotCount` (default 16) in both the struct transcription and its round-trip
  default-value coverage (its existing "absent `Scenarios` key" defaults enumeration).
- `work_orders/STEP70_ScenarioScriptDataLua_IO.md` — the `Params::Scenarios` → Lua-text
  renderer; must render the new field as the `MAX_ARMY_SLOT_COUNT` global (point 2 above) and
  emit the point-2 export-time validation warning when `maxArmySlotCount` < the authored army
  count. It explicitly excludes `_Scenarios_Runtime.lua`'s own bundled content — the
  `BuildSlotPattern` port (point 1) and the `ApplyScenario` loop fix (point 3) belong wherever
  that bundled runtime resource's content is authored/maintained (WO6's runtime-resource
  ticket, not this step).
- `work_orders/DESIGN_ScenariosTabAndLuaEditor_R1.md` §1/§3 — its `ScenarioSettings` struct
  predates this ratification and places `maxArmySlotCount` (default 8, range 1-16) on a UI-side
  type, decoupled from `recipe.armies.size()` (its own open question 7). Superseded: the field
  lives on `Params::Scenarios` (point 2 above), default **16** (not 8 — matches the live
  reference, not the lobby UI's current exposed limit), and its "decoupled from
  `recipe.armies.size()`" open question is now answered — decoupled in the permissive direction
  (may exceed it) but coupled in the safety direction (point 2's export-time warning when
  below it).

