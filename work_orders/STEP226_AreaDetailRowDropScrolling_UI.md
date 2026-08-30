# STEP226 — Area detail row: drop the horizontal-scroll wrapper (corrects STEP225)

## Summary
The human's correction over STEP225, verbatim: "Ther is no reason for scrolling, they should all
fit, the color needs no label, there should be a universal color picker that is used throught the
program. The name field can be small/short there should be plenty of room."

STEP225's own width analysis leaned on a unit-test window proxy (`ImVec2(300, ...)` in
`MarkersTab_Bundles_UI_Test.cpp`) to estimate the row's real available width — that was the wrong
reference. The actual docked "Generator Settings" window
(`Application_Draw_UI.cpp:17-29`/`Application_Settings_UI.h:30`) defaults to
`settingsWindowWidth = 700.0f` and is freely user-resizable; after the 190px left pane
(`leftPaneWidth`) and window padding, the Areas tab's real content width is several hundred pixels
by default and grows further if the human widens the window — nowhere near the ~260-300px figure
STEP225 sized the scroll-child against. The human is correct that there is no real need for a
scroll region.

The other two points in the human's message are ALREADY true and need no change, confirmed by
direct code reading: `ColorSwatchOptions::bLabelHidden` is already set for the Areas swatch
(STEP225), so the Color control already shows no label; and `DrawColorSwatch` is already the
single universal color-picker widget used by every tab in the program (stratum tints, team color,
markers, decals, props, and now areas) — there is no separate/duplicate color picker anywhere to
consolidate.

## Required reading
- `src/ui/AreasTab_UI.cpp` (current, post-STEP225 — full file, ~265 lines), specifically
  `DrawAreaSettings` (the function STEP225 just rewrote).
- `src/ui/AreasTab_UI.h` (current, post-STEP225 — full file) — the `kAreaDetailRow*` constants
  STEP225 added.
- `src/ui/AreasTab_UI_Test.cpp` — the `RunAreaDetailSingleLineRowAcceptanceChecks` test STEP225
  added, which currently asserts the row's controls land inside the `"##areaSettingsRow"` child
  region — this ticket removes that child, so the test's MECHANISM must change even though what it
  proves (all seven controls share one Y band, i.e. one real imgui line) does not.
- `src/ui/Application_Draw_UI.cpp:17-29`, `src/ui/Application_Settings_UI.h:30` — the real panel
  width this ticket's reasoning rests on.

## 1. `src/ui/AreasTab_UI.cpp` — remove the `BeginChild`/`EndChild` wrapper

In `DrawAreaSettings`, the current block (right after the `PlayableArea` disabled-text line):
```cpp
    // STEP225 — the human's own explicit instruction: Name, X, Z, W, L, Color and the map-size
    // button all on ONE line. Seven controls at their full, usable size do not fit this row's
    // normal ~260-300px content width, so the row draws inside its own single-row-tall,
    // horizontally-scrolling child (LuaCodeEditor_UI.cpp's own precedent for this exact
    // BeginChild(id, size, border, ImGuiWindowFlags_HorizontalScrollbar) shape) rather than
    // shrinking the sliders below a usable drag precision or silently wrapping to a second line.
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                        ImVec2(kAreaDetailRowItemSpacingPixels, ImGui::GetStyle().ItemSpacing.y));
    const float rowHeight = ResolveWidgetTrackHeight(WidgetStyle()) + kAreaDetailRowVerticalPaddingPixels;
    ImGui::BeginChild("##areaSettingsRow", ImVec2(0.0f, rowHeight), false,
                      ImGuiWindowFlags_HorizontalScrollbar);

    if (bIsPlayableArea) {
```
becomes:
```cpp
    // STEP226 — corrects STEP225: no scroll region. The docked settings window is several hundred
    // pixels wide by default and freely resizable (Application_Settings_UI.h's
    // settingsWindowWidth), so Name/X/Z/W/L/Color/"Map Size" fit on one plain SameLine-chained
    // line with no artificial child/scrollbar. Only the tightened ItemSpacing survives from
    // STEP225 — a real, harmless width saving, not the thing the human objected to.
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                        ImVec2(kAreaDetailRowItemSpacingPixels, ImGui::GetStyle().ItemSpacing.y));

    if (bIsPlayableArea) {
```
And the end of the function, currently:
```cpp
    ImGui::SameLine();
    if (ImGui::Button("Map Size")) bCommitted = SetAreaToMapSize(area, mapSize) || bCommitted;

    ImGui::EndChild();
    ImGui::PopStyleVar();
    return bCommitted;
}
```
becomes:
```cpp
    ImGui::SameLine();
    if (ImGui::Button("Map Size")) bCommitted = SetAreaToMapSize(area, mapSize) || bCommitted;

    ImGui::PopStyleVar();
    return bCommitted;
}
```
Nothing else in the function body (the Name/X/Z/W/L/Color controls themselves, the rename-retarget
block, the `bIsPlayableArea` branches) changes — STEP225's actual control composition and widths
were correct; only the wrapping child/scrollbar is removed.

