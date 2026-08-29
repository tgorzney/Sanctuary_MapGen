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
