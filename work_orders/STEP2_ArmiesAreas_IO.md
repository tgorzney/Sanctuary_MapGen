# Work-Order (RATIFIED — cleared for the Coder) — Step 2: Armies + Areas PARAMS + IO round-trip

*Constitution §7. Executor: SanGen Coder. Proposed by the orchestrating session against the
already-ratified `ENTITY_AUTHORING_PARAMS_SPEC.md` (first session: `Army`/`UnitGroup`/
`UnitTransform`/`MapArea`) and verified against the real ground-truth C# source
(`D:\Projects\Sanctuary\Sanmap File Format\SanMap.cs`, `SanMap.Types.cs`, `Types.cs`,
`MapUtils.cs`) and the current `src/io` code. This is the first of the §3 "ratified, ready,
nothing blocking" items from `work_orders/SESSION_HANDOFF_ImportExport.md` to get a coder
work-order. Reviewed by the ARCH Expert (conformant, no blocking changes) and the IO
Architecture Expert (file-split ruling applied; caught and this document now fixes a real
wiring-order bug in the original draft — see "Critical wiring correction"). Human sign-off
given, including on the one open interpretive call (rotation handling, item 2 below). Cleared
for the Coder.*

## Title
Give `Army`/`UnitGroup`/`UnitTransform`/`MapArea` a real `Params::` home and wire them into
the `.sanmap` `armies`/`areas` round-trip, replacing the two permanently-empty placeholder
objects the exporter writes today.

## Root problem
`ENTITY_AUTHORING_PARAMS_SPEC.md` ratified the shape of `Params::Army`/`UnitGroup`/
`UnitTransform`/`MapArea` in its first session. Nothing implements it yet:
- `src/params/MapRecipe_PARAMS.h` has no `armies`/`areas` member.
- `src/io/MapExporter_Recipe_IO.cpp:88-89` writes `document["areas"]` and
  `document["armies"]` as permanently empty JSON objects (`MapExporter_IO.h` SCOPE NOTE 1:
  "entity export is its own work-order" — this is that work-order for two of the six
  domains).
- `src/io/MapImporter_IO.cpp`'s `ParseSanmapJsonText` never reads `areas`/`armies` at all
  (`MapImporter_IO.h` SCOPE NOTE 2, same reasoning).
- `src/ui/ArmiesTab_UI.h` SCOPE NOTE 1 explicitly says "AN ARMY HAS NO `_PARAMS` HOME
  ... A durable `Army_PARAMS` is its own work-order" — this work-order gives it that home
  (but does NOT retire the UI scope note itself; see Explicit out-of-scope).

## Target files
New:
- `src/params/Army_PARAMS.h` — `Faction`, `UnitTransform`, `UnitGroup`, `Army`.
- `src/params/MapArea_PARAMS.h` — `MapArea`.
- `src/io/MapExporter_Armies_IO.cpp` — `BuildArmiesJson`.
- `src/io/MapExporter_Areas_IO.cpp` — `BuildAreasJson`.
- `src/io/MapImporter_Armies_IO.cpp` — `ReadArmiesJson`.
- `src/io/MapImporter_Areas_IO.cpp` — `ReadAreasJson`.
  (Four files, not two — see "IO Architecture Expert ruling" below. `armies`/`areas` are
  independent top-level format keys with no shared parent, unlike the four `PlacementRules`
  arrays, which genuinely share one JSON parent object and so legitimately share one file pair.)

Modified:
- `src/params/MapRecipe_PARAMS.h` — add `std::vector<Army> armies; std::vector<MapArea> areas;`
  and the two new includes.
- `src/io/MapExporter_Recipe_IO.h` — declare the two new builders (unchanged ruling: they join
  the existing shared header, same as `Layers`/`Rules` do today, each from its own `.cpp`).
- `src/io/MapExporter_Recipe_IO.cpp` — `BuildSanmapJsonText`: replace the two empty-object
  literals with real calls.
- `src/io/MapImporter_Recipe_IO.h` — declare the two new readers (same shared-header ruling).
- `src/io/MapImporter_IO.cpp` — `ParseSanmapJsonText`: call the two new readers **unconditionally,
  operating on the top-level `document`, immediately after the existing top-level `width`/`height`
  reads and BEFORE the `mapGeneratorData`-presence gate/early-return** — see "Critical wiring
  correction" below. This is NOT "alongside the existing block readers" (`ReadGeometryJson` etc.),
  which all correctly operate inside that gate on `generatorData` — armies/areas must not.
