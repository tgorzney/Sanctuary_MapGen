# PHASE_A — Scenario Data Migration: Pandemonium Isthmus

**Revision 2 (2026-09-04)** — updated after two ARCH ratifications landed since revision 1:
`ARCH_15_05`'s `AMENDED 2026-09-04` (`SlotRangeOccupiedCount`, closes the `slots5to8AnyFilled` gap
revision 1 had to exclude) and `ARCH_15_12`/`ARCH_15_13` (the `ScenarioSpawnPoint` shared pool,
which **retires** the old `Spawns` wire shape this document originally used — a breaking change,
ratified with no migration shim). Nothing below was written to any `.sanmap`, game install path,
or SanGen source file; this is still content/data, hand-transcribed under the sanctioned §14
migration path.

## 0. What this is

A **hand-transcribed** migration of the live reference file
`LJ/lua/maps/Pandemonium Isthmus/Pandemonium Isthmus_Scenarios_Script.lua` into
`Params::Scenarios` JSON shape, produced under the sanctioned "one-time human migration"
path (`MAP_SCENARIO_SPEC.md` §14). This is **not** SanGen-generated and **not** read back
from Lua by any code path — every value below was read from the Lua source text and
hand-transcribed against the real C++/IO field names in `src/params/Scenario_PARAMS.h`,
`src/io/MapImporter_ScenarioRecord_IO.cpp`, `src/io/MapExporter_Scenarios_IO.cpp`, and the
`ARCH_15_12`/`ARCH_15_05` amendment texts for the two shapes revision 1 didn't have yet.

**All 8 live scenarios are now covered** (revision 1 could only cover 7 — `slots5to8AnyFilled`
was a documented, un-fakeable gap until `SlotRangeOccupiedCount` was ratified).

## 1. `Params::Scenarios` JSON — paste under the `.sanmap`'s top-level `"Scenarios"` key

