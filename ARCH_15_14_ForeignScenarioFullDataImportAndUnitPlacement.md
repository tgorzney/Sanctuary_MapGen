[← ARCH index](ARCH.md) · [§15 ARCH_15_MapScenarioSystem](ARCH_15_MapScenarioSystem.md) · SanGen ARCH §15.14. **Only the ARCH Expert writes this file.**

### 15.14 Narrow extension of §15.11's carve-out (2026-09-11, AMENDED 2026-09-11 same day, AMENDED AGAIN 2026-09-12 — see both "Amendment" notes below) — count/slot-range match-condition extraction, exact `slotPattern` string extraction, literal unit-placement-tuple extraction from a FOREIGN scenario `.lua`; new declarative `Params::ScenarioUnitPlacement` type

**Ruled: GRANTED, modified from the human's own request in two load-bearing ways (see "Corrections to the
request" below) — not a rubber-stamp.** This section responds to a human request to reopen §15.3/§15.11
for real Sanctuary: Shattered Sun scenario files. Verified independently against
`src/params/Scenario_PARAMS.h`, `resources/lua/SanGenScenarioRuntime.lua`,
`src/io/ScenarioScript_DataLua_IO.cpp`, `src/io/ScenarioScript_AreaRectangleExtract_IO.h`/
`ScenarioScript_AreaImport_IO.h`, `sangen_arch_pack/specs/MAP_SCENARIO_SPEC.md`, and the real
`map_scripts_backup/Pandemonium Isthmus_Scenarios_Script.lua.officialbak` before ruling — every live
`COUNT_SCENARIOS` match function in that file (`1v1`, `2h1ai`, `4human`, `1h3ai`, `6total`, `2hRestAI`,
`floor169`, `slots5to8AnyFilled`) was confirmed to fit one of the two closed shapes below with zero
exceptions.

