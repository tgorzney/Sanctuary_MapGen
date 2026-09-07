# STEP252 — Implement `ARCH_15_12`/`ARCH_15_13`: retire `ScenarioSpawn`, build the `spawnId` pool

**Layer:** PARAMS + IO/BRIDGE + UI + the bundled Lua runtime. **Domain:** `Params::ScenarioSpawn` and
every reader/writer of it. **Sequence:** ratifies nothing new — `ARCH_15_12_ScenarioSpawnIdentity.md`
and `ARCH_15_13_ScenarioSpawnIdRuntimeResolution.md` are already law (ratified 2026-09-04, corrected
2026-09-05). This ticket is the first thing that actually implements them. Depends on nothing undone.

**⚠️ Urgent, not routine.** `Params::ScenarioSpawn` (`src/params/Scenario_PARAMS.h:23`) is currently
LIVE, SHIPPED code that is out of compliance with already-ratified architecture — every scenario map
exported today still uses the retired shape. This is not a stale doc; it is real code contradicting
real law, confirmed by a full backlog audit (2026-09-05). Land this before anything else in the
Scenario track (`STEP251`, `PHASE_A_ScenarioDataMigration_PandemoniumIsthmus.md`'s hand-conversion,
and `STEP250` if it touches anything Scenario-adjacent — it does not, per that ticket's own file list,
but re-check before merging).

**Read `ARCH_15_12_ScenarioSpawnIdentity.md` and `ARCH_15_13_ScenarioSpawnIdRuntimeResolution.md` in
full before starting.** They are the binding law; this ticket restates their exact shapes with real
file/line anchors against the CURRENT tree (re-verified 2026-09-05, not copied from an older read) —
it is not a paraphrase to work from independently. Where this ticket's own text and the two ARCH files
disagree on anything, the ARCH files win; flag the discrepancy rather than silently picking one.

---

## 0. The immovable constraint (unchanged, do not re-derive)

The final applied write always lands keyed by `ARMY_XX`
(`GameInfo.MapData.markers.Spawn.transforms[army.name]`, `common/gameUtils.lua`'s hardcoded read
path — base-engine code, unmodifiable). Nothing in this ticket touches that. `spawnId` is additive
identity layered on top of `armyName`, never a replacement for it.

## 1. PARAMS — `src/params/Scenario_PARAMS.h` (EDIT)

Current (line 23): `struct ScenarioSpawn { std::string armyName; float positionX = 0.0f, positionY
= 0.0f, positionZ = 0.0f; };`, consumed by `ScenarioBody::spawns` (line 51,
`std::vector<ScenarioSpawn> spawns;`).

**Retire `ScenarioSpawn` entirely.** Replace with:

```cpp
// The shared, Scenarios-level custom-spawn-point pool row. Authored ONCE; referenced by spawnId
// from any scenario's spawnIds list. Never holds an ARMY_XX default position (ARCH_15_12) — the
// .sanmap's own baked markers.Spawn.transforms[ARMY_XX] is the single source of truth for that;
// this pool holds only genuine per-scenario overrides.
struct ScenarioSpawnPoint {
    std::string spawnId;    // required non-empty; unique across the whole pool. Must NOT be
                             // ARMY_XX-shaped (Io::ResemblesArmyIdentityCaseInsensitive) — see §2.
    std::string armyName;   // required non-empty — the ARMY_XX this custom point targets.
    float positionX = 0.0f, positionY = 0.0f, positionZ = 0.0f;
};
```

`ScenarioBody::spawns` (line 51) becomes:
```cpp
std::vector<std::string> spawnIds;   // RESHAPED (was std::vector<ScenarioSpawn> spawns). Each entry
                                      // is EITHER a Scenarios::spawnPoints[].spawnId (a custom
                                      // override) OR a literal ARMY_XX name (use that army's own
                                      // live baked .sanmap default) — resolved by the two-step
                                      // lookup in §3/§5, never distinguished by shape here.
```

`Scenarios` (the struct holding `patternScenarios`/`countScenarios`/`defaultScenario`/
`maxArmySlotCount`) gains:
```cpp
std::vector<ScenarioSpawnPoint> spawnPoints;   // NEW — the shared custom-spawn-point pool.
```

