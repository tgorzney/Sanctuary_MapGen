# STEP260 — `ScenarioUnitPlacement`: PARAMS + `.sanmap` round-trip + validation

**Layer:** PARAMS + IO/BRIDGE. **Domain:** `src/params/Scenario_PARAMS.h`,
`src/io/MapExporter_ScenarioRecord_IO.cpp`, `src/io/MapImporter_ScenarioRecord_IO.cpp`, new
`ScenarioUnitPlacementValidation_IO.h/.cpp`. **Executor:** SanGen Coder. **Sequence:** implements
`ARCH_15_14_ForeignScenarioFullDataImportAndUnitPlacement.md`'s new data type. First ticket in the
scenario-import track (STEP260-265) — every other ticket in the track depends on this one landing
first, since the struct must exist before anything can populate it.

**Read `ARCH_15_14_ForeignScenarioFullDataImportAndUnitPlacement.md` in full before starting,
including its amendment correcting the original `facingDegrees` proposal to a quaternion.** It is
the binding law; this ticket restates its shape with real file/line anchors against the current tree.
Where this ticket's text and the ARCH section disagree, the ARCH section wins.

## 0. Why

`ScenarioBody::spawnsUnits` (`Scenario_PARAMS.h:56-62`) is a bare opt-in bool with no dispatch data —
"true alone spawns nothing." The real Pandemonium Isthmus scenario spawns explicit navy/air units at
literal, pre-computed positions via `Scenario.SpawnUnits(instructions)`, a flat
`{armyIndex, templateIdentifier, x, y, z}` array — SanGen has no way to author or store this today.
`ARCH_15_14` closes the gap with `ScenarioUnitPlacement`, reusing `Params::UnitTransform`'s quaternion
rotation shape (`Army_PARAMS.h`) rather than a `facingDegrees` float: `Engine.CreateUnit` genuinely
accepts a quaternion `orientation` argument end-to-end (confirmed against
`engineFunctions.lua`/`unitsUtilities.lua`); only the game's own `gameUtils.lua` wrapper this scenario
system calls currently drops it before forwarding (`-- TODO: rotation`) — a game-code gap, not an
engine limitation, so SanGen's own data model stores the real shape. No scale field: confirmed no
unit-level scale consumer exists anywhere in ground truth (unlike props/decals, which do consume
scale via `Engine.InstantiatePrefab`).

## 1. PARAMS — `src/params/Scenario_PARAMS.h`

