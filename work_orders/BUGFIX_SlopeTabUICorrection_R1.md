# Correct Slope Tab gradient editor: colors, degree-based stops, universal color picker, default blend

**Layer:** UI. **Executor:** SanGen Coder.

## Context

Current defaults (`MakeSlopeRamp()`, `src/ui/Application_PreviewRamps_UI.cpp:43-49`) are placeholder
greenish/yellowish/reddish colors at normalized locations 0.0/0.5/1.0, mapped to a 0-45° domain
(`SlopeTab_UI.h:47-48`). Per-stop editing uses raw `ImGui::ColorEdit4`/`SliderFloat` directly
(`GradientEditorWidget_Draw_UI.cpp:99-108`) instead of the existing universal color-picker widget
(`Ui::DrawColorSwatch`, `src/ui/ColorSwatch_UI.h`). The gradient editor widget is **shared** by 5 tabs
(Height, Water, Slope, Flow, Accumulation) — every change below that touches the shared widget must
be additive/optional, not a hardcoded Slope-only behavior change.

## Part 1 — new default Slope ramp values

`MakeSlopeRamp()`, `src/ui/Application_PreviewRamps_UI.cpp`, replace the 3 stops. Colors are stored
linear-float RGBA in [0,1] (`Params::GradientStop::color`) — convert the human's byte values (÷255):

```cpp
stops.push_back(MakeStop(0.0f,      0.090196f, 0.474510f, 0.043137f, 0.603922f)); // 0°,  green  23,121,11
stops.push_back(MakeStop(2.0f/90.0f, 1.0f,      1.0f,      0.0f,     0.603922f)); // 2°,  yellow 255,255,0
stops.push_back(MakeStop(30.0f/90.0f, 0.901961f, 0.149020f, 0.101961f, 0.603922f)); // 30°, red   230,38,26
```
All three alpha = 154/255 = 0.603922 (confirmed with the human — this supersedes the individually-stated
alphas in the original request).

`location` stays normalized [0,1] internally (unchanged field semantics, `GradientRamp_PARAMS.h:15-17`)
— the values above are `degrees / maximumDegrees` using the new 90° domain from Part 2.

## Part 2 — Slope domain becomes 0-90°, not 0-45°

`SlopeTabState` (`SlopeTab_UI.h:47-48`): change defaults `minimumDegrees=0` (unchanged),
`maximumDegrees` from `45` to `90`.

## Part 3 — stop-location editor shows degrees for Slope, stays normalized elsewhere

Verified before writing this: Slope is the only one of the 5 gradient tabs with a bounded real-world
unit for its stop locations. `WaterTabState`/`FlowTabState`/`AccumulationTabState` (`WaterTab_UI.h:67`,
`FlowTab_UI.h:53`, `AccumulationTab_UI.h:75`) each carry their own range fields (water level,
precipitation rate, spillover threshold) that are unrelated to their gradient's stop-location value —
there is no `HeightTabState` domain field either. For those four, a gradient stop's `location` is just
"where along the normalized 0-1 output" the color sits, with nothing to convert it to. Only Slope has an
existing bounded domain (`SlopeTabState::minimumDegrees/maximumDegrees`, `SlopeTab_UI.h:47-48`, already
used to convert degrees↔gradient for its own separate domain sliders). So this isn't a reversible
option/compatibility toggle — it's a per-call conversion that Slope needs and the other four structurally
don't have an equivalent for. `DrawSelectedStopControls` (`GradientEditorWidget_Draw_UI.cpp:99-108`)
currently edits `location` directly as a 0-1 slider; add a per-call display-conversion pair to the
widget's options struct (mirroring how `SlopeTab_UI.h` already converts degrees↔gradient for its own
domain sliders elsewhere in that file):

```cpp
struct GradientEditorOptions {
    // ... existing fields ...
    float (*ToDisplayUnits)(float normalizedLocation) = nullptr;   // nullptr = identity (0-1, existing behavior)
    float (*FromDisplayUnits)(float displayValue) = nullptr;       // nullptr = identity
    const char* locationSliderLabel = "Stop location";             // Slope passes "Stop location (deg)"
};
```
`DrawSelectedStopControls` calls `ToDisplayUnits`/`FromDisplayUnits` around the existing
`SliderFloat` (defaulting to identity, so Height/Water/Flow/Accumulation are byte-for-byte unchanged).
Slope's call site (`SlopeTab_UI.cpp:52`) passes `ToDisplayUnits = [](float n){ return n * 90.0f; }`,
`FromDisplayUnits = [](float d){ return d / 90.0f; }`, and the slider range becomes `0.0f..90.0f`
instead of `0.0f..1.0f` when the callbacks are set (widget clamps display range to `[ToDisplayUnits(0),
ToDisplayUnits(1)]` rather than a hardcoded `0..1`).

