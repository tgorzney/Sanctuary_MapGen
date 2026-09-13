# STEP268 — Stratum Tab: compact Enabled/Name row + Erodable checkbox

**Layer:** UI. **Domain:** `src/ui/Checkbox_UI.h`, `src/ui/Checkbox_UI.cpp`,
`src/ui/StratumsTab_Material_UI.cpp`, `src/ui/StratumsTab_Soil_UI.cpp`.
**Executor:** SanGen Coder. **Sequence:** independent, no dependencies. First ticket of the
procedural-stratum-masking track (see `work_orders/HANDOFF_TRACK_ProceduralStratumMasking.md`) but
does not depend on any other part of that track landing first, and nothing later in the track depends
on this ticket's exact row layout — safe to dispatch standalone.

Read `sangen_arch_pack/specs/UI_FRAMEWORK_SPEC.md`'s "Universal widget library" section before
starting; this ticket extends that library's existing `bLabelHidden` convention, it does not invent a
new one. Consulted with the UI Expert before writing this ticket (design-only consult, no code
written by that consult).

## 0. Why

Human request: compress the Stratum Tab's per-layer UI to match the compact-row convention already
shipped elsewhere (`ArmiesTab_RowLayout_UI.cpp`, `AreasTab_UI.cpp`) — "remove enabled label and name
label ... the label was keyword text inside the input."

Verified current state: `StratumsTab_Material_UI.cpp:47-52` (`DrawStratumMaterialPanel`) draws the
Enabled checkbox and Name field as two separate, full-width, visibly-labeled lines:
```cpp
NotifyStratumsTabChange(DrawCheckbox("Enabled", stratum.bEnabled).bCommitted, previewDriver);
NotifyStratumsTabChange(
    DrawTextInput("Name", stratum.appearance.name, StratumNameRules()).bCommitted, previewDriver);
```
`DrawTextInput` (`TextInput_UI.h:105-108`) already supports `hintText` + `bLabelHidden` +
`fixedWidthPixels` — used exactly this way for `Army::displayName` in
`ArmiesTab_RowLayout_UI.cpp:18-23` (hint `"Army Name"`, `bLabelHidden=true`,
`fixedWidthPixels=140.0f`), a field with the identical "usually already populated, but a legal empty
state exists" property as `Stratum::appearance.name` (`FormatStratumSectionLabel`,
`StratumsTab_Options_UI.h:75-79`, falls back to `"Stratum %d"` when empty — a tested state,
`StratumsTab_UI_Test.cpp:87-88`). Use the same hint-text treatment here, not a separate compact-label
glyph.

`DrawCheckbox` (`Checkbox_UI.h:83`, `Checkbox_UI.cpp:60-66`) has **no** `bLabelHidden` parameter today
— unlike `Combo`/`ColorSwatch`/`TextInput`, a bare tick box has no self-evident meaning from its own
rendered content (a swatch shows a color, a combo shows its selection text), so hiding its label
needs a fallback: a hover tooltip carrying the label text, so the meaning isn't lost, only the
always-visible text. This ticket adds that parameter to the shared widget rather than special-casing
the Stratum tab, so every future caller gets it for free and no rival ad-hoc "checkbox without a
label" gets invented elsewhere.

## 1. `src/ui/Checkbox_UI.h` — add `bLabelHidden`

Change the declaration at line 83 and its preceding comment:
```cpp
// Draws the box + its label and runs the interaction above. `bLabelHidden` (default false, every
// existing call site unchanged) drops the visible label text while `label` still salts the imgui id
// and becomes a hover tooltip instead — mirroring TextInputRules/ColorSwatchOptions's established
// `bLabelHidden` shape, but pairing it with a tooltip because a bare tick box (unlike a swatch's
// color or a combo's selection text) has no self-evident meaning once its label is gone.
WidgetChange DrawCheckbox(const char* label, bool& value, const WidgetStyle& style = WidgetStyle(),
                          bool bLabelHidden = false);
```
Leave `DrawExclusiveCheckboxRow` untouched — out of scope (see §4).

## 2. `src/ui/Checkbox_UI.cpp` — implement it

Change `DrawCheckbox` (lines 60-66):
```cpp
WidgetChange DrawCheckbox(const char* label, bool& value, const WidgetStyle& style, bool bLabelHidden) {
    ImGui::PushID(label);
    const bool bClicked = TickBoxWasClicked("##box", bLabelHidden ? nullptr : label, value, style);
    if (bLabelHidden && ImGui::IsItemHovered() && label != nullptr && label[0] != '\0')
        ImGui::SetTooltip("%s", label);
    const WidgetChange change = StepCheckboxInteraction(value, bClicked);
    ImGui::PopID();
    return change;
}
```
`TickBoxWasClicked` (line 41) already treats a null/empty `label` as "draw no label text" (line 44's
`label != nullptr && label[0] != '\0'` guard) and sizes its `InvisibleButton` to just the box in that
case — no change needed there. `ImGui::IsItemHovered()` reads the `InvisibleButton` pushed by
`TickBoxWasClicked` (the tick box itself is drawn via `ImDrawList`, not a separate imgui item, so the
"last item" is still that button). The `PushID(label)` id salt is unchanged either way, so no call
site's persistent widget identity moves.