Add the new struct near `ScenarioSpawnPoint` (same file, same "hand-authored, pass-through data"
posture per this file's own header comment), and one new field on `ScenarioBody`, appended after
`authoringNote` (last field today, `:73-76`) to match the wire order below.

```cpp
// ADDED (STEP260, ARCH_15_14_ForeignScenarioFullDataImportAndUnitPlacement.md §15.14) -- closes the
// gap left by `spawnsUnits` (a bare opt-in bool with no dispatch data, see comment above): explicit,
// pre-baked unit placements, one row per unit. Mirrors Params::UnitTransform's quaternion rotation
// shape (Army_PARAMS.h) rather than a facingDegrees float -- ARCH_15_14's ruling: Engine.CreateUnit
// genuinely accepts a quaternion `orientation` argument; only the game's own gameUtils.lua wrapper
// this scenario system calls currently drops it before forwarding (a game-code gap, not an engine
// limitation) -- so SanGen's own data model stores the real shape, not a lossy substitute. No scale
// field: confirmed no unit-level scale consumer exists anywhere in ground truth.
struct ScenarioUnitPlacement {
    std::string armyName;            // required non-empty -- the ARMY_XX this unit belongs to.
    std::string templateIdentifier;  // required non-empty -- unit blueprint/template id (e.g.
                                      // "ucn3001"). Free-text, NOT validated against a live template
                                      // list -- no such list exists in this codebase (mirrors
                                      // ScenarioAlloyOverride::markerName's own honest-fallback posture).
    float positionX = 0.0f, positionY = 0.0f, positionZ = 0.0f;
    float rotationX = 0.0f, rotationY = 0.0f, rotationZ = 0.0f, rotationW = 1.0f;  // quaternion
                                      // identity default -- same shape/default as
                                      // Params::UnitTransform (Army_PARAMS.h).
};
```

`ScenarioBody` gains:
```cpp
    std::vector<ScenarioUnitPlacement> unitPlacements;   // NEW (STEP260) -- see struct doc comment above.
```

## 2. IO — `.sanmap` JSON leg

Per ARCH's binding wire-shape ruling: **flat PascalCase fields, no nested `Position`/`Rotation`
sub-object** — deliberately diverging from `ScenarioSpawnPoint`'s nested `Position`, so do not reuse
the existing `BuildPositionJson`/`ReadPositionJson` helpers (both nest). Compute the Z-flip inline
instead, same formula, same self-inverse guarantee.

### Exporter — `MapExporter_ScenarioRecord_IO.cpp`

New helper in the anonymous namespace, alongside `BuildAlloyOverridesJson`/`BuildAlloyRemovalsJson`:

```cpp
nlohmann::ordered_json BuildUnitPlacementsJson(const std::vector<Params::ScenarioUnitPlacement>& placements,
                                               int mapSize) {
    nlohmann::ordered_json array = nlohmann::ordered_json::array();
    for (const Params::ScenarioUnitPlacement& p : placements) {
        array.push_back({ { "ArmyName", p.armyName }, { "TemplateIdentifier", p.templateIdentifier },
                          { "PositionX", p.positionX }, { "PositionY", p.positionY },
                          { "PositionZ", static_cast<float>(mapSize) - p.positionZ - 1.0f },
                          { "RotationX", p.rotationX }, { "RotationY", p.rotationY },
                          { "RotationZ", p.rotationZ }, { "RotationW", p.rotationW } });
    }
    return array;
}
```

In `BuildScenarioRecordJson` (`:72-97`), add after `json["AuthoringNote"] = body.authoringNote;`:
```cpp
    json["UnitPlacements"] = BuildUnitPlacementsJson(body.unitPlacements, mapSize);
```
Always emitted, `[]` when empty — matches this record's "every field always present" convention.

### Importer — `MapImporter_ScenarioRecord_IO.cpp`

New helper alongside `ReadAlloyOverridesJson`:

```cpp
void ReadUnitPlacementsJson(const nlohmann::json& parent, const char* key,
                            std::vector<Params::ScenarioUnitPlacement>& outPlacements, int mapSize) {
    if (!parent.contains(key) || !parent[key].is_array()) return;
    outPlacements.clear();
    for (const nlohmann::json& entryJson : parent[key]) {
        if (!entryJson.is_object()) continue;
        Params::ScenarioUnitPlacement entry;
        ReadJsonText(entryJson, "ArmyName", entry.armyName);
        ReadJsonText(entryJson, "TemplateIdentifier", entry.templateIdentifier);
        ReadJsonFloat(entryJson, "PositionX", entry.positionX);
        ReadJsonFloat(entryJson, "PositionY", entry.positionY);
        float jsonZ = static_cast<float>(mapSize) - entry.positionZ - 1.0f;
        if (ReadJsonFloat(entryJson, "PositionZ", jsonZ))
            entry.positionZ = static_cast<float>(mapSize) - jsonZ - 1.0f;
        ReadJsonFloat(entryJson, "RotationX", entry.rotationX);
        ReadJsonFloat(entryJson, "RotationY", entry.rotationY);
        ReadJsonFloat(entryJson, "RotationZ", entry.rotationZ);
        ReadJsonFloat(entryJson, "RotationW", entry.rotationW);
        outPlacements.push_back(entry);
    }
}
```

In `ReadScenarioBodyJson` (`:111-137`), add after `ReadJsonText(json, "AuthoringNote", body.authoringNote);`:
```cpp
    ReadUnitPlacementsJson(json, "UnitPlacements", body.unitPlacements, mapSize);
```
Absent key (every pre-STEP260 `.sanmap`) leaves `unitPlacements` at its struct default, an empty
vector — never an error, same idiom as every other optional field in this function.

## 3. Versioning

None needed. Purely additive vector field; absent key → default-constructed empty vector, same
posture as `SpawnIds`/`slotRangeStart` before it. No `SanGenVersion` bump, no
`<Domain>_Migrate_V<N>_IO` unit, no `Sanmap_MigrationManifest_IO` touch.

## 4. Validation — new `ScenarioUnitPlacementValidation_IO.h/.cpp`

Model directly on `ScenarioSpawnIdValidation_IO.h`'s shape (report struct + one-wording
`SummaryText()`, pure/disk-free, never called from inside `BuildSanmapJsonText`):

