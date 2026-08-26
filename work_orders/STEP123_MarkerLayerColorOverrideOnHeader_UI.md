# STEP123 — Move Layer's Color Override checkbox + color swatch onto the collapsed row header

**Layer:** UI (widget-library generalization + composition). **Domain:**
`DraggableListWidget_RowLayout_UI.h`, `DraggableListWidget_UI_Test.cpp`/`DraggableList_TestScene_UI.h`,
`ColorSwatch_UI.h`/`.cpp`, `MarkersTab_ManualLayers_UI.h`/`.cpp`, `MarkersTab_ManualLayerRowBody_UI.cpp`.
**Sequence:** no dependency on other undone work-orders; independent of STEP106/STEP107 (both already
landed against this same file — every line cited below was re-read fresh this session, post-STEP110/
STEP116/STEP119/STEP120/STEP200).

## Problem
The human wants the "Color Override" checkbox + its color swatch — a Manual Marker Layer's own
per-layer override of `Params::MarkerInstanceLayer::bColorOverrideEnabled`/`color`
(`src/params/MarkerInstance_PARAMS.h:23,25,42-49`) — visible and editable directly on the row's own
**collapsed** header line, to the LEFT of the row's existing `[o]`/`[U]`/`X` affordance strip, instead
of only inside the row's EXPANDED body where STEP116 placed it.

Confirmed live (re-verified fresh this session, not re-derived from STEP116's own ticket text):

- **Current location.** `MarkersTab_ManualLayerRowBody_UI.cpp:33-39`, inside `DrawLayerRowBody`
  (STEP110's per-row EXPANDED-body function, declared `MarkersTab_ManualLayers_UI.h:122-126`, drawn
  only when that row's own `CollapsingHeader` is open):
  ```cpp
  bool bColorOverrideCommitted = false;
  if (!state.bUseGroupColor) {
      bColorOverrideCommitted = DrawCheckbox("Color Override", layer.bColorOverrideEnabled).bCommitted;
      ImGui::BeginDisabled(!layer.bColorOverrideEnabled);
      DrawColorSwatch("Color", layer.color, state.previewColorOptions, state.selectedLayerColorToggle);
      ImGui::EndDisabled();
  }
  ```
  Gated on `!state.bUseGroupColor` — while the block-wide "Use Group Color" toggle is on, the control
  is hidden entirely (not merely disabled). Matters for the header design below.

- **What is already on the row's own collapsed header today**, left to right (confirmed against
  `DraggableListWidget_RowAffordances_UI.h:46-66`'s `DrawRowAffordances`, called from
  `DraggableListWidget_RowLayout_UI.h:34`'s `RenderCollapsibleRow`): the disclosure arrow + row
  label/name (the `CollapsingHeader` itself, `ImGuiTreeNodeFlags_SpanFullWidth`), then, right-aligned
  via `ImGui::SameLine(rowAvailWidthPixels - kAffordanceStripWidthPixels - extraButtonWidthPixels)`
  (`RowAffordances_UI.h:49-50`): a visibility icon `[o]`/`[-]` (`RowAffordances_UI.h:35-37` —
  suppressed for the Manual Marker Layers list specifically; `MarkerInstanceLayer` carries no
  `bVisible`/`bHidden`, per `MarkersTab_ManualLayers_UI.h:17-18`'s own SCOPE NOTE 2, so this icon is
  currently inert dead-clickable UI for this consumer, unrelated to this ticket, not touched here),
  the lock icon `[L]`/`[U]` (`bLocked`, STEP106), the delete button `X`, and an optional per-row
  `extraButtonLabel` (STEP150, unused by Manual Marker Layers today). **No mechanism exists today for
  arbitrary extra widget content (a checkbox + a color swatch) to sit on this header line**, only a
  single opt-in text-button slot to the RIGHT of `X`. The new controls must sit BEFORE (left of) all
  of the above, on the header's own line.

- **`DraggableList<T>::Render`'s exact current signature**
  (`DraggableListWidget_RowLayout_UI.h:64-98`) is a 2-callback template (`describeRow`, `drawRowBody`)
  plus `selectedRowIndex`/`layout`. **19 existing call sites** across the codebase
  (`MarkersTab_ManualLayers_UI.cpp`, `MarkersTab_RuleLayers_UI.cpp`, `ArmiesTab_UI.cpp`,
  `Application_ViewLayersPopup_UI.cpp`, `MarkersTab_ManualInstance_UI.cpp`, `LayersTab_UI.cpp`,
  `LayerEditor_UI.cpp`, `LayerEditor_Group_UI.cpp`, `MarkersTab_Manual_UI.cpp`,
  `DraggableList_TestScene_UI.h`, `PropsTab_ManualDecals_UI.cpp`, `PropsTab_Manual_UI.cpp`,
  `ScenariosTab_ListMechanics_UI.h`, `AreasTab_UI.cpp`, `PropsTab_Decals_UI.cpp`, `PropsTab_UI.cpp`,
  plus 3 test files) all bind this exact 2-callback shape — any change to `DraggableList<T>::Render`
  must not force an edit to any of them.

- **Bundle-tree leaf rows do NOT go through `DraggableList<T>::Render` at all.**
  `MarkersTab_Bundles_UI.cpp:28-30`'s `DrawMarkerGroupLeafBody` calls `DrawLayerRowBody` (the
  EXPANDED-body function) directly, as a leaf-body callback into a completely different generic
  widget, `TreeListWidget_UI<T, LeafKeyT>` (`TreeListWidget_UI.h:23-38`). Confirmed by reading
  `TreeListWidget_UI.h` in full: it has **no** lock/visibility/delete affordance strip, no header-extra
  slot, nothing analogous to `DraggableListRow`'s icons — a leaf row is a label + an optional expanded
  body only. A bundled `MarkerInstanceLayer` (`parentBundleIdentifier != -1`) is also explicitly
  **suppressed** in the root `DraggableList<Params::MarkerInstanceLayer>::Render` call
  (`MarkersTab_ManualLayers_UI.cpp:42`, `row.bRowSuppressed = (... .parentBundleIdentifier != -1)`) —
  it never draws a header there at all. **Consequence: if Color Override is removed from
  `DrawLayerRowBody`'s body outright (as this ticket's own prompt suggested as the likely default),
  every bundled Manual Marker Layer loses all UI access to Color Override, with no replacement** — the
  Bundle tree has no header slot to move it to. This is a real, ticket-blocking finding, not a
  hypothetical; see the Fix's explicit reversal of that default below.