- `src/io/MapExporter_IO.h` / `src/io/MapImporter_IO.h` — narrow SCOPE NOTE 1 / SCOPE NOTE 2 so
  they no longer claim `areas`/`armies` are empty (they still correctly describe
  `markers`/`props`/`decals`/`chains`, which stay out of scope — see below).
- `src/io/MapExporter_IO_Test.cpp` — `TestDocumentCarriesTheFormatsOwnFields`'s comment on line
  72 currently reads "the entity domains are written empty and valid" as a blanket claim; narrow
  it to the four domains that are still actually empty.
- `src/io/MapFormat_TestSupport_IO.h` / `MapImporter_IO_Test.cpp` — extend the shared fixture
  (`BuildPopulatedRecipe`) and the round-trip check (`RunRoundTripTests`) with armies/areas
  content, following the exact pattern `CheckLayerStackAndRules`/`FillFixturePlacementRules`
  already use for the rule vectors.

## Layer & accuracy class
PARAMS + IO/BRIDGE. Accuracy class: Exact (format-shape correctness and round-trip fidelity,
not a numeric tolerance) for everything except rotation, which is explicitly scoped as
best-effort pass-through (see "Rotation" below).

## Backend policy
CPU only — JSON text I/O, not a dispatchable calculation.

## ARCH rules invoked
- `ENTITY_AUTHORING_PARAMS_SPEC.md` (first session) — the ratified `Faction`/`UnitTransform`/
  `UnitGroup`/`Army`/`MapArea` shapes and field names. Implement exactly as spec'd; do not
  retype or rename anything here without a fresh ruling.
- `SANMAP_FORMAT_SPEC.md` "Entity collections" section and Correction 11 (`armyColor`/`alias`
  merge into `armies[key]`).
- ARCH_01_08_ParamsFieldNamingByKind.md §1.8 (naming law) — already applied by the spec; this work-order does not re-derive it.
- Constitution §6 — every field is validated on import; a missing/wrong-typed key falls back to
  default and is logged, never crashes or silently corrupts.
- Constitution §8 — no format string literal at more than one write site; reuse the existing
  `ReadJson*`/`ReadJsonEnumeration` typed accessors in `MapImporter_Recipe_IO.h` rather than
  hand-rolling new ones.

## Expert review (both completed — this revision folds in both verdicts)

