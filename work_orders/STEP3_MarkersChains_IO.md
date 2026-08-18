# Work-Order — Step 3: Markers + Chains PARAMS + IO round-trip

*Constitution §7. Executor: SanGen Coder. Proposed by the orchestrating session against the
already-ratified `ENTITY_AUTHORING_PARAMS_SPEC.md` (second session: `InstancedTransform`,
`MarkerInstanceGroup`, `MarkerTransform`, `MarkerChain`, `ChainMarker`), verified against the
real ground-truth C# source (`SanMap.cs`, `SanMap.Types.cs`, `Types.cs`, `MapUtils.cs`) and the
current `src/io`/`src/ui` code. Second of the §3 "ratified, ready, nothing blocking" items to get
a coder work-order — follows `STEP2_ArmiesAreas_IO.md`'s pattern directly, reusing the lessons
that review pass already surfaced (the wiring-order and file-split rulings below are applied
directly from precedent, not re-derived).*

## Title
Give `InstancedTransform`/`MarkerInstanceGroup`/`MarkerTransform`/`MarkerChain`/`ChainMarker` a
real `Params::` home and wire them into the `.sanmap` `markers`/`chains` round-trip, replacing
the two permanently-empty placeholder objects the exporter writes today.

## Root problem
Same shape as Step 2, one domain family later. `MarkersTab_Placed_UI.h`'s SCOPE NOTE says it
directly: "v1's editable manual markers ... are RECIPE content with no `_PARAMS` home in the
tree: there is no `Params::ManualMarker` and no `MapRecipe` slice to hold one. Adding that type
is a PARAMS work-order this one does not own." `MapExporter_Recipe_IO.cpp:90-91` writes
`document["markers"]`/`document["chains"]` as permanently empty objects; `MapImporter_IO.cpp`
never reads either key.

## Target files
New:
- `src/params/InstancedTransform_PARAMS.h` — the shared `{positionX/Y/Z, rotationX/Y/Z/W,
  scaleX/Y/Z}` base, per the spec's own "File organization" section (ratified, not a fresh
  naming decision).
- `src/params/MarkerInstance_PARAMS.h` — `MarkerTransform`, `MarkerInstanceGroup`.
- `src/params/MarkerChain_PARAMS.h` — `ChainMarker`, `MarkerChain`.
- `src/io/MapExporter_Markers_IO.cpp` — `BuildMarkersJson`.
- `src/io/MapExporter_Chains_IO.cpp` — `BuildChainsJson`.
- `src/io/MapImporter_Markers_IO.cpp` — `ReadMarkersJson`.
- `src/io/MapImporter_Chains_IO.cpp` — `ReadChainsJson`.
  (Four IO files, applying the IO Architecture Expert's Step-2 ruling directly: `markers`/
  `chains` are independent top-level format keys with no shared JSON parent — same reasoning
  that split Armies from Areas. Not re-dispatched for this call since the ruling generalizes
  cleanly; the ARCH Expert conformance pass below still checks the PARAMS shape.)

Modified:
- `src/params/MapRecipe_PARAMS.h` — add `std::vector<MarkerInstanceGroup> markers;
  std::vector<MarkerChain> chains;` and the two new includes.
- `src/io/MapExporter_Recipe_IO.h`/`.cpp` — declare + wire the two new builders.
- `src/io/MapImporter_Recipe_IO.h` — declare the two new readers.
- `src/io/MapImporter_IO.cpp` — call `ReadMarkersJson`/`ReadChainsJson` unconditionally on the
  top-level `document`, in the SAME place and for the SAME reason `ReadAreasJson`/`ReadArmiesJson`
  are called (see "Wiring — apply the Step 2 correction directly" below) — `markers`/`chains` are
  top-level siblings of `mapGeneratorData` too.
- `src/io/MapExporter_IO.h`/`MapImporter_IO.h` — narrow the SCOPE NOTE comments further: after
  this ticket only `props`/`decals` remain empty/unread.
