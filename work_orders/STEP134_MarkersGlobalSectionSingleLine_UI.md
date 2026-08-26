# STEP134 — Global section: genuine single line per type, + select-color swatch

**Layer:** UI. **Domain:** `SliderScalar_UI.h/.cpp`, `SliderScalar_Track_UI.h/.cpp`,
`MarkersTab_Globals_UI.h/.cpp`, `ColorSwatch_UI.h`. **Sequence:** independent — no dependency on any
other Round-2 ticket, fully disjoint file set.

Ratifies `work_orders/DESIGN_MarkersUICorrectionRound2_R1.md` items 1+14.

## Problem

**Confirmed root cause:** `DrawGlobalScaleRow` (`MarkersTab_Globals_UI.cpp:61-79`) has zero
`ImGui::SameLine()` calls; the category label sits on its own line above a 3-column block, and the
tallest column (`DrawSliderScalar`) is itself 3 lines tall (label / track / value+toggle) because
`ReserveScalarSliderTrack` (`SliderScalar_Track_UI.cpp:10-19`) always claims
`GetContentRegionAvail().x` with nothing SameLine'd after it. Net ~4 lines per type, not 1.

Also: `selectColorAlloy/Plasma/Spawn` (STEP124) have no editing control anywhere in the UI (item 14).

## Fix

### 1. New compact slider entry point — `SliderScalar_UI.h/.cpp`

`DrawSliderScalar`/`DrawSliderScalarInteger` stay byte-identical — every other caller depends on the
3-line shape. Add a new function in the same file:

```cpp
WidgetChange DrawSliderScalarCompact(const char* label, float& value, const ScalarSliderRange& range,
                                      RealtimeToggle& realtimeToggle, float trackWidthPixels,
                                      float fieldWidthPixels, const WidgetStyle& style = WidgetStyle(),
                                      const char* valueFormat = "%.2f");
```

Composition: `PushID(label)` → reserve the track at the caller-supplied fixed width → `SameLine()` →
narrow `DragFloat` at `fieldWidthPixels` → `SameLine()` → the existing small RT button
(`style.realtimeButtonWidth`, 30px) → paint the track. No visible label line — instead
`if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", label);` on the track item, mirroring the
already-shipped pattern at `MarkersTab_ManualLayerRowBody_UI.cpp:90-91`.

**Two small, default-preserving additions to `SliderScalar_Track_UI.h/.cpp`:**
- `ReserveScalarSliderTrack(style, requestedWidthPixels = 0.0f)` — `0` keeps today's "rest of the
  line" behavior (existing callers unaffected); `>0` uses a fixed width. Mirrors
  `ColorSwatchOptions::swatchWidth`'s exact `<=0`/`>0` convention (`ColorSwatch_UI.h:27-28`).
- The anonymous-namespace `DrawFloatFieldRow` (`SliderScalar_UI.cpp:15-30`) gains the same
  `<=0`/`>0` field-width override — stays local to this TU, no header promotion needed.

### 2. Global row composition — `MarkersTab_Globals_UI.cpp`

One `ImGui::SameLine()`-chained row per type, no `ImGui::Columns`:

`[Icon Button] [Label text] [Compact scale slider] [Normal-color swatch, label hidden] [Select-color swatch, label hidden]`

**Width budget — measure against the live panel, do not assume it fits.** Rough estimate (icon
32-48px / label ~50px / track ~90px / field ~40px / RT 30px / swatch 20px ×2 / RT 30px ×2 + spacing)
lands near 350-380px. If it overflows the actual Markers-tab panel width, shrink the icon button
(48px→24-32px) and the compact track width FIRST, before touching anything else in the row. Confirm
final widths empirically, do not hardcode from this estimate.

### 3. Select-color field threading — `MarkersTab_Globals_UI.h`

`GlobalMarkerScaleRowFields` (`.h:70-89`) gains one more pointer, mirroring `color`:
```cpp
float* selectColor = nullptr;   // 4 floats: selectColorAlloy/Plasma/Spawn (GlobalMarkerSettings_PARAMS.h)
```
`ResolveGlobalMarkerScaleRowFields`'s 3-case switch gains `settings.selectColorAlloy/Plasma/Spawn` in
each branch (`selectColorDefault` is never resolved to by this per-type row — it's the resolver's
own unmatched-name fallback, not a 4th row).

`MarkerGlobalScaleRow` (`.h:41-51`) gains a third `RealtimeToggle selectColorToggle{true};` — kept
INDEPENDENT of `iconScaleToggle`/`previewColorToggle` (do not merge toggles; each field keeps its own
RT-tweakability, matching the struct's existing two-separate-toggles convention).

Both color swatches use `DrawColorSwatch(..., ColorSwatchOptions{.bLabelHidden = true, .swatchWidth = <small fixed px>}, ...)` — the already-shipped pattern at `MarkersTab_ManualLayerRowBody_UI.cpp:94-100`.

## Verify

- New headless-frame test (mirroring `MarkersTab_ManualLayerColorOverrideHeader_UI_Test.cpp`'s
  harness): render one Global row, assert every one of the 5 controls' item rects share the same Y
  (genuinely one line) and are laid out left-to-right in the stated order with no overlap.
- `DrawSliderScalarCompact`: extend/add to `SliderScalar_UI_Test.cpp` — value commit, RT-toggle
  interaction, and a check that `trackWidthPixels`/`fieldWidthPixels` are actually honored (measured
  item rect widths, not just "it compiles").
- `ReserveScalarSliderTrack(style, 0.0f)` behavior is confirmed byte-identical to the pre-change
  function (regression proof for every existing `DrawSliderScalar`/`DrawSliderScalarInteger` caller —
  do NOT just trust the default-parameter reasoning, assert it against at least one existing caller's
  test).
- `ResolveGlobalMarkerScaleRowFields`: extend existing coverage (`MarkersTab_GlobalScaleRowFields_UI_Test.cpp`)
  to assert `selectColor` resolves to the correct field per type.
- Existing suites (`MarkersTab_UI_Test`, `SliderScalar_UI_Test`, `ColorSwatch_UI_Test`,
  `MarkersTab_GlobalScaleRowFields_UI_Test`, `ApplicationShell_IconBridge_UI_Test`) stay green.

## Out of scope

- Everything else in `BRIEF_MarkersUICorrectionRound2_R1.md` — separately ticketed (STEP128-133).