**ARCH Expert: CONFORMANT, no blocking changes.** Verified the PARAMS shape is a byte-for-byte
match to `ENTITY_AUTHORING_PARAMS_SPEC.md`, module boundaries hold (IO's coordinate-flip
arithmetic is a format-transform at the IO seam, not a simulation — same class of operation
`SANMAP_FORMAT_SPEC.md:97-101` already assigns to IO: "Export applies it; import must invert
it"), and naming law has no drift. Two notes folded in below: `armyColor`'s shape is upgraded
from "inferred" to "confirmed by an already-ratified general rule" (see finding 5), and the
rotation call needs an explicit one-line human/Format-Expert sign-off that it satisfies
`ENTITY_AUTHORING_PARAMS_SPEC`'s "must close the rotation gap" instruction (see finding 4) —
the interpretation is reasonable but is an interpretation, not a re-derivation.

**IO Architecture Expert: file split into four (not two), shared headers unchanged, no shared
JSON-walk helper yet, no migration/version bump — AND caught a real wiring bug in the original
draft's point 6.** All folded into "Target files" above and "Critical wiring correction" below.

## Critical wiring correction (IO Architecture Expert catch — this is why the pipeline exists)
The original draft of this work-order (point 5/6, now corrected) instructed calling the new
readers "after `ReadGeometryJson`" — inside `MapImporter_IO.cpp`'s `if (!document.contains
("mapGeneratorData")...) return true;` gate. **That is wrong and would have shipped a real bug.**
`armies`/`areas` are top-level `.sanmap` keys, siblings of `mapGeneratorData`
(`SANMAP_FORMAT_SPEC.md`'s "Entity collections" section lists them under "Top-level map fields",
separate from `mapGeneratorData`) — confirmed directly against `MapExporter_Recipe_IO.cpp:88-89`,
which writes `document["areas"]`/`document["armies"]` at the same level as `document["mapGeneratorData"]`,
not inside it. But `MapImporter_IO.cpp:84-87` returns early, before any block reader runs, when
`mapGeneratorData` is absent or not an object. A hand-authored `.sanmap` (e.g. from the real Unity
editor) carrying real `armies`/`areas` content but no `mapGeneratorData` block — precisely the
scenario this ticket exists to support — would have had its armies/areas silently skipped
entirely, directly contradicting this ticket's own purpose.
**Fix:** `ReadAreasJson(document, outRecipe)` and `ReadArmiesJson(document, outRecipe)` take the
top-level `document`, not `generatorData`, and are called unconditionally, right after the
existing top-level `width`/`height` reads (`MapImporter_IO.cpp:77-82`) and BEFORE the
`mapGeneratorData`-presence check. This does not complicate the flip-inverse: `outRecipe.geometry.
mapSize` is already populated from the top-level `width` key at that point, before the gate.

## Ground-truth findings this proposal is based on (verified against real files, not the specs
alone — the specs deliberately left some of this open for "the entity-export work-order")

1. **JSON shape is DICTIONARY, not array — this differs from the Rules precedent.**
   `SanMap.cs:147-148`: `Dictionary<string, Area> areas` and `Dictionary<string, Army> armies`.
   `Army.groups`/`UnitGroup.units`/`UnitGroup.groups` are dictionaries too
   (`SanMap.Types.cs:84,90-91`). The existing `MapExporter_Rules_IO.cpp`/
   `MapImporter_Rules_IO.cpp` pair (the four placement-rule vectors) is an ARRAY pattern —
   **do not copy it verbatim**. `BuildArmiesJson`/`BuildAreasJson` must build a JSON *object*
   keyed by each `Army::name`/`MapArea::name`/`UnitGroup::name`/`UnitTransform::name` (the
   folded-in dictionary key, per the spec's structural ruling), not a `nlohmann::ordered_json::array()`.
   The reader needs a `parent[key].items()` walk (keyed by JSON key = the `name` field) instead
   of the existing `ReadRuleArray` template, and the `UnitGroup.groups`/`UnitGroup.units` levels
   recurse the same way one level deeper.

2. **Every `InstancedTransform`-shaped field name is `position`/`rotation`/`scale`, each a
   nested `{x,y,z}` / `{x,y,z,w}` object** (`Types.cs` `Vector3`/`Quaternion`: fields literally
   `x,y,z` / `x,y,z,w`) — confirmed directly from `Types.cs:20-32,50-66`, not inferred. Write/read
   `UnitTransform` as nested `position`/`rotation`/`scale` objects, each built from the flat
   `positionX/Y/Z`/`rotationX/Y/Z/W`/`scaleX/Y/Z` PARAMS fields — mirroring the existing
   `ReadJsonFloatVector4` pattern in shape (four named-component object), but three of these are
   3-component, not 4. `type`/`tpid` are siblings of `position`/`rotation`/`scale` at the same
   object level, not nested under them. `tpid` reuses the exact bounded-buffer convention
   `MapExporter_Rules_IO.cpp`/`MapImporter_Rules_IO.cpp` already use for `ScatterTransform::
   templateIdentifier` (`char[8]`, NUL-safe copy) — same field, same type, same file family's
   sibling pattern; do not reinvent it.

3. **The coordinate flip applies to `UnitTransform.position.z` but NOT to `MapArea`.** This was
   genuinely ambiguous from the specs alone (`SANMAP_FORMAT_SPEC`'s flip note lists "markers/
   entities" without naming every domain) and is resolved here by reading the actual reference
   converter, `MapUtils.cs:GetSanMap`:
   - `TextureToWorldOrigin` (`MapUtils.cs:206-209`, `new Vector3(x, y, textureHeight - z - 1)`)
     is applied to marker (`MapUtils.cs:121,136`) and decal (`MapUtils.cs:166`) positions — every
     domain built on `InstancedTransform`. `UnitTransform` is `: InstancedTransform` too
     (`SanMap.Types.cs:95`) and `SANMAP_FORMAT_SPEC`'s flip note explicitly names "units" as a
     flip target — so `UnitTransform.positionZ` gets the same treatment on export
     (`world.z = mapSize - positionZ - 1`) and import (invert: `positionZ = mapSize -
     world.z - 1`). `positionX`/`positionY` and `rotation`/`scale` are untouched by the flip.
     "`mapSize`" here is `recipe.geometry.mapSize` (the format's `length`, per `SanMap.cs:23`
     and `MapUtils.cs:208`'s `textureHeight` parameter — confirmed the same map-length value,
     not a separate quantity).
   - `MapArea` (the `Area{x,y,width,height}` written for `PlayableArea`) is built at
     `MapUtils.cs:152` DIRECTLY from `map.PlayableArea.GetX/Y/Z/W()` — **it never passes through
     `TextureToWorldOrigin`**, unlike every entity transform in the same function. This is
     positive evidence, not silence: the one call site that populates an `Area` in the whole
     reference converter deliberately skips the flip helper every other domain in the same
     function uses. **`MapArea.originX`/`originZ` round-trip verbatim, no flip.**

4. **Rotation: round-trip verbatim, do NOT invent a flip-conversion.** The known defect
   (`SESSION_HANDOFF_ImportExport.md` §5 item 5, `ENTITY_AUTHORING_PARAMS_SPEC`'s flagged item 2)
   is that the reference exporter writes an **identity quaternion for every entity, unconditionally**
   (`MapUtils.cs:121,136,166`: `new Quaternion()` with a `// TODO: Convert the transform.rotation
   angles to quaternion` comment) — it never even attempts a conversion, for any domain, so there
   is no real-world formula anywhere in the codebase to confirm or reuse for how a Z-mirror should
   affect a quaternion. Inventing one now would be exactly the "synthesize a fallback" Constitution
   §6 forbids. This work-order's `UnitTransform.rotationX/Y/Z/W` reads/writes the JSON
   `rotation.{x,y,z,w}` object **verbatim, with no coordinate transform applied** — which still
   closes the letter of the defect for armies specifically (previously `armies` was written as an
   empty object, so literally nothing was written; now a designer's real stored rotation is
   written instead of a forced identity). If a real flip-conversion is later confirmed, that is a
   fresh Format-Expert/ARCH ruling, not an inference from this ticket's evidence.
   **ARCH Expert nuance (not a blocking objection):** reading "write the real stored rotation
   instead of a forced identity" as satisfying `ENTITY_AUTHORING_PARAMS_SPEC`'s "the entity-export
   work-order... must close [the rotation] gap" instruction is a reasonable, conservative
   interpretation — but it IS an interpretation, not a re-derivation from new evidence. Wants an
   explicit one-line human sign-off on this reading before implementation, not a silent assumption.

5. **`armyColor` JSON shape — CONFIRMED, not merely inferred (per ARCH Expert review).**
   `Params::Army::armyColor` is a SanGen-added field (Correction 11) with no direct C# analog,
   but `SANMAP_FORMAT_SPEC.md:669-681` already documents `Color ({r,g,b,a})` as this format
   pack's own ratified general shape for a SanGen-added Color field (used for
   `PropInstanceLayer`/`DecalInstanceLayer::color`), and `diffuseRemap`
   (`MapExporter_Recipe_IO.cpp:25-26`, already shipped) writes the identical `{r,g,b,a}` shape.
   `{"r":..,"g":..,"b":..,"a":..}` is therefore the established convention, not a fresh
   invention — implement it with that confidence.

## Solution

1. **`Army_PARAMS.h`** (new, `namespace SanmapGen::Params`):
   ```cpp
   enum class Faction { Chosen, Guard, EDA };   // 0/1/2

   struct UnitTransform {
       std::string name;
       float positionX = 0.0f, positionY = 0.0f, positionZ = 0.0f;
       float rotationX = 0.0f, rotationY = 0.0f, rotationZ = 0.0f, rotationW = 1.0f;
       float scaleX = 1.0f, scaleY = 1.0f, scaleZ = 1.0f;
       char        templateIdentifier[8] = { 0,0,0,0,0,0,0,0 };
       std::string legacyTypeTag;
   };

   struct UnitGroup {
       std::string name;
       std::vector<UnitTransform> units;
       std::vector<UnitGroup>     groups;
   };

   struct Army {
       std::string name;
       Faction     faction = Faction::Chosen;
       float       alloys  = 500.0f;
       float       energy  = 500.0f;
       std::vector<UnitGroup> groups;
       float       armyColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
       std::string alias;
   };
   ```
   (Verbatim from `ENTITY_AUTHORING_PARAMS_SPEC.md`'s "The types" section — no deviation.)

2. **`MapArea_PARAMS.h`** (new):
   ```cpp
   struct MapArea {
       std::string name;
       float originX = 0.0f;
       float originZ = 0.0f;
       float width   = 0.0f;
       float length  = 0.0f;
   };
   ```

3. **`MapRecipe_PARAMS.h`**: add `#include "Army_PARAMS.h"`, `#include "MapArea_PARAMS.h"`, and
   `std::vector<Army> armies; std::vector<MapArea> areas;` members. Do not touch `IsValid()` —
   an empty armies/areas list is legal (SANMAP_FORMAT_SPEC never requires either to be non-empty
   at the recipe-validity level; `PlayableArea` presence is a UI-layer rule — `AreasTab_List_UI.h`
   `EnsurePlayableArea` — not an IO-layer one).