- `src/io/MapExporter_IO_Test.cpp` — narrow the stale "entity domains are written empty" comment
  again (now only `props`/`decals`).
- `src/io/MapFormat_TestSupport_IO.h`/`MapImporter_IO_Test.cpp` — extend the fixture and add
  `CheckMarkersAndChains`, following `CheckArmiesAndAreas`'s exact pattern from Step 2.

## Layer & accuracy class
PARAMS + IO/BRIDGE. Accuracy class: Exact, except rotation (best-effort verbatim pass-through,
same ruling as Step 2 — see below).

## Backend policy
CPU only — JSON text I/O.

## ARCH rules invoked
- `ENTITY_AUTHORING_PARAMS_SPEC.md` (second session) — the ratified `InstancedTransform`/
  `MarkerTransform`/`MarkerInstanceGroup`/`MarkerChain`/`ChainMarker` shapes. Implement verbatim.
- `SANMAP_FORMAT_SPEC.md` "Entity collections" section and Correction 11 (`markers[key].alias`).
- Constitution §6/§8 — same as Step 2: total/degrade-gracefully readers, reuse the existing
  `ReadJson*` typed accessors, no duplicated string literals across write sites.

## Ground-truth findings (verified directly, not assumed from Step 2's precedent alone)

1. **`MarkerTransform` composes `InstancedTransform`, it does NOT flatten it — unlike
   `UnitTransform`.** This is the one structural difference from Step 2 worth calling out
   explicitly so the Coder doesn't copy `UnitTransform`'s flat-scalar shape by habit.
   `ENTITY_AUTHORING_PARAMS_SPEC.md`'s own ruling: `UnitTransform` was "deliberately not
   retrofitted" to compose `InstancedTransform` because it was already shipped; `MarkerTransform`
   has no such history and correctly composes it as a member:
   ```cpp
   struct MarkerTransform { std::string name; InstancedTransform transform; std::string alias; };
   ```
   So field access in the IO code is `markerTransform.transform.positionX`, not
   `markerTransform.positionX`. On the WIRE, though, `position`/`rotation`/`scale` are still
   top-level siblings of `alias` inside one JSON object (confirmed: `MarkerTransform :
   InstancedTransform` in the real C#, `SanMap.Types.cs:168-176` — inheritance flattens on the
   wire even though the C++ `Params::` type composes) — the JSON shape looks identical to
   `UnitTransform`'s, only the C++ member access path differs.

2. **`markers` is a two-level dictionary fold-in** (confirmed `SanMap.cs:151`,
   `SanMap.Types.cs:161-176`): `document["markers"]` is an object keyed by
   `MarkerInstanceGroup::name` (the marker TYPE, e.g. `"Spawn"`/`"Alloys"`), each value
   `{"resource":bool,"transforms":<object>}`, where `transforms` is itself an object keyed by
   `MarkerTransform::name` (the instance name, e.g. `"Mex 0"`), each value
   `{"position":{x,y,z},"rotation":{x,y,z,w},"scale":{x,y,z},"alias":"..."}`. Two levels of the
   same name-keyed-object pattern Step 2 established for `Army.groups`/`UnitGroup.units` — reuse
   that same walking approach (file-local, not shared — same reasoning as Step 2 finding: promote
   only once a fourth domain needs the identical shape).

