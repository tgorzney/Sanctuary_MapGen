[← ARCH index](ARCH.md) · SanGen ARCH §15. Part of the ratified v2 architecture; the Constitution (`sangen_arch_pack/CONSTITUTION.md`) and this file's preamble in `ARCH.md` bind alongside it. **Only the ARCH Expert writes this file.**

## 15. The SanGen Map Scenario system — formalized as first-class law (ratifies `MAP_SCENARIO_SPEC.md`)

The game's per-army spawn position, alloy/mex marker visibility, and playable-area resolution —
for every player-count/composition a lobby can produce — is deterministically resolved once, at
map load, by the SanGen Map Scenario system: an `<MapName>_data.lua` orchestrator paired with an
`<MapName>_Scenarios_Script.lua` scenario module, both colocated in the engine's script tree
(`LJ/lua/maps/<MapName>/`), linked at runtime via
`Import("maps/<MapName>/<MapName>_Scenarios_Script.lua").Scenario`. **Status: DEPLOYED and
confirmed working live in-game (2026-08-20).** This ruling promotes that deployment to binding
ARCH law. Full contract — the two-file split, the module API (`ResolveAndApply`/
`SpawnNavalFleets`), the three-tier `PATTERN_SCENARIOS`/`COUNT_SCENARIOS`/`DEFAULT_SCENARIO`
matching system, the four `alloyMode` semantics (`explicit`/`occupancy`/`keepAll`/`delta`), the
§6 hard requirement that a scenario needing deterministic spawns must declare an explicit
`spawns` table, and the execution/timing law — lives entirely in `MAP_SCENARIO_SPEC.md`; it is
not re-derived here. **Note (§15.3 below): the two-file split named in this paragraph is the
ORIGINAL design — §15.3–§15.9 ratify a three-file successor. This paragraph is left as written
(the historical record of what was first promoted to law) rather than edited in place; §15.4 is
the current binding file-shape law.**

**Note (2026-08-28): the module API named in this paragraph (`SpawnNavalFleets`) is also stale —
same "historical record, left as written" treatment as the file-shape note above.** The live
reference script replaced it 2026-08-27 with a generic `spawnsUnits`-gated, name-keyed dispatch
(`Scenario.SpawnMatchedScenarioUnits`/`Scenario.SpawnUnits`); the naval-specific
`Params::ScenarioNavalFleet` family this pack shaped from the old `SpawnNavalFleets` body is
retired. Full correction, replacement design, and an explicitly flagged open gap (where
per-scenario unit-spawn dispatch code lives under the three-file split): `ARCH_15_05`'s "RETIRED
2026-08-28" and "OPEN" notes.


---

### Subsections of §15

| § | File | Ruling |
|---|---|---|
| §15.1 | [ARCH_15_01_LayerClassification.md](ARCH_15_01_LayerClassification.md) | Layer classification |
| §15.2 | [ARCH_15_02_IoScopeRuling.md](ARCH_15_02_IoScopeRuling.md) | IO scope ruling — corrects an earlier assumption, does not reverse it |
| §15.3 | [ARCH_15_03_ExportOnlyLuaRatified.md](ARCH_15_03_ExportOnlyLuaRatified.md) | Design ratified: option (c) — export-only, SanGen never parses Lua back (resolves §15.2's open question / `MAP_SCENARIO_SPEC.md` §8) |
| §15.4 | [ARCH_15_04_ThreeFileOnDiskShape.md](ARCH_15_04_ThreeFileOnDiskShape.md) | Three-file on-disk shape + overwrite safety (ratifies `MAP_SCENARIO_SPEC.md` §2/§2.1/§2.2) |
| §15.5 | [ARCH_15_05_ParamsScenariosType.md](ARCH_15_05_ParamsScenariosType.md) | `Params::Scenarios` — the new PARAMS type (shape ruling); naval-fleet types retired 2026-08-28; `ScenarioBody::areaName` (named-`Area` reference, additive wire key `AreaName`) added 2026-08-28 |
| §15.6 | [ARCH_15_06_CountScenariosOrdering.md](ARCH_15_06_CountScenariosOrdering.md) | `COUNT_SCENARIOS` ordering — array order IS the match-priority authoring action |
| §15.7 | [ARCH_15_07_OwnershipSplit.md](ARCH_15_07_OwnershipSplit.md) | Ownership split — who ratifies what for the new `Params::Scenarios` family |
| §15.8 | [ARCH_15_08_ThirdPartyDependencyRuling.md](ARCH_15_08_ThirdPartyDependencyRuling.md) | Third-party dependency ruling — ImGuiColorTextEdit + embedded LuaJIT |
| §15.9 | [ARCH_15_09_EngineWhitelistMigrationPath.md](ARCH_15_09_EngineWhitelistMigrationPath.md) | Engine-whitelist migration path (recorded as intended future simplification, not built) |
| §15.10 | [ARCH_15_10_SlotPatternConstructionMoves.md](ARCH_15_10_SlotPatternConstructionMoves.md) | Slot-pattern construction moves into the runtime; `maxArmySlotCount` becomes authored data (ratifies the human's construction-code-belongs-in-universal-mod-code decision; amends `MAP_SCENARIO_SPEC.md` §2/§3/§4) |