4. **`MapExporter_Areas_IO.cpp`** (new, own file per the IO Architecture Expert's split ruling —
   `areas` and `armies` are independent top-level domains with no shared JSON parent, unlike the
   four `PlacementRules` arrays which genuinely share one). One public function:
   ```cpp
   nlohmann::ordered_json BuildAreasJson(const Params::MapRecipe& recipe);
   ```
   One JSON object, one key per `MapArea::name`, value
   `{"x":originX,"y":originZ,"width":width,"height":length}` — verbatim pass-through, no flip
   (finding 3). Flat; no recursion, no shared helper needed.

5. **`MapExporter_Armies_IO.cpp`** (new, own file — separate from Areas per the same ruling; this
   is the more complex of the two, with three levels of recursion, a `Color`-shaped field, and the
   bounded `tpid` buffer copy, so keeping it apart from `Areas` also protects the ARCH_01_05_FileSizeCeilings.md §1.5
   file-size ceiling). One public function:
   ```cpp
   nlohmann::ordered_json BuildArmiesJson(const Params::MapRecipe& recipe);
   ```
   One JSON object, one key per `Army::name`, value
   `{"faction":int(faction),"alloys":alloys,"energy":energy,"groups":<object>,
   "armyColor":{"r":..,"g":..,"b":..,"a":..},"alias":alias}`. `groups` is itself an object keyed
   by `UnitGroup::name`, each value `{"units":<object>,"groups":<recursive object>}`. `units` is
   an object keyed by `UnitTransform::name`, each value
   `{"position":{"x":positionX,"y":positionY,"z":mapSize - positionZ - 1},
   "rotation":{"x":rotationX,"y":rotationY,"z":rotationZ,"w":rotationW},
   "scale":{"x":scaleX,"y":scaleY,"z":scaleZ},"type":legacyTypeTag,
   "tpid":<bounded string from templateIdentifier, same pattern as BuildScatterTransformJson>}`.
   Write a small recursive helper for `UnitGroup` (it nests) — local to this file.

