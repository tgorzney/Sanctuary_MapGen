# Work-Order M5-1 — core input widgets (`RangeSlider`, `LabelledDial`, `RtToggle`)

*Constitution §7. Milestone M5. **BATCH 1 (parallel).** Own files, no dep on other M5
work-orders. Executor: SanGen Coder (imgui).*

## Root problem
`UI_FRAMEWORK_SPEC`: the UI is one shared widget library drawn with direct `ImDrawList`,
not per-tab hand-rolled imgui. Start with the three most-used controls.

## Target files
- `src/ui/RangeSliderWidget_UI.h/.cpp`, `LabelledDialWidget_UI.h/.cpp`,
  `RtToggleWidget_UI.h/.cpp` (+ any small shared `WidgetHelpers_UI.h`).

## Layer & accuracy
`UI`. Visual. imgui + `ImDrawList` custom drawing.

## Solution
- **RangeSlider** — dual-handle min/max float slider via `AddRectFilled` + two
  `InvisibleButton` hit-tests (the `UIHelpers.h` precedent), fully styleable.
- **LabelledDial** — a labelled scalar knob/field.
- **RtToggle** — the per-control "realtime" wrapper: while off, dragging updates the value
  but **defers** the expensive recompute until mouse-release (trips `bNeedsPreviewRender`
  or `bNeedsMapUpdate` only then). Keeps FPS high during scrubbing.
Every control returns "changed?" + the value; none owns app state. Consistent styling.

## Acceptance
Headless logic tests where feasible (range clamping, RT defer semantics: value updates
during drag, "commit" signal fires only on release); a manual/screenshot check of
rendering is acceptable for the draw path. Builds clean; files within ceilings.

## Out of scope
List/gradient/icon widgets (M5-2/M5-3); tabs (M5-6).