- **`ColorSwatch_UI.h`/`.cpp`'s current `DrawColorSwatch` cannot sit inline on a header line as-is.**
  `ColorSwatch_UI.cpp:45-46`: `ImGui::PushID(label); ImGui::TextUnformatted(label);` — the label is
  drawn as its **own line**, unconditionally, before the swatch button (drawn on the line below,
  `ColorSwatch_UI.cpp:49-53`, `ResolveSwatchSize` defaults to consuming the remaining content width
  minus the RT-button width, `ColorSwatch_UI.cpp:29-36`). There is no way to call this today without
  eating an extra line and, by default, the row's full remaining width — incompatible with a compact
  header slot sitting left of a fixed-width affordance strip.

## Fix

### 1. `DraggableList<T>` — new OPTIONAL header-extra slot, additive-only, zero risk to the 19 existing call sites
`DraggableListWidget_RowLayout_UI.h`. Add a **second overload** of `Render` that accepts a third
callback (`drawRowHeaderExtra`) and a fixed reserved width (`headerExtraWidthPixels`); the EXISTING
2-callback overload becomes a thin delegator passing a no-op lambda and `0.0f` — every current call
site recompiles unchanged because the new overload requires two more non-defaulted parameters than any
existing call supplies (arity alone disambiguates; no SFINAE needed):

