# STEP213 — Generic pressable toggle-button widget + Auto-Level heightmap toggle

**Layer:** UI. **Domain:** the shared widget library (`RtToggleWidget_UI.h`/`.cpp`), the toolbar
(`Application_Draw_UI.cpp`), the HeightRamp `PreviewFieldLayer` (`PreviewComposite_Settings_UI.h`),
`UI_FRAMEWORK_SPEC.md`. **Executor:** SanGen Coder. Authored by the SanGen UI Expert. Every file
this ticket cites was read directly against the live tree while drafting it (`RtToggleWidget_UI.h`,
`RtToggleWidget_UI.cpp`, `RtToggleWidget_UI_Test.cpp`, `WidgetHelpers_UI.h`,
`sangen_arch_pack/specs/UI_FRAMEWORK_SPEC.md`, `PreviewComposite_Settings_UI.h`,
`PreviewComposite_Prepare_UI.cpp`, `SlopeTab_UI.cpp`, `Application_PreviewSetup_UI.cpp`,
`Application_Draw_UI.cpp`, `Application_UI.h`, `Application_ViewLayersPopup_UI.cpp`,
`Application_PanelTerrain_UI.cpp`, `TerrainOverlayTab_UI.h`, `ListWidget_TestFrame_UI.h`,
`SliderScalar_UI_Test.cpp`, `MarkersTab_Bundles_UI_Test.cpp`, `CoreInputWidgets_LiveFrame_UI_Test.cpp`,
`CMakeLists.txt`) — this ticket is immediately buildable, with no forward-looking prerequisites.

## Summary
Two independent parts:

**Part 1** generalizes `RtToggleWidget_UI`'s "a real pressable toggle, not a checkbox" primitive
(`ImGui::Button` + an active-state color push through the shared `WidgetStyle`/`ResolveWidgetColor`
mechanism) into `DrawToggleButton` — a plain-`bool&` toggle button any tab or the toolbar can draw.
`DrawRealtimeToggleButton` becomes a thin, behavior-preserving adapter over it: same fixed `"RT"`
label, same fixed `style.realtimeButtonWidth` (30px, load-bearing elsewhere — several other
widgets' own row-layout math subtracts it by name), same two tooltip strings, same click-frame
tooltip semantics (verified bit-for-bit below). No existing call site of `DrawRealtimeToggleButton`
changes.

**Part 2** wires a new "Auto-Level" toggle button into the canvas toolbar, right after "View", using
`DrawToggleButton` from Part 1. It flips `bAutoDomainFromField` on the HeightRamp `PreviewFieldLayer`
specifically — the same CPU min/max-scan mechanism (`PreviewComposite_Prepare_UI.cpp`'s `FieldRange`)
already live for Slope/Flow/Accumulation, just never turned on for the heightmap. This is presentation
state (no stage hashes it), so the write is followed by the same `previewDriver.NotifyParametersChanged()`
call every other composite-presentation edit already uses (`Application_PanelTerrain_UI.cpp`'s
`DrawHeightRampSection`, `SlopeTab_UI.cpp`'s own checkbox) — never a new dirty-flag path of its own.

## Required reading
`sangen_arch_pack/specs/UI_FRAMEWORK_SPEC.md` (its "Universal widget library" section and bypass-
toolkit item 7 — the RT toggle this ticket generalizes) and `RtToggleWidget_UI.h`'s own "THE SPLIT"
convention (`WidgetHelpers_UI.h`) that every widget in this library follows.

---

## 1. Modified: `src/ui/RtToggleWidget_UI.h`

Full new file content (the class body is byte-identical to today's; only the header comment and the
free-function declarations below it change):

```cpp
// RtToggleWidget_UI.h — the universal pressable toggle-button widget, plus the per-control
// "realtime" wrapper built on top of it. Layer: UI.
// UI_FRAMEWORK_SPEC §7: while RT is OFF, dragging updates the value every frame but DEFERS the
// expensive recompute until mouse-release, so scrubbing a slider stays at frame rate; while RT
// is ON the commit fires on every change and the caller recomputes live.
//
// STEP213 — DrawToggleButton is the general "real pressable toggle, not a checkbox" primitive
// (ImGui::Button + an active-state color push through the shared WidgetStyle/ResolveWidgetColor
// mechanism) the whole widget library now shares; DrawRealtimeToggleButton is reduced to a thin
// adapter that translates RealtimeToggle's own IsRealtimeEnabled()/SetRealtimeEnabled() through a
// local bool and binds the fixed "RT" label + its two ON/OFF tooltip strings — no existing call
// site of DrawRealtimeToggleButton changes. The Auto-Level toolbar toggle (Application_Draw_UI.cpp)
// is DrawToggleButton's first plain-bool, non-RT caller.
//
// Ui::RealtimeToggle is a pure state machine over three bits of interaction — is an edit in
// progress, did the value move, is a commit pending — and holds NO app state: the caller owns
// one instance next to the control it wraps and decides what a commit costs (bNeedsMapUpdate vs
// bNeedsPreviewRender on Pipeline::PreviewDriver). It is therefore drivable headless from a
// synthetic mouse-down/drag/up sequence, which is exactly how it is acceptance-tested.
//
// This replaces the legacy UIHelpers.h RT flag, which lived in three function-local `static`s
// shared by every slider in the program — one drag could clobber another control's state.
#pragma once
#include "WidgetHelpers_UI.h"

namespace SanmapGen {
namespace Ui {

class RealtimeToggle {
public:
    RealtimeToggle() = default;
    explicit RealtimeToggle(bool bRealtimeEnabledInitially) : bRealtimeEnabled(bRealtimeEnabledInitially) {}

    bool IsRealtimeEnabled() const { return bRealtimeEnabled; }
    // Turning RT on does not itself commit; any pending change flushes on the next Update, so a
    // control switched to realtime mid-drag catches up on the following frame.
    void SetRealtimeEnabled(bool bEnabled) { bRealtimeEnabled = bEnabled; }
    // True while a value edit is waiting on mouse-release to be paid for.
    bool IsCommitDeferred() const { return bDeferredCommitPending; }

    // One frame of interaction.
    //   bEditInProgress      — the control is being dragged (the mouse is held on it).
    //   bValueMovedThisFrame — the caller's value actually changed this frame.
    // Returns the live/expensive pair (WidgetChange). With RT off the commit lands on the frame
    // the drag ENDS, and only if the value moved during it — a click that moves nothing costs
    // nothing. A move that is not part of a drag (a typed field, a keyboard step) has no release
    // to wait for and commits immediately, so a deferral can never get stuck.
    WidgetChange Update(bool bEditInProgress, bool bValueMovedThisFrame) {
        WidgetChange change;
        change.bValueChanged = bValueMovedThisFrame;
        if (bValueMovedThisFrame) bDeferredCommitPending = true;

        const bool bReleasedThisFrame = bEditWasInProgress && !bEditInProgress;
        const bool bEditOutsideDrag   = bValueMovedThisFrame && !bEditInProgress && !bEditWasInProgress;
        bEditWasInProgress = bEditInProgress;

        change.bCommitted = bDeferredCommitPending &&
                            (bRealtimeEnabled || bReleasedThisFrame || bEditOutsideDrag);
        if (change.bCommitted) bDeferredCommitPending = false;
        return change;
    }

    // Pays for a pending change now, regardless of RT state — for the caller that must settle
    // before it stops drawing the control (a tab switch or a collapsed window mid-drag, where no
    // release frame will ever arrive). Reports whether a commit was actually owed.
    WidgetChange FlushPendingCommit() {
        WidgetChange change;
        change.bCommitted = bDeferredCommitPending;
        bDeferredCommitPending = false;
        bEditWasInProgress = false;
        return change;
    }

private:
    bool bRealtimeEnabled       = false;   // default OFF: cheap scrubbing is the safe default
    bool bDeferredCommitPending = false;
    bool bEditWasInProgress     = false;
};

// The universal pressable toggle button (STEP213) — every toggle-button control in the widget
// library draws through this one function, so "a real button, not a checkbox" is implemented
// exactly once. `enabled` is the caller's own plain state; the widget owns none of it
// (WidgetHelpers_UI.h "no control owns app state") — it flips `enabled` in place and reports the
// click back so the caller can decide what, if anything, that flip costs (a PreviewDriver
// notification, a PARAMS write, nothing).
//   buttonWidth   — 0 auto-sizes the button to its own label's content, matching ImGui::Button's
//                   own "size axis == 0 means auto-fit" contract — the right default for an
//                   arbitrary label like "Auto-Level" (a negative value instead stretches to fill
//                   the remaining line width minus |buttonWidth|, also legal, unused by either
//                   caller in this ticket). This is a plain function parameter rather than a
//                   WidgetStyle field on purpose: `WidgetStyle::realtimeButtonWidth` is already a
//                   distinct, load-bearing 30px slot several OTHER widgets' own row-layout math
//                   subtracts by name (RangeSliderWidget_UI.cpp, LabelledDialWidget_UI.cpp,
//                   ColorSwatch_UI.cpp, SliderScalar_Track_UI.cpp, Levels_UI.cpp) — repurposing its
//                   meaning for every toggle button in the library would silently change every one
//                   of those layouts. A caller that must reproduce that exact fixed width (only
//                   DrawRealtimeToggleButton does) passes it through explicitly.
//   tooltipOn/tooltipOff — each optional; `nullptr` means "no tooltip for that state" (Auto-Level
//                   passes neither; DrawRealtimeToggleButton passes both, unchanged from before).
//                   The tooltip reflects the state as it was BEFORE this frame's click (not after),
//                   matching the original DrawRealtimeToggleButton's own (pre-existing) behavior.
// Returns true on the frame it was clicked (the same frame `enabled` flips).
bool DrawToggleButton(const char* identifier, const char* label, bool& enabled,
                      const WidgetStyle& style = WidgetStyle(), float buttonWidth = 0.0f,
                      const char* tooltipOn = nullptr, const char* tooltipOff = nullptr);

// Draws the little "RT" button that flips `realtimeToggle`, returns true on the frame it was
// clicked. Consumes exactly one imgui item and no horizontal layout of its own, so a control
// can place it with SameLine (RangeSliderWidget_UI/LabelledDialWidget_UI both do).
// STEP213 — now a thin adapter over DrawToggleButton: binds the fixed "RT" label, the fixed
// `style.realtimeButtonWidth` slot (unchanged visual width — SliderScalar_UI_Test.cpp:201-221
// asserts this exact width today and must keep passing unmodified), and the two ON/OFF tooltip
// strings, translating RealtimeToggle's own IsRealtimeEnabled()/SetRealtimeEnabled() through a
// local bool DrawToggleButton can flip.
bool DrawRealtimeToggleButton(const char* identifier, RealtimeToggle& realtimeToggle,
                              const WidgetStyle& style = WidgetStyle());

// The three style resolvers every control in the library shares, so "unset means follow the
// imgui theme" is implemented exactly once. Defined beside the RT button because it is the one
// piece every control embeds.
//   `imguiThemeColor` is an ImGuiCol enumerator, taken as int so this header stays imgui-free;
//   the result is the ImU32 an ImDrawList wants.
unsigned int ResolveWidgetColor(PackedColor requestedColor, int imguiThemeColor);
float ResolveWidgetTrackHeight(const WidgetStyle& style);
float ResolveWidgetRounding(const WidgetStyle& style);

} // namespace Ui
} // namespace SanmapGen
```

---

## 2. Modified: `src/ui/RtToggleWidget_UI.cpp`

Full new file content:

```cpp
// RtToggleWidget_UI.cpp — the imgui side of the universal toggle button (STEP213) and the RT
// wrapper built on it, plus the style resolvers the whole widget library shares. Layer: UI. This
// is one of the translation units that include imgui.h; the RT state machine itself
// (Ui::RealtimeToggle) is pure and lives in the header, so its acceptance test needs no imgui
// frame (see RtToggleWidget_UI.h "THE SPLIT").
#include "RtToggleWidget_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {

unsigned int ResolveWidgetColor(PackedColor requestedColor, int imguiThemeColor) {
    if (requestedColor == kThemeColor)
        return ImGui::GetColorU32(static_cast<ImGuiCol>(imguiThemeColor));
    return static_cast<unsigned int>(requestedColor);
}

float ResolveWidgetTrackHeight(const WidgetStyle& style) {
    return style.trackHeight > 0.0f ? style.trackHeight : ImGui::GetFrameHeight();
}

float ResolveWidgetRounding(const WidgetStyle& style) {
    return style.cornerRounding >= 0.0f ? style.cornerRounding : ImGui::GetStyle().FrameRounding;
}

bool DrawToggleButton(const char* identifier, const char* label, bool& enabled,
                      const WidgetStyle& style, float buttonWidth,
                      const char* tooltipOn, const char* tooltipOff) {
    // Captured BEFORE the click is applied — the color push below must reflect the state the
    // button was actually drawn in, and the tooltip (below) intentionally still describes that
    // same pre-click state on the click's own frame, exactly matching the original
    // DrawRealtimeToggleButton's own (pre-existing) ordering.
    const bool bWasEnabled = enabled;
    const ImU32 activeColor = ResolveWidgetColor(style.realtimeActiveColor, ImGuiCol_ButtonActive);

    ImGui::PushID(identifier);
    ImGui::PushStyleColor(ImGuiCol_Button, bWasEnabled ? activeColor : ImGui::GetColorU32(ImGuiCol_Button));
    const bool bClicked = ImGui::Button(label, ImVec2(buttonWidth, 0.0f));
    ImGui::PopStyleColor();
    if (bClicked) enabled = !bWasEnabled;
    const char* const tooltip = bWasEnabled ? tooltipOn : tooltipOff;
    if (tooltip != nullptr && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tooltip);
    ImGui::PopID();
    return bClicked;
}

bool DrawRealtimeToggleButton(const char* identifier, RealtimeToggle& realtimeToggle,
                              const WidgetStyle& style) {
    bool bRealtimeEnabled = realtimeToggle.IsRealtimeEnabled();
    const bool bClicked = DrawToggleButton(identifier, "RT", bRealtimeEnabled, style,
                                           style.realtimeButtonWidth,
                                           "Realtime: ON - recompute on every change",
                                           "Realtime: OFF - recompute once, on mouse release");
    if (bClicked) realtimeToggle.SetRealtimeEnabled(bRealtimeEnabled);
    return bClicked;
}

} // namespace Ui
} // namespace SanmapGen
```

**Verification against the port source (today's `RtToggleWidget_UI.cpp`, pre-this-ticket):** the
color-push condition, the `ImGui::Button` call and its exact `ImVec2(style.realtimeButtonWidth, 0.0f)`
size, the flip-on-click assignment, and — the one place a naive refactor breaks parity — the
**tooltip using the pre-click state, not the post-click one** (today's code captures
`const bool bRealtimeEnabled = realtimeToggle.IsRealtimeEnabled();` once at the top, draws with it,
flips the toggle if clicked, then reads the tooltip off that SAME pre-click local) were each checked
line-by-line and reproduced exactly via `bWasEnabled` above. `DrawRealtimeToggleButton`'s own
`bRealtimeEnabled` local plays the identical role: `DrawToggleButton` flips it to `!bWasEnabled`
when clicked, so `realtimeToggle.SetRealtimeEnabled(bRealtimeEnabled)` receives exactly the value
today's `SetRealtimeEnabled(!bRealtimeEnabled)` call would have.

---

## 3. Modified: `src/ui/RtToggleWidget_UI_Test.cpp`

**Header comment** (replaces the current top-of-file comment):
```cpp
// RtToggleWidget_UI_Test.cpp — acceptance test for the M5-1 RT (realtime) defer semantics, plus
// (STEP213) the universal DrawToggleButton draw path both DrawRealtimeToggleButton and the new
// Auto-Level toolbar toggle share. The RealtimeToggle state-machine checks below still drive it
// with synthetic mouse-down/drag/up sequences and link no imgui at all (RtToggleWidget_UI.h
// "THE SPLIT"). The STEP213 additions at the bottom close a real pre-existing gap:
// DrawRealtimeToggleButton's own draw path had NO test before this ticket (only the pure
// RealtimeToggle machinery did) — they use the SAME headless imgui harness
// (ListWidget_TestFrame_UI.h's HeadlessImguiSession/RunHeadlessFrame) a dozen other SanGen UI
// tests already share; no window, no GL, no new CMake linkage (every add_sangen_test target
// already links SanGenV2, which links imgui PUBLIC — CMakeLists.txt:398).
// The claim under test (UI_FRAMEWORK_SPEC §7): with RT OFF the value tracks the drag every frame
// while the expensive commit fires exactly ONCE, on release.
#include "ListWidget_TestFrame_UI.h"
#include "RtToggleWidget_UI.h"
#include <cstdio>

using namespace SanmapGen;

static int failureCount = 0;

static void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failureCount; }
}

static bool IsNear(float value, float expected, float tolerance = 1.0e-5f) {
    const float difference = value - expected;
    return difference < tolerance && difference > -tolerance;
}
```
(the `#include "RtToggleWidget_UI.h"` line already existed; `#include "ListWidget_TestFrame_UI.h"`
and the new `IsNear` helper are additions — `Check`/`failureCount` are unchanged from today.)

Everything from today's `DragTally`/`RunDrag` through `TestModeSwitchAndOutOfDragEdits()` is
**unchanged** — keep it verbatim.

**New test functions**, appended directly after `TestModeSwitchAndOutOfDragEdits()`:

```cpp
// STEP213 — DrawToggleButton's own click contract, driven with a real headless imgui frame
// (ImGui::Button commits on the RELEASE frame, not the press — the same 3-frame probe/press/
// release shape MarkersTab_Bundles_UI_Test.cpp's own button tests already use).
static void TestDrawToggleButtonFlipsPlainBoolOnClickRelease() {
    Ui::HeadlessImguiSession session;
    bool enabled = false;
    const ImVec2 windowSize(300.0f, 100.0f);

    ImVec2 itemMin, itemMax;
    Ui::RunHeadlessFrame(Ui::HeadlessMouseState(), windowSize, [&] {
        const bool bClicked = Ui::DrawToggleButton("autoLevelToggle", "Auto-Level", enabled);
        Check(!bClicked, "no click reported while the mouse is nowhere near the button");
        itemMin = ImGui::GetItemRectMin();
        itemMax = ImGui::GetItemRectMax();
    });
    Check(!enabled, "state has not moved yet");

    const ImVec2 center((itemMin.x + itemMax.x) * 0.5f, (itemMin.y + itemMax.y) * 0.5f);
    Ui::HeadlessMouseState press; press.position = center; press.bLeftButtonDown = true;
    Ui::RunHeadlessFrame(press, windowSize, [&] {
        const bool bClicked = Ui::DrawToggleButton("autoLevelToggle", "Auto-Level", enabled);
        Check(!bClicked, "the PRESS frame reports no click yet (imgui commits on release)");
        Check(!enabled, "and does not flip the state early");
    });

    Ui::HeadlessMouseState release = press; release.bLeftButtonDown = false;
    bool bClickedOnRelease = false;
    Ui::RunHeadlessFrame(release, windowSize, [&] {
        bClickedOnRelease = Ui::DrawToggleButton("autoLevelToggle", "Auto-Level", enabled);
    });
    Check(bClickedOnRelease, "the RELEASE frame (still hovering) reports the click");
    Check(enabled, "the caller's own bool flips in place -- exactly what Auto-Level's toolbar "
                  "wiring reads back");

    // A second click cycle flips it back off, confirming the widget owns no latched state of its own.
    Ui::RunHeadlessFrame(press, windowSize, [&] { Ui::DrawToggleButton("autoLevelToggle", "Auto-Level", enabled); });
    bool bSecondClick = false;
    Ui::RunHeadlessFrame(release, windowSize, [&] {
        bSecondClick = Ui::DrawToggleButton("autoLevelToggle", "Auto-Level", enabled);
    });
    Check(bSecondClick && !enabled, "clicking again flips it back off");
}

// STEP213 — DrawRealtimeToggleButton is now a thin adapter over DrawToggleButton; this proves the
// port is behavior-preserving: same click contract, same exact drawn width
// (SliderScalar_UI_Test.cpp:201-221's own width assertion depends on this staying exact).
static void TestDrawRealtimeToggleButtonStillFlipsThroughTheGenericPrimitive() {
    Ui::HeadlessImguiSession session;
    Ui::RealtimeToggle realtimeToggle;   // default OFF
    const ImVec2 windowSize(300.0f, 100.0f);

    ImVec2 itemMin, itemMax;
    Ui::RunHeadlessFrame(Ui::HeadlessMouseState(), windowSize, [&] {
        Ui::DrawRealtimeToggleButton("rtToggle", realtimeToggle);
        itemMin = ImGui::GetItemRectMin();
        itemMax = ImGui::GetItemRectMax();
    });
    Check(IsNear(itemMax.x - itemMin.x, Ui::WidgetStyle().realtimeButtonWidth),
         "the RT button keeps its exact historical fixed width after the generalization");

    const ImVec2 center((itemMin.x + itemMax.x) * 0.5f, (itemMin.y + itemMax.y) * 0.5f);
    Ui::HeadlessMouseState press; press.position = center; press.bLeftButtonDown = true;
    Ui::RunHeadlessFrame(press, windowSize, [&] { Ui::DrawRealtimeToggleButton("rtToggle", realtimeToggle); });
    Check(!realtimeToggle.IsRealtimeEnabled(), "still off mid-press");

    Ui::HeadlessMouseState release = press; release.bLeftButtonDown = false;
    bool bClickedOnRelease = false;
    Ui::RunHeadlessFrame(release, windowSize, [&] {
        bClickedOnRelease = Ui::DrawRealtimeToggleButton("rtToggle", realtimeToggle);
    });
    Check(bClickedOnRelease && realtimeToggle.IsRealtimeEnabled(),
         "the click flips RealtimeToggle's own state, same as before the refactor");
}

// STEP213 — the default buttonWidth (0) auto-fits the label, unlike RT's own fixed 30px slot --
// required for a longer label like "Auto-Level" that would otherwise visibly clip.
static void TestDrawToggleButtonAutoSizesUnlikeTheFixedRtWidth() {
    Ui::HeadlessImguiSession session;
    bool enabled = false;
    ImVec2 itemMin, itemMax;
    Ui::RunHeadlessFrame(Ui::HeadlessMouseState(), ImVec2(300.0f, 100.0f), [&] {
        Ui::DrawToggleButton("autoLevelToggle", "Auto-Level", enabled);
        itemMin = ImGui::GetItemRectMin();
        itemMax = ImGui::GetItemRectMax();
    });
    Check(itemMax.x - itemMin.x > Ui::WidgetStyle().realtimeButtonWidth,
         "a longer label auto-sizes wider than the RT button's fixed 30px slot by default");
}
```

**`main()`** — replace with:
```cpp
int main() {
    TestRealtimeOffDefersToRelease();
    TestDeferredCommitSurvivesAStillLastFrame();
    TestClickWithoutMovementCostsNothing();
    TestRealtimeOnCommitsEveryChange();
    TestModeSwitchAndOutOfDragEdits();
    TestDrawToggleButtonFlipsPlainBoolOnClickRelease();
    TestDrawRealtimeToggleButtonStillFlipsThroughTheGenericPrimitive();
    TestDrawToggleButtonAutoSizesUnlikeTheFixedRtWidth();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
```

No CMakeLists.txt change: `RtToggleWidget_UI_Test` is already registered
(`add_sangen_test(RtToggleWidget_UI_Test src/ui/RtToggleWidget_UI_Test.cpp)`, `CMakeLists.txt:652`)
and already links `SanGenV2`, which links `imgui` `PUBLIC` (`CMakeLists.txt:232`) — the new
`#include "ListWidget_TestFrame_UI.h"`/`imgui.h` usage needs no new target or linkage.

---

## 4. Modified: `sangen_arch_pack/specs/UI_FRAMEWORK_SPEC.md`

**Bullet 7 of "The bypass toolkit"** — append a clause (do not otherwise reword this bullet):
```
7. **RT (realtime) toggles.** Widgets (e.g. the range slider) carry a per-control
   "RT" flag: while off, dragging updates the value but defers the expensive
   recompute until mouse-release — keeping FPS high during slider scrubbing.
   (STEP213: the RT button's own draw path is now `DrawToggleButton`, a generic
   pressable toggle-button primitive the whole widget library shares — see
   "Universal widget library" below.)
```

**"Universal widget library" section** — insert a new bullet immediately after the existing
`ConfirmDialog_UI` bullet, before `## v2 guidance`:
```
- **`DrawToggleButton`** — the generic pressable toggle-button primitive (`ImGui::Button` + an
  active-state color push through the shared `WidgetStyle`/`ResolveWidgetColor` mechanism): a real
  button, not a checkbox. Added (STEP213) by generalizing the toolkit's original RT ("realtime")
  toggle (item 7 above) into a widget the whole library shares — `DrawRealtimeToggleButton` is now
  a thin adapter over it, binding the fixed "RT" label/width/tooltip pair; the toolbar's Auto-Level
  toggle (`Application_Draw_UI.cpp`, over `PreviewFieldLayer::bAutoDomainFromField`) is its first
  general-purpose, non-RT consumer. Five other toggle-shaped controls surveyed elsewhere in the
  codebase (`Checkbox_UI`, the `DraggableList` row visibility icon, the Scenarios 3-state cycle
  button, the Marker bundle header buttons) are each a distinct interaction shape (a checkbox, an
  icon glyph, a tri-state cycle) and are NOT migrated onto this primitive by this entry — that is
  separate, unratified future work, not an oversight.
```

This is a documentation note over an already-established convention (a widget generalizing its own
draw path), not a new binding rule — it does not require ARCH-level ratification. If a future ticket
wants to *mandate* migrating the other five toggle-shaped controls onto `DrawToggleButton`, that
mandate belongs to the ARCH Expert, not this spec note.

---

## 5. Modified: `src/ui/Application_Draw_UI.cpp`

**New includes** — add alongside the existing ones:
```cpp
#include "Application_UI.h"
#include "RtToggleWidget_UI.h"
#include "TerrainOverlayTab_UI.h"
#include <algorithm>
#include <cfloat>
#include <imgui.h>
```

**`DrawCanvasWindow()`** — insert directly after the existing `if (ImGui::Button("View")) ...` line
(today's line 53) and before today's `ImGui::SetNextWindowSizeConstraints(...)` line:

```cpp
void Application::DrawCanvasWindow() {
    // STEP78 — auto-exit: browsing away from the Scenarios panel closes its detail panel in every
    // practical sense, so Scenario Edit Mode must not keep exclusive canvas ownership behind it.
    if (scenarioEditMode.IsActive() && tabState.activePanel != ApplicationPanel::Scenarios)
        scenarioEditMode.Deactivate();

    ImGui::Begin("Map Preview");
    if (ImGui::Button("View")) ImGui::OpenPopup("ViewLayersPopup");
    ImGui::SameLine();
    // STEP213 — Auto-Level: ports v1's min/max-scan heightmap normalization
    // (gui/PreviewRenderer.cpp:270-284,500, legacy) onto the SAME `PreviewFieldLayer::
    // bAutoDomainFromField` mechanism the Slope/Flow/Accumulation layers already expose
    // (PreviewComposite_Prepare_UI.cpp's FieldRange CPU scan), wired here for the HeightRamp layer
    // specifically -- the one field layer it was never turned on for. This is composite
    // PRESENTATION state, not recipe content (PreviewComposite_Settings_UI.h's own header note),
    // so no stage's parameter hash can see the flip -- the driver derives its own recomposite-only
    // tier off the SAME NotifyParametersChanged() call every other composite-presentation edit
    // already uses (Application_PanelTerrain_UI.cpp's DrawHeightRampSection, SlopeTab_UI.cpp's own
    // "Auto Domain From Field" checkbox). The toolbar button is the ONLY write path for this flag
    // in this ticket -- see the ticket's own "Explicit out-of-scope" for why no matching control
    // is added to the View popup's terrain section or to the Heightmap tab.
    PreviewFieldLayer* const heightRampLayer =
        PreviewFieldLayerOfKind(composite.Settings(), PreviewLayerKind::HeightRamp);
    if (heightRampLayer != nullptr) {
        bool bAutoLevelEnabled = heightRampLayer->bAutoDomainFromField;
        if (DrawToggleButton("autoLevelToggle", "Auto-Level", bAutoLevelEnabled)) {
            heightRampLayer->bAutoDomainFromField = bAutoLevelEnabled;
            previewDriver.NotifyParametersChanged();
        }
    }
    // STEP200 — defense-in-depth against the auto-fit-to-content growth feedback loop (an
    // unconstrained item width inside a BeginPopup window feeds back into that same window's next-
    // frame width): every item the popup draws is now itself fixed-width, but a max width here means
    // a future item added without SetNextItemWidth still cannot reintroduce runaway growth.
    ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, 0.0f), ImVec2(420.0f, FLT_MAX));
    if (ImGui::BeginPopup("ViewLayersPopup")) {
        DrawViewLayersPopup();
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    if (canvas.HasSelection()) ImGui::Text("Selected entity: %u", canvas.SelectedEntityIdentifier());
    else                       ImGui::TextUnformatted("Selected entity: none");
    // STEP78 — the legend strip + "Preview As" toggle row, drawn above the canvas image itself.
    DrawScenarioEditModeChrome(scenarioEditMode, recipe.armies, recipe.scenarios.maxArmySlotCount);
    const ImVec2 available = ImGui::GetContentRegionAvail();
    const float  fittedSide = std::min(available.x, available.y);
    const float  regionSide = fittedSide > 0.0f ? fittedSide : settings.canvasRegionSidePixels;
    canvas.Draw("mapCanvas", regionSide);
    ImGui::End();
}
```

Everything else in the file (`DrawSettingsWindow`, `DrawActivePanel`) is unchanged.

No `CMakeLists.txt` edit — `src/ui/*.cpp` is already covered by the library's own
`file(GLOB_RECURSE ... "src/ui/*.cpp" "src/ui/*.h")` glob.

---

## ARCH rules invoked
- Constitution §1 (The layers) — UI owns no sim logic; sets params and trips dirty flags. Auto-Level
  writes a plain presentation flag and calls the existing `previewDriver.NotifyParametersChanged()`
  entry point; it neither reimplements nor bypasses the CPU min/max scan `FieldRange` already does.
- ARCH §14.8 (four dirty-flag tiers) — `bAutoDomainFromField` is presentation state no stage hashes;
  the driver derives its own recomposite-only tier from `NotifyParametersChanged()`, exactly like
  every other field-layer domain/ramp edit already wired (`SlopeTab_UI.cpp`, `DrawHeightRampSection`).
  This ticket adds no new dirty-flag path.
- UI_FRAMEWORK_SPEC "Universal widget library" — one shared implementation, one look, DRY: the new
  Auto-Level button draws through the SAME `DrawToggleButton` `RtToggleWidget_UI.cpp` now hosts,
  never a hand-rolled `ImGui::Button` + color-push pair of its own.
- Constitution §8 (Total tweakability) — `tooltipOn`/`tooltipOff`/`buttonWidth` are parameters, not
  literals baked into the draw function; the label is a parameter, not a hardcoded string.
- Constitution §6 (Input & asset safety) — `PreviewFieldLayerOfKind` may return `nullptr` (a
  composite settings object that somehow carries no HeightRamp layer); the toolbar button is
  gated on that pointer, drawing nothing and notifying nothing rather than dereferencing blind.

## Explicit out-of-scope
- **No migration of the other five toggle-shaped controls** the earlier UI audit found
  (`Checkbox_UI`, the `DraggableList` row visibility icon, the Scenarios 3-state cycle button, the
  Marker bundle header buttons) onto `DrawToggleButton` — each is a distinct interaction shape or a
  separate, unratified piece of future work; this ticket only generalizes RT and adds Auto-Level.
- **No Map Areas work of any kind** — independent of STEP211 (Areas field-layer) and STEP212 (Areas
  lock/cursor fix); nothing in this ticket touches `recipe.areas`, `AreasTab_UI.*`, or the canvas
  gesture surface.
- **No View-popup terrain-section per-row Auto-Level control.** `DrawTerrainSection` in
  `Application_ViewLayersPopup_UI.cpp` carries reorder + blend-mode only for EVERY field layer
  (including Slope/Flow/Accumulation, which also have their own `bAutoDomainFromField` and are
  likewise absent from that popup) — there is no existing precedent for a per-row domain-auto
  checkbox there for any layer, so adding one only for HeightRamp would be a new, asymmetric
  pattern, not a fix. The toolbar button is the sole write path for this flag.
- **No matching checkbox added to `DrawHeightRampSection`** (the Heightmap tab's own presentation
  section) either — a second, separate control over the exact same flag would create a two-write-
  paths-one-flag ambiguity the ticket does not need; the toolbar button alone satisfies "expose it
  somewhere," per the background note's own framing of the gap.
- **No rename of `RtToggleWidget_UI.h`/`.cpp`** despite now hosting a generic primitive — the file
  is `#include`d by name in numerous widget translation units; a rename is pure ripple for zero
  behavior change and is left to a future, dedicated pass if the human wants one.
- **No rename of `WidgetStyle::realtimeButtonWidth`/`realtimeActiveColor`** — both fields are reused
  as-is by the new generic function (the color field for its active-state push, the width field
  only by `DrawRealtimeToggleButton`'s own explicit pass-through); renaming either would ripple into
  every widget that already reads them by name.
- **No ARCH-level ratification authored by this ticket** — the `UI_FRAMEWORK_SPEC.md` edit above is
  a documentation note over an already-established pattern (a widget's own draw-path
  generalization), not a new binding rule. If migrating the other five toggle-shaped controls is
  ever mandated rather than merely permitted, that mandate must come from the ARCH Expert.

## Acceptance test
1. Full `SanGenV2` build stays clean; every existing call site of `DrawRealtimeToggleButton`
   (`RangeSliderWidget_UI.cpp`, `LabelledDialWidget_UI.cpp`, `SliderScalar_*_UI.cpp`,
   `Levels_UI.cpp`, `ColorSwatch_UI.cpp`) compiles unmodified.
2. `RtToggleWidget_UI_Test` passes with `ALL PASS`, including the three new STEP213 checks: a plain
   `bool&` flips on the release frame (not the press frame); `DrawRealtimeToggleButton` still flips
   `RealtimeToggle` and keeps its exact `realtimeButtonWidth` on-screen width; a longer label
   auto-sizes wider than that fixed width by default.
3. `SliderScalar_UI_Test` continues to pass unmodified — its own RT-button-width assertion
   (`SliderScalar_UI_Test.cpp:201-221`) still holds after the refactor.
4. With a `PreviewCompositeSettings` carrying the default HeightRamp layer
   (`ConfigureDefaultPreview`), clicking the new "Auto-Level" toolbar button flips
   `heightRampLayer->bAutoDomainFromField` and calls `previewDriver.NotifyParametersChanged()`
   exactly once per click — confirm by code inspection (this ticket adds no new live-imgui test at
   the `Application` shell level; `DrawToggleButton`'s own click contract is already proven by #2).
5. With a hypothetical `PreviewCompositeSettings` carrying no HeightRamp layer at all,
   `PreviewFieldLayerOfKind` returns `nullptr` and the toolbar draws no Auto-Level button and calls
   `previewDriver.NotifyParametersChanged()` zero times (the `if (heightRampLayer != nullptr)` guard).
6. The View popup ("View" button → `ViewLayersPopup`) is visually and functionally unchanged — still
   label + blend-mode combo per terrain row, no new column, no new checkbox.
7. Toggling Auto-Level ON, then regenerating the heightmap (any Heightmap-tab edit), re-derives the
   preview's ramp domain from the freshly baked field's own min/max on the next composite pass
   (`FieldRange` re-runs every `BuildLayerConfigurations()` call, unconditionally on the flag) — no
   code change required to prove this; it falls out of `bAutoDomainFromField` being read fresh every
   frame, the same mechanism Slope already exercises.

## Interpretation calls made
1. **`DrawToggleButton`'s exact signature** deviates from the ticket's own suggested shape by one
   parameter: an explicit `float buttonWidth = 0.0f` (0 = auto-size to the label) rather than
   reusing `WidgetStyle::realtimeButtonWidth` implicitly. Justification: that field is a distinct,
   load-bearing 30px slot several OTHER widgets' row-layout math already subtracts by name; silently
   repurposing its meaning for "whatever width this toggle button wants" would change every one of
   those layouts the moment a caller passed a non-default style. A plain function parameter, `<= 0`
   meaning auto (mirroring `WidgetStyle`'s own `<0`-means-"derive" convention on `trackHeight`/
   `cornerRounding`/`dialRadius`, just expressed as a parameter instead of a style field since this
   one is per-call, not per-control-family), keeps the two concerns (RT's row math vs. a toggle
   button's own content-fit width) independent.
2. **Preserved the exact pre-click tooltip semantics** (captured `bWasEnabled` before applying the
   click) rather than the more "obvious" post-click read a naive refactor would produce — flagged
   explicitly in §2's "Verification against the port source," since it is the one place this
   generalization could have silently changed on-screen behavior on every single click.
3. **Auto-Level's home is the toolbar button alone** — no View-popup per-row control, no duplicate
   checkbox in `DrawHeightRampSection`. Justified in "Explicit out-of-scope" above: the View popup
   carries no domain-auto control for ANY layer today (not even Slope/Flow/Accumulation, which
   already have `bAutoDomainFromField`), so adding one only for HeightRamp would be new and
   asymmetric rather than filling a gap; a second control over the same flag elsewhere would create
   an ambiguous two-writers situation this ticket has no reason to introduce.
4. **No file rename for `RtToggleWidget_UI.h`/`.cpp`** despite the generalization — flagged as a
   deliberate minimal-blast-radius call (CLAUDE.md's own "smallest reusable, hyper-specific units;
   minimal blast radius" law cuts both ways here: the ideal end-state name is arguably
   `ToggleButtonWidget_UI.h`, but renaming a header included by name across the widget library for a
   pure rename with zero behavior change is exactly the kind of diff-noise minimal-blast-radius
   argues against; left as explicit future work if the human wants it).
5. **Added a real headless-imgui-frame test suite to `RtToggleWidget_UI_Test.cpp`** for the new
   `DrawToggleButton`/refactored `DrawRealtimeToggleButton` draw paths, even though
   `DrawRealtimeToggleButton`'s own draw path had NO test at all before this ticket (only the pure
   `RealtimeToggle` state machine did, per `CoreInputWidgets_LiveFrame_UI_Test.cpp`'s own scope and
   the grep confirming no other test references `DrawRealtimeToggleButton`). This was judged worth
   doing (rather than leaving the pre-existing gap in place) because it costs nothing new — every
   `add_sangen_test` target already links `imgui` transitively via `SanGenV2`
   (`CMakeLists.txt:398`,`232`), and `ListWidget_TestFrame_UI.h`'s harness is already the established,
   reused pattern (a dozen other UI test files use it) — not a new testing mechanism being
   introduced. `CoreInputWidgets_LiveFrame_UI_Test.cpp`'s own header comment calling itself "the ONLY
   SanGen test that links imgui" is accordingly already stale as of today's tree (`ListWidgets_UI_Test`
   also explicitly links it, and `SliderScalar_UI_Test`/`MarkersTab_*_UI_Test` etc. exercise real
   imgui frames transitively through `SanGenV2`) — this ticket does not fix that stale comment
   (out of scope; it belongs to whichever ticket last touched `CoreInputWidgets_LiveFrame_UI_Test.cpp`).
6. **`UI_FRAMEWORK_SPEC.md`'s edit is additive documentation only**, not a new rule — flagged
   explicitly per this ticket's own instruction not to author ARCH-level ratification.

## Key files read/cited while drafting
`D:\Projects\Sanctuary\Map Generator\src\ui\RtToggleWidget_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\RtToggleWidget_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\RtToggleWidget_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\WidgetHelpers_UI.h`,
`D:\Projects\Sanctuary\Map Generator\sangen_arch_pack\specs\UI_FRAMEWORK_SPEC.md`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_Settings_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_Prepare_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\SlopeTab_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\TerrainOverlayTab_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\Application_PreviewSetup_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\Application_Draw_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\Application_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\Application_ViewLayersPopup_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\Application_PanelTerrain_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\ListWidget_TestFrame_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\SliderScalar_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MarkersTab_Bundles_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\CoreInputWidgets_LiveFrame_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\sangen_arch_pack\CONSTITUTION.md`,
`D:\Projects\Sanctuary\Map Generator\CMakeLists.txt`,
and `work_orders\STEP210_AreaCanvasGesture_UI.md` used for this document's own structure/rigor.
