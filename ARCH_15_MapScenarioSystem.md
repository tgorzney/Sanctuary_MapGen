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

**Note (2026-08-29): §15.3's "never read scenario content back" sentence is NARROWED — not
reversed — by §15.11.** One carve-out exists: a human-triggered, one-shot, non-executing import of
**area rectangles only** from a **foreign** (SanGen-never-writes-it) scenario `.lua`. §15.3 still
binds absolutely for every SanGen-generated `.lua` and for every other kind of scenario content.

**Note (2026-09-11): §15.11's carve-out is EXTENDED — not reversed — by §15.14.** Two more narrow,
closed shapes may now be extracted from the same class of FOREIGN scenario `.lua`: (1) a fixed
AND-chain of `t`/`h`/`a` count comparisons or a fixed `pattern:sub(N,M):find("[^-]")` slot-range
template, both mapping onto the already-ratified `ScenarioCountCondition` shape, and (2) an exact
`PatternScenario::slotPattern` string literal. §15.14 also ratifies a new, additive, declarative
`Params::ScenarioUnitPlacement` type (independent of, not a replacement for, `spawnsUnits`) plus a
narrowly-scoped literal-tuple import shape for it. Every other kind of scenario content — `spawns`,
`alloys`, `alloyMode`, any `or`-bearing or otherwise non-template match expression, and all
procedural placement logic — remains absolutely forbidden per §15.3/§15.11's standing language.

**Amendment (2026-09-11, same day): §15.14's original `ScenarioUnitPlacement` rotation ruling was
corrected.** The section's first pass wrongly characterized rotation ("`facingDegrees`") as having
no confirmed engine consumer. Independently re-verified against the vendored game's own Lua/FFI
sources: `Engine.CreateUnit` genuinely takes a native quaternion `orientation` parameter, wired
end-to-end through the engine's own unit-creation path — units in this engine DO support rotation.
The gap is narrower and shallower than "unsupported": specific game-owned Lua call sites
(`common/gameUtils.lua:386,453`, and this pack's own `Scenario.SpawnUnits`, mirroring the live
reference) simply don't forward the parameter today. `§15.14` now gives `ScenarioUnitPlacement`
`Params::UnitTransform`-shaped quaternion rotation fields (not the retracted ad-hoc float) and
specifies the runtime forwarding fix for SanGen's own executor — see §15.14 itself for the full
corrected ruling and ground truth.

**Amendment 2 (2026-09-12): §15.14's own two binding Lua snippets contradicted each other on
rotation's wire shape — the render leg nested it, the runtime read it flat — and STEP263's coder
correctly built both exactly as written rather than silently reconciling them, leaving the bug live
in the tree (baked-placement rotation silently no-oped end-to-end). Independently re-verified
against the committed source (`ScenarioScript_DataLua_IO.cpp`, `SanGenScenarioRuntime.lua`) before
ruling. RULED: flat wins — it already matched `Scenario_PARAMS.h`, the shipped `.sanmap` JSON leg
(STEP260), and the runtime snippet; only the Lua-render leg was wrong and is corrected in `§15.14`
in place, with a narrow, concrete coder fix specified there.** No architectural change — a spec-text
self-contradiction fix, not a new ruling.

---

### Subsections of §15

