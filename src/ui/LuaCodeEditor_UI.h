// LuaCodeEditor_UI.h — the shared Lua runtime-script code editor widget. Layer: UI. Accuracy
// class: Visual. `ARCH_15_08_ThirdPartyDependencyRuling.md` §15.8: wraps the vendored
// ImGuiColorTextEdit `TextEditor` type, calling `Sys::CheckLuaSyntax` (STEP65) to drive its
// error-marker gutter. It is a SINGLE-INSTANCE editor widget, not a high-cardinality list or
// picker (§15.8's own scoping note) — ordinary per-frame ImGui, never the 100k-entity bypass
// toolkit.
//
// THE SPLIT (ColorSwatch_UI.cpp's convention): this header is plain settings/state, no imgui, no
// third-party type in sight. The vendored `TextEditor` code is isolated entirely to
// LuaCodeEditor_UI.cpp — the ONE translation unit in `src/` allowed to include its header
// (CMakeLists.txt's per-source-file INCLUDE_DIRECTORIES scoping, the same posture STEP65 gave
// LuaSyntaxCheck_SYS.cpp over the LuaJIT headers).
#pragma once
#include <string>

namespace SanmapGen {
namespace Ui {

struct LuaCodeEditorState {
    std::string bufferText;
    bool        bDirty = false;         // buffer differs from what was last loaded/saved
    int         lastCheckedErrorLine = 0;
    std::string lastCheckedErrorMessage;
    bool        bLastCheckSucceeded  = true;
};

// Runs Sys::CheckLuaSyntax over state.bufferText and refreshes lastCheckedErrorLine/
// lastCheckedErrorMessage/bLastCheckSucceeded. Pure/headless — the exact function
// DrawLuaCodeEditor calls internally on a focus-loss transition and the "Validate Now" button,
// exposed here so the trigger's EFFECT is testable with no imgui frame (STEP77 acceptance tests
// 2/3). Never throws; an empty buffer is syntactically valid (bLastCheckSucceeded == true).
void ValidateLuaCodeEditorBuffer(LuaCodeEditorState& state);

// The pure TRIGGER policy behind "validate on focus-loss, never per-keystroke": true exactly on
// the frame the widget WAS focused and no longer is. Headless-testable on its own (STEP77
// acceptance test 1) — DrawLuaCodeEditor is the only caller in real use.
bool ShouldValidateLuaCodeEditorOnFocusTransition(bool bWasFocusedLastFrame, bool bFocusedThisFrame);

// Draws the editor. Runs ValidateLuaCodeEditorBuffer on the frame focus is lost and on an
// explicit "Validate Now" button click — never per-keystroke (avoids thrashing an immediate-mode
// loop for text edited in bursts). Errors surface as an inline gutter marker (ImGuiColorTextEdit's
// native error-marker support) plus a one-line status bar. `bReadOnly` shows the SAME buffer text
// but blocks edits at the vendored TextEditor's own level (its real SetReadOnly, not just an
// ImGui::BeginDisabled dim — a raw ImDrawList-driven widget like this one does not reliably honor
// BeginDisabled's item-flag alone) — a caller wanting the resolved bundled default shown inert
// (ScenariosTab_RuntimeScript_UI.cpp's "OFF" state) passes true. Returns true the frame the buffer
// text actually changed from user editing (never true while bReadOnly).
bool DrawLuaCodeEditor(const char* label, LuaCodeEditorState& state, bool bReadOnly = false);

} // namespace Ui
} // namespace SanmapGen