## 3. `src/ui/StratumsTab_Material_UI.cpp` — compact the Enabled/Name row

Replace lines 50-52:
```cpp
NotifyStratumsTabChange(
    DrawCheckbox("Enabled", stratum.bEnabled, WidgetStyle(), /*bLabelHidden=*/true).bCommitted,
    previewDriver);
ImGui::SameLine();
NotifyStratumsTabChange(
    DrawTextInput("Name", stratum.appearance.name, StratumNameRules(), WidgetStyle(), "Stratum Name",
                 /*bLabelHidden=*/true, /*fixedWidthPixels=*/140.0f)
        .bCommitted,
    previewDriver);
```
Checkbox first (it gates whether the rest of the row's settings do anything, so it leads), Name
second, `SameLine()`-chained — same ordering logic as `DrawArmySettings`. Every other row in this
panel (`DrawAssetNameCombo` calls, texture pickers, mask-mode toggle) is unchanged.

## 4. `src/ui/StratumsTab_Soil_UI.cpp` — Erodable checkbox

Same gap, same fix. Change line 74:
```cpp
const WidgetChange erodableChange =
    DrawCheckbox("Erodable", soilPhysics.bErodable, WidgetStyle(), /*bLabelHidden=*/true);
```
This stays on its own line (no adjacent control to chain it with — the Soil Physics panel's scalar
rows are one-per-line today and a horizontal-layout redesign of that panel is explicitly out of scope,
see §5); only the always-visible "Erodable" text becomes a hover tooltip instead. Everything else in
`DrawStratumSoilPanel` is unchanged.

## 5. Out of scope

- `StratumsTab_Scalars_UI.cpp`'s 15 one-per-line scalar rows (Mask Remap Min/Max, Tile Size/Far,
  Normal Scale, Hardness/Friction/Cohesion/etc.) — pairing these into same-line groups is a real
  layout redesign (grouping/pairing decisions), not a label-hiding change. Left for a future ticket.
- `StratumsTab_Soil_UI.cpp:35`'s `DrawCombo("Preset", ...)` visible label — judgment call, not
  requested, left as-is.
- `DrawAssetNameCombo`/`DrawTexturePathRow` labels in `StratumsTab_Material_UI.cpp` (Environment,
  Material, Albedo, Normal, Composite) — these labels disambiguate real pickers; not touched.
- `DrawExclusiveCheckboxRow` (`Checkbox_UI.h:86-88`) — unrelated call sites (symmetry-axis groups),
  no `bLabelHidden` added there.
- Anything about the procedural stratum mask-filter feature itself (slope/height/roughness filter
  chains, blend modes) — that is the rest of
  `work_orders/HANDOFF_TRACK_ProceduralStratumMasking.md`, not this ticket. This ticket only compacts
  existing widgets; it adds no new `Params::Stratum` fields or PROC behavior.

## 6. Tests

`Checkbox_UI.cpp`'s own file comment states rendering is "verified by eye against a live frame, never
by test" — `Checkbox_UI_Test.cpp` covers only the pure `StepCheckboxInteraction`/exclusive-mask
functions in the header, which this ticket does not change (the new `bLabelHidden` parameter is
render-only, consumed by `TickBoxWasClicked`'s label argument and the tooltip call, never touching
`StepCheckboxInteraction`). Consistent with that existing convention:

1. No new interaction-logic test is added for `bLabelHidden` itself — there is nothing pure to assert
   on beyond what `Checkbox_UI_Test.cpp` already covers.
2. **Regression, must still pass unmodified:** `Checkbox_UI_Test.cpp`, `StratumsTab_UI_Test.cpp`,
   `StratumsTab_Soil_UI_Test.cpp` — none of their assertions touch rendering, so none should need
   edits; if any do, that's a signal this ticket drifted from its own scope.
3. Confirm by reading (not by running the app): every existing `DrawCheckbox` call site outside this
   ticket's three edits (14+ sites, e.g. symmetry-axis toggles, other tabs) omits the new fourth
   argument and therefore keeps its default `bLabelHidden=false` — unchanged behavior. Grep
   `DrawCheckbox(` across `src/ui/` after the edit and check each call site still compiles with its
   existing argument count.

## 7. Files touched

**Modified:** `src/ui/Checkbox_UI.h`, `src/ui/Checkbox_UI.cpp`, `src/ui/StratumsTab_Material_UI.cpp`,
`src/ui/StratumsTab_Soil_UI.cpp`.
