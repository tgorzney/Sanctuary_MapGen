# STEP111 — Icon overlay RGB tint (`OverlayVisibleInstance` gains real color)

**Layer:** UI. **Domain:** `MapCanvas_IconLayer_UI.h`, `MapCanvas_IconLayer_CullEmit_UI.cpp`,
`MapCanvas_IconLayer_CullProcedural_UI.cpp`, `MapCanvas_IconLayer_CullManual_UI.cpp`,
`MapCanvas_IconLayer_CullHelpers_UI.cpp`, `MapCanvas_IconLayer_CullInternal_UI.h`,
`MapCanvas_IconLayer_Cull_UI.cpp`, `MapCanvas_IconLayer_Draw_UI.cpp`, `PropInstance_PARAMS.h`
(two new pure helpers, matching an already-shipped precedent — no new fields). **Sequence:**
independent of STEP112 (`ManualMarkerTint`/army color), STEP113 (tab-gating), STEP114 (icon
override) — no shared call sites with any of them.

## ⚠️ ARCH-ruling course correction — read before implementing

The dispatching brief instructed: resolve Alloy/Plasma color via a reserved-name-literal match
on `Params::MarkerRuleLayer::name`, mirroring `kSpawnMarkerGroupName`'s precedent, and to STOP
and flag back if `MarkerCategory` turns out to be the only reachable signal instead. Having read
the real cull/emit call chain, **neither of those two options is what the code actually
supports** — a materially better, already-baked, already-precedented signal exists, and the
brief's proposed one does not reach this call site at all without new plumbing:

1. **`Params::MarkerRuleLayer::name` is NOT reachable at cull/emit time and is not a reserved
   literal anywhere.** `ResolveProceduralSubLayer` (`src/ui/MapCanvas_IconLayer_CullProcedural_UI.cpp:18-46`)
   only receives a flat `ruleIndex` (int) and `PlacementCollectionKind_UI` — no
   `Params::MarkerRuleLayer`/`Params::MarkerRule` in scope. `kSpawnMarkerGroupName`
   (`src/params/MarkerInstance_PARAMS.h:66`) reserves `Params::MarkerInstanceGroup::name` — the
   MANUAL roster's outer group key — a completely different struct from the PROCEDURAL
   `MarkerRuleLayer::name`, which is proven free text by its own header comment
   ("e.g. an 'outer expansions' layer vs. a 'start Alloys' layer",
   `src/params/MarkerRule_PARAMS.h:73-76`). No procedural code anywhere compares
   `MarkerRuleLayer::name` against a literal (confirmed by grep across `src/`).

2. **The real, already-shipped precedent for this exact routing decision is
   `Params::MarkerRule::category`, not any name.** `SeedMarkerDomains`
   (`src/ui/Application_OverlaySetup_Seed_UI.cpp:53-63`) already routes every procedural marker
   rule to the Alloy or SpawnsArmies overlay domain using
   `rule.category == Params::MarkerCategory::Spawn ? spawnsArmiesLayer : alloyLayer` (line 57).

3. **Better still: the resolved category is already baked per-instance into DATA, one hop away
   in the exact loop that needs it.** `Data::PlacementInstance::category` —
   `"int category = 0;  // Params::MarkerCategory as int (DATA never includes PARAMS)"`
   (`src/data/PlacementInstance_DATA.h:47`) — is written at bake time by
   `Placement_MarkerRules_PROC.cpp:25` (`configuration.category = static_cast<int>(rule.category);`)
   and `Placement_Emit_PROC.cpp:69` (`instance.category = configuration.category;`), landing in
   the SoA column `Data::PlacementInstances::category` (`src/data/PlacementInstances_DATA.h:24`).
   `ResolveProceduralSubLayer` already holds `instances` and `instanceIndex` in scope
   (`MapCanvas_IconLayer_CullProcedural_UI.cpp:26,31-32`) — `instances.category[instanceIndex]`
   is a direct, zero-new-plumbing read, no `input.recipe->markerRuleLayers` walk needed at all.

