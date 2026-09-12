// FilesTab_ScenarioImportReview_Draw_UI.cpp — STEP266 §2's top-level composition: the shared
// target-scenario combo helpers, the unit-placements subsection (the simplest of the three shapes,
// one shared batch row), and DrawScenarioImportReviewSection itself. The match-conditions and
// slot-patterns subsections are split into their own translation units for the ARCH §1.5 ceiling
// (FilesTab_ScenarioImportReviewMatchConditions_Draw_UI.cpp /
// FilesTab_ScenarioImportReviewSlotPatterns_Draw_UI.cpp) — declared once in
// FilesTab_ScenarioImportReview_UI.h, called only from here.
#include "FilesTab_ScenarioImportReview_UI.h"
#include "Combo_UI.h"
#include "../params/MapRecipe_PARAMS.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {

std::vector<std::string> BuildScenarioImportReviewTargetLabels(const Params::Scenarios& scenarios) {
    std::vector<std::string> labels;
    labels.reserve(scenarios.patternScenarios.size() + scenarios.countScenarios.size() + 1u);
    for (const Params::PatternScenario& scenario : scenarios.patternScenarios)
        labels.push_back("Pattern: " + (scenario.body.name.empty() ? std::string("(unnamed)") : scenario.body.name));
    for (const Params::CountScenario& scenario : scenarios.countScenarios)
        labels.push_back("Count: " + (scenario.body.name.empty() ? std::string("(unnamed)") : scenario.body.name));
    labels.push_back("Default: " + (scenarios.defaultScenario.name.empty()
                                     ? std::string("(default)") : scenarios.defaultScenario.name));
    return labels;
}

void DrawScenarioImportReviewTargetCombo(const std::vector<std::string>& targetLabels, int& targetComboIndex) {
    std::vector<const char*> labelPointers;
    labelPointers.reserve(targetLabels.size());
    for (const std::string& label : targetLabels) labelPointers.push_back(label.c_str());
    ComboOptions options;
    options.labels = labelPointers.data();
    options.count  = static_cast<int>(labelPointers.size());
    DrawCombo("Target Scenario", targetComboIndex, options);
}

void DrawScenarioImportReviewUnitPlacementRow(ScenarioImportReviewState& state, Params::Scenarios& scenarios,
                                              const std::vector<std::string>& targetLabels) {
    if (state.unitPlacementCandidates.empty()) {
        ImGui::TextUnformatted("No unit-placement candidates from the last import.");
        return;
    }
    ImGui::Text("%d unit placement(s) extracted", static_cast<int>(state.unitPlacementCandidates.size()));
    DrawScenarioImportReviewTargetCombo(targetLabels, state.unitPlacementTargetComboIndex);
    const ScenarioImportReviewTarget target =
        ResolveScenarioImportReviewTarget(scenarios, state.unitPlacementTargetComboIndex);
    Params::ScenarioBody* bodyTarget = ScenarioImportReviewTargetBody(scenarios, target);
    ImGui::BeginDisabled(bodyTarget == nullptr);
    if (ImGui::Button("Append to selected scenario's unit placements") && bodyTarget != nullptr)
        AppendScenarioUnitPlacementsToBody(*bodyTarget, state.unitPlacementCandidates);
    ImGui::EndDisabled();
}

void DrawScenarioImportReviewSection(ScenarioImportReviewState& state, Params::MapRecipe& recipe) {
    if (!DrawSectionBegin("Scenario Import Review (Conditions / Patterns / Placements)", state.section))
        return;
    if (state.matchConditionCandidates.empty() && state.slotPatternCandidates.empty()
            && state.unitPlacementCandidates.empty()) {
        ImGui::TextUnformatted("Run \"Import Scenario Data (Conditions/Patterns/Placements)\" above to "
                               "populate candidates here.");
        DrawSectionEnd();
        return;
    }
    const std::vector<std::string> targetLabels = BuildScenarioImportReviewTargetLabels(recipe.scenarios);
    ImGui::TextUnformatted("Match Conditions");
    DrawScenarioImportReviewMatchConditionRows(state, recipe.scenarios, targetLabels);
    ImGui::Separator();
    ImGui::TextUnformatted("Slot Patterns");
    DrawScenarioImportReviewSlotPatternRows(state, recipe.scenarios, targetLabels);
    ImGui::Separator();
    ImGui::TextUnformatted("Unit Placements");
    DrawScenarioImportReviewUnitPlacementRow(state, recipe.scenarios, targetLabels);
    DrawScenarioImportReviewSlotPatternOverwriteConfirmDialog(state, recipe.scenarios);
    DrawSectionEnd();
}

} // namespace Ui
} // namespace SanmapGen
