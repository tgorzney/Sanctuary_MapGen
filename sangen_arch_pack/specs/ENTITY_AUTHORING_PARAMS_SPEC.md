# ENTITY_AUTHORING_PARAMS_SPEC — pass-through entity PARAMS: armies, unit groups/transforms,
# map areas, markers, props, decals, marker chains

Source of truth: `SanMap.Types.cs` (`Sanmap File Format\SanMap.Types.cs`, the `EM.Map` namespace —
confirmed byte-identical to `Sanctuary-Map-Generation-develop\Sanctuary\*.cs`; **NOT**
`Sanctuary-Map-Generation-develop\src\*` / the `ExtraneousMapGen` namespace, an unrelated
third-party tool — never read or cite that tree for this domain). `SANMAP_FORMAT_SPEC`'s "Entity
collections" section documents the wire shape; this spec documents the concrete `Params::` C++
shape and the naming derivation (ARCH §1.8) that produced it.

This spec was ratified in four sessions. The first covered `Army`/`UnitGroup`/`UnitTransform`/
`MapArea` — the hand-placed *army/unit* and *area-rectangle* domains. The second extended the exact
same pass-through bucket to the four remaining resolved/baked entity domains: `markers`, `props`,
`decals`, `chains`. The third (ARCH §12) adds **manual-layer authoring** for
hand-placed props/decals — `PropTransform`/`DecalTransform::layerIndex` plus the separate
`PropInstanceLayer`/`DecalInstanceLayer` metadata arrays — and, in doing so, **supersedes** the
second session's "props/decals need no wrapper transform type" ruling (see that section below for
the correction). The fourth (ARCH §16, `SANMAP_FORMAT_SPEC` Correction 15/16) extends the same
manual-layer pattern to markers — `MarkerTransform::layerIndex`/`symmetryGroupIdentifier` plus the
new `MarkerInstanceLayer` metadata array — and adds the new per-layer `SymmetrySetting` field family
to that array. All four sessions share one framing, one naming rule (ARCH §1.8), and one home file
family — see the Scope section below.

## Scope — why this is a separate type family from `PLACEMENT_SCATTER_SPEC`
`Params::MarkerRule`/`PropRule`/`DecalRule`/`UnitRule` (`PLACEMENT_SCATTER_SPEC`) are **procedural
placement RULES** — a PROC stage (`Placement_PROC`) reads them and computes where entities land.
`Params::Army`/`UnitGroup`/`UnitTransform`/`MapArea`/`MarkerInstanceGroup`/`PropInstanceGroup`/
`DecalInstanceGroup`/`MarkerChain` (this spec) are the opposite kind of data: **manually-placed,
human-authored entities** a designer positions directly (mouse clicks on the canvas, or values
imported unchanged from an existing `.sanmap`). No PROC stage computes or reinterprets them —
round-trip fidelity is their entire purpose (ARCH §1.8, "pass-through" bucket). The two mechanisms
coexist per domain: an `ArmiesTab_UI` row can show both hand-placed units (`Army.groups`, this
spec) and the procedural `UnitRule`s that spawn more units for the same army
(`ScatterRule_PARAMS.h`) — two independent producers of the same `armies[]` roster, exactly as v1
had. The same pattern repeats for markers: `MarkersTab_UI` shows both hand-placed marker instances
(`MarkerInstanceGroup`, this spec) and the procedural `MarkerRule`s that generate more
(`MarkerRule_PARAMS.h`) — two independent producers of the same `markers[]` collection. The manual
prop/decal *layer* concept the third session adds (`PropInstanceLayer`/`DecalInstanceLayer`), and
the manual marker-layer concept the fourth session adds (`MarkerInstanceLayer`), are the same kind
of authoring-convenience metadata as `ManualPropGroup`
(`PropsTab_Manual_UI.h`) — organizational grouping for hand-placed instances, not a procedural rule.

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

