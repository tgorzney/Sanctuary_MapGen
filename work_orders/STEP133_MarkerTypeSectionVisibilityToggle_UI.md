# STEP133 — Per-Type Hide/Unhide toggle on each Marker Type-section header

**Layer:** UI. **Domain:** `MarkersTab_TypeSections_UI.h`, `MarkersTab_UI.cpp`, `MapCanvas_UI.h/.cpp`,
`MapCanvas_IconLayer_Ops_UI.h`, `MapCanvas_IconLayer_Draw_UI.cpp`,
`MapCanvas_IconLayer_CullManual_UI.cpp`, `MapCanvas_IconLayer_CullProcedural_UI.cpp`,
`MapCanvas_IconLayer_CullInternal_UI.h`, `Application_UI.cpp`, `Application_OverlaySetup_Seed_UI.cpp`.
**Sequence:** independent.

Adds a right-aligned Hide/Unhide button to each of the three Alloy/Plasma/Spawn Section headers in
the (currently gutted) Markers tab. Clicking it toggles that Type's markers off the map preview
entirely — both manual and procedural. This is a session-only UI preview filter, never serialized,
never a PARAMS field (mirrors `OverlayLayerSettings`'s own established posture, confirmed by direct
read of its own header comment).

## Ground truth, confirmed by direct investigation before drafting

- The universal collapsing-section widget (`Section_UI.h`) already supports a right-aligned header
  button via `SectionOptions::reservedRightWidth` — the EXACT pattern to reuse is
  `HeightmapTab_UI.cpp:29-35,102-107` ("Add GeoLayer"): reserve `buttonWidth + spacing`, then
  `if (DrawSectionBegin(...)) { ImGui::SameLine(); <button>; ...; DrawSectionEnd(); }`. No widget
  library change needed.
- **Manual markers**: `Params::MarkerInstanceGroup::name` IS the marker Type name already
  (`MarkerInstance_PARAMS.h:86-88`) — the exact same vocabulary (`Alloy`/`Alloys`, `Plasma`/
  `Plasmas`, `kSpawnMarkerGroupName`/`Spawns`) `ResolveMarkerGroupTypeTintColor` already switches on.
  Gate `ResolveMarkersManual` (`MapCanvas_IconLayer_CullManual_UI.cpp:128-192`) at the group level
  (where `bIsSpawnGroup` is computed today, line ~152) — mirrors `ResolvePropsManual`'s existing
  `bReclaimable` group-level gate exactly. **No new plumbing needed.**
- **Procedural markers**: `Params::MarkerRuleLayer::markerTypeName` is the real Type signal
  (`MarkerRule_PARAMS.h:86-88`) — `Params::MarkerCategory` (the only per-instance signal
  `ResolveProceduralSubLayer` currently has, `MapCanvas_IconLayer_CullProcedural_UI.cpp:47-48`) has
  **no `Plasma` enumerator** and cannot disambiguate Alloy from Plasma. **New plumbing required**: a
  `ruleIndex -> markerTypeName` lookup, built once per frame (or cached, gated the same way the AABB
  cache already is) by walking `recipe.markerRuleLayers` the same way
  `Application_OverlaySetup_Seed_UI.cpp:53-63`'s `SeedMarkerDomains` already walks it to route
  Alloy-vs-SpawnsArmies (that code only checks `rule.category == Spawn` today and discards layer
  identity — this ticket needs the FULL `markerTypeName`, not just the Spawn/non-Spawn split).
- **Where the toggle state lives and reaches MapCanvas**: mirrors `MapCanvas::
  SetManualMarkerSelectionSource`'s exact injected-pointer shape (`MapCanvas_UI.h:124-126`) — a new
  small struct/pointer, NOT a `Params::MapRecipe` field. `OverlayLayerSettings` (the View popup's own
  6-domain-row struct, `OverlayLayer_Settings_UI.h`) is the wrong precedent to extend directly — it's
  a 2-way Alloy/SpawnsArmies split with no Plasma row, and restructuring it to add a third domain is
  a much larger, riskier change than this ticket needs.
