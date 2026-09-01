# STEP236 — `DrawDialCompact`: one-line radial-dial slider variant

**Layer:** UI. **Domain:** `LabelledDialWidget_UI.h/.cpp` only. **Sequence:** independent, fully
disjoint file set from STEP234/STEP235.

Ratifies `work_orders/DESIGN_MarkerLink_R1.md` §4.2. ARCH-approved (casual pass, confirmed).

## Session coordination

Check `ListAgents`/message peer sessions before touching `LabelledDialWidget_UI.h`/`.cpp`.

## Problem

`DrawLabelledDial` is a real, shipped knob widget but always draws a 2×radius-tall, two-line
(label+field+RT stack) layout — too tall for a single section-header row. No compact one-line
variant exists, unlike the linear slider's `DrawSliderScalar`/`DrawSliderScalarCompact` pair.

## Fix

New function in `LabelledDialWidget_UI.h/.cpp`, mirroring `DrawSliderScalarCompact`'s relationship
to `DrawSliderScalar` exactly:
```cpp
WidgetChange DrawDialCompact(const char* label, float& value, const DialRange& range,
                             RealtimeToggle& realtimeToggle, float fieldWidthPixels,
                             const WidgetStyle& style = WidgetStyle(), const char* valueFormat = "%.2f",
                             bool bShowRealtimeToggle = true);
```
One line: `PushID(label)` → knob (diameter = the row's own frame height via `style.dialRadius`) →
`SameLine()` → fixed-width `DragFloat` → optional RT button. No separate label line — use
`SetTooltip` on hover instead, mirroring the existing pattern at
`MarkersTab_ManualLayerRowBody_UI.cpp:90-91`. Reuse `DrawLabelledDial`'s existing arc/pointer draw
code (factor into a shared static helper if not already separable) — do not duplicate the drawing
logic.

## Verify

- Extend `LabelledDialWidget_UI_Test.cpp`: value commit via drag, RT-toggle interaction,
  `fieldWidthPixels` honored (measured item rect), and a one-line check (knob + field + RT button
  item rects share the same Y), mirroring STEP134's own single-line verification pattern.
- Existing `LabelledDialWidget_UI_Test` suite stays green.

## Out of scope

- Wiring this into the Markers-tab Type-section header — needs the still-ARCH-ratification-pending
  `scaleSelectedAlloy/Plasma/Spawn` PARAMS fields (`DESIGN_MarkerLink_R1.md` §4.3/§4.4). Separate,
  blocked ticket.