```cpp
template <typename DescribeRowFunction, typename DrawRowBodyFunction>
static DraggableListSignal Render(const char* listIdentifier, const std::vector<T>& items,
                                  DescribeRowFunction describeRow, DrawRowBodyFunction drawRowBody,
                                  int selectedRowIndex = -1,
                                  DraggableListRowLayout layout = DraggableListRowLayout::Collapsible) {
    return Render(listIdentifier, items, describeRow, drawRowBody, [](int) {}, 0.0f,
                  selectedRowIndex, layout);
}

// STEP123: the OPTIONAL per-row header-extra slot, drawn INLINE on the header/Flat row's own line,
// to the LEFT of the [o]/[U]/X strip. `headerExtraWidthPixels` is a FIXED width the CALLER supplies
// (not measured per row like extraButtonLabel's text width — Color Override's checkbox+swatch is a
// constant size for every row) and is reserved UNCONDITIONALLY so the strip sits at one constant
// offset regardless of any individual row's own state (bColorOverrideEnabled, bUseGroupColor, ...).
// headerExtraWidthPixels == 0.0f (the overload above) draws nothing and reserves nothing.
template <typename DescribeRowFunction, typename DrawRowBodyFunction, typename DrawRowHeaderExtraFunction>
static DraggableListSignal Render(const char* listIdentifier, const std::vector<T>& items,
                                  DescribeRowFunction describeRow, DrawRowBodyFunction drawRowBody,
                                  DrawRowHeaderExtraFunction drawRowHeaderExtra,
                                  float headerExtraWidthPixels, int selectedRowIndex = -1,
                                  DraggableListRowLayout layout = DraggableListRowLayout::Collapsible) {
    DraggableListSignal signal;
    if (listIdentifier == nullptr) return signal;
    const char* const payloadIdentifier =
        (std::strlen(listIdentifier) < 32u) ? listIdentifier : "SanGenDraggableListRow";
    ImGui::PushID(listIdentifier);
    const int rowCount = static_cast<int>(items.size());
    for (int rowIndex = 0; rowIndex < rowCount; ++rowIndex) {
        ImGui::PushID(rowIndex);
        const DraggableListRow row = describeRow(rowIndex);
        if (!row.bRowSuppressed) {
            const float extraButtonWidthPixels = DraggableListExtraButtonWidthPixels(row);
            const float rowAvailWidthPixels = ImGui::GetContentRegionAvail().x;
            if (layout == DraggableListRowLayout::Flat)
                RowLayoutDetail::RenderFlatRow(payloadIdentifier, row, rowIndex, rowAvailWidthPixels,
                    extraButtonWidthPixels, headerExtraWidthPixels, selectedRowIndex, drawRowBody,
                    drawRowHeaderExtra, signal);
            else
                RowLayoutDetail::RenderCollapsibleRow(payloadIdentifier, row, rowIndex, rowAvailWidthPixels,
                    extraButtonWidthPixels, headerExtraWidthPixels, selectedRowIndex, drawRowBody,
                    drawRowHeaderExtra, signal);
        }
        ImGui::PopID();
    }
    ImGui::PopID();
    return signal;
}
```

`RenderCollapsibleRow`/`RenderFlatRow` (`DraggableListWidget_RowLayout_UI.h:19-56`) each gain the same
two new parameters. **No signature change to `DrawRowAffordances`
(`DraggableListWidget_RowAffordances_UI.h:46-66`) is needed** — its only use of `rowAvailWidthPixels`
is the single `ImGui::SameLine(rowAvailWidthPixels - kAffordanceStripWidthPixels - extraButtonWidthPixels)`
offset (`RowAffordances_UI.h:49-50`), so pre-subtracting `headerExtraWidthPixels` from the VALUE passed
into that call achieves the left-shift with zero edits to that file:

```cpp
template <typename DrawRowBodyFunction, typename DrawRowHeaderExtraFunction>
void RenderCollapsibleRow(const char* payloadIdentifier, const DraggableListRow& row, int rowIndex,
                          float rowAvailWidthPixels, float extraButtonWidthPixels,
                          float headerExtraWidthPixels, int selectedRowIndex,
                          DrawRowBodyFunction drawRowBody, DrawRowHeaderExtraFunction drawRowHeaderExtra,
                          DraggableListSignal& signal) {
    const float stripStartX = ImGui::GetCursorScreenPos().x + rowAvailWidthPixels
        - static_cast<float>(kAffordanceStripWidthPixels) - extraButtonWidthPixels - headerExtraWidthPixels;
    const bool bExpanded = ImGui::CollapsingHeader(
        row.label != nullptr ? row.label : "",
        ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_SpanFullWidth |
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen |
        (rowIndex == selectedRowIndex ? ImGuiTreeNodeFlags_Selected : 0));
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && ImGui::GetIO().MousePos.x < stripStartX)
        RecordSignal(signal, DraggableListSignalKind::Select, rowIndex);
    DetectRowDragAndDrop(payloadIdentifier, rowIndex, signal);
    if (headerExtraWidthPixels > 0.0f) {
        ImGui::SameLine(rowAvailWidthPixels - static_cast<float>(kAffordanceStripWidthPixels)
            - extraButtonWidthPixels - headerExtraWidthPixels);
        drawRowHeaderExtra(rowIndex);
    }
    DrawRowAffordances(row, rowIndex, signal, extraButtonWidthPixels,
                       rowAvailWidthPixels - headerExtraWidthPixels, false);
    if (bExpanded) { ImGui::Indent(); drawRowBody(rowIndex); ImGui::Unindent(); }
}
```
`RenderFlatRow` gets the same `headerExtraWidthPixels > 0.0f` block inserted after its own
`drawRowBody(rowIndex)` line and before its own `DrawRowAffordances(...)` call, same pattern, same
`rowAvailWidthPixels - headerExtraWidthPixels` substitution — included for interface symmetry even
though no Flat-mode (View popup) consumer opts in today.