**Amendment (2026-09-11, same day): the original "facingDegrees is inert, no confirmed consumer"
ruling below was FACTUALLY WRONG and is corrected in place.** A follow-up investigation, independently
re-verified before this correction was recorded (not taken at face value), found real engine-level
rotation support this pass missed. See "Corrections to the request" item 1 for the corrected ruling
and ground truth; `ScenarioUnitPlacement` now carries `UnitTransform`-shaped quaternion rotation
fields instead of the retracted ad-hoc `facingDegrees` float, and the runtime algorithm forwards them.
The rest of this section (Part B's three import shapes) is unaffected by this correction.

**Amendment 2 (2026-09-12): this section's own two binding Lua snippets contradicted each other on
`rotation`'s wire shape, and STEP263's coder built BOTH exactly as written — correctly declining to
silently reconcile a discrepancy in binding ARCH text — leaving the contradiction live in the tree
(the Lua-render leg emitted a nested `rotation = {x=,y=,z=,w=}` sub-table; the runtime read flat
`placement.rotationX/Y/Z/W`, always `nil` against that nested shape). Independently re-verified
against the committed source before ruling (not taken on the report's word): confirmed exact in
`src/io/ScenarioScript_DataLua_IO.cpp` lines 92-110 (`BuildUnitPlacementRowBodies`, nested) versus
`resources/lua/SanGenScenarioRuntime.lua` lines 375-392 (`Scenario.SpawnBakedUnitPlacements`, flat).
**RULED: FLAT wins — the render leg is wrong, the runtime is right; the render leg is the one that
changes.** Rationale: `Params::ScenarioUnitPlacement` (`Scenario_PARAMS.h`) already stores flat
`rotationX/Y/Z/rotationW`; the already-shipped `.sanmap` JSON leg (`MapExporter_ScenarioRecord_IO.cpp`,
STEP260) already renders flat PascalCase siblings `RotationX/RotationY/RotationZ/RotationW` with an
explicit comment recording the deliberate choice ("NO nested Position/Rotation sub-object"); and the
runtime consumption code this same section specifies is flat. Three of four legs already agreed —
the Lua-render leg was the outlier, and outnumered-outlier is the right one to fix, not a coin flip.
The "Export/wire shape" paragraph below is corrected in place to specify flat
`rotationX=/rotationY=/rotationZ=/rotationW=` sibling keys on the Lua-render leg, matching the
`positionX`→bare-`x` precedent's OWN reason for existing (a flat, unprefixed `x/y/z/w` would collide
with the row's own position `x`/`y`/`z` keys — the prefixed form is what avoids that collision, which
is presumably why the runtime snippet was written prefixed-flat in the first place; the render leg's
nested form was this section's own drafting error, not a considered alternative). **Concrete coder
fix, narrow and mechanical:** in `BuildUnitPlacementRowBodies`
(`ScenarioScript_DataLua_IO.cpp:92-110`), replace the single `row += "rotation = { x = ... }";` line
with four flat `row += "rotationX = " + RenderLuaNumber(placement.rotationX) + ", ";` (and Y/Z/W
siblings, comma-joined, matching every other field in the same row) — no change needed anywhere in
`SanGenScenarioRuntime.lua`, which already reads the correct shape. `ScenarioScript_DataLua_IO_Test.cpp`'s
matching assertion(s) need the same flat-shape update. Part B's Shape 3 (foreign-file import grammar)
is UNAFFECTED and deliberately NOT changed to match — it describes a plausible hand-authored INPUT
shape from a foreign file SanGen never writes, independent of SanGen's own render-leg OUTPUT
convention; extracting a nested foreign `rotation = {...}` into the flat PARAMS fields is not a
contradiction; the extractor's output is the same flat struct either way.

#### Part A — new PARAMS type `Params::ScenarioUnitPlacement`, additive

```
struct ScenarioUnitPlacement {
    std::string armyName;             // "ARMY_XX" — resolved to a live Armies-table index at
                                       // spawn time (see runtime algorithm below), never stored
                                       // as a raw armyIndex integer (see "Corrections" item 3)
    std::string templateIdentifier;   // tpId, e.g. "ucn3001" — verbatim, same field name
                                       // `Scenario.SpawnUnits`'s instructions already use
    float positionX = 0.0f;
    float positionY = 0.0f;           // world height — SnapToGround if 0 (MAP_UNIT_SPAWNING_SPEC §5)
    float positionZ = 0.0f;
    float rotationX = 0.0f;           // quaternion (x, y, z, w) — SAME field names, types, and
    float rotationY = 0.0f;           // identity default as Params::UnitTransform
    float rotationZ = 0.0f;           // (Army_PARAMS.h) — a deliberate field-shape mirror, not a
    float rotationW = 1.0f;           // type-reuse: ScenarioUnitPlacement stays its own distinct
                                       // type (armyName/templateIdentifier have no UnitTransform
                                       // analog, and per item 1 below, no scale field is carried)
};
```

Added to `ScenarioBody`: `std::vector<ScenarioUnitPlacement> unitPlacements;` — additive, always
legal empty, same posture as every other §15.5-family vector field.

**Corrections to the request:**

1. **CORRECTED 2026-09-11 (same day) — rotation is a real, engine-supported, wired concept for units;
   the original ruling's "no confirmed consumer" characterization was wrong and is retracted.**
   Independently re-verified against the vendored game's own Lua tree
   (`E:\...\Sanctuary Shattered Sun Demo\`), not taken on a relayed report's word:
   - `engine/LJ/lua/host/generated/ffi/luaToEngineDelegates.lua:128`'s native delegate typedef —
     `typedef int32_t (*CreateUnit)(int32_t typeID, int32_t army, float3 location, quaternion
     rotation, float constructionProgress);` — and the matching documented signature,
     `Engine.CreateUnit(typeId, army, position, orientation, buildProgress)`
     (`documentation/engineFunctions.lua:271`), both carry a genuine native quaternion orientation
     parameter for units. This is not a documentation aspiration: the engine's own unit-creation
     path, `engine/LJ/lua/host/units/unitsUtilities.lua:15-30` (`_G.CreateUnit(armyId, tpId,
     position, orientation, progress)`), defaults `orientation` to `GetIdentityQuaternion()` when
     omitted and forwards it, unmodified, straight into `Engine.InstantiatePrefab(...,
     orientation, ...)` — rotation is wired end-to-end.
   - By contrast, that SAME call site hardcodes `EngineClasses.float3(1.0, 1.0, 1.0)` for scale on
     every unit, unconditionally — never a per-unit value. This is the real, verified distinction
     between rotation and scale for units: rotation has a genuine, always-present parameter and
     data path; unit scale has **no consumer anywhere in the unit-creation pipeline**, confirmed by
     direct read, not merely "not observed." (Props/decals are different — `common/mapUtils.lua:98,173`
     pass a real per-instance `transformData.scale` into `Engine.InstantiatePrefab`, so scale genuinely
     is wired for those domains, just never for units.) This is why `ScenarioUnitPlacement` gains
     rotation fields but still, correctly, carries no scale field.
   - The reason today's scenario-spawned units end up unrotated in practice is a narrower, shallower
     gap than "unsupported": `common/gameUtils.lua:386` and `:453` — the wrapper functions the base
     game's own map-authored unit-spawn helpers call — invoke `CreateUnit(armyIndex, unitData.tpId,
     table.deepCopy(unitData.position)) -- TODO: rotation`, a 3-argument call that leaves the
     available 4th `orientation` parameter unpassed, with the gap explicitly acknowledged in a
     comment. `Scenario.SpawnUnits`'s own `CreateUnit` call (this pack's runtime, mirroring the real
     reference `Scenarios_Script.lua`'s identical 3-argument call) has the same shallow gap. **This is
     a fact about specific Lua call sites in the game's current code, not a permanent architectural
     dead end** — exactly the distinction Constitution §6's no-guessing discipline requires getting
     right before ruling a field "inert." `ScenarioUnitPlacement` therefore carries real rotation data
     from day one, and the runtime algorithm below closes this exact gap for SanGen-baked placements
     specifically (it does not and cannot fix `gameUtils.lua`'s own unrelated call sites, which are
     hand-authored game code outside SanGen's `_Scenarios_Runtime.lua`).
2. **`ScenarioUnitPlacement` does NOT replace `Params::ScenarioBody::spawnsUnits` — it is additive
   and independent.** The request called `spawnsUnits` "a bare bool" to be replaced; that is not what
   ground truth shows. `spawnsUnits` is load-bearing today: `ScenarioScript_CategoryFourExport_IO.cpp:50`
   (`if (!body.spawnsUnits) return;`) gates whether SanGen scaffolds/exports the category-4
   hand-authored `<MapName>_Scenarios_<ScenarioName>.lua` generator file at all, and
   `ScenariosTab_Detail_UI.cpp:134` exposes it as a real "Spawns Units" checkbox. Retiring it would
   silently break the existing procedural-generator escape hatch (`ARCH_15_04`'s category 4) that
   future scenarios still need for placement logic too dynamic to hand-bake (arbitrary player-count
   scaling, live terrain queries, etc. — exactly what item 3b below keeps out of scope for baking).
   The two mechanisms coexist and are independently optional: a scenario may have `spawnsUnits=true`
   (category-4 generator), a non-empty `unitPlacements` (baked literal spawns), both, or neither.
3. **Why `armyName`, not the executor's raw `armyIndex`.** The real reference's `CreateUnit(armyIndex, ...)`
   call passes the `pairs(Armies)` iteration key directly — the same file's own
   `BuildSlots5to8Instructions` treats that key as a distinct, opaque handle from the `ARMY_XX` slot
   number (it re-derives `slotNumber` from `army.name:match("^ARMY_0?(%d+)$")` separately rather than
   assuming `armyIndex == slotNumber`). Storing a raw `armyIndex` in PARAMS would be storing an
   unstable runtime artifact, not an identity. `armyName` is safe, stable, and consistent with every
   sibling type in this family (`ScenarioSpawnPoint.armyName`, `ScenarioAlloyOverride.armyName`).

**Runtime consumption — specified now, binding, per the §15.12/§15.13 precedent of closing an
algorithm gap in the same ratification that creates the need for it (a Generator/IO-Architecture-Expert
work-order still builds it; this is not built by this ruling itself). AMENDED 2026-09-11 to forward
rotation, closing the gap item 1 above identifies as narrow and fixable within this pack's own
runtime — never touching `gameUtils.lua`, which is outside SanGen's ownership. CONFIRMED CORRECT,
UNCHANGED, by Amendment 2 (2026-09-12) — this is the flat shape that wins; only the renderer below
was wrong:**

```lua
local function ResolveArmyIndexByName(armyName)
    for armyIndex, army in pairs(Armies) do
        if army.name == armyName then return armyIndex end
    end
    return nil
end

function Scenario.SpawnBakedUnitPlacements(scenario)
    if not scenario.unitPlacements then return end
    local instructions = {}
    for _, placement in ipairs(scenario.unitPlacements) do
        local armyIndex = ResolveArmyIndexByName(placement.armyName)
        if armyIndex then
            instructions[#instructions + 1] = { armyIndex = armyIndex,
                templateIdentifier = placement.templateIdentifier,
                x = placement.x, y = placement.y, z = placement.z,
                rotationX = placement.rotationX, rotationY = placement.rotationY,
                rotationZ = placement.rotationZ, rotationW = placement.rotationW }
        else
            Warn("SANGEN: scenario unit placement named unknown army '"..tostring(placement.armyName)..
                 "' -- skipped.")
        end
    end
    Scenario.SpawnUnits(instructions)
end
```

`Scenario.SpawnUnits(instructions)` itself (the shared executor, also fed by category-4 hand-authored
generators) is widened to forward rotation, nil-safely so existing category-4 generator output —
which has never produced a rotation field and must keep working unchanged — falls back to identity,
exactly today's behavior:

```lua
function Scenario.SpawnUnits(instructions)
    local sinceYield = 0
    local placed, failed = 0, 0
    for _, instr in ipairs(instructions) do
        local rotation = (instr.rotationW ~= nil)
            and { x = instr.rotationX, y = instr.rotationY, z = instr.rotationZ, w = instr.rotationW }
            or IDENTITY_ROTATION   -- the file's own existing constant; unchanged default behavior
        local ok, unit = pcall(CreateUnit, instr.armyIndex, instr.templateIdentifier,
            EngineClasses.float3(instr.x, instr.y, instr.z), rotation)
        if ok and unit then placed = placed + 1 else failed = failed + 1 end

        sinceYield = sinceYield + 1
        if sinceYield >= UNIT_SPAWN_BATCH_SIZE then
            sinceYield = 0
            WaitTicks(1)
        end
    end
    Log(string.format("SANGEN: SpawnUnits placed %d, failed %d (of %d requested).",
        placed, failed, #instructions))
end
```

The plain `{x=,y=,z=,w=}` table shape for `rotation` (the LOCAL variable this function builds to hand
to `CreateUnit`, not the wire/render shape — see Amendment 2 above for why those are two different
things that must not be conflated) mirrors this SAME file's own already-shipped `IDENTITY_ROTATION`
constant verbatim (used today for marker-transform rotation tables consumed by `common/mapUtils.lua`'s
`Engine.InstantiatePrefab` calls) — the engine's FFI quaternion binding already accepts a plain
field-matching Lua table at that call shape, confirmed by that existing, already-shipped,
already-working code path; no new marshaling convention is introduced.

`Scenario.ResolveAndApply` must retain the full matched-scenario table (not just its name) in a new
upvalue, `currentMatchedScenario`, alongside the existing `currentMatchedScenarioName`.
`Scenario.SpawnMatchedScenarioUnits(area)` calls `Scenario.SpawnBakedUnitPlacements(currentMatchedScenario)`
first, unconditionally, THEN continues with its existing lazy category-4 `Import()` attempt exactly as
today — both mechanisms fire independently, per item 2 above.

**Export/wire shape (exact key spelling is the Format Expert's follow-up, per §16.4's precedent).
CORRECTED 2026-09-12 by Amendment 2 above — this paragraph originally specified a nested Lua-render
shape that contradicted the runtime snippet above; the flat shape below is now the sole binding text:**
additive `UnitPlacements` array on each scenario's wire object, PascalCase siblings
`ArmyName`/`TemplateIdentifier`/`PositionX`/`PositionY`/`PositionZ`/`RotationX`/`RotationY`/
`RotationZ`/`RotationW` — no `SanGenVersion` bump (purely additive, same precedent as `SpawnIds`/
`slotRangeStart`), already shipped this way in the `.sanmap` JSON leg (`MapExporter_ScenarioRecord_IO.cpp`,
STEP260). **The Lua-render leg (`ScenarioScript_DataLua_IO.cpp`) renders `x`/`y`/`z` bare (matching
this file's own existing `spawns`/`alloys` convention) PLUS `rotationX`/`rotationY`/`rotationZ`/
`rotationW` as FLAT SIBLING keys on the SAME row table — never a nested `rotation = {...}`
sub-table** (a nested sub-table was this section's own original text and is retracted: it would read
as `placement.rotation.x`, not `placement.rotationX`, contradicting the runtime consumption snippet
above, which is the one that is correct). The flat-prefixed form is not arbitrary: unlike position
(bare `x`/`y`/`z`, no prefix needed because nothing else on the row uses those bare names), a bare,
unprefixed rotation `x`/`y`/`z`/`w` WOULD collide with the row's own position `x`/`y`/`z` keys — the
`rotation`-prefix is what makes flat siblings collision-safe, and is why the runtime snippet above was
already written that way. `positionX`/`positionY` render unflipped, `positionZ` flipped (applying the
same `FlipPositionZ(positionZ, mapSize)` transform already applied to `spawns`/`alloys` positions,
`BuildPositionedRowBody`), and **rotation renders completely verbatim, untouched by any flip** — this
is not a new rule invented for this type; it is the SAME established law already stated identically in
`MapExporter_Armies_IO.cpp`/`MapExporter_Decals_IO.cpp`/`MapExporter_Props_IO.cpp`/
`MapExporter_Markers_IO.cpp` ("positionX/positionY and rotation/scale are untouched by the flip"),
confirmed by direct read of all four before writing this rule, extended here to a fifth entity kind
for the first time with no new reasoning required.

#### Part B — three narrow, permanently-bounded FOREIGN-`.lua` import shapes

All three reuse §15.11's load-bearing machinery verbatim: the `kScenarioGeneratedFileBannerLine` +
SanGen-owned-filename refusal guard (item 1), human-triggered/one-shot/no-live-binding invocation
(item 8), additive-never-destructive reconciliation with a logged collision policy (item 9),
Constitution §6 byte/count caps (item 10), no-round-trip-framing (item 11), and the `IO`/
`ScenarioScript_*_IO` family placement, never `MapImporter_*` (§15.2). **No execution, no LuaJIT, ever,
on any of these three paths** — §15.11 item 3's ban is repeated here by name, not by reference, because
this is the section most likely to be misread as license to "just run `LuaTableEvaluate_SYS` on it" —
it is not, for the same instant-`Import(...)`-error reason item 3 already gives.

Each shape is its OWN narrow, single-purpose grammar/extractor — they are never combined into "read
a whole scenario entry." A combined reader would be indistinguishable from "the reader half of the
exporter," which §15.11 item 11 permanently forbids naming this family as. Working names:
`ScenarioScript_MatchConditionExtract_IO`, `ScenarioScript_SlotPatternExtract_IO`,
`ScenarioScript_UnitPlacementExtract_IO` (all pure, disk-free, mirroring
`ScenarioScript_AreaRectangleExtract_IO`); exact file split / whether they share one disk-touching
orchestrator alongside the existing `ScenarioScript_AreaImport_IO` is the IO Architecture Expert's call.

**Shape 1 — count/slot-range match conditions → `Params::ScenarioCountCondition[]`.**
Recognizes exactly two closed function-literal templates inside a `match = function(t, h, a, pattern)
return ... end` field (comments/strings skipped first, per §15.11 item 5):
- **Template A — AND-chain of `t`/`h`/`a` count comparisons.** `return COND (and COND)*` where each
  `COND` is `VAR OP N` (whitespace-tolerant, parens around the whole chain tolerated), `VAR` ∈
  `{t, h, a}`, `OP` ∈ the six literal spellings `==`/`~=`/`>`/`>=`/`<`/`<=` (mapping verbatim to
  `ScenarioComparator`), `N` a signed integer literal. One or more conjuncts; **`or` anywhere in the
  expression is an automatic near-miss** — not accepted, not partially extracted. `VAR` compared to
  another `VAR` (not a literal), arithmetic on `N`, or any identifier other than `t`/`h`/`a`/`pattern`
  is rejected. Each conjunct becomes one `ScenarioCountCondition{field=Total|HumanCount|AiCount,
  comparator, value=N}` (`t`→`Total`, `h`→`HumanCount`, `a`→`AiCount`); the whole chain becomes one
  scenario's `conditions` vector, AND semantics preserved verbatim (§15.5's own AND-only model).
  Confirmed to cover, byte-for-byte modulo whitespace, six of eight live entries: `1v1`, `4human`,
  `1h3ai`, `6total`, `2hRestAI`, `floor169`.
- **Template B — slot-range occupancy.** `return pattern:sub(N, M):find("[^-]") ~= nil` — every token
  except the two integers `N`/`M` must match this template byte-for-byte (method names, the exact
  string literal `"[^-]"`, the `~= nil` comparison); a different pattern string, a different method
  chain, or a negated form is a near-miss, never guessed. This fixed template has one, and only one,
  reproduction inside `ScenarioCountCondition`'s existing `field`/`comparator`/`value`/
  `slotRangeStart`/`slotRangeEnd` shape — `field=SlotRangeOccupiedCount, comparator=GreaterOrEqual,
  value=1, slotRangeStart=N, slotRangeEnd=M` — the unique pairing that reproduces "at least one slot in
  [N,M] is filled" exactly; the extractor never chooses a comparator/value itself, it always emits this
  one fixed pairing for this one fixed template. Confirmed to cover the eighth live entry,
  `slots5to8AnyFilled` (`pattern:sub(5,8):find("[^-]") ~= nil` → `slotRangeStart=5, slotRangeEnd=8`).
- **Absolutely not extracted, matching §15.11 item 2's standing language:** the surrounding scenario
  record's `name`, `area`, `spawnsUnits`, `alloyMode`, `spawns`, `alloys`, or
  `COUNT_SCENARIOS`/`PATTERN_SCENARIOS` array ordering — this extractor returns bare
  `ScenarioCountCondition` vectors, never a `CountScenario`, never a `ScenarioBody`. Wiring an
  extracted condition list to a name/area is a separate human authoring action in the Scenarios tab,
  not something this extractor infers or bundles.
- **Anything not matching Template A or B exactly is a near-miss** (§15.11 item 6's posture),
  including any future third shape a different map's file might use — widening to a Template C is a
  new ARCH ratification, not a coder's inference from "it looks similar."

**Shape 2 — exact `slotPattern` string → `PatternScenario::slotPattern`.** Inside a
`local <IDENT> = { { ... pattern = "STRING", ... }, ... }`-shaped top-level array-of-tables (the
`PATTERN_SCENARIOS` shape), extract the `pattern` field's string literal verbatim (no escape
processing beyond ordinary Lua string-literal decoding) alongside the sibling `name` field verbatim,
as a `{name, slotPattern}` pair. This is the lowest-risk of the three shapes — a bare string literal
read, the same risk class as the ratified area-rectangle extractor's numeric literals. Length/charset
validation against the map's authored `maxArmySlotCount` is an ordinary post-import validator
(`ScenarioSlotRangeValidation_IO.h`'s existing family), not this extractor's job — a too-long or
too-short pattern is imported as-authored and flagged by that existing validator, never silently
truncated/padded.

**Shape 3 — literal unit-placement tuples → `ScenarioUnitPlacement[]`.** Recognizes a top-level
`local <IDENT> = { { armyName = "S1", templateIdentifier = "S2", x = X, y = Y, z = Z }, ... }`-shaped
array of keyed table literals (the five position/identity keys required, in any order; numeric
literals only for `x`/`y`/`z`; quoted string literals for `armyName`/`templateIdentifier`; a missing
key, an extra key, or a non-literal value is a near-miss for that one row, not the whole array).
**Ground-truth caveat, stated plainly rather than assumed away: no file in this repository's
`map_scripts_backup/` corpus currently contains a literal table of this shape** — the real reference
computes every position procedurally (`BuildSlots5to8Instructions`) — so this extractor has zero
real-world exercise today. It is ratified as forward-looking infrastructure for (a) other foreign maps
that may hardcode positions directly, mirroring the historical `BATTLESHIP_POSITIONS` pattern this
same file's own comments describe as having once existed, and (b) hand-transcribed or
authoring-tool-produced literal files. **Binding field-mapping rule, the one genuinely new risk this
shape introduces beyond §15.11's existing model:** a literal `armyIndex` integer (the `pairs(Armies)`
runtime handle `CreateUnit`'s first argument actually uses) is NOT a stable `ARMY_XX` identity (Part A,
"Corrections" item 3) and must never be reinterpreted as one. **This extractor therefore only
recognizes rows that spell the army as a string key, `armyName = "ARMY_03"` (or equivalent) — never a
bare `armyIndex` integer.** A candidate row keyed by `armyIndex` instead of `armyName` is rejected as a
near-miss naming exactly this ambiguity, never guessed across. Lua `x`→`positionX` and `y`→`positionY`
verbatim (no arithmetic); Lua `z`→`positionZ` via `mapSize - z - 1` (self-inverse of `FlipPositionZ`,
Part A) — **this extractor is therefore not context-free text-in/values-out like the area-rectangle
extractor; it additionally takes the target map's `mapSize` as an ordinary scalar parameter**, still
performing no filesystem access and no Lua execution. An optional sixth key, a nested
`rotation = { x = RX, y = RY, z = RZ, w = RW }` table (all four sub-fields required together if the
key is present at all — no partial rotation), maps verbatim (no flip, per Part A) to
`rotationX/Y/Z/rotationW`; its absence is not a near-miss — it defaults to identity `(0,0,0,1)`, since
no known foreign shape (including the live reference) currently emits a literal rotation for units at
all (Part A item 1's own finding). **This nested INPUT grammar is deliberately independent of, and
unaffected by, Amendment 2's flat-sibling ruling for SanGen's own render-leg OUTPUT above (2026-09-12)
— a foreign hand-authored file's shape and SanGen's own generated-file shape are two separate
questions; extracting a nested foreign literal into the same flat `ScenarioUnitPlacement` fields is
not a contradiction, since both legs' OUTPUT is the identical flat struct.**

**Reaffirmed absolute, unchanged by this section (§15.3/§15.11's language stands):** `spawns`/`alloys`/
`alloysToAdd`/`alloysToRemove`/`alloyMode`/`area` assignment/`COUNT_SCENARIOS` ordering/any `or`-bearing
or otherwise non-Template-A/B boolean expression/any procedural placement logic
(`FindFleetAnchorForArmy`, `FindDeepestWaterNear`, `AppendUnitGrid`, `GetSpiralGridXZ`, or any future
map's equivalent live terrain/water search) — none of these may be extracted, inferred, reproduced as a
declarative system, or captured by any general fallback mechanism, per the human's own explicit item 3b
scoping. Widening any boundary in this section, or in §15.11, is a new ARCH ratification.

**Downstream work this implies — flagged, not resolved here (per the §15.7/§16.10 routing precedent):**
- **Immediate, narrow coder follow-up (per Amendment 2, 2026-09-12):** fix
  `BuildUnitPlacementRowBodies` in `ScenarioScript_DataLua_IO.cpp` to emit flat
  `rotationX=/rotationY=/rotationZ=/rotationW=` sibling keys instead of a nested `rotation = {...}`
  sub-table; update `ScenarioScript_DataLua_IO_Test.cpp`'s matching assertion(s) to match. No change
  needed to `SanGenScenarioRuntime.lua`, `Scenario_PARAMS.h`, or the `.sanmap` JSON leg — all three
  are already correct.
- Format Expert: `MAP_SCENARIO_SPEC.md`/`SANMAP_FORMAT_SPEC.md` catch-up for `UnitPlacements`'s wire
  shape and key spelling.
- `Params::ScenarioUnitPlacement` implementation in `Scenario_PARAMS.h`, `ScenarioBody::unitPlacements`
  — **already shipped (STEP260/STEP263), confirmed matching this section as amended.**
- `MapExporter_Scenarios_IO`/`MapImporter_Scenarios_IO` JSON leg — **already shipped (STEP260),
  confirmed flat and correct;** `ScenarioScript_DataLua_IO.cpp` Lua-render leg — shipped (STEP263) but
  needs the narrow fix named above.
- Three new pure extractor IO units (Part B) plus whichever disk-touching orchestrator the IO
  Architecture Expert designs to invoke them alongside `ScenarioScript_AreaImport_IO` — **partially
  shipped** (`ScenarioScript_MatchConditionExtract_IO.h`, `ScenarioScript_SlotPatternExtract_IO.h`
  exist in-tree; confirm against this section, not re-litigated here).
- UI: a Scenarios-tab editor surface for `unitPlacements` — **already shipped**
  (`ScenariosTab_DetailUnitPlacements_UI.cpp`); not re-reviewed in this pass.
- **New, opened by the 2026-09-11 rotation amendment:** whether `gameUtils.lua:386,453`'s own "TODO:
  rotation" gap is ever worth fixing is entirely the base game's call, not SanGen's — flagged here
  only so it is not mistaken for a SanGen defect; SanGen's own runtime (`Scenario.SpawnUnits`) now
  forwards rotation correctly for everything SanGen itself spawns, once the renderer fix above lands.

**For the human, before implementation starts:** none outstanding on the design; the narrow renderer
fix named above is dispatchable as a small, self-contained coder work-order with no further ARCH
input needed.
