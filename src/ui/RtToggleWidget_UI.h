// RtToggleWidget_UI.h — the per-control "realtime" wrapper. Layer: UI.
// UI_FRAMEWORK_SPEC §7: while RT is OFF, dragging updates the value every frame but DEFERS the
// expensive recompute until mouse-release, so scrubbing a slider stays at frame rate; while RT
// is ON the commit fires on every change and the caller recomputes live.
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

// Draws the little "RT" button that flips `realtimeToggle`, returns true on the frame it was
// clicked. Consumes exactly one imgui item and no horizontal layout of its own, so a control
// can place it with SameLine (RangeSliderWidget_UI/LabelledDialWidget_UI both do).
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