**Why this shape and not a `DraggableListRow` field.** Rejected alternative: bake
`bColorOverrideEnabled`/a `float* color` pointer directly onto `DraggableListRow` and have
`DrawRowAffordances` itself draw the checkbox+swatch. Rejected because it puts a Marker-layer-domain
concept ("Color Override") and color-picker/RealtimeToggle plumbing into the generic, domain-agnostic
widget every other tab (Props, Decals, Areas, Armies, GeoLayers, the View popup) shares — that file's
own header states it "Owns NO application state and MUTATES NOTHING," and stays that way. A
caller-supplied draw callback is the same idiom this file already uses twice (`drawRowBody`,
`extraButtonLabel`'s STEP150 precedent) and keeps the widget generic.

### 2. `ColorSwatch_UI.h`/`.cpp` — compact, label-less swatch mode
New `ColorSwatchOptions` field (`ColorSwatch_UI.h:24-29`):
```cpp
struct ColorSwatchOptions {
    bool  bAlphaEnabled  = false;
    bool  bAlphaBarShown = false;
    float swatchWidth    = 0.0f;
    float swatchHeight   = 0.0f;
    bool  bLabelHidden   = false;   // NEW — STEP123: skip the TextUnformatted(label) line so the
                                     // button + RT toggle sit on ONE line via SameLine (a header slot).
                                     // `label` is still used to scope ImGui::PushID; only the visible
                                     // text is skipped.
};
```
`ColorSwatch_UI.cpp:45-46`:
```cpp
ImGui::PushID(label);
if (!options.bLabelHidden) ImGui::TextUnformatted(label);
```
This is the ONLY change to `DrawColorSwatch`'s body — the button, popup, and RT-toggle-button lines
(`ColorSwatch_UI.cpp:48-59`) are unaffected, so the header swatch keeps the SAME picker popup and the
SAME RT-toggle-button every other swatch has (no functionality lost by moving out of the body).

### 3. New named width constants — `MarkersTab_ManualLayers_UI.h`
Beside `ManualMarkerLayersState` (Constitution §8 — named settings, not literals at the call site):
```cpp
// STEP123 — the collapsed row header's reserved width for the Color Override checkbox + compact
// swatch (DrawManualMarkerLayerColorOverrideHeaderControl, MarkersTab_ManualLayerRowBody_UI.cpp),
// left of DraggableList's own [o]/[U]/X strip. Eyeballed against a live frame, same posture every
// other ImDrawList pixel budget in this widget family already accepts (Checkbox_UI.cpp/
// ColorSwatch_UI.cpp: "Rendering is verified by eye against a live frame, never by test").
inline constexpr float kMarkerLayerColorOverrideHeaderWidthPixels = 90.0f;
inline constexpr float kMarkerLayerColorOverrideSwatchWidthPixels = 24.0f;
```
Budget: no-label checkbox (`ResolveWidgetTrackHeight`, ~19px) + `ItemSpacing` + a 24px swatch button +
`DrawColorSwatch`'s own internal `SameLine` + RT-toggle button (`WidgetStyle::realtimeButtonWidth`,
default 30px, `WidgetHelpers_UI.h:33`) ≈ 89px; rounded to 90.

