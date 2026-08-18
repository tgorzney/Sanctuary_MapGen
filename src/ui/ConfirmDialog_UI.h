// ConfirmDialog_UI.h — a generic, reusable OK/Cancel confirm popup. Layer: UI. Accuracy class:
// Visual. UI_FRAMEWORK_SPEC "Universal widget library" (STEP5_PropsDecalsValidation_UI's first
// consumer is the Files tab's blueprintPath warning, but this is a widget-library primitive, not
// a one-off — matches the ColorSwatch_UI/Combo_UI convention of one generic control per concern).
//
// Human-ratified UX: OK exports/continues anyway (the caller's call), Cancel aborts with nothing
// done — so the options are OK + Cancel, never OK-only, and (per bClosableWithoutChoice) never a
// silent ESC/backdrop/X dismissal unless the caller explicitly allows one.
//
// Everything here is a pure settings/result struct (WidgetHelpers_UI.h "THE SPLIT"); only
// ConfirmDialog_UI.cpp includes imgui.
#pragma once
#include <string>

namespace SanmapGen {
namespace Ui {

// Caller pre-formats bodyText (multi-line ok) — the widget invents no wording of its own.
struct ConfirmDialogOptions {
    std::string title;
    std::string bodyText;
    std::string primaryButtonLabel   = "OK";
    std::string secondaryButtonLabel = "Cancel";
    bool        bClosableWithoutChoice = false;   // false: no ESC/backdrop/X — a button must be clicked
};

// Caller-owned, one instance per call site — the widget holds no state of its own (ARCH §3.2).
struct ConfirmDialogState {
    bool bOpenRequested = false;
};

// One frame of interaction. Neither flag is set on a frame where the popup stays open with no
// button pressed yet.
struct ConfirmDialogChange {
    bool bPrimaryClicked   = false;
    bool bSecondaryClicked = false;
};

// `identifier` is the imgui popup id (ImGui::PushID scope, not shown) — distinguishes multiple
// confirm dialogs coexisting in the same frame. Call unconditionally, every frame the caller wants
// the dialog reachable: it opens the popup the frame `state.bOpenRequested` is true and clears the
// flag, then draws nothing further once the popup itself has closed.
ConfirmDialogChange DrawConfirmDialog(const char* identifier, ConfirmDialogState& state,
                                      const ConfirmDialogOptions& options);

} // namespace Ui
} // namespace SanmapGen
