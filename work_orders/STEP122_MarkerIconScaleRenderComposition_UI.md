# STEP122 — Compose Global × per-layer Icon Scale into actual rendered marker size

**Layer:** UI (render pipeline — screen-space icon cull/emit + roster dot-draw). **Domain:**
`ResolveLodModeAndIcon` (`MapCanvas_IconLayer_CullEmit_UI.cpp`), `ResolveMarkersManual`
(`MapCanvas_IconLayer_CullManual_UI.cpp`), `ResolveProceduralSubLayer`
(`MapCanvas_IconLayer_CullProcedural_UI.cpp`), `DrawManualMarkerRoster`/`ManualMarkerTint`
(`MapCanvas_MarkerDrag_UI.cpp`), `GlobalMarkerSettings_PARAMS.h`. **Sequence:** no code
dependency on STEP121 (a separate, still-drafting companion ticket) — see the explicit
landing-order note below. Does not depend on any other undone work-order.

## Problem
Two PARAMS fields exist, fully round-tripped through IO, and are completely dead — never read
by any renderer:

- `Params::GlobalMarkerSettings::scaleAlloy/scalePlasma/scaleSpawn`
  (`src/params/GlobalMarkerSettings_PARAMS.h:21-23`, each defaulting to `0.17f`). Confirmed
  round-tripped: `src/io/MapExporter_MarkersStack_IO.cpp:96-98` writes
  `"MarkerScaleAlloy"`/`"MarkerScalePlasma"`/`"MarkerScaleSpawn"`,
  `src/io/MapImporter_MarkersStack_IO.cpp:97-99` reads them back, covered by
  `src/io/MapImporter_IO_Test.cpp:351-353,1168-1170` and
  `src/io/Sanmap_MigrationRunner_IO_Test.cpp:490`.
- `Params::MarkerInstanceLayer::iconScale` (`src/params/MarkerInstance_PARAMS.h:26`, default
  `1.0f`).