Update this file's own top-of-file "source of truth" comment to add a citation to
`ARCH_15_12_ScenarioSpawnIdentity.md` for `ScenarioSpawnPoint`/`ScenarioBody::spawnIds` specifically
(alongside its existing `ARCH_15_05`/`ARCH_15_10` citations).

## 2. New predicate — `src/io/Sanmap_ArmyIdentity_IO.h` (EDIT)

Add, as a sibling of `IsArmyIdentityWellFormed` (do not modify that function — it stays case-sensitive
for its own STEP76 purpose):

```cpp
// True when `name` merely RESEMBLES an ARMY_XX identity, case-insensitively — a narrower question
// than IsArmyIdentityWellFormed's "is this a real, usable army identity" (which is case-sensitive,
// correct for STEP76 minting: the engine's Spawn.transforms lookup is exact-case, so a genuinely
// malformed lowercase name must still be flagged there). Used ONLY to forbid a Scenarios::spawnPoints
// row's spawnId from shadowing ARMY_XX (ARCH_15_12's Validator, corrected 2026-09-05) — never
// substituted for IsArmyIdentityWellFormed's own call sites.
inline bool ResemblesArmyIdentityCaseInsensitive(const std::string& name) {
    std::string folded = name;
    for (char& c : folded) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return IsArmyIdentityWellFormed(folded);
}
```
Add `#include <cctype>` for `std::tolower`.

## 3. New shared validator — `src/io/ScenarioSpawnIdValidation_IO.h`/`.cpp` (NEW)

Mirrors `MapExporter_ArmySpawnMarkerValidation_IO.h`'s WARN-ONLY report shape and
`STEP251`'s `ScenarioNameValidation_IO.h`'s "shared by UI + both export legs, pure, no filesystem"
posture — do not reinvent either pattern.

```cpp
// ScenarioSpawnIdValidation_IO.h -- pure, disk-free validation of the spawnId pool and each
// scenario's spawnIds references (ARCH_15_12's Validator section). Layer: IO. SHARED by the UI
// (live, per-frame, non-blocking inline warning) and both export legs — one validator, never two
// independently invented copies of the rule.
#pragma once
#include <string>
#include <vector>

namespace SanmapGen {
namespace Params { struct Scenarios; struct ScenarioBody; }
namespace Io {

// Resolves one spawnId against the pool (first-match) then the literal-ARMY_XX fallback. Returns
// false (unresolvable) with outArmyName/outX/Y/Z left untouched — ARCH_15_12 §Resolve step 3.
// `currentArmyIdentityTransforms` is nullable (export-time validation may run with no live
// GameInfo — pass nullptr and step 2 simply cannot resolve; the UI/runtime pass the real thing).
struct ArmyIdentityTransformLookup {
    // Returns true and fills outX/Y/Z if `armyName` names a real, currently-known Spawn transform.
    virtual bool Find(const std::string& armyName, float& outX, float& outY, float& outZ) const = 0;
    virtual ~ArmyIdentityTransformLookup() = default;
};
bool ResolveSpawnId(const std::string& spawnId, const std::vector<Params::ScenarioSpawnPoint>& pool,
                    const ArmyIdentityTransformLookup* armyDefaults,
                    std::string& outArmyName, float& outX, float& outY, float& outZ);

struct ScenarioSpawnIdValidationReport {
    struct Violation {
        std::string descriptor;   // "pool" or the scenario's own name/tier+index fallback
        std::string detail;       // the offending spawnId/armyName, as-authored
        std::string reason;       // one full sentence
    };
    std::vector<Violation> violations;
    bool AllValid() const { return violations.empty(); }
    std::string SummaryText() const;
};

// Walks Scenarios::spawnPoints for: (a) ARMY_XX-shaped spawnId (reserved namespace,
// Io::ResemblesArmyIdentityCaseInsensitive), (b) duplicate spawnId within the pool. Then walks every
// ScenarioBody's spawnIds for: (c) two entries resolving to the same armyName within ONE scenario
// (cross-scenario reuse of the same armyName is explicitly fine, per the human's own ruling — only
// checked per-scenario), (d) an unresolvable spawnId (warn-only, never dropped). Fix-up posture
// (first-authored-wins drop for a/b/c, warn-only leave-alone for d) is applied by the CALLER using
// this report — this function only REPORTS, matching ArmySpawnMarkerValidationReport's own posture.
ScenarioSpawnIdValidationReport ValidateScenarioSpawnIds(const Params::Scenarios& scenarios);

} // namespace Io
} // namespace SanmapGen
```