### 4. The header control itself — new function, `MarkersTab_ManualLayerRowBody_UI.cpp`
**Placed in the aspect-split sibling file, not `MarkersTab_ManualLayers_UI.cpp`.**
`MarkersTab_ManualLayers_UI.cpp` is currently 124 lines against the **hard 150-line ceiling**
(`ARCH_01_05_FileSizeCeilings.md` §1.5); adding this function there would land it at the ceiling for no
reason. `MarkersTab_ManualLayerRowBody_UI.cpp` is 53 lines (plenty of headroom under the 100-line soft
ceiling) and already owns "one row's own settings" — the natural home, same file `DrawLayerRowBody`
lives in. Declared beside `DrawLayerRowBody` in `MarkersTab_ManualLayers_UI.h:122-126`, defined here:
```cpp
// STEP123: the row header's own compact Color Override control — checkbox + a small inline swatch,
// drawn on EVERY row's collapsed header line via DraggableList's new header-extra slot, NOT gated on
// the row's own expand state. Disabled (not hidden) while state.bUseGroupColor forces one shared
// tint, so the header's own width never shifts when that block-wide toggle flips — deliberately
// unlike the (unchanged, still-hidden-when-forced) body copy this ticket leaves in place; see the
// Out of Scope note on why the body copy is NOT removed.
void DrawManualMarkerLayerColorOverrideHeaderControl(Params::MarkerInstanceLayer& layer,
                                                      ManualMarkerLayersState& state, bool& bAnyCommitted) {
    ImGui::BeginDisabled(state.bUseGroupColor);
    const bool bOverrideCommitted = DrawCheckbox("", layer.bColorOverrideEnabled).bCommitted;
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Color Override");
    ImGui::SameLine();
    ImGui::BeginDisabled(!layer.bColorOverrideEnabled);
    Ui::ColorSwatchOptions headerSwatchOptions = state.previewColorOptions;  // COPY: do not mutate
                                                                              // the shared block-level
                                                                              // options struct
    headerSwatchOptions.bLabelHidden = true;
    headerSwatchOptions.swatchWidth  = kMarkerLayerColorOverrideSwatchWidthPixels;
    const bool bColorCommitted = DrawColorSwatch("ColorOverrideHeaderSwatch", layer.color,
        headerSwatchOptions, state.selectedLayerColorToggle).bCommitted;
    ImGui::EndDisabled();
    ImGui::EndDisabled();
    if (bOverrideCommitted || bColorCommitted) bAnyCommitted = true;
}
```
`DrawCheckbox("", ...)` — an EMPTY label, not `"##..."` — `TickBoxWasClicked`
(`Checkbox_UI.cpp:41-56`) only checks `label[0] != '\0'` to decide whether to draw text; imgui's `##`
label-hiding convention does not apply here (this control never calls an imgui `Button`/`Selectable`
family function with the label), so a literal `"##colorOverrideHeader"` string would have printed
verbatim. `ImGui::PushID("")` inside `DrawCheckbox` is safe here — this is the only empty-label
checkbox in this row's own `PushID(rowIndex)` scope.

### 5. Wiring — `DrawLayerList`, `MarkersTab_ManualLayers_UI.cpp`
`DrawLayerList` (lines 33-53) gains the third callback + width, both passed straight through:
```cpp
DraggableListSignal DrawLayerList(std::vector<Params::MarkerInstanceLayer>& markerLayers,
                                  std::vector<Params::MarkerInstanceGroup>& markers,
                                  const Params::Geometry& geometry, int globalSymmetryMask, int globalRadialRepeatCount,
                                  Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings,
                                  ManualMarkerLayersState& state, bool& bAnyNameCommitted) {
    return DraggableList<Params::MarkerInstanceLayer>::Render(
        "manualMarkerLayers", markerLayers,
        [&](int rowIndex) {
            DraggableListRow row;
            row.bRowSuppressed = (markerLayers[static_cast<std::size_t>(rowIndex)].parentBundleIdentifier != -1);
            row.label   = ManualMarkerLayerRowLabel(markerLayers[static_cast<std::size_t>(rowIndex)]);
            row.bLocked = markerLayers[static_cast<std::size_t>(rowIndex)].bLocked;
            return row;
        },
        [&](int rowIndex) {
            if (DrawLayerRowBody(markerLayers[static_cast<std::size_t>(rowIndex)], rowIndex, markerLayers, markers,
                                 geometry, globalSymmetryMask, globalRadialRepeatCount, markerSymmetryFixSettings, state))
                bAnyNameCommitted = true;
        },
        [&](int rowIndex) {
            DrawManualMarkerLayerColorOverrideHeaderControl(
                markerLayers[static_cast<std::size_t>(rowIndex)], state, bAnyNameCommitted);
        },
        kMarkerLayerColorOverrideHeaderWidthPixels,
        state.selectedLayerIndex);
}
```
Bundled rows (`row.bRowSuppressed == true`) never reach `RenderCollapsibleRow`, so the new header
lambda never runs for them — consistent with them never having drawn a header here at all.

