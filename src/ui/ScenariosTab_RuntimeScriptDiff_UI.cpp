// ScenariosTab_RuntimeScriptDiff_UI.cpp — STEP77 Fix §2's simplified bundled-vs-override diff
// banner, split out of ScenariosTab_RuntimeScript_UI.cpp for the Constitution §1.5 ceiling.
// Layer: UI.
//
// ⚠️ SIMPLIFIED from the original design's "newer bundled" framing (which assumed a version/hash
// baseline this data model has no field to store — flagged to ARCH/IO, not built here): this only
// asks "does the override's text differ from the bundled text RIGHT NOW". Any real customization
// trips this banner every fresh load — honest given what's knowable, not a regression to hide.
#include "ScenariosTab_UI.h"
#include "../io/ScenarioScript_RuntimeResource_IO.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {

void DrawScenarioRuntimeScriptDiffBanner(ScenariosTabState& state) {
    if (state.bRuntimeScriptDiffBannerDismissed) return;
    const Io::ScenarioRuntimeResourceResult bundled =
        Io::LoadScenarioRuntimeText(state.scenarioRuntimeResourceDirectory, std::string());
    if (!bundled.bSucceeded || bundled.runtimeLuaText == state.runtimeScriptEditor.bufferText) return;
    ImGui::TextWrapped("Your Runtime Script override differs from SanGen's current bundled default.");
    if (ImGui::SmallButton("View Bundled Default")) state.bViewingBundledDefaultPanel = true;
    ImGui::SameLine();
    if (ImGui::SmallButton("Dismiss")) state.bRuntimeScriptDiffBannerDismissed = true;
    if (!state.bViewingBundledDefaultPanel) return;
    ImGui::TextUnformatted("Bundled default (read-only; no auto-merge):");
    ImGui::BeginChild("runtimeScriptBundledPreview", ImVec2(0.0f, 150.0f), true,
                      ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::TextUnformatted(bundled.runtimeLuaText.c_str());
    ImGui::EndChild();
    if (ImGui::SmallButton("Close Preview")) state.bViewingBundledDefaultPanel = false;
}

} // namespace Ui
} // namespace SanmapGen