The one true "final icon size" computation point is `ResolveLodModeAndIcon`
(`src/ui/MapCanvas_IconLayer_CullEmit_UI.cpp:27-45`):
```cpp
const float thumbnailScreenSize = worldUnitsPerCell > 0.0f
    ? (baseFootprint * instanceScale) / worldUnitsPerCell
          * input.composite->PixelsPerPreviewCell() * input.view->ZoomScale()
    : 0.0f;
```
`instanceScale` (the function's 5th parameter) is the ONLY scale input today. Tracing its two
call sites:
- Manual markers: `ConsiderManualInstance` (`MapCanvas_IconLayer_CullManual_UI.cpp:26-42`) is
  called from `ResolveMarkersManual` at line 164 with `transform.transform.scaleX` — the
  individual `MarkerTransform`'s own placement scale, unrelated to either Global or per-layer
  scale.
- Procedural markers: `ResolveProceduralSubLayer` (`MapCanvas_IconLayer_CullProcedural_UI.cpp:20-65`)
  reads `const float scale = instances.scaleX[instanceIndex];` at line 42 — the baked
  placement's own scale.

Neither `scaleAlloy/Plasma/Spawn` nor `MarkerInstanceLayer::iconScale` feeds into either path.

Separately, the dot-renderer `DrawManualMarkerRoster` (`MapCanvas_MarkerDrag_UI.cpp:86-127`)
draws every manual marker at a HARDCODED radius, `kManualMarkerDotRadiusScreenPixels = 6.0f`
(line 15) — used at both `drawList.AddCircleFilled(screenCenter, kManualMarkerDotRadiusScreenPixels, tint)`
(line 116) and the ghost-point `drawList.AddCircle(screenCenter, kManualMarkerDotRadiusScreenPixels, ghostTint, 0, 2.0f)`
(line 122) — completely independent of any scale field, individual `MarkerTransform` scale
included.

**Companion ticket, STEP121 (drafted in parallel this session)** makes the Global section's
Icon Scale slider bind to the real `GlobalMarkerSettings::scaleAlloy/Plasma/Spawn` fields.
Confirmed live, this session: today the slider is fully disconnected — `MarkersTab_Globals_UI.h:37-48`
(`MarkerGlobalScaleRow::iconScale`, one of `MarkersTabGlobals::scaleRows[3]`) is caller-owned UI
scratch state per that file's own SCOPE NOTE 1 (lines 5-12, now stale — it asserts "no `_PARAMS`
home in the tree exists," which is no longer true; `GlobalMarkerSettings::scaleAlloy/Plasma/Spawn`
already is that home), and `MarkersTab_Globals_UI.cpp:26`
(`DrawSliderScalar("Icon Scale", row.iconScale, ...)`) writes into that scratch row, never into
`recipe.globalMarkerSettings`. **This ticket does NOT touch that binding** — it assumes the real
PARAMS fields already exist and are correctly wired for IO round-trip (true today, independent of
STEP121's landing order). Until STEP121 lands, nothing in the app ever writes a non-default value
into `scaleAlloy/Plasma/Spawn` interactively — but importing a `.sanmap` that already carries a
non-default value, or hand-editing the default in a future ticket, is enough to exercise this
ticket's composition regardless.

**Flagged, not silently resolved — landing-order visible-behavior risk.** `scaleAlloy/scalePlasma/
scaleSpawn` all default to `0.17f`, not `1.0f`. The moment this ticket's composition lands, EVERY
map's Alloy/Plasma/Spawn markers — including ones that never touch STEP121's now-real UI control —
render at roughly 17% of their current on-screen size, a highly visible change independent of
STEP121's landing order. `GlobalMarkerSettings_PARAMS.h`'s header comment ties this struct to
`ARCH_11_GlobalMarkerSettings.md §11`/`SANMAP_FORMAT_SPEC` Correction 7, suggesting `0.17` is a
deliberate v1-matching value, not a placeholder — but this ticket does not itself rule on that;
it is called out here for the ARCH Expert/human to confirm before merge, the same posture as this
ticket's own item 5 ruling on strategic mode below. Do not silently "fix" the default to `1.0f` to
avoid the visible change — that is a product decision, not a render-composition bug this ticket
owns.

## Fix

### 1. New resolver — `GlobalMarkerSettings_PARAMS.h`, beside `ResolveMarkerGroupTypeTintColor`
Mirrors that function's exact shape (`GlobalMarkerSettings_PARAMS.h:32-40`) — same group-name
vocabulary (`kSpawnMarkerGroupName`/`"Spawns"`, `"Alloy"`/`"Alloys"`, `"Plasma"`/`"Plasmas"`), but
a multiplicative no-op (`1.0f`) rather than white for an unrecognized name — white is
`ResolveMarkerGroupTypeTintColor`'s "unset" convention for a COLOR channel (STEP115 ruling #5,
cited in that function's own comment, lines 26-31); `1.0f` is the analogous no-op for a
MULTIPLIER, not an arbitrary new convention:
```cpp
// STEP122: the group-name -> GlobalMarkerSettings scale-field mapping, mirroring
// ResolveMarkerGroupTypeTintColor's exact group-name vocabulary above. Unrecognized group name
// (Generic/Expansion/freeform) resolves to 1.0f — a multiplicative no-op, the correct "unset"
// convention for a scale factor (ResolveMarkerGroupTypeTintColor's own white-for-unset is a
// color-channel convention, not directly reusable here).
inline float ResolveMarkerGroupTypeScale(const std::string& groupName, const GlobalMarkerSettings& settings) {
    if (groupName == kSpawnMarkerGroupName || groupName == "Spawns") return settings.scaleSpawn;
    if (groupName == "Alloy" || groupName == "Alloys")               return settings.scaleAlloy;
    if (groupName == "Plasma" || groupName == "Plasmas")             return settings.scalePlasma;
    return 1.0f;
}
```

### 2. Manual markers, icon-overlay path — `ResolveMarkersManual`, `MapCanvas_IconLayer_CullManual_UI.cpp:124-170`
Hoist `layer.iconScale` ONCE per call, mirroring `bLayerOverrideEnabled`'s own once-per-call
hoist immediately above it (lines 130-134, STEP116's precedent — `subLayerArrayIndex` is
invariant for the whole function):
```cpp
// STEP122: layer.iconScale hoisted ONCE per call, same posture as bLayerOverrideEnabled above.
const float layerIconScale = (subLayerArrayIndex >= 0
    && static_cast<std::size_t>(subLayerArrayIndex) < input.recipe->markerLayers.size())
    ? input.recipe->markerLayers[static_cast<std::size_t>(subLayerArrayIndex)].iconScale : 1.0f;
```
Hoist the group-type scale ONCE PER GROUP, alongside the existing per-group tint resolution
(lines 147-150 — `group.name` is invariant within the inner transform loop, same reasoning
already used there):
```cpp
const float groupTypeScale = ResolveMarkerGroupTypeScale(group.name, input.recipe->globalMarkerSettings);   // STEP122
```
Change the `ConsiderManualInstance` call (line 162-167) to compose the final scale instead of
passing `transform.transform.scaleX` bare (line 164):
```cpp
ConsiderManualInstance(input, layer, layerIndex, templateIdentifier,
                       transform.transform.positionX, transform.transform.positionZ,
                       transform.transform.scaleX * groupTypeScale * layerIconScale,   // STEP122
                       PlacementCollectionKind_UI::Markers, static_cast<std::int32_t>(index),
                       groupTintRed, groupTintGreen, groupTintBlue,
                       stableOrderCounter, outAabb, viewRect, diagnostics, outCandidates);
```
`MarkerLayerBundle` (STEP119, `src/params/MarkerLayerBundle_PARAMS.h`) is a parent-grouping
container that gives `MarkerInstanceLayer` a `parentBundleIdentifier` — confirmed it does NOT
restructure `recipe.markerLayers`'s own flat-vector layout or `subLayerArrayIndex`'s positional
meaning (`ResolveMarkersManual`'s indexing above, re-read live this session, is unchanged from
STEP116's original shape).

### 3. Manual markers, dot-renderer — `MapCanvas_MarkerDrag_UI.cpp`
Replace the hardcoded `kManualMarkerDotRadiusScreenPixels` (line 15) with a pure resolver
mirroring `ManualMarkerTint`'s exact shape (lines 24-34 — same anonymous-namespace,
same-signature-family posture, reusing the same `globalMarkerSettings` parameter STEP116 already
threads through this file, not a second one):
```cpp
constexpr float kManualMarkerBaseDotRadiusScreenPixels = 6.0f;

float ManualMarkerDotRadius(const std::vector<Params::MarkerInstanceLayer>& markerLayers, int layerIndex,
                            const std::string& groupName, const Params::GlobalMarkerSettings& globalMarkerSettings) {
    const float layerIconScale = (layerIndex >= 0 && layerIndex < static_cast<int>(markerLayers.size()))
        ? markerLayers[static_cast<std::size_t>(layerIndex)].iconScale : 1.0f;
    return kManualMarkerBaseDotRadiusScreenPixels
         * Params::ResolveMarkerGroupTypeScale(groupName, globalMarkerSettings) * layerIconScale;
}
```
Replace both radius call sites inside `DrawManualMarkerRoster` (lines 86-127) with
`ManualMarkerDotRadius(markerLayers, transform.layerIndex, group.name, globalMarkerSettings)`:
- Line 116: `drawList.AddCircleFilled(screenCenter, ManualMarkerDotRadius(markerLayers, transform.layerIndex, group.name, globalMarkerSettings), tint);`
- Line 122 (ghost points, inside the `for (const Pipeline::WorldSymmetryOrbitPoint& ghost : ...)` loop): same call, using the dragged transform's own `group.name`/`transform.layerIndex` already in scope from the outer loop (the drag-group's own dot size, consistent with the ghost being that same group's sibling orbit slots).

No signature change to `DrawManualMarkerRoster` itself (`MapCanvas_MarkerDrag_UI.h:44-50`) —
`markerLayers`/`globalMarkerSettings` are already parameters.

### 4. Procedural markers — `ResolveProceduralSubLayer`, `MapCanvas_IconLayer_CullProcedural_UI.cpp:20-65`
Procedural markers have no `MarkerInstanceLayer` (confirmed: `Data::PlacementInstances` carries
no per-instance layer-scale concept) — only the Global multiplier applies, no per-layer term.
Procedural markers resolve tint via `Params::MarkerCategory` (the baked `instances.category`
column, lines 46-49), NOT `group.name` — `ResolveMarkerGroupTypeScale` (item 1, string-keyed)
cannot be reused here. Mirror `ResolveMarkerCategoryTintColor`'s own exact enum-switch shape
(`MapCanvas_IconLayer_CullHelpers_UI.cpp:61-77`, declared `MapCanvas_IconLayer_CullInternal_UI.h:72-74`)
with a new sibling, declared/defined beside it:
```cpp
// STEP122: mirrors ResolveMarkerCategoryTintColor's exact switch shape/posture (same file).
// Params::MarkerCategory (MarkerRule_PARAMS.h:18: Generic, Spawn, Alloys, Expansion) has NO
// Plasma value — the same pre-existing gap ResolveMarkerCategoryTintColor already documents
// ("Plasma has no MarkerCategory value at all") — Plasma-named procedural markers fall into the
// default branch below, same as Generic/Expansion. Not this ticket's gap to close.
float ResolveMarkerCategoryScale(Params::MarkerCategory category, const Params::GlobalMarkerSettings& settings) {
    switch (category) {
        case Params::MarkerCategory::Spawn:  return settings.scaleSpawn;
        case Params::MarkerCategory::Alloys: return settings.scaleAlloy;
        default:                             return 1.0f;
    }
}
```
Change `scale` from `const` to mutable and compose inside the existing `Markers` branch
(lines 42, 46-49):
```cpp
float scale = instances.scaleX[static_cast<std::size_t>(instanceIndex)];   // was const
...
if (collection == PlacementCollectionKind_UI::Markers && input.recipe != nullptr) {
    const Params::MarkerCategory category =
        static_cast<Params::MarkerCategory>(instances.category[static_cast<std::size_t>(instanceIndex)]);
    ResolveMarkerCategoryTintColor(category, input.recipe->globalMarkerSettings, tintRed, tintGreen, tintBlue);
    scale *= ResolveMarkerCategoryScale(category, input.recipe->globalMarkerSettings);   // STEP122
} else if (collection == PlacementCollectionKind_UI::Units && input.recipe != nullptr) {
    ...
```
`EmitCandidateIfVisible(..., scale, ...)` (line 61) already reads `scale` by value — no other
change needed at that call.

### 5. Strategic-mode independence — confirmed, deliberate, not a gap
`ResolveLodModeAndIcon`'s strategic branch (`MapCanvas_IconLayer_CullEmit_UI.cpp:41-42`) sets
`resolved.screenSize = layer.strategicIconScreenSizePixels;` — `instanceScale` (this ticket's
composed value) is used ONLY to compute `thumbnailScreenSize` (lines 34-37), which in turn only
decides which LOD branch is taken (the `>=` threshold test, line 39); once strategic mode is
selected, `instanceScale` is discarded entirely for sizing purposes.
`OverlayLayer_Settings_UI.h:38-42`'s own comment calls `strategicIconScreenSizePixels` "§14.3's
strategic-mode fixed screen size" — "fixed" is the operative word: strategic/zoomed-out mode is a
minimap-style abstraction where every marker in a domain renders at one uniform, legible size
regardless of individual scale. **Ruling: Global/per-layer Icon Scale composed by this ticket
affects WHICH mode a marker crosses into (via the composed value feeding `thumbnailScreenSize`'s
threshold test) but never affects strategic mode's own rendered pixel size once selected — this is
deliberate, consistent with the "fixed" comment's existing intent, and this ticket must not touch
`layer.strategicIconScreenSizePixels`'s own composition.**

## File-size ceiling — documented exception (ARCH signoff, Constitution §7)
`MapCanvas_MarkerDrag_UI.cpp` (185 lines), `MapCanvas_IconLayer_CullManual_UI.cpp` (248 lines),
`MapCanvas_IconLayer_Cull_UI_Test.cpp` (644 lines), and `MapCanvas_MarkerDrag_UI_Test.cpp`
(313 lines) are already well over ARCH §1.5's hard 150-line ceiling before this ticket's modest
additions, with no prior documented exception on record. Making the ratchet deliberate rather than
silent — do not use this ticket as the trigger to split any of the four, that is a separate cleanup
ticket's job.

## Out of scope
- **STEP121's UI-binding work** (`MarkersTab_Globals_UI.h/.cpp`'s scale-row -> `recipe.globalMarkerSettings`
  wiring). This ticket assumes the real PARAMS fields already exist and round-trip correctly,
  which they do today regardless of STEP121's landing order.