**`markers` applies the identical rule one level deeper still — the same recursive-tree posture as
`Army`, not a new mechanism.** `markers: Dictionary<string, MarkerType{ resource, transforms:
Dictionary<string, MarkerTransform> }>` is a *two-level* dictionary, confirmed field-for-field
against `SanMap.Types.cs` (`SanMap.cs:151`, `MarkerType`/`MarkerTransform` at
`SanMap.Types.cs:161-176`). Both levels fold in exactly as `Army`/`UnitGroup` already do: the outer
key (the marker *type* name, e.g. `Spawn`/`Alloys`) becomes `MarkerInstanceGroup::name`, the inner
key (the *instance* name, e.g. `Mex 0`) becomes `MarkerTransform::name`.

**`props`/`decals` need no dict→vector conversion at all.** `PropType[]`/`DecalType[]` are already
bare C# arrays at the top level (`SanMap.cs:153,157`), and each entry's own `transforms` is a
`List<PropTransform>`/`List<DecalTransform>` (`SanMap.Types.cs:112,130`) — an *ordered array*, not
a dictionary, with no per-instance key to fold in. `std::vector` is the direct, verbatim
translation; no `name` field is invented for something the format never keyed.

**`chains` folds in exactly like `markers`' outer level, but its value is a bare array, not another
dictionary.** `chains: Dictionary<string, MarkerChain.Marker[]>` (`SanMap.cs:150`) — the outer key
(the chain name, e.g. `FirstChain`) folds in as `MarkerChain::name`; the value is directly an
ordered `Marker[]`, which becomes `MarkerChain::markers` (a `std::vector`, verbatim field name from
the format's own `MarkerChain.markers` C# field at `SanMap.Types.cs:147` — see "`Marker` →
`ChainMarker`" below for why the *element* type is renamed while the *field* name is not).

## `InstancedTransform` promoted to a real shared PARAMS type — `UnitTransform` is NOT retrofitted
The format's `InstancedTransform` (`SanMap.Types.cs:179`, `{ position, rotation, scale }`) is the
base every entity transform type inherits (`UnitTransform`, `PropTransform`, `DecalTransform`,
`MarkerTransform` all `: InstancedTransform` — confirmed by reading each constructor). The first
ratification session already spelled `UnitTransform`'s ten scalar fields out flat
(`positionX/Y/Z`, `rotationX/Y/Z/W`, `scaleX/Y/Z`) rather than composing a shared base, because no
such base existed yet in `Params::`. This session promotes those same ten fields into a real,
reusable `Params::InstancedTransform` (matching the composition precedent already live in
`PropRule`/`DecalRule`/`UnitRule`/`MarkerRule`, which all hold a `ScatterTransform transform`
member rather than duplicating its fields) — `PropTransform`/`DecalTransform` compose it as their
one geometric member (see the third-session revision below), and `MarkerTransform` adds
`name`/`alias` alongside it.

`UnitTransform` itself is **deliberately not retrofitted** to compose `InstancedTransform` in this
ratification. It is an already-shipped, already-spec'd type (previous session); rewriting its ten
inline scalar fields into `InstancedTransform transform;` is pure churn against working law for no
functional gain — nothing reads `UnitTransform` differently either way, and doing it now would edit
a type this same spec already finalized without new evidence driving the change (mirrors the ARCH
§1.8 "never retype without a fresh ruling backed by newly confirmed evidence" posture applied to
shape, not naming). **Noted as an optional future consistency pass — not part of this
ratification, not to be inferred as "already decided" by a coder.**

## The naming derivation (ARCH §1.8 applied)
All types across the four sessions sit in the same "pass-through" bucket: format spelling by
default, case-converted, with named exceptions. See ARCH §1.8 for the general rule; this table is
its application here. Rows above the first divider are the first session (`Army`/`UnitGroup`/
`UnitTransform`/`MapArea`); rows between the dividers are the second session (`markers`/`props`/
`decals`/`chains`); the next row is the third session (`layerIndex`, ARCH §12 — see the dedicated
section below for the full reasoning, not re-derived in the table alone); the final two rows are the
fourth session (`MarkerTransform::layerIndex`/`symmetryGroupIdentifier`, ARCH §16 — see the "ARCH
§16" section below).

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
| `markers` outer dict key (marker TYPE name) | `MarkerInstanceGroup::name` | dict→vector+name, one level of the two-level fold-in |
| `MarkerType.resource` | `MarkerInstanceGroup::bResource` | verbatim word, §1.1 `b`-boolean prefix |
| `MarkerType.transforms` (dict) | `MarkerInstanceGroup::transforms` (vector) | verbatim name; dict→vector, other level of the fold-in |
| `markers[type].transforms` inner dict key (instance name) | `MarkerTransform::name` | dict→vector+name, deepest level |
| `markers[type][name].alias` | `MarkerTransform::alias` | already-ratified SanGen-added sibling field (`SANMAP_FORMAT_SPEC` Correction 11) — same pattern as `Army`'s `armyColor`/`alias` |
| `PropType.blueprintPath` | `PropInstanceGroup::blueprintPath` | verbatim |
| `PropType.transforms` (`List`) | `PropInstanceGroup::transforms` (vector) | verbatim name; ordered array, not a dict — no `name` fold-in (nothing to fold) |
| `DecalType.blueprintPath` | `DecalInstanceGroup::blueprintPath` | verbatim |
| `DecalType.transforms` (`List`) | `DecalInstanceGroup::transforms` (vector) | same as `PropType` |
| `chains` dict key (chain name) | `MarkerChain::name` | dict→vector+name |
| `MarkerChain.markers` (`Marker[]`) | `MarkerChain::markers` | verbatim field name, borrowed directly from the format's own C# field |
| `MarkerChain.Marker` (nested struct) | `ChainMarker` (top-level struct) | renamed — see "`Marker` → `ChainMarker`" below |
| `Marker.type` | `ChainMarker::type` | verbatim |
| `Marker.name` | `ChainMarker::name` | verbatim |
| *(no format field — SanGen-added)* | `PropTransform::layerIndex` / `DecalTransform::layerIndex` | ARCH §12, direct field injection — see below |
| *(no format field — SanGen-added)* | `MarkerTransform::layerIndex` | ARCH §16, direct field injection, spelled identically to the ARCH §12 `layerIndex` above — see "ARCH §16" section below |
| *(no format field — SanGen-added)* | `MarkerTransform::symmetryGroupIdentifier` | ARCH §16.5, direct field injection, spelled in full — NOT `symmetryGroupId`, an abbreviation this ratification rejected (§1.1/§1.8) |

## `armyColor` / `alias` — the two already-ratified `armies[key]` additions
`SANMAP_FORMAT_SPEC` Correction 11 already ratified `armyColor` and `alias` as SanGen-added,
lowerCamelCase siblings merged into the format's own `armies[key]` dictionary entry — they exist in
the schema today, just with no `Params::Army` to live on. This ratification gives them that home;
it does not invent them. `Params::Army` therefore carries the format-native fields (`faction`,
`alloys`, `energy`, `groups`, plus the folded-in `name`) and the two SanGen-added siblings
(`armyColor`, `alias`) together in one flat type — there is no reason to split "format-native" and
"SanGen-added" fields across two types when both round-trip into the same JSON object.

The same Correction 11 also ratifies `markers[key]` gaining **`alias`** (see
`SANMAP_FORMAT_SPEC`'s "Merges into existing format-native collections" note) — this session's
`MarkerTransform::alias` field gives *that* addition its home, exactly the same move one level
deeper in the marker tree.

## `layerIndex` / `PropGroups` / `DecalGroups` — the manual-layer authoring addition (ARCH §12)
Third session. Adds manual-layer membership for hand-placed props/decals — the `PropsTab_Manual_UI`/
future `DecalsTab_Manual_UI` grouping concept, which previously had no serialized `Params::` home
(`PropsTab_Manual_UI.h`'s SCOPE NOTE 1: "v1's manual prop GROUPS ... have no `_PARAMS` home in the
tree"). Full reasoning and the format-native-injection-vs-separate-array general principle: ARCH
§12. Summary:

- **`PropTransform`/`DecalTransform::layerIndex`** (`int`, default `0`) is a **direct field
  injection** into the per-instance transform, not a contiguous index-range in a side table. A
  range-based alternative was considered and rejected: it has a real silent-corruption failure
  mode — if any external tool (hand-editing, or the real Unity map editor, both supported workflows
  for this format) reorders an entry in `transforms[]` without changing the count, the ranges stay
  internally self-consistent while silently misattributing instances to the wrong layer,
  undetectable by any validation. `layerIndex` travels with the instance, so external reordering
  cannot desync it. Cost is trivial (~1 MB even at Forge's 63.5k prop instances,
  `SANMAP_FORMAT_SPEC`'s 23-map survey) against the file's actual bulk, which is textures, not JSON.
- **Not on shared `InstancedTransform`.** `layerIndex` is Prop/Decal-specific authoring metadata; if
  it lived on the shared base it would leak onto `MarkerTransform`'s composed member and every
  future consumer of the base that has no concept of a manual layer.
- **JSON key `layerIndex`, lowerCamelCase**, merged directly into the existing transform object —
  the same rule already governing `armyColor`/`alias` (ARCH §1.6 Correction 0), applied one level
  deeper: to an *array-element* object (`props[].transforms[]`) rather than a *dictionary-value*
  object, because `props`/`decals` are arrays, not dictionaries (see the structural ruling above).
- **Separate layer-metadata arrays**, one per domain, top-level PascalCase schema-v3 keys
  `PropGroups`/`DecalGroups` (`SANMAP_FORMAT_SPEC` Correction 14) — not `PropLayers`/`DecalLayers`:
  `PropsStack`/`DecalsStack`'s Group→Layer(rule) procedural hierarchy (`SANMAP_FORMAT_SPEC`
  Correction 7) already uses "Layer" for something unrelated (a rule inside a Stack), and reusing it
  here would collide two different concepts under the same word. `ManualPropGroup` is also the
  already-live identifier in `src/ui/PropsTab_Manual_UI.h`, so `PropGroups`/`DecalGroups` picks up
  an existing name rather than inventing a new one.
- **Import validation:** `layerIndex` out of range against the corresponding `PropGroups`/
  `DecalGroups` array size is a loud logged clamp to `0` (Constitution §6) — never a hard refusal,
  because this is authoring-convenience metadata, not gameplay-authoritative data. A missing
  `layerIndex` key on an older/foreign file degrades for free to `0` (the field's own default).

## `MarkerTransform::layerIndex` / `symmetryGroupIdentifier` / `MarkerGroups` — layer-scoped marker symmetry (ARCH §16)
Fourth session. Extends the exact ARCH §12 pattern above to markers, plus one genuinely new field
`layerIndex` has no analog for: `symmetryGroupIdentifier`. Full reasoning: ARCH §16.5;
wire-format placement and casing: `SANMAP_FORMAT_SPEC` Correction 15/16. Summary:

- **`MarkerTransform::layerIndex`** (`int`, default `0`) — same direct-field-injection shape as
  `PropTransform`/`DecalTransform::layerIndex` above, same reasoning (external-reorder desync
  avoidance), indexes the new `MapRecipe::markerLayers` (`MarkerGroups` on disk). Not on shared
  `InstancedTransform`, for the same reason `PropTransform`/`DecalTransform::layerIndex` isn't.
- **`MarkerTransform::symmetryGroupIdentifier`** (`int`, default `0`, `0` = ungrouped) — NEW, no
  Prop/Decal analog. Names which symmetry-linked clone set a hand-placed marker belongs to. Spelled
  in full — **not** `symmetryGroupId`, an abbreviation the underlying design proposed and ARCH §16.5
  corrected, since "Id" is not on ARCH §1.1's permitted abbreviation list and `src/params/` carries
  zero existing precedent for it.
- **`MarkerInstanceLayer`** (below) is the `MarkerGroups`-backing metadata array, mirroring
  `PropInstanceLayer`/`DecalInstanceLayer` but carrying two things those don't: a stable `layerId`
  and a per-layer `SymmetrySetting` (ARCH §16.1) — see the type definition below.
- **Import validation:** `layerIndex` follows the identical loud-clamp-to-`0` rule as
  `PropTransform`/`DecalTransform::layerIndex`. `symmetryGroupIdentifier` has no range to validate —
  `0` is always legal (ungrouped), any positive value is accepted as-is.

## Why props/decals now need a wrapper transform type — SUPERSEDES the second session's ruling
The second session ruled `PropTransform`/`DecalTransform` need no wrapper type at all, because
`PropTransform`/`DecalTransform` in the C# format add **zero fields** beyond `InstancedTransform`
(confirmed — their only C# content is a constructor that assigns `position`/`rotation`/`scale`,
nothing else), so `PropInstanceGroup`/`DecalInstanceGroup` held `std::vector<InstancedTransform>`
directly. **That premise no longer holds.** `layerIndex` (above) is a genuinely new per-instance
field that is not part of `InstancedTransform` and must not leak onto it, so `PropTransform`/
`DecalTransform` are now real, named `Params::` wrapper types — thin, but no longer empty:

```cpp
struct PropTransform  { InstancedTransform transform; int layerIndex = 0; };
struct DecalTransform { InstancedTransform transform; int layerIndex = 0; };
```

`MarkerTransform` was always the odd one out for exactly this reason (it already carried `name`/
`alias` alongside its `InstancedTransform`) — `PropTransform`/`DecalTransform` now follow the same
shape, composing an `InstancedTransform transform;` member rather than inheriting or flattening it,
matching the composition precedent `MarkerRule`/`PropRule`/`DecalRule`/`UnitRule` already use for
`ScatterTransform`.

## Cardinality ruling — the marker-type key stays `std::string`, NOT retyped to `MarkerCategory`
`MarkerInstanceGroup::name` (the folded-in outer `markers` dict key — the marker *type* name, e.g.
`Spawn`/`Alloys`) is ruled a free-form `std::string`, **not** retyped to the existing
`enum class MarkerCategory { Generic, Spawn, Alloys, Expansion }` (`MarkerRule_PARAMS.h`).

This is confirmed correct, and on stronger grounds than a closed-set-vs-open-set intuition alone:

1. **The format itself is structurally open.** `markers: Dictionary<string, MarkerType>`
   (`SanMap.cs:151`) — a C# string-keyed dictionary, not an enum-keyed one. There is no format-side
   constraint of any kind on what a key may be; retyping to a closed C++ enum would silently
   drop or corrupt any marker type outside the known set on import. This is exactly the failure
   mode ARCH §1.8's pass-through-verbatim-by-default rule exists to prevent (compare `Army.faction`,
   which *is* retyped — but only because the format itself types that field as a closed 0/1/2 int,
   not a string).
2. **`MarkerCategory` is demonstrably not the same set, in either direction.** It is a SanGen-owned
   procedural-*rule* categorization (`MarkerRule_PARAMS.h`, used by `Placement_PROC` to decide what
   the AI-analysis pass should expect at a generated marker) — not a wire-format enum at all. It
   already contains `Generic` and `Expansion`, neither of which has ever been observed in any real
   `.sanmap` (`SANMAP_FORMAT_SPEC`'s 23-map survey found only `Spawn`/`Alloys` in the wild), and it
   omits whatever a human/tool might legally type in as a hand-authored marker-type key. Equating
   "the pass-through wire-level type-name string" with "SanGen's own placement-intent enum" would
   be a category error even setting the closed/open question aside — they classify different
   things (one is an identity the format assigns, the other is a purpose SanGen infers for its own
   procedural rules).
3. The 23-map survey's "only `Spawn`/`Alloys` observed" is corroborating, not load-bearing — a
   small, non-exhaustive sample cannot license retyping a structurally open format field to a
   closed set; that would need an explicit, engine-confirmed enumeration of every legal marker type
   name, which does not exist.

**Ruling confirmed as proposed:** `MarkerInstanceGroup::name` is `std::string`.

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
*(Kept as its already-ratified flat-scalar shape — NOT retrofitted onto `InstancedTransform` below;
see "`InstancedTransform` promoted... `UnitTransform` is NOT retrofitted" above.)*

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

The shared base promoted in the second session, and the resolved/baked-instance types built on it:

```
struct InstancedTransform {
    float positionX = 0.0f; float positionY = 0.0f; float positionZ = 0.0f;
    float rotationX = 0.0f; float rotationY = 0.0f; float rotationZ = 0.0f; float rotationW = 1.0f;
    float scaleX = 1.0f; float scaleY = 1.0f; float scaleZ = 1.0f;
};
```

```
// Third session (ARCH §12): PropTransform/DecalTransform are now thin named wrapper types, not a
// bare InstancedTransform — see "Why props/decals now need a wrapper transform type" above.
struct PropTransform  { InstancedTransform transform; int layerIndex = 0; };
struct DecalTransform { InstancedTransform transform; int layerIndex = 0; };

struct PropInstanceGroup  { std::string blueprintPath; std::vector<PropTransform>  transforms; };
struct DecalInstanceGroup { std::string blueprintPath; std::vector<DecalTransform> transforms; };
```

```
// Third session (ARCH §12): the separate manual-layer metadata array, one entry per authored
// layer, indexed by PropTransform/DecalTransform::layerIndex. Same shape for both domains.
struct PropInstanceLayer  { std::string name; float color[4] = {1.0f,1.0f,1.0f,1.0f}; float iconScale = 1.0f; };
struct DecalInstanceLayer { std::string name; float color[4] = {1.0f,1.0f,1.0f,1.0f}; float iconScale = 1.0f; };
```

```
// Fourth session (ARCH §16): MarkerInstanceLayer — the marker-side manual-layer metadata array,
// mirroring PropInstanceLayer/DecalInstanceLayer above but with two additions markers carry from
// day one rather than retrofitting later (as Props/Decals did via the earlier STEP56 work-order):
// a stable layerId, and a per-layer SymmetrySetting (ARCH §16.1 — the same shared struct also
// used by the procedural MarkerRuleLayer wrapper, PLACEMENT_SCATTER_SPEC). `layerId` is flagged,
// not re-ruled, as a probable follow-up naming correction (SANMAP_FORMAT_SPEC Correction 16) —
// kept as proposed here, not renamed by this ratification.
struct MarkerInstanceLayer {
    std::string name;
    float color[4] = {1.0f,1.0f,1.0f,1.0f};
    float iconScale = 1.0f;
    int layerId = 0;              // stable id — legacy-backfill by array index on import when absent
    SymmetrySetting symmetry;     // ARCH §16.1 — bSymmetryUseGlobal / symmetryMask / radialSymmetryRepeatCount
};
```

```
struct MarkerTransform {
    std::string name;             // folded-in inner dict key — instance name (e.g. "Mex 0")
    InstancedTransform transform;
    std::string alias;            // SanGen-added, already-ratified SANMAP_FORMAT_SPEC Correction 11
    int layerIndex = 0;                // SanGen-added, ARCH §16 — indexes MapRecipe::markerLayers
                                        // (MarkerGroups); same shape/reasoning as PropTransform/
                                        // DecalTransform::layerIndex above
    int symmetryGroupIdentifier = 0;   // SanGen-added, ARCH §16.5 — spelled in full (NOT
                                        // symmetryGroupId); 0 = ungrouped
};

struct MarkerInstanceGroup {
    std::string name;                        // folded-in outer dict key — marker TYPE name
                                              // (e.g. "Spawn"/"Alloys") — free-form std::string,
                                              // NOT MarkerCategory; see cardinality ruling above
    bool bResource = false;                  // format's `resource`, b-prefixed per §1.1
    std::vector<MarkerTransform> transforms;
};
```

```
struct ChainMarker { std::string type; std::string name; };  // deliberately renamed from format's
                                                                // bare "Marker" — see naming
                                                                // exception above

struct MarkerChain {
    std::string name;                   // folded-in outer dict key — chain name
    std::vector<ChainMarker> markers;   // ORDERED — semantically meaningful sequence, never
                                         // resorted. Field name borrowed directly from the
                                         // format's own MarkerChain.markers C# field.
};
```

All types live in `Params::` — PARAMS holds no logic beyond what's shown (Constitution §1); no
member function belongs on any of these except perhaps trivial validity checks matching existing
precedent (`MapRecipe::IsValid()`).

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

## `Marker` → `ChainMarker` — a naming exception of the same class as `Area.height` → `length`
The format's C# nested type is literally named `Marker` (`MarkerChain.Marker`, `SanMap.Types.cs:
150`). Verbatim would collide inside a codebase that already has `MarkerTransform`,
`MarkerInstanceGroup`, `MarkerRule`, and `MarkerCategory` all live in the same `Params::` namespace
— a bare `Marker` sitting next to those is genuinely ambiguous about which "marker" concept it
names (a placement-rule marker? a resolved instance? a chain reference?). This is the same
"verbatim would collide with an established SanGen quantity" exception class ARCH §1.8 already
carves out for `Area.height` → `length`, applied to a type name instead of a field name. The
`markers`/`name` field *names* on `ChainMarker` stay verbatim — only the *type* name changes.

## File organization
- `Army_PARAMS.h`, `MapArea_PARAMS.h` — first session, `Army`/`UnitGroup`/`UnitTransform`/`Faction`
  and `MapArea` respectively (unchanged since).
- `InstancedTransform_PARAMS.h` — the shared base, added in the second session. Depended on by
  `MarkerInstance_PARAMS.h` and `PropInstance_PARAMS.h` below.
- `MarkerInstance_PARAMS.h` — `MarkerTransform`, `MarkerInstanceGroup`, and, as of the fourth
  session (ARCH §16), `MarkerInstanceLayer` (depends on `Params::SymmetrySetting`, ARCH §16.1).
- `PropInstance_PARAMS.h` — `PropTransform`, `DecalTransform`, `PropInstanceGroup`,
  `DecalInstanceGroup`, `PropInstanceLayer`, `DecalInstanceLayer` together, mirroring the existing
  `ScatterRule_PARAMS.h` multi-type-per-file precedent (`PropRule`/`DecalRule`/`UnitRule` already
  share one file for the same reason: near-identical shapes, always touched together). The third
  session's four new types join this file rather than starting a new one — same reasoning.
- `MarkerChain_PARAMS.h` — `ChainMarker`, `MarkerChain`.

## Where these land (for the coder work-order — not built here)
`MapRecipe_PARAMS.h` gains, alongside its existing `markerRules`/`propRules`/`decalRules`/
`unitRules` (procedural) and `armies`/`areas` (first-session pass-through):

```cpp
std::vector<Army>                armies;
std::vector<MapArea>             areas;
std::vector<MarkerInstanceGroup> markers;
std::vector<PropInstanceGroup>   props;
std::vector<DecalInstanceGroup>  decals;
std::vector<MarkerChain>         chains;
std::vector<PropInstanceLayer>   propLayers;    // ARCH §12 — metadata for schema v3 `PropGroups`
std::vector<DecalInstanceLayer>  decalLayers;   // ARCH §12 — metadata for schema v3 `DecalGroups`
std::vector<MarkerInstanceLayer> markerLayers;  // ARCH §16 — metadata for schema v3 `MarkerGroups`
```

That edit, the matching IO round-trip (`MapImporter`/`MapExporter`, mirroring the existing
`*_Rules_IO` pair), and retiring the corresponding UI scope notes are a separate coder
work-order — this ratification fixes only the shape.

One integration question is deliberately left for that work-order, not decided here:
`Params::UnitRule::armyIndex` (the procedural scatter rule's "which army") currently has no
`Params::Army` list to index into (`ArmiesTab_UI.h`'s SCOPE NOTE 1) — whether it becomes an index
into the new `recipe.armies`, and how `ArmiesTab_UI`'s presentation-only `ArmyPresentation` folds
into or is replaced by `Params::Army`, is UI/PIPELINE wiring, not a PARAMS-shape question, and is
not decided by this spec.

**Second integration question, added by the third session, also left open:** how `Ui::ManualPropGroup`
(`PropsTab_Manual_UI.h`) folds into or is replaced by `Params::PropInstanceLayer` is UI wiring, not
decided here. This ratification only gives the concept a durable, serializable `Params::` home
(`PropInstanceLayer`/`PropGroups`) where none existed before — closing the "no `_PARAMS` home"
half of that file's SCOPE NOTE 1. The UI presentation type, its `previewColorToggle`/
`iconScaleToggle` realtime-preview machinery, and its dirty-flag notification are unaffected and
remain a separate UI work-order.

## Flagged for the future entity-export IO coder work-order — items 2–4 NOT resolved by this
## ratification; item 1 RATIFIED in a later session (see below)
This ratification fixes shape only. Four items were originally flagged, not resolved, for
whichever work-order wired `PropInstanceGroup`/`DecalInstanceGroup`/`MarkerInstanceGroup`/
`MarkerChain` into IO. Item 1 has since been closed by an explicit human ruling, recorded in full
below; items 2–4 remain open exactly as originally flagged:
1. **RATIFIED — warn, never block.** `blueprintPath` validation is mandatory before any export of
   `PropInstanceGroup`/`DecalInstanceGroup`: a single unresolvable path aborts the rest of the live
   game's map load (`SPEC-1_PropFormatCorrections_DOCS.md` Correction 3, already applied to
   `SANMAP_FORMAT_SPEC`). Never synthesize a fallback path (e.g. `<code>/<code>.santp`) — resolve
   literally against the real pack. **This closes the item's original "...resolve literally against
   the real pack or fail loudly (Constitution §6)" ambiguity in favor of fail-loudly meaning
   surfaced loudly to the human, not hard-refused by the tool:** an unresolvable path is *reported*
   — via the IO-layer `ValidatePropAndDecalBlueprintPaths` check (a pure, read-only function against
   an already-opened `SanpackReader`, declared in the public `MapExporter_IO.h`, returning a
   `BlueprintValidationReport`) and a `ConfirmDialog_UI` warning that names the runtime risk
   (`UI_FRAMEWORK_SPEC.md`'s "Universal widget library") — never silently dropped, and never
   silently used to refuse the export outright. The designer sees the warning and chooses: OK
   exports anyway, Cancel aborts with nothing written. Any non-UI caller that supplies a
   `SanpackReader` (tests, a future non-UI entry point) gets the identical check run as a **logged,
   non-gating** safety net — `result.bSucceeded` is never gated by an unresolved-path finding. A
   caller with no `SanpackReader` (`assetPack == nullptr` — every call site that predates this
   ruling) skips validation entirely: zero behavior change. Human-ratified this session; implemented
   by `work_orders/STEP4_PropsDecals_IO.md` (PARAMS/pure-builder groundwork, unwired) and
   `work_orders/STEP5_PropsDecalsValidation_UI.md` (the validation function, the live wiring, and
   `ConfirmDialog_UI`).
2. **Rotation is currently unimplemented in the existing exporter, and props export is currently
   disabled entirely** (`SANMAP_FORMAT_SPEC`'s "Known gaps in the current exporter" note — identity
   quaternions written for every entity type, props commented out). These are pre-existing
   fix-targets this PARAMS ratification does not itself resolve; the entity-export work-order that
   wires these new types into IO must close both.
3. **The confirmed coordinate flip (`world.z = length - z - 1`, `SANMAP_FORMAT_SPEC`) applies to
   every `InstancedTransform.positionZ` on `PropInstanceGroup`/`DecalInstanceGroup`/
   `MarkerTransform` on both import and export** — the same flip that will apply to
   `UnitTransform.positionZ`, extended to the three new position-carrying types this session adds.
4. **`layerIndex` range validation (ARCH §12) is an import-time concern, not an export-time one.**
   On export, `layerIndex` is written as-is (it is always in range by construction, since only the
   UI can produce it and only against the live `propLayers`/`decalLayers` array). On import, a
   `layerIndex` at or beyond `propLayers.size()`/`decalLayers.size()` is a loud, logged clamp to
   `0` (Constitution §6) — the work-order implementing the importer must apply this per-instance,
   not just once at file scope, since different instances can carry different out-of-range values.
   **The same rule applies to `MarkerTransform::layerIndex` against `markerLayers.size()` (ARCH
   §16), added by the fourth session.**
</content>
