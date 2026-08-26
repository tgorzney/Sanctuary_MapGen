# STEP118 — Compact Position X/Y/Z onto one line; RT-enabled by default throughout the Markers tab

**Layer:** UI (layout composition + per-struct state defaults). **Domain:** `MarkersTab_ManualInstance_UI.cpp` (Part A), five Markers-domain state structs across `MarkersTab_UI.h`, `MarkersTab_Globals_UI.h`, `MarkersTab_ManualLayers_UI.h`, `MarkersTab_Manual_UI.h`, `MarkersTab_Rules_UI.h` (Part B). **Sequence:** no dependency on other undone work-orders; both parts touch `MarkersTab_ManualInstance_UI.cpp`/its sibling headers, bundled here to avoid two tiny tickets colliding on the same file (same posture STEP106 documented for its own two-feature bundle).

## Part A — Position X/Y/Z compact to one line

### Problem
`DrawSelectedMarkerInstance`'s Position block (`src/ui/MarkersTab_ManualInstance_UI.cpp:124-136`, current live state post-STEP106) draws three full-width `DrawSliderScalar` calls stacked vertically:
```cpp
const bool bLayerLocked = IsMarkerInstanceLayerLocked(markerLayers, transform.layerIndex);
ImGui::BeginDisabled(bLayerLocked);
const WidgetChange positionXChange = DrawSliderScalar("Position X", transform.transform.positionX,
    state.positionHorizontalRange, state.positionXToggle, WidgetStyle(), "%.1f");
const WidgetChange positionYChange = DrawSliderScalar("Position Y (Elevation)", transform.transform.positionY,
    state.positionElevationRange, state.positionYToggle, WidgetStyle(), "%.1f");
const WidgetChange positionZChange = DrawSliderScalar("Position Z", transform.transform.positionZ,
    state.positionHorizontalRange, state.positionZToggle, WidgetStyle(), "%.1f");
if (positionXChange.bCommitted || positionZChange.bCommitted)
    QuantizeMarkerPositionToLayerGrid(markerLayers, transform.layerIndex,
                                      transform.transform.positionX, transform.transform.positionZ);
ImGui::EndDisabled();
bCommitted = positionXChange.bCommitted || positionYChange.bCommitted || positionZChange.bCommitted || bCommitted;
```
Each `DrawSliderScalar` reserves the label on its own line, then the track, then a numeric-field-plus-RT-button row (`SliderScalar_UI.cpp:34-53`) — a three-row block per axis, six rows total for X/Y/Z. Confirmed `DrawSliderScalar`/`ReserveScalarSliderTrack`/`ScalarSliderNumericFieldWidth` (`SliderScalar_Track_UI.cpp:10-19,48-52`) both size themselves from `ImGui::GetContentRegionAvail().x` unconditionally — there is no width parameter, no "compact"/"no-label-row" variant, anywhere in `SliderScalar_UI.h`, `SliderScalar_Track_UI.cpp`, or `SliderScalar_Integer_UI.cpp`. Three side-by-side `DrawSliderScalar` calls at full window width would overlap.

Checked Props/Decals (`PropsTab_Manual_UI.cpp:115-133`, `PropsTab_ManualDecals_UI.cpp`, its Decals twin) for the same pattern: their position fields are read-only display text inside `RenderVirtualRows` (`"%d: %.7s (%.1f, %.1f, %.1f) scale %.2f"`, `PropsTab_Manual_UI.cpp:126-130`) — already one compact line, not three separate editable sliders. No genuine three-slider-row parallel exists there to flag as a follow-up; this ticket does not invent one.