Implement `.cpp` per the report's own doc comment. `SummaryText()` mirrors
`ScenarioNameValidationReport::SummaryText()`'s shape (empty when `AllValid()`, else a count line +
one indented `<descriptor>: <detail> — <reason>` line per violation).

**Fix-up application (both export legs, before rendering):**
- Pool: drop later-authored `ScenarioSpawnPoint` rows whose `spawnId` duplicates an earlier one, or
  is `ARMY_XX`-shaped, from `spawnPoints` — first-authored-wins, loud-logged, never a flat refusal.
- Per scenario: resolve every `spawnIds` entry (via `ResolveSpawnId`), drop later entries that
  resolve to an `armyName` an earlier entry in the SAME scenario already resolved to.
- Unresolvable `spawnIds` entries: leave as-authored, warn-only.

## 4. `.sanmap` JSON leg — `src/io/MapExporter_Scenarios_IO.cpp` (EDIT)

- **Retire `BuildSpawnsJson`** (lines 32-39). Replace with:
  ```cpp
  nlohmann::ordered_json BuildSpawnIdsJson(const std::vector<std::string>& spawnIds) {
      nlohmann::ordered_json array = nlohmann::ordered_json::array();
      for (const std::string& spawnId : spawnIds) array.push_back(spawnId);
      return array;
  }

  nlohmann::ordered_json BuildSpawnPointsJson(const std::vector<Params::ScenarioSpawnPoint>& spawnPoints,
                                              int mapSize) {
      nlohmann::ordered_json array = nlohmann::ordered_json::array();
      for (const Params::ScenarioSpawnPoint& point : spawnPoints) {
          array.push_back({ { "SpawnId", point.spawnId }, { "ArmyName", point.armyName },
                           { "Position", BuildPositionJson(point.positionX, point.positionY,
                                                           point.positionZ, mapSize) } });
      }
      return array;
  }
  ```
- `BuildScenarioRecordJson` (line 90): `json["Spawns"] = BuildSpawnsJson(body.spawns, mapSize);`
  becomes `json["SpawnIds"] = BuildSpawnIdsJson(body.spawnIds);`.
- `BuildScenariosJson` (line ~126-131): before building `document`, call
  `ValidateScenarioSpawnIds` on a **copy** of `recipe.scenarios` and apply the fix-ups from §3 to
  that copy (mirrors the existing STEP76 `AssignArmyIdentities` re-mint-defensively pattern — export
  never trusts the roster is already clean). Add the fifth top-level sibling key:
  `document["SpawnPoints"] = BuildSpawnPointsJson(fixedUpScenarios.spawnPoints, mapSize);`.
