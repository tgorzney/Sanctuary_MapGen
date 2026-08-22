# STEP62 — `bReclaimable` on `PropRule`/`PropInstanceGroup`, the template-level reclaim flag

**Layer:** PARAMS + IO. **Domain:** `Params::PropRule`, `Params::PropInstanceGroup`,
`Io::MapExporter_PropsStack_IO`/`MapImporter_PropsStack_IO`, `Io::MapExporter_Props_IO`/
`MapImporter_Props_IO`. **Sequence:** transcribes an ARCH ruling ratified this session (§14.2's
Reclaim domain row, updated per this ticket); precedent is §14.6.

## Root problem
`ARCH_14_02_DataModel.md` §14.2's domain table currently reads, for Reclaim:
> n/a — no data yet | n/a — no rule type yet; slot reserved, zero cost until it ships

`OverlayDomainKind_UI::Props` and `::Reclaim` are both already declared in the §14.2 enum
(`Alloy, SpawnsArmies, Units, Props, Reclaim, Decals`), but nothing distinguishes a reclaimable
prop from a decorative one anywhere in `recipe.propRules`/`recipe.props` today — there is no field
to partition on. Real game prop blueprints carry a `tags` array containing `"HARVESTABLE"` plus an
`economy.harvest{alloys, plasma|energy}` yield table (Format Expert consult, this session) —
confirming reclaim-ability is a property of the blueprint template, not of an individual placed
instance. SanGen's own representation does not need to mirror the tag/yield shape 1:1; a single
bool is sufficient for this ticket's scope (see Out of scope).

## Fix
Add `bool bReclaimable = false;` to two structs, rule-level and blueprint-group-level — never to
`PropTransform` (the per-instance placement record), matching the real-game "reclaim is a
blueprint property" semantics stated above.

### 1. `src/params/ScatterRule_PARAMS.h` — `Params::PropRule`
Current fields (verified): `bEnabled, density, minSlope, maxSlope, minHeight, maxHeight,
bAvoidWater, bNearCliffs, spacingMinimum, mapEdgePadding, maskStratumIndex, maskWeightMinimum,
obstacleDistanceMinimum, nearCliffDistanceMaximum, bSymmetryUseGlobal, symmetryMask,
radialSymmetryRepeatCount, transform`. Add, next to `bNearCliffs` (same "gate flag" grouping):
```cpp
bool  bReclaimable = false;   // template-level: does this rule's placed instances belong to the
                               // Reclaim overlay sub-layer partition, not the Props one (§14.2/§14.6)
```

### 2. `src/params/PropInstance_PARAMS.h` — `Params::PropInstanceGroup`
Current fields (verified): `std::string blueprintPath; std::vector<PropTransform> transforms;`.
Add:
```cpp
struct PropInstanceGroup { std::string blueprintPath; bool bReclaimable = false; std::vector<PropTransform> transforms; };
```
`PropTransform` (`{ InstancedTransform transform; int layerIndex = 0; }`) is untouched — confirmed
this is the per-instance record; reclaim-ability does not belong there per the ruling.

### 3. Wire representation — corrects the ticket's own assumed key names
⚠️ The ticket description that produced this work-order guessed the two JSON containers were named
`PropRules`/`PropGroups`. Verified against real code, both are wrong:
- `Params::PropRule` entries serialize to the top-level `"PropsStack"` array
  (`MapExporter_PropsStack_IO.cpp::BuildPropsStackJson`, `MapImporter_PropsStack_IO.cpp::
  ReadPropsStackJson` via `ReadRuleArray(document, "PropsStack", ...)`), **not** `"PropRules"` —
  that string does not exist anywhere in the codebase.
- `Params::PropInstanceGroup` entries serialize to the top-level `"props"` array (lowercase;
  `MapExporter_Props_IO.cpp::BuildPropsJson`, `MapImporter_Props_IO.cpp::ReadPropsJson`).
  `"PropGroups"` (PascalCase) is a **different, unrelated** type —
  `Params::PropInstanceLayer` (`name`/`color`/`iconScale`, the manual-layer metadata record,
  §14.5) — and is not touched by this ticket.

Boolean wire-key casing convention on `PropRule`: existing bool fields drop the `b` prefix and go
PascalCase — `bAvoidWater` -> `"AvoidWater"`, `bNearCliffs` -> `"NearCliffs"`
(`MapExporter_PropsStack_IO.cpp:18`, `MapImporter_PropsStack_IO.cpp:17-18`). `bReclaimable` follows
the same convention: wire key `"Reclaimable"`, on both containers.

