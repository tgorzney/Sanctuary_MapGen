# DRAFT — RATIFIED 2026-09-04 into ARCH_15_12_ScenarioSpawnIdentity.md / ARCH_15_13_ScenarioSpawnIdRuntimeResolution.md

**Status: RATIFIED, kept as historical record only — read §15.12/§15.13 as the authoritative text,
not this file.** This revision-3 draft landed verbatim on 2026-09-04. One clause it proposed was
subsequently found incorrect and corrected in the ratified text on 2026-09-05 (the
`IsArmyIdentityWellFormed`-negation approach below, in "Proposed binding shape"/"Validator"/"Open
questions"/"Scope for implementation", is case-sensitive and does not catch `"army_01"`/`"Army_01"` —
see `work_orders/ARCH_CORRECTION_DRAFT_ScenarioSpawnIdCaseInsensitivity.md` for the diagnosis and
`ARCH_15_12_ScenarioSpawnIdentity.md`'s Validator section for the corrected, ratified rule: a
dedicated case-insensitive predicate, `Io::ResemblesArmyIdentityCaseInsensitive`, not a negation of
`IsArmyIdentityWellFormed`). The body below is preserved unedited as what was actually proposed and
ratified at the time — do not silently "fix" it to match the later correction; read it as history.

**Status (original): UNRATIFIED DRAFT (revision 3).** Produced 2026-09-03 by an advisory (read-only) consult on
the SanGen ARCH Expert's behalf — this document is **not** itself authoritative and binds nothing. It
exists so the human can carry a precise, ready-to-ratify text into the ARCH Expert's own dedicated
setup conversation. Nothing in `ARCH.md`, `ARCH_15_04_ThreeFileOnDiskShape.md`,
`ARCH_15_05_ParamsScenariosType.md`, or `sangen_arch_pack/` has been touched to produce this file.
Until ratified there, treat every clause below as a proposal, not law.

**Revision notes.**
- Revision 1 laid out two structurally different designs — (A) scenario-scoped `spawnId` inline on
  each spawn entry, vs. (B) a shared, `Scenarios`-level pool referenced by `spawnId` — and left the
  choice open. **RULED — (B).** Revision 2 rewrote the shape throughout to match; (A)'s reasoning is
  preserved under "Rejected alternatives" (item 6), not deleted.
- Revision 2 raised two further open questions: whether a pool `spawnId` should be forbidden from
  colliding with a literal `ARMY_XX` name, and how the breaking `Spawns`→`SpawnIds` wire-shape change
  should be migrated. **Both are now RULED** by the human. This revision (3) incorporates both: pool
  `spawnId`s are a hard-forbidden reserved namespace (`Validator` section), and this amendment
  produces NO migration entry at all — the one existing `.sanmap` is hand-edited directly
  (`Wire-format consequence` section). **This draft has no remaining open question** — see the closing
  note at the end of "Open questions" below.

Full background/reasoning trail (not repeated here): the advisory consult that produced this draft;
`Scenario_PARAMS.h`; `ARCH_15_04_ThreeFileOnDiskShape.md` (both the original §15.4 and its
"AMENDED 2026-09-03" category-4 section — the closest existing precedent for a Runtime.lua algorithm
ruling, see below); `ARCH_15_05_ParamsScenariosType.md` §15.5 (binding shape) and its
"AMENDED 2026-08-28" `areaName` section (the closest existing precedent for a `Scenarios`-level
reference pool — cited throughout, including for where this draft's design deliberately diverges from
it); `MAP_SCENARIO_SPEC.md` §6/§8/§9; `src/io/Sanmap_ArmyIdentity_IO.h` (the canonical `ARMY_XX`
well-formedness predicate this revision reuses for the reserved-namespace rule); the live reference
script `Pandemonium Isthmus_Scenarios_Script.lua` lines 101-113, 148-253 (the incident this amendment
answers, and the literal triple-duplicated `ARMY_01` coordinate: `{x=855, y=79.12979888916016, z=920}`
appears verbatim in the `1v1` (line 155), `4human` (line 191), and `1h3ai` (line 226) entries).

---

## Problem restated, confirmed against the live code (not just the human's paraphrase)

`Params::ScenarioSpawn` (`src/params/Scenario_PARAMS.h:23`) is today:

```cpp
struct ScenarioSpawn { std::string armyName; float positionX = 0.0f, positionY = 0.0f, positionZ = 0.0f; };
```