6. **`MapImporter_Areas_IO.cpp`** (new, own file). One public function:
   ```cpp
   void ReadAreasJson(const nlohmann::json& document, Params::MapRecipe& outRecipe);
   ```
   Total per Constitution §6: a missing `document["areas"]` key, or one that isn't a JSON object,
   leaves `outRecipe.areas` untouched (empty, the default). A plain `for (auto& [key, value] :
   document["areas"].items())` loop, skipping a non-object `value` per entry rather than aborting
   the whole domain — no template/recursion needed, `Area` is flat.

7. **`MapImporter_Armies_IO.cpp`** (new, own file). One public function:
   ```cpp
   void ReadArmiesJson(const nlohmann::json& document, Params::MapRecipe& outRecipe);
   ```
   Same total/degrade-gracefully posture. This is the one domain that genuinely needs a
   recursive name-keyed-object walker (`armies` → `Army.groups` → `UnitGroup.groups`/
   `UnitGroup.units`, three levels) — write it as a local template in this file's own anonymous
   namespace, exactly mirroring `ReadRuleArray`'s role in `MapImporter_Rules_IO.cpp` (which is
   itself file-local despite being reused four times — the established bar is "share only when a
   second FILE needs it," not "share the moment a pattern repeats once"). Do NOT promote this to
   `MapImporter_Recipe_IO.h` or invent a `JsonPrimitives_IO.h` file for it — that shared-primitives
   file is `IO_MIGRATION_SPEC.md`'s eventual home for exactly this class of helper, but it isn't
   built yet and building it now as a side effect of this ticket would be scope creep; promote only
   if a THIRD domain later needs the identical shape.
   `faction`/enum values read through the existing `ReadJsonEnumeration` (3 values) so an
   out-of-range int degrades to the field's current default instead of corrupting the enum.
   `positionZ` reads as `mapSize - jsonZ - 1` (finding 3's inverse) using `outRecipe.geometry.
   mapSize` — see "Critical wiring correction" above for exactly where this must be called from.

8. **Wire into `BuildSanmapJsonText`** (`MapExporter_Recipe_IO.cpp`): replace the two
   `document["areas"] = ...empty...; document["armies"] = ...empty...;` lines with real calls to
   `BuildAreasJson(recipe)`/`BuildArmiesJson(recipe)`, same call style as the existing
   `BuildStratumLayersJson`/`BuildMapGeneratorDataJson` calls.
   **Wire into `ParseSanmapJsonText`** (`MapImporter_IO.cpp`): call `ReadAreasJson(document,
   outRecipe)`/`ReadArmiesJson(document, outRecipe)` immediately after the existing top-level
   `width` read (line 82) and BEFORE the `if (!document.contains("mapGeneratorData")...)` gate —
   see "Critical wiring correction" above. This is the one step in this work-order most likely to
   be gotten wrong by copying the existing call style too literally; the Coder should re-read that
   section before writing this step.

9. **Update the two SCOPE NOTE comments** (`MapExporter_IO.h` SCOPE NOTE 1, `MapImporter_IO.h`
   SCOPE NOTE 2) to say `areas`/`armies` now round-trip; `markers`/`props`/`decals`/`chains`
   remain empty/unread and out of scope (unchanged reasoning, narrower claim).

10. **Extend the shared test fixture** (`MapFormat_TestSupport_IO.h`'s `BuildPopulatedRecipe`,
   defined in `MapImporter_IO_Test.cpp`): add one `Params::MapArea` (non-default origin/width/
   length, distinct from `{0,0,0,0}`) and one `Params::Army` with a non-default `faction`,
   `armyColor`, `alias`, and at least one nested `UnitGroup` containing one `UnitTransform` with
   a non-default position (including a non-zero `positionZ` so the flip is actually exercised),
   non-identity rotation, non-unit scale, and a non-empty `templateIdentifier`/`legacyTypeTag`.
   Add a `CheckArmiesAndAreas` function beside `CheckLayerStackAndRules`, called from
   `RunRoundTripTests`, asserting: the area's `originZ` equals what was set (NOT flipped), and the
   unit's roundtripped `positionZ` equals the original `positionZ` (i.e. flip-then-unflip is the
   identity — the test never needs to know the map-size constant, only that going out and back
   returns the original value), and the rotation/scale/tpid/legacyTypeTag/name/armyColor/alias
   all survive exactly.

## Explicit out-of-scope
- **`markers`/`props`/`decals`/`chains`** — separate work-order(s), per the handoff's §3 table
  and §6 deferred items (blueprint-path validation in particular needs its own design before any
  of those four can export safely).
- **`UnitRule::armyIndex`'s relationship to the new `recipe.armies` list** — explicitly deferred
  by `ENTITY_AUTHORING_PARAMS_SPEC` itself ("left as an open integration question for whoever
  writes the entity-export work-order") and by handoff §6 item 4. This work-order does not touch
  `ScatterRule_PARAMS.h` or resolve what `armyIndex` indexes into.
- **UI wiring** — `ArmiesTab_UI.h`'s `ArmyPresentation`/SCOPE NOTE 1 and `AreasTab_List_UI.h`'s
  `MapAreaRectangle` stay exactly as they are; neither is retyped onto `Params::Army`/`MapArea`
  here. The spec itself defers this ("how `ArmyPresentation` folds into or is replaced by
  `Params::Army` ... is UI/PIPELINE wiring ... not decided by this spec") and the handoff's
  recommended-next-step section calls out UI-scope-note retirement as its own coder work-order.
  Consequence: after this ticket, a designer editing armies/areas in the UI still edits
  presentation-only state that isn't serialized — this ticket only makes the *format* capable of
  carrying real data (e.g. via a hand-edited or externally-imported `.sanmap`), it does not yet
  make the SanGen UI produce that data. Worth flagging to the user as a real, if expected, gap.
- **Props/decals rotation and the "props export disabled" defect** — not touched; those live on
  `PropTransform`/`DecalTransform`, not `UnitTransform`, and are explicitly a separate domain
  (see out-of-scope item 1).
- **Name-uniqueness enforcement on import** — `MakeAreaNamesUnique` is a UI-layer rule
  (`AreasTab_List_UI.h`) triggered on edit, not an IO-layer invariant; a hand-edited file with two
  areas sharing a name is legal JSON (impossible actually — JSON object keys are inherently
  unique; the last-written one simply wins, which is the correct total/non-crashing behavior
  Constitution §6 asks for) and needs no special-case handling here.

## Open items for ratification — STATUS AFTER BOTH EXPERT REVIEWS
1. ~~`armyColor`'s `{r,g,b,a}` JSON shape~~ — **RESOLVED.** ARCH Expert confirmed this against
   `SANMAP_FORMAT_SPEC.md`'s already-ratified `Color ({r,g,b,a})` general convention (finding 5).
   No longer open.
2. **RATIFIED by the human, pending external confirmation.** Rotation is round-tripped verbatim
   with no flip-conversion (finding 4). Both experts agree there is no ground truth to invent a
   formula from, and the human has approved shipping verbatim pass-through now rather than
   blocking on an answer of unknown timing: nothing is written for armies today at all (empty
   placeholder), so verbatim is strictly an improvement, and inventing an unverified mirror formula
   risks the same wrongness with less evidence behind it. **Standing question for the live-engine/
   Unity-editor developer, to be asked out of band:** "In the `.sanmap` format, an entity's position
   gets a Z-axis mirror when converting between the generator's texture-space and the engine's
   world-space (`world.z = mapSize − textureZ − 1`). Does the entity's rotation quaternion need a
   matching transform for that same conversion, or does rotation carry over unchanged?" If the
   answer confirms a transform is needed, that is a fresh, narrowly-scoped follow-up work-order
   (touching only the rotation read/write sites in `MapExporter_Armies_IO.cpp`/
   `MapImporter_Armies_IO.cpp` added here, plus the same question likely applies to
   `PropTransform`/`DecalTransform`/`MarkerTransform` rotation once those domains are wired) — not
   a reason to hold this ticket.
3. ~~New IO file names~~ — **RESOLVED.** IO Architecture Expert ruled: split into four files
   (`MapExporter_Armies_IO.cpp`/`MapExporter_Areas_IO.cpp`/`MapImporter_Armies_IO.cpp`/
   `MapImporter_Areas_IO.cpp`), not two — `armies`/`areas` are independent top-level domains with
   no shared JSON parent, unlike the four `PlacementRules` arrays. Folded into "Target files" and
   the Solution steps above. No longer open.
4. **NEW — the most important finding of the whole review pass.** The IO Architecture Expert
   caught a real wiring bug in the original draft: the new readers were originally placed inside
   `MapImporter_IO.cpp`'s `mapGeneratorData`-presence gate, which would have silently skipped
   `armies`/`areas` on any hand-authored file lacking a `mapGeneratorData` block — directly
   contradicting this ticket's purpose. **Fixed** in "Critical wiring correction" and Solution
   step 8 above. This is resolved in the document, but is called out here because it's exactly the
   kind of error the propose→ratify→implement pipeline exists to catch before a Coder ships it.

## Acceptance test
`out/build/.../MapImporter_IO_Test.exe` (the existing round-trip binary) passes with the
extended fixture: `RunRoundTripTests` reports the new `CheckArmiesAndAreas` assertions passing,
`result.warningCount == 0` still holds (no key the exporter writes should ever produce an import
warning), and the pre-existing assertions (`CheckGeometryAndWater`, `CheckLayerStackAndRules`)
are unaffected. `MapExporter_IO_Test.exe`'s existing tests still pass with the narrowed SCOPE
NOTE comment. Full `SanGenV2` build stays clean.
