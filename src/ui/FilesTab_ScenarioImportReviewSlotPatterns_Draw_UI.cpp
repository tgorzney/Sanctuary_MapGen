// FilesTab_ScenarioImportReviewSlotPatterns_Draw_UI.cpp — STEP266 §2's slot-patterns subsection:
// one row per extracted `{name, slotPattern}` pair, each with its own target-scenario combo and a
// "Set as selected scenario's slotPattern" button, disabled unless the pick resolves to a
// `PatternScenario` — plus the destructive-overwrite confirm dialog this shape alone needs (§2:
// overwriting a non-empty `slotPattern` is destructive, unlike the additive appends the other two
// shapes use). Split out of FilesTab_ScenarioImportReview_Draw_UI.cpp for the ARCH §1.5 ceiling.
#include "FilesTab_ScenarioImportReview_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {

void DrawScenarioImportReviewSlotPatternRows(ScenarioImportReviewState& state, Params::Scenarios& scenarios,
                                             const std::vector<std::string>& targetLabels) {
    if (state.slotPatternCandidates.empty()) {
        ImGui::TextUnformatted("No slot-pattern candidates from the last import.");
        return;
    }
    for (std::size_t index = 0u; index < state.slotPatternCandidates.size(); ++index) {
        ImGui::PushID(static_cast<int>(index));
        const Io::ScenarioSlotPatternExtractionEntry& candidate = state.slotPatternCandidates[index];
        ImGui::Text("%s: \"%s\"", candidate.name.empty() ? "(unnamed)" : candidate.name.c_str(),
                   candidate.slotPattern.c_str());
        DrawScenarioImportReviewTargetCombo(targetLabels, state.slotPatternTargetComboIndex[index]);
        const ScenarioImportReviewTarget target =
            ResolveScenarioImportReviewTarget(scenarios, state.slotPatternTargetComboIndex[index]);
        Params::PatternScenario* patternTarget = ScenarioImportReviewTargetPatternScenario(scenarios, target);
        ImGui::BeginDisabled(patternTarget == nullptr);
        if (ImGui::Button("Set as selected scenario's slotPattern") && patternTarget != nullptr) {
            if (ScenarioSlotPatternOverwriteNeedsConfirmation(*patternTarget)) {
                state.pendingSlotPatternOverwriteRow = static_cast<int>(index);
                state.slotPatternOverwriteConfirm.bOpenRequested = true;
            } else {
                SetScenarioSlotPatternOnPatternScenario(*patternTarget, candidate.slotPattern);
            }
        }
        ImGui::EndDisabled();
        ImGui::Separator();
        ImGui::PopID();
    }
}

// ONE shared confirm-dialog instance for the whole list (§2's own one-at-a-time posture, mirrors
// FilesTabState::bConfirmActionPending). Re-resolves the pending row's OWN combo pick at confirm
// time rather than caching a raw pointer across frames (`Params::Scenarios`' vectors could
// reallocate; a stale pointer must never be dereferenced).
void DrawScenarioImportReviewSlotPatternOverwriteConfirmDialog(ScenarioImportReviewState& state,
                                                                Params::Scenarios& scenarios) {
    ConfirmDialogOptions options;
    options.title    = "Overwrite Slot Pattern?";
    options.bodyText = "The selected scenario already has a non-empty slotPattern. Overwrite it with "
                       "the imported candidate? This cannot be undone.";
    const ConfirmDialogChange change =
        DrawConfirmDialog("scenarioImportReviewSlotPatternOverwriteConfirm",
                          state.slotPatternOverwriteConfirm, options);
    if (!change.bPrimaryClicked && !change.bSecondaryClicked) return;
    const int row = state.pendingSlotPatternOverwriteRow;
    state.pendingSlotPatternOverwriteRow = -1;
    if (!change.bPrimaryClicked) return;
    if (row < 0 || row >= static_cast<int>(state.slotPatternCandidates.size())) return;
    const ScenarioImportReviewTarget target = ResolveScenarioImportReviewTarget(
        scenarios, state.slotPatternTargetComboIndex[static_cast<std::size_t>(row)]);
    Params::PatternScenario* patternTarget = ScenarioImportReviewTargetPatternScenario(scenarios, target);
    if (patternTarget != nullptr)
        SetScenarioSlotPatternOnPatternScenario(
            *patternTarget, state.slotPatternCandidates[static_cast<std::size_t>(row)].slotPattern);
}

} // namespace Ui
} // namespace SanmapGen