### Fix
No new widget-library primitive. Constrain each slider's own `GetContentRegionAvail()`-derived width by drawing all three inside an `ImGui::Columns(3, ...)` block — real, in-codebase precedent: `LayerEditor_Layer_UI.cpp:95-104`'s `DrawLayerEditorNameRow` puts a `DrawTextInput` and a `DrawLayerEditorIntegerRow` in two `ImGui::Columns(2, "nameStratumColumns", false)` cells for the identical reason (each control's own width call reads the column's available width, not the window's). Since `SliderScalar_Track_UI.cpp`'s two `GetContentRegionAvail()` calls are queried fresh inside whatever cell they're drawn in, no change to `SliderScalar_UI.h`/`.cpp` is needed.

Labels shorten to "X"/"Y"/"Z" (a bare "Position X" etc. would either overflow the narrowed column or visually orphan the word "Position" three times); a single `ImGui::TextUnformatted("Position")` line above the row keeps the group's meaning, replacing the per-axis label as the one place "Position" appears. `positionY`'s elevation semantics are unchanged — it still reads/writes `state.positionElevationRange` — only its on-screen label shortens from "Position Y (Elevation)" to "Y".

Replace `MarkersTab_ManualInstance_UI.cpp:124-136` with:
```cpp
const bool bLayerLocked = IsMarkerInstanceLayerLocked(markerLayers, transform.layerIndex);
ImGui::BeginDisabled(bLayerLocked);
ImGui::TextUnformatted("Position");
ImGui::Columns(3, "markerPositionColumns", false);
const WidgetChange positionXChange = DrawSliderScalar("X", transform.transform.positionX,
    state.positionHorizontalRange, state.positionXToggle, WidgetStyle(), "%.1f");
ImGui::NextColumn();
const WidgetChange positionYChange = DrawSliderScalar("Y", transform.transform.positionY,
    state.positionElevationRange, state.positionYToggle, WidgetStyle(), "%.1f");
ImGui::NextColumn();
const WidgetChange positionZChange = DrawSliderScalar("Z", transform.transform.positionZ,
    state.positionHorizontalRange, state.positionZToggle, WidgetStyle(), "%.1f");
ImGui::Columns(1);
if (positionXChange.bCommitted || positionZChange.bCommitted)
    QuantizeMarkerPositionToLayerGrid(markerLayers, transform.layerIndex,
                                      transform.transform.positionX, transform.transform.positionZ);
ImGui::EndDisabled();
bCommitted = positionXChange.bCommitted || positionYChange.bCommitted || positionZChange.bCommitted || bCommitted;
```
`ImGui::Columns(1)` resets the column state before `EndDisabled()`/the function's remaining code, matching `DrawLayerEditorNameRow`'s own `Columns(1)` reset at its end. The `"markerPositionColumns"` ID is scoped to whatever outer `PushID` the row-body lambda already applies (`DrawMarkerInstanceList`'s per-row `DraggableList` body, `MarkersTab_ManualInstance_UI.cpp:157-161`), so it cannot collide with another row's identical ID.

### Out of scope
- Any new "compact slider row" or "no-label" primitive in `SliderScalar_UI.h`/`.cpp` — not invented; `ImGui::Columns` composition at the call site is sufficient and precedented.
- Props/Decals instance editors — confirmed no equivalent editable three-slider pattern exists there today (their transform lists are read-only single-line labels); no follow-up needed.
- Any change to `positionHorizontalRange`/`positionElevationRange` values, `QuantizeMarkerPositionToLayerGrid`, or the lock-gate (`bLayerLocked`) — all STEP106 behavior, untouched.

## Part B — RT enabled by default throughout the Markers tab

### Problem
`RtToggleWidget_UI.h`'s `RealtimeToggle` defaults `bRealtimeEnabled = false` (`RtToggleWidget_UI.h:66`, own comment: "default OFF: cheap scrubbing is the safe default") — a GLOBAL widget default shared by every tab in the app (Heightmap, Water, Stratums, etc.). Changing that struct's own default would change behavior everywhere, which is not what was asked ("Marker UI" scoped explicitly). `RealtimeToggle`'s public constructor `explicit RealtimeToggle(bool bRealtimeEnabledInitially)` (`RtToggleWidget_UI.h:23`) lets a containing struct override the default per-member without touching the shared class.

Grepped every `RealtimeToggle`-typed field under `src/ui/MarkersTab_*.h` and `src/ui/MapCanvas_IconLayer_*.h` (the latter: zero matches — no Markers-domain `RealtimeToggle` lives there). Confirmed 23 fields across 5 structs, all currently default-constructed (RT off):

| Struct | File | Fields (line) |
|---|---|---|
| `MarkersTabState` | `MarkersTab_UI.h:47-100` | `slopeToggle` (62), `heightToggle` (63), `densityToggle` (64), `countToggle` (65), `clearanceSpacingToggle` (66), `obstacleDistanceToggle` (67) |
| `MarkerGlobalScaleRow` | `MarkersTab_Globals_UI.h:37-43` | `iconScaleToggle` (41), `previewColorToggle` (42) |
| `ManualMarkerLayersState` | `MarkersTab_ManualLayers_UI.h:38-72` | `groupColorToggle` (48), `layerIconScaleToggle` (49), `selectedLayerColorToggle` (54), `selectedLayerIconScaleToggle` (55), `selectedLayerGridSnapToggle` (56), `fixSymmetryToleranceToggle` (68) |
| `ManualMarkersState` | `MarkersTab_Manual_UI.h:46-` | `positionXToggle` (61), `positionYToggle` (62), `positionZToggle` (63) |
| `MarkerRuleDetailState` | `MarkersTab_Rules_UI.h:34-57` | `areaRadiusMinimumToggle` (47), `areaRadiusMaximumToggle` (48), `areaHeightRangeToggle` (49), `focusRadiusToggle` (50), `focusStrengthToggle` (51), `focusContrastToggle` (52) |

`MarkersTab_RuleLayers_UI.h` and `MarkersTab_Placed_UI.h` were also checked — neither declares a `RealtimeToggle` member; not touched.

### Fix
At each of the 23 declaration sites above, add the explicit-constructor override — the field's TYPE and NAME are unchanged, only its in-class initializer:
```cpp
RealtimeToggle slopeToggle{true};
RealtimeToggle heightToggle{true};
RealtimeToggle densityToggle{true};
RealtimeToggle countToggle{true};
RealtimeToggle clearanceSpacingToggle{true};
RealtimeToggle obstacleDistanceToggle{true};
```
(`MarkersTab_UI.h:62-67`), and identically for the other four structs' fields listed above — each bare `RealtimeToggle <name>;` becomes `RealtimeToggle <name>{true};`. `RealtimeToggle`'s own class definition (`RtToggleWidget_UI.h`) is NOT edited; its default-constructed value stays `false` for every non-Markers tab. This is a real member-initializer override (direct-list-initialization against the `explicit` one-arg constructor), not a hack — the same mechanism `RangeSliderWidget_UI.h`/callers elsewhere already use when they need a non-default `RealtimeToggle(true)` (confirmed the constructor is public and already exercised by `RtToggleWidget_UI_Test.cpp`'s own default-vs-explicit distinction).

### Out of scope
- `RealtimeToggle`'s own struct default (`RtToggleWidget_UI.h:66`) — stays `false`; every other tab (Heightmap, Water, Stratums, Armies, Props, Areas, Scenarios, System) is unaffected.
- Any `RealtimeToggle` living outside the five Markers-domain structs enumerated above — none found under `MapCanvas_IconLayer_*` or elsewhere in the Markers call chain; if a future ticket adds one, it inherits the class default (off) unless it also opts in explicitly.
- `MarkersTab_RuleLayers_UI.h`/`MarkersTab_Placed_UI.h` — confirmed no `RealtimeToggle` member; nothing to change.

## Files touched
- `src/ui/MarkersTab_ManualInstance_UI.cpp` — `DrawSelectedMarkerInstance`'s Position block: `ImGui::Columns(3, ...)` layout, shortened "X"/"Y"/"Z" labels, added "Position" text line (Part A)
- `src/ui/MarkersTab_UI.h` — `MarkersTabState`'s six toggles gain `{true}` (Part B)
- `src/ui/MarkersTab_Globals_UI.h` — `MarkerGlobalScaleRow`'s two toggles gain `{true}` (Part B)
- `src/ui/MarkersTab_ManualLayers_UI.h` — `ManualMarkerLayersState`'s six toggles gain `{true}` (Part B)
- `src/ui/MarkersTab_Manual_UI.h` — `ManualMarkersState`'s three toggles gain `{true}` (Part B)
- `src/ui/MarkersTab_Rules_UI.h` — `MarkerRuleDetailState`'s six toggles gain `{true}` (Part B)

## Verify

**Part A** — pure ImGui layout change with no new logic (no clamp, no mirror, no mask arithmetic added or changed); `QuantizeMarkerPositionToLayerGrid`/`IsMarkerInstanceLayerLocked`/the `bCommitted` OR-composition are UNCHANGED lines, just reflowed around the new `Columns` calls. There is nothing here a headless unit test can assert beyond what STEP106's existing `QuantizeMarkerPositionToLayerGrid`/`IsMarkerInstanceLayerLockedChecks` tests already cover — stating this explicitly rather than inventing a fake layout test, matching this project's honesty-about-coverage-gaps posture. Acceptance for Part A is the manual/visual check (Coder/human, not an agent, per the no-manual-testing-by-agents rule): confirm X/Y/Z sit on one row, the lock-disable still visually dims all three, and grid-snap still fires on commit.

**Part B** — new unit tests, one `RunRealtimeDefaultChecks()` per existing test binary that already includes the relevant header, using `RealtimeToggle::IsRealtimeEnabled()` (the same public accessor `RtToggleWidget_UI_Test.cpp:37` already uses to assert the OFF default):

- `src/ui/MarkersTab_UI_Test.cpp` (already includes `MarkersTab_UI.h`, transitively `MarkersTab_Globals_UI.h`/`MarkersTab_Rules_UI.h`) — add, registered in its existing `main()` alongside `RunRuleMirrorChecks()`/`RunEnumMirrorChecks()`/`RunSelectionFenceChecks()` (`MarkersTab_UI_Test.cpp:116-125`):
  ```cpp
  void RunRealtimeDefaultChecks() {
      MarkersTabState state;
      Check(state.slopeToggle.IsRealtimeEnabled() && state.heightToggle.IsRealtimeEnabled()
            && state.densityToggle.IsRealtimeEnabled() && state.countToggle.IsRealtimeEnabled()
            && state.clearanceSpacingToggle.IsRealtimeEnabled()
            && state.obstacleDistanceToggle.IsRealtimeEnabled(),
            "MarkersTabState's six rule-stack toggles default to realtime ON (STEP118)");
      Check(state.ruleDetail.areaRadiusMinimumToggle.IsRealtimeEnabled()
            && state.ruleDetail.areaRadiusMaximumToggle.IsRealtimeEnabled()
            && state.ruleDetail.areaHeightRangeToggle.IsRealtimeEnabled()
            && state.ruleDetail.focusRadiusToggle.IsRealtimeEnabled()
            && state.ruleDetail.focusStrengthToggle.IsRealtimeEnabled()
            && state.ruleDetail.focusContrastToggle.IsRealtimeEnabled(),
            "MarkerRuleDetailState's six toggles default to realtime ON (STEP118)");
      Check(state.globals.scaleRows[0].iconScaleToggle.IsRealtimeEnabled()
            && state.globals.scaleRows[0].previewColorToggle.IsRealtimeEnabled(),
            "MarkerGlobalScaleRow's two toggles default to realtime ON (STEP118)");
  }
  ```
- `src/ui/MarkersTab_ManualLayers_UI_Test.cpp` — add, registered alongside `RunIsMarkerInstanceLayerLockedChecks()`/`RunQuantizeMarkerPositionToLayerGridChecks()` (`MarkersTab_ManualLayers_UI_Test.cpp:84-91`):
  ```cpp
  void RunRealtimeDefaultChecks() {
      ManualMarkerLayersState state;
      Check(state.groupColorToggle.IsRealtimeEnabled() && state.layerIconScaleToggle.IsRealtimeEnabled()
            && state.selectedLayerColorToggle.IsRealtimeEnabled()
            && state.selectedLayerIconScaleToggle.IsRealtimeEnabled()
            && state.selectedLayerGridSnapToggle.IsRealtimeEnabled()
            && state.fixSymmetryToleranceToggle.IsRealtimeEnabled(),
            "ManualMarkerLayersState's six toggles default to realtime ON (STEP118)");
  }
  ```
- `src/ui/MarkersTab_Manual_UI_Test.cpp` — add, registered alongside the file's existing ten `Run*Checks()` calls (`MarkersTab_Manual_UI_Test.cpp:183-193`):
  ```cpp
  void RunRealtimeDefaultChecks() {
      ManualMarkersState state;
      Check(state.positionXToggle.IsRealtimeEnabled() && state.positionYToggle.IsRealtimeEnabled()
            && state.positionZToggle.IsRealtimeEnabled(),
            "ManualMarkersState's three position toggles default to realtime ON (STEP118)");
  }
  ```

**Existing suites stay green**: `RtToggleWidget_UI_Test.cpp` is untouched and keeps asserting the CLASS default is off (`RealtimeToggle realtimeToggle; Check(!realtimeToggle.IsRealtimeEnabled(), ...)`, line 36-37) — this ticket never contradicts that; it only overrides the default at 23 specific member-declaration sites, all outside `RtToggleWidget_UI.h` itself.
