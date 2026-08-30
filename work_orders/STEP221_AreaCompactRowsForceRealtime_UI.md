# STEP221 — Area Position/Size compact rows, force-realtime (no RT button)

## Summary
The human: "I want the Position and Size on one line (all inputs in compact format as a
universal input widget scalars, put the label to the left of the value display/input, and the
slider to the right of the label and remove RT because it should always be realtime, it is simple
calculations and should calculate instantly" — and confirmed the Area color swatch is already the
same universal `DrawColorSwatch` every other tab uses (no widget-identity change needed there),
but its RT button should be removed for the same "always realtime" reason.

Ruled by the SanGen UI Expert (this session) after reading `AreasTab_UI.cpp/.h`,
`SliderScalar_UI.h/.cpp`, `ColorSwatch_UI.h`, `RtToggleWidget_UI.h`. Two existing, ALREADY-SHIPPED
widget features cover this with zero widget-library changes:
- `DrawSliderScalarCompact(label, value, range, realtimeToggle, trackWidthPixels,
  fieldWidthPixels, style, valueFormat, bShowRealtimeToggle=true)` (`SliderScalar_UI.h:134-137`,
  STEP134) — the one-line track+field shape, with an existing `bShowRealtimeToggle` parameter.
- `ColorSwatchOptions::bRealtimeToggleHidden` (`ColorSwatch_UI.h:36`) — already exists and is
  already exercised with the IDENTICAL reasoning at `MarkersTab_Globals_UI.cpp:87`
  (`"color edits are always realtime, no choice"`).

**Hiding the RT button alone is not enough.** `RealtimeToggle` default-constructs
`bRealtimeEnabled = false` (`RtToggleWidget_UI.h:75`) and `RealtimeToggle::Update`
(`RtToggleWidget_UI.h:48-61`) still defers `bCommitted` to mouse-release while
`bRealtimeEnabled` is false — hiding only the button would leave the field working exactly as
before, just with no way to ever see or flip the (still-OFF) state. Area position/size/color feed
`PreviewComposite::BuildMapAreaConfigurations` — a real recomposite — so a deferred commit would
visibly freeze the fill mid-drag, the opposite of "should calculate instantly." Every affected
`RealtimeToggle` must be **force-constructed realtime-ON**, not merely have its button hidden.

## Required reading
- `src/ui/AreasTab_UI.h` (full file, 104 lines)
- `src/ui/AreasTab_UI.cpp` (full file, 190 lines)
- `src/ui/SliderScalar_UI.h:113-137` (`DrawSliderScalar` vs `DrawSliderScalarCompact`)
- `src/ui/ColorSwatch_UI.h:24-37` (`ColorSwatchOptions`)
- `src/ui/RtToggleWidget_UI.h:29-78` (`RealtimeToggle`, its constructor and default)
- `src/ui/MarkersTab_Globals_UI.cpp:87-99` (existing `bRealtimeToggleHidden`/`bShowRealtimeToggle`
  precedent to mirror, both reasoning and call shape)

## 1. `src/ui/AreasTab_UI.h` — reorder, fix a pre-existing seeding bug, force realtime, add widths

**Reorder:** move the existing `AreasTabColorSwatchOptions()` free function (currently lines
61-66, AFTER `struct AreasTabState`) to BEFORE `struct AreasTabState` (i.e. immediately after the
`namespace Ui {` opening line). `AreasTabState`'s own default member initializer for `colorOptions`
is about to call this function (next bullet), so the function must be declared before that point in
the same translation unit — a plain top-to-bottom reorder within this header, no signature change.

**Fix a pre-existing seeding bug, discovered by the UI Expert while ruling on this ticket:**
`AreasTabState::colorOptions` is currently seeded from a bare `ColorSwatchOptions()` default
(line 36) instead of from `AreasTabColorSwatchOptions()` — meaning the alpha channel and the
picker's vertical alpha bar this same file's own header comment (lines 8-9) claims are on for
Areas ("the areas tab is the one caller `ColorSwatchOptions::bAlphaBarShown` was added for") are
NOT actually seeded into the live struct today. `AreasTabColorSwatchOptions()` itself is otherwise
dead in production, referenced only by `AreasTab_UI_Test.cpp:113`. Fold the fix into this ticket
(bundled, not silent scope creep — call this out in your own report as a bundled fix): change

```cpp
ColorSwatchOptions colorOptions = ColorSwatchOptions();
```
to
```cpp
ColorSwatchOptions colorOptions = AreasTabColorSwatchOptions();
```

**Add `bRealtimeToggleHidden = true` to `AreasTabColorSwatchOptions()`** (after its existing two
lines):
```cpp
inline ColorSwatchOptions AreasTabColorSwatchOptions() {
    ColorSwatchOptions options;
    options.bAlphaEnabled        = true;
    options.bAlphaBarShown       = true;
    options.bRealtimeToggleHidden = true;   // STEP221 — area color is always realtime, no choice
    return options;
}
```

