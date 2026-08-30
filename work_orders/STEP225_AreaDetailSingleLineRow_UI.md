# STEP225 — Area detail row: Name/X/Z/W/L/Color/Map-Size on ONE line (corrects STEP221)

## Summary
The human's own correction, verbatim: "I said I wated all the inputs ona single line Name, X, Z,
W, L, Color and the set to map size button for areas." STEP221 (shipped) put Name on its own
full-width line, X+Z on one compact line, W+L on another, Color on its own line, and "Set to Map
Size" on its own line — five lines, not the one the human actually asked for. This ticket corrects
that: all seven controls (Name, X, Z, W, L, Color, the map-size button) belong on one imgui line.

Ruled by the SanGen UI Expert (this session), after reading `TextInput_UI.h/.cpp`,
`ColorSwatch_UI.h/.cpp`, `SliderScalar_UI.h/.cpp`, `SliderScalar_Track_UI.cpp`, the current shipped
`AreasTab_UI.h/.cpp`, and the codebase's own established "typical docked panel" width
(`ImVec2(300, ...)` in `MarkersTab_Bundles_UI_Test.cpp` et al.):

**Blocking widget gap, now fixed.** `DrawTextInput` unconditionally calls
`ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x)` — it always claims the ENTIRE
remaining line width, with no way for a caller to override it from outside. This is the actual
reason STEP221 gave Name its own line instead of chaining it. This ticket adds a `fixedWidthPixels`
parameter, following the exact "0/default preserves prior behavior, positive value fixes it"
convention `ColorSwatchOptions::swatchWidth` and `DrawSliderScalarCompact`'s own
`trackWidthPixels`/`fieldWidthPixels` already established — strictly additive, the ~15 other
`DrawTextInput` call sites across the codebase pass 3-5 positional args today and none can break.