- **Changing `GlobalMarkerSettings::scaleAlloy/Plasma/Spawn`'s own default `0.17f` values.**
  Flagged above as a visible-behavior-change risk, not resolved by this ticket — a product/ARCH
  call, not a render-pipeline coder call.
- **Adding a `Plasma` value to `Params::MarkerCategory`** (`MarkerRule_PARAMS.h:18`) to close the
  pre-existing procedural-scale/tint gap for Plasma-named markers. Already flagged as pre-existing
  by `ResolveMarkerCategoryTintColor`'s own comment; this ticket's `ResolveMarkerCategoryScale`
  inherits the identical gap deliberately, not as new scope.
- **`layer.strategicIconScreenSizePixels`'s own composition/tunability.** Confirmed
  scale-independent by design, item 5 above — untouched.
- **`ResolveLodModeAndIcon`'s LOD-threshold formula itself** (`baseFootprint`/`worldUnitsPerCell`/
  `PixelsPerPreviewCell`/`ZoomScale` terms). Only its `instanceScale` INPUT changes (composed
  upstream at the two call sites), not the formula.
- **Any UI control for `MarkerInstanceLayer::iconScale`.** Confirmed this field already has a
  documented consumer in `MarkersTab_ManualLayerRowBody_UI.cpp`'s "Icon Scale" slider
  (STEP106-era code, unrelated to this ticket) — this ticket only makes the render pipeline
  finally READ the field; it does not touch the authoring control.