4. **This does not solve Plasma, and nothing does today — confirmed, not worked around.**
   `Params::MarkerCategory` is `{Generic, Spawn, Alloys, Expansion}`
   (`src/params/MarkerRule_PARAMS.h:18`) — no `Plasma` value, and no other field on
   `MarkerRule`/`MarkerRuleLayer` signals a Plasma-vs-Alloy resource distinction (confirmed: no
   `"Plasma"` match anywhere under `src/proc`). This is a pre-existing PARAMS gap. Per the
   brief's own instruction not to extend the enum, and since a name-literal workaround on free
   text is strictly worse (silent, fragile, not the established pattern), **procedural Plasma
   coloring is out of scope for this ticket** and needs a follow-up PARAMS/ARCH ticket (a new
   `MarkerCategory` value or a `MarkerRule::resourceType` field) to give it a signal at all.
   `GlobalMarkerSettings::colorPlasma`/`iconNamePlasma` stay unconsumed by this ticket.

**Ruling for this ticket: resolve procedural marker tint from
`Data::PlacementInstances::category` → `Params::MarkerCategory` → `Alloys → colorAlloy`,
`Spawn → colorSpawn`, `Generic`/`Expansion → white/no tint` (no strong color opinion, per the
dispatching brief's own fallback guidance).** This is not a downgrade from the brief's intent —
it is the same category-keyed resolution the brief asked for, reached through the actual
existing pipe instead of a nonexistent one.

## Problem
`OverlayVisibleInstance` (`src/ui/MapCanvas_IconLayer_UI.h:38-49`) carries only
`float tintAlpha = 1.0f;` (line 44) — no RGB. `FlushIconLayerBucket`
(`src/ui/MapCanvas_IconLayer_Draw_UI.cpp:79`) hardcodes
`ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, instance.tintAlpha))` — every icon
(procedural markers, manual/procedural Props, Decals, Units) draws pure white regardless of
`GlobalMarkerSettings::colorAlloy/colorSpawn` (`src/params/GlobalMarkerSettings_PARAMS.h:17-19`)
or `PropInstanceLayer::color`/`DecalInstanceLayer::color`
(`src/params/PropInstance_PARAMS.h:30-31`) — all three already round-trip through IO
(`src/io/MapExporter_MarkersStack_IO.cpp:89-94`, `MapImporter_MarkersStack_IO.cpp:93-95`; Props/
Decals layer colors have no dedicated color-specific IO test but the layer struct itself
round-trips via the existing `PropGroups`/`DecalGroups` wire arrays) but never reach the icon
draw call. This is a real WYSIWYG gap: a layer's authored color is invisible in the overlay.

## Fix

### 1. `OverlayVisibleInstance` — three new fields
`src/ui/MapCanvas_IconLayer_UI.h:38-49`, insert after line 44 (`tintAlpha`):
```cpp
struct OverlayVisibleInstance {
    float screenCenterX = 0.0f, screenCenterY = 0.0f;
    float screenSize     = 0.0f;
    float uvMinimumX = 0.0f, uvMinimumY = 0.0f, uvMaximumX = 1.0f, uvMaximumY = 1.0f;
    int   atlasPage         = 0;
    std::uint64_t textureIdentifier = 0;
    float tintAlpha = 1.0f;           // layer.opacity, folded in once here (§14.2)
    float tintColorRed = 1.0f, tintColorGreen = 1.0f, tintColorBlue = 1.0f;   // NEW — STEP111
    int   layerIndex = 0;             // vector order = Z order = decimation priority (§14.7/§14.9)
    ...
```
Three named floats (naming law §1.1), white default (`1.0f` each) — matches every existing
call site that never sets them (the two throwaway/benchmark reconstructions of the tint formula,
see Out of Scope) staying correct by default.

### 2. `FlushIconLayerBucket` — consume the new fields
`src/ui/MapCanvas_IconLayer_Draw_UI.cpp:79`, replace:
```cpp
const ImU32 tint = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, instance.tintAlpha));
```
with:
```cpp
const ImU32 tint = ImGui::ColorConvertFloat4ToU32(
    ImVec4(instance.tintColorRed, instance.tintColorGreen, instance.tintColorBlue, instance.tintAlpha));
```
No other change in this function. The C2 cache path (`CaptureAndCacheBuckets`,
`src/ui/MapCanvas_IconLayer_DrawCache_UI.cpp:35-52`) captures raw `ImDrawVert` bytes AFTER this
function writes them (`FlushIconLayerBucket(drawList, bucket);` then a memcpy of the just-written
vertex range, lines 40-43) — this one-line fix is sufficient for both the live-flush and the
cache-replay path; `ReplayCachedBuckets` (lines 54-76) only rebases indices, never touches color.

### 3. Thread three new tint params through the emit call chain
`EmitCandidateIfVisible`/`AppendCandidate` currently have no way to receive a resolved RGB — add
three trailing float params to both, right before `stableOrderCounter`:

`src/ui/MapCanvas_IconLayer_CullInternal_UI.h:58-63` (declaration) — add
`float tintColorRed, float tintColorGreen, float tintColorBlue,` between `instanceIndex` and
`int* stableOrderCounter`. Also add, near the existing declaration for the new helper (§5 below):
```cpp
void ResolveMarkerCategoryTintColor(Params::MarkerCategory category,
                                    const Params::GlobalMarkerSettings& settings,
                                    float& outRed, float& outGreen, float& outBlue);
```
and at the top of the file (after the existing includes, line 11), add:
```cpp
#include "../params/MarkerRule_PARAMS.h"
#include "../params/GlobalMarkerSettings_PARAMS.h"
```
(needed for `Params::MarkerCategory`/`Params::GlobalMarkerSettings` in the new declaration's
signature; both are pulled into all three `MapCanvas_IconLayer_Cull*_UI.cpp` TUs that already
include this shared header, matching its existing "declarations shared by the trio" role.)

`src/ui/MapCanvas_IconLayer_CullEmit_UI.cpp:47-91` — `AppendCandidate` gains the three params and
sets them on the instance (after the existing `instance.tintAlpha = layer.opacity;` line, 62):
```cpp
instance.tintAlpha = layer.opacity;   // §14.2 — opacity folded into tint, never a second blend path
instance.tintColorRed = tintColorRed; instance.tintColorGreen = tintColorGreen; instance.tintColorBlue = tintColorBlue;
```
`EmitCandidateIfVisible` gains the same three params (inserted between `instanceIndex` and
`stableOrderCounter` in both its declaration and definition) and passes them straight through to
`AppendCandidate`'s call at line 89-90.

### 4. Procedural markers — resolve via `Data::PlacementInstances::category`
`src/ui/MapCanvas_IconLayer_CullProcedural_UI.cpp` — add
`#include "../params/MapRecipe_PARAMS.h"` (needed for `input.recipe->globalMarkerSettings` member
access; `MapCanvas_IconLayer_Ops_UI.h`, pulled in transitively, only forward-declares
`Params::MapRecipe`). In `ResolveProceduralSubLayer` (lines 18-46), before the
`EmitCandidateIfVisible` call at line 43-44:
```cpp
float tintRed = 1.0f, tintGreen = 1.0f, tintBlue = 1.0f;
if (collection == PlacementCollectionKind_UI::Markers && input.recipe != nullptr) {
    const Params::MarkerCategory category =
        static_cast<Params::MarkerCategory>(instances.category[static_cast<std::size_t>(instanceIndex)]);
    ResolveMarkerCategoryTintColor(category, input.recipe->globalMarkerSettings, tintRed, tintGreen, tintBlue);
}
EmitCandidateIfVisible(input, layer, layerIndex, templateIdentifier, worldX, worldZ, scale,
                       collection, instanceIndex, tintRed, tintGreen, tintBlue,
                       stableOrderCounter, diagnostics, outCandidates);
```
Props/Units/Decals procedural rules (`PropRule`/`DecalRule`, `src/params/ScatterRule_PARAMS.h:13-116`)
carry no `color`/layer-association field at all (confirmed by reading the full struct — no such
field exists) — they fall through with the `1.0f, 1.0f, 1.0f` default, unchanged from today.
This is a genuine PARAMS gap (there is nothing to resolve, not an ambiguity to pick between),
same posture as the Plasma gap in the ⚠️ section — not this ticket's to invent a field for.

### 5. New helper — `ResolveMarkerCategoryTintColor`
`src/ui/MapCanvas_IconLayer_CullHelpers_UI.cpp` — add, mirroring `MarkerCategoryLabel`'s own
UI-owned resolution of this same enum (`src/ui/MarkersTab_Rules_UI.h:68-72`, same precedent:
category → UI-facing value lives in UI, not PARAMS):
```cpp
void ResolveMarkerCategoryTintColor(Params::MarkerCategory category,
                                    const Params::GlobalMarkerSettings& settings,
                                    float& outRed, float& outGreen, float& outBlue) {
    switch (category) {
        case Params::MarkerCategory::Spawn:
            outRed = settings.colorSpawn[0]; outGreen = settings.colorSpawn[1]; outBlue = settings.colorSpawn[2];
            return;
        case Params::MarkerCategory::Alloys:
            outRed = settings.colorAlloy[0]; outGreen = settings.colorAlloy[1]; outBlue = settings.colorAlloy[2];
            return;
        // Generic/Expansion: no reserved color today. Plasma has no MarkerCategory value at all
        // (see this ticket's ⚠️ section) — deliberately falls here too, not a bug.
        default:
            outRed = outGreen = outBlue = 1.0f;
            return;
    }
}
```

### 6. Manual Props/Decals — thread the layer's own color
New pure helpers, `src/params/PropInstance_PARAMS.h`, beside `ResolvePropInstanceLayerId`/
`ResolveDecalInstanceLayerId` (lines 37-44), same shape/posture (a single source of truth both
PROC and UI could call, out-of-range-safe, additive):
```cpp
inline void ResolvePropInstanceLayerColor(int layerIndex, const std::vector<PropInstanceLayer>& layers,
                                          float& outRed, float& outGreen, float& outBlue) {
    if (layerIndex < 0 || static_cast<std::size_t>(layerIndex) >= layers.size()) { outRed = outGreen = outBlue = 1.0f; return; }
    outRed = layers[static_cast<std::size_t>(layerIndex)].color[0];
    outGreen = layers[static_cast<std::size_t>(layerIndex)].color[1];
    outBlue = layers[static_cast<std::size_t>(layerIndex)].color[2];
}
inline void ResolveDecalInstanceLayerColor(int layerIndex, const std::vector<DecalInstanceLayer>& layers,
                                           float& outRed, float& outGreen, float& outBlue) {
    if (layerIndex < 0 || static_cast<std::size_t>(layerIndex) >= layers.size()) { outRed = outGreen = outBlue = 1.0f; return; }
    outRed = layers[static_cast<std::size_t>(layerIndex)].color[0];
    outGreen = layers[static_cast<std::size_t>(layerIndex)].color[1];
    outBlue = layers[static_cast<std::size_t>(layerIndex)].color[2];
}
```
`src/ui/MapCanvas_IconLayer_CullManual_UI.cpp` — the shared local helper `ConsiderManualInstance`
(lines 26-40) gains the same three trailing tint params, threaded straight to its own
`EmitCandidateIfVisible` call (line 38-39).

- **`ResolvePropsManual`** (lines 84-105): after the existing
  `const int targetLayerId = Params::ResolvePropInstanceLayerId(subLayerArrayIndex, input.recipe->propLayers);`
  (line 90), add:
  ```cpp
  float layerTintRed = 1.0f, layerTintGreen = 1.0f, layerTintBlue = 1.0f;
  Params::ResolvePropInstanceLayerColor(subLayerArrayIndex, input.recipe->propLayers,
                                        layerTintRed, layerTintGreen, layerTintBlue);
  ```
  and pass `layerTintRed, layerTintGreen, layerTintBlue` into the `ConsiderManualInstance` call
  (lines 98-102).
- **`ResolveDecalsManual`** (lines 107-125): identical pattern with
  `Params::ResolveDecalInstanceLayerColor(subLayerArrayIndex, input.recipe->decalLayers, ...)`
  after line 112, passed into the `ConsiderManualInstance` call (lines 118-122).
- **`ResolveUnitsManual`/`CollectUnitGroupInstances`** (lines 53-80, Units): no color PARAMS field
  reaches this pipeline (confirmed — no `UnitTransform`/`Army`/`UnitGroup` color field anywhere).
  Pass the white default `1.0f, 1.0f, 1.0f` through `ConsiderManualInstance`'s two call sites
  (line 60-63 and its recursive sibling) — literally unchanged rendering, just plumbing the new
  required params through with a no-op value. Units color is explicitly out of scope (see below).

### 7. The C2 replay-selection path — same category resolution
`src/ui/MapCanvas_IconLayer_Cull_UI.cpp` — add `#include "../params/MapRecipe_PARAMS.h"` (the
file currently only includes `MapCanvasView_UI.h`/`PreviewComposite_UI.h` besides the internal
header; `input.recipe->globalMarkerSettings` needs the full type). In
`ResolveSelectedInstanceCandidate` (lines 108-139), which already gates
`collection != PlacementCollectionKind_UI::Markers` away (line 112-113) — i.e. this function only
ever runs for markers — before the `EmitCandidateIfVisible` call (lines 131-134):
```cpp
float tintRed = 1.0f, tintGreen = 1.0f, tintBlue = 1.0f;
if (input.recipe != nullptr) {
    const Params::MarkerCategory category =
        static_cast<Params::MarkerCategory>(markers.category[index]);
    ResolveMarkerCategoryTintColor(category, input.recipe->globalMarkerSettings, tintRed, tintGreen, tintBlue);
}
EmitCandidateIfVisible(input, layer, static_cast<int>(layerIndex), templateIdentifier,
                       markers.positionX[index], markers.positionZ[index], markers.scaleX[index],
                       PlacementCollectionKind_UI::Markers, instanceIndex, tintRed, tintGreen, tintBlue,
                       &stableOrderCounter, nullptr, outCandidates);
```
Without this, a selected marker would flash white for one C2-cache-valid frame before its owning
bucket rebuild restores real color — the same staleness class §14.8's C2 contract exists to
avoid for every other per-instance property.

### 8. `BuildOneLayerAabb` (`MapCanvas_IconLayer_Cull_UI.cpp:39-57`) — no change needed
It calls `ResolveProceduralSubLayer`/`ResolveManualSubLayer` with `viewRect == nullptr` (the
AABB-only pass) — both resolvers' new tint params are computed unconditionally before their
`EmitCandidateIfVisible` call, but that call itself is never reached when `viewRect == nullptr`
(both resolvers return before emission in that mode — confirmed:
`ConsiderManualInstance` line 34 `if (viewRect == nullptr) return;`; `ResolveProceduralSubLayer`
line 36 `if (viewRect == nullptr) continue;` before the emit). No wasted work of consequence, no
correctness risk.

## ⚠️ Sequencing note — required, added at ARCH signoff

STEP114 (a separate, concurrently-drafted ticket — per-marker icon override) ALSO edits
`ConsiderManualInstance` (`src/ui/MapCanvas_IconLayer_CullManual_UI.cpp:26-40`): it adds a brand-new
caller, `ResolveMarkersManual`, written against the PRE-STEP111 (pre-this-ticket) 14-argument
signature — i.e. with no tint params. Both tickets independently claim "no shared call sites, either
order" in their own text; that claim is FALSE for this one function. Whichever ticket's coder
implements second will hit a compile error at that one call site (self-evident, not a silent bug,
but neither ticket's own Files-Touched list names it). Concretely:
- **If this ticket (STEP111) lands first**: no action needed here — STEP114's implementer must add
  `1.0f, 1.0f, 1.0f` (white, matching this ticket's own §6 Units-color no-op convention) to
  `ResolveMarkersManual`'s `ConsiderManualInstance` call when STEP114 lands.
- **If STEP114 lands first**: this ticket's implementer must additionally update
  `ResolveMarkersManual`'s `ConsiderManualInstance` call site (in `MapCanvas_IconLayer_CullManual_UI.cpp`)
  to pass real tint values (resolved the same way `ResolvePropsManual`/`ResolveDecalsManual` do in §6
  above) — this ticket's own Files Touched list did not originally name that call site; treat it as
  in-scope for this ticket's edit pass regardless of which ticket's coder reaches it first.

## Out of scope
- **Manual (hand-placed) markers.** Render via `DrawManualMarkerRoster`/`ManualMarkerTint`
  (`src/ui/MapCanvas_MarkerDrag_UI.cpp`), a completely separate renderer that does not go through
  `OverlayVisibleInstance` at all. Confirmed unreachable via this pipeline even though
  `SeedMarkerDomains` pushes `OverlaySubLayerRef_UI::Manual` refs into the Alloy/SpawnsArmies
  overlay domains (`Application_OverlaySetup_Seed_UI.cpp:44-51`): `ResolveManualSubLayer`'s
  switch (`MapCanvas_IconLayer_CullManual_UI.cpp:136-151`) has `default: return;` for any
  domainKind besides Units/Props/Reclaim/Decals, with the comment "Alloy/SpawnsArmies carry no
  Manual sub-layers this sequence" (line 150) — those Manual refs are inert here today. STEP112's
  territory.
- **Procedural Plasma marker coloring.** No `MarkerCategory::Plasma` and no other resource-type
  signal exists on `MarkerRule`/`MarkerRuleLayer` (⚠️ section). `GlobalMarkerSettings::colorPlasma`
  stays unconsumed until a follow-up PARAMS/ARCH ticket adds a real signal.
  `Params::MarkerCategory` itself is untouched by this ticket (no new enumerator added).
- **Procedural Props/Decals coloring.** `PropRule`/`DecalRule` (`ScatterRule_PARAMS.h`) carry no
  color/layer-association field — nothing to resolve, not an ambiguity.
- **Units coloring.** No color PARAMS field reaches `UnitTransform`/`Army`/`UnitGroup`. Always
  white by construction of §6's plumbing above.
- **Spawn/army-specific coloring beyond the flat `colorSpawn` swatch** (e.g. per-army color) —
  STEP112's territory.
- **`MarkersTab_Globals_UI.h`'s `MarkersTabGlobals::scaleRows[*].previewColor`.** Confirmed
  (`MarkersTab_Globals_UI.cpp`) this array is genuinely disconnected caller-owned UI state per its
  own SCOPE NOTE 1 — never read from or written to `GlobalMarkerSettings`, never touches this
  pipeline. Pre-existing, unrelated to this ticket; not touched.
- **The two throwaway/benchmark reconstructions of the tint formula** —
  `src/ui/MapCanvas_IconLayer_MicrobenchmarkScenarios_UI_Test.cpp:108`
  (`DrawNaivePerInstanceImages`, explicitly "a throwaway comparison, never touching production
  code," per its own header comment) and the fixture builders in
  `MapCanvas_IconLayer_DrawChunkTestSupport_UI.h:43`/`MapCanvas_IconLayer_Draw_UI_Test.cpp:27`
  that only set `tintAlpha`. Left untouched — they never assert on color and the new fields'
  white default (`1.0f`) keeps their existing behavior byte-identical.
- **`Params::ResolvePropInstanceLayerId`/`ResolveDecalInstanceLayerId` themselves** — untouched;
  the two new color-resolving siblings added beside them do not change their behavior or signature.

## Files touched
- `src/ui/MapCanvas_IconLayer_UI.h` — `OverlayVisibleInstance` gains `tintColorRed/Green/Blue`
- `src/ui/MapCanvas_IconLayer_Draw_UI.cpp` — `FlushIconLayerBucket`'s tint construction
- `src/ui/MapCanvas_IconLayer_CullInternal_UI.h` — `EmitCandidateIfVisible`/`ResolveProceduralSubLayer`-
  adjacent declaration gains three tint params; new `ResolveMarkerCategoryTintColor` declaration;
  new `#include`s for `MarkerRule_PARAMS.h`/`GlobalMarkerSettings_PARAMS.h`
- `src/ui/MapCanvas_IconLayer_CullEmit_UI.cpp` — `AppendCandidate`/`EmitCandidateIfVisible` gain
  and set the three tint params
- `src/ui/MapCanvas_IconLayer_CullProcedural_UI.cpp` — `ResolveProceduralSubLayer` resolves tint
  from `instances.category[instanceIndex]` for Markers; new `#include "../params/MapRecipe_PARAMS.h"`
- `src/ui/MapCanvas_IconLayer_CullHelpers_UI.cpp` — new `ResolveMarkerCategoryTintColor`
- `src/ui/MapCanvas_IconLayer_CullManual_UI.cpp` — `ConsiderManualInstance` gains three tint
  params; `ResolvePropsManual`/`ResolveDecalsManual` resolve via the layer's own color;
  `ResolveUnitsManual` passes white
- `src/ui/MapCanvas_IconLayer_Cull_UI.cpp` — `ResolveSelectedInstanceCandidate` resolves the same
  category-based tint for the C2 replay path; new `#include "../params/MapRecipe_PARAMS.h"`
- `src/params/PropInstance_PARAMS.h` — new `ResolvePropInstanceLayerColor`/
  `ResolveDecalInstanceLayerColor`, mirroring `ResolvePropInstanceLayerId`/
  `ResolveDecalInstanceLayerId`'s existing shape (additive, no field/struct change)
- `src/ui/MapCanvas_IconLayer_TestFixture_UI.h` — new `AppendMarkerInstance` fixture helper
  (mirrors `AppendPropInstance`, lines 67-75, with an added `category` parameter)
- `src/ui/MapCanvas_IconLayer_Cull_UI_Test.cpp` — new checks (below)

## Verify
Acceptance bar: `OverlayVisibleInstance` carries real RGB; `Alloys`/`Spawn` procedural markers
tint from `GlobalMarkerSettings`; manual Props/Decals tint from their own layer's `color`;
Generic/Expansion markers, procedural Props/Decals/Units, and manual markers stay white
(unaffected); the C2 selection-replay path never flashes white. New/updated unit tests, following
this suite's own established style (end-to-end through `ResolveVisibleCandidates`/
`DrawOverlayIconLayers`, per `MapCanvas_IconLayer_Cull_UI_Test.cpp`'s existing `Check*` functions
— not isolated tests of the tiny pure helpers alone, matching the fact that
`ResolvePropInstanceLayerId` itself has no isolated test today either).

- **New fixture helper — `AppendMarkerInstance`**, `MapCanvas_IconLayer_TestFixture_UI.h`, beside
  `AppendPropInstance` (lines 67-75):
  ```cpp
  inline void AppendMarkerInstance(Data::PlacementResults& placements, float worldX, float worldZ,
                                   int ruleIndex, Params::MarkerCategory category,
                                   const char* templateIdentifier, float scale = 1.0f) {
      Data::PlacementInstance instance;
      instance.positionX = worldX; instance.positionZ = worldZ;
      instance.scaleX = instance.scaleY = instance.scaleZ = scale;
      instance.ruleIndex = ruleIndex;
      instance.category = static_cast<int>(category);
      instance.templateIdentifier = Data::MakeTemplateIdentifier(templateIdentifier);
      placements.markers.Append(instance);
  }
  ```
- **New test — Alloys category resolves `colorAlloy`**: `IconLayerTestFixture`, set
  `fixture.recipe.globalMarkerSettings.colorAlloy = {0.1f, 0.2f, 0.3f, 1.0f}`; append a marker
  instance with `Params::MarkerCategory::Alloys`, build `ruleBucketIndex.markers`, seed an atlas
  entry, push an `OverlayLayer_UI` with `domainKind = Alloy` and a `ProceduralRule` sub-layer ref;
  call `ResolveVisibleCandidates`; assert the resulting candidate's `tintColorRed/Green/Blue`
  equal `0.1f/0.2f/0.3f`.
- **New test — Spawn category resolves `colorSpawn`**: identical shape with
  `Params::MarkerCategory::Spawn` and `colorSpawn`, `domainKind = SpawnsArmies`.
- **New test — Generic/Expansion stay white**: same shape with
  `Params::MarkerCategory::Generic` (and a second case with `Expansion`), non-default
  `colorAlloy`/`colorSpawn` set on the fixture to prove no bleed-through; assert
  `tintColorRed == tintColorGreen == tintColorBlue == 1.0f`.
- **New test — manual Prop layer color threads through**: `PropInstanceLayer` with
  `color = {0.4f, 0.5f, 0.6f, 1.0f}` pushed to `fixture.recipe.propLayers`; a `PropInstanceGroup`
  with one `PropTransform` whose `layerIndex` targets it; an `OverlayLayer_UI` with
  `domainKind = Props` and a `Manual` sub-layer ref matching that layer's position; assert the
  resolved candidate's RGB equals `0.4f/0.5f/0.6f`.
- **New test — manual Decal layer color threads through**: identical shape for
  `DecalInstanceLayer`/`domainKind = Decals`.
- **New test — procedural Props/Units stay white**: a `PropRule`-sourced candidate (existing
  `AppendPropInstance` fixture pattern) and a Units manual candidate both assert
  `tintColorRed/Green/Blue == 1.0f, 1.0f, 1.0f` regardless of any `GlobalMarkerSettings`/
  `PropInstanceLayer::color` values set on the fixture — proves no accidental cross-talk from the
  Markers-only category resolution.
- **New test — `ResolveSelectedInstanceCandidate` (C2 replay path) resolves the same color**:
  build a fixture with one Alloys-category marker, a valid `selectedInstanceKey` pointing at it;
  call `ResolveSelectedInstanceCandidate` directly; assert the single emitted candidate's RGB
  matches `colorAlloy` (not white) — closes the "flashes white on a cache-valid frame" risk.
- **New test — out-of-range layer index defaults to white**: call
  `Params::ResolvePropInstanceLayerColor`/`ResolveDecalInstanceLayerColor` directly with an empty
  layers vector and any index; assert `outRed == outGreen == outBlue == 1.0f` (mirrors
  `IsMarkerInstanceLayerLocked`'s existing out-of-range-safe convention, STEP106 §3).
- **Existing suites stay green**: every existing call site of `EmitCandidateIfVisible`/
  `AppendCandidate`/`ConsiderManualInstance` across `MapCanvas_IconLayer_Cull_UI_Test.cpp`,
  `MapCanvas_IconLayer_Draw_UI_Test.cpp`, `MapCanvas_IconLayer_Budget_UI_Test.cpp`,
  `MapCanvas_IconLayer_Cache_UI_Test.cpp`, `MapCanvas_IconLayer_Microbenchmark*_UI_Test.cpp`
  (whichever call the changed signatures) updated to pass the three new tint params — the
  signature change means every existing test call site must be updated to compile, not left
  broken; assertions unrelated to color stay byte-identical (default white keeps every existing
  `check()` that doesn't reference RGB passing unchanged).