### 6. The expanded body — **left unchanged; the "remove it" default from this ticket's own prompt is reversed here, with reasoning**
`MarkersTab_ManualLayerRowBody_UI.cpp:33-39` (the existing body copy) is **NOT deleted.** This
reverses the ticket prompt's stated default ("most likely: remove it from the body") because reading
`MarkersTab_Bundles_UI.cpp`/`TreeListWidget_UI.h` fresh this session surfaced a concrete regression the
prompt could not have known about: bundled Manual Marker Layers reach their settings ONLY through
`DrawLayerRowBody` via the Bundle tree's leaf body — they never draw a header through
`DraggableList<Params::MarkerInstanceLayer>::Render` at all (suppressed, §5), and
`TreeListWidget_UI<T, LeafKeyT>` has no affordance-strip/header-extra mechanism of its own to move the
control to. Deleting the body copy would silently strip Color Override from every bundled layer with
no UI path left to reach it.

Consequence, stated plainly: for an **ungrouped** (root-list) row, once expanded, the control now
appears TWICE — once on the header (always visible), once in the body (only while
`!state.bUseGroupColor`, unchanged STEP116 gating). This cannot desync in the data sense the prompt
worried about (both write the exact same `layer.bColorOverrideEnabled`/`layer.color` fields; whichever
one commits last simply overwrites with the same value the picker produced) — the only cost is a
redundant control on an already-expanded row, judged strictly cheaper than a silent functionality gap
for every bundled layer. **A true follow-up ticket** — giving `TreeListWidget_UI` its own equivalent
header-extra slot, then removing the body copy for real — is a comparably-sized undertaking of its own
and is explicitly NOT scoped or attempted here.

### 7. Stale comment correction — `MarkersTab_ManualLayers_UI.h:48-52`
`selectedLayerColorToggle`'s doc comment ("ONE shared toggle set for the SELECTED row's own
color/scale") was already slightly stale post-STEP110 (bodies draw per-EXPANDED-row, not
per-"selected"-row) and is now used by the header control on **every** row, every frame, not just an
expanded one. Reword to: "ONE shared `RealtimeToggle` instance reused across every row's own
color/scale control, header or body — `Params::MarkerInstanceLayer` cannot carry a `RealtimeToggle`
member (a pure round-tripping type). Pre-existing limitation (already true for multiple simultaneously
expanded row bodies since STEP110); this ticket widens how often it's exercised, not the limitation
itself." Comment-only; no behavior change.

## Out of scope
- **Removing `DrawLayerRowBody`'s existing Color Override block.** See Fix §6 — explicitly reversed
  from this ticket's own initial framing, with the Bundle-tree regression as the reason. Do not delete
  it under this ticket.
- **Giving `TreeListWidget_UI<T, LeafKeyT>` an equivalent header-extra slot** so bundled Manual Marker
  Layers could ALSO get the header control and the body copy could then be safely removed. Flagged as
  a genuine, real follow-up; not attempted here — comparable in size to this entire ticket.
- **The visibility icon `[o]`/`[-]`'s dead-click status** for the Manual Marker Layers list
  (`MarkerInstanceLayer` has no `bVisible`; SCOPE NOTE 2, `MarkersTab_ManualLayers_UI.h:17-18`).
  Pre-existing, unrelated, untouched.
- **`MarkersTab_RuleLayers_UI.cpp`'s Procedural layer rows.** Confirmed fresh this session (grep, full
  read of that file's `Render` call region): no `ColorOverride` concept exists there at all — Color
  Override is a Manual-layer-only field on `MarkerInstanceLayer`, never on `MarkerRuleLayer`. This
  ticket does not touch that file, and the new `DraggableList<T>::Render` overload is opt-in — its
  existing 2-callback call there (`MarkersTab_RuleLayers_UI.cpp:75`) is unaffected either way.
