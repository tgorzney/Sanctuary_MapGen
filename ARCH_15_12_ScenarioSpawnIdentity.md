[← ARCH index](ARCH.md) · [§15 ARCH_15_MapScenarioSystem](ARCH_15_MapScenarioSystem.md) · SanGen ARCH §15.12. **Only the ARCH Expert writes this file.**

### 15.12 `ScenarioSpawnPoint` / `ScenarioBody::spawnIds` — shared spawn-point pool, binding shape, wire format, validator (ratifies `work_orders/ARCH_AMENDMENT_DRAFT_ScenarioSpawnId.md`, revision 3)

**The problem this closes.** `Params::ScenarioSpawn` (`src/params/Scenario_PARAMS.h`) had no identity
for a physical spawn location independent of which army occupies it, and no shared authored source
for one — the live reference retypes the identical `ARMY_01` coordinate verbatim across three
scenario entries (`1v1`/`4human`/`1h3ai`), the exact duplication class that caused a prior shipped
regression (§8's `spawns` hard requirement exists because of it). There was also no validator
stopping two `ScenarioSpawn` rows in one scenario from naming the same `armyName` — the runtime
silently resolves iteration-order-wins.

**RULED — a shared, `Scenarios`-level pool (not a scenario-scoped `spawnId`).** `ScenarioSpawn`
(`armyName` + inline position) is retired outright, not extended. Replaced by:

```cpp
// Authored ONCE in the pool; referenced by spawnId from any scenario's spawnIds list. Never holds
// an ARMY_XX default position — only genuine per-scenario overrides (e.g. "North_1v1" -> ARMY_01).
struct ScenarioSpawnPoint {
    std::string spawnId;    // required non-empty; unique across the WHOLE pool (flat, shared
                             // namespace). Must NOT be ARMY_XX-shaped -- see Validator below.
    std::string armyName;   // required non-empty -- the ARMY_XX this custom point targets.
    float positionX = 0.0f, positionY = 0.0f, positionZ = 0.0f;
};

struct ScenarioBody {
    // ... name / area / areaName / spawnsUnits / alloyMode unchanged (§15.5) ...
    std::vector<std::string> spawnIds;    // RESHAPED (was std::vector<ScenarioSpawn> spawns). Each
                                           // entry is EITHER a Scenarios::spawnPoints[].spawnId (a
                                           // custom override) OR a literal ARMY_XX name (use that
                                           // army's own live baked .sanmap default) -- resolved by
                                           // the two-step lookup below.
    // ... alloys / alloysToAdd / alloysToRemove / authoringNote unchanged ...
};

struct Scenarios {
    // ... patternScenarios / countScenarios / defaultScenario / maxArmySlotCount unchanged ...
    std::vector<ScenarioSpawnPoint> spawnPoints;   // NEW -- the shared custom-spawn-point pool.
};
```

The pool holds ONLY custom/authored spawn points — never pre-seeded with `ARMY_XX` defaults, which
already live in the `.sanmap`/`GameInfo.MapData` as the single source of truth. `.sanmap`'s
`markers.Spawn.transforms[ARMY_XX]` is completely unchanged by this ruling.

**Two-step resolution — one algorithm, applied identically by every consumer (C++ validator/export,
Lua runtime), never reimplemented per call site:**

```
Resolve(spawnId):
  1. Scan Scenarios::spawnPoints for a row whose spawnId matches (first-match, mirrors this file
     family's own "first-match, never last-wins" idiom -- areaName's ResolveScenarioAreaRect).
     Found -> (row.armyName, row.positionX, row.positionY, row.positionZ).
  2. Not found -> spawnId IS the literal ARMY_XX name. Its position is never separately stored --
     read live from the current ARMY_XX Spawn marker transform (the .sanmap's own baked default).
  3. Matches neither -> unresolvable. See Validator below.
```

Steps 1 and 2 can never both match the same string — a structural guarantee (see Validator's
reserved-namespace rule), not a scan-order behavior.

**Wire format — both legs, breaking change, no migration.**
- **`.sanmap` JSON leg.** `BuildScenariosJson` gains a fifth top-level sibling key,
  `document["SpawnPoints"]` — an array of `{ "SpawnId", "ArmyName", "Position" }` objects, one per
  `Scenarios::spawnPoints` row (mirror `ReadSpawnPointsJson`). Per-`<ScenarioRecord>`, the existing
  `"Spawns"` key (array of `{ArmyName, Position}`) is **retired** and replaced by `"SpawnIds"` (a
  flat array of strings); `BuildSpawnsJson`/`ReadSpawnsJson` are retired, not extended. The C++
  field is renamed `spawns` → `spawnIds`, mirroring this file's own `navy` → `spawnsUnits`
  rename-on-meaning-change precedent (§15.5). This is a real breaking wire-type change
  (array-of-objects → array-of-strings), not an additive one. **RULED — no migration entry, no
  importer backward-compat shim.** Exactly one `.sanmap` exists (`Pandemonium Isthmus.sanmap`); the
  human hand-edits it directly to the new shape. **No `SanGenVersion` bump is tied to this shape
  change.** The one required addition: if a scenario record's
  `"Spawns"` key is present with object-shaped elements (the retired shape's own structural
  signature), log one loud WARN-ONLY message naming the scenario and stating the legacy shape was
  found and not read — never converted, never populated into `spawnIds`.
- **`<MapName>_Scenarios_Data.lua` leg — the pool's coordinates DO reach Lua**, the opposite of
  `areaName`'s posture (§15.5's amendment): `areaName`'s referent is always fully C++-resolved before
  either leg renders; this pool's literal-`ARMY_XX`-fallback case resolves to a value only the live
  runtime has, at match-load time, so the runtime must do the two-step lookup itself. A new
  `SCENARIO_SPAWN_POINTS` global, one flat row per pool entry (`{ spawnId, armyName, x, y, z }`),
  always rendered even when empty — genuinely authored `Params::Scenarios` data, the same category
  as `PATTERN_SCENARIOS`/`COUNT_SCENARIOS`, **not** a derived global like `ARMY_ID_TO_NAME`.
  `AppendScenarioBodyFields`'s per-scenario `spawns` array-of-tables render is replaced by a flat
  array-of-quoted-strings render of `body.spawnIds`.