## Files touched
- `src/params/GlobalMarkerSettings_PARAMS.h` — new `ResolveMarkerGroupTypeScale`
- `src/ui/MapCanvas_IconLayer_CullManual_UI.cpp` — `ResolveMarkersManual` composes
  `transform.transform.scaleX * groupTypeScale * layerIconScale` at the `ConsiderManualInstance`
  call
- `src/ui/MapCanvas_IconLayer_CullProcedural_UI.cpp` — `scale` becomes mutable, composed with the
  new `ResolveMarkerCategoryScale` inside the existing `Markers`-collection tint branch
- `src/ui/MapCanvas_IconLayer_CullHelpers_UI.cpp` — new `ResolveMarkerCategoryScale`, beside
  `ResolveMarkerCategoryTintColor`
- `src/ui/MapCanvas_IconLayer_CullInternal_UI.h` — declares `ResolveMarkerCategoryScale`
- `src/ui/MapCanvas_MarkerDrag_UI.cpp` — new `kManualMarkerBaseDotRadiusScreenPixels`,
  `ManualMarkerDotRadius`; `DrawManualMarkerRoster`'s two `AddCircleFilled`/`AddCircle` call
  sites use it instead of the retired `kManualMarkerDotRadiusScreenPixels`

## Verify
Acceptance bar: manual markers, procedural markers, and the roster dot-draw all compose Global ×
per-layer scale into the final rendered size; strategic mode's fixed screen size is provably
untouched; unrecognized group names/categories are provably no-op (`1.0f`), not zero or a crash.

