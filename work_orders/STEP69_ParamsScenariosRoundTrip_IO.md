# STEP69 — `Params::Scenarios` + ordinary `.sanmap` round-trip (`Scenarios` section)

**Layer:** PARAMS + IO. **Domain:** ordinary per-domain `.sanmap` JSON section
(`Scenarios`), NOT the Lua-rendering leg. **Sequence:** Map Scenario IO track,
`work_orders/DESIGN_MapScenarioIO_R1.md` §6, **Work-Order 1 of 8 — foundational; every
later scenario work-order (WO5 `ScenarioScript_DataLua_IO` above all) consumes the type
this ticket ships.** No dependency on STEP63 (`LuaTableWriter_IO`)/STEP64
(`GameInstallLocation_IO`)/STEP65 (`LuaSyntaxCheck_SYS`) — parallel with all three. No
dependency on STEP60 or any marker PARAMS work (verified below).

## Root problem
`ARCH_15_05_ParamsScenariosType.md` §15.5 ratifies the `Params::Scenarios` C++ shape and §15.7 assigns its
`.sanmap` JSON persistence to the Format Expert, who has delivered it as
`SANMAP_FORMAT_SPEC.md` Correction 17 (`Scenarios`, ~line 883). Neither the type nor its
IO exists in the tree yet — confirmed: `src/params/` has no `Scenario*` file, no
`MapExporter_Scenarios_IO`/`MapImporter_Scenarios_IO` exist, and `MapRecipe` has no
`scenarios` member. `DESIGN_MapScenarioIO_R1.md` §0 identified this leg explicitly as
**an ordinary domain** — full reuse of `MapExporter_<Domain>_IO`/`MapImporter_<Domain>_IO`
+ `JsonPrimitives_IO`, normal future `<Domain>_Migrate_V<N>_IO` candidacy — structurally
nothing like the Lua-rendering leg (WO5/`ScenarioScript_*_IO`). **Note:** the DESIGN
doc's own file table (§1) still says `MapExporter_MapScenario_IO`/`Params::MapScenario`
— that predates ARCH_15_05_ParamsScenariosType.md §15.5's later ratification of the type as `Params::Scenarios`
(also Correction 17's own file-name references, `MapImporter_Scenarios_IO`). This
ticket follows **`ARCH_15_05_ParamsScenariosType.md` §15.5/Correction 17 verbatim** (the newer, binding naming);
the design doc's terminology is superseded on this one point, not reopened otherwise.

## Scope
1. New `src/params/Scenario_PARAMS.h` — `ARCH_15_05_ParamsScenariosType.md` §15.5's type block, verbatim.
2. `MapRecipe::scenarios` — a flat sibling of `armies`/`areas`/etc. (ARCH_15_05_ParamsScenariosType.md §15.5's own
   instruction), wired into `BuildSanmapJsonText`/`ParseSanmapJsonText`.
3. `MapExporter_Scenarios_IO.cpp` / `MapImporter_Scenarios_IO.cpp` — the ordinary
   per-domain pair, composing `JsonPrimitives_IO`.
4. One new shared primitive (`JsonPrimitives_IO.h`) needed by this domain (see §4).
5. Test coverage, including the load-bearing `CountScenarios` order-preservation test.

**Out of scope** (do not build, do not stub beyond what §2 requires):
- The Lua-rendering leg (`ScenarioScript_DataLua_IO`) — WO5.
- `ScenarioScript_RuntimeResource_IO`, `GameInstallLocation_IO` — WO6/STEP64.
- `ScenarioScript_Export_IO` (the export orchestrator) — WO7.
- All UI (Files tab wiring, `DraggableList` authoring surface for `countScenarios`,
  the ImGuiColorTextEdit editor) — WO8, UI Expert's ticket.
- Any `<Domain>_Migrate_V<N>_IO` file — none is needed (§10) and none may be added
  reflexively.

## 1. `src/params/Scenario_PARAMS.h` (NEW)

`ARCH_15_05_ParamsScenariosType.md` §15.5's block, verbatim, in the `SanmapGen::Params` namespace, with the
needed includes (`<cstdint>` for `ScenarioNavalPondSide : int8_t`, `<string>`,
`<vector>`, and `"MapArea_PARAMS.h"` for `ScenarioBody::area`'s `Params::MapArea`
reuse). Header comment states: source of truth is `ARCH_15_05_ParamsScenariosType.md` §15.5, this file is a
verbatim transcription, not a reinterpretation — any future shape change is an ARCH
ratification first, this file second.

⚠️ **`Scenarios` carries a fourth member beyond §15.5's original three — `ARCH_15_10_SlotPatternConstructionMoves.md` §15.10
amends the shape:**
```cpp
struct Scenarios {
    std::vector<PatternScenario> patternScenarios;   // TIER 1
    std::vector<CountScenario>   countScenarios;     // TIER 2 — ORDER IS LOAD-BEARING, §15.6
    ScenarioBody                 defaultScenario;    // TIER 3 — always matches, exactly one
    int                          maxArmySlotCount = 16;  // §15.10 — slotPattern string length;
                                                         // rendered as the Lua global
                                                         // MAX_ARMY_SLOT_COUNT (STEP70)
};
```
Top-level and map-wide, **not** per-scenario — TIER 1 exact-match requires every authored
pattern in a map to share one length. Default `16`, matching the live reference (deliberately
the map's real ceiling, not the lobby UI's currently-exposed 8). Read §15.5 **and** §15.10
together; §15.5 alone is stale on this point.

⚠️ **`ScenarioBody::area` reuses `Params::MapArea` wholesale** (per ARCH_15_05_ParamsScenariosType.md §15.5), but
`MapArea::name` has no counterpart in Correction 17's `Area` JSON object (`{x,y,width,
height}` only — no `name` key inside `Area`; the record's own `Name` field is a
sibling, not nested). The importer/exporter must leave `area.name` empty/unused when
building/reading `ScenarioBody.area` — never populate it from `ScenarioBody.name`,
never emit it into the `Area` JSON object. Note this in the `.h`'s own comment on that
member so a future reader doesn't "fix" what looks like an unset field.

## 2. `MapRecipe_PARAMS.h` (EDIT)
- `#include "Scenario_PARAMS.h"` alongside the other includes (alphabetical slot, after
  `"ScatterRule_PARAMS.h"`).