## Part 4 — remove "Slope Gradient" label (Slope only, other 4 tabs keep their label)

The `label` parameter of `DrawGradientEditor` (`GradientEditorWidget_Draw_UI.cpp:124-125`) is used both
for `ImGui::PushID(label)` scoping AND for the printed `ImGui::TextUnformatted(label)` — these must be
decoupled so removing the visible text doesn't break ID scoping for Slope's widget instance. Add
`GradientEditorOptions::bLabelHidden = false` (mirrors `ColorSwatchOptions::bLabelHidden`,
`ColorSwatch_UI.h`). `PushID(label)` always runs (ID scoping unaffected); `TextUnformatted(label)` is
skipped when `bLabelHidden` is true. Slope's call site (`SlopeTab_UI.cpp:52`) sets `bLabelHidden = true`;
Height/Water/Flow/Accumulation leave it `false` (unchanged, still show their label).

## Part 5 — per-stop color editing uses the universal color picker

`DrawSelectedStopControls` (`GradientEditorWidget_Draw_UI.cpp:99-103`): replace the raw
`ImGui::ColorEdit4("Stop color", color, ImGuiColorEditFlags_AlphaBar)` call with `Ui::DrawColorSwatch`
(`ColorSwatch_UI.h`), passing `ColorSwatchOptions{ bAlphaEnabled = true, bAlphaBarShown = true }` to
preserve today's alpha-editing capability. `GradientStop::color`'s 4-float linear RGBA layout is already
confirmed byte-compatible with `DrawColorSwatch`'s expected input (`ColorSwatch_UI.h:7-9`). This is a
shared-widget change affecting all 5 gradient-editor tabs — intentional; it replaces the same
raw-input-field pattern everywhere, not just Slope, consistent with using the universal widget wherever
a raw color field currently exists.

## Part 6 — default preview blend mode: Overlay

Slope layer construction, `Application_PreviewSetup_UI.cpp:73-74`:
```cpp
MakeFieldLayer(PreviewLayerKind::Slope, PreviewBlendMode::AlphaBlend, slopeRampRow, 0.0f, 1.0f, 1.0f)
```
Change `PreviewBlendMode::AlphaBlend` → `PreviewBlendMode::Overlay` (enum value confirmed present,
`PreviewComposite_Settings_UI.h:35-38`, index 8 of `previewBlendModeNames[]` used by the generic
per-layer blend-mode dropdown in `Application_ViewLayersPopup_UI.cpp:68-69`). No dropdown-side changes
needed — the control is already generic across all layers.

## Files touched
**Modified:** `Application_PreviewRamps_UI.cpp` (Slope ramp defaults), `SlopeTab_UI.h` (domain
0-90°), `SlopeTab_UI.cpp` (pass new `GradientEditorOptions` fields), `GradientEditorWidget_Draw_UI.cpp`
/ `.h` (add `GradientEditorOptions` fields, decouple label from PushID key, swap to `DrawColorSwatch`),
`Application_PreviewSetup_UI.cpp` (default blend mode), plus every test file exercising
`DrawGradientEditor`/`MakeSlopeRamp`/`SlopeTabState` defaults.

## Acceptance test
1. Fresh app load, Slope tab: gradient shows green→yellow→red at 0°/2°/30°, alpha 154 on all three,
   no "Slope Gradient" text label above the widget, default preview blend mode is Overlay.
2. Height/Water/Flow/Accumulation tabs: gradient editor unchanged — label still shown, stop-location
   slider still reads/writes 0-1, color editing now uses the universal picker (visual change, same
   underlying data).
3. Slope tab's stop-location slider displays and accepts values in the 0-90° range; dragging a stop
   to 45° stores `location = 0.5` internally (verify via the ramp's serialized value).
4. Moving a stop's color via the universal picker updates `GradientStop::color` identically to the old
   `ColorEdit4` path (same 4-float linear RGBA write).