**`MapExporter_PropsStack_IO.cpp` (`BuildPropRuleJson`)** — add, next to the existing
`AvoidWater`/`NearCliffs` pair:
```cpp
json["Reclaimable"] = rule.bReclaimable;
```
**`MapImporter_PropsStack_IO.cpp` (`ReadPropRuleJson`)** — add, next to the existing
`ReadJsonBoolean(json, "AvoidWater", ...)`/`"NearCliffs"` pair:
```cpp
ReadJsonBoolean(json, "Reclaimable", rule.bReclaimable);
```
**`MapExporter_Props_IO.cpp` (`BuildPropsJson`)** — add to `groupJson`, next to `blueprintPath`:
```cpp
groupJson["Reclaimable"] = group.bReclaimable;
```
**`MapImporter_Props_IO.cpp` (`ReadPropInstanceGroupJson`)** — add, next to the existing
`ReadJsonText(json, "blueprintPath", group.blueprintPath)`:
```cpp
ReadJsonBoolean(json, "Reclaimable", group.bReclaimable);
```

### Import default/clamp — Constitution §6, no new error handling
`ReadJsonBoolean` already implements "missing/wrong-typed key leaves `destination` at its
caller-supplied value" (`JsonPrimitives_IO.h:36-40`: returns `false` and does not touch
`destination` when the key is absent or not a JSON boolean). Since `bReclaimable`'s field default
is `false` and both readers run against a freshly-default-constructed `PropRule`/
`PropInstanceGroup`, a missing key on import silently resolves to `false` — never a refusal, never
new bespoke handling. This is the exact posture every other optional `PropRule`/
`PropInstanceGroup` field already gets (e.g. `AvoidWater`, `blueprintPath`'s own absence — no
special-cased "missing means reject the group" branch exists), and matches Constitution §6's
"validate-then-default-then-log" posture for unreliable pre-alpha input; a schema-version refusal
never applies to an ordinary optional field.

### `ARCH_14_02_DataModel.md` §14.2 table update
Update the Reclaim row (the only row this ticket changes) from:
```
| Reclaim | n/a — no data yet | n/a — no rule type yet; slot reserved, zero cost until it ships |
```
to:
```
| Reclaim | `recipe.props[i]` where `bReclaimable == true` | `recipe.propRules[i]` where `bReclaimable == true` |
```
and correct the `Props` row's data source description alongside it (same table, adjacent row) to
note the now-mutually-exclusive partition:
```
| Props | `recipe.propLayers[i]` (`PropInstanceLayer`) — sub-layer set unchanged; each sub-layer's resolved instances now filtered to `recipe.props[i].bReclaimable == false` | `recipe.propRules[i]` filtered to `bReclaimable == false` |
```
This transcription belongs to the ARCH Expert to actually apply to `ARCH.md` (this agent never
writes `ARCH.md` — Constitution law, `CLAUDE.md`); recorded here as the exact text so it isn't
re-derived or drifted when applied.

## Why a partition, not an overlap (transcribed ARCH reasoning, §14.6 precedent)
`Props`/`Reclaim` must mutually-exclusively partition `recipe.propRules`/`recipe.props` by
`bReclaimable` — a Props sub-layer reads entries where `bReclaimable == false`, a Reclaim sub-layer
reads entries where `bReclaimable == true`, never both. This is the **exact same pattern** §14.6
already ratified for `Alloy`/`SpawnsArmies`, which mutually-exclusively re-slice the existing
`markers` buffer by its existing `category` column ("A UI enum may re-slice an existing DATA
collection by its own field without the DATA shape changing"). An overlapping design (both
domains reading every entry regardless of `bReclaimable`) would double-count the same
`PlacementInstance` across two simultaneously-visible `OverlayLayer_UI`s when both `Props` and
`Reclaim` layers are enabled at once — inflating the cross-layer visible-vertex budget (§14.9's
mandatory decimation threshold) with the same instance counted twice for no representational
benefit. Per §14.6's own asymmetry warning, `domain == DATA-bucket identity` must never be assumed:
`Props`/`Reclaim` partition **one** DATA collection (`propRules`/`props`) two ways, exactly as
`Alloy`/`SpawnsArmies` partition `markers` two ways — a coder must not treat this as two separate
underlying arrays.