- **New unit test — `ResolveMarkerGroupTypeScale`**, alongside the existing
  `CheckResolveMarkerIconTemplateIdentifier`-style isolated checks in
  `src/ui/MapCanvas_IconLayer_Cull_UI_Test.cpp`: a `GlobalMarkerSettings` with distinct
  non-default `scaleAlloy`/`scalePlasma`/`scaleSpawn`; assert `"Spawn"`/`Params::kSpawnMarkerGroupName`/`"Spawns"`
  resolve `scaleSpawn`, `"Alloy"`/`"Alloys"` resolve `scaleAlloy`, `"Plasma"`/`"Plasmas"` resolve
  `scalePlasma`, and `"Generic"`/`"Expansion"`/an arbitrary freeform name resolve `1.0f`.
- **New unit test — manual marker scale end to end**, extending `ManualMarkerTestFixture`
  (`MapCanvas_IconLayer_Cull_UI_Test.cpp:520-558`, the exact fixture
  `CheckManualMarkerGroupNamePartition`/`CheckManualMarkerIconOverrideWinsEndToEnd` already reuse):
  set `fixture.recipe.markerLayers[0].iconScale = 2.0f` and
  `fixture.recipe.globalMarkerSettings.scaleAlloy = 3.0f`, resolve through the real
  `ResolveManualSubLayer` switch via the `alloyLayer`; assert the one candidate's `screenSize`
  equals the baseline `CheckThumbnailModeAboveThreshold`-style unscaled value (line 46-62's
  formula) multiplied by `2.0f * 3.0f` (within the same tolerance band that test already uses).
