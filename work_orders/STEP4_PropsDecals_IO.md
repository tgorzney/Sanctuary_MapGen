# Work-Order — Step 4: Props + Decals PARAMS + pure JSON round-trip (NOT yet live-wired)

*Constitution §7. Executor: SanGen Coder. Proposed against the already-ratified
`ENTITY_AUTHORING_PARAMS_SPEC.md` (second and fourth ratification sessions:
`PropTransform`/`DecalTransform`/`PropInstanceGroup`/`DecalInstanceGroup`/`PropInstanceLayer`/
`DecalInstanceLayer`) and `SANMAP_FORMAT_SPEC.md` Correction 14 (`PropGroups`/`DecalGroups`).
Follows `STEP2_ArmiesAreas_IO.md`/`STEP3_MarkersChains_IO.md`'s pattern and file-split
conventions directly.

**This ticket deliberately does NOT wire props/decals into the live exported document — read
"Why this ticket stops short of live-wiring" before implementing anything.** It is the first of
two tickets; the second (not yet drafted, pending a UI Expert consult on a new confirmation-dialog
widget) is what actually connects this code to a real export.*

## Title
Give `PropTransform`/`DecalTransform`/`PropInstanceGroup`/`DecalInstanceGroup`/
`PropInstanceLayer`/`DecalInstanceLayer` a real `Params::` home, and build (but do not yet
live-wire) their `.sanmap` `props`/`decals`/`PropGroups`/`DecalGroups` JSON round-trip.

## Root problem
`PropTransform`/`DecalTransform`/`PropInstanceGroup`/`DecalInstanceGroup`/`PropInstanceLayer`/
`DecalInstanceLayer` are ratified but unimplemented — no `Params::` type exists, `MapRecipe_
PARAMS.h` has no `props`/`decals`/`propLayers`/`decalLayers` members, and `MapExporter_Recipe_
IO.cpp:93-94` writes `document["props"]`/`document["decals"]` as permanently empty arrays.
`PropsTab_Manual_UI.h`'s SCOPE NOTE 1 says the same thing from the UI side: "v1's manual prop
GROUPS ... have no `_PARAMS` home in the tree."

## Why this ticket stops short of live-wiring
`ENTITY_AUTHORING_PARAMS_SPEC.md`'s flagged item 1 is unambiguous: `blueprintPath` validation is
"mandatory before any export of `PropInstanceGroup`/`DecalInstanceGroup`" — a single unresolvable
path aborts the rest of the map's load in-game (confirmed via the real Lua loader,
`mapUtils.lua:107`: an unresolved path calls `Engine.Error()`, which kills `RunMapSetup` before
anything parsed after `props` in file order ever runs). The human has ruled (this session) that
the actual UX should be: **warn the designer with an explicit confirm dialog explaining the
runtime risk, and if they click OK anyway, export proceeds and writes the paths verbatim** — never
silently drop, never silently block. That dialog does not exist yet (no confirmation-modal widget
exists anywhere in this UI codebase today — confirmed by search). Shipping the live JSON write
sites now, before that dialog exists, would mean a designer could export a map with a completely
unresolvable `blueprintPath` and get no warning at all until the game crashes on load — reproducing
exactly the defect the spec calls mandatory to prevent.

So: this ticket builds everything that is safe to build without that risk — the PARAMS types and
a fully independently-tested, pure (disk-free) JSON round-trip, following the exact same pattern
`BuildAreasJson`/`BuildMarkersJson`/etc. already use — but `document["props"]`/`document["decals"]`
in `BuildSanmapJsonText` KEEP writing empty arrays, exactly as today. The follow-up ticket (Step 5,
not yet drafted) adds the validation predicate, the confirm-dialog widget, and the Files-tab
wiring, and is the one that flips `BuildSanmapJsonText` to call these new builders for real.

## Target files
New:
- `src/params/PropInstance_PARAMS.h` — `PropTransform`, `DecalTransform`, `PropInstanceGroup`,
  `DecalInstanceGroup`, `PropInstanceLayer`, `DecalInstanceLayer` together (per the spec's own
  "File organization" section — these share one file, same precedent as `ScatterRule_PARAMS.h`
  bundling `PropRule`/`DecalRule`/`UnitRule`).
