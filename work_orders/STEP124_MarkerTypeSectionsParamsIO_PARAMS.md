# STEP124 — Marker Type-Sections + Instance Selection: PARAMS/IO (ARCH §19.13/§19.16/§19.17, Ticket A)

**Layer:** PARAMS, IO. **Domain:** `Params::MarkerRuleLayer::markerTypeName`,
`Params::MarkerInstanceLayer::markerTypeName`, `Params::MarkerTransform::instanceIdentifier`,
`Params::GlobalMarkerSettings`'s four `selectColor*` fields + `ResolveMarkerGroupSelectTintColor`.
**Sequence:** no dependency on other undone work-orders; unblocks Ticket B (the Type-section tab
restructure) and Ticket C (the instance list + selection highlight), both separately drafted per
`work_orders/DESIGN_MarkerTypeSectionsAndInstanceSelection_R1.md`'s own delivery-split
recommendation.

Ratifies `work_orders/DESIGN_MarkerTypeSectionsAndInstanceSelection_R1.md`'s Ticket A scope per
`ARCH_19_13_MarkerRuleLayerTypeName.md`, `ARCH_19_16_InstanceIdentifier.md`, and
`ARCH_19_17_SelectColorFields.md` — all three ratified "as designed," so every field/wire-key name
below is the design doc's own working name, not a corrected one (unlike STEP119, which corrected
several `MarkerLayerBundle` names ARCH ratification changed). This ticket also carries a small
**required file-size-ceiling remediation** on the IO side (§4 below), and a **scope correction** on
one line item the design doc's own delivery split names for Ticket A but this codebase's own
precedent places elsewhere (see immediately below).

## Scope correction: minting is deferred to Ticket C, not built here

The design doc's delivery-split section groups "`MarkerTransform::instanceIdentifier` + minting +
legacy-backfill" under Ticket A, and `ARCH_19_16_InstanceIdentifier.md` describes a
`NextMarkerInstanceIdentifier(const std::vector<Params::MarkerInstanceGroup>& markers)` minting
helper "same shape as `NextMarkerLayerId`." Confirmed by direct read: **`NextMarkerLayerId` itself
does NOT live in PARAMS or IO** — it lives in `src/ui/MarkerLayerId_UI.h` (`Ui::` namespace),
touched by zero IO/PARAMS ticket. Its own one-tier-up sibling, `NextMarkerLayerBundleId`
(identical `max(identifier) + 1` shape), lives inline in `src/ui/MarkersTab_Bundles_UI.h:65-69` —
**Ticket B's own file**, not STEP119 (this ticket's own direct predecessor, "Ticket A" for the
Bundle feature one round earlier). STEP119's "Files touched" list confirms zero `src/ui/*` files
were touched by that ticket; minting was never part of it despite `MarkerLayerBundle::identifier`
being a STEP119 field.