3. **`chains` is a THIRD, different container shape — an object of ARRAYS, not an object of
   objects.** Confirmed directly: `public Dictionary<string, MarkerChain.Marker[]> chains`
   (`SanMap.cs:150`) — the outer key folds into `MarkerChain::name`, but the VALUE is a bare JSON
   ARRAY of `{"type":..,"name":..}` objects, not a wrapping `{"markers":[...]}` object. (The C++
   `Params::MarkerChain` type has a `.markers` member for convenience, per the ratified spec — but
   that member name does NOT appear as a JSON key; it's implicit in the array being the whole
   value.) This means `chains` needs neither the Rules array-of-objects pattern nor the Step 2
   name-keyed-object-of-objects pattern — it's a name-keyed-object-of-ARRAYS, simple enough to
   write as a plain loop with no shared/generic helper:
   ```cpp
   // export
   nlohmann::ordered_json chains = nlohmann::ordered_json::object();
   for (const Params::MarkerChain& chain : recipe.chains) {
       nlohmann::ordered_json markersArray = nlohmann::ordered_json::array();
       for (const Params::ChainMarker& marker : chain.markers)
           markersArray.push_back({ {"type", marker.type}, {"name", marker.name} });
       chains[chain.name] = markersArray;
   }
   ```
   Import is the direct inverse: `for (auto& [name, arrayJson] : document["chains"].items())`,
   skip if `!arrayJson.is_array()`, else walk it into `ChainMarker{type,name}` entries.

4. **The coordinate flip applies to `MarkerTransform.transform.positionZ`, exactly like
   `UnitTransform` — CONFIRMED, not inferred by analogy this time.** Read directly in
   `MapUtils.cs:117-128,130-149`: both the `ArmySpawnMarker` and `AlloySpotMarker` construction
   sites wrap their position in `TextureToWorldOrigin(...)` before writing. This is the strongest
   possible evidence class (the reference converter actually exercises this path, unlike
   `UnitTransform`, which it never populates) — apply `mapSize - positionZ - 1` on export,
   inverse on import, `positionX`/`positionY`/`rotation`/`scale` untouched. `ChainMarker` has NO
   position field at all (`{type,name}` only — it's a reference to a marker by type+name, not a
   transform), so the flip question does not apply to `chains` at all.

5. **Rotation: same verbatim ruling as Step 2, same evidence.** `MapUtils.cs:121,136` write
   `new Quaternion()` (identity) unconditionally for both marker construction sites — the
   reference converter never attempts real marker rotation either. Apply the identical,
   already-human-ratified Step 2 decision: round-trip `MarkerTransform.transform.rotationX/Y/Z/W`
   verbatim, no transform. Not re-opening this question — same class of decision, same answer.