- New member, placed with `armies`/`areas`/`markers`/`chains` since it is
  human-authored, pass-through data like they are:
  ```cpp
  Params::Scenarios scenarios;
  ```

## 3. Field-for-field parity check — CONFIRMED, no gap

Cross-checked `ARCH_15_05_ParamsScenariosType.md` §15.5's struct block against `SANMAP_FORMAT_SPEC.md`
Correction 17's JSON shape, member by member:

| C++ (`ScenarioBody`) | JSON (`<ScenarioRecord>`) |
|---|---|
| `name` | `Name` |
| `area` (`Params::MapArea`) | `Area` (`{x,y,width,height}`) |
| `navy` | `Navy` |
| `alloyMode` | `AlloyMode` |
| `spawns` | `Spawns` |
| `alloys` | `Alloys` |
| `alloysToAdd` | `AlloysToAdd` |
| `alloysToRemove` | `AlloysToRemove` |
| `authoringNote` | `AuthoringNote` |
| `navalFleet` (`fleet`/`pondSideByArmy`/`sideBiasDistance`) | `NavalFleet` (`Fleet`/`PondSideByArmy`/`SideBiasDistance`) |

10 fields each side, exact 1:1. Same result for `PatternScenario`+`Pattern`,
`CountScenario`+`Conditions`, `ScenarioCountCondition`↔`{Field,Comparator,Value}`,
`ScenarioSpawn`↔`{ArmyName,Position}`, `ScenarioAlloyOverride`↔`{ArmyName,MarkerName,
Position}`, `ScenarioAlloyRemoval`↔`{ArmyName,MarkerName}`,
`ScenarioNavalFleetEntry`↔`{TemplateIdentifier,Count}`,
`ScenarioNavalPondAssignment`↔`{ArmyName,Side}`, `Scenarios`↔`{PatternScenarios,
CountScenarios,DefaultScenario}`. **No ⚠️ to raise** — the two sides were designed
together (ARCH_15_07_OwnershipSplit.md §15.7) and it shows.

## 4. New shared primitive: `ReadJsonEnumerationText` (`JsonPrimitives_IO.h`, EDIT)

**Gap found:** every existing enum in `src/io/` is stored as a JSON **integer**
(`ReadJsonEnumeration(parent, key, valueCount, destination)`, fenced `[0,valueCount)`)
— confirmed by grep across `src/io/`, zero counter-examples. Correction 17 requires
**string-spelled** enums in three places within this one domain alone (`AlloyMode`,
`Field`, `Comparator`) — no existing primitive reads a string into a fenced enum index.
Given it recurs 3× inside this single ticket, add one new primitive, sibling to
`ReadJsonEnumeration`:

