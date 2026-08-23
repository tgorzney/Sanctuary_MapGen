// LuaCodeEditor_UI.cpp — the vendored ImGuiColorTextEdit wiring. Layer: UI.
// ONLY this translation unit includes TextEditor.h (`ARCH_15_08_ThirdPartyDependencyRuling.md`
// §15.8, CMakeLists.txt's per-source-file INCLUDE_DIRECTORIES scoping).
//
// TextEditor instances are HEAVY (its own line/undo storage) and the header above is
// deliberately a plain std::string bag (THE SPLIT), so this file keeps one TextEditor per
// caller-owned LuaCodeEditorState, keyed by that state's OWN address — never a single function
// static shared across every call site (Section_UI.h's "the v1 bug the widget library exists to
// kill"): two distinct LuaCodeEditorState instances (e.g. the Scenarios tab's editor and a test's
// scratch state) always get two distinct TextEditor objects, never accidental cross-talk.
#include "LuaCodeEditor_UI.h"
#include "../sys/LuaSyntaxCheck_SYS.h"
#include "TextEditor.h"
#include "imgui.h"
#include <unordered_map>

namespace SanmapGen {
namespace Ui {
namespace {

struct LuaCodeEditorInstance {
    TextEditor  editor;
    std::string lastSyncedText;         // what the editor's own buffer last agreed with state
    bool        bWasFocusedLastFrame = false;
    bool        bInitialized         = false;
};

std::unordered_map<const LuaCodeEditorState*, LuaCodeEditorInstance>& EditorInstances() {
    static std::unordered_map<const LuaCodeEditorState*, LuaCodeEditorInstance> instances;
    return instances;
}

LuaCodeEditorInstance& InstanceFor(const LuaCodeEditorState& state) {
    LuaCodeEditorInstance& instance = EditorInstances()[&state];
    if (!instance.bInitialized) {
        instance.editor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
        instance.editor.SetPalette(TextEditor::GetDarkPalette());
        instance.editor.SetShowWhitespaces(false);
        instance.bInitialized = true;
    }
    return instance;
}

} // namespace

void ValidateLuaCodeEditorBuffer(LuaCodeEditorState& state) {
    const Sys::LuaSyntaxCheckResult result = Sys::CheckLuaSyntax(state.bufferText);
    state.bLastCheckSucceeded  = result.bSucceeded;
    state.lastCheckedErrorLine = result.lineNumber;
    state.lastCheckedErrorMessage = result.message;
}

bool ShouldValidateLuaCodeEditorOnFocusTransition(bool bWasFocusedLastFrame, bool bFocusedThisFrame) {
    return bWasFocusedLastFrame && !bFocusedThisFrame;
}

bool DrawLuaCodeEditor(const char* label, LuaCodeEditorState& state, bool bReadOnly) {
    LuaCodeEditorInstance& instance = InstanceFor(state);
    TextEditor& editor = instance.editor;
    editor.SetReadOnly(bReadOnly);

    // External change (a fresh file load, a programmatic reset) — never mistaken for the user's
    // own typing, which is written back through the SAME field below.
    if (instance.lastSyncedText != state.bufferText) {
        editor.SetText(state.bufferText);
        instance.lastSyncedText = state.bufferText;
    }

    TextEditor::ErrorMarkers markers;
    if (!state.bLastCheckSucceeded && state.lastCheckedErrorLine > 0)
        markers[state.lastCheckedErrorLine] = state.lastCheckedErrorMessage;
    editor.SetErrorMarkers(markers);

    ImGui::PushID(label);
    ImGui::TextUnformatted(label);
    ImGui::SameLine();
    ImGui::BeginDisabled(bReadOnly);
    if (ImGui::SmallButton("Validate Now")) ValidateLuaCodeEditorBuffer(state);
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (!state.bLastCheckSucceeded)
        ImGui::TextColored(ImVec4(0.95f, 0.35f, 0.35f, 1.0f), "Line %d: %s", state.lastCheckedErrorLine,
                           state.lastCheckedErrorMessage.c_str());
    else
        ImGui::TextColored(ImVec4(0.45f, 0.85f, 0.45f, 1.0f), "Syntax OK");

    ImGui::BeginChild("luaEditorChild", ImVec2(0.0f, 300.0f), true, ImGuiWindowFlags_HorizontalScrollbar);
    editor.SetImGuiChildIgnored(true);   // this child IS the editor's own — no nested child
    editor.Render("##luaEditorInner");
    const bool bFocusedThisFrame = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
    ImGui::EndChild();

    bool bTextChangedThisFrame = false;
    if (editor.IsTextChanged()) {
        state.bufferText = editor.GetText();
        instance.lastSyncedText = state.bufferText;
        state.bDirty = true;
        bTextChangedThisFrame = true;
    }

    if (ShouldValidateLuaCodeEditorOnFocusTransition(instance.bWasFocusedLastFrame, bFocusedThisFrame))
        ValidateLuaCodeEditorBuffer(state);
    instance.bWasFocusedLastFrame = bFocusedThisFrame;

    ImGui::PopID();
    return bTextChangedThisFrame;
}

} // namespace Ui
} // namespace SanmapGen