- **Cache-invalidation gotcha, the reason this ticket exists rather than being a 10-minute edit**:
  `MapCanvas_IconLayer_Draw_UI.cpp:107-115` computes `revision` from
  `input.overlayLayerSettings->layerSettingsRevision` ONLY, and gates the entire C2 render cache
  (`ShouldInvalidateIconLayerCache`) on it. A new, independent visibility source that isn't folded
  into this revision will NOT invalidate the cache when toggled — the Hide/Unhide button would flip
  its own state but the canvas would keep showing the stale (pre-toggle) render until something else
  (a pan/zoom) happens to invalidate it. **This must not be worked around by writing to
  `OverlayLayerSettings::layerSettingsRevision` directly from the Markers tab** (that struct is
  Application-shell-owned session state, not Markers-tab-owned, and mutating it from a different
  tab's code would be a real layering violation) — instead, give the new visibility-source struct its
  own small revision counter, thread it through `DrawOverlayIconLayersInput` as an additional field,
  and combine it into the revision computed at `MapCanvas_IconLayer_Draw_UI.cpp:110`, e.g.:
  ```cpp
  const std::uint64_t revision = input.overlayLayerSettings->layerSettingsRevision
                               + input.markerTypeVisibilityRevision * 1000003ull;   // large-prime spread
  ```

## Fix

### 1. New visibility-source struct

New small header (e.g. `MarkerTypeVisibility_UI.h`), mirroring `OverlayLayerSettings`'s own
`BumpLayerSettingsRevision` shape:
```cpp
struct MarkerTypeVisibility_UI {
    std::unordered_map<std::string, bool> hiddenByTypeName;   // absent/false = visible (default)
    std::uint64_t revision = 0;
    void SetHidden(const std::string& typeName, bool bHidden) {
        hiddenByTypeName[typeName] = bHidden;
        ++revision;
    }
    bool IsHidden(const std::string& typeName) const {
        const auto it = hiddenByTypeName.find(typeName);
        return it != hiddenByTypeName.end() && it->second;
    }
};
```
Owned by `MarkersTabState` (new field), NOT `OverlayLayerSettings` — session-local to the Markers tab,
same posture `selectedManualInstanceIdentifier` already has one tier up.

### 2. The Hide/Unhide button, per Type-section

In `MarkersTab_UI.cpp`'s per-type loop (currently three empty sections), mirror
`HeightmapTab_UI.cpp`'s exact composition:
```cpp
SectionOptions HideToggleSectionOptions(bool bHidden) {
    SectionOptions options;
    const float buttonWidth = ImGui::CalcTextSize(bHidden ? "Unhide" : "Hide").x
                             + ImGui::GetStyle().FramePadding.x * 2.0f;
    options.reservedRightWidth = buttonWidth + kHideToggleButtonSpacingPixels;   // named constant
    return options;
}
// ...
const bool bHidden = state.markerTypeVisibility.IsHidden(typeName);
if (DrawSectionBegin(typeName, perTypeSectionState, HideToggleSectionOptions(bHidden))) {
    ImGui::SameLine();
    if (ImGui::SmallButton(bHidden ? "Unhide" : "Hide"))
        state.markerTypeVisibility.SetHidden(typeName, !bHidden);
    DrawSectionEnd();
}
```
Right-align the button precisely within the reserved zone (not just flush-left of it) so it reads as
right-aligned regardless of which label ("Hide" vs the wider "Unhide") is currently showing — use
`ImGui::SetCursorPosX` to position it at the reserved zone's own right edge minus its own measured
width, rather than relying on `SameLine()` alone to land it flush.

### 3. Thread the new source into MapCanvas

`MapCanvas_UI.h`: new setter mirroring `SetManualMarkerSelectionSource` exactly:
```cpp
void SetMarkerTypeVisibilitySource(const MarkerTypeVisibility_UI* visibility) {
    markerTypeVisibilitySource = visibility;
}
// private: const MarkerTypeVisibility_UI* markerTypeVisibilitySource = nullptr;
```
Wired in `Application_UI.cpp`'s `WireCallbacks()`, mirroring the existing
`canvas.SetManualMarkerSelectionSource(&tabState.markers.selectedManualInstanceIdentifier);` call —
same file, same function, one more line: `canvas.SetMarkerTypeVisibilitySource(&tabState.markers.markerTypeVisibility);`.

`DrawOverlayIconLayersInput` (`MapCanvas_IconLayer_Ops_UI.h`) gains:
```cpp
const MarkerTypeVisibility_UI* markerTypeVisibility = nullptr;
std::uint64_t markerTypeVisibilityRevision = 0;   // 0 when the pointer is null (no filtering, always visible)
```
`MapCanvas::DrawOverlayIconLayerPass` (`MapCanvas_Draw_UI.cpp`, wherever it assembles
`DrawOverlayIconLayersInput` from the canvas's own injected pointers) sets both fields from
`markerTypeVisibilitySource` (null-safe: null pointer means `markerTypeVisibility = nullptr`,
`markerTypeVisibilityRevision = 0`, i.e. today's exact unfiltered behavior).

### 4. Cache-invalidation revision combination

`MapCanvas_IconLayer_Draw_UI.cpp:110`, exactly as shown in "Ground truth" above — combine
`input.markerTypeVisibilityRevision` into the revision passed to `ShouldInvalidateIconLayerCache`.

### 5. Manual gate

`ResolveMarkersManual` (`MapCanvas_IconLayer_CullManual_UI.cpp`): immediately after entering the
per-group loop, before any transform is walked:
```cpp
if (input.markerTypeVisibility != nullptr && input.markerTypeVisibility->IsHidden(group.name)) continue;
```

### 6. Procedural gate + new ruleIndex->markerTypeName lookup

New per-frame (or AABB-cache-style-cached, gated the same revision way) lookup, built by walking
`recipe.markerRuleLayers` — mirror `SeedMarkerDomains`'s existing walk shape
(`Application_OverlaySetup_Seed_UI.cpp:53-63`) but keep the FULL `markerTypeName`, not just a
Spawn/non-Spawn bool. Ground the exact `ruleIndex` numbering in the real code (confirm it matches
`FlatMarkerRuleIndexBase`'s own numbering from STEP132, `ProceduralInstanceRuleIndex_UI.h` —
reuse that existing flat-index convention if it already produces the right numbering; do not invent
a second, possibly-inconsistent numbering scheme).

`ResolveProceduralSubLayer` (`MapCanvas_IconLayer_CullProcedural_UI.cpp`): gate using the lookup,
mirroring the manual gate's shape — skip the whole sub-layer's candidate walk when its resolved
`markerTypeName` is hidden.

## Verify

- Headless-frame test: the Hide/Unhide button's label toggles correctly on click; `SmallButton`'s own
  item rect sits right-aligned within the reserved header zone regardless of which label is current
  (measure both states, confirm the button's own right edge lands at the same X both times).
- `MarkerTypeVisibility_UI`: `SetHidden`/`IsHidden` round-trip; `revision` increments exactly once per
  actual state change (not on a no-op set-to-same-value, if that's cheap to guarantee — otherwise
  document that a redundant SetHidden still bumps revision, which is harmless, just an extra cache
  rebuild).
- **The cache-invalidation regression this ticket exists to prevent**: a synthetic two-frame test
  proving that toggling `MarkerTypeVisibility_UI`'s hidden state between frame 1 and frame 2, with
  the view/selection otherwise UNCHANGED, still causes `ShouldInvalidateIconLayerCache` to return
  true on frame 2 (i.e. the toggle alone, nothing else, triggers a rebuild).
- Manual gate: a fixture with two `MarkerInstanceGroup`s ("Alloys" and "Spawn"), hiding "Alloys"
  produces candidates only from the Spawn group.
- Procedural gate: a fixture with two `MarkerRuleLayer`s typed "Alloy" and "Plasma" respectively
  (same `MarkerCategory::Alloys` on both, to prove the category-alone ambiguity this ticket's own
  "Ground truth" section flags is real and correctly resolved by the new lookup, not accidentally
  working via category), hiding "Plasma" produces candidates only from the Alloy-typed rule.
- Null-safe: `markerTypeVisibility == nullptr` (no shell has wired the source — a test calling
  `ResolveMarkersManual`/`ResolveProceduralSubLayer` directly, or `DrawOverlayIconLayers` before
  `SetMarkerTypeVisibilitySource` was ever called) produces the SAME candidates as today, unfiltered.
- Full existing suites (`MapCanvas_IconLayer_Cull_UI_Test`, `MapCanvas_IconLayer_Cache_UI_Test`,
  `MapCanvas_IconLayer_Cull_OverlayDomainToggle_UI_Test`, `MarkersTab_UI_Test`) stay green.
