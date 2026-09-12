# STEP267 — Slope tab defaults: Steep Angle 30°, yellow ramp stop to 9°

**Layer:** UI. **Domain:** `src/ui/Application_PreviewSetup_UI.cpp`, `src/ui/Application_PreviewRamps_UI.cpp`.
**Executor:** SanGen Coder. **Sequence:** independent, no dependencies. Human-requested default value
change, verified against the current tree before writing this ticket.

## 0. What's being asked, and current ground truth

Human request: "Step Angle" (the Slope tab's **Steep Angle** slider,
`SlopeTab_UI.cpp:26-27` — `DrawSliderScalar("Steep Angle", state.maximumDegrees, ...)`) should
default to **30°**; the Slope Gradient ramp's **red** stop should be at **30°**; the **yellow** stop
should be at **9°**.

Verified current state:
- **Steep Angle default**: `Application_PreviewSetup_UI.cpp:73-74` constructs the Slope field layer
  with `domainMaximum = 1.0f` (gradient-magnitude units, i.e. `tan(degrees)`). Round-tripped through
  `SlopeDegreesFromGradient`/`SlopeGradientFromDegrees` (`SlopeTab_UI.h:29-38`, `atan`/`tan` pair,
  self-inverse), `1.0f` corresponds to **45°** (`tan(45°) == 1.0` exactly) — **not** 90° as the
  adjacent code comment (`:69-72`) claims; that comment is stale relative to the actual conversion
  functions and is corrected below as part of this ticket's own edit, not treated as ground truth.
- **Red stop**: `Application_PreviewRamps_UI.cpp:48` — `MakeStop(30.0f/90.0f, ...)` — **already 30°**,
  matches the request exactly. No change needed here.
- **Yellow stop**: `Application_PreviewRamps_UI.cpp:47` — `MakeStop(2.0f/90.0f, ...)` — currently
  **2°**, needs to become **9°**.

## 1. `src/ui/Application_PreviewRamps_UI.cpp` — yellow stop to 9°

Line 47, change the location argument and its inline comment:
```cpp
ramp.stops.push_back(MakeStop(9.0f/90.0f, 1.0f, 1.0f, 0.0f, 0.603922f)); // 9 deg, yellow
```
(was `2.0f/90.0f`, `// 2 deg, yellow`). The color/alpha arguments and the red stop
(`30.0f/90.0f`, line 48, already correct) are unchanged. The ramp's own 0-90° location domain
(`SlopeTab_UI.cpp:58-59`'s `ToDisplayUnits`/`FromDisplayUnits`) is a fixed axis independent of the
Steep Angle domain default below — do not conflate the two; only the stop's own position on that
fixed axis moves.

## 2. `src/ui/Application_PreviewSetup_UI.cpp` — Steep Angle default to 30°

Add `#include "SlopeTab_UI.h"` (for `SlopeGradientFromDegrees`) alongside the existing includes
(`:13-14`). Change line 74's `domainMaximum` argument from the magic `1.0f` to the named conversion,
consistent with how every other consumer of this domain already converts through the same function
rather than hardcoding a `tan()` value:
```cpp
previewSettings.fieldLayers.push_back(MakeFieldLayer(
    PreviewLayerKind::Slope, PreviewBlendMode::Overlay, slopeRampRow, 0.0f,
    SlopeGradientFromDegrees(30.0f), 1.0f));
```
Update the preceding comment (`:69-72`), which currently and incorrectly states "0..1 is 0..90
degrees" — correct it to describe the actual default and the real conversion:
```cpp
// The slope domain is gradient magnitude (rise/run), the pinned unit — SlopeGradientFromDegrees/
// SlopeDegreesFromGradient (SlopeTab_UI.h) convert to/from the Steep Angle slider's degrees. Default
// ceiling is 30 degrees (STEP267), matching the Slope Gradient ramp's red "hazard" stop
// (Application_PreviewRamps_UI.cpp). Overlay (not AlphaBlend) is the default blend so the ramp reads
// as a terrain tint rather than a flat paint-over (WO BUGFIX_SlopeTabUICorrection_R1 Part 6).
```
`domainMinimum` (`0.0f` = Flat Angle default, 0°) is unchanged — only the maximum moves.

## 2b. Optional but recommended — fix the stale comment's root cause note

This ticket does not change `SlopeTabState::maximumDegrees`'s own struct-literal default
(`SlopeTab_UI.h:48`, `= 90.0f`) — that field is a mirror overwritten from `layer.domainMaximum` on
the very first frame (`LoadSlopeTabValues`, called unconditionally unless a slider commit is
mid-drag, `SlopeTab_UI.cpp:18-19`), so its struct-literal value is never actually shown to a user and
is not part of this bug. Leave it as-is; do not touch `SlopeTab_UI.h`.

## 3. Tests

1. A freshly-constructed `PreviewCompositeSettings` (via `ConfigureDefaultPreview`) has its Slope
   field layer's `domainMaximum` equal to `SlopeGradientFromDegrees(30.0f)` (≈`0.57735f`), and
   round-tripping it through `SlopeDegreesFromGradient` recovers `30.0f` within the existing test
   tolerance (mirror `SlopeTab_UI_Test.cpp`'s existing `NearlyEqual` pattern, e.g. `1.0e-2f`).
2. `MakeSlopeRamp()`'s returned ramp has exactly 3 stops in order: green at `0.0f`, yellow at
   `9.0f/90.0f`, red at `30.0f/90.0f` — update/extend whatever existing test (if any) already checks
   `MakeSlopeRamp`'s stop positions; add one if none exists.
3. Regression: every other field layer's defaults constructed by `ConfigureDefaultPreview`
   (HeightRamp, StratumSplat, Water, Flow, Accumulation, MapAreas) are unchanged by this ticket.

## 4. Out of scope

- Any change to the ramp's fixed 0-90° stop-location axis or `GradientEditorOptions` conversion
  lambdas (`SlopeTab_UI.cpp:57-61`) — untouched.
- Any change to `SlopeTabState`'s own struct-literal defaults (`SlopeTab_UI.h:47-48`) — see §2b.
- Any change to the Flat Angle default (`domainMinimum`, stays `0.0f`/0°).
- Any change to `MakeWaterDepthRamp`/`MakeFlowRamp`/`MakeAccumulationRamp` or any other ramp.

## 5. Files touched

**Modified:** `src/ui/Application_PreviewRamps_UI.cpp`, `src/ui/Application_PreviewSetup_UI.cpp`, plus
whichever existing test file(s) cover `ConfigureDefaultPreview`/`MakeSlopeRamp` defaults (confirm exact
file — likely `Application_PreviewSetup_UI_Test.cpp` and/or `Application_PreviewRamps_UI_Test.cpp` if
either exists; check before starting).
