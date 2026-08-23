// ScenariosTab_RuntimeScript_UI.cpp — STEP77 Fix §2: the file-based Runtime Script editor.
// Layer: UI. CORRECTION over `DESIGN_ScenariosTabAndLuaEditor_R1.md` §7 (this ticket's own top-of-
// file note): there is no `ScenarioSettings::runtimeScriptText` buffer in the ratified data model —
// editing is ALWAYS editing a real file on disk, either the designer's own override
// (`scenarioRuntimeOverridePath`) or, read-only, the bundled default
// (`Io::LoadScenarioRuntimeText`, STEP72). Touches no `Params::` field.
#include "ScenariosTab_UI.h"
#include "Checkbox_UI.h"
#include "ConfirmDialog_UI.h"
#include "FilePathPicker_UI.h"
#include "../io/FileDialog_IO.h"
#include "../io/FilesystemPrimitives_IO.h"
#include "../io/ScenarioScript_RuntimeResource_IO.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

const Io::FileDialogFilter kLuaOverrideFilters[] = { { "Lua script", "*.lua" }, { "All files", "*.*" } };

// Resolves once when the section is FIRST opened (the one-shot guard) — never crashes, never shows
// an empty silent buffer: a failed resolution is surfaced verbatim instead of an editor.
void EnsureRuntimeScriptLoaded(ScenariosTabState& state) {
    if (state.bRuntimeScriptLoaded) return;
    const std::string overridePath = *state.scenarioRuntimeOverridePath;
    const Io::ScenarioRuntimeResourceResult result =
        Io::LoadScenarioRuntimeText(state.scenarioRuntimeResourceDirectory, overridePath);
    state.bRuntimeScriptResolutionSucceeded = result.bSucceeded;
    state.runtimeScriptResolutionAdvisory   = result.errorMessage;
    state.runtimeScriptEditor.bufferText    = result.runtimeLuaText;
    state.runtimeScriptEditor.bDirty        = false;
    if (result.bSucceeded) ValidateLuaCodeEditorBuffer(state.runtimeScriptEditor);
    state.bRuntimeScriptLoaded             = true;
    state.bRuntimeScriptDiffBannerDismissed = false;   // Fix §2: re-armed on every fresh load
}

// The picker row that appears ONLY while OFF (no override set yet) — Fix §2's "prompts via
// FilePathPicker_UI (.lua fence) for where to save the new override file ... never silently
// invents a path." Picking a path here is what turns the (derived, read-only) toggle ON.
void DrawNewOverridePathPicker(ScenariosTabState& state) {
    FilePathPickerOptions options;
    options.allowedExtensions = ".lua";
    options.browseButtonLabel = "Choose Save Location...";
    options.bClearButtonShown = false;
    std::string scratchPath;   // the row never shows a stored value here — it always names a NEW file
    const FilePathPickerResult result = DrawFilePathPicker("New Override File", scratchPath, options);
    if (!result.bBrowseRequested) return;
    Io::FileDialogRequest request;
    request.title            = "Choose where to save your Runtime Script override";
    request.defaultExtension = ".lua";
    request.filters          = kLuaOverrideFilters;
    request.filterCount      = 2;
    std::string chosenPath;
    if (!Io::FileDialog::SaveFilePath(request, chosenPath) || chosenPath.empty()) return;
    *state.scenarioRuntimeOverridePath = chosenPath;
    state.bRuntimeScriptLoaded = false;   // forces EnsureRuntimeScriptLoaded to reload next frame
}

// Save (Fix §2): enabled only while ON with a path set. Sys::CheckLuaSyntax re-runs ONLY on the
// click itself (belt-and-suspenders against a stale status, never per-frame — the widget's own
// focus-loss/"Validate Now" triggers already keep bLastCheckSucceeded current in the normal case) —
// a failure blocks the save with the SAME gutter/status feedback the editor already shows, no file
// touched.
void DrawSaveButton(ScenariosTabState& state) {
    if (!ImGui::Button("Save Override")) {
        if (!state.runtimeScriptEditor.bLastCheckSucceeded)
            ImGui::TextUnformatted("Fix the syntax error above before saving.");
        return;
    }
    ValidateLuaCodeEditorBuffer(state.runtimeScriptEditor);
    if (!state.runtimeScriptEditor.bLastCheckSucceeded) return;   // blocked; status now shows why
    const std::string& text = state.runtimeScriptEditor.bufferText;
    if (Io::WriteBinaryFileBytes(*state.scenarioRuntimeOverridePath, text.data(), text.size()))
        state.runtimeScriptEditor.bDirty = false;
}

// Reset to bundled default (Fix §2): ConfirmDialog_UI, then CLEARS the path only — never deletes
// the override file itself (MAP_SCENARIO_SPEC.md §2.2 point 4's never-delete posture, by analogy).
void DrawResetControls(ScenariosTabState& state) {
    if (ImGui::Button("Reset to Bundled Default")) state.runtimeScriptResetConfirm.bOpenRequested = true;
    ConfirmDialogOptions options;
    options.title               = "Reset Runtime Script";
    options.bodyText            = "Revert to SanGen's bundled default? Your override file at\n"
                                  + *state.scenarioRuntimeOverridePath + "\nis kept on disk, untouched.";
    options.primaryButtonLabel  = "Reset";
    const ConfirmDialogChange change =
        DrawConfirmDialog("runtimeScriptResetConfirm", state.runtimeScriptResetConfirm, options);
    if (change.bPrimaryClicked) {
        state.scenarioRuntimeOverridePath->clear();
        state.bRuntimeScriptLoaded = false;
    }
}

} // namespace

void DrawScenarioRuntimeScriptSection(ScenariosTabState& state) {
    if (!DrawSectionBegin("Runtime Script (advanced)", state.runtimeScriptSection)) return;
    if (state.scenarioRuntimeOverridePath == nullptr) {
        ImGui::TextUnformatted("Runtime Script editing needs machine-local settings "
                               "(Files tab, STEP64) not wired in this build.");
        DrawSectionEnd();
        return;
    }
    EnsureRuntimeScriptLoaded(state);
    if (!state.bRuntimeScriptResolutionSucceeded) {
        ImGui::TextColored(ImVec4(0.95f, 0.35f, 0.35f, 1.0f), "%s",
                           state.runtimeScriptResolutionAdvisory.c_str());
        DrawSectionEnd();
        return;
    }
    if (!state.runtimeScriptResolutionAdvisory.empty())
        ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.15f, 1.0f), "%s",
                           state.runtimeScriptResolutionAdvisory.c_str());

    const bool bUsingOverride = !state.scenarioRuntimeOverridePath->empty();
    bool toggleDisplay = bUsingOverride;
    ImGui::BeginDisabled(true);   // status only — see this file's own DrawNewOverridePathPicker note
    DrawCheckbox("Use a custom Runtime Script", toggleDisplay);
    ImGui::EndDisabled();

    if (!bUsingOverride) {
        ImGui::TextUnformatted("Editing SanGen's bundled default (read-only).");
        DrawLuaCodeEditor("Runtime Script", state.runtimeScriptEditor, /*bReadOnly=*/true);
        DrawNewOverridePathPicker(state);
    } else {
        DrawScenarioRuntimeScriptDiffBanner(state);   // RuntimeScriptDiff.cpp
        DrawLuaCodeEditor("Runtime Script", state.runtimeScriptEditor, /*bReadOnly=*/false);
        DrawSaveButton(state);
        ImGui::SameLine();
        DrawResetControls(state);
    }
    DrawSectionEnd();
}

} // namespace Ui
} // namespace SanmapGen