- **New unit test — manual marker scale, unrecognized group name stays no-op**: same fixture
  shape, a group named `"Generic"` instead of `"Alloys"`/`Spawn`, `markerLayers[0].iconScale = 2.0f`;
  assert `screenSize` is scaled by `2.0f` only (the layer term), not further multiplied by any
  group-type factor.
- **New unit test — procedural marker scale end to end**, mirroring
  `CheckMarkerAlloysCategoryResolvesColorAlloy`/`CheckMarkerSpawnCategoryResolvesColorSpawn`
  (`MapCanvas_IconLayer_Cull_UI_Test.cpp:261-303`) but asserting `screenSize`: use
  `AppendMarkerInstance`'s existing `scale` parameter
  (`MapCanvas_IconLayer_TestFixture_UI.h:77-87`) to seed a known base instance scale, set
  `fixture.recipe.globalMarkerSettings.scaleAlloy`/`scaleSpawn` to a known non-default value for
  each of the `Alloys`/`Spawn` category cases; assert the resolved candidate's `screenSize` scales
  proportionally. Include a `Generic`/`Expansion` case (mirroring
  `CheckMarkerGenericAndExpansionStayWhite`, lines 307-333) asserting `screenSize` uses the raw
  instance scale unmultiplied (category-scale no-op == `1.0f`).
- **New unit test — `ManualMarkerDotRadius`**, `src/ui/MapCanvas_MarkerDrag_UI_Test.cpp`, alongside
  `RunTypeDefaultColorChecks` (lines 252-333, the STEP116 precedent this test mirrors): a
  `markerLayers` vector with `iconScale = 2.0f` at index 0, a `GlobalMarkerSettings` with
  `scaleAlloy = 3.0f`; assert `ManualMarkerDotRadius(markerLayers, 0, "Alloys", settings) ==
  6.0f * 2.0f * 3.0f` (exposing the currently-anonymous-namespace function the same way
  `ManualMarkerTint` already is for this file's own tests — no new header declaration needed, this
  file already compiles against `MapCanvas_MarkerDrag_UI.cpp`'s translation unit per its existing
  `RunTypeDefaultColorChecks` coverage of `ManualMarkerTint`). Include an out-of-range `layerIndex`
  case (returns the base radius unscaled by any layer term) and an unrecognized `groupName` case
  (returns base radius times only the layer term).
- **Existing suites stay green with no behavior change to any assertion this ticket does not
  itself add**: every existing `screenSize`/tint assertion in `MapCanvas_IconLayer_Cull_UI_Test.cpp`
  that does NOT set a non-default `iconScale`/`scaleAlloy`/`scalePlasma`/`scaleSpawn` must still
  pass unchanged, since every default (`MarkerInstanceLayer::iconScale = 1.0f`,
  `ResolveMarkerGroupTypeScale`/`ResolveMarkerCategoryScale`'s own no-op fallback) leaves the
  composed multiplier at `1.0f` — EXCEPT any fixture that happens to construct a default
  `GlobalMarkerSettings` and route through the `Alloys`/`Spawn` group-name-or-category path, whose
  scale is now the real `0.17f` default rather than `1.0f`; audit
  `MapCanvas_IconLayer_Cull_UI_Test.cpp`'s existing `Alloy`/`SpawnsArmies`-domain `screenSize`
  assertions (if any exist beyond the tint-only ones already read this session) for this exact
  regression before declaring the suite green.

Relevant files read live this session (all citations above verified against current tree):
`D:\Projects\Sanctuary\Map Generator\src\params\GlobalMarkerSettings_PARAMS.h`,
`D:\Projects\Sanctuary\Map Generator\src\params\MarkerInstance_PARAMS.h`,
`D:\Projects\Sanctuary\Map Generator\src\params\MarkerRule_PARAMS.h`,
`D:\Projects\Sanctuary\Map Generator\src\params\MarkerLayerBundle_PARAMS.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_IconLayer_CullEmit_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_IconLayer_CullManual_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_IconLayer_CullProcedural_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_IconLayer_CullHelpers_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_IconLayer_CullInternal_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_MarkerDrag_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_MarkerDrag_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\OverlayLayer_Settings_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MarkersTab_Globals_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MarkersTab_Globals_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_IconLayer_TestFixture_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_IconLayer_Cull_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_MarkerDrag_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\io\MapExporter_MarkersStack_IO.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\io\MapImporter_MarkersStack_IO.cpp`.