## 2. `src/ui/AreasTab_UI.h` — drop the now-unused constant

`kAreaDetailRowVerticalPaddingPixels` (STEP225, used only to size the removed child's height) has
no remaining reader after step 1 — remove its declaration entirely (Constitution — no dead
settings left behind). `kAreaDetailRowItemSpacingPixels` stays; it is still used by the surviving
`PushStyleVar` call. `kAreaDetailNameFieldWidthPixels`, `kAreaDetailColorSwatchWidthPixels`, and
`AreasTabColorSwatchOptions()`'s `bLabelHidden`/`swatchWidth` all stay unchanged — none of those
were the human's objection.

## ARCH rules invoked
- Constitution §8 — a setting with no remaining reader is removed, not left as dead code.
- ARCH §3.2 — no change to widget ownership/state semantics; this ticket only removes a layout
  wrapper, not a widget behavior.

## Explicit out-of-scope
- No change to `TextInput_UI.h/.cpp`'s new `fixedWidthPixels` parameter (STEP225) — still needed,
  still correct, unrelated to the scrolling complaint.
- No change to `DrawSliderScalarCompact`'s track/field widths, `ColorSwatchOptions`, or the "Map
  Size" button's shortened label — none of those were the human's objection.
- No change to any other `BeginChild`/`ImGuiWindowFlags_HorizontalScrollbar` use elsewhere in the
  codebase (`LuaCodeEditor_UI.cpp`, `FilesTab_Draw_UI.cpp`'s log, etc.) — those are legitimate,
  unrelated uses of the same imgui mechanism for genuinely large/scrollable content, not touched
  by this ticket.

## Acceptance test
- `AreasTab_UI_Test.cpp`'s `RunAreaDetailSingleLineRowAcceptanceChecks` (STEP225) currently proves
  "one line" by finding controls inside the `"##areaSettingsRow"` child window — that child no
  longer exists after this ticket. Adapt the test's MECHANISM (search the row body's own draw
  list/item rects directly for a shared Y band across Name/an X-slider/the "Map Size" button)
  while preserving what it proves: all seven controls render on the same horizontal line. Do not
  delete the test's intent, only its now-obsolete child-window lookup.
- Full existing test suite: zero regressions.
- A real-window sanity check (if feasible without manual interaction, per this project's
  automated-verification-only rule): confirm no horizontal scrollbar renders under the Areas
  detail row at the settings window's default 700px width.

## Interpretation calls made
1. The tightened `ImGuiStyleVar_ItemSpacing` (4px, half default) is kept even though it was
   originally justified as part of the "make seven controls fit" argument for the scroll-child —
   it is still a legitimate width saving with no downside, and removing it would only make the row
   wider for no benefit now that scrolling itself is gone. If the human would rather have normal
   (8px) spacing on this row now that room is not scarce, that is a one-line revert
   (`kAreaDetailRowItemSpacingPixels`'s value, or dropping the `PushStyleVar`/`PopStyleVar` pair
   entirely) — flagged here rather than assumed silently either way.