```cpp
// Reads a STRING-valued enum (unlike ReadJsonEnumeration's integer-valued one — the
// first string-spelled enum anywhere in src/io/, needed by Correction 17's
// AlloyMode/Field/Comparator). `spellings[0..spellingCount)` are the exact candidate
// strings, index order IS the enum's integer value on a match. Unrecognized text is
// treated the same as an absent key: returns false, `destination` untouched — the
// caller's own pre-loaded current/default value survives, exactly ReadJsonEnumeration's
// posture on out-of-range.
inline bool ReadJsonEnumerationText(const nlohmann::json& parent, const char* key,
                                     const char* const* spellings, int spellingCount,
                                     int& destination) {
    std::string text;
    if (!ReadJsonText(parent, key, text)) return false;
    for (int index = 0; index < spellingCount; ++index) {
        if (text == spellings[index]) { destination = index; return true; }
    }
    return false;
}
```

Total, idempotent, matches the file's existing style. Callers use the same
`int enumerationValue = static_cast<int>(current); if (ReadJsonEnumerationText(...))
current = static_cast<EnumType>(enumerationValue);` idiom `ReadArmyJson`/
`ReadMarkerRuleJson` already use for the integer version.

**`ScenarioNavalPondSide` is a separate case — do NOT use this primitive for `Side`.**
It is a raw signed int (`-1`/`1`), not a 0-based contiguous index (`West=-1,East=1`,
ARCH_15_05_ParamsScenariosType.md §15.5's deliberate non-enumeration convention, matching the live Lua reference).
Read it with a small domain-local helper in `MapImporter_Scenarios_IO.cpp`:
`ReadJsonInteger` the raw value, accept only exactly `-1` or `1`, else leave the field
at its default (`East`) — never `ReadJsonEnumeration`/`ReadJsonEnumerationText`.

## 5. Enum ↔ JSON string tables (exact spellings, per Correction 17)

Domain-local `constexpr const char*` arrays in both `MapExporter_Scenarios_IO.cpp` and
`MapImporter_Scenarios_IO.cpp` (mirrors `markerCategoryCount`-style local constants —
each file owns its own copy, matching existing per-domain precedent):

```cpp
// Index == the C++ enum's own declaration order (`ARCH_15_05_ParamsScenariosType.md` §15.5) — do not reorder.
constexpr const char* kScenarioAlloyModeSpellings[4]    = { "explicit", "occupancy", "keepAll", "delta" };
constexpr int         kScenarioAlloyModeCount           = 4;

constexpr const char* kScenarioCountFieldSpellings[3]   = { "Total", "HumanCount", "AiCount" };
constexpr int         kScenarioCountFieldCount          = 3;

constexpr const char* kScenarioComparatorSpellings[6]   =
    { "Equal", "NotEqual", "GreaterThan", "GreaterOrEqual", "LessThan", "LessOrEqual" };
constexpr int         kScenarioComparatorCount          = 6;
```

## 6. `MapExporter_Scenarios_IO.cpp` (NEW) + `MapExporter_Recipe_IO.h` (EDIT)

New free function, declared in `MapExporter_Recipe_IO.h` alongside the other
`Build*Json` declarations (comment block referencing Correction 17 the way
`BuildPropsJson`'s references Correction 14):
```cpp
nlohmann::ordered_json BuildScenariosJson(const Params::MapRecipe& recipe);
```

Structure (mirrors `BuildArmiesJson`'s layered-builder shape — a private
`BuildScenarioRecordJson(const Params::ScenarioBody&) -> ordered_json` composed by
three call sites: one per `patternScenarios` entry adding `"Pattern"`, one per
`countScenarios` entry adding `"Conditions"` as an array, one direct call for
`defaultScenario`):

- Emit `<ScenarioRecord>` fields in Correction 17's own listed order (`Name`, `Area`,
  `Navy`, `AlloyMode`, `Spawns`, `Alloys`, `AlloysToAdd`, `AlloysToRemove`,
  `AuthoringNote`, `NavalFleet`) — `nlohmann::ordered_json` already in use throughout
  this layer, so insertion order is write order.
- `Area`: 4 `float` assigns (`x=area.originX, y=area.originZ, width=area.width,
  height=area.length`) — the exact mapping `BuildAreasJson` already uses; do not
  introduce a shared helper for 4 lines used in two places (matches this codebase's
  "promote only on a third use" discipline).
- `Position` (on `Spawns`/`Alloys`/`AlloysToAdd`): `{x,y,z}` from `positionX/Y/Z` —
  ⚠️ **coordinate-flip question, see §9.**
- `Spawns`/`Alloys`/`AlloysToAdd`/`AlloysToRemove`: plain arrays, one object per vector
  element, order preserved (not load-bearing per spec, but free with a `for` loop).
- `NavalFleet`: always emitted (even when `navy == false` — Correction 17's worked
  example shows non-navy scenarios still writing
  `"NavalFleet": {"Fleet": [], "PondSideByArmy": [], "SideBiasDistance": 90.0}`).
- **`CountScenarios` MUST be built as `nlohmann::ordered_json::array()`, iterated in
  `recipe.scenarios.countScenarios`'s own vector order** — the load-bearing property;
  do not route through any container that could reorder (no `std::map`/
  `std::unordered_map` staging).
- Top level: `document["Scenarios"] = BuildScenariosJson(recipe);` added to
  `AppendEntityDomainsJson` (`MapExporter_DocumentAssembly_IO.cpp`), as a new line
  after `document["DecalGroups"] = BuildDecalGroupsJson(recipe);` — same tier; extend
  that function's header comment key list to include `Scenarios`.

## 7. `MapImporter_Scenarios_IO.cpp` (NEW) + `MapImporter_Recipe_IO.h` (EDIT)

```cpp
void ReadScenariosJson(const nlohmann::json& document, Params::MapRecipe& outRecipe,
                       MapImportResult& result);
```
(Needs `MapImportResult&` — unlike `ReadAreasJson`/`ReadArmiesJson` — because the
malformed-array case requires `result.Warn(...)`, matching `ReadPropsJson`/
`ReadStratumLayersJson`'s existing signature shape.)

- Absent `"Scenarios"` key or not an object → return immediately, `outRecipe.scenarios`
  stays default-constructed `Params::Scenarios{}` (empty vectors, zero-valued
  `defaultScenario` whose `alloyMode` is still `Occupancy`, and `maxArmySlotCount == 16`,
  all via their default-initializers).
  **Never an error, never a fabricated entry** — Correction 17's contract, Constitution §6.
- `MaxArmySlotCount` absent → stays at the struct default `16` (`ARCH_15_10_SlotPatternConstructionMoves.md` §15.10). Read it
  with `ReadJsonInteger` into the pre-loaded current value, same idiom as every other scalar.
  **Do not clamp, do not validate against the army roster here** — the export-time warning
  for `maxArmySlotCount < authored army count` is STEP70's job (`ARCH_15_10_SlotPatternConstructionMoves.md` §15.10 point 2),
  not the importer's; a `.sanmap` authored elsewhere must round-trip whatever it carries.
- `PatternScenarios`/`CountScenarios` present but **not a JSON array** (e.g. an object):
  **do not silently coerce, do not iterate object keys as if ordered.**
  ```cpp
  result.Warn("Scenarios.CountScenarios is present but not a JSON array; TIER 2 match "
              "priority depends on array order, which an object has none of. Treated as "
              "empty rather than guessing an order.");
  ```
  Same message shape for `PatternScenarios`, minus the ordering clause (ordering isn't
  load-bearing there — still warn, a non-array is still malformed). Either way: treat
  as empty, never a hard refusal (Constitution §6 — authored recipe data, not a version
  gate).
- `CountScenarios` **present and a valid array** → read element-by-element,
  `push_back` in document order, following `ReadRuleArray`'s discipline
  (`MapImporter_ScatterTransform_IO.h`) — reuse that template if its shape fits; if it
  doesn't cleanly fit, a domain-local loop is fine, but **do** preserve its
  "silent degrade-per-entry, never abort-the-array" contract either way.
- `AlloyMode` absent from a `<ScenarioRecord>` → `ReadJsonEnumerationText` returns
  `false`, `body.alloyMode` stays at whatever it was pre-loaded with — pre-load from
  `Params::ScenarioAlloyMode::Occupancy` (the struct default) before the read call,
  exactly `ReadArmyJson`'s `int factionValue = static_cast<int>(army.faction);` idiom.
  **No separate "if absent, set occupancy" branch needed.**
- `Side` (`ScenarioNavalPondAssignment`): per §4, raw-int read validated against
  `{-1,1}`; unrecognized/missing → `ScenarioNavalPondSide::East` (struct default).
- Wire into `MapImporter_ParseDocument_IO.cpp`'s `ParseEntityDomainsJson` (already takes
  `MapImportResult& result` — no signature change):
  ```cpp
  ReadScenariosJson(document, outRecipe, result);
  ```
  placed after `ReadStratumGenerationSettingsJson(document, outRecipe, result);`.

## 8. `Sanmap_KnownTopLevelKeys_IO.cpp` (EDIT) — load-bearing, do not skip

Add `"Scenarios"` to the `(a)` bucket (top-level keys read directly, unconditionally)
in `KnownTopLevelSanmapKeys()`. Correction 17 is explicit: **"Once this Correction
lands, `Scenarios` becomes one of `SANMAP_FORMAT_SPEC`'s current sections... and is
therefore consumed by its own dedicated `MapImporter_Scenarios_IO` reader, not captured
by the `UnknownImport` passthrough any longer."** Skipping this edit would double-bag
`Scenarios` into `UnknownImport` on every import even after the real reader exists — a
real, silent-looking bug this file's header comment exists specifically to prevent.

## 9. ⚠️ Open item — coordinate flip on `Position.z`, not settled by ARCH/Correction 17

`UnitTransform`/`MarkerTransform`/`PropTransform`/`DecalTransform.positionZ` all apply
`world.z = mapSize - positionZ - 1` on export — `MapArea` is the one documented
**exception** (ENTITY_AUTHORING_PARAMS_SPEC finding 3: no flip). Correction 17 says
`ScenarioSpawn`/`ScenarioAlloyOverride`'s `Position` "reuses the existing
`InstancedTransform.position` `Vector3` shape verbatim" — that confirms the **on-disk
shape** but does not state whether the **flip convention** travels with it.

**RULED BY THE HUMAN, 2026-08-21: implement WITH the flip** (`world.z = mapSize -
positionZ - 1`), consistent with every other `InstancedTransform`-shaped position field
(Armies/Markers/Props/Decals all flip; `MapArea` is a rect, not a point, and is the
named exception). **Do not block on further confirmation — build it.**

**Required: leave an attention comment at BOTH coordinate call sites** (the export
`Position` write in `MapExporter_Scenarios_IO.cpp` and the matching import read in
`MapImporter_Scenarios_IO.cpp`), worded so a future reader can find it by grep, e.g.:

```cpp
// ⚠️ ATTENTION — COORDINATE FLIP UNCONFIRMED FOR SCENARIOS.
// Applies the same `mapSize - z - 1` flip every other InstancedTransform-shaped
// position field uses (Armies/Markers/Props/Decals). NOT independently ratified for
// the Scenarios domain by ARCH_15_05_ParamsScenariosType.md §15.5 or SANMAP_FORMAT_SPEC Correction 17 — chosen for
// consistency, per the human's 2026-08-21 ruling to implement now and verify later.
// IF SCENARIO SPAWNS/ALLOYS APPEAR MIRRORED ALONG Z IN-GAME, THIS IS THE FIRST PLACE
// TO LOOK: removing the flip here and at the matching import/export call site is the
// entire fix. Round-trip tests pass either way (export and import are inverses), so
// the test suite will NOT catch a wrong choice — only in-game verification will.
```

Nothing else in this ticket depends on the choice; the two call sites are the entire
blast radius.

## 10. `SanGenVersion` — no bump, confirmed

Purely additive top-level section, same precedent as Corrections 12/14
(`StratumGenerationSettings`/`PropGroups`/`DecalGroups`) — both landed after
`SanGenVersion = 3` without a further bump. **No `<Domain>_Migrate_V<N>_IO` file, no
manifest edit, no runner change.** A coder must not create `Scenarios_Migrate_V3_IO`
"to be safe" — there is nothing V3-shaped this section reshapes; it does not exist in
any pre-Correction-17 document at all.

## Files touched
- NEW `src/params/Scenario_PARAMS.h`
- EDIT `src/params/MapRecipe_PARAMS.h` (include + `scenarios` member)
- NEW `src/io/MapExporter_Scenarios_IO.cpp`
- EDIT `src/io/MapExporter_Recipe_IO.h` (`BuildScenariosJson` declaration)
- EDIT `src/io/MapExporter_DocumentAssembly_IO.cpp` (`AppendEntityDomainsJson` — one new
  line + comment-list extension)
- NEW `src/io/MapImporter_Scenarios_IO.cpp`
- EDIT `src/io/MapImporter_Recipe_IO.h` (`ReadScenariosJson` declaration)
- EDIT `src/io/MapImporter_ParseDocument_IO.cpp` (`ParseEntityDomainsJson` — one new line)
- EDIT `src/io/Sanmap_KnownTopLevelKeys_IO.cpp` (add `"Scenarios"`)
- EDIT `src/io/JsonPrimitives_IO.h` (add `ReadJsonEnumerationText`)
- NEW `src/io/MapImporter_Scenarios_IO_Test.cpp`
- EDIT `CMakeLists.txt` (one new test registration)

`src/params/*.h`/`src/io/*.cpp`/`*.h` are all `GLOB_RECURSE`'d into `SANGEN_V2_SOURCES`
(`CMakeLists.txt:142-150`) — the new `.cpp`/`.h` need no explicit source-list entry,
only the test target does.

## Backend policy
N/A — pure CPU-side JSON build/parse, called at most once per export/import (not
per-frame). No compute dispatch, no SIMD, no GPU handle; does not touch `Dispatch_SYS`.

## Acceptance test — NEW `src/io/MapImporter_Scenarios_IO_Test.cpp`

Mirrors `MapImporter_PropsDecals_IO_Test.cpp`'s posture: calls `BuildScenariosJson`/
`ReadScenariosJson` directly against hand-built fixtures, **plus** one live-document
check through `MapExporter::BuildSanmapJsonText`/`MapImporter::ParseSanmapJsonText`
(proving the assembly/parse wiring, since this ticket live-wires on day one).

1. **⚠️ Load-bearing: `CountScenarios` order round-trips exactly.** Fixture with 3
   `CountScenario` entries named `"Zulu"`, `"Alpha"`, `"Mike"` (deliberately
   non-alphabetical) — after build → parse, assert
   `countScenarios[0].body.name == "Zulu"`, `[1] == "Alpha"`, `[2] == "Mike"` — exact
   index match, not just "all three present."
2. **Absent `Scenarios` key** → `outRecipe.scenarios` at `Params::Scenarios{}` defaults:
   empty `patternScenarios`/`countScenarios`, `defaultScenario.alloyMode ==
   ScenarioAlloyMode::Occupancy`. Zero `result.warningCount` (absence is not a warning).
3. **`CountScenarios` present but a JSON object, not array** → `countScenarios` empty
   AND `result.warningCount >= 1` AND the logged message names ordering (substring
   check for `"order"`) — proves it's the specific ordering-hazard warning. Same for
   `PatternScenarios` as an object.
4. **`AlloyMode` absent** → reads back as `ScenarioAlloyMode::Occupancy`, zero warnings
   for that cause.
5. **Full field round trip, one of each tier**: one `PatternScenario` (with
   `slotPattern`), one `CountScenario` with 3 AND'd conditions spanning all 3
   `ScenarioCountField` values and ≥3 distinct `ScenarioComparator` values, one
   `DefaultScenario` — each with non-empty `spawns`/`alloys`/`alloysToAdd`/
   `alloysToRemove`, non-empty `authoringNote`, `navy = true` with a non-empty
   `navalFleet` (`fleet` 2 entries, `pondSideByArmy` 1 entry incl. a `West` side,
   non-default `sideBiasDistance`). Build → parse → assert every field equals the
   fixture exactly (floats via `NearlyEqual`).
6. **Empty `countScenarios`/`patternScenarios` still serialize as `[]`**, not omitted or
   `{}` — inspect the raw `ordered_json` (`.is_array() && .empty()`).
7. **Live-document integration**: `BuildSanmapJsonText` on a fixture recipe with
   populated `scenarios`, then `ParseSanmapJsonText` on the result — assert the
   round-tripped `scenarios` matches, proving the wiring is live, not just the pure
   builder/reader pair in isolation.
8. Full solo rebuild + `ctest -C Debug`: previously-passing suite stays green, zero
   pre-existing test files edited.

`CMakeLists.txt` addition (near `PropsDecals_IO_Test`, ~line 486-487):
```cmake
add_sangen_test(Scenarios_IO_Test src/io/MapImporter_Scenarios_IO_Test.cpp)
target_link_libraries(Scenarios_IO_Test PRIVATE nlohmann_json::nlohmann_json)
```

## Verify
- New `src/io/MapImporter_Scenarios_IO_Test.cpp` passes, registered as `Scenarios_IO_Test`.
- Full solo rebuild + `ctest -C Debug`: 100% pass, zero pre-existing test files edited
  or broken.
- Confirm `Sanmap_KnownTopLevelKeys_IO.cpp` now includes `"Scenarios"` (grep-checkable)
  — the load-bearing wiring edit most likely to be silently skipped.