```json
{
  "PatternScenarios": [],
  "SpawnPoints": [
    { "SpawnId": "Army01_NorthSpawn", "ArmyName": "ARMY_01", "Position": { "x": 855, "y": 79.12979888916016, "z": 920 } },
    { "SpawnId": "Army02_NorthSpawn", "ArmyName": "ARMY_02", "Position": { "x": 1193, "y": 79.12979888916016, "z": 1128 } },
    { "SpawnId": "Army03_FourPlayerSpawn", "ArmyName": "ARMY_03", "Position": { "x": 833, "y": 80.794922, "z": 857 } },
    { "SpawnId": "Army04_FourPlayerSpawn", "ArmyName": "ARMY_04", "Position": { "x": 1215, "y": 80.759766, "z": 1191 } }
  ],
  "CountScenarios": [
    {
      "Name": "slots5to8AnyFilled",
      "Area": { "x": 0, "y": 0, "width": 2048, "height": 2048 },
      "AreaName": "",
      "SpawnsUnits": true,
      "AlloyMode": "occupancy",
      "SpawnIds": [],
      "Alloys": [],
      "AlloysToAdd": [],
      "AlloysToRemove": [],
      "AuthoringNote": "PHASE_A migration rev.2 -- previously EXCLUDED (rev.1) as a documented, un-fakeable gap; now natively authorable via ARCH_15_05's 2026-09-04 SlotRangeOccupiedCount amendment. Hand-transcribed from the live Lua 'name=\"slots5to8AnyFilled\"' entry: 'pattern:sub(5,8):find(\"[^-]\") ~= nil' becomes SlotRangeOccupiedCount >= 1 over slots 5-8. This is the ONE entry that opts into unit spawning (SpawnsUnits=true) -- per MAP_SCENARIO_SPEC.md §11, that alone spawns nothing without STEP251's category-4 generator file also existing and being filled in. MUST stay first in this array (ARCH_15_06 first-match-wins + MAP_SCENARIO_SPEC.md §5.1: an identity-based rule must be authored above every aggregate-count rule it could otherwise be shadowed by -- this is exactly the live reference's own ordering discipline, preserved here.",
      "Conditions": [
        { "Field": "SlotRangeOccupiedCount", "Comparator": "GreaterOrEqual", "Value": 1, "SlotRangeStart": 5, "SlotRangeEnd": 8 }
      ]
    },
    {
      "Name": "1v1",
      "Area": { "x": 846, "y": 846, "width": 356, "height": 356 },
      "AreaName": "",
      "SpawnsUnits": false,
      "AlloyMode": "explicit",
      "SpawnIds": [ "Army01_NorthSpawn", "Army02_NorthSpawn" ],
      "Alloys": [
        { "ArmyName": "ARMY_01", "MarkerName": "AlloyMarker_219", "Position": { "x": 857, "y": 78.72360229492188, "z": 911 } },
        { "ArmyName": "ARMY_01", "MarkerName": "AlloyMarker_237", "Position": { "x": 869, "y": 78.72360229492188, "z": 919 } },
        { "ArmyName": "ARMY_01", "MarkerName": "AlloyMarker_97",  "Position": { "x": 873, "y": 78.72360229492188, "z": 923 } },
        { "ArmyName": "ARMY_02", "MarkerName": "AlloyMarker_220", "Position": { "x": 1191, "y": 78.72360229492188, "z": 1137 } },
        { "ArmyName": "ARMY_02", "MarkerName": "AlloyMarker_240", "Position": { "x": 1179, "y": 78.72360229492188, "z": 1129 } },
        { "ArmyName": "ARMY_02", "MarkerName": "AlloyMarker_99",  "Position": { "x": 1175, "y": 78.72360229492188, "z": 1125 } }
      ],
      "AlloysToAdd": [],
      "AlloysToRemove": [],
      "AuthoringNote": "PHASE_A migration rev.2 (MAP_SCENARIO_SPEC.md §14, hand-transcribed, not Lua-parsed) from the live Pandemonium Isthmus_Scenarios_Script.lua COUNT_SCENARIOS entry 'name=\"1v1\"'. SpawnIds reference the shared pool above (ARCH_15_12) instead of inlining ARMY_01/02's position a second time -- this is the exact duplication ARCH_15_12 was ratified to close, since '4human'/'1h3ai' below reuse the identical two positions verbatim in the live Lua.",
      "Conditions": [
        { "Field": "Total", "Comparator": "Equal", "Value": 2, "SlotRangeStart": 1, "SlotRangeEnd": 1 }
      ]
    },
    {
      "Name": "2h1ai",
      "Area": { "x": 668.4444444444445, "y": 824, "width": 711.1111111111111, "height": 400 },
      "AreaName": "",
      "SpawnsUnits": false,
      "AlloyMode": "keepAll",
      "SpawnIds": [],
      "Alloys": [],
      "AlloysToAdd": [],
      "AlloysToRemove": [],
      "AuthoringNote": "PHASE_A migration rev.2, hand-transcribed from the live Lua 'name=\"2h1ai\"' entry. No explicit SpawnIds in the source; alloyMode=keepAll, so per MAP_SCENARIO_SPEC.md §8 an omitted spawn override is documented-acceptable, intentional inheritance of the .sanmap's single baked ARMY_XX Spawn transform. NOTE: §8 also documents a real, previously-observed regression where this exact composition (2 human + 1 AI) matched correctly but spawned players at 6-player positions because this scenario lacked an override -- verify a live 3-player playtest lands armies inside this AREA_169 rect before shipping, do not assume the baked default is correct untested.",
      "Conditions": [
        { "Field": "HumanCount", "Comparator": "Equal", "Value": 2, "SlotRangeStart": 1, "SlotRangeEnd": 1 },
        { "Field": "AiCount", "Comparator": "Equal", "Value": 1, "SlotRangeStart": 1, "SlotRangeEnd": 1 },
        { "Field": "Total", "Comparator": "Equal", "Value": 3, "SlotRangeStart": 1, "SlotRangeEnd": 1 }
      ]
    },
    {
      "Name": "4human",
      "Area": { "x": 668.4444444444445, "y": 824, "width": 711.1111111111111, "height": 400 },
      "AreaName": "",
      "SpawnsUnits": false,
      "AlloyMode": "explicit",
      "SpawnIds": [ "Army01_NorthSpawn", "Army02_NorthSpawn", "Army03_FourPlayerSpawn", "Army04_FourPlayerSpawn" ],
      "Alloys": [
        { "ArmyName": "ARMY_01", "MarkerName": "AlloyMarker_219", "Position": { "x": 857, "y": 78.72360229492188, "z": 911 } },
        { "ArmyName": "ARMY_01", "MarkerName": "AlloyMarker_237", "Position": { "x": 869, "y": 78.72360229492188, "z": 919 } },
        { "ArmyName": "ARMY_01", "MarkerName": "AlloyMarker_97",  "Position": { "x": 873, "y": 78.72360229492188, "z": 923 } },
        { "ArmyName": "ARMY_02", "MarkerName": "AlloyMarker_220", "Position": { "x": 1191, "y": 78.72360229492188, "z": 1137 } },
        { "ArmyName": "ARMY_02", "MarkerName": "AlloyMarker_240", "Position": { "x": 1179, "y": 78.72360229492188, "z": 1129 } },
        { "ArmyName": "ARMY_02", "MarkerName": "AlloyMarker_99",  "Position": { "x": 1175, "y": 78.72360229492188, "z": 1125 } },
        { "ArmyName": "ARMY_03", "MarkerName": "AlloyMarker_282", "Position": { "x": 823, "y": 80.744141, "z": 848 } },
        { "ArmyName": "ARMY_03", "MarkerName": "AlloyMarker_283", "Position": { "x": 833, "y": 80.759766, "z": 856 } },
        { "ArmyName": "ARMY_03", "MarkerName": "AlloyMarker_284", "Position": { "x": 835, "y": 80.681641, "z": 860 } },
        { "ArmyName": "ARMY_04", "MarkerName": "AlloyMarker_285", "Position": { "x": 1225, "y": 80.712891, "z": 1200 } },
        { "ArmyName": "ARMY_04", "MarkerName": "AlloyMarker_286", "Position": { "x": 1215, "y": 80.771484, "z": 1192 } },
        { "ArmyName": "ARMY_04", "MarkerName": "AlloyMarker_287", "Position": { "x": 1213, "y": 80.712891, "z": 1188 } }
      ],
      "AlloysToAdd": [],
      "AlloysToRemove": [],
      "AuthoringNote": "PHASE_A migration rev.2, hand-transcribed from the live Lua 'name=\"4human\"' entry. The live Lua entry still carries a dead `navy = true` field (ARCH_15_05_ParamsScenariosType.md, RETIRED 2026-08-28) -- deliberately NOT carried into this record; SpawnsUnits below is the false struct default, matching the source's own absence of `spawnsUnits`. SpawnIds reuse the same pool entries as '1v1' (ARMY_01/02) plus two new ones (ARMY_03/04) -- '1h3ai' below reuses all four verbatim, which is exactly the cross-scenario duplication ARCH_15_12 exists to eliminate.",
      "Conditions": [
        { "Field": "Total", "Comparator": "Equal", "Value": 4, "SlotRangeStart": 1, "SlotRangeEnd": 1 },
        { "Field": "HumanCount", "Comparator": "Equal", "Value": 4, "SlotRangeStart": 1, "SlotRangeEnd": 1 }
      ]
    },
    {
      "Name": "1h3ai",
      "Area": { "x": 668.4444444444445, "y": 824, "width": 711.1111111111111, "height": 400 },
      "AreaName": "",
      "SpawnsUnits": false,
      "AlloyMode": "explicit",
      "SpawnIds": [ "Army01_NorthSpawn", "Army02_NorthSpawn", "Army03_FourPlayerSpawn", "Army04_FourPlayerSpawn" ],
      "Alloys": [
        { "ArmyName": "ARMY_01", "MarkerName": "AlloyMarker_219", "Position": { "x": 857, "y": 78.72360229492188, "z": 911 } },
        { "ArmyName": "ARMY_01", "MarkerName": "AlloyMarker_237", "Position": { "x": 869, "y": 78.72360229492188, "z": 919 } },
        { "ArmyName": "ARMY_01", "MarkerName": "AlloyMarker_97",  "Position": { "x": 873, "y": 78.72360229492188, "z": 923 } },
        { "ArmyName": "ARMY_02", "MarkerName": "AlloyMarker_220", "Position": { "x": 1191, "y": 78.72360229492188, "z": 1137 } },
        { "ArmyName": "ARMY_02", "MarkerName": "AlloyMarker_240", "Position": { "x": 1179, "y": 78.72360229492188, "z": 1129 } },
        { "ArmyName": "ARMY_02", "MarkerName": "AlloyMarker_99",  "Position": { "x": 1175, "y": 78.72360229492188, "z": 1125 } },
        { "ArmyName": "ARMY_03", "MarkerName": "AlloyMarker_282", "Position": { "x": 823, "y": 80.744141, "z": 848 } },
        { "ArmyName": "ARMY_03", "MarkerName": "AlloyMarker_283", "Position": { "x": 833, "y": 80.759766, "z": 856 } },
        { "ArmyName": "ARMY_03", "MarkerName": "AlloyMarker_284", "Position": { "x": 835, "y": 80.681641, "z": 860 } },
        { "ArmyName": "ARMY_04", "MarkerName": "AlloyMarker_285", "Position": { "x": 1225, "y": 80.712891, "z": 1200 } },
        { "ArmyName": "ARMY_04", "MarkerName": "AlloyMarker_286", "Position": { "x": 1215, "y": 80.771484, "z": 1192 } },
        { "ArmyName": "ARMY_04", "MarkerName": "AlloyMarker_287", "Position": { "x": 1213, "y": 80.712891, "z": 1188 } }
      ],
      "AlloysToAdd": [],
      "AlloysToRemove": [],
      "AuthoringNote": "PHASE_A migration rev.2, hand-transcribed from the live Lua 'name=\"1h3ai\"' entry. Source's own comment states this entry's spawns/alloys are identical to '4human' (same underlying data) -- transcribed identically here (same four pool SpawnIds), not re-derived.",
      "Conditions": [
        { "Field": "Total", "Comparator": "Equal", "Value": 4, "SlotRangeStart": 1, "SlotRangeEnd": 1 },
        { "Field": "HumanCount", "Comparator": "Equal", "Value": 1, "SlotRangeStart": 1, "SlotRangeEnd": 1 },
        { "Field": "AiCount", "Comparator": "Equal", "Value": 3, "SlotRangeStart": 1, "SlotRangeEnd": 1 }
      ]
    },
    {
      "Name": "6total",
      "Area": { "x": 537, "y": 472, "width": 974, "height": 1104 },
      "AreaName": "",
      "SpawnsUnits": false,
      "AlloyMode": "occupancy",
      "SpawnIds": [],
      "Alloys": [],
      "AlloysToAdd": [],
      "AlloysToRemove": [],
      "AuthoringNote": "PHASE_A migration rev.2, hand-transcribed from the live Lua 'name=\"6total\"' entry. The source itself comments this scenario as still in progress, with no explicit spawns authored yet. Per MAP_SCENARIO_SPEC.md §8 an omitted spawn override is documented-acceptable for occupancy mode pending future data -- do NOT invent spawn positions here; author real pool entries (or literal ARMY_XX SpawnIds) before this composition is production-ready.",
      "Conditions": [
        { "Field": "Total", "Comparator": "Equal", "Value": 6, "SlotRangeStart": 1, "SlotRangeEnd": 1 }
      ]
    },
    {
      "Name": "2hRestAI",
      "Area": { "x": 0, "y": 0, "width": 2048, "height": 2048 },
      "AreaName": "",
      "SpawnsUnits": false,
      "AlloyMode": "occupancy",
      "SpawnIds": [],
      "Alloys": [],
      "AlloysToAdd": [],
      "AlloysToRemove": [],
      "AuthoringNote": "PHASE_A migration rev.2, hand-transcribed from the live Lua 'name=\"2hRestAI\"' entry. No explicit SpawnIds in the source; alloyMode=occupancy, so per MAP_SCENARIO_SPEC.md §8 this is documented-acceptable, inheriting the .sanmap's baked ARMY_XX Spawn transforms. Confirm the baked full-map defaults are appropriate for a 2-human/rest-AI composition before shipping.",
      "Conditions": [
        { "Field": "HumanCount", "Comparator": "Equal", "Value": 2, "SlotRangeStart": 1, "SlotRangeEnd": 1 },
        { "Field": "AiCount", "Comparator": "GreaterOrEqual", "Value": 2, "SlotRangeStart": 1, "SlotRangeEnd": 1 },
        { "Field": "Total", "Comparator": "GreaterOrEqual", "Value": 4, "SlotRangeStart": 1, "SlotRangeEnd": 1 }
      ]
    },
    {
      "Name": "floor169",
      "Area": { "x": 668.4444444444445, "y": 824, "width": 711.1111111111111, "height": 400 },
      "AreaName": "",
      "SpawnsUnits": false,
      "AlloyMode": "occupancy",
      "SpawnIds": [],
      "Alloys": [],
      "AlloysToAdd": [],
      "AlloysToRemove": [],
      "AuthoringNote": "PHASE_A migration rev.2, hand-transcribed from the live Lua 'name=\"floor169\"' entry -- the broadest COUNT_SCENARIOS fallback (t > 2), kept last per MAP_SCENARIO_SPEC.md §4/ARCH_15_06's first-match-wins array-order rule. No explicit SpawnIds in the source; alloyMode=occupancy, documented-acceptable per §8, inheriting the .sanmap's baked ARMY_XX Spawn transforms.",
      "Conditions": [
        { "Field": "Total", "Comparator": "GreaterThan", "Value": 2, "SlotRangeStart": 1, "SlotRangeEnd": 1 }
      ]
    }
  ],
  "DefaultScenario": {
    "Name": "default",
    "Area": { "x": 846, "y": 846, "width": 356, "height": 356 },
    "AreaName": "",
    "SpawnsUnits": false,
    "AlloyMode": "occupancy",
    "SpawnIds": [],
    "Alloys": [],
    "AlloysToAdd": [],
    "AlloysToRemove": [],
    "AuthoringNote": "PHASE_A migration rev.2, hand-transcribed from the live Lua DEFAULT_SCENARIO record -- Tier 3, always matches, mandatory singleton fallback (MAP_SCENARIO_SPEC.md §4)."
  },
  "MaxArmySlotCount": 16
}
```

