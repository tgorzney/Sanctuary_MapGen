# ENTITY_AUTHORING_PARAMS_SPEC — pass-through entity PARAMS: armies, unit groups/transforms, map areas

Source of truth: `SanMap.Types.cs` (`Sanmap File Format\SanMap.Types.cs`, the `EM.Map` namespace —
confirmed byte-identical to `Sanctuary-Map-Generation-develop\Sanctuary\*.cs`; **NOT**
`Sanctuary-Map-Generation-develop\src\*` / the `ExtraneousMapGen` namespace, an unrelated
third-party tool — never read or cite that tree for this domain). `SANMAP_FORMAT_SPEC`'s "Entity
collections" section documents the wire shape; this spec documents the concrete `Params::` C++
shape and the naming derivation (ARCH §1.8) that produced it.

## Scope — why this is a separate type family from `PLACEMENT_SCATTER_SPEC`
`Params::MarkerRule`/`PropRule`/`DecalRule`/`UnitRule` (`PLACEMENT_SCATTER_SPEC`) are **procedural
placement RULES** — a PROC stage (`Placement_PROC`) reads them and computes where entities land.
`Params::Army`/`UnitGroup`/`UnitTransform`/`MapArea` (this spec) are the opposite kind of data:
**manually-placed, human-authored entities** a designer positions directly (mouse clicks on the
canvas, or values imported unchanged from an existing `.sanmap`). No PROC stage computes or
reinterprets them — round-trip fidelity is their entire purpose (ARCH §1.8, "pass-through"
bucket). The two mechanisms coexist: an `ArmiesTab_UI` row can show both hand-placed units
(`Army.groups`, this spec) and the procedural `UnitRule`s that spawn more units for the same army
(`ScatterRule_PARAMS.h`) — two independent producers of the same `armies[]` roster, exactly as v1
had.

This closes the confirmed gap flagged in `work_orders/RECIPE_PARITY_BACKLOG.md` Tier 1 ("Areas" —
"no `Params::MapArea` type; the area rectangles don't round-trip") and in the standing scope notes
of `ArmiesTab_UI.h` ("AN ARMY HAS NO `_PARAMS` HOME... A durable `Army_PARAMS` is its own
work-order") and `AreasTab_UI.h` ("AN AREA HAS NO `_PARAMS` HOME... a durable `MapArea_PARAMS`...
is its own work-order") — both written under ARCH §8.4 ("a coder never invents a missing type").
This ratification is that missing ARCH ruling; the follow-on coder work-order wires
`MapRecipe_PARAMS.h` to hold these types and retires those two UI scope notes.

## Structural ruling — recursive tree preserved, dictionaries become vectors + `name`
The format's `Army → UnitGroup → UnitGroup*/UnitTransform*` shape is a genuinely recursive tree (a
`UnitGroup` nests further `UnitGroup`s). SanGen's usual argument against nested containers (the
optimization pillars) governs **hot per-cell DATA** — this is human-edited PARAMS, touched on
mouse clicks, at counts in the tens/hundreds, never the millions. The tree shape is kept as-is.

Every `Dictionary<string, X>` becomes `std::vector<X>` with the dictionary key folded in as a
`name` field on `X` — already the live choice for `Area` (`AreasTab_List_UI.h`'s
`MapAreaRectangle`), applied one level deeper here for `Army.groups` and
`UnitGroup.units`/`UnitGroup.groups`. `name` is the export round-trip identity (the JSON key), not
decoration: `MakeAreaNamesUnique` (`AreasTab_List_UI.h`) is the existing precedent for keeping it
unique on edit, and the live engine's `GameUtils.GetArea(name)` (`UNIT_PROP_MARKER_DATA_SPEC`)
confirms `MapArea.name` specifically is a load-bearing gameplay identifier, not cosmetic.

## The naming derivation (ARCH §1.8 applied)
All four types sit in the "pass-through" bucket: format spelling by default, case-converted, with
named exceptions. See ARCH §1.8 for the general rule; this table is its application here.

| Format field | PARAMS field | Rule applied |
| --- | --- | --- |
| `Area.x` | `originX` | §1.8 named exception — texture-space origin vs. `positionX/Y/Z` world convention |
| `Area.y` | `originZ` | same; the format's 2D `y` maps to SanGen's world `z` (ground plane), never `y` (elevation) |
| `Area.width` | `width` | verbatim — no ambiguity |
| `Area.height` | `length` | §1.8 named exception — format's Z-extent/depth, not elevation |
| `Army.faction` | `faction` | verbatim word, retyped `enum class Faction` (§1.8 named exception) |
| `Army.alloys` | `alloys` | verbatim |
| `Army.energy` | `energy` | verbatim |
| `Army.groups` (dict) | `groups` (vector) | verbatim name; dict→vector (structural ruling above) |
| `UnitGroup.units` (dict) | `units` (vector) | verbatim name; dict→vector |
| `UnitGroup.groups` (dict, nested) | `groups` (vector, nested) | verbatim name; dict→vector |
| `UnitTransform.type` | `legacyTypeTag` | NOT §1.8 verbatim — see "`type` → `legacyTypeTag`" below |
| `UnitTransform.tpid` | `templateIdentifier` | §1.8 named exception |
| `InstancedTransform.position` | `positionX/Y/Z` | matches the established SanGen world convention (`PlacementInstance_DATA.h`) rather than introducing a `Vector3` PARAMS type |
| `InstancedTransform.rotation` | `rotationX/Y/Z/W` | quaternion, same convention |
| `InstancedTransform.scale` | `scaleX/Y/Z` | same convention |

