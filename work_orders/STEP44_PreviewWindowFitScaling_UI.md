# STEP44 — Map preview scales to fill its window

**Layer:** UI. **Domain:** Application shell / MapCanvas host.

## Problem
`Application::DrawCanvasWindow` (`src/ui/Application_Draw_UI.cpp:44-52`) draws the map preview
with `canvas.Draw("mapCanvas", settings.canvasRegionSidePixels)`, where
`canvasRegionSidePixels` is a **hardcoded 760.0f** (`Application_Settings_UI.h:32`). Resizing the
"Map Preview" window changes nothing — the canvas always draws at a fixed 760x760 square,
cropped or under-filling depending on the window's real size.

`MapCanvas::Draw` itself (`MapCanvas_Draw_UI.cpp:15`) is already size-agnostic — it just draws
whatever square side it's given. The bug is entirely in the caller never asking imgui for the
real available space.

## Fix
In `DrawCanvasWindow`, replace the hardcoded value with the window's actual current content
region, kept square (min of width/height) so the composite texture — itself always square,
`Resolution()` x `Resolution()` — never stretches non-uniformly:

```cpp
void Application::DrawCanvasWindow() {
    ImGui::Begin("Map Preview");
    if (ImGui::Button("Regenerate")) canvas.RequestRegeneration();
    ImGui::SameLine();
    if (canvas.HasSelection()) ImGui::Text("Selected entity: %u", canvas.SelectedEntityIdentifier());
    else                       ImGui::TextUnformatted("Selected entity: none");
    const ImVec2 available = ImGui::GetContentRegionAvail();
    const float  fittedSide = std::min(available.x, available.y);
    const float  regionSide = fittedSide > 0.0f ? fittedSide : settings.canvasRegionSidePixels;
    canvas.Draw("mapCanvas", regionSide);
    ImGui::End();
}
```

**⚠️ CORRECTED (this ticket's first version used `std::max`, which is wrong)**: the setting is a
fallback for a truly degenerate (`<= 0`) region only — the first frame, or before imgui's layout
stabilizes — never a continuous floor. `std::max` would clamp the canvas to never shrink below
760px, reproducing the exact "stays a fixed size, gets clipped" bug this ticket exists to fix,
just in the shrink direction. The canvas must be free to shrink and grow with the window in the
normal case. Don't delete the setting; it's the degenerate-frame fallback only.

This matches the codebase's existing idiom for computing available space
(`ImGui::GetContentRegionAvail()` is already used ~14 other places, e.g.
`Section_UI.cpp:46`, `GradientEditorWidget_Draw_UI.cpp:77`) — no new pattern introduced.

## Files touched
- `src/ui/Application_Draw_UI.cpp` — `DrawCanvasWindow`
- `src/ui/Application_Settings_UI.h` — comment on `canvasRegionSidePixels` only if renaming

## Verify
Full solo rebuild + `ctest -C Debug`, full suite green. This is a pure layout change with no
new testable pure-function logic (it reads live imgui window state), so no new test is expected
— note that explicitly rather than inventing one. Manual on-screen confirmation that the preview
now fills a resized window is the human's own job (see memory: no interactive testing by AI).