**⚠️ CountScenarios array order is load-bearing (`ARCH_15_06_CountScenariosOrdering.md` §15.6,
first-match-wins).** Paste the eight entries above in exactly this order — `slots5to8AnyFilled`
**must stay first** (an identity-based rule must be authored above every aggregate-count rule it
could otherwise be shadowed by, `MAP_SCENARIO_SPEC.md` §5.1) — do not alphabetize, do not reorder
in a UI list.

**⚠️ `SpawnPoints` is a flat, shared, map-wide pool — author it once, reference by `SpawnId` from
any scenario's `SpawnIds` list.** A pool `SpawnId` must never be `ARMY_XX`-shaped (`ARCH_15_12`'s
`Io::IsArmyIdentityWellFormed` negation) — the four names above (`Army01_NorthSpawn`, etc.) satisfy
this by construction. A `SpawnIds` entry that matches no pool row is treated as a literal `ARMY_XX`
name and reads that army's own live `.sanmap`-baked default position — this is how every
`"SpawnIds": []` scenario below still gets a position at all.

## 2. Lossy / non-transcribable values, and how each was handled

1. **`slots5to8AnyFilled` — RESOLVED in this revision, no longer excluded.** Revision 1 of this
   document excluded this scenario entirely because neither PARAMS tier could express "any of
   slots 5-8 occupied" — Tier 1 is exact-string-only, Tier 2 only had `Total`/`HumanCount`/
   `AiCount`. `ARCH_15_05`'s `AMENDED 2026-09-04` section ratified a fourth `ScenarioCountField`,
   `SlotRangeOccupiedCount`, precisely to close this gap (`work_orders/DESIGN_ScenarioSlotRangeCondition_R1.md`
   Path B). The live predicate `pattern:sub(5,8):find("[^-]") ~= nil` transcribes exactly to
   `{Field: SlotRangeOccupiedCount, SlotRangeStart: 5, SlotRangeEnd: 8, Comparator: GreaterOrEqual,
   Value: 1}` — "at least 1 of slots 5-8 is occupied," the same semantics, no approximation. This
   is the only Tier-2 entry checked ahead of every aggregate-count rule, matching the live
   reference's own ordering discipline exactly.
   - **Still not enough on its own to spawn units.** `SpawnsUnits: true` here is necessary but not
     sufficient — per `MAP_SCENARIO_SPEC.md` §11 and `work_orders/STEP251_ScenarioCategoryFourExport_IO.md`,
     the matched scenario also needs its own category-4 generator file
     (`<MapName>_Scenarios_slots5to8AnyFilled.lua`) with `GenerateScenarioUnits(area)` filled in —
     SanGen only scaffolds an empty stub on export, it never migrates the live reference's existing
     `BuildSlots5to8Instructions` logic for you (that hand-carry step is called out explicitly in
     `MAP_SCENARIO_SPEC.md` §14's migration bullet).

2. **`6total`'s missing spawn override** — handled as instructed: not invented. Carried an empty
   `SpawnIds: []` plus an `AuthoringNote` stating the source itself flags this as still in
   progress, per `MAP_SCENARIO_SPEC.md` §8's explicit allowance for `occupancy`-mode entries
   pending future spawn data.

3. **`2h1ai`, `2hRestAI`, `floor169`'s missing spawn overrides** — same §8 allowance (`occupancy`/
   `keepAll` modes may omit an override if documented); each carries its own `AuthoringNote`. For
   `2h1ai` specifically, `MAP_SCENARIO_SPEC.md` §8 documents a **real, previously observed
   regression** where this exact composition (2 human + 1 AI) matched correctly but spawned
   units at 6-player positions because it lacked an override — flagged verbatim in that entry's
   `AuthoringNote` as something to verify with a live playtest before shipping, not assumed fixed
   by this migration alone.

