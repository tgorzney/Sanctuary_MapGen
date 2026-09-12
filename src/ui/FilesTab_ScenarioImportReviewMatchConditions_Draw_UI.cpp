// FilesTab_ScenarioImportReviewMatchConditions_Draw_UI.cpp — STEP266 §2's match-conditions
// subsection: one row per extracted `ScenarioMatchConditionCandidate`, each with its own
// target-scenario combo and an "Append to selected scenario's conditions" button, disabled unless
// the current pick resolves to a `CountScenario` (only that type has a `conditions` field). Split
// out of FilesTab_ScenarioImportReview_Draw_UI.cpp for the ARCH §1.5 ceiling.
#include "FilesTab_ScenarioImportReview_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {

void DrawScenarioImportReviewMatchConditionRows(ScenarioImportReviewState& state, Params::Scenarios& scenarios,
                                                const std::vector<std::string>& targetLabels) {
    if (state.matchConditionCandidates.empty()) {
        ImGui::TextUnformatted("No match-condition candidates from the last import.");
        return;
    }
    for (std::size_t index = 0u; index < state.matchConditionCandidates.size(); ++index) {
        ImGui::PushID(static_cast<int>(index));
        const Io::ScenarioMatchConditionCandidate& candidate = state.matchConditionCandidates[index];
        ImGui::Text("%s (%d condition(s))",
                   candidate.identifier.empty() ? "(no context)" : candidate.identifier.c_str(),
                   static_cast<int>(candidate.conditions.size()));
        DrawScenarioImportReviewTargetCombo(targetLabels, state.matchConditionTargetComboIndex[index]);
        const ScenarioImportReviewTarget target =
            ResolveScenarioImportReviewTarget(scenarios, state.matchConditionTargetComboIndex[index]);
        Params::CountScenario* countTarget = ScenarioImportReviewTargetCountScenario(scenarios, target);
        ImGui::BeginDisabled(countTarget == nullptr);
        if (ImGui::Button("Append to selected scenario's conditions") && countTarget != nullptr)
            AppendScenarioMatchConditionsToCountScenario(*countTarget, candidate.conditions);
        ImGui::EndDisabled();
        ImGui::Separator();
        ImGui::PopID();
    }
}

} // namespace Ui
} // namespace SanmapGen