- `src/io/MapExporter_Props_IO.cpp` — `BuildPropsJson`, `BuildPropGroupsJson`.
- `src/io/MapExporter_Decals_IO.cpp` — `BuildDecalsJson`, `BuildDecalGroupsJson`.
- `src/io/MapImporter_Props_IO.cpp` — `ReadPropsJson`, `ReadPropGroupsJson`.
- `src/io/MapImporter_Decals_IO.cpp` — `ReadDecalsJson`, `ReadDecalGroupsJson`.
  (Four IO files, one per top-level format domain, same split ruling as Steps 2/3 — `props`/
  `decals` are independent top-level `.sanmap` keys, and `PropGroups`/`DecalGroups` are their own
  new SanGen-owned top-level siblings per Correction 14, small enough to share the file with their
  parent domain rather than warranting yet another split.)

Modified:
- `src/params/MapRecipe_PARAMS.h` — add `std::vector<PropInstanceGroup> props;
  std::vector<DecalInstanceGroup> decals; std::vector<PropInstanceLayer> propLayers;
  std::vector<DecalInstanceLayer> decalLayers;` and the one new include.
- `src/io/MapExporter_Recipe_IO.h` — declare the four new builders (join the shared header, same
  as every prior step).
- `src/io/MapImporter_Recipe_IO.h` — declare the four new readers.
- **`src/io/MapExporter_Recipe_IO.cpp` — DELIBERATELY NOT touched at the two empty-array write
  sites** (`document["props"]`/`document["decals"]` stay `nlohmann::ordered_json::array()`, exactly
  as today). Do NOT add `document["PropGroups"]`/`document["DecalGroups"]` to the live document
  either — same reasoning, same hold. The new builder functions exist and are tested; they are
  simply not called from `BuildSanmapJsonText` yet.
- **`src/io/MapImporter_IO.cpp` — also NOT touched.** No `ReadPropsJson`/`ReadDecalsJson` calls
  added to `ParseSanmapJsonText`. (This is the one meaningful deviation from Steps 2/3's pattern —
  those wired straight into the live parse; this one deliberately does not, for the reason above.)
- `src/io/MapExporter_IO.h`/`MapImporter_IO.h`/`MapExporter_IO_Test.cpp` — SCOPE NOTE wording only:
  clarify that props/decals PARAMS+builders now exist and are independently tested, but the live
  document still withholds real content pending the blueprintPath validation/confirm-dialog ticket
  — do not claim more than that.
- A new test file or an addition to the existing `MapFormat_TestSupport_IO.h` family — see
  "Acceptance test" below. This does NOT touch `RunRoundTripTests`/`BuildPopulatedRecipe` (those
  drive the LIVE `BuildSanmapJsonText`/`ParseSanmapJsonText` pair, which correctly still produce
  empty `props`/`decals` — asserting real prop/decal content through that path would be asserting
  something this ticket deliberately does not yet do).

## Layer & accuracy class
PARAMS + IO/BRIDGE. Accuracy class: Exact for shape/JSON correctness; rotation is best-effort
verbatim pass-through (same standing ruling as Steps 2/3, reused, not reopened).

## Backend policy
CPU only.

## ARCH rules invoked
- `ENTITY_AUTHORING_PARAMS_SPEC.md` (second session's `PropTransform`/`DecalTransform`/
  `PropInstanceGroup`/`DecalInstanceGroup`, fourth session's `layerIndex`/`PropInstanceLayer`/
  `DecalInstanceLayer` additions and the "why props/decals now need a wrapper transform type"
  supersession).
- `SANMAP_FORMAT_SPEC.md` Correction 14 (`PropGroups`/`DecalGroups` shape and casing).
- ARCH §12 (the `layerIndex` direct-field-injection ruling, and the range-validation-is-import-only
  posture).
- Constitution §6 — total/degrade-gracefully readers; `layerIndex` out of range against
  `propLayers`/`decalLayers` is a loud logged clamp to `0`, per-instance, not per-file (ARCH §12's
  own explicit instruction — this is the one place in this ticket where import validation logic is
  real and required, distinct from the blueprintPath question this ticket defers).

## Ground-truth findings