- Non-empty `templateIdentifier` — violation reported, **never dropped** (warn-only).
- `armyName` resolves against the live roster — reuse the existing `ArmyIdentityTransformLookup`
  interface (already declared in `ScenarioSpawnIdValidation_IO.h`, do not invent a second one),
  warn-only if unresolvable — never a hard refusal (Constitution's never-abort-parsing posture).
- No duplicate/uniqueness check — placement rows are independent; two placements at the same or
  different position are both legal (multiple units spawn at once in the real reference file).
- Do **not** validate `templateIdentifier` against real sanpack/blueprint existence at this layer —
  mirrors the props `blueprintPath` lesson; an unresolved tpId must never abort parsing (the runtime
  already handles it gracefully via `pcall(CreateUnit, ...)`).

## 5. Tests

1. **PARAMS round-trip**: a `ScenarioUnitPlacement` with real, non-default values (armyName,
   templateIdentifier, position, non-identity rotation) round-trips through both IO legs unchanged.
2. **Legacy `.sanmap` compatibility**: a scenario record with no `"UnitPlacements"` key reads back as
   an empty vector — not an error, not a crash.
3. **Z-flip**: a placement authored at `positionZ = 10` with `mapSize = 100` exports
   `"PositionZ": 89`; importing that JSON with `mapSize = 100` recovers `positionZ == 10` exactly.
4. **Rotation round-trips verbatim**, no flip applied — test a non-identity quaternion (e.g.
   `{0, 0.707, 0, 0.707}`).
5. **Wire spelling**: `"UnitPlacements"` is an array of objects with `ArmyName`/`TemplateIdentifier`/
   `PositionX`/`PositionY`/`PositionZ`/`RotationX`/`RotationY`/`RotationZ`/`RotationW` keys, all always
   present (a placement with e.g. `rotationX == 0` still emits the key).
6. **Validation**: an unresolvable `armyName` produces a named, logged warning and does NOT drop the
   placement row; an empty `templateIdentifier` produces a named warning and does NOT drop the row.

## 6. Out of scope

- UI editor for this field — `STEP261`.
- Any foreign-`.lua` import path — `STEP262`-`STEP264`.
- `SANMAP_FORMAT_SPEC.md` documentation of this field and the rest of the (currently entirely
  undocumented) `Scenarios` object — route to the ARCH Expert in parallel (specs under
  `sangen_arch_pack/` are the ARCH Expert's sole-writer domain), not this ticket's job.
- Runtime Lua consumption of `unitPlacements` (`SanGenScenarioRuntime.lua` / a new
  `Scenario.SpawnBakedUnitPlacements`, named in `ARCH_15_14`) — a separate ticket if/when the
  category-4 generator export path needs to actually emit calls into it; flag to the Generator Expert,
  out of scope for this PARAMS/IO ticket, which only handles the `.sanmap` round-trip.

## 7. Files touched

**New:** `src/io/ScenarioUnitPlacementValidation_IO.h`, `src/io/ScenarioUnitPlacementValidation_IO.cpp`,
plus a new test file for it (mirror `ScenarioSpawnIdValidation_IO`'s own test file's shape).

**Modified:** `src/params/Scenario_PARAMS.h`, `src/io/MapExporter_ScenarioRecord_IO.cpp`,
`src/io/MapImporter_ScenarioRecord_IO.cpp`, plus additions to the existing
`MapExporter`/`MapImporter` `ScenarioRecord` round-trip test file(s) for the new field.