4. **`4human`'s dead `navy = true` field** — confirmed retired
   (`ARCH_15_05_ParamsScenariosType.md`, "RETIRED 2026-08-28", `spawnsUnits` is the only live
   field) and deliberately not carried into the JSON. `SpawnsUnits` is emitted as `false`
   (the struct default), matching the source's own lack of a `spawnsUnits` key on this entry.

5. **`AreaName`** — left as an empty string (not omitted; the wire format always emits this key,
   confirmed in `MapExporter_Scenarios_IO.cpp`'s `BuildScenarioRecordJson`) on every entry. All
   eight scenarios use private, disconnected rectangles (`AREA_356`/`AREA_169`/`AREA_1024`/
   `AREA_FULL`), not named references into `recipe.areas` — per `MAP_SCENARIO_SPEC.md` §6.2.

6. **Coordinate flip** — resolved by code inspection, not assumption: `MapExporter_Scenarios_IO.cpp`'s
   `BuildPositionJson` and `ScenarioScript_DataLua_IO.cpp`'s `FlipPositionZ` apply the *identical*
   `mapSize - z - 1` transform to the same internal `Params` z value for the JSON leg and the
   Lua-rendering leg respectively. That means the live Lua file's `z` numbers and the `.sanmap`
   JSON `Position.z` numbers are the same value by construction — the Lua source values above
   (both the `SpawnPoints` pool and every `Alloys` entry) were transcribed with no additional flip.

7. **`SpawnPoints` pool deduplication (new in rev.2, per `ARCH_15_12`)** — the live Lua repeats
   `ARMY_01`'s exact position across `1v1`/`4human`/`1h3ai`, and `ARMY_02`/`03`/`04`'s positions
   across `4human`/`1h3ai` — the identical duplication class `ARCH_15_12`'s own ruling text calls
   out by name as the cause of a prior shipped regression. This migration authors each distinct
   position **once** in the shared pool and references it by `SpawnId` from every scenario that
   needs it, rather than reproducing the old file's duplication in JSON form.

## 3. How to get this into your working `.sanmap`

The exact `.sanmap` file you're actively editing, and whether it's safe to write to directly,
wasn't known at transcription time — no `.sanmap` file was touched producing this document.
Two options:

1. **Direct JSON merge**: open the target `.sanmap` in a JSON-aware text editor, add (or
   replace) a top-level `"Scenarios"` key with the object from §1 above. This is safe to try on
   an existing map — a 2026-08-20 live test confirmed the game tolerates an unrecognized
   top-level `.sanmap` section (parsed then dropped by `LoadMapData`'s whitelist), so adding this
   section is low-risk even against a map that predates it.
2. **Hand-enter via SanGen's Scenarios tab**: open the target `.sanmap` in SanGen, go to the
   Scenarios tab, and manually create the `SpawnPoints` pool plus each of the 8 `CountScenarios`
   entries (plus `DefaultScenario`) using this file purely as a source-of-truth reference for
   every field value (name, area rect, alloy mode, conditions, spawn-id references, alloys) —
   this exercises the real authoring surface and its own validation (name charset/uniqueness,
   `maxArmySlotCount`, stale-`areaName` checks, the `SpawnId` reserved-namespace check) rather
   than trusting a hand-pasted blob. Note per `ARCH_15_13`, `ScenariosTab_Detail_UI.cpp`'s spawn
   editor needs its own redesign for the pool/reference model — if that redesign hasn't landed
   yet in your build, option 1 (direct JSON merge) is the only route until it does.

Either way, remember §2 item 1 above: `slots5to8AnyFilled`'s `SpawnsUnits: true` alone still
spawns nothing until its category-4 generator file (`STEP251`) is filled in by hand.