- **Any change to `bColorOverrideEnabled`'s resolution/consumer semantics.**
  `EffectiveManualMarkerLayerColor`/whatever `MapCanvas_IconLayer_CullManual_UI.cpp`/
  `MapCanvas_MarkerDrag_UI.cpp` do with the resolved color is untouched — this ticket is authoring-UI
  placement only.
- **The View popup (Flat-mode) rows.** `RenderFlatRow` gains the same optional parameters for
  interface symmetry (§1) but no Flat-mode consumer opts in under this ticket.
- **Any change to `DraggableListRow`'s own fields** (`DraggableListWidget_Types_UI.h`) — untouched; the
  new mechanism is callback-based specifically to avoid domain-specific fields there (see Fix §1's
  "why this shape" note).
- **Visual/pixel-perfect header layout verification.** Like every other ImDrawList control in this
  widget family, final on-screen fit is verified by eye against a live frame, not by test (matches
  `Checkbox_UI.cpp`/`ColorSwatch_UI.cpp`'s own stated posture) — the new unit tests below verify the
  *geometry/interaction contract* (reserved width, correct row index on click, strip shift), not pixel
  aesthetics.

## Files touched
- `src/ui/DraggableListWidget_RowLayout_UI.h` — `DraggableList<T>::Render` gains a second,
  3-callback overload (`drawRowHeaderExtra`, `headerExtraWidthPixels`); the existing 2-callback
  overload becomes a delegating call with a no-op lambda and `0.0f`; `RenderCollapsibleRow`/
  `RenderFlatRow` each gain the same two parameters and an `if (headerExtraWidthPixels > 0.0f)` draw
  block, and pass `rowAvailWidthPixels - headerExtraWidthPixels` into their existing
  `DrawRowAffordances` calls
- `src/ui/ColorSwatch_UI.h` — `ColorSwatchOptions` gains `bLabelHidden`
- `src/ui/ColorSwatch_UI.cpp` — `DrawColorSwatch` skips `TextUnformatted(label)` when
  `bLabelHidden`
- `src/ui/MarkersTab_ManualLayers_UI.h` — new `kMarkerLayerColorOverrideHeaderWidthPixels`,
  `kMarkerLayerColorOverrideSwatchWidthPixels`; new declaration
  `DrawManualMarkerLayerColorOverrideHeaderControl` beside `DrawLayerRowBody`; corrected doc comment
  on `selectedLayerColorToggle`
- `src/ui/MarkersTab_ManualLayerRowBody_UI.cpp` — new function
  `DrawManualMarkerLayerColorOverrideHeaderControl`; `DrawLayerRowBody` itself is UNCHANGED (Fix §6)
- `src/ui/MarkersTab_ManualLayers_UI.cpp` — `DrawLayerList` gains the third `Render` callback +
  width argument
- `src/ui/DraggableListWidget_UI_Test.cpp` / `src/ui/DraggableList_TestScene_UI.h` — new generic
  header-extra-slot acceptance test (see Verify)
- No change to `DraggableListWidget_Types_UI.h`, `DraggableListWidget_RowAffordances_UI.h`,
  `MarkersTab_RuleLayers_UI.cpp`, `MarkersTab_Bundles_UI.cpp`, or any of the other 17
  `DraggableList<T>::Render` call sites.

## Verify
Acceptance bar: the header control toggles the real field independent of row expand state, is inert
while `bUseGroupColor` forces a shared tint, the generic `DraggableList` mechanism is provably a no-op
for every one of the 19 existing call sites, and the reserved-width geometry is correct (strip shifts
left by exactly the reserved amount, no click-target overlap).

- **New test — `DraggableListWidget_UI_Test.cpp`, `TestOptionalHeaderExtraContentIsGenericAndRowScoped`**
  (mirrors `TestOptionalExtraButtonIsGenericAndRowScoped`'s existing precedent, same file, same scene
  helpers): extend `DraggableList_TestScene_UI.h`'s `RunSceneFrame` with an optional
  `headerExtraWidthPixels`/`drawRowHeaderExtra` (default `0.0f`/no-op, calling the ORIGINAL 2-callback
  `Render` overload when unset, so the existing three tests in this file keep exercising the exact
  code path they always have). New test: opt a scene into a nonzero `headerExtraWidthPixels` with a
  `drawRowHeaderExtra` that draws one `ImGui::SmallButton("##probe")`; assert (a) sweeping X in the
  reserved band left of the strip finds a clickable probe reporting the correct row index (via a
  scene-local out-param, since `ExtraButton`'s `DraggableListSignalKind` is unrelated — a raw bool/int
  captured by the test's own lambda is sufficient, no new signal kind needed), (b) the
  visibility/lock/delete strip's own X-offsets (found by the SAME sweep technique
  `TestAffordanceSignalsCarryTheRightIndex` already uses) are shifted left by exactly
  `headerExtraWidthPixels` versus the zero-reservation scene, (c) a scene that never opts in
  (`headerExtraWidthPixels == 0.0f`, the existing 2-callback overload) reproduces
  `TestAffordanceSignalsCarryTheRightIndex`'s existing `visibilityX`/`lockX`/`deleteX` values
  byte-for-byte — proof the additive overload changes nothing for a consumer that doesn't opt in.
- **New test — a headless-imgui-frame test for `DrawManualMarkerLayerColorOverrideHeaderControl`**,
  new file `MarkersTab_ManualLayerColorOverrideHeader_UI_Test.cpp` (this control has no existing
  headless-frame test file of its own to extend; `MarkersTab_ManualLayers_UI_Test.cpp` is pure-logic
  only, no imgui frame, per its own header comment — mirror `ListWidget_TestFrame_UI.h`'s
  `HeadlessImguiSession`/`RunHeadlessFrame` harness instead, the same one
  `DraggableListWidget_UI_Test.cpp` uses):
  - Build a one-entry `markerLayers` vector, a default `ManualMarkerLayersState`. Run a frame calling
    `DrawManualMarkerLayerColorOverrideHeaderControl` directly (no `DraggableList::Render` wrapper
    needed — this is a leaf imgui function, testable standalone). Synthesize a click on the checkbox's
    known screen position (first frame: hover; second: press+release, mirroring
    `DraggableList_TestScene_UI.h`'s `ClickAt` hover-then-click pattern — a plain checkbox has no
    `AllowOverlap` hover requirement, but matching the established click helper shape costs nothing).
    Assert `layer.bColorOverrideEnabled` flipped and the function's `bAnyCommitted` out-param went
    true.
  - Set `state.bUseGroupColor = true`; re-run the same click sequence at the same coordinates; assert
    `layer.bColorOverrideEnabled` did NOT change (the `ImGui::BeginDisabled` wrap makes the checkbox
    unclickable) — proves the header control actually goes inert, not merely visually grayed.
  - With `layer.bColorOverrideEnabled = false` (`state.bUseGroupColor = false`), assert a click on the
    swatch button's known position does not open the popup / does not report `bColorEdited` (disabled
    swatch); with `bColorOverrideEnabled = true`, assert the swatch button IS clickable (opens the
    picker popup, `ImGui::IsPopupOpen` on the frame after click).