**The honest width problem, and why this ticket uses a scrolling row, not shrunk sliders.** Seven
controls at their tightest STILL-USABLE sizes (56px slider tracks — narrower makes the absolute-
position drag scrub too coarse to be usable, confirmed against `SliderScalar_Track_UI.cpp`'s
linear cursor-to-value mapping — plus numeric fields, short labels, a small color chip, and a
button) total roughly 600px. The row's real available width, inside `DraggableList`'s Collapsible
row body (`ImGui::Indent()`'d) at the panel widths this codebase's own tests already use, is
roughly 260-300px. That is not a rounding gap; shrinking the sliders further would make them
imprecise rather than merely tight. Rather than silently breaking the literal "one line" ask (by
wrapping) or degrading the sliders STEP221 just tuned, this ticket draws the row inside its own
single-row-tall, horizontally-scrolling child region (`ImGuiWindowFlags_HorizontalScrollbar`,
already an established pattern in this codebase — `LuaCodeEditor_UI.cpp:87` uses the exact same
`BeginChild(id, size, border, ImGuiWindowFlags_HorizontalScrollbar)` shape). Every control stays at
full, precise size, all seven sit on one true imgui line, and the ONLY visible cost is a scrollbar
under this one dense row if the panel is narrower than the row's natural width — the surrounding
tab/panel itself is untouched.

## Required reading
- `src/ui/TextInput_UI.h` / `.cpp` (full files)
- `src/ui/ColorSwatch_UI.h:24-37` (`ColorSwatchOptions`, especially `bLabelHidden`/`swatchWidth`)
- `src/ui/AreasTab_UI.h` (current, post-STEP223 — full file)
- `src/ui/AreasTab_UI.cpp` (current, post-STEP223 — full file, especially `DrawAreaSettings`)
- `src/ui/RtToggleWidget_UI.h:124` (`ResolveWidgetTrackHeight` — used to size the child region)
- `src/ui/LuaCodeEditor_UI.cpp:87` — the exact `BeginChild(...)` call shape to mirror (confirms
  this imgui build accepts the legacy `(id, size, bool border, ImGuiWindowFlags flags)` overload;
  do not use the newer `ImGuiChildFlags` enum form for this ticket, to match this precedent
  byte-for-byte rather than mixing both styles in the same file).
- `src/ui/IconGridWidget_Draw_UI.cpp:73-75` — the existing precedent for scoping a tightened
  `ImGuiStyleVar_ItemSpacing` around one dense region only, popped before returning to normal
  spacing (mirror this for the new row's `SameLine` gaps).

## 1. `src/ui/TextInput_UI.h` — add `fixedWidthPixels`

Change the declaration (current lines 101-104):
```cpp
WidgetChange DrawTextInput(const char* label, std::string& value,
                           const TextInputRules& rules = TextInputRules(),
                           const WidgetStyle& style = WidgetStyle(), const char* hintText = nullptr,
                           bool bLabelHidden = false);
```
to:
```cpp
WidgetChange DrawTextInput(const char* label, std::string& value,
                           const TextInputRules& rules = TextInputRules(),
                           const WidgetStyle& style = WidgetStyle(), const char* hintText = nullptr,
                           bool bLabelHidden = false, float fixedWidthPixels = 0.0f);
```
Add one line to the doc comment directly above (after the existing `bLabelHidden` explanation):
"`fixedWidthPixels` (default 0.0f, every existing call site unchanged): <= 0 keeps today's 'fill
remaining content width' behavior; a positive value fixes the box's own width instead, mirroring
`ColorSwatchOptions::swatchWidth` — the seam that lets a caller sit this control beside others via
`SameLine()` instead of always claiming the rest of the line."

## 2. `src/ui/TextInput_UI.cpp` — one line changes

The function definition (current line 34-35) gains the parameter:
```cpp
WidgetChange DrawTextInput(const char* label, std::string& value, const TextInputRules& rules,
                           const WidgetStyle& style, const char* hintText, bool bLabelHidden,
                           float fixedWidthPixels) {
```
Line 41, currently `ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);`, becomes:
```cpp
ImGui::SetNextItemWidth(fixedWidthPixels > 0.0f ? fixedWidthPixels : ImGui::GetContentRegionAvail().x);
```
Nothing else in this file changes.

## 3. `src/ui/AreasTab_UI.h` — new width/spacing constants, and swatch options

Add one constant BEFORE `AreasTabColorSwatchOptions()` (it needs this value):
```cpp
// STEP225 — the Color swatch's fixed width in the single-line Area detail row.
inline constexpr float kAreaDetailColorSwatchWidthPixels = 24.0f;
```
Extend `AreasTabColorSwatchOptions()` with two more lines:
```cpp
inline ColorSwatchOptions AreasTabColorSwatchOptions() {
    ColorSwatchOptions options;
    options.bAlphaEnabled         = true;
    options.bAlphaBarShown        = true;
    options.bRealtimeToggleHidden = true;   // STEP221 — area color is always realtime, no choice
    options.bLabelHidden          = true;   // STEP225 — chained inline, no room for a text label
    options.swatchWidth           = kAreaDetailColorSwatchWidthPixels;   // STEP225 — fits one line
    return options;
}
```
Add three more constants beside the existing `kAreaScalarCompactTrackWidthPixels`/
`kAreaScalarCompactFieldWidthPixels`/`kAreaCenterButtonWidthPixels` block (values unchanged for the
two STEP221 constants — do NOT shrink them, per this ticket's own reasoning above):
```cpp
// STEP225 — the single-line Area detail row's remaining fixed widths/spacing.
inline constexpr float kAreaDetailNameFieldWidthPixels     = 70.0f;
inline constexpr float kAreaDetailRowItemSpacingPixels     = 4.0f;   // half the default 8px, this row only
inline constexpr float kAreaDetailRowVerticalPaddingPixels = 8.0f;   // room for the child region's own frame
```

## 4. `src/ui/AreasTab_UI.cpp` — rewrite `DrawAreaSettings`'s body

Replace the ENTIRE function body (from `const ScalarSliderRange originRange = ...` through the
final `return bCommitted;`, i.e. everything currently between the signature and the closing brace)
with:
```cpp
bool DrawAreaSettings(Params::MapArea& area, AreasTabState& state, int mapSize,
                      std::vector<AreaColorEntry>& areaColors,
                      std::vector<AreaVisibilityEntry>& areaVisibility) {
    const ScalarSliderRange originRange = AreaOriginSliderRange(mapSize);
    const ScalarSliderRange extentRange = AreaExtentSliderRange(mapSize);
    bool bCommitted = false;
    const bool bIsPlayableArea = IsPlayableArea(area);
    if (bIsPlayableArea)
        ImGui::TextDisabled("PlayableArea is required by the engine: it cannot be renamed or removed.");

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
        ImGui::BeginDisabled();
        DrawTextInput("Name", area.name, TextInputRules(), WidgetStyle(), nullptr,
                     /*bLabelHidden=*/true, kAreaDetailNameFieldWidthPixels);
        ImGui::EndDisabled();
    } else {
        // Captured BEFORE the edit: if the name commits to something new, the color, lock, AND
        // visibility entries keyed on the OLD name must all be retargeted, or a rename silently
        // reverts the area's color to default, its lock to LOCKED, and its visibility to VISIBLE
        // next frame (STEP21 ruling #5 for color; STEP212/STEP223 extend the same repair to lock
        // and visibility).
        const std::string nameBeforeEdit = area.name;
        TextInputRules nameRules;
        nameRules.maximumLength = 48;
        nameRules.bAllowEmpty   = false;
        nameRules.fallbackText  = "Area";
        bCommitted = DrawTextInput("Name", area.name, nameRules, WidgetStyle(), nullptr,
                                   /*bLabelHidden=*/true, kAreaDetailNameFieldWidthPixels).bCommitted;
        if (bCommitted && area.name != nameBeforeEdit) {
            for (AreaColorEntry& entry : areaColors)
                if (entry.name == nameBeforeEdit) { entry.name = area.name; break; }
            for (AreaLockEntry& entry : state.areaLocks)
                if (entry.name == nameBeforeEdit) { entry.name = area.name; break; }
            for (AreaVisibilityEntry& entry : areaVisibility)
                if (entry.name == nameBeforeEdit) { entry.name = area.name; break; }
        }
    }
    ImGui::SameLine();

    ImGui::TextUnformatted("X");
    ImGui::SameLine();
    bCommitted = DrawSliderScalarCompact("X Position", area.originX, originRange, state.originXToggle,
                                         kAreaScalarCompactTrackWidthPixels,
                                         kAreaScalarCompactFieldWidthPixels, WidgetStyle(), "%.0f",
                                         /*bShowRealtimeToggle=*/false).bCommitted || bCommitted;
    ImGui::SameLine();
    ImGui::TextUnformatted("Z");
    ImGui::SameLine();
    bCommitted = DrawSliderScalarCompact("Z Position", area.originZ, originRange, state.originZToggle,
                                         kAreaScalarCompactTrackWidthPixels,
                                         kAreaScalarCompactFieldWidthPixels, WidgetStyle(), "%.0f",
                                         /*bShowRealtimeToggle=*/false).bCommitted || bCommitted;
    ImGui::SameLine();
    ImGui::TextUnformatted("W");
    ImGui::SameLine();
    bCommitted = DrawSliderScalarCompact("Width", area.width, extentRange, state.widthToggle,
                                         kAreaScalarCompactTrackWidthPixels,
                                         kAreaScalarCompactFieldWidthPixels, WidgetStyle(), "%.0f",
                                         /*bShowRealtimeToggle=*/false).bCommitted || bCommitted;
    ImGui::SameLine();
    ImGui::TextUnformatted("L");
    ImGui::SameLine();
    bCommitted = DrawSliderScalarCompact("Length", area.length, extentRange, state.lengthToggle,
                                         kAreaScalarCompactTrackWidthPixels,
                                         kAreaScalarCompactFieldWidthPixels, WidgetStyle(), "%.0f",
                                         /*bShowRealtimeToggle=*/false).bCommitted || bCommitted;
    ImGui::SameLine();

    // ARCH §14.17 item 10 / §14.18 item 16 — PlayableArea is always Green and non-editable: re-pin
    // its color before drawing and disable the control so a designer cannot pick a different one.
    float* const color = ResolveAreaColor(areaColors, area.name);
    if (bIsPlayableArea) {
        color[0] = kPlayableAreaColor[0]; color[1] = kPlayableAreaColor[1];
        color[2] = kPlayableAreaColor[2]; color[3] = kPlayableAreaColor[3];
        ImGui::BeginDisabled();
        DrawColorSwatch("Color", color, state.colorOptions, state.colorToggle);
        ImGui::EndDisabled();
    } else {
        bCommitted = DrawColorSwatch("Color", color, state.colorOptions,
                                     state.colorToggle).bCommitted || bCommitted;
    }
    ImGui::SameLine();
    if (ImGui::Button("Map Size")) bCommitted = SetAreaToMapSize(area, mapSize) || bCommitted;

    ImGui::EndChild();
    ImGui::PopStyleVar();
    return bCommitted;
}
```
Note the button's visible label shortens from `"Set to Map Size"` to `"Map Size"` to help it fit —
its behavior (`SetAreaToMapSize`) and ID are unchanged; only the on-screen text is shorter.

`DrawAreaList`'s existing call site (`bAreasMoved = DrawAreaSettings(area, state, mapSize,
areaColors, areaVisibility) || bAreasMoved;`) needs NO change — the signature above is identical to
what STEP223 already shipped.

## ARCH rules invoked
- Constitution §8 — every new pixel width/spacing value is a named constant, not a literal at the
  draw site.
- UI_FRAMEWORK_SPEC "Universal widget library" — `DrawTextInput`'s new parameter follows the exact
  precedent `ColorSwatchOptions::swatchWidth`/`DrawSliderScalarCompact`'s own added parameters
  already set (0/default = unchanged prior behavior).
- ARCH §3.2 — the new `fixedWidthPixels` parameter changes only how wide the control draws; the
  widget still owns no app state and still returns a plain `WidgetChange`.

## Explicit out-of-scope
- No shrinking of `kAreaScalarCompactTrackWidthPixels`/`kAreaScalarCompactFieldWidthPixels` below
  their STEP221 values (56.0f/44.0f) — the UI Expert's ruling is explicit that going narrower
  trades away real drag precision, and the scrolling child avoids needing that trade.
- No change to the Area Stack row's header (the `[o]`/`[U]`/`X` strip, the "Center" button, the row
  label/expand arrow) — this ticket touches only `DrawAreaSettings`'s own inline body.
- No change to any other tab's `DrawTextInput` call site — the new parameter defaults to 0.0f,
  byte-identical to today's behavior, everywhere it is not explicitly passed.
- No attempt to make the row fit WITHOUT a horizontal scrollbar at every possible panel width —
  the human asked for one line, not for a specific minimum panel width; if the docked panel is
  ever widened to ~600px+ for this tab, the scrollbar simply never activates (imgui's own
  child-region behavior, no extra code needed for that case).

## Acceptance test
- `AreasTab_UI_Test.cpp`: any existing test asserting the OLD multi-line layout, or the OLD button
  label `"Set to Map Size"`, needs updating to the new one-line shape / `"Map Size"` label — adapt,
  don't weaken.
- A new headless-imgui check (extending whatever click-sweep harness STEP222/STEP223 already
  built): confirm all seven controls — Name field, X, Z, W, L, Color swatch, Map Size button —
  render inside the SAME `BeginChild`/`EndChild` region (same Y position, i.e. genuinely one line,
  not visually stacked) for a non-PlayableArea row.
- A `TextInput_UI_Test.cpp` (or wherever `DrawTextInput` is already unit-tested) case: passing a
  positive `fixedWidthPixels` does not change the field's committed VALUE behavior (typing/commit
  semantics unaffected — only the rendered width changes, which is not itself assertable headless,
  so assert instead that `fixedWidthPixels`'s default `0.0f` produces byte-identical behavior to
  calling the 6-argument form, protecting every other existing call site).
- Full existing test suite: zero regressions.

## Interpretation calls made
1. `ImGui::BeginChild(..., false, ImGuiWindowFlags_HorizontalScrollbar)` — no border (`false`),
   since this is a dense inline row inside an already-bordered/indented row body, not a standalone
   panel; `LuaCodeEditor_UI.cpp`'s own precedent uses `true`, but that is a much larger, visually
   distinct code-editing region. If this reads wrong once built (e.g. the row's content bleeds
   into the row above/below with no visual separation), switching this one bool to `true` is a
   fast, contained follow-up — not a reason to block this ticket.
2. The button's visible text shortens to `"Map Size"` (from `"Set to Map Size"`) purely to help the
   row fit — its function, ID, and behavior are unchanged. If the human wants the original full
   label restored (accepting a slightly wider scroll extent), that is a one-word revert.
3. `rowHeight = ResolveWidgetTrackHeight(WidgetStyle()) + kAreaDetailRowVerticalPaddingPixels`
   (8.0f) is a starting value. If a coder finds the child region clips the numeric field or the
   color swatch vertically (a second, unwanted scrollbar would appear), increase only
   `kAreaDetailRowVerticalPaddingPixels`, not the widget.