| § | File | Ruling |
|---|------|--------|
| §15.1 | [ARCH_15_01_LayerClassification.md](ARCH_15_01_LayerClassification.md) | Layer classification |
| §15.2 | [ARCH_15_02_IoScopeRuling.md](ARCH_15_02_IoScopeRuling.md) | IO scope ruling — corrects an earlier assumption, does not reverse it |
| §15.3 | [ARCH_15_03_ExportOnlyLuaRatified.md](ARCH_15_03_ExportOnlyLuaRatified.md) | Design ratified: option (c) — export-only, SanGen never parses Lua back (resolves §15.2's open question / `MAP_SCENARIO_SPEC.md` §8); **narrowed 2026-08-29 by §15.11, extended again 2026-09-11 by §15.14** |
| §15.4 | [ARCH_15_04_ThreeFileOnDiskShape.md](ARCH_15_04_ThreeFileOnDiskShape.md) | Three-file on-disk shape + overwrite safety (ratifies `MAP_SCENARIO_SPEC.md` §2/§2.1/§2.2) |
| §15.5 | [ARCH_15_05_ParamsScenariosType.md](ARCH_15_05_ParamsScenariosType.md) | `Params::Scenarios` — the new PARAMS type (shape ruling); naval-fleet types retired 2026-08-28; `ScenarioBody::areaName` (named-`Area` reference, additive wire key `AreaName`) added 2026-08-28 |
| §15.6 | [ARCH_15_06_CountScenariosOrdering.md](ARCH_15_06_CountScenariosOrdering.md) | `COUNT_SCENARIOS` ordering — array order IS the match-priority authoring action |
| §15.7 | [ARCH_15_07_OwnershipSplit.md](ARCH_15_07_OwnershipSplit.md) | Ownership split — who ratifies what for the new `Params::Scenarios` family |
| §15.8 | [ARCH_15_08_ThirdPartyDependencyRuling.md](ARCH_15_08_ThirdPartyDependencyRuling.md) | Third-party dependency ruling — ImGuiColorTextEdit + embedded LuaJIT |
| §15.9 | [ARCH_15_09_EngineWhitelistMigrationPath.md](ARCH_15_09_EngineWhitelistMigrationPath.md) | Engine-whitelist migration path (recorded as intended future simplification, not built) |
| §15.10 | [ARCH_15_10_SlotPatternConstructionMoves.md](ARCH_15_10_SlotPatternConstructionMoves.md) | Slot-pattern construction moves into the runtime; `maxArmySlotCount` becomes authored data (ratifies the human's construction-code-belongs-in-universal-mod-code decision; amends `MAP_SCENARIO_SPEC.md` §2/§3/§4) |
| §15.11 | [ARCH_15_11_ForeignScenarioAreaImport.md](ARCH_15_11_ForeignScenarioAreaImport.md) | Narrow, permanently-bounded carve-out from §15.3 — one-shot import of AREA RECTANGLES ONLY from a FOREIGN scenario `.lua` (2026-08-29); **extended 2026-09-11 by §15.14** |
| §15.12 | [ARCH_15_12_ScenarioSpawnIdentity.md](ARCH_15_12_ScenarioSpawnIdentity.md) | `ScenarioSpawnPoint`/`ScenarioBody::spawnIds` — shared custom-spawn-point pool replacing the retired per-scenario `ScenarioSpawn`; two-step `spawnId` resolution |
| §15.13 | [ARCH_15_13_ScenarioSpawnIdRuntimeResolution.md](ARCH_15_13_ScenarioSpawnIdRuntimeResolution.md) | `SanGenScenarioRuntime.lua`'s `ApplyScenario` rewrite consuming §15.12's shape |
| §15.14 | [ARCH_15_14_ForeignScenarioFullDataImportAndUnitPlacement.md](ARCH_15_14_ForeignScenarioFullDataImportAndUnitPlacement.md) | (2026-09-11, amended 2026-09-11 and again 2026-09-12) Narrow extension of §15.11 — closed-grammar extraction of count/slot-range match conditions and exact `slotPattern` strings from a FOREIGN scenario `.lua`; new additive `Params::ScenarioUnitPlacement` type (coexists with `spawnsUnits`) plus its own literal-tuple import shape; rotation is CONFIRMED real, engine-wired data (quaternion fields mirroring `Params::UnitTransform`); **2026-09-12: fixes a self-contradiction between the section's own render-leg and runtime-consumption snippets — FLAT rotation siblings win, a narrow coder fix is specified in place** |