- **No new test for `ColorSwatchOptions::bLabelHidden` in `ColorSwatch_UI_Test.cpp`.** That file is
  pure-logic-only by design and its own header comment already states the analogous `NoInputs` flag is
  "a draw-path flag... not testable headless" — `bLabelHidden` is the same category, verified by eye
  (and indirectly by the new header-control test above, which would visibly overflow/misalign if the
  label line were still drawn — not a substitute for a dedicated assertion, but the honest coverage
  this flag gets).
- **Existing suites stay green, unchanged assertions**: `DraggableListWidget_UI_Test.cpp`'s three
  existing tests (`TestApplySignalMovesAndDeletes`, `TestAffordanceSignalsCarryTheRightIndex`,
  `TestSyntheticDragProducesTheExpectedOrder`, `TestOptionalExtraButtonIsGenericAndRowScoped`) all
  still call the 2-callback `Render` overload and must produce byte-identical results;
  `MarkersTab_ManualLayers_UI_Test.cpp`'s three existing pure-logic tests are untouched by this ticket
  (no field/function they cover changed shape); `ColorSwatch_UI_Test.cpp`'s three existing tests are
  untouched (no existing `ColorSwatchOptions` field's meaning changed, only a new field added with a
  `false` default matching every existing call site's implicit current behavior).