**Validator — re-expressed against RESOLVED army names, not raw struct fields.**
- **Duplicate resolved `armyName` within one scenario.** Resolve every `spawnId` in one scenario's
  `spawnIds` (steps 1/2 above), then verify no two resolve to the same `armyName` — strictly
  per-scenario; two different `spawnId`s may legitimately resolve to the same `armyName` ACROSS
  different scenarios. Enforcement: loud, logged, never a flat refusal of the export (§15.5's "never
  a flat refusal" posture); fix-up is first-authored-wins, later-resolving `spawnId`(s) dropped from
  the scenario's rendered `spawnIds` on both legs.
- **Duplicate `spawnId` within `Scenarios::spawnPoints`.** The pool is a flat, shared namespace — a
  duplicate row is a real integrity problem. Same posture: loud/logged/non-blocking, first-authored-
  wins, later duplicate row(s) excluded from render.
- **Unresolvable `spawnId`** (matches neither the pool nor a real `ARMY_XX`). **Warn-only, never
  dropped, never invented** — reuses `MapExporter_ArmySpawnMarkerValidation_IO.h`'s
  `ArmySpawnMarkerValidationReport` posture for the sibling case of an army with no `Spawn` marker.
- **RULED (corrected 2026-09-05) — `ARMY_XX`-shaped strings are a reserved namespace, forbidden as
  pool `spawnId`s, case-insensitively.** Enforced by a NEW, dedicated predicate,
  `Io::ResemblesArmyIdentityCaseInsensitive(name)` — NOT `Io::IsArmyIdentityWellFormed`
  (`src/io/Sanmap_ArmyIdentity_IO.h:34-43`, STEP76), which stays case-sensitive and unchanged for its
  own STEP76 army-minting purpose: an army genuinely named `army_01` must still be correctly flagged
  as malformed there, since the engine's `markers.Spawn.transforms` lookup is exact-case, and folding
  case in that predicate would be a regression, not a fix. The new predicate case-folds `name` to
  lowercase — ASCII `std::tolower`, the one case-folding idiom that actually exists in this codebase
  today (`TemplateSourceScan_IO.cpp:37`'s extension check; the not-yet-built
  `ScenarioNameValidation_IO.cpp` planned in `work_orders/STEP251_ScenarioCategoryFourExport_IO.md`
  §1b point 2 will use this same idiom for its own, unrelated reserved-word check on scenario names)
  — and applies the identical prefix-plus-two-or-more-digits shape `IsArmyIdentityWellFormed` checks,
  on the folded string. A `Scenarios::spawnPoints` row is invalid if
  `ResemblesArmyIdentityCaseInsensitive(row.spawnId)` is true. Same enforcement points/posture as the
  sibling rules above (UI-authoring time and export time), the offending row's write refused/excluded
  with a named error. This makes steps 1/2 of the resolution algorithm structurally non-colliding, by
  construction — not a convention an author could forget to check. The new predicate is added as a
  sibling function alongside `IsArmyIdentityWellFormed` in the same header (`Sanmap_ArmyIdentity_IO.h`),
  never substituting for that predicate's own STEP76 call sites. (Corrected 2026-09-05: originally
  this bullet specified negating `IsArmyIdentityWellFormed` directly, but that predicate is
  case-sensitive and would not catch `"army_01"`/`"Army_01"` — see
  `work_orders/ARCH_CORRECTION_DRAFT_ScenarioSpawnIdCaseInsensitivity.md` for the full diagnosis.)

Runtime consumption of this shape (`SanGenScenarioRuntime.lua`'s `ApplyScenario` rewrite) is ratified
separately at **§15.13**, with the same weight as this section — read both before implementing.