Two confirmed gaps, both independently verified (not taken on the human's word alone):

1. **No identity for the physical spawn location itself, independent of which army occupies it, and
   no shared authored source for it.** Verified in the live reference: the exact tuple `{855,
   79.12979888916016, 920}` for `ARMY_01` is retyped verbatim in three separate scenario entries
   (`1v1`, `4human`, `1h3ai`) with no shared authored source — precisely the class of duplication the
   file's own comment (lines 101-113) says caused the original regression ("ARMY_01's position used
   to be a single static value in the .sanmap shared by every composition... editing it to test the
   6-player case silently broke 1v1... the same collision hit the 4-human and 1-human-3-AI
   compositions too").
2. **No validator stops two `ScenarioSpawn` entries in the same `ScenarioBody::spawns` vector from
   naming the same `armyName`.** Confirmed by reading `MapImporter_ScenarioRecord_IO.cpp:31-40`
   (`ReadSpawnsJson`) and `SanGenScenarioRuntime.lua:169-181` (`ApplyScenario`) — the runtime
   `ipairs()`-walks the flat `spawns` array and unconditionally overwrites
   `spawnTransforms[spawnRow.armyName]` for every row; two rows naming the same army silently resolve
   to **iteration-order-wins**, not a defined "first authored wins" or any other rule. This is a live,
   real gap, not hypothetical.

**The immovable constraint** (not open for reinterpretation, confirmed by reading the codebase's own
description of `common/gameUtils.lua`'s hardcoded read path — base-engine code, unmodifiable): the
final applied write must always land keyed by `ARMY_XX`
(`GameInfo.MapData.markers.Spawn.transforms[army.name]`). `spawnId` is additive identity layered on
top of `armyName`, never a replacement for it, at every leg of this system — unchanged by this
revision.

---

## RULED — shared pool (B), settled 2026-09-03 after two rounds of refinement

**Open Question 1 from revision 1 is CLOSED.** The human ruled (B): a new, shared, `Scenarios`-level
pool of named custom spawn points, referenced by `spawnId` from any scenario. The settled design has
four load-bearing points, each addressed in full below:

1. **`.sanmap`'s `markers.Spawn.transforms[ARMY_XX]` is completely unchanged** — still the per-map
   baked default per army, still what determines max player count. This amendment does not touch it,
   anywhere.
2. **The new pool holds ONLY custom/authored spawn points — never pre-seeded with the `ARMY_XX`
   defaults.** The human's own reasoning: copying every `ARMY_XX` default position into the pool too
   would be a second, redundant source of truth for data that already lives in the `.sanmap` and is
   already resident in memory at runtime (`GameInfo.MapData.markers.Spawn.transforms`) — exactly the
   kind of duplicated-authored-data hazard this whole amendment exists to eliminate. See "Rejected
   alternatives" item 7.
3. **`ScenarioBody::spawns` becomes a flat list of bare `spawnId` strings**, resolved by a two-step
   lookup (pool first, then literal `ARMY_XX` fallback) wherever a scenario's spawns are needed — at
   export/validation time in C++ and at match-load time in Lua.
4. **The runtime script itself is rewritten**, and that rewrite is itself part of what gets ratified —
   the human's own words: "The script will need to be modified and re ratified into law." This is
   treated with the same weight as `ARCH_15_04`'s own already-ratified category-4 dispatch mechanism
   (a concrete, binding Lua code block, not a paraphrase of intent) — see "Runtime rewrite" below.

---

## Proposed binding shape

**`ScenarioSpawn` (old shape, `armyName` + inline position) is RETIRED entirely** — not extended with
a `spawnId` field as revision 1 proposed. It is replaced by two new, differently-scoped types:

```cpp
// NEW — one row in the shared, Scenarios-level custom-spawn-point pool. Authored ONCE; referenced by
// spawnId from any scenario's spawnIds list (point 3 above). Never holds an ARMY_XX default position
// (point 2 above) -- only genuine per-scenario overrides, e.g. "North_1v1" targeting ARMY_01.
struct ScenarioSpawnPoint {
    std::string spawnId;    // required non-empty; unique across the WHOLE pool (a flat, shared
                             // namespace -- there is exactly one row per spawnId, unlike the
                             // per-scenario-scoped uniqueness revision 1 proposed under design (A)).
                             // RULED: must NOT be ARMY_XX-shaped -- see "Validator" below.
    std::string armyName;   // required non-empty -- the ARMY_XX this custom point targets.
    float positionX = 0.0f, positionY = 0.0f, positionZ = 0.0f;
};

struct ScenarioBody {
    // ... name / area / areaName / spawnsUnits / alloyMode unchanged ...
    std::vector<std::string> spawnIds;    // RESHAPED (was std::vector<ScenarioSpawn> spawns). Each
                                           // entry is EITHER a Scenarios::spawnPoints[].spawnId (a
                                           // custom override) OR a literal ARMY_XX name (use that
                                           // army's own live baked .sanmap default) -- resolved by the
                                           // two-step lookup below, never distinguished by shape or
                                           // any other field on ScenarioBody itself.
    // ... alloys / alloysToAdd / alloysToRemove / authoringNote unchanged ...
};

struct Scenarios {
    std::vector<PatternScenario>    patternScenarios;
    std::vector<CountScenario>      countScenarios;
    ScenarioBody                    defaultScenario;
    int                             maxArmySlotCount = 16;
    std::vector<ScenarioSpawnPoint> spawnPoints;   // NEW -- the shared custom-spawn-point pool
                                                    // (point 2 above). Parallel in ROLE to
                                                    // recipe.areas (§15.5's areaName precedent: "a
                                                    // scenario may reference something authored once
                                                    // elsewhere") but NOT parallel in scope -- this
                                                    // pool lives on Scenarios, not MapRecipe, since
                                                    // nothing outside the scenario system has any use
                                                    // for a custom spawn point.
};
```

**Two-step resolution — the one algorithm every consumer (C++ validator/export, Lua runtime) applies
identically, never reimplemented per call site with different fallback behavior:**

```
Resolve(spawnId):
  1. Scan Scenarios::spawnPoints for a row whose spawnId matches (first-match, mirrors this file
     family's own established "first-match, never last-wins" idiom -- see areaName's
     ResolveScenarioAreaRect). Found -> (row.armyName, row.positionX, row.positionY, row.positionZ).
  2. Not found in the pool -> spawnId IS treated as a literal ARMY_XX name. Its resolved armyName is
     spawnId itself; its position is NEVER separately stored -- it is read live from whatever the
     current ARMY_XX Spawn marker transform already holds (the .sanmap's own baked default, or
     GameInfo.MapData.markers.Spawn.transforms[spawnId] at runtime).
  3. Matches neither (typo, a pool row that was renamed/deleted, or a string that names no real
     ARMY_XX) -> unresolvable. See "Validator" below for the loud/logged/non-blocking posture.
```

**RULED: steps 1 and 2 can never both match the same string.** See "Validator" below — pool
`spawnId`s are a reserved-namespace-forbidden set that structurally excludes every `ARMY_XX`-shaped
string, so this is a structural guarantee of the shape itself, not a behavior that depends on the
scan order above.

---

## Validator — re-expressed against RESOLVED army names, not raw struct fields

Revision 1's gap-2 validator ("no duplicate `armyName` within one scenario's spawn list") still
applies, but its INPUT changed: `ScenarioBody::spawnIds` no longer carries `armyName` directly, so the
check is now: **resolve every `spawnId` in one scenario's `spawnIds` list (step 1/2 above), then
verify no two entries resolve to the same `armyName`.** Two different `spawnId`s legitimately can
resolve to the same `armyName` ACROSS different scenarios (the human's own explicit example) — this
rule is strictly per-scenario, over resolved values, same as revision 1's intent, just re-pointed at
the new indirection.

- **Duplicate resolved `armyName` within one scenario.** Real functional consequence (unchanged from
  revision 1's reasoning: the runtime's iteration-order-wins clobber). Enforcement: **loud, logged,
  never a flat refusal of the whole export** (§15.5's own "never a flat refusal" posture); the
  specific fix-up is **first-authored-wins, the later-resolving `spawnId`(s) dropped** from the
  scenario's rendered `spawnIds` on both legs — a third instance of this exact file family's own
  "first-match, never last-wins" idiom (`areaName`'s `ResolveScenarioAreaRect`,
  `AreasTab_List_UI.h`'s `ResolveAreaColor`, `UniqueNameList_UI.h`'s `NameIsTakenBefore`).
- **Duplicate `spawnId` within `Scenarios::spawnPoints`.** The pool is a flat, shared namespace (point
  3 of the ruled design) — a duplicate `spawnId` row is a real integrity problem (which row a
  reference resolves to becomes order-dependent). Enforcement: same loud/logged/non-blocking posture;
  fix-up is first-authored-wins, later duplicate pool row(s) excluded from render, same idiom again.
- **Unresolvable `spawnId`** (step 3 above — matches neither the pool nor a real `ARMY_XX`).
  Enforcement: **warn-only, never dropped, never invented.** This reuses an EXISTING, directly
  on-point precedent already in the codebase — `MapExporter_ArmySpawnMarkerValidation_IO.h`'s
  `ArmySpawnMarkerValidationReport`, which is explicitly "WARN-ONLY: this REPORTS and nothing else...
  never auto-created, never auto-deleted, never blocking" for the sibling case of an army with no
  matching `Spawn` marker at all. An unresolvable `spawnId` gets the same posture, at the same two
  enforcement points (UI-authoring time and export time) this whole file family already uses for
  `areaName` staleness and `name` charset/uniqueness.
- **RULED 2026-09-03 — `ARMY_XX`-shaped strings are a reserved namespace, forbidden as pool
  `spawnId`s.** The human's own words: "no, these are reserved and cannot be used as custom."
  Enforced by negating the exact existing well-formedness predicate already ratified for `ARMY_XX`
  minting — `Io::IsArmyIdentityWellFormed` (`src/io/Sanmap_ArmyIdentity_IO.h:34-43`, STEP76) — never a
  second, reinvented charset rule: a `Scenarios::spawnPoints` row is invalid if
  `IsArmyIdentityWellFormed(row.spawnId)` is true. Same enforcement points and posture as the sibling
  duplicate-`spawnId`/duplicate-resolved-`armyName` rules directly above: UI-authoring time and export
  time, loud/logged, non-blocking to the whole export, the specific offending pool row's write
  refused/excluded with a named error. **This is now a structural guarantee, not a convention or
  judgment call**: since `ARMY_XX`-shaped strings can never occupy the pool, resolution step 1 (pool
  scan) and step 2 (literal-`ARMY_XX` fallback) can never both match the same string — the shadowing
  footgun revision 2 flagged as an open judgment call is closed by construction here, not by a rule an
  author could forget to check.

---

## Wire-format consequence — both legs (the Lua-leg posture is the OPPOSITE of `areaName`'s, explained)

**`.sanmap` JSON leg.**
- `MapExporter_Scenarios_IO.cpp`'s `BuildScenariosJson` (lines 100-133) gains a fifth top-level
  sibling key, alongside the existing `PatternScenarios`/`CountScenarios`/`DefaultScenario`/
  `MaxArmySlotCount`: `document["SpawnPoints"]` — an array of `{ "SpawnId", "ArmyName", "Position" }`
  objects, one per `Scenarios::spawnPoints` row, same per-entry shape `BuildSpawnsJson` used to emit
  (just with `SpawnId` added). `MapImporter_ScenarioRecord_IO.cpp` gains the mirror `ReadSpawnPointsJson`.
- **Per-`<ScenarioRecord>`, the existing `"Spawns"` key (array of `{ArmyName, Position}` objects) is
  RETIRED and replaced by a new key, `"SpawnIds"` (a plain array of strings).** `BuildSpawnsJson`/
  `ReadSpawnsJson` (`MapExporter_Scenarios_IO.cpp:32-36`, `MapImporter_ScenarioRecord_IO.cpp:31-40`)
  are retired; a new pair reads/writes a flat `std::vector<std::string>`. The C++ field is likewise
  renamed `spawns` → `spawnIds`, mirroring this exact file's own precedent for a rename-on-meaning-
  change (`navy` → `spawnsUnits`, STEP204) rather than silently overloading the old name for a new
  shape.
  - **This is NOT a purely additive wire change, unlike every prior amendment to this file
    (`AreaName`, `name`'s charset rule).** It changes an EXISTING field's on-disk TYPE
    (array-of-objects → array-of-strings) — a real breaking change, not something "absent key defaults
    to struct default" can paper over on its own.
  - **RULED 2026-09-03 — NO migration entry, no importer backward-compat shim, in this
    ratification.** The human's own words: "we do not need to be concerned with migration yet, I only
    have the one map file, we will manually change it instead of trying to make a specific importer."
    This is a deliberate, ratified, narrow **exception** to this file's general "loud migration, never
    silent loss" posture for HARD-REQUIRED data (§8) — recorded as an exception with its own stated
    reason (exactly one existing hand-authored `.sanmap`, `Pandemonium Isthmus.sanmap`, to be hand-
    edited by the human directly to the new `SpawnIds`/`SpawnPoints` shape), **not** as a contradiction
    of that posture's general case. No `IO_MIGRATION_SPEC.md` migration unit is produced or required by
    this amendment; no `SanGenVersion` bump is tied to this shape change.
  - **Importer posture for the retired old shape — decided, kept deliberately small.**
    `ReadSpawnsJson`/`BuildSpawnsJson` are retired outright, exactly as stated above — the new
    `ReadSpawnIdsJson` has no code path that understands the old array-of-objects `"Spawns"` shape at
    all, mirroring this exact file's own existing precedent for `navy`/`NavalFleet` ("the function
    simply never reads them... falls through exactly like any other unrecognized field"). **The one
    addition, to satisfy Constitution §6's "loud, never silent" for what could still be real
    HARD-REQUIRED data on some future map: a single, small, WARN-ONLY structural detection.** If a
    scenario record's `"Spawns"` key is present and its array elements are JSON objects (the old
    shape's own structural signature, cleanly distinguishable from the new shape's plain-string array
    elements — no schema ambiguity), log one loud warning naming the scenario, stating the legacy shape
    was found and NOT read. Never attempt to interpret/convert it; never populate `spawnIds` from it.
    This is **not** a migration (no data transformation, no version bump, no new file/unit) — it is
    the same class of lightweight, bolted-on WARN-ONLY check this file family already uses elsewhere
    (`ArmySpawnMarkerValidationReport`'s own precedent), sized to do exactly one thing: make sure a
    human editing a legacy file by hand gets a clear signal instead of silently losing data with no
    trace, without any migration machinery behind it.

**`<MapName>_Scenarios_Data.lua` leg — the pool's actual coordinates DO need to reach Lua, the OPPOSITE
of `areaName`'s posture, and that difference is real, not an oversight.** Revision 1 of this draft
(before the human's ruling) proposed spawnId never reach the Lua leg at all, reasoning by direct
analogy to `areaName` ("nothing for a Lua-side string to do"). **That reasoning does not transfer to
this shape and is retracted** — see "Rejected alternatives" item 2. `areaName`'s referent (a rectangle)
is always fully resolved by the C++ exporter before either leg is rendered, so the Lua leg only ever
needs the FINAL numbers, never the reference. This pool's referent is different in kind: a scenario's
own `spawnIds` list can resolve to the literal-`ARMY_XX`-fallback case (step 2 of the algorithm), whose
correct value is **whatever the live `.sanmap` baked transform holds at match-load time** — not a
value SanGen's own exporter can bake in ahead of time without reintroducing exactly the "one static
value shared by every composition" bug this whole system exists to prevent for the DEFAULT case too.
The runtime genuinely needs the real, current data to do the two-step lookup itself, live, every match.
- `ScenarioScript_DataLua_IO.cpp` gains a new top-level render, alongside the existing
  `PATTERN_SCENARIOS`/`COUNT_SCENARIOS`/`DEFAULT_SCENARIO` globals (`BuildPatternScenariosTable`/
  `BuildCountScenariosTable`/`BuildDefaultScenarioTable`, lines 138-191): a `SCENARIO_SPAWN_POINTS`
  global, one flat row per `Scenarios::spawnPoints` entry (`{ spawnId = "...", armyName = "...", x =
  ..., y = ..., z = ... }`), always rendered even when the pool is empty (`SCENARIO_SPAWN_POINTS = {}`)
  — matching this family's "every table is always present" convention, never omitted.
  - **`SCENARIO_SPAWN_POINTS` is genuinely AUTHORED `Params::Scenarios` data, unlike
    `ARMY_ID_TO_NAME`/`KNOWN_ALLOY_MARKERS`.** `ScenarioScript_DataLua_IO.h`'s own header comment
    warns a future reader not to add a `Params::Scenarios` field to match those two DERIVED globals —
    that warning does not apply here; `SCENARIO_SPAWN_POINTS` is a direct 1:1 render of
    `Scenarios::spawnPoints`, the same category as `PATTERN_SCENARIOS`/`COUNT_SCENARIOS`, not the
    derived-globals category. Flagged explicitly so a future implementer does not conflate the two.
  - `AppendScenarioBodyFields` (`ScenarioScript_DataLua_IO.cpp:101-131`) replaces its
    `AppendArrayOfTables(out, indentLevel, "spawns", BuildSpawnRowBodies(...))` line with a flat
    array-of-quoted-strings render of `body.spawnIds` (a small new primitive alongside
    `AppendArrayOfTables` in `LuaTableWriter_IO.h`, or an inline equivalent — left to the coder,
    unchanged in spirit from how every other primitive in this file already composes).

---

## Runtime rewrite — `resources/lua/SanGenScenarioRuntime.lua`'s `ApplyScenario`

**Category-2 (`_Scenarios_Runtime.lua`) content per `ARCH_15_04`'s file categories — ratified with the
same weight as any other Runtime.lua algorithm change, per the human's own explicit instruction, not a
Data.lua-only or PARAMS-only change.**

`ScenarioData.PATTERN_SCENARIOS`/etc.'s capture block (`SanGenScenarioRuntime.lua:40-43`) gains a
fifth capture:

```lua
local SCENARIO_SPAWN_POINTS = ScenarioData.SCENARIO_SPAWN_POINTS
```

`ApplyScenario`'s spawn-application block (currently `SanGenScenarioRuntime.lua:169-181`) is replaced:

```lua
-- Two-step resolution (ARCH_15_12 draft): the custom pool first, then a literal ARMY_XX fallback
-- read straight off the live .sanmap default -- never a second stored copy of that default. Pool
-- spawnIds are RULED to never be ARMY_XX-shaped, so these two steps can never collide on one string.
-- Returns nil (and Warn()s) if spawnId matches neither.
local function ResolveSpawnId(spawnId, spawnTransforms)
    for _, point in ipairs(SCENARIO_SPAWN_POINTS) do
        if point.spawnId == spawnId then
            return point.armyName, point.x, point.y, point.z
        end
    end
    local defaultTransform = spawnTransforms and spawnTransforms[spawnId]
    if defaultTransform then
        return spawnId, defaultTransform.position.x, defaultTransform.position.y, defaultTransform.position.z
    end
    Warn("SANGEN: scenario spawnId '"..tostring(spawnId).."' matched neither the custom spawn-point "..
         "pool nor a live ARMY_XX Spawn marker -- skipped, no transform written.")
    return nil
end

local function ApplyScenario(scenario, total, slotPattern)
    if scenario.spawnIds then
        local spawnTransforms = GameInfo.MapData.markers and GameInfo.MapData.markers.Spawn
            and GameInfo.MapData.markers.Spawn.transforms
        if spawnTransforms then
            for _, spawnId in ipairs(scenario.spawnIds) do
                local armyName, x, y, z = ResolveSpawnId(spawnId, spawnTransforms)
                if armyName and spawnTransforms[armyName] then    -- UNCHANGED guard: never creates a
                    spawnTransforms[armyName].position.x = x       -- missing transform, same as today
                    spawnTransforms[armyName].position.y = y
                    spawnTransforms[armyName].position.z = z
                end
            end
        end
    end
    -- ... alloy handling below is UNCHANGED by this amendment ...
```

- **Same final write target as today, different resolution path feeding it** — the human's own
  framing, implemented literally: `spawnTransforms[armyName]` is still the only thing ever mutated.
- **The literal-`ARMY_XX`-fallback branch (step 2) writes back the exact value it just read** — a
  harmless, deliberate no-op (`x, y, z` read from `spawnTransforms[spawnId].position` and immediately
  written back to the identical location, since `armyName == spawnId` in that branch). Not special-
  cased/skipped — a single uniform code path for both branches is more legible than adding a
  conditional to avoid a no-op write with no measurable cost (this runs once per map load, not
  per-frame; rough-estimate, not benchmarked: O(spawnIds authored) transform writes at load time,
  negligible against any per-frame budget).
- **Performance basis, tagged per this consult's own discipline:** `ResolveSpawnId`'s pool scan is a
  **rough-estimate** O(N) linear search over `SCENARIO_SPAWN_POINTS`, N bounded by how many custom
  spawn points a human hand-authors (realistically tens, not thousands), run once per scenario match
  at map load — not a hot path, no index/hash structure justified. Not benchmarked; if pool sizes ever
  grow large enough for this to matter, a `spawnId -> row` lookup table built once at file-load time is
  a straightforward future optimization, not required by this ruling.
- **`Scenario.SpawnMatchedScenarioUnits`/`Scenario.SpawnUnits`/the alloy-application block below are
  UNCHANGED** — this amendment touches only the spawn-application block quoted above.

---

## Rejected alternatives, recorded so a future reader does not re-propose them

1. **`spawnId` replaces or repurposes `armyName` as the marker key.** Rejected outright — the
   immovable engine constraint (`common/gameUtils.lua`'s hardcoded read path).
2. **Never rendering the pool's coordinates to the Lua leg, mirroring `areaName`'s posture verbatim.**
   This was revision 1's own proposal. **Retracted, not merely superseded** — see "Wire-format
   consequence" above for why the reasoning does not actually transfer: `areaName`'s referent is
   always fully C++-resolved before either leg renders; this pool's literal-`ARMY_XX` fallback case
   resolves to a value only the live runtime has, at match-load time.
3. **Silently auto-generating a `spawnId` for legacy entries with no human-visible trace.** Rejected —
   Constitution §6, and this file's own established posture for every other field
   (`areaName`/`name`). No migration exists in this ratification (see item 9 below), but IF one is
   ever authored later, it must remain loud/logged/deterministic/visible, never silent invention with
   no record.
4. **Last-resolved-wins instead of first-authored-wins on a duplicate resolved `armyName` or a
   duplicate pool `spawnId`.** Rejected — contradicts this file family's own repeatedly-cited
   "first-match, never last-wins" idiom.
5. **Blocking the whole export outright on any duplicate/unresolvable `spawnId`/`armyName`.**
   Rejected — contradicts §15.5's own repeatedly-stated "never a flat refusal" posture; a targeted,
   defined fix-up plus a loud warning is the established shape.
6. **Design (A), scenario-scoped `spawnId` inline on each `ScenarioSpawn` (revision 1's original
   proposal).** Not rejected for being wrong on its own terms — it cleanly solved gap 2 and required a
   smaller shape change — but it left gap 1 (cross-scenario literal-coordinate duplication) completely
   unsolved, which is the entire premise of the human's ask. Superseded by the explicit 2026-09-03
   ruling for (B). Recorded, not deleted, so a future reader sees the tradeoff was considered, not
   missed.
7. **Pre-seeding the pool with every `ARMY_XX` default position (a "complete" pool, custom AND
   default rows together).** Rejected — the human's own explicit reasoning: this would create a
   second, redundant, driftable copy of data the `.sanmap`/`GameInfo.MapData` already holds as the
   single source of truth, the exact class of hazard (duplicated authored data) this whole amendment
   exists to eliminate. This is why the pool holds custom overrides ONLY and the literal-`ARMY_XX`
   fallback reads the live default directly instead of a baked copy.
8. **Treating the old `navy`-field "silently drop, never warn, never migrate" precedent as fully
   applicable to legacy `Spawns` data.** Rejected as a full match — `navy` was confirmed fully dead
   (zero readers) before it was dropped; a legacy scenario's inline `Spawns` array is live,
   HARD-REQUIRED (§8), deterministic-composition-critical data, so it gets the extra WARN-ONLY
   detection described in "Wire-format consequence" that `navy` never needed — but see item 9: a full
   migration unit is still out of scope, for a different, human-stated reason (below).
9. **Building a full `IO_MIGRATION_SPEC.md` migration unit / importer backward-compat shim for the
   legacy `Spawns` shape, in this ratification.** Rejected — out of scope per the human's explicit
   ruling: exactly one hand-authored `.sanmap` exists today and will be hand-converted directly, not
   machine-migrated; inventing migration machinery to serve one, human-supervised, one-time conversion
   is unnecessary process weight. Not a permanent ruling against ever migrating this shape — if/when a
   second pre-amendment map needs importing, that is a new, separate ratification question.

---

## Open questions for the human

1. ~~(A) scenario-scoped vs. (B) shared pool.~~ **RULED — (B).** No longer open.
2. ~~Should a custom pool `spawnId` be forbidden from colliding with a literal `ARMY_XX`-shaped
   string?~~ **RULED 2026-09-03 — FORBIDDEN, reserved namespace**, enforced via
   `Io::IsArmyIdentityWellFormed` negated (see "Validator" above). No longer open.
3. ~~Migration mechanics and timing for the breaking `Spawns`→`SpawnIds` wire-shape change.~~
   **RULED 2026-09-03 — NO migration entry in this ratification**; the single existing `.sanmap` is
   hand-edited directly by the human (see "Wire-format consequence" above). No longer open.

**This draft has no remaining open question.** Every question raised across all three revisions —
shared pool vs. scenario-scoped, the reserved-namespace shadowing hazard, and migration mechanics — is
now ruled and reflected in the binding shape, wire format, validator, and runtime sections above. This
consult's own analysis defers nothing further; the draft is ready to carry into the ARCH Expert's
ratification conversation as-is. (Ratification itself may of course surface new questions once the
ARCH Expert reads this against the fuller pack context — that is a normal part of ratification, not an
outstanding item this consult left unresolved.)

---

## File-size ceiling — where this amendment should land, decided (re-examined for the larger scope)

**Recommendation, revised from revision 1: TWO new sibling files, not one — because this amendment's
content now genuinely spans both of this exact family's already-established divisions of labor, and
neither destination file has room left.** Applying `ARCH_01_05_FileSizeCeilings.md`'s actual numbers:

- **`ARCH_15_05_ParamsScenariosType.md` is 371 lines** (2.47x the hard 150-line ceiling, already a
  documented Constitution §7 exception). The binding-shape/wire-format/validator content above
  (`ScenarioSpawnPoint`, `ScenarioBody::spawnIds`, both wire legs, the validator) is PARAMS/IO-shape
  content, §15.5's own natural home by subject matter — but appending it would push §15.5 well past
  520 lines, worse than doubling an already-doubled exception, and (unlike the 2026-09-03 `name`
  addendum) this content has no dependency on §15.5's own OPEN-item narrative that would justify
  forcing it into the same file. **Recommendation: new sibling `ARCH_15_12_ScenarioSpawnIdentity.md`**
  for this content, matching the `ARCH_15_10`/`ARCH_15_11` standalone-ruling precedent already
  established in this exact family.
- **`ARCH_15_04_ThreeFileOnDiskShape.md` is 135 lines** — only 15 lines of headroom before its own
  hard 150-line ceiling. §15.4 is this family's own established home for concrete Runtime.lua
  ALGORITHM content (it already carries the ratified category-4 dispatch code block for exactly this
  reason). The `ResolveSpawnId`/`ApplyScenario` rewrite above is squarely that same kind of content —
  but at ~15 lines of headroom, it cannot fit even a fraction of the ~70-100 lines the runtime section
  above runs to. **Recommendation: new sibling `ARCH_15_13_ScenarioSpawnIdRuntimeResolution.md`** for
  the runtime rewrite, cross-referencing `ARCH_15_12` for the shape it resolves against — the same
  "new sibling when the natural home is full" logic applied to §15.4 that was already applied to
  §15.5.
- **Precedent for splitting exactly this kind of tightly-coupled pair already exists in this same
  family and is not a new pattern this draft invents**: the 2026-09-03 `name`-becomes-path-component
  validation rule (PARAMS/validator content) lives in §15.5, while the dispatch mechanism it exists
  BECAUSE OF (Runtime.lua algorithm content) lives in §15.4 — two files for one tightly-coupled
  ruling, exactly the split recommended here.
- **Re-confirmed for revision 3, not carried forward unexamined.** Removing the migration-entry
  paragraph trimmed roughly 15-20 lines from what would have landed in `ARCH_15_12`'s wire-format
  section; the new reserved-namespace ruling adds back a comparable handful of lines to its validator
  section. Net effect on `ARCH_15_12`'s projected length is negligible either way — it was never close
  to fitting inside §15.5's remaining (nonexistent) headroom regardless. `ARCH_15_13`'s runtime-rewrite
  content is untouched by either of this revision's rulings. **The two-new-sibling-file recommendation
  stands unchanged.**
- `Scenario_PARAMS.h`'s "source of truth" header comment would need a third citation
  (`ARCH_15_12_ScenarioSpawnIdentity.md`) for `ScenarioSpawnPoint`/`ScenarioBody::spawnIds`;
  `resources/lua/SanGenScenarioRuntime.lua`'s own header comment would need a citation
  (`ARCH_15_13_ScenarioSpawnIdRuntimeResolution.md`) alongside its existing `ARCH_15_04` one.

---

## Scope for implementation (pointer only, not a work order)

Once ratified, the coder-facing work order should cover, at minimum:
- `Scenario_PARAMS.h`: retire `ScenarioSpawn`; add `ScenarioSpawnPoint`; add `Scenarios::spawnPoints`;
  change `ScenarioBody::spawns` (`std::vector<ScenarioSpawn>`) to `ScenarioBody::spawnIds`
  (`std::vector<std::string>`).
- `MapExporter_Scenarios_IO.cpp`: retire `BuildSpawnsJson`; add `BuildSpawnIdsJson` (flat string array)
  for the per-scenario `SpawnIds` key; add a `BuildSpawnPointsJson` for the new top-level `SpawnPoints`
  pool array in `BuildScenariosJson`.
- `MapImporter_ScenarioRecord_IO.cpp`: retire `ReadSpawnsJson`; add the mirror `ReadSpawnIdsJson`/
  `ReadSpawnPointsJson`; add the small WARN-ONLY legacy-shape detection described in "Wire-format
  consequence" (old `"Spawns"` array-of-objects present → log, do not read).
- **No migration work is in scope for this ratification** (RULED — see "Wire-format consequence"): no
  `IO_MIGRATION_SPEC.md` unit, no `SanGenVersion` bump, no IO Architecture Expert coordination needed
  for this change. The one existing `.sanmap` (`Pandemonium Isthmus.sanmap`) is hand-edited by the
  human directly to the new shape.
- The shared validator (wherever it lands — UI + both export legs, per "Validator" above) must reject/
  warn any `Scenarios::spawnPoints` row whose `spawnId` satisfies `Io::IsArmyIdentityWellFormed`
  (`src/io/Sanmap_ArmyIdentity_IO.h`) — reuse that function directly, do not reimplement the `ARMY_XX`
  charset check a second time.
- `ScenarioScript_DataLua_IO.cpp`: replace the per-scenario `spawns` array-of-tables render with a
  flat `spawnIds` array-of-strings render; add the new top-level `SCENARIO_SPAWN_POINTS` render
  (genuinely authored data, not a derived global — see the explicit flag in "Wire-format consequence").
- `resources/lua/SanGenScenarioRuntime.lua`: add the `SCENARIO_SPAWN_POINTS` capture and replace
  `ApplyScenario`'s spawn-application block with `ResolveSpawnId` + the two-step-lookup loop, exactly
  as specified in "Runtime rewrite" above — this is itself part of the ratified change, not left to
  coder discretion the way an ordinary implementation detail would be.
- `ScenariosTab_Detail_UI.cpp`: `DrawScenarioSpawnsList` (line 118) needs a real redesign — it
  currently edits a `std::vector<Params::ScenarioSpawn>` directly (`DrawArmyNameField` per row, line
  124); it must become an editor over `ScenarioBody::spawnIds` (string references) plus a separate
  editor for `Scenarios::spawnPoints` (the pool itself, likely its own list section, structurally
  similar to how `recipe.areas` gets its own `AreasTab_List_UI.h`), wired to the resolved-armyName,
  pool-uniqueness, and reserved-namespace validators above.