**Force every affected `RealtimeToggle` realtime-ON at construction** — change the five member
declarations (currently plain `RealtimeToggle originXToggle;` etc.) to:
```cpp
RealtimeToggle originXToggle{true};
RealtimeToggle originZToggle{true};
RealtimeToggle widthToggle{true};
RealtimeToggle lengthToggle{true};
RealtimeToggle colorToggle{true};
```
(`RealtimeToggle`'s `explicit RealtimeToggle(bool bRealtimeEnabledInitially)` constructor already
exists, `RtToggleWidget_UI.h:32` — no widget-library change needed.)

**Add two new named width constants** (Constitution §8 — settings, not literals in the draw code),
beside the existing `AreaExtentSliderRange`/`AreaOriginSliderRange` functions:
```cpp
// STEP221 — the compact Position/Size rows' fixed pixel widths (DrawSliderScalarCompact).
inline constexpr float kAreaScalarCompactTrackWidthPixels = 56.0f;
inline constexpr float kAreaScalarCompactFieldWidthPixels = 44.0f;
```

## 2. `src/ui/AreasTab_UI.cpp` — two compact rows replacing the four `DrawSliderScalar` calls

Replace lines 49-56 (the four `DrawSliderScalar` calls for X Position / Z Position / Width /
Length) with:

```cpp
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
```

The single-letter labels (`X`/`Z`/`W`/`L`) are the VISIBLE text; the full strings (`"X Position"`,
`"Z Position"`, `"Width"`, `"Length"`) are unchanged as the string handed to
`DrawSliderScalarCompact` itself, which still uses it to scope the imgui ID and as the hover
tooltip (`SliderScalar_UI.cpp`'s existing `IsItemHovered()/SetTooltip` pattern) — so the full name
stays discoverable on hover.

**No change needed to the color swatch call sites** (lines 67, 70) — `state.colorOptions` now
carries `bRealtimeToggleHidden = true` via the header fix above, and `state.colorToggle` is now
force-constructed realtime-on, so `DrawColorSwatch("Color", color, state.colorOptions,
state.colorToggle)` picks up both changes with zero call-site edits.

**No change needed to `DrawAreaSettings`'s signature, `DrawAreaList`, `ApplyAreaListSignal`, or
`DrawAreasTab`** — this ticket touches only the body of `DrawAreaSettings`'s Position/Size block
and the header's state/options seeding.

## ARCH rules invoked
- Constitution §8 — no magic literals; the two new pixel widths are named constants.
- UI_FRAMEWORK_SPEC §7 — realtime semantics: RT off defers commit to release, RT on commits every
  change; area geometry/color feed a real recomposite, so both must be forced ON, not merely have
  their toggle button hidden.
- ARCH §3.2 — a widget owns no app state; `RealtimeToggle`'s existing explicit-bool constructor is
  the correct seam, not a new field on the widget.

## Explicit out-of-scope
- No change to `SliderScalar_UI.h/.cpp`, `ColorSwatch_UI.h/.cpp`, or `RtToggleWidget_UI.h` — every
  primitive this ticket needs already exists and ships unmodified.
- No change to the "Set to Map Size" button, the Name field, the Area Stack list rows, the
  per-area lock/color resolution, or anything about the `[o]` visibility icon — those are
  untouched by this ticket (a real per-area visibility toggle and a "Center in Map" button are
  separate follow-up tickets, STEP222/STEP223).
- No change to the foreign-scenario area-import IO layer — unrelated, separate follow-up
  (STEP224).

## Acceptance test
- `AreasTab_UI_Test.cpp` (and any other test exercising `DrawAreaSettings`/`AreasTabState`) must
  still pass. Any existing test that asserts the OLD 4-line `DrawSliderScalar` call shape, or that
  constructs an `AreasTabState` and checks a `RealtimeToggle`'s default `IsRealtimeEnabled() ==
  false`, is now testing a shape this ticket deliberately changes — update those assertions to the
  new compact-row/force-realtime shape rather than leaving them failing. Do not weaken or delete a
  test to make it pass; adapt it to assert the new, correct behavior.
- A fresh manual/automated check: dragging Area X/Z/Width/Length or editing its color must commit
  on every frame of the drag (no visible freeze until release) — confirms `RealtimeToggle{true}`
  actually took effect, not just that the RT button disappeared.
- Full existing test suite: zero regressions elsewhere (no other call site touches
  `AreasTabColorSwatchOptions`/`AreasTabState`'s reordered declarations).

## Interpretation calls made
1. Single-letter labels (`X`/`Z`/`W`/`L`) rather than the full words, to reliably fit two fields on
   one line at the panel's default width — the full name is still on the hover tooltip
   (`DrawSliderScalarCompact`'s own existing behavior, unchanged). If a coder finds these clip or
   look wrong at the shipped default panel width, adjust ONLY the two new width constants
   (`kAreaScalarCompactTrackWidthPixels`/`kAreaScalarCompactFieldWidthPixels`), not the widget.
2. The `colorOptions` seeding-bug fix (bare `ColorSwatchOptions()` → `AreasTabColorSwatchOptions()`)
   is bundled into this ticket because it is directly adjacent to the line already being edited for
   `bRealtimeToggleHidden`, and leaving it unfixed would mean the new flag is set on a struct
   nothing seeds it from. Flagged explicitly here and in the coder's own report, per Constitution
   §6 (fix discovered defects, do not silently route around them).