1. **JSON shape for `props`/`decals` — ARRAY, not dictionary, unlike every prior step.** Confirmed
   directly: `SanMap.cs:153,157`: `PropType[] decals = new DecalType[0]` / `PropType[] props = new
   PropType[0]` are bare C# arrays, not `Dictionary<string,...>`. This is the Rules-array pattern
   (`ReadRuleArray`-style), NOT the Armies/Markers dictionary-fold pattern — `PropInstanceGroup`/
   `DecalInstanceGroup` have no folded-in `name` key at their own level (confirmed:
   `ENTITY_AUTHORING_PARAMS_SPEC.md`: "`props`/`decals` need no dict→vector conversion at all ...
   `std::vector` is the direct, verbatim translation; no `name` field is invented"). Each array
   entry is `{"blueprintPath":"...","transforms":[...]}` — `transforms` IS an array too (`List<
   PropTransform>`, `SanMap.Types.cs:112`), not a dictionary — `PropTransform`/`DecalTransform`
   have no folded-in name either. So this domain needs a plain array-of-objects builder/reader,
   closer to `MapExporter_Rules_IO.cpp`'s shape than to Armies'/Markers' — do not reach for the
   `ReadNameKeyedObject` pattern here, it doesn't apply.

2. **`PropTransform`/`DecalTransform` compose `InstancedTransform` plus `layerIndex`, per the
   fourth-session supersession — this is NOT the same shape as the second session's original
   ruling.** Verbatim from the spec:
   ```cpp
   struct PropTransform  { InstancedTransform transform; int layerIndex = 0; };
   struct DecalTransform { InstancedTransform transform; int layerIndex = 0; };
   ```
   On the wire, `position`/`rotation`/`scale` are top-level siblings of `layerIndex` inside one
   JSON object (same flattening-on-the-wire-despite-composing-in-C++ pattern `MarkerTransform`
   already established in Step 3) — `{"position":{...},"rotation":{...},"scale":{...},
   "layerIndex":N}`.

3. **Coordinate flip — confirmed for decals, evidenced-but-unexecuted for props (same ruling
   applies to both).** `MapUtils.cs:166`: decal transforms are built via
   `TextureToWorldOrigin(transform.Position, map.Length)` — an ACTUALLY EXECUTED flip, the
   strongest evidence class. Props (`MapUtils.cs:179-201`) show the IDENTICAL call pattern
   (`TextureToWorldOrigin(transform.Position, map.Length)`, line 191) but the whole block is
   commented out (props export is disabled in the reference tool — "many prop formats are outdated,
   causing maps to fail loading"). This is still positive evidence of INTENT (the exact same
   flip call was written for props, just never executed because props export itself was disabled
   for an unrelated reason) — apply the identical flip to `PropTransform.transform.positionZ` as
   `DecalTransform.transform.positionZ`: `mapSize - positionZ - 1` on export, inverse on import.

4. **Rotation — same verbatim ruling as Steps 2/3, same evidence class, reused not reopened.**
   `MapUtils.cs:166,191` both write `new Quaternion()` unconditionally with the identical
   `// TODO: Convert the transform.rotation angles to quaternion` comment already cited twice.
   Round-trip `rotationX/Y/Z/W` verbatim.

5. **`PropGroups`/`DecalGroups` shape — confirmed via `SANMAP_FORMAT_SPEC.md` Correction 14**
   (already read in full for Step 2's `armyColor` finding): top-level PascalCase arrays, siblings
   of `props`/`decals` (i.e., top-level document keys, not nested in `mapGeneratorData`):
   ```
   PropGroups / DecalGroups: [ N × { Name (string), Color ({r,g,b,a}), IconScale (float) } ]
   ```
   PascalCase field names (`Name`, `Color`, `IconScale`) — this is a SanGen-owned array, casing
   law differs from the lowerCamelCase format-native `props`/`decals` keys themselves (ARCH §1.6).
   `Color` reuses the same `{r,g,b,a}` shape already confirmed and shipped for `armyColor` in
   Step 2 — same convention, not a fresh decision.

6. **`layerIndex` import-time range validation is real, required logic in THIS ticket — not
   deferred like blueprintPath.** ARCH §12: an out-of-range `layerIndex` (at or beyond
   `propLayers.size()`/`decalLayers.size()`) is a loud, logged clamp to `0`, applied per-instance
   on import (different instances in the same file can carry different out-of-range values, so
   this cannot be a single file-scope check). This is unlike blueprintPath — it's authoring-
   convenience metadata, not gameplay-authoritative, so ARCH already settled its failure mode
   (clamp, never abort) and there's no open question to defer here.

## Solution
1. **`PropInstance_PARAMS.h`** — verbatim from the spec's "The types" section:
   ```cpp
   struct PropTransform  { InstancedTransform transform; int layerIndex = 0; };
   struct DecalTransform { InstancedTransform transform; int layerIndex = 0; };
   struct PropInstanceGroup  { std::string blueprintPath; std::vector<PropTransform>  transforms; };
   struct DecalInstanceGroup { std::string blueprintPath; std::vector<DecalTransform> transforms; };
   struct PropInstanceLayer  { std::string name; float color[4] = {1,1,1,1}; float iconScale = 1.0f; };
   struct DecalInstanceLayer { std::string name; float color[4] = {1,1,1,1}; float iconScale = 1.0f; };
   ```
   (`#include "InstancedTransform_PARAMS.h"`, already created in Step 3.)
2. **`MapRecipe_PARAMS.h`**: add the four vectors and the one include. `IsValid()` untouched.
3. **`MapExporter_Props_IO.cpp`**: `BuildPropsJson` — plain array of `{"blueprintPath":...,
   "transforms":[ per finding 2's shape, flip per finding 3 ]}`. `BuildPropGroupsJson` — plain
   array of `{"Name":...,"Color":{r,g,b,a},"IconScale":...}` per finding 5. Both pure functions,
   no disk access, no validation call of any kind.
4. **`MapExporter_Decals_IO.cpp`**: the same shape, for decals.
5. **`MapImporter_Props_IO.cpp`**: `ReadPropsJson`/`ReadPropGroupsJson`, the exact inverse,
   including the `layerIndex` clamp-to-0-if-out-of-range-against-`propLayers.size()` from finding
   6, applied per instance while walking `transforms`.
6. **`MapImporter_Decals_IO.cpp`**: the same, for decals.
7. **Do NOT wire any of the above into `BuildSanmapJsonText`/`ParseSanmapJsonText`.** This is not
   an oversight — see "Why this ticket stops short of live-wiring." Narrow the SCOPE NOTE wording
   only, per "Target files" above.
8. **New test coverage, NOT via the live round-trip fixture**: write a small, separate,
   self-contained test (either a new `MapExporter_PropsDecals_IO_Test.cpp`/
   `MapImporter_PropsDecals_IO_Test.cpp` pair, or add standalone `Test*` functions inside the new
   `.cpp` files' own translation units if this codebase's test-runner convention supports that —
   check how `MapExporter_IO_Test.cpp`'s existing free-standing tests are registered and mirror
   that) that calls `BuildPropsJson`/`BuildDecalsJson`/`BuildPropGroupsJson`/`BuildDecalGroupsJson`
   directly against a hand-built `Params::MapRecipe`, and `ReadPropsJson`/etc. directly against the
   resulting JSON, asserting a full round trip (including the flip-then-unflip identity on
   `positionZ`, `layerIndex` surviving in-range, and an out-of-range `layerIndex` clamping to 0 on
   import). This deliberately bypasses `BuildSanmapJsonText`/`ParseSanmapJsonText` entirely, since
   those are not wired to call these new functions.

## Explicit out-of-scope
- **Live document wiring** — the whole point of this ticket's structure; see above. Tracked as
  Step 5, pending a UI Expert consult on the confirm-dialog widget.
- **`blueprintPath` validation of any kind** — no `SanpackReader` call, no existence check, nothing.
  That's Step 5 entirely.
- **UI wiring** — `PropsTab_Manual_UI.h`'s SCOPE NOTE 1 (`ManualPropGroup` has no `_PARAMS` home)
  is given a home by `PropInstanceLayer` existing now, but the UI itself is not retyped onto it —
  same posture as every prior step's UI exclusion.
- **`UnitRule::armyIndex`-style integration questions** — none apply to this domain; not relevant.

## Acceptance test
The new standalone test binary/functions report all-pass: full round trip of `PropInstanceGroup`/
`DecalInstanceGroup`/`PropInstanceLayer`/`DecalInstanceLayer` through the new pure builders/readers,
`positionZ` flip-then-unflip identity, in-range `layerIndex` survives exactly, out-of-range
`layerIndex` clamps to `0` on import with a logged warning. Separately: the EXISTING
`MapImporter_IO_Test.exe`/`MapExporter_IO_Test.exe` round-trip suite (Steps 2/3's tests) must still
pass UNCHANGED — `document["props"]`/`document["decals"]` in a real exported document are still
empty arrays after this ticket, exactly as before it. Full `SanGenV2` build stays clean.