No `Vector3`/`Quaternion` math type exists in `src/math/` today (confirmed by grep), and none of
the flat-scalar PARAMS/DATA types built so far (`Data::PlacementInstance`,
`Erosion_Droplet_PROC.h`'s droplet state) use one; introducing one now is out of scope for a types
ratification and would itself be a §8.4 violation in reverse (inventing a type nobody asked for).
`UnitTransform` therefore follows the same flat-scalar convention its DATA-layer sibling
`Data::PlacementInstance` already uses.

## `armyColor` / `alias` — the two already-ratified `armies[key]` additions
`SANMAP_FORMAT_SPEC` Correction 11 already ratified `armyColor` and `alias` as SanGen-added,
lowerCamelCase siblings merged into the format's own `armies[key]` dictionary entry — they exist in
the schema today, just with no `Params::Army` to live on. This ratification gives them that home;
it does not invent them. `Params::Army` therefore carries the format-native fields (`faction`,
`alloys`, `energy`, `groups`, plus the folded-in `name`) and the two SanGen-added siblings
(`armyColor`, `alias`) together in one flat type — there is no reason to split "format-native" and
"SanGen-added" fields across two types when both round-trip into the same JSON object.

## `UnitTransform.type` → `legacyTypeTag` — non-canonical passthrough (ruled, not re-derived)
`type`'s real-world meaning is unresolved, and the evidence that previously existed for it came
from an unconfirmed third-party map-editor tool — now ruled untrustworthy. It is kept as a
**non-canonical passthrough string**:
- Round-tripped faithfully on import — never discarded, in case a real file populates it.
- **Never computed, interpreted, or branched on by any SanGen stage.** `templateIdentifier`
  (`tpId`) remains the sole field SanGen uses to identify a unit.
- Named `legacyTypeTag`, not `type`, specifically so it does **not** read as a normal, meaningful
  field — a future reader who sees `legacyTypeTag` should ask before relying on it, which is the
  point of the name. **Do not "fix" or start deriving behavior from this field without a fresh
  ARCH ruling backed by newly confirmed evidence** — the prior evidence was already retracted
  once.

## Live-engine scope note (out of scope here — apply, don't re-derive)
Whether the live game engine's unit-spawn path actually reads `armies[x].groups`, or instead uses
a separate `_data.lua`-based `groups` registry, is **explicitly out of scope** for this
ratification. This spec's job is that PARAMS/IO round-trip the `.sanmap` format's own
`armies[x].groups` shape exactly as the official format and editor already do, independent of what
the live engine currently consumes.

## The types

```
enum class Faction { Chosen, Guard, EDA };   // 0/1/2 — UNIT_PROP_MARKER_DATA_SPEC "Factions"
```

```
struct UnitTransform {
    std::string name;                  // folded-in dictionary key (UnitGroup.units[key])

    float positionX = 0.0f;            // world/game units, absolute (SANMAP_FORMAT_SPEC)
    float positionY = 0.0f;            // elevation
    float positionZ = 0.0f;
    float rotationX = 0.0f;            // quaternion (x, y, z, w)
    float rotationY = 0.0f;
    float rotationZ = 0.0f;
    float rotationW = 1.0f;
    float scaleX    = 1.0f;
    float scaleY    = 1.0f;
    float scaleZ    = 1.0f;

    char        templateIdentifier[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };  // tpid — the identity SanGen uses
    std::string legacyTypeTag;                                       // `type` — passthrough only, see above
};
```

```
struct UnitGroup {
    std::string name;                          // folded-in dictionary key (parent's `groups[key]`)
    std::vector<UnitTransform> units;           // this group's own units
    std::vector<UnitGroup>     groups;          // nested child groups (recursive)
};
```

```
struct Army {
    std::string name;                  // folded-in dictionary key (armies[key])
    Faction     faction = Faction::Chosen;
    float       alloys  = 500.0f;      // SANMAP_FORMAT_SPEC's confirmed export default
    float       energy  = 500.0f;      // same
    std::vector<UnitGroup> groups;     // recursive pre-placed unit tree

    float       armyColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };  // SanGen-added, Correction 11
    std::string alias;                                       // SanGen-added, Correction 11
};
```

```
struct MapArea {
    std::string name;          // folded-in dictionary key (areas[key]) — a LOAD-BEARING gameplay
                                // identifier: GameUtils.GetArea(name), UNIT_PROP_MARKER_DATA_SPEC
    float originX = 0.0f;      // format's `x`
    float originZ = 0.0f;      // format's `y`
    float width   = 0.0f;      // format's `width`
    float length  = 0.0f;      // format's `height`
};
```

All four types live in `Params::` (new files under `src/params/`, e.g. `Army_PARAMS.h`,
`MapArea_PARAMS.h`) — PARAMS holds no logic beyond what's shown (Constitution §1); no member
function belongs on any of these except perhaps trivial validity checks matching existing
precedent (`MapRecipe::IsValid()`).

## Where these land (for the coder work-order — not built here)
`MapRecipe_PARAMS.h` gains `std::vector<Army> armies;` and `std::vector<MapArea> areas;` alongside
its existing `markerRules`/`propRules`/`decalRules`/`unitRules`. That edit, the matching IO
round-trip (`MapImporter`/`MapExporter`, mirroring the existing `*_Rules_IO` pair), and retiring
the two UI scope notes above are a separate coder work-order — this ratification fixes only the
shape.

One integration question is deliberately left for that work-order, not decided here:
`Params::UnitRule::armyIndex` (the procedural scatter rule's "which army") currently has no
`Params::Army` list to index into (`ArmiesTab_UI.h`'s SCOPE NOTE 1) — whether it becomes an index
into the new `recipe.armies`, and how `ArmiesTab_UI`'s presentation-only `ArmyPresentation` folds
into or is replaced by `Params::Army`, is UI/PIPELINE wiring, not a PARAMS-shape question, and is
not decided by this spec.
