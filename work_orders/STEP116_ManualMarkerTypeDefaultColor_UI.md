# STEP116 — Manual markers get real type-based default color

**Layer:** PARAMS (new field), IO (wire), UI (resolution + control). **Domain:**
`MarkerInstance_PARAMS.h`, `GlobalMarkerSettings_PARAMS.h`, `MapExporter_Markers_IO.cpp`,
`MapImporter_Markers_IO.cpp`, `MapCanvas_MarkerDrag_UI.h/.cpp`, `MapCanvas_IconLayer_CullManual_UI.cpp`,
`MarkersTab_ManualLayers_UI.cpp`. **Sequence:** independent of any other undone work-order — STEP111/
112/115 are already shipped and this ticket only adds to what they left in place.

## ⚠️ Scope correction — read before implementing

The dispatching brief scoped this ticket to `ManualMarkerTint`/`MapCanvas_MarkerDrag_UI.cpp` (the
dot-renderer) only, on the premise (accurate when STEP111/112 were written) that manual markers have
exactly one render consumer. **That premise is now stale.** `ResolveMarkersManual`
(`src/ui/MapCanvas_IconLayer_CullManual_UI.cpp:117-148`, added by STEP114) is a SECOND, independent
render path for the exact same manual-marker instances — it feeds the icon-atlas overlay pipeline
(`OverlayVisibleInstance`/`EmitCandidateIfVisible`, STEP111's machinery) real icon sprites via
`ResolveMarkerIconTemplateIdentifier`, and it is reachable in production: `SeedMarkerDomains`
(`Application_OverlaySetup_Seed_UI.cpp:29-51`) already pushes `Manual` sub-layer refs into the
Alloy/SpawnsArmies overlay domains (unchanged since before STEP111 — STEP111 itself quoted this same
code and called those refs "inert" only because `ResolveManualSubLayer`'s switch had no case for
those domains yet); STEP114 added that case
(`MapCanvas_IconLayer_CullManual_UI.cpp:216-220`, `case OverlayDomainKind_UI::Alloy: case
OverlayDomainKind_UI::SpawnsArmies: ResolveMarkersManual(...)`), activating it. Both paths draw in the
same frame, confirmed by the real call order in `MapCanvas_Draw_UI.cpp:43-45`:
`DrawOverlayIconLayerPass` (icons, includes this path) runs, THEN `DrawManualMarkerDragPass` (dots,
STEP94's "stopgap... on top of the terrain/overlay stack") draws over it. `ResolveMarkersManual`
today hardcodes its tint to `1.0f, 1.0f, 1.0f` with the comment "no tint-RGB PARAMS field reaches
manual markers this ticket — always white... STEP111 is separate, out-of-scope work"
(lines 141-144) — i.e. a manual marker's ICON is white today even where its DOT (drawn on top) is
correctly colored. Fixing only the dot-renderer leaves the icon underneath permanently white — a real
WYSIWYG gap the ARCH §14 overlay-layering design exists to prevent, and a direct contradiction of the
human's own stated requirement ("**ALL** Markers need to be colored depending on type").

**Ruling: this ticket fixes both render paths**, sharing one new PARAMS-layer resolution function
(§3 below) so they can never disagree. The ARMY-color enhancement for the icon path (STEP112's rule,
applied to `ResolveMarkersManual`'s Spawn slots) is explicitly **out of scope** for this ticket — it
requires threading `armies` through `ConsiderManualInstance`/`EmitCandidateIfVisible`, a materially
larger, separably-schedulable plumbing change unlike the group-name color fix, which is free once the
shared resolver exists. See Out of Scope.

## Problem

Confirmed live, `src/ui/MapCanvas_MarkerDrag_UI.cpp:24-29`:
```cpp
ImU32 ManualMarkerTint(const std::vector<Params::MarkerInstanceLayer>& markerLayers, int layerIndex) {
    if (layerIndex < 0 || layerIndex >= static_cast<int>(markerLayers.size()))
        return IM_COL32(220, 220, 220, 255);
    const float* color = markerLayers[static_cast<std::size_t>(layerIndex)].color;
    return ImGui::ColorConvertFloat4ToU32(ImVec4(color[0], color[1], color[2], color[3]));
}
```
Zero type-awareness — every manual marker in every group other than Spawn (which STEP112 already
special-cased with real army color, unchanged/untouched by this ticket) renders its raw
`MarkerInstanceLayer::color`, which defaults to white (`MarkerInstance_PARAMS.h:25`,
`float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};`). And per the course-correction above,
`ResolveMarkersManual` (`MapCanvas_IconLayer_CullManual_UI.cpp:117-148`) hardcodes white unconditionally.

**No existing signal distinguishes "never touched, use type default" from "deliberately set to
white."** Confirmed: `MarkerInstanceLayer` has 3 existing booleans for exactly this class of ambiguity
in exactly this struct — `bLocked`, `bGridSnapEnabled` (both STEP106) — each added because a bare
field couldn't represent "on vs. off" unambiguously. The identical shape applies here.

## Design call: Option B — explicit `bool bColorOverrideEnabled`

Two options were weighed (per dispatch): (A) treat float-exact white as "unset," no new field; (B) an
explicit boolean, defaulting `false`, mirroring `bLocked`/`bGridSnapEnabled`'s established shape in
this exact struct.

**Ruling: Option B.** Beyond the dispatch brief's own reasoning (matches established precedent, lets
a human genuinely choose white, composes for free with STEP115's synthesized layers — all confirmed
below), Option A has a concrete correctness defect Option B does not: `MarkerInstanceLayer::color`
round-trips through `.sanmap` JSON as four independent floats
(`MapExporter_Markers_IO.cpp:70-71`/`MapImporter_Markers_IO.cpp:124-129`, `nlohmann::json` ->
`double` -> `float` on read). A bit-exact `color == {1,1,1,1}` comparison is not guaranteed stable
across a save/reload round-trip depending on JSON float formatting/parsing precision — Option A would
make "is this an override" silently flip on some future numeric-formatting change, a class of bug
Option B (a plain `bool`, losslessly JSON round-tripped) cannot have at all.

**Composes with STEP115 for free, confirmed**: `ReconcileMarkerLayers`
(`MapImporter_MarkerLayerReconcile_IO.cpp:177-193`) synthesizes `Params::MarkerInstanceLayer layer;`
with no field overrides beyond `name`/`layerId` — every synthesized layer already has
`bColorOverrideEnabled = false` (struct default) once this ticket adds the field, with zero additional
STEP115-side work, exactly as STEP115's own Out-of-Scope note anticipated ("Any type-based color
resolution... is a separate, not-yet-drafted UI-domain ticket; this ticket must not preempt it").

## Fix

### 1. New field — `MarkerInstance_PARAMS.h`

`MarkerInstanceLayer` (`src/params/MarkerInstance_PARAMS.h:23-42`), beside the STEP106 pair:
```cpp
    bool  bLocked = false;                    // STEP106 §1. Blocks drag/reposition/add/remove for
                                               // every marker on this layer.
    bool  bGridSnapEnabled = false;            // STEP106 §2. Per-layer, not global (see §2).
    float gridSnapSizeWorldUnits = 1.0f;       // STEP106 §2. World-unit cell size; only meaningful
                                               // while bGridSnapEnabled is true.
    bool  bColorOverrideEnabled = false;       // STEP116. false (struct default — every pre-existing
                                               // and every STEP115-synthesized layer): `color` is
                                               // ignored, every marker on this layer resolves its
                                               // owning group's TYPE-default color instead
                                               // (GlobalMarkerSettings::colorAlloy/colorPlasma/
                                               // colorSpawn), white for an unrecognized group name.
                                               // true: `color` is used verbatim, including a
                                               // deliberately-chosen white.
```
Additive-only, no `SanGenVersion` bump — same posture as every prior field on this struct.

### 2. Wire — `MapExporter_Markers_IO.cpp` / `MapImporter_Markers_IO.cpp`

Wire key `"ColorOverrideEnabled"`, PascalCase matching `"Locked"`/`"GridSnapEnabled"`.

`BuildMarkerGroupsJson` (`MapExporter_Markers_IO.cpp:65-83`), after the existing
`layerJson["GridSnapSizeWorldUnits"] = layer.gridSnapSizeWorldUnits;` (line 79), before
`markerGroups.push_back(layerJson);` (line 80):
```cpp
layerJson["ColorOverrideEnabled"] = layer.bColorOverrideEnabled;
```

`ReadMarkerGroupsJson` (`MapImporter_Markers_IO.cpp:116-144`), after the existing
`ReadJsonFloat(layerJson, "GridSnapSizeWorldUnits", layer.gridSnapSizeWorldUnits);` (line 140), still
inside the `if (layerJson.is_object())` block:
```cpp
ReadJsonBoolean(layerJson, "ColorOverrideEnabled", layer.bColorOverrideEnabled);
```
Absent key (every legacy/foreign file, and every STEP115-synthesized layer) keeps the struct default
`false` — no clamp needed (a bool has no range to validate).

### 3. New pure resolver — `GlobalMarkerSettings_PARAMS.h`

One new pure, additive, RGB-only helper — matches the unanimous existing shape of this exact function
family (`ResolveMarkerCategoryTintColor`, `MapCanvas_IconLayer_CullHelpers_UI.cpp`;
`ResolvePropInstanceLayerColor`/`ResolveDecalInstanceLayerColor`, `PropInstance_PARAMS.h:199-212`,
STEP111 §6 — all three are `void`, three-float-out, white-on-no-match). Group-name vocabulary mirrors
`ResolveMarkerIconTemplateIdentifier`'s own already-shipped reserved-literal-plus-singular/plural set
(`MapCanvas_IconLayer_CullManual_UI.cpp:182-193`, STEP114) exactly — so a marker's icon name and its
color always agree on what a group name means, and manual **Plasma** coloring (impossible for
procedural markers per STEP111's own deferral — no `MarkerCategory::Plasma` enumerator exists,
`MarkerRule_PARAMS.h:18`) works here for free, since manual groups key off a free-form string, not the
closed enum:
```cpp
// GlobalMarkerSettings_PARAMS.h
#pragma once
#include <string>
#include "MarkerInstance_PARAMS.h"   // NEW — kSpawnMarkerGroupName

namespace SanmapGen {
namespace Params {

struct GlobalMarkerSettings { /* unchanged */ };

// STEP116: the group-name -> GlobalMarkerSettings-field mapping a manual marker resolves a
// TYPE-default color through, once "no explicit per-layer override" is established by the caller
// (MarkerInstanceLayer::bColorOverrideEnabled). Mirrors ResolveMarkerIconTemplateIdentifier's own
// vocabulary (MapCanvas_IconLayer_CullManual_UI.cpp, STEP114) — Spawn/Spawns, Alloy/Alloys,
// Plasma/Plasmas. Any other group name (Generic/Expansion/freeform) resolves to opaque white — the
// established "unset" convention (STEP115 ruling #5), not a strong color opinion.
inline void ResolveMarkerGroupTypeTintColor(const std::string& groupName, const GlobalMarkerSettings& settings,
                                            float& outRed, float& outGreen, float& outBlue) {
    const float* color = nullptr;
    if (groupName == kSpawnMarkerGroupName || groupName == "Spawns") color = settings.colorSpawn;
    else if (groupName == "Alloy" || groupName == "Alloys")          color = settings.colorAlloy;
    else if (groupName == "Plasma" || groupName == "Plasmas")        color = settings.colorPlasma;
    if (color == nullptr) { outRed = outGreen = outBlue = 1.0f; return; }
    outRed = color[0]; outGreen = color[1]; outBlue = color[2];
}

} // namespace Params
} // namespace SanmapGen
```
No cycle: `MarkerInstance_PARAMS.h` does not include `GlobalMarkerSettings_PARAMS.h` today (confirmed
by reading its full include list) — this is a one-way, legal PARAMS-to-PARAMS dependency, same posture
`MapRecipe_PARAMS.h` already uses to aggregate both.

**Out-of-range/no-layer handling stays with each caller, not this function** — deliberately not a
combined "layer-array + index" resolver. The two consumers' existing out-of-range fallback VALUES
differ (`ManualMarkerTint`'s is neutral gray `IM_COL32(220,220,220,255)`, pinned by STEP112's own
existing tests; `ResolveMarkersManual`'s sibling functions all use white) — baking one fallback into a
shared function would force one of the two to change its already-tested behavior for no reason. Each
caller keeps its own existing in-range check and calls this function only for the "layer says no
override" branch.

### 4. Dot-renderer — `ManualMarkerTint`/`DrawManualMarkerRoster`, `MapCanvas_MarkerDrag_UI.cpp`

`ManualMarkerTint` (lines 24-29) gains `groupName`/`globalMarkerSettings`, keeps its existing
out-of-range gray fallback verbatim (preserves every STEP112 test unchanged — all of them pass an
empty `markerLayers`, so out-of-range is the ONLY branch they exercise):
```cpp
ImU32 ManualMarkerTint(const std::vector<Params::MarkerInstanceLayer>& markerLayers, int layerIndex,
                       const std::string& groupName, const Params::GlobalMarkerSettings& globalMarkerSettings) {
    if (layerIndex < 0 || layerIndex >= static_cast<int>(markerLayers.size()))
        return IM_COL32(220, 220, 220, 255);
    const Params::MarkerInstanceLayer& layer = markerLayers[static_cast<std::size_t>(layerIndex)];
    if (layer.bColorOverrideEnabled)
        return ImGui::ColorConvertFloat4ToU32(ImVec4(layer.color[0], layer.color[1], layer.color[2], layer.color[3]));
    float typeRed = 1.0f, typeGreen = 1.0f, typeBlue = 1.0f;
    Params::ResolveMarkerGroupTypeTintColor(groupName, globalMarkerSettings, typeRed, typeGreen, typeBlue);
    return ImGui::ColorConvertFloat4ToU32(ImVec4(typeRed, typeGreen, typeBlue, layer.color[3]));
}
```
The type-default branch reuses `layer.color[3]` (default `1.0`) as alpha — `MarkerInstanceLayer` has
no separate opacity field the way `OverlayVisibleInstance::tintAlpha`/`layer.opacity` split it
(STEP111 §14.2); the single `color[4]` swatch is the only alpha control this struct has, and
`bColorOverrideEnabled` gates the whole `color[4]` as one unit, not RGB alone.

**Priority order** — confirmed against the real current call site (`DrawManualMarkerRoster`, lines
101-108):
```cpp
ImU32 tint;
if (bThisGroupDragging && dragState.bSpawnCardinalityRefused) {
    tint = refusedTint;
} else if (IsSpawnMarkerGroup(group)) {
    tint = ManualSpawnArmyTint(armies, transform.name, ManualMarkerTint(markerLayers, transform.layerIndex));
} else {
    tint = ManualMarkerTint(markerLayers, transform.layerIndex);
}
```
becomes:
```cpp
ImU32 tint;
if (bThisGroupDragging && dragState.bSpawnCardinalityRefused) {
    tint = refusedTint;
} else if (IsSpawnMarkerGroup(group)) {
    tint = ManualSpawnArmyTint(armies, transform.name,
                               ManualMarkerTint(markerLayers, transform.layerIndex, group.name, globalMarkerSettings));
} else {
    tint = ManualMarkerTint(markerLayers, transform.layerIndex, group.name, globalMarkerSettings);
}
```
Resulting priority, unchanged in shape from STEP112, extended at its one fallback leaf: (1) refused-red
> (2) Spawn army-color match > (3) explicit layer override (`bColorOverrideEnabled`) > (4) group-name
type default > (5) white for an unrecognized group name. Tier (2)'s own fallback (an orphaned Spawn
slot) now genuinely improves: it resolves through `ManualMarkerTint(..., "Spawn", ...)`, which — for a
layer with `bColorOverrideEnabled == false` — now returns `colorSpawn`, not a flat gray, for any
in-range layer. (STEP112's own "orphaned slot falls back to neutral gray" test still passes because
that test uses an empty `markerLayers` — out-of-range, tier 4/5 never reached; see Verify for a NEW
test proving the improved in-range case.)

**Threading**: `DrawManualMarkerRoster`'s signature (`MapCanvas_MarkerDrag_UI.h:42-47`) gains
`globalMarkerSettings` after `armies`:
```cpp
void DrawManualMarkerRoster(const std::vector<Params::MarkerInstanceGroup>& markers,
                            const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                            const std::vector<Params::Army>& armies,
                            const Params::GlobalMarkerSettings& globalMarkerSettings,
                            const MarkerDragGestureState& dragState, const PreviewComposite& composite,
                            const MapCanvasView& view, float regionOriginX, float regionOriginY,
                            ImDrawList& drawList);
```
`#include "../params/GlobalMarkerSettings_PARAMS.h"` in `MapCanvas_MarkerDrag_UI.h` (the signature
names `Params::GlobalMarkerSettings` directly — same posture STEP112 used for `Army_PARAMS.h`). No new
include in the `.cpp`: already transitively reachable via `../params/MapRecipe_PARAMS.h`
(`MapCanvas_MarkerDrag_UI.cpp:8`, which includes `GlobalMarkerSettings_PARAMS.h`, confirmed).

`MapCanvas::DrawManualMarkerDragPass` (`MapCanvas_MarkerDrag_UI.cpp:164-172`):
```cpp
void MapCanvas::DrawManualMarkerDragPass(float regionOriginX, float regionOriginY) {
    if (composite == nullptr || manualMarkerDragMarkers == nullptr) return;
    static const std::vector<Params::MarkerInstanceLayer> kNoLayers;
    static const std::vector<Params::Army> kNoArmies;
    static const Params::GlobalMarkerSettings kDefaultGlobalMarkerSettings;
    DrawManualMarkerRoster(*manualMarkerDragMarkers, manualMarkerDragLayers != nullptr ? *manualMarkerDragLayers : kNoLayers,
                          manualMarkerDragRecipe != nullptr ? manualMarkerDragRecipe->armies : kNoArmies,
                          manualMarkerDragRecipe != nullptr ? manualMarkerDragRecipe->globalMarkerSettings : kDefaultGlobalMarkerSettings,
                          manualMarkerDragState, *composite, view, regionOriginX, regionOriginY,
                          *ImGui::GetWindowDrawList());
}
```
No new `MapCanvas` member — `manualMarkerDragRecipe` (`MapCanvas_UI.h:190`) already carries
`globalMarkerSettings`, same reuse STEP112 already established for `armies`.

Also update `DrawManualMarkerRoster`'s own header comment
(`MapCanvas_MarkerDrag_UI.h:36-41`, "...tinted by its layer's color (or a neutral default)") to say
"...tinted by its layer's color override, its group's type-default color, or a neutral default" —
documentation accuracy only, no behavior change.

### 5. Icon-overlay path — `ResolveMarkersManual`, `MapCanvas_IconLayer_CullManual_UI.cpp`

Current (lines 117-148), tint hardcoded per §"⚠️ Scope correction" above. The override check is
hoisted ONCE per call (`subLayerArrayIndex`/`markerLayers` are invariant for the whole function), the
type-default resolves once per GROUP iteration (mirrors this file's own established "en bloc, not per
transform" discipline, e.g. `ResolvePropsManual`'s `bReclaimable` partition comment, line 85-86 —
NOT per-transform, since `group.name` is invariant within one group's inner loop too):
```cpp
void ResolveMarkersManual(const DrawOverlayIconLayersInput& input, const OverlayLayer_UI& layer,
                          int layerIndex, int subLayerArrayIndex, int* stableOrderCounter,
                          LayerWorldAabb_UI* outAabb, const ViewWorldRect_UI* viewRect,
                          IconLayerCullDiagnostics_UI* diagnostics,
                          std::vector<OverlayVisibleInstance>& outCandidates) {
    const bool bWantSpawnGroups = (layer.domainKind == OverlayDomainKind_UI::SpawnsArmies);
    const bool bLayerOverrideEnabled = subLayerArrayIndex >= 0
        && static_cast<std::size_t>(subLayerArrayIndex) < input.recipe->markerLayers.size()
        && input.recipe->markerLayers[static_cast<std::size_t>(subLayerArrayIndex)].bColorOverrideEnabled;
    float overrideTintRed = 1.0f, overrideTintGreen = 1.0f, overrideTintBlue = 1.0f;
    if (bLayerOverrideEnabled) {
        const Params::MarkerInstanceLayer& overrideLayer =
            input.recipe->markerLayers[static_cast<std::size_t>(subLayerArrayIndex)];
        overrideTintRed = overrideLayer.color[0]; overrideTintGreen = overrideLayer.color[1];
        overrideTintBlue = overrideLayer.color[2];
    }
    for (const Params::MarkerInstanceGroup& group : input.recipe->markers) {
        const bool bIsSpawnGroup = group.name == Params::kSpawnMarkerGroupName;
        if (bIsSpawnGroup != bWantSpawnGroups) continue;
        float groupTintRed = overrideTintRed, groupTintGreen = overrideTintGreen, groupTintBlue = overrideTintBlue;
        if (!bLayerOverrideEnabled)
            Params::ResolveMarkerGroupTypeTintColor(group.name, input.recipe->globalMarkerSettings,
                                                     groupTintRed, groupTintGreen, groupTintBlue);
        for (std::size_t index = 0; index < group.transforms.size(); ++index) {
            const Params::MarkerTransform& transform = group.transforms[index];
            if (transform.layerIndex != subLayerArrayIndex) continue;
            const std::string templateIdentifier =
                ResolveMarkerIconTemplateIdentifier(transform, group, input.recipe->globalMarkerSettings);
            ConsiderManualInstance(input, layer, layerIndex, templateIdentifier,
                                   transform.transform.positionX, transform.transform.positionZ,
                                   transform.transform.scaleX, PlacementCollectionKind_UI::Markers,
                                   static_cast<std::int32_t>(index),
                                   groupTintRed, groupTintGreen, groupTintBlue,   // STEP116
                                   stableOrderCounter, outAabb, viewRect, diagnostics, outCandidates);
        }
    }
}
```
No signature change to `ConsiderManualInstance`/`EmitCandidateIfVisible` — both already accept three
tint floats (STEP111). No new `#include`: `Params::MarkerInstanceLayer`/`Params::GlobalMarkerSettings`/
`Params::ResolveMarkerGroupTypeTintColor` are all already reachable via the existing
`#include "../params/MapRecipe_PARAMS.h"` (line 18), which includes `GlobalMarkerSettings_PARAMS.h`.

### 6. UI control — `DrawLayerRowBody`, `MarkersTab_ManualLayers_UI.cpp`

Current (lines 34-56), the swatch already sits behind the block-wide `state.bUseGroupColor` gate
(a pre-existing, **confirmed dead/unwired** editor-preview-only toggle — grep confirms
`EffectiveManualMarkerLayerColor` has zero callers anywhere; not touched by this ticket, noted only so
the new control's nesting reads correctly):
```cpp
    if (!state.bUseGroupColor)
        DrawColorSwatch("Color", layer.color, state.previewColorOptions, state.selectedLayerColorToggle);
```
becomes, mirroring the Snap-to-Grid checkbox-gates-disabled-sibling-control pattern already in this
same function (lines 48-52) exactly:
```cpp
    bool bColorOverrideCommitted = false;
    if (!state.bUseGroupColor) {
        bColorOverrideCommitted = DrawCheckbox("Color Override", layer.bColorOverrideEnabled).bCommitted;
        ImGui::BeginDisabled(!layer.bColorOverrideEnabled);
        DrawColorSwatch("Color", layer.color, state.previewColorOptions, state.selectedLayerColorToggle);
        ImGui::EndDisabled();
    }
```
Fold into the return composition (current line 55, `return bNameCommitted || bSnapCommitted ||
bSnapSizeCommitted;`) becomes `return bNameCommitted || bColorOverrideCommitted || bSnapCommitted ||
bSnapSizeCommitted;` — rides the existing `bAnyNameCommitted` → `MakeNamesUnique` fold-up path for
free, same as STEP106 §7's controls. `Checkbox_UI.h` is already included
(`MarkersTab_ManualLayers_UI.cpp:6`) — no new include.

## Out of scope

- **Army-color priority for `ResolveMarkersManual`'s (icon-path) Spawn slots.** The dot-renderer keeps
  its STEP112 army-color match untouched; the icon path gets type-default color (this ticket) but NOT
  army color — that requires threading `armies` through `ConsiderManualInstance`/
  `EmitCandidateIfVisible` (a real signature-growth change across 4+ call sites in this file), a
  materially larger, separably-schedulable unit unlike the group-name fix, which needed zero new
  plumbing. Flagged as a real, known gap: today a Spawn slot's ICON (if a real atlas sprite resolves)
  shows `colorSpawn` while its DOT (drawn on top) shows the real army color — the two agree only when
  no army matches. Follow-up ticket territory, same shape as STEP112 was for the dot path.
- **Procedural marker coloring.** STEP111's shipped territory, untouched.
- **`GlobalMarkerSettings::colorAlloy`/`colorPlasma`/`colorSpawn`'s own default VALUES.** Untouched —
  this ticket only wires resolution of the already-existing settings, never changes them.
- **`MarkersTab_ManualLayers_UI.h`'s `bUseGroupColor`/`EffectiveManualMarkerLayerColor`.** Confirmed
  dead (zero callers) and pre-existing; not this ticket's concern to wire up or remove.
- **`DrawManualMarkerRoster`'s own "Superseded outright by a future real overlay/icon ticket" header
  comment** (`MapCanvas_MarkerDrag_UI.h:40`) — now stale (STEP114 shipped that "future" overlay/icon
  path without retiring the dot renderer; both draw today, per the ⚠️ section). Documentation-accuracy
  cleanup only; not fixing the underlying double-render question (whether the dot renderer should be
  retired now that icons exist) — that is an ARCH-scope call about `MapCanvas_Draw_UI.cpp`'s draw-pass
  composition, not a color-resolution question this ticket owns.
- **`Params::MarkerCategory` enum.** Untouched — manual markers key off the free-form
  `MarkerInstanceGroup::name` string (already the case before this ticket), never this enum.
- **Any change to Props/Decals layer color resolution.** `ResolvePropInstanceLayerColor`/
  `ResolveDecalInstanceLayerColor` (STEP111 §6) are untouched; this ticket's new function is a
  markers-only sibling, not a generalization of them.

## Files touched
- `src/params/MarkerInstance_PARAMS.h` — `bColorOverrideEnabled` on `MarkerInstanceLayer`
- `src/params/GlobalMarkerSettings_PARAMS.h` — new `ResolveMarkerGroupTypeTintColor`;
  `#include "MarkerInstance_PARAMS.h"`
- `src/io/MapExporter_Markers_IO.cpp` — `BuildMarkerGroupsJson` writes `"ColorOverrideEnabled"`
- `src/io/MapImporter_Markers_IO.cpp` — `ReadMarkerGroupsJson` reads `"ColorOverrideEnabled"`
- `src/ui/MapCanvas_MarkerDrag_UI.h` — `DrawManualMarkerRoster` gains `globalMarkerSettings` parameter;
  `#include "../params/GlobalMarkerSettings_PARAMS.h"`; header comment update
- `src/ui/MapCanvas_MarkerDrag_UI.cpp` — `ManualMarkerTint` gains `groupName`/`globalMarkerSettings`
  and resolves override-or-type-default; `DrawManualMarkerRoster`'s two call sites pass the new
  params; `MapCanvas::DrawManualMarkerDragPass` threads `manualMarkerDragRecipe->globalMarkerSettings`
  (with a `kDefaultGlobalMarkerSettings` null-safe fallback)
- `src/ui/MapCanvas_MarkerDrag_UI_Test.cpp` — all five existing `DrawManualMarkerRoster` call sites
  gain a `globalMarkerSettings` argument; new checks (see Verify)
- `src/ui/MapCanvas_IconLayer_CullManual_UI.cpp` — `ResolveMarkersManual` resolves real tint
  (override-or-type-default) instead of hardcoded white
- `src/ui/MapCanvas_IconLayer_Cull_UI_Test.cpp` — new checks (see Verify)
- `src/ui/MarkersTab_ManualLayers_UI.cpp` — `DrawLayerRowBody` gains the "Color Override" checkbox,
  gates the swatch on it, folds into the return composition
- `src/io/MapImporter_IO_Test.cpp` — extend `FillFixtureMarkersAndChains`/`CheckMarkersAndChains`,
  `CheckMarkerGroupsLegacyLockAndSnapDefaults` (see Verify)

## Verify

Acceptance bar: `bColorOverrideEnabled` round-trips (including legacy/absent-key defaulting `false`);
`ResolveMarkerGroupTypeTintColor` resolves Spawn/Alloys/Plasma (with singular/plural variants) and
defaults white otherwise; BOTH render paths (`ManualMarkerTint`/dot and `ResolveMarkersManual`/icon)
honor override-then-type-then-white in that order; every STEP112 test stays green unchanged; a
STEP115-synthesized layer gets real type color with zero additional code.

- **New test — `MarkerInstanceLayer::bColorOverrideEnabled` round-trip**, `MapImporter_IO_Test.cpp`:
  extend `FillFixtureMarkersAndChains` (line ~1206) with `markerLayer.bColorOverrideEnabled = true;`
  and `CheckMarkersAndChains` (line ~669) with `Check(loadedLayer.bColorOverrideEnabled ==
  originalLayer.bColorOverrideEnabled, "MarkerInstanceLayer::bColorOverrideEnabled survives,
  non-default");`.
- **New test — legacy default**: extend `CheckMarkerGroupsLegacyLockAndSnapDefaults` (line ~1975) with
  `Check(layer.bColorOverrideEnabled == false, "bColorOverrideEnabled keeps its struct default (false)
  when the key is absent");`.
- **New test — `ResolveMarkerGroupTypeTintColor`** (a new pure-helper test, `GlobalMarkerSettings_PARAMS.h`
  has no existing test file — add one, or place beside an existing PARAMS-helper test if the Coder
  finds a closer sibling): `Spawn`/`Spawns` resolve `colorSpawn`; `Alloy`/`Alloys` resolve `colorAlloy`;
  `Plasma`/`Plasmas` resolve `colorPlasma`; `Generic`/`Expansion`/an arbitrary freeform name all resolve
  white regardless of non-default settings values set on the fixture (proves no accidental match).
- **New test — dot renderer, override wins over type default**: `MapCanvas_MarkerDrag_UI_Test.cpp`,
  a one-entry `markerLayers` with `bColorOverrideEnabled = true`, `color = {0.1f, 0.2f, 0.3f, 1.0f}`,
  group name `"Alloys"` (so a type-default WOULD otherwise resolve `colorAlloy`, deliberately
  different); assert `LastVertexColor(drawList) == ImGui::ColorConvertFloat4ToU32(ImVec4(0.1f, 0.2f,
  0.3f, 1.0f))`, not `colorAlloy`.
- **New test — dot renderer, type default when override disabled**: same shape, `bColorOverrideEnabled
  = false`, group name `"Alloys"`, `globalMarkerSettings.colorAlloy = {0.4f, 0.5f, 0.6f, 1.0f}`; assert
  the vertex color matches `colorAlloy` (RGB) with alpha from `layer.color[3]` (struct default `1.0`).
- **New test — dot renderer, unrecognized group name resolves white**: `bColorOverrideEnabled = false`,
  group name `"Generic"`, non-default `colorAlloy`/`colorSpawn` set on the fixture; assert the vertex
  color is opaque white, proving no bleed-through.
- **New test — dot renderer, orphaned Spawn slot with a real in-range layer now resolves `colorSpawn`**
  (the improved case STEP112's own empty-`markerLayers` test could not exercise): Spawn group, a
  transform name matching no army, ONE real `markerLayers` entry with `bColorOverrideEnabled = false`,
  `globalMarkerSettings.colorSpawn` set non-default; assert the vertex color matches `colorSpawn`, not
  the old flat gray — this is the exact WYSIWYG improvement this ticket delivers for orphaned slots.
- **New test — icon-overlay path, type default resolves end to end**: extend
  `ManualMarkerTestFixture`-style coverage in `MapCanvas_IconLayer_Cull_UI_Test.cpp` (reuse its exact
  shape, lines 520-558): set `fixture.recipe.globalMarkerSettings.colorAlloy = {0.1f, 0.2f, 0.3f,
  1.0f}`, leave `markerLayers[0].bColorOverrideEnabled` at its default `false`; resolve via the Alloy
  domain layer; assert the emitted candidate's `tintColorRed/Green/Blue` equal `0.1f/0.2f/0.3f`.
  Repeat for the SpawnsArmies domain/`colorSpawn`.
- **New test — icon-overlay path, layer override wins over type default**: same fixture,
  `markerLayers[0].bColorOverrideEnabled = true`, `markerLayers[0].color = {0.7f, 0.8f, 0.9f, 1.0f}`,
  non-default `colorAlloy` also set (to prove no bleed); assert the resolved candidate's RGB equals
  `0.7f/0.8f/0.9f`.
- **New test — icon-overlay path, Generic/Expansion stay white**: mirrors
  `CheckMarkerGenericAndExpansionStayWhite`'s existing shape (this file, procedural-marker coverage)
  but for a MANUAL group named `"Generic"`; assert RGB stays `1.0f/1.0f/1.0f` despite non-default
  `colorAlloy`/`colorSpawn` on the fixture.
- **Existing suites stay green, no assertion this ticket doesn't add changes**: every existing
  `DrawManualMarkerRoster` call site in `MapCanvas_MarkerDrag_UI_Test.cpp` (`RunHitTestChecks` doesn't
  call it; `RunDrawAtRestAndSoftHideChecks`, `RunDrawRefusedTintChecks`, `RunSpawnArmyTintChecks` all
  do, all with empty `markerLayers` — updated to pass a default-constructed
  `Params::GlobalMarkerSettings{}`, out-of-range branch unaffected, every existing assertion
  byte-identical); `CheckManualMarkerGroupNamePartition`/`CheckManualMarkerIconOverrideWinsEndToEnd`/
  `CheckManualMarkerLayerIndexFilter` in `MapCanvas_IconLayer_Cull_UI_Test.cpp` (their fixture's
  `markerLayers[0].bColorOverrideEnabled` stays struct-default `false`, so their assertions — which
  don't check color — are unaffected by tint now resolving to `colorAlloy`/`colorSpawn` instead of
  white, since none of them assert on `tintColorRed/Green/Blue`); `MapExporter_IO_Test`/
  `MapImporter_IO_Test` unrelated fixtures untouched.