Every existing minting helper in this codebase for a freshly-added stable-identity field lives in
`src/ui/`, callable only from the "Add X" authoring action that actually needs a fresh id — never
in PARAMS/IO, which has no such action to trigger from. This is a direct application of
`ARCH_19_02_GenericitySplit.md`'s domain-touching-vs-pure-mechanics split: minting a fresh identifier
is a UI-authoring-time concern (STEP124 has no "Add Marker" call site to wire it into), not IO
mechanics. **Ruled here: `NextMarkerInstanceIdentifier` is OUT OF SCOPE for this ticket**, deferred
to Ticket C (which needs it for the `MarkersTab_ManualInstance_UI.h`/roster-editor "Add Marker"
action per Open Q7's own instance-list design) — this ticket delivers only the field, its wire key,
and the legacy-backfill counter (a genuinely IO-side concern: it runs during import, not
authoring). Flagged explicitly rather than silently dropped, since the design doc's own text
names it for Ticket A — **the human/ARCH should confirm this placement**, though it is grounded
directly in existing, shipped code, not a guess.

## Problem

Three additive PARAMS/IO gaps block Ticket B (Type-sections) and Ticket C (instance selection):

1. **No per-Layer type-scoping tag.** `MarkerLayerBundle::markerTypeName` (STEP119) lets a Bundle
   declare which Type-section it belongs to, but `MarkerRuleLayer`/`MarkerInstanceLayer` — the
   leaf layers Ticket B's per-type "Ungrouped Procedural/Manual" lists filter — have no equivalent
   field. Confirmed by direct read: neither `src/params/MarkerRule_PARAMS.h` nor
   `src/params/MarkerInstance_PARAMS.h` declares `markerTypeName` today.
2. **No stable per-instance identity for manual markers.** `MarkerTransform` has no field a
   selection state can address across frames — `layerIndex`/`symmetryGroupIdentifier` are both
   already spoken for (neither is a stable per-transform identity; the latter is 0 for the common
   "never dragged" case). Ticket C's selection/highlight mechanism (ARCH §19.16/§19.19) needs one.
3. **No select-tint storage.** `GlobalMarkerSettings` has `color*`/`iconName*`/`scale*` triplets but
   nothing for a selection highlight; Ticket C's tint-priority rewrite (ARCH §19.18) needs
   per-type select colors to compose against.

## Fix

### 1. `markerTypeName` on `MarkerRuleLayer` (`src/params/MarkerRule_PARAMS.h`)

Insert after the existing `int parentBundleIdentifier = -1;` (line 85), before `std::vector<MarkerRule> rules;` (line 86):
```cpp
    std::string markerTypeName;   // ARCH §19.13 — free-form, same string space as
                                   // MarkerLayerBundle::markerTypeName (STEP119), NOT MarkerCategory
                                   // (ARCH §19.21 — the two stay permanently independent). Additive.
```
Default `""` (a plain `std::string` default-constructs empty) — renders in Ticket B's
`"(Unassigned)"` bucket per `ARCH_19_14_TypeSectionUiDerived.md`'s ordering rule.

### 2. `markerTypeName` on `MarkerInstanceLayer` (`src/params/MarkerInstance_PARAMS.h`)

Insert after the existing `int parentBundleIdentifier = -1;` (line 50), before the struct's closing
`};` (line 51) — same field, same comment, mirrored on the sibling type:
```cpp
    std::string markerTypeName;   // ARCH §19.13 — free-form, same string space as
                                   // MarkerLayerBundle::markerTypeName (STEP119), NOT MarkerCategory
                                   // (ARCH §19.21). Additive.
```

### 3. `instanceIdentifier` on `MarkerTransform` (`src/params/MarkerInstance_PARAMS.h`)

Insert after the existing `std::string iconNameOverride;` field and its trailing comment (through
line 68), before the struct's closing `};` (line 69) — appended last, matching this struct's own
chronological-append convention (`layerIndex`→`symmetryGroupIdentifier`[STEP68]→
`iconNameOverride`[STEP114]→this):
```cpp
    // ARCH §19.16. Stable, GLOBALLY unique across every MarkerInstanceGroup's transforms (not
    // per-group) — never reused, -1 = unassigned. A THIRD bare int alongside layerIndex/
    // symmetryGroupIdentifier, spelled in full per §1.9 to stay unambiguous among the three. Exists
    // solely for stable UI-selection addressing (Ticket C) — carries no round-tripping/export-key
    // role of its own; MakeNamesUnique's existing name-based identity is untouched.
    int instanceIdentifier = -1;
```

### 4. IO homes for markerTypeName — mirrors STEP119's `parentBundleIdentifier` file list exactly

Same four IO files STEP119 touched for `parentBundleIdentifier`, since `markerTypeName` merges into
the identical two wire objects:

**Exporter — `src/io/MapExporter_MarkersStack_IO.cpp`**, `BuildMarkerRuleLayerJson`: insert after
the existing `json["ParentBundleIdentifier"] = layer.parentBundleIdentifier;` (line 62), before the
`Rules` array build:
```cpp
    json["MarkerTypeName"] = layer.markerTypeName;
```

**Exporter — `src/io/MapExporter_Markers_IO.cpp`**, `BuildMarkerGroupsJson`: insert after the
existing `layerJson["ParentBundleIdentifier"] = layer.parentBundleIdentifier;` (line 82), before
`markerGroups.push_back(layerJson);`:
```cpp
    layerJson["MarkerTypeName"] = layer.markerTypeName;
```

**Importer — `src/io/MapImporter_MarkersStack_IO.cpp`**, `ReadMarkerRuleLayerJson`: insert after
the existing `ReadJsonInteger(json, "ParentBundleIdentifier", layer.parentBundleIdentifier);`
(line 64), before `ReadRuleArray(json, "Rules", layer.rules, ReadMarkerRuleJson);`:
```cpp
    ReadJsonText(json, "MarkerTypeName", layer.markerTypeName);
```

**Importer — `ReadMarkerGroupsJson`, RELOCATING to a new file** (was
`src/io/MapImporter_Markers_IO.cpp:116-146`) — see the file-size remediation below; the edit itself
is the same one-line insert, after the existing
`ReadJsonInteger(layerJson, "ParentBundleIdentifier", layer.parentBundleIdentifier);`, before the
closing brace of the `if (layerJson.is_object())` block:
```cpp
            ReadJsonText(layerJson, "MarkerTypeName", layer.markerTypeName);
```

No import-time cross-check against the containing Bundle's or Group's own `markerTypeName`
(ARCH §19.13's own ruling, applying §19.12's already-established soft-validation posture) — no
range/format validation needed for a free-form string.

### 5. Wire home for `instanceIdentifier` — `src/io/MapExporter_Markers_IO.cpp` / a required file split on the import side

**Exporter — `BuildMarkerTransformJson`** (`src/io/MapExporter_Markers_IO.cpp:18-40`): insert after
the existing `json["iconNameOverride"] = markerTransform.iconNameOverride;` (line 38), before
`return json;`, same unconditional-write posture as `symmetryGroupIdentifier`/`iconNameOverride`:
```cpp
    json["InstanceIdentifier"] = markerTransform.instanceIdentifier;
```

**Importer — legacy-backfill, threaded through the existing nested walk in
`src/io/MapImporter_Markers_IO.cpp`.** `ARCH_19_16_InstanceIdentifier.md` rules this must mirror
`layerId`'s own precedent EXACTLY: `ReadMarkerGroupsJson` assigns
`layer.layerId = static_cast<int>(outRecipe.markerLayers.size());` as a POSITIONAL DEFAULT before
reading the JSON, then `ReadJsonInteger(layerJson, "Id", layer.layerId);` overwrites it if the file
actually carries the key. Applied to `instanceIdentifier`, with a counter that must survive across
the WHOLE nested group→transform walk (not reset per group), instead of the vector-size trick
(there is no single flat vector to size against — `MarkerTransform`s live inside per-group
sub-vectors):

`ReadMarkerTransformJson` gains a threaded counter parameter, and the backfill-then-overwrite
happens right after the existing `iconNameOverride` read (before line 76's closing brace):
```cpp
void ReadMarkerTransformJson(const nlohmann::json& json, Params::MarkerTransform& markerTransform,
                             int mapSize, int& inOutNextInstanceIdentifier) {
    // ... existing body unchanged through the iconNameOverride read ...
    // ARCH §19.16 — legacy-backfill mirrors layerId's own precedent (now MapImporter_
    // MarkerGroups_IO.cpp): the counter's CURRENT value is always assigned first (so an absent key
    // backfills to this transform's position in the encounter order), then overwritten if the file
    // actually carries the key. The counter always advances, whether or not this transform's
    // backfilled default was kept — same "assign eagerly, allow overwrite" shape as layerId, not a
    // conditional increment.
    markerTransform.instanceIdentifier = inOutNextInstanceIdentifier++;
    ReadJsonInteger(json, "InstanceIdentifier", markerTransform.instanceIdentifier);
}
```
`ReadMarkerInstanceGroupJson` and `ReadMarkersJson` thread the same `int&` down through their
existing lambda captures (no other body change):
```cpp
void ReadMarkerInstanceGroupJson(const nlohmann::json& json, Params::MarkerInstanceGroup& group,
                                 int mapSize, std::size_t markerLayerCount, MapImportResult& result,
                                 int& inOutNextInstanceIdentifier) {
    ReadJsonBoolean(json, "resource", group.bResource);
    ReadNameKeyedObject(json, "transforms", group.transforms,
                        [mapSize, markerLayerCount, &result, &inOutNextInstanceIdentifier]
                        (const nlohmann::json& transformJson, Params::MarkerTransform& markerTransform) {
                            ReadMarkerTransformJson(transformJson, markerTransform, mapSize,
                                                    inOutNextInstanceIdentifier);
                            ClampMarkerLayerIndex(markerTransform, markerLayerCount, result);
                        });
}

void ReadMarkersJson(const nlohmann::json& document, Params::MapRecipe& outRecipe, MapImportResult& result) {
    if (!document.contains("markers") || !document["markers"].is_object()) return;
    const int mapSize = outRecipe.geometry.mapSize;
    const std::size_t markerLayerCount = outRecipe.markerLayers.size();
    // ARCH §19.16 — threaded across the ENTIRE nested walk below, never reset per group.
    int nextInstanceIdentifier = 0;
    ReadNameKeyedObject(document, "markers", outRecipe.markers,
                        [mapSize, markerLayerCount, &result, &nextInstanceIdentifier]
                        (const nlohmann::json& groupJson, Params::MarkerInstanceGroup& group) {
                            ReadMarkerInstanceGroupJson(groupJson, group, mapSize, markerLayerCount,
                                                        result, nextInstanceIdentifier);
                        });
}
```

**Load-bearing subtlety, confirmed by direct read of the actual import path — flagged so the
Coder's tests don't assume file-write order:** `document`'s type all the way down this call chain is
plain `nlohmann::json`, NOT `nlohmann::ordered_json` (confirmed:
`src/io/MapImporter_ParseDocument_IO.cpp:135`, `document = nlohmann::json::parse(documentText);`,
plain `nlohmann::json`). Plain `nlohmann::json`'s default object type is a `std::map`, which
iterates keys LEXICOGRAPHICALLY, not in the order they were written. `markers` and
`markers[type].transforms` are both JSON OBJECTS (name-keyed dictionaries), so **"encounter order"
for this counter means alphabetical-by-group-name, then alphabetical-by-transform-name within each
group — NOT the original export's write order.** (`MarkerGroups`/`MarkersStack`/
`MarkerLayerBundles`, by contrast, are JSON ARRAYS — array element order is preserved regardless of
this policy, which is why `layerId`'s own backfill-by-vector-size never had this subtlety.) This is
still faithful to `ARCH_19_16_InstanceIdentifier.md`'s literal wording ("in encounter order" — the
walk's own iteration order, whatever that is) — flagged here only so the Verify section's fixtures
are constructed correctly and so the Coder does not silently assume insertion order.

### 6. `GlobalMarkerSettings` select-color fields + resolver (`src/params/GlobalMarkerSettings_PARAMS.h`)

Insert after the existing `float scaleSpawn = 0.17f;` (line 23), before the struct's closing `};`
(line 24) — verbatim per `ARCH_19_17_SelectColorFields.md`:
```cpp
    // ARCH §19.17 — selection-highlight tint. selectColorAlloy/Plasma/Spawn strictly mirror
    // colorAlloy/Plasma/Spawn's shape/placement; selectColorDefault is the one signed-off 4th-field
    // deviation from that mirror (see ResolveMarkerGroupSelectTintColor below for why).
    float selectColorAlloy[4]   = {1.0f, 1.0f, 0.0f, 1.0f};
    float selectColorPlasma[4]  = {1.0f, 1.0f, 0.0f, 1.0f};
    float selectColorSpawn[4]   = {1.0f, 1.0f, 0.0f, 1.0f};
    float selectColorDefault[4] = {1.0f, 1.0f, 0.0f, 1.0f};
```
New resolver, inserted after `ResolveMarkerGroupTypeScale` (after line 52), before the closing
`} // namespace Params`:
```cpp
// ARCH §19.17: the select-tint counterpart to ResolveMarkerGroupTypeTintColor, same name-matching
// vocabulary (Spawn/Spawns, Alloy/Alloys, Plasma/Plasmas) — but an unmatched group name resolves to
// settings.selectColorDefault, NOT hardcoded white: a select tint that fell back to white would
// make "selected" indistinguishable from "unselected" for any Generic/Expansion/freeform group,
// since that same unmatched name's normal (unselected) fill already resolves to white via
// ResolveMarkerGroupTypeTintColor's own fallback absent a per-layer color override — a real
// correctness gap, not a cosmetic one (ARCH §19.17's signed-off deviation from the 3-field mirror).
inline void ResolveMarkerGroupSelectTintColor(const std::string& groupName, const GlobalMarkerSettings& settings,
                                              float& outRed, float& outGreen, float& outBlue) {
    const float* color = settings.selectColorDefault;
    if (groupName == kSpawnMarkerGroupName || groupName == "Spawns") color = settings.selectColorSpawn;
    else if (groupName == "Alloy" || groupName == "Alloys")          color = settings.selectColorAlloy;
    else if (groupName == "Plasma" || groupName == "Plasmas")        color = settings.selectColorPlasma;
    outRed = color[0]; outGreen = color[1]; outBlue = color[2];
}
```

### 7. Wire keys for the select-color fields — derived by direct analogy, flagged as not letter-for-letter ratified

`ARCH_19_17_SelectColorFields.md` pins the C++ field names and the resolver's signature but does not
spell out the wire-key text for `selectColorAlloy/Plasma/Spawn/Default` (unlike `markerTypeName`'s
`"MarkerTypeName"` and `instanceIdentifier`'s `"InstanceIdentifier"`, both explicitly ratified).
This ticket derives the wire spelling from the SAME established prefix convention
`colorAlloy/Plasma/Spawn`'s own wire keys already use — confirmed at
`SANMAP_FORMAT_SPEC.md:512-514` and `GlobalMarkerSettings_PARAMS.h`'s own header comment: wire keys
keep the `Marker`-prefixed spelling even though the C++ field drops it
(`colorAlloy` ↔ `"MarkerColorAlloy"`). Applied identically:
`"MarkerSelectColorAlloy"` / `"MarkerSelectColorPlasma"` / `"MarkerSelectColorSpawn"` /
`"MarkerSelectColorDefault"`. **Flagged for explicit ARCH/Format-Expert sign-off** (the same split
`ARCH_19_04_WireShape.md` names for any wire-key micro-spelling ARCH's own ruling didn't enumerate)
— this is a well-grounded derivation, not a guess, but it is this ticket's own call, not a repeated
ARCH citation.

**Exporter — `BuildGlobalMarkerSettingsJson`** (`src/io/MapExporter_MarkersStack_IO.cpp:84-100`):
insert after the existing `json["MarkerScaleSpawn"] = settings.scaleSpawn;` (line 98), before
`return json;`, same `{r,g,b,a}` object shape `MarkerColorAlloy/Plasma/Spawn` already use:
```cpp
    json["MarkerSelectColorAlloy"]   = { { "r", settings.selectColorAlloy[0] },
        { "g", settings.selectColorAlloy[1] }, { "b", settings.selectColorAlloy[2] },
        { "a", settings.selectColorAlloy[3] } };
    json["MarkerSelectColorPlasma"]  = { { "r", settings.selectColorPlasma[0] },
        { "g", settings.selectColorPlasma[1] }, { "b", settings.selectColorPlasma[2] },
        { "a", settings.selectColorPlasma[3] } };
    json["MarkerSelectColorSpawn"]   = { { "r", settings.selectColorSpawn[0] },
        { "g", settings.selectColorSpawn[1] }, { "b", settings.selectColorSpawn[2] },
        { "a", settings.selectColorSpawn[3] } };
    json["MarkerSelectColorDefault"] = { { "r", settings.selectColorDefault[0] },
        { "g", settings.selectColorDefault[1] }, { "b", settings.selectColorDefault[2] },
        { "a", settings.selectColorDefault[3] } };
```

**Importer — `ReadGlobalMarkerSettingsJson`** (`src/io/MapImporter_MarkersStack_IO.cpp:86-100`):
insert after the existing `ReadJsonFloat(json, "MarkerScaleSpawn", settings.scaleSpawn);` (line 99),
before the closing brace, reusing the file's own existing `ReadJsonColorRgba` helper:
```cpp
    ReadJsonColorRgba(json, "MarkerSelectColorAlloy", settings.selectColorAlloy);
    ReadJsonColorRgba(json, "MarkerSelectColorPlasma", settings.selectColorPlasma);
    ReadJsonColorRgba(json, "MarkerSelectColorSpawn", settings.selectColorSpawn);
    ReadJsonColorRgba(json, "MarkerSelectColorDefault", settings.selectColorDefault);
```
No range validation needed (any RGBA is legal, same posture as the existing three colors); no new
top-level key (`GlobalMarkerSettings` is already allowlisted in
`src/io/Sanmap_KnownTopLevelKeys_IO.cpp:32` — this ticket adds no new top-level `.sanmap` key at
all, only merged fields on already-known objects/arrays, so `Sanmap_KnownTopLevelKeys_IO.cpp` is
**not touched** by this ticket, unlike STEP119).

## File-size ceiling — required remediation, not optional (Constitution §7)

Confirmed by direct `wc -l` before any edit: **`src/io/MapImporter_Markers_IO.cpp` is currently 162
lines** — already over `ARCH_01_05_FileSizeCeilings.md`'s 150-line hard ceiling, with no documented
exception on record (its own sibling file's header comment,
`MapImporter_MarkerLayerReconcile_IO.cpp:1-4`, already flagged this same fact as "already 160
lines" back at STEP115 and split `ReconcileMarkerLayers` out for exactly that reason — this file has
been over budget, un-remediated, since before STEP119 added its own `ParentBundleIdentifier` line
without flagging it).

This ticket's own edits — one `ReadJsonText` line for `markerTypeName`, plus the
`instanceIdentifier` counter threaded through THREE function signatures (§5 above) — would land the
file at roughly 172–176 lines if made in place, a silent further ratchet on an already-broken
ceiling. **Required split, along the file's own existing fault line** (its two independent
top-level entry points, `ReadMarkerGroupsJson` for the `MarkerGroups` array vs. `ReadMarkersJson`
for the `markers` name-keyed dictionary — these share no helper functions today; confirmed by
direct read, `ReadNameKeyedObject`/`ReadMarkerTransformJson`/`ClampMarkerLayerIndex`/
`ReadMarkerInstanceGroupJson` are used only by the `ReadMarkersJson` side):

**New file — `src/io/MapImporter_MarkerGroups_IO.cpp`.** Moves `ReadMarkerGroupsJson` out verbatim
(plus this ticket's own one-line `MarkerTypeName` addition, §4 above), mirroring
`MapImporter_MarkerLayerReconcile_IO.cpp`'s own precedent for splitting a function out of this exact
file for this exact reason (STEP115):
```cpp
// MapImporter_MarkerGroups_IO.cpp — the top-level `MarkerGroups` array -> `recipe.markerLayers`
// (STEP60_MarkerInstanceLayer_PARAMS). Layer: IO. Split out of MapImporter_Markers_IO.cpp (STEP124)
// once that file's own line count — already over ARCH §1.5's 150-line hard ceiling before this
// ticket (162 lines, unremediated since STEP115 first flagged it) — would have crossed further
// with this ticket's own additions (MarkerTypeName here; the InstanceIdentifier legacy-backfill
// counter threaded through the `markers`-dictionary side that stays behind). Mirrors
// MapImporter_MarkerLayerReconcile_IO.cpp's own split-out-of-this-exact-file precedent (STEP115).
// `ReadMarkerGroupsJson` MUST still run before `ReadMarkersJson` (MapImporter_Recipe_IO.h's own
// header comment, unchanged) — this split changes which TRANSLATION UNIT defines the function, not
// the call order in MapImporter_ParseDocument_IO.cpp.
#include "JsonPrimitives_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {

// `MarkerGroups` — a plain array walk, same shape as `ReadPropGroupsJson`'s `{r,g,b,a}` read
// (STEP60_MarkerInstanceLayer_PARAMS). `layerId` legacy-backfills by array index — a file with no
// `"Id"` key on an entry lands it on the vector position it was read at.
void ReadMarkerGroupsJson(const nlohmann::json& document, Params::MapRecipe& outRecipe) {
    if (!document.contains("MarkerGroups") || !document["MarkerGroups"].is_array()) return;
    outRecipe.markerLayers.clear();
    for (const nlohmann::json& layerJson : document["MarkerGroups"]) {
        Params::MarkerInstanceLayer layer;
        layer.layerId = static_cast<int>(outRecipe.markerLayers.size());   // legacy-backfill default
        if (layerJson.is_object()) {
            ReadJsonText(layerJson, "Name", layer.name);
            if (layerJson.contains("Color") && layerJson["Color"].is_object()) {
                const nlohmann::json& color = layerJson["Color"];
                ReadJsonFloat(color, "r", layer.color[0]);
                ReadJsonFloat(color, "g", layer.color[1]);
                ReadJsonFloat(color, "b", layer.color[2]);
                ReadJsonFloat(color, "a", layer.color[3]);
            }
            ReadJsonFloat(layerJson, "IconScale", layer.iconScale);
            ReadJsonInteger(layerJson, "Id", layer.layerId);
            ReadJsonBoolean(layerJson, "SymmetryUseGlobal", layer.symmetry.bSymmetryUseGlobal);
            ReadJsonInteger(layerJson, "SymmetryMask", layer.symmetry.symmetryMask);
            ReadJsonInteger(layerJson, "RadialSymmetryRepeatCount", layer.symmetry.radialSymmetryRepeatCount);
            ReadJsonBoolean(layerJson, "Locked", layer.bLocked);
            ReadJsonBoolean(layerJson, "GridSnapEnabled", layer.bGridSnapEnabled);
            ReadJsonFloat(layerJson, "GridSnapSizeWorldUnits", layer.gridSnapSizeWorldUnits);
            ReadJsonBoolean(layerJson, "ColorOverrideEnabled", layer.bColorOverrideEnabled);
            ReadJsonInteger(layerJson, "ParentBundleIdentifier", layer.parentBundleIdentifier);
            ReadJsonText(layerJson, "MarkerTypeName", layer.markerTypeName);   // NEW, STEP124/ARCH §19.13
        }
        outRecipe.markerLayers.push_back(layer);
    }
}

} // namespace Io
} // namespace SanmapGen
```
No declaration change needed in `src/io/MapImporter_Recipe_IO.h` — `ReadMarkerGroupsJson` is
already declared there (line 62); only update its attributing comment (lines 57-61) to note the
function now DEFINES in `MapImporter_MarkerGroups_IO.cpp`, not `MapImporter_Markers_IO.cpp`. No
`CMakeLists.txt` edit needed — new non-test `.cpp` files are auto-discovered by
`SANGEN_V2_SOURCES`'s `GLOB_RECURSE CONFIGURE_DEPENDS` (`CMakeLists.txt:174`).

**Remaining `src/io/MapImporter_Markers_IO.cpp`** keeps `ReadNameKeyedObject`,
`ReadMarkerTransformJson` (+ the `instanceIdentifier` counter parameter), `ClampMarkerLayerIndex`,
`ReadMarkerInstanceGroupJson` (+ threaded parameter), `ReadMarkersJson` (+ counter declaration and
threading) — projected at roughly 138–142 lines after the split (removing `ReadMarkerGroupsJson`'s
~31 lines nets out the ~10 lines this ticket's own `instanceIdentifier` threading adds), comfortably
under the 150-line hard ceiling, though still over the 100-line soft target — an existing condition
this ticket does not newly create and is not asked to further remediate. Trim its own header comment
(lines 8-11) to stop describing `MarkerGroups`/`ReadMarkerGroupsJson` as this file's own content —
that description moves to the new file's header.

**Coder: confirm the actual post-split line counts of both files once written** — if either still
exceeds its ceiling, that is this ticket's own second exception to flag explicitly (Constitution
§7), not a signal to silently absorb.

## Out of scope

- **`NextMarkerInstanceIdentifier`** — see the Scope correction section above; deferred to Ticket C.
- **All UI** — no `src/ui/*` file is touched by this ticket (the scope correction above is what
  keeps this true despite the design doc's own Ticket A framing). Ticket B owns the Type-section tab
  restructure; Ticket C owns the instance list, selection state, `MapCanvas` wiring, and the
  tint-priority rewrite (ARCH §19.18/§19.19/§19.20).
- **The selection-highlight computation itself** (`Pipeline::BuildWorldSymmetryOrbit` +
  sibling-orbit matching, ARCH §19.19) — Ticket C, PIPELINE-query-from-UI, not this ticket.
- **The tint-priority rewrite** (`DrawManualMarkerRoster`'s branch chain, ARCH §19.18) — Ticket C;
  this ticket only stores the four `selectColor*` fields and the resolver that will feed it.
- **Import-time cross-validation of `markerTypeName` against the owning Bundle/Group** — ARCH
  §19.12/§19.13 both explicitly rule this stays soft/UI-only; no validation added here.
- **`Sanmap_KnownTopLevelKeys_IO.cpp`** — not touched; this ticket adds no new top-level `.sanmap`
  key (unlike STEP119's `MarkerLayerBundles` array).
- **`MapRecipe_PARAMS.h`** — not touched; every field this ticket adds lives inside an
  already-embedded struct (`MarkerRuleLayer`/`MarkerInstanceLayer`/`MarkerTransform`/
  `GlobalMarkerSettings`), none of which needs a new `MapRecipe`-level vector/include.
- **`MarkerLayerBundle_PARAMS.h`/`MarkerLayerBundleQuery_PARAMS.h`** — `MarkerLayerBundle` already
  carries its own `markerTypeName` (STEP119); not touched again here.
- **`MapImporter_MarkerLayerReconcile_IO.cpp`'s own header comment** — still says the sibling file
  was "already 160 lines" as of STEP115; now stale relative to this ticket's own split (the file
  will be ~140 lines, in a different file, after this ticket). Left untouched — a comment about a
  past decision's rationale, not a functional claim; flagged here as an optional follow-up, not
  required by this ticket.

## Files touched

- `src/params/MarkerRule_PARAMS.h` — `MarkerRuleLayer` gains `markerTypeName`
- `src/params/MarkerInstance_PARAMS.h` — `MarkerInstanceLayer` gains `markerTypeName`;
  `MarkerTransform` gains `instanceIdentifier`
- `src/params/GlobalMarkerSettings_PARAMS.h` — four new `selectColor*` fields; new
  `ResolveMarkerGroupSelectTintColor`
- `src/io/MapExporter_MarkersStack_IO.cpp` — `BuildMarkerRuleLayerJson` writes `"MarkerTypeName"`;
  `BuildGlobalMarkerSettingsJson` writes the four `"MarkerSelectColor*"` keys
- `src/io/MapExporter_Markers_IO.cpp` — `BuildMarkerGroupsJson` writes `"MarkerTypeName"`;
  `BuildMarkerTransformJson` writes `"InstanceIdentifier"`
- `src/io/MapImporter_MarkersStack_IO.cpp` — `ReadMarkerRuleLayerJson` reads `"MarkerTypeName"`;
  `ReadGlobalMarkerSettingsJson` reads the four `"MarkerSelectColor*"` keys
- `src/io/MapImporter_MarkerGroups_IO.cpp` — **new file**: `ReadMarkerGroupsJson`, relocated out of
  `MapImporter_Markers_IO.cpp` (file-size remediation), plus its own `"MarkerTypeName"` read
- `src/io/MapImporter_Markers_IO.cpp` — `ReadMarkerGroupsJson` removed (relocated);
  `ReadMarkerTransformJson`/`ReadMarkerInstanceGroupJson`/`ReadMarkersJson` thread the
  `instanceIdentifier` legacy-backfill counter; header comment trimmed
- `src/io/MapImporter_Recipe_IO.h` — comment attributing `ReadMarkerGroupsJson` updated to the new
  file (declaration itself unchanged)
- `src/params/GlobalMarkerSettings_PARAMS_Test.cpp` — `MakeNonDefaultSettings` gains non-default
  `selectColor*` values; new `ResolveMarkerGroupSelectTintColor` checks in `main()`
- `src/io/MapImporter_IO_Test.cpp` — extends `CheckMarkerRuleLayerTwoLevelRoundTrip`,
  `FillFixtureMarkersAndChains`/`CheckMarkersAndChains`,
  `CheckMarkerRuleNewFieldsAndGlobalMarkerSettings` (+ its own fixture, lines 1157-1170); new
  `CheckMergedMarkerTypeNameLegacyDefault`,
  `CheckMarkerInstanceIdentifierLegacyBackfillAcrossGroups`

## Verify

Acceptance bar: `markerTypeName` round-trips on both `MarkerRuleLayer`/`MarkerInstanceLayer`,
old-file and new-file; `instanceIdentifier` round-trips when present and legacy-backfills to a
globally-unique, non-per-group-reset sequence when absent, exercised across a real multi-group
fixture; the four `selectColor*` fields round-trip; `ResolveMarkerGroupSelectTintColor` matches
Spawn/Alloy/Plasma correctly and falls back to `selectColorDefault` (never white) for an unmatched
name; both split IO files compile standalone and stay under their line ceilings; every existing
suite this ticket does not itself touch stays green.

- **Extend `CheckMarkerRuleLayerTwoLevelRoundTrip`** (`MapImporter_IO_Test.cpp:363-438`): set
  `layerOne.markerTypeName = "Alloy";` (non-default); leave `layerTwo.markerTypeName` at its `""`
  default. Add `&& loadedLayer.markerTypeName == originalLayer.markerTypeName` to the existing
  per-layer field-equality `Check(...)` (around line 436) so both the non-default and the
  still-default case are exercised in the same loop.
- **Extend `FillFixtureMarkersAndChains`/`CheckMarkersAndChains`** (`MapImporter_IO_Test.cpp:1221-
  1271` / `658-749`): set `markerLayer.markerTypeName = "Spawn";` (non-default) on the fixture's
  `MarkerInstanceLayer`; assert `loadedLayer.markerTypeName == originalLayer.markerTypeName` beside
  the existing `parentBundleIdentifier` check (line ~679-680). Set
  `markerTransform.instanceIdentifier = 999;` (non-default, explicit — present in the exported JSON,
  so this exercises the OVERWRITE half of the backfill-then-overwrite logic through the REAL
  `BuildSanmapJsonText`/`ParseSanmapJsonText` path, complementing the direct-call backfill test
  below); assert `loadedMarker.instanceIdentifier == originalMarker.instanceIdentifier` beside the
  existing `iconNameOverride` check (line ~715-716). This fixture's own "no warning" assertion
  (`RunRoundTripTests`) must stay satisfied — `999` requires no clamp/validation, so it does not.
- **Extend `CheckMarkerRuleNewFieldsAndGlobalMarkerSettings`** (`MapImporter_IO_Test.cpp:323-355`)
  and its fixture (`MapImporter_IO_Test.cpp:1157-1170`): set all four `selectColor*` fields to
  distinct non-default values, e.g.
  `globalMarkerSettings.selectColorAlloy[0] = 0.91f; ... selectColorDefault[3] = 0.44f;` (mirroring
  the fixture's existing per-component distinct-value style for `colorAlloy/Plasma/Spawn`); add a
  new `Check(...)` block asserting all 16 components (`selectColorAlloy/Plasma/Spawn/Default`, 4
  components each) survive, mirroring the existing `colorAlloy/Plasma/Spawn` assertion's shape
  (lines 344-350).
- **New unit test — `CheckMergedMarkerTypeNameLegacyDefault`**, mirroring
  `CheckMergedParentBundleIdentifierLegacyDefault`'s exact two-part shape
  (`MapImporter_IO_Test.cpp:2149-2169`): a hand-built `MarkerGroups` entry with only `"Name"`
  present (no `"MarkerTypeName"` key), read via `Io::ReadMarkerGroupsJson` directly, asserts
  `markerLayers[0].markerTypeName.empty()`; a hand-built `MarkersStack` entry with only `"Name"`
  present, read via `Io::ReadMarkersStackJson` directly, asserts
  `markerRuleLayers[0].markerTypeName.empty()`. Register the call in `main()` beside
  `CheckMergedParentBundleIdentifierLegacyDefault()`.
- **New unit test — `CheckMarkerInstanceIdentifierLegacyBackfillAcrossGroups`** (the task's own
  explicit ask: globally-unique, non-colliding, not-reset-per-group backfill, plus the
  overwrite-still-advances-the-counter interleaving), calling `Io::ReadMarkersJson` directly:
  ```cpp
  void CheckMarkerInstanceIdentifierLegacyBackfillAcrossGroups() {
      // Group "Alloys" (alphabetically first — see §5's load-bearing note on nlohmann::json's
      // sorted, non-insertion-order object iteration) has two transforms: "AAA" (no
      // InstanceIdentifier key) and "BBB" (an explicit, out-of-band value, 500).
      nlohmann::json transformAAA;                         // no "InstanceIdentifier" key
      nlohmann::json transformBBB; transformBBB["InstanceIdentifier"] = 500;
      nlohmann::json groupAlloys;
      groupAlloys["resource"] = true;
      groupAlloys["transforms"] = nlohmann::json::object({ { "AAA", transformAAA }, { "BBB", transformBBB } });

      // Group "Spawn" (alphabetically second) has one transform, also no InstanceIdentifier key —
      // proves the counter is NOT reset per group (ARCH §19.16's core requirement).
      nlohmann::json transformCCC;                         // no "InstanceIdentifier" key
      nlohmann::json groupSpawn;
      groupSpawn["resource"] = false;
      groupSpawn["transforms"] = nlohmann::json::object({ { "CCC", transformCCC } });

      nlohmann::json document;
      document["markers"] = nlohmann::json::object({ { "Alloys", groupAlloys }, { "Spawn", groupSpawn } });

      Params::MapRecipe recipe;
      Io::MapImportResult result;
      Io::ReadMarkersJson(document, recipe, result);

      Check(recipe.markers.size() == 2, "both marker groups survive");
      if (recipe.markers.size() != 2) return;
      Check(recipe.markers[0].name == "Alloys" && recipe.markers[1].name == "Spawn",
            "groups are visited in nlohmann::json's own sorted-key order (Alloys before Spawn)");
      Check(recipe.markers[0].transforms.size() == 2 && recipe.markers[1].transforms.size() == 1,
            "both transforms in Alloys and the one transform in Spawn survive");
      if (recipe.markers[0].transforms.size() != 2 || recipe.markers[1].transforms.empty()) return;

      // Counter starts at 0: AAA (no key) backfills to 0; BBB (has key) still CONSUMES counter
      // slot 1 before being overwritten to 500 (the counter always advances); CCC (no key, in the
      // SECOND group) backfills to 2 — proving the counter is threaded across groups, not reset.
      Check(recipe.markers[0].transforms[0].instanceIdentifier == 0,
            "AAA (no key) backfills to the counter's value at its own position (0)");
      Check(recipe.markers[0].transforms[1].instanceIdentifier == 500,
            "BBB's explicit InstanceIdentifier (500) overwrites the counter's positional default");
      Check(recipe.markers[1].transforms[0].instanceIdentifier == 2,
            "CCC (no key, in the SECOND group) backfills to 2, not 0 — the counter is threaded "
            "across the whole nested walk, never reset per group (ARCH §19.16)");
  }
  ```
  Register the call in `main()` beside the other Marker-domain legacy/backfill checks (near
  `CheckMarkerGroupsLegacyBackfill`/`CheckMergedParentBundleIdentifierLegacyDefault`).
- **Extend `GlobalMarkerSettings_PARAMS_Test.cpp`**: extend `MakeNonDefaultSettings()` with distinct
  non-default `selectColor*` values (mirroring its existing `colorSpawn/colorAlloy/colorPlasma`
  style); add to `main()`, mirroring the existing `ResolveMarkerGroupTypeTintColor` block's exact
  shape:
  ```cpp
  ResolveMarkerGroupSelectTintColor(kSpawnMarkerGroupName, settings, red, green, blue);
  Check(red == settings.selectColorSpawn[0] && green == settings.selectColorSpawn[1]
        && blue == settings.selectColorSpawn[2], "\"Spawn\" resolves selectColorSpawn");
  ResolveMarkerGroupSelectTintColor("Alloys", settings, red, green, blue);
  Check(red == settings.selectColorAlloy[0] && green == settings.selectColorAlloy[1]
        && blue == settings.selectColorAlloy[2], "\"Alloys\" (plural) resolves selectColorAlloy");
  ResolveMarkerGroupSelectTintColor("Plasma", settings, red, green, blue);
  Check(red == settings.selectColorPlasma[0] && green == settings.selectColorPlasma[1]
        && blue == settings.selectColorPlasma[2], "\"Plasma\" resolves selectColorPlasma");
  // The deviation this ticket exists to prove: unmatched names resolve selectColorDefault, NOT
  // white — unlike ResolveMarkerGroupTypeTintColor's own fallback, exercised just above in this
  // same file for the identical group names.
  ResolveMarkerGroupSelectTintColor("Generic", settings, red, green, blue);
  Check(red == settings.selectColorDefault[0] && green == settings.selectColorDefault[1]
        && blue == settings.selectColorDefault[2],
        "\"Generic\" resolves selectColorDefault, NOT white (ARCH §19.17's signed-off deviation)");
  ResolveMarkerGroupSelectTintColor("SomeFreeformGroupName", settings, red, green, blue);
  Check(red == settings.selectColorDefault[0] && green == settings.selectColorDefault[1]
        && blue == settings.selectColorDefault[2],
        "an arbitrary freeform name also resolves selectColorDefault");
  ```
- **Compile-standalone check for the new file**: `src/io/MapImporter_MarkerGroups_IO.cpp` must build
  as its own translation unit (it is auto-discovered by `SANGEN_V2_SOURCES`'s glob, so a normal full
  build already exercises this — no separate test binary needed, same posture as every other
  non-test `.cpp` under `src/io/`).
- **Existing suites stay green with no behavior change to any assertion this ticket does not itself
  add**: `CheckMarkerGroupsLegacyBackfill`, `CheckMarkerGroupsLegacyLockAndSnapDefaults`,
  `CheckMarkerLayerIndexClampsOnImport`, `CheckMarkerIconNameOverrideLegacyDefault`,
  `CheckKnownTopLevelSanmapKeysCoverage` (untouched allowlist, untouched key set),
  `MapExporter_IO_Test`, `MapImporter_PropsDecals_IO_Test`, `MapImporter_Scenarios_IO_Test`, and
  every other IO test binary that touches `MapRecipe`/`BuildSanmapJsonText`/`ParseSanmapJsonText` —
  every field this ticket adds is additive with a struct default, so no existing fixture's
  assertions change outcome.