6. **`resource`/`bResource` casing.** `MarkerType.resource` (verbatim C# field, `SanMap.Types.
   cs:163`) → `MarkerInstanceGroup::bResource` — verbatim word, `b`-prefixed per ARCH §1.1,
   exactly as the spec's naming table already states. JSON key stays `"resource"` (format-native,
   lowerCamelCase, unaffected by the C++ `b`-prefix convention).

## Wiring — apply the Step 2 correction directly, do not repeat the bug
`markers`/`chains` are top-level `.sanmap` keys, siblings of `mapGeneratorData`, confirmed the
same way `armies`/`areas` were (`SANMAP_FORMAT_SPEC.md`'s "Entity collections" section lists all
four together under top-level fields; `MapExporter_Recipe_IO.cpp:90-91` writes them at document
top level, not nested in `mapGeneratorData`). `ReadMarkersJson(document, outRecipe)` and
`ReadChainsJson(document, outRecipe)` MUST be called unconditionally, alongside (immediately
after) `ReadAreasJson`/`ReadArmiesJson` in `MapImporter_IO.cpp`, BEFORE the
`mapGeneratorData`-presence gate — not inside it. This is not a new finding, it's applying Step
2's already-caught-and-fixed bug preemptively so it isn't reintroduced for this domain.

## Solution
1. **`InstancedTransform_PARAMS.h`**:
   ```cpp
   struct InstancedTransform {
       float positionX = 0.0f, positionY = 0.0f, positionZ = 0.0f;
       float rotationX = 0.0f, rotationY = 0.0f, rotationZ = 0.0f, rotationW = 1.0f;
       float scaleX = 1.0f, scaleY = 1.0f, scaleZ = 1.0f;
   };
   ```
2. **`MarkerInstance_PARAMS.h`** (`#include "InstancedTransform_PARAMS.h"`):
   ```cpp
   struct MarkerTransform {
       std::string name;
       InstancedTransform transform;
       std::string alias;
   };
   struct MarkerInstanceGroup {
       std::string name;
       bool bResource = false;
       std::vector<MarkerTransform> transforms;
   };
   ```
3. **`MarkerChain_PARAMS.h`**:
   ```cpp
   struct ChainMarker { std::string type; std::string name; };
   struct MarkerChain {
       std::string name;
       std::vector<ChainMarker> markers;
   };
   ```
4. **`MapRecipe_PARAMS.h`**: add the two includes and `std::vector<MarkerInstanceGroup> markers;
   std::vector<MarkerChain> chains;`. `IsValid()` untouched — empty is legal.
5. **`MapExporter_Markers_IO.cpp`** — `BuildMarkersJson`, per finding 2's shape, flip per finding
   4, rotation verbatim per finding 5, reusing the `tpid`-style bounded-buffer helper pattern is
   NOT needed here (`MarkerTransform` has no `tpid`/`type` fields — those are `UnitTransform`-only).
6. **`MapExporter_Chains_IO.cpp`** — `BuildChainsJson`, per finding 3's array shape. No flip, no
   rotation concern (no position/rotation fields on `ChainMarker` at all).
7. **`MapImporter_Markers_IO.cpp`** — `ReadMarkersJson`, the two-level name-keyed-object inverse
   of step 5, flip-inverse on `positionZ` using `outRecipe.geometry.mapSize` (already populated
   from the top-level `width` key before this runs — same as Step 2).
8. **`MapImporter_Chains_IO.cpp`** — `ReadChainsJson`, the array-walk inverse of step 6.
9. **Wire into `BuildSanmapJsonText`/`ParseSanmapJsonText`** per the "Wiring" section above.
10. **Narrow the SCOPE NOTE comments** in `MapExporter_IO.h`/`MapImporter_IO.h`/
    `MapExporter_IO_Test.cpp` to say only `props`/`decals` remain empty/unread now.
11. **Extend the test fixture**: one `MarkerInstanceGroup` (non-default `bResource`) with one
    `MarkerTransform` (non-zero `positionZ` to exercise the flip, non-identity rotation, non-empty
    `alias`), and one `MarkerChain` with two `ChainMarker` entries. Add `CheckMarkersAndChains`
    called from `RunRoundTripTests`, mirroring `CheckArmiesAndAreas`'s style exactly (assert the
    area/army-equivalent fields survive, and that `positionZ` survives a flip-then-unflip
    identically to the original value without the test needing to know the map-size constant).

## Explicit out-of-scope
- **`props`/`decals`** — deliberately held back from this batch. `ENTITY_AUTHORING_PARAMS_SPEC`'s
  flagged item 1 requires `blueprintPath` validation "mandatory before any export," and the
  handoff (§6 item 3) states the validation mechanism itself "was explicitly left to IO
  Architecture Expert, not yet designed." That's real design work, not wiring — it gets its own
  preliminary design dispatch before a work-order can be drafted for it. Tracked as the next step
  after this ticket.
- **UI wiring** — `MarkersTab_Placed_UI.h`'s SCOPE NOTE and any future `ChainsTab_UI` (none exists
  today) are untouched, same posture as Step 2's UI exclusion.
- **Name-uniqueness / `Spawn`/`Alloys` default-marker synthesis** — this ticket round-trips
  whatever is in `recipe.markers`/`recipe.chains`; it does not synthesize default spawn/mex
  markers or enforce the `MarkerCategory` cardinality ruling (already settled by the spec as
  free-form `std::string`, not re-litigated here).

## Acceptance test
Same shape as Step 2's: the existing round-trip binary (`MapImporter_IO_Test.exe`) passes with
the extended fixture, `CheckMarkersAndChains` assertions included, `result.warningCount == 0`
still holds, and all pre-existing assertions (including Step 2's `CheckArmiesAndAreas`) are
unaffected. Full `SanGenV2` build stays clean.