## Zero rendering/overlay consumer in this ticket
This ticket lands the field and its IO round-trip only. `OverlaySubLayerRef_UI`/
`OverlayLayer_UI` (`work_orders/STEP51_OverlayLayerDataModel_UI.md`) already anticipate the
Props/Reclaim domain split conceptually (§14.2's enum already has both variants), but the actual
filter-by-`bReclaimable` query that resolves a `Reclaim` sub-layer's instance set belongs to
STEP53's draw pass (or its CSR bucket build, STEP50) as a follow-up — **not** this ticket. This
ticket has no dependency on STEP50/STEP51/STEP53 landing first; it is a standalone PARAMS+IO
addition that those tickets will later consume.

## Files touched
- `src/params/ScatterRule_PARAMS.h` — `bReclaimable` on `PropRule`
- `src/params/PropInstance_PARAMS.h` — `bReclaimable` on `PropInstanceGroup`
- `src/io/MapExporter_PropsStack_IO.cpp` — `"Reclaimable"` write in `BuildPropRuleJson`
- `src/io/MapImporter_PropsStack_IO.cpp` — `"Reclaimable"` read in `ReadPropRuleJson`
- `src/io/MapExporter_Props_IO.cpp` — `"Reclaimable"` write in `BuildPropsJson`
- `src/io/MapImporter_Props_IO.cpp` — `"Reclaimable"` read in `ReadPropInstanceGroupJson`

## Out of scope
- Any overlay/draw-pass filter query consuming `bReclaimable` (STEP50/STEP53).
- `ARCH_14_02_DataModel.md` §14.2 table text edit itself (ARCH Expert only; exact replacement text given above).
- Auto-populating `bReclaimable` from a future blueprint `tags`/`HARVESTABLE` import — whether the
  bool is later derived from real game blueprint data or hand-toggled in a future Props tab UI
  widget is explicitly undecided here.
- Any UI checkbox/widget exposing `bReclaimable` for authoring (`PropsTab_Rules_UI.cpp`,
  `PropsTab_Manual_UI.cpp`) — not requested by this ticket; PARAMS+IO only.
- `Placement_Hash_PROC.cpp`'s rule-change hash (`HashInteger(seed, (rule.bEnabled ? 1 : 0) |
  (rule.bAvoidWater ? 2 : 0) | ...)`) deliberately does **not** gain a `bReclaimable` bit:
  `bReclaimable` does not affect where/how a prop is scattered (no placement-math input), only its
  overlay-domain classification — including it in the placement-dirty hash would trigger a needless
  Tier-A full regen (§14.8) for a value that changes zero scatter output. If a future ticket proves
  this wrong, that is its own ARCH-routed change, not a silent addition here.
- `PropTransform`/`DecalTransform`/`DecalRule`/`DecalInstanceGroup` — this ticket is Props-domain
  only; Decals were not named in the ARCH ruling this transcribes.

## Verify
- `src/params/PlacementRules_PARAMS_Test.cpp` — extend the existing `PropRule prop;` default check:
  `if (prop.density != 0.5f || prop.bAvoidWater || prop.bNearCliffs || prop.bReclaimable)`, same
  "FAIL prop defaults" branch (defaults must be `false`).
- `src/io/MapImporter_IO_Test.cpp` — extend `CheckPropsAndDecals`'s existing prop-rule fixture to
  set `bReclaimable = true` on the fixture `PropRule` and assert
  `loaded.propRules[0].bReclaimable` survives the round trip (same pattern as the existing
  `loaded.propRules[0].bAvoidWater` check at line 144).
- `src/io/MapImporter_PropsDecals_IO_Test.cpp` — extend `BuildFixtureRecipe`'s `propGroup` with
  `propGroup.bReclaimable = true;` and assert `loadedGroup.bReclaimable == originalGroup.bReclaimable`
  alongside the existing `blueprintPath` check (~line 543-544).
- A missing `"Reclaimable"` key on an otherwise-valid `PropsStack`/`props` entry (i.e. every
  pre-existing `.sanmap` fixture in the test suite that predates this ticket) must still parse
  clean with `bReclaimable == false` and zero new warnings — run the full existing
  `MapImporter_IO_Test`/`MapImporter_PropsDecals_IO_Test`/`PlacementRules_PARAMS_Test` suites and
  confirm all pre-existing cases stay green with no edits beyond the two extensions above.

## Performance estimate
Two `bool` fields (1 byte each, padded) plus four `ReadJsonBoolean`/direct-assignment lines — O(1)
per rule/group, no measurable cost against any existing benchmark; no basis-tag benchmark needed
per Constitution §7 (a same-shape sibling of `AvoidWater`/`NearCliffs`, whose cost is already
absorbed into the existing `PropRule`/`PropInstanceGroup` read/write paths).