- **WARN-ONLY legacy-shape detector** (new, small): this is import-side per §15.12's own text
  ("if a scenario record's `"Spawns"` key is present with object-shaped elements... log... never
  read") — implement in the IMPORTER (§5), not here; the exporter never needs to detect its own
  retired shape since it only ever writes the new one.

## 5. `.sanmap` JSON leg (import) — `src/io/MapImporter_ScenarioRecord_IO.cpp` (EDIT)

- **Retire `ReadSpawnsJson`** (lines 31-42). Replace with:
  ```cpp
  void ReadSpawnIdsJson(const nlohmann::json& parent, const char* key,
                       std::vector<std::string>& outSpawnIds) {
      if (!parent.contains(key) || !parent[key].is_array()) return;
      outSpawnIds.clear();
      for (const nlohmann::json& entry : parent[key])
          if (entry.is_string()) outSpawnIds.push_back(entry.get<std::string>());
  }

  void ReadSpawnPointsJson(const nlohmann::json& parent, const char* key,
                          std::vector<Params::ScenarioSpawnPoint>& outSpawnPoints, int mapSize) {
      if (!parent.contains(key) || !parent[key].is_array()) return;
      outSpawnPoints.clear();
      for (const nlohmann::json& pointJson : parent[key]) {
          if (!pointJson.is_object()) continue;
          Params::ScenarioSpawnPoint point;
          ReadJsonText(pointJson, "SpawnId", point.spawnId);
          ReadJsonText(pointJson, "ArmyName", point.armyName);
          ReadPositionJson(pointJson, point.positionX, point.positionY, point.positionZ, mapSize);
          outSpawnPoints.push_back(point);
      }
  }
  ```
  `ReadSpawnPointsJson` is called once, at the `Scenarios`-level parse site (wherever
  `MaxArmySlotCount` is currently read — same tier as that field, not per-`ScenarioBody`).
- `ReadScenarioBodyJson` (line 99): `ReadSpawnsJson(json, "Spawns", body.spawns, mapSize);` becomes
  `ReadSpawnIdsJson(json, "SpawnIds", body.spawnIds);`.
- **WARN-ONLY legacy-shape detector**, per `ARCH_15_12`: inside `ReadScenarioBodyJson`, before (or
  instead of) the `ReadSpawnIdsJson` call, check: if `json.contains("Spawns")` and
  `json["Spawns"].is_array()` and its first element (if any) `is_object()` — this is the OLD shape's
  own structural signature (new shape's array elements are plain strings). Log one loud warning
  naming `body.name` and stating the legacy shape was found and NOT read; do not attempt to convert
  it into `spawnIds`. This is the entire migration posture ARCH_15_12 rules for this ticket — no
  transformation, no version bump, no `IO_MIGRATION_SPEC.md` unit (confirmed out of scope, human's
  own ruling: exactly one hand-authored `.sanmap` exists and is hand-converted directly).

## 6. Lua-rendering leg — `src/io/ScenarioScript_DataLua_IO.cpp` (EDIT)

- `AppendScenarioBodyFields` (line 123): replace
  `AppendArrayOfTables(out, indentLevel, "spawns", BuildSpawnRowBodies(body.spawns, mapSize));`
  with a flat array-of-quoted-strings render of `body.spawnIds` — reuse
  `AppendArrayOfQuotedStrings` (already used identically for `KNOWN_ALLOY_MARKERS`'s per-army marker
  list at line 269): `AppendArrayOfQuotedStrings(out, indentLevel, "spawnIds", body.spawnIds);`.
  **Delete** `BuildSpawnRowBodies` (lines 66-74) — no longer called anywhere.
- **New top-level render**, alongside `BuildArmyIdToNameTable`/`BuildKnownAlloyMarkersTable` (called
  from `BuildScenarioDataLuaText`, lines 293-294 — same tier, add a third call there):
  ```cpp
  std::string BuildSpawnPointsTable(const std::vector<Params::ScenarioSpawnPoint>& spawnPoints,
                                    int mapSize) {
      std::vector<std::string> rows;
      rows.reserve(spawnPoints.size());
      for (const Params::ScenarioSpawnPoint& point : spawnPoints) {
          rows.push_back("spawnId = " + QuotedLuaString(point.spawnId)
                        + ", armyName = " + QuotedLuaString(point.armyName)
                        + ", x = " + RenderLuaNumber(point.positionX)
                        + ", y = " + RenderLuaNumber(point.positionY)
                        + ", z = " + RenderLuaNumber(FlipPositionZ(point.positionZ, mapSize)));
      }
      std::string out;
      AppendArrayOfTables(out, 0, "SCENARIO_SPAWN_POINTS", rows);
      return out;
  }
  ```
  `SCENARIO_SPAWN_POINTS` is genuinely authored `Params::Scenarios` data (a direct 1:1 render of
  `spawnPoints`), **not** a derived global like `ARMY_ID_TO_NAME`/`KNOWN_ALLOY_MARKERS` — this file's
  own header comment warning against adding a `Params::Scenarios` field to match those two does not
  apply here. Always rendered, even when `spawnPoints` is empty (`SCENARIO_SPAWN_POINTS = {}`),
  matching this family's "every table always present" convention.
- Call `ValidateScenarioSpawnIds` + apply fix-ups here too, on the same copy-before-render pattern as
  §4 (this leg is independently rendered from the JSON leg, never shares its resolved data — same
  "each leg owns its own copy" precedent this file's own header comment already states for its
  spelling tables).

## 7. Runtime — `resources/lua/SanGenScenarioRuntime.lua` (EDIT)

Exactly as specified in `ARCH_15_13_ScenarioSpawnIdRuntimeResolution.md` — this is ratified, binding
Lua code, not left to coder discretion:

- Add a fifth capture near the existing `ScenarioData.PATTERN_SCENARIOS` etc. captures:
  `local SCENARIO_SPAWN_POINTS = ScenarioData.SCENARIO_SPAWN_POINTS`.
- Replace `ApplyScenario`'s spawn block (current lines 169-182, quoted in full above) with:
  ```lua
  local function ResolveSpawnId(spawnId, spawnTransforms)
      for _, point in ipairs(SCENARIO_SPAWN_POINTS) do
          if point.spawnId == spawnId then
              return point.armyName, point.x, point.y, point.z
          end
      end
      local defaultTransform = spawnTransforms and spawnTransforms[spawnId]
      if defaultTransform then
          return spawnId, defaultTransform.position.x, defaultTransform.position.y,
                 defaultTransform.position.z
      end
      Warn("SANGEN: scenario spawnId '"..tostring(spawnId).."' matched neither the custom "..
           "spawn-point pool nor a live ARMY_XX Spawn marker -- skipped, no transform written.")
      return nil
  end

  local function ApplyScenario(scenario, total, slotPattern)
      if scenario.spawnIds then
          local spawnTransforms = GameInfo.MapData.markers and GameInfo.MapData.markers.Spawn
              and GameInfo.MapData.markers.Spawn.transforms
          if spawnTransforms then
              for _, spawnId in ipairs(scenario.spawnIds) do
                  local armyName, x, y, z = ResolveSpawnId(spawnId, spawnTransforms)
                  if armyName and spawnTransforms[armyName] then
                      spawnTransforms[armyName].position.x = x
                      spawnTransforms[armyName].position.y = y
                      spawnTransforms[armyName].position.z = z
                  end
              end
          end
      end
      -- alloy handling below (lines 184+) is UNCHANGED
  ```
- Update this file's own header comment: the "STRUCTURAL PORT... KEYED BY ARMY NAME vs. FLAT ARRAYS"
  note (current lines 162-167) is now stale in a NEW way (it already correctly describes the old
  `spawns`/`armyName`-per-row shape as a structural port from the reference script — extend it, in
  place, to also describe this second port: `spawnIds` is a flat array of bare strings, resolved
  against `SCENARIO_SPAWN_POINTS` + the live `.sanmap` default, per `ARCH_15_13`).
- Add `ARCH_15_13_ScenarioSpawnIdRuntimeResolution.md` to this file's own top-of-file citation list
  (alongside whatever it currently cites).

## 8. UI — `src/ui/ScenariosTab_Detail_UI.cpp` (EDIT)

`DrawScenarioSpawnsList` (lines 118-138) currently edits `std::vector<Params::ScenarioSpawn>`
directly (`DrawArmyNameField` + 3 sliders per row). **Redesign required — UI Expert's call on exact
widget composition, this ticket specifies the data contract only:**

- One editor becomes TWO: (a) a per-scenario `spawnIds` editor — a flat list of text-input rows
  (or a picker showing the pool's own `spawnId`s plus every `ARMY_XX` as selectable options) bound to
  `ScenarioBody::spawnIds`, structurally similar to `DrawScenarioSpawnsList`'s existing "+ Add" /
  per-row remove-button shape but editing bare strings instead of `ScenarioSpawn` structs; (b) a
  pool editor for `Scenarios::spawnPoints` (spawnId / armyName / x / y / z per row) — likely its own
  section, structurally similar to how `recipe.areas` gets `AreasTab_List_UI.h` as its own dedicated
  list, since the pool is `Scenarios`-level, not per-scenario-body.
- Wire `ValidateScenarioSpawnIds` (§3) into both editors as a live, non-blocking inline warning
  (`ImGui::TextColored` or this codebase's existing warning-banner idiom — check `ScenariosTab_
  SpawnsWarning_UI.cpp` for the established pattern before inventing a new one).
- `DrawScenarioAlloyOverridesList`/other alloy-related draw functions in this file are UNCHANGED —
  `ScenarioAlloyOverride`/`ScenarioAlloyRemoval` are untouched by this ticket.

## 9. Out of scope — do not build, do not stub

- Any `IO_MIGRATION_SPEC.md` migration unit, any `SanGenVersion` bump — ruled out explicitly (§5).
- Any change to `STEP251`'s category-4 export/dispatcher work — separate, coordinate on merge only
  (both touch `ScenarioBody`-adjacent fields; land this ticket first, STEP251 second, and re-verify
  STEP251's own text against whatever this ticket actually ships before dispatching it).
- Any change to `Scenario.SpawnMatchedScenarioUnits`/`Scenario.SpawnUnits`/the alloy-application block
  in the runtime — untouched by `ARCH_15_13`, untouched here.
- Hand-applying `PHASE_A_ScenarioDataMigration_PandemoniumIsthmus.md`'s content to the real
  `Pandemonium Isthmus.sanmap` — that is a separate, manual, human action *after* this ticket ships
  (the file needs code that reads the new shape to exist first).

## 10. Acceptance tests

- **NEW `src/params/Scenario_PARAMS_Test.cpp`** (or extend the existing one if present): construct a
  `ScenarioSpawnPoint`, confirm default-construction is well-formed (empty strings, zero floats).
- **NEW `src/io/ScenarioSpawnIdValidation_IO_Test.cpp`**: (1) pool `spawnId` `"ARMY_01"`/`"army_01"`/
  `"Army_01"` all flagged reserved-namespace-forbidden; `"North_1v1"` not flagged. (2) duplicate pool
  `spawnId` flagged, first-authored kept on fix-up. (3) two `spawnIds` in one scenario resolving to
  the same `armyName` flagged; the SAME two armyNames across DIFFERENT scenarios NOT flagged. (4) an
  unresolvable `spawnId` (matches neither pool nor a real `ARMY_XX`-shaped string with a live
  transform) flagged warn-only, never dropped by the fix-up. (5) `ResolveSpawnId` pool-hit,
  literal-fallback-hit, and miss, each independently.
- **EDIT `src/io/MapImporter_IO_Test.cpp`**: legacy fixture with the OLD `"Spawns"`
  array-of-objects shape — assert it is NOT read into `spawnIds`, and the importer logs (does not
  crash on) the legacy shape. A NEW-shape `"SpawnIds"` (array of strings) fixture round-trips
  exactly. A `"SpawnPoints"` top-level pool fixture round-trips exactly (`spawnId`/`armyName`/
  position all survive).
- **EDIT `src/io/ScenarioScript_DataLua_IO_Test.cpp`** (or wherever `BuildScenarioDataLuaText`'s
  existing tests live): assert `SCENARIO_SPAWN_POINTS` is always rendered (including `= {}` when the
  pool is empty); assert a scenario's `spawnIds` renders as a flat array of quoted strings, not
  array-of-tables.
- **EDIT `src/ui/ScenariosTab_Detail_UI_Test.cpp`** (if one exists) or NEW: exercise the redesigned
  spawn editors against a headless frame, per this codebase's usual `HeadlessImguiSession` pattern.

## 11. Verify

- Full solo rebuild + `ctest -C Debug` at 100%.
- `grep -rn "ScenarioSpawn\b" src/` (word-boundary, excluding `ScenarioSpawnPoint`/
  `ScenarioSpawnIdValidation`) returns nothing — confirms the old struct is fully retired, not left
  as dead code anywhere.
- `grep -n "body.spawns\b" src/` returns nothing; every call site now reads `body.spawnIds`.
- Live-frame check (per this codebase's UI-testing convention): open the Scenarios tab, confirm the
  spawn editors draw, the pool editor lets you add/remove rows, and a reserved-namespace violation
  shows the inline warning.
