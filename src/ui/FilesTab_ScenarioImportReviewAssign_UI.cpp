// FilesTab_ScenarioImportReviewAssign_UI.cpp — the pure, headless-testable half of STEP266's
// review/assign panel: no imgui, no window, no GL context. See FilesTab_ScenarioImportReview_UI.h
// for the contract; FilesTab_ScenarioImportReview_Draw_UI.cpp is the only caller.
#include "FilesTab_ScenarioImportReview_UI.h"
#include <utility>

namespace SanmapGen {
namespace Ui {

void ResetScenarioImportReviewState(
        ScenarioImportReviewState& state,
        std::vector<Io::ScenarioMatchConditionCandidate> matchConditionCandidates,
        std::vector<Io::ScenarioSlotPatternExtractionEntry> slotPatternCandidates,
        std::vector<Params::ScenarioUnitPlacement> unitPlacementCandidates) {
    state.matchConditionCandidates = std::move(matchConditionCandidates);
    state.slotPatternCandidates    = std::move(slotPatternCandidates);
    state.unitPlacementCandidates  = std::move(unitPlacementCandidates);
    // Fresh -1 scratch, sized to match — never carries a stale row's target pick forward into a
    // brand-new candidate list (ARCH_15_14 Part B: never inferred).
    state.matchConditionTargetComboIndex.assign(state.matchConditionCandidates.size(), -1);
    state.slotPatternTargetComboIndex.assign(state.slotPatternCandidates.size(), -1);
    state.unitPlacementTargetComboIndex = -1;
    state.pendingSlotPatternOverwriteRow = -1;
    state.slotPatternOverwriteConfirm = ConfirmDialogState();
}

ScenarioImportReviewTarget ResolveScenarioImportReviewTarget(const Params::Scenarios& scenarios,
                                                              int comboIndex) {
    ScenarioImportReviewTarget target;
    if (comboIndex < 0) return target;
    const int patternCount = static_cast<int>(scenarios.patternScenarios.size());
    if (comboIndex < patternCount) {
        target.kind  = ScenarioImportReviewTargetKind::Pattern;
        target.index = comboIndex;
        return target;
    }
    comboIndex -= patternCount;
    const int countCount = static_cast<int>(scenarios.countScenarios.size());
    if (comboIndex < countCount) {
        target.kind  = ScenarioImportReviewTargetKind::Count;
        target.index = comboIndex;
        return target;
    }
    comboIndex -= countCount;
    if (comboIndex == 0) target.kind = ScenarioImportReviewTargetKind::Default;   // index stays -1
    return target;
}

Params::ScenarioBody* ScenarioImportReviewTargetBody(Params::Scenarios& scenarios,
                                                      const ScenarioImportReviewTarget& target) {
    if (target.kind == ScenarioImportReviewTargetKind::Pattern) {
        if (target.index < 0 || target.index >= static_cast<int>(scenarios.patternScenarios.size()))
            return nullptr;
        return &scenarios.patternScenarios[static_cast<std::size_t>(target.index)].body;
    }
    if (target.kind == ScenarioImportReviewTargetKind::Count) {
        if (target.index < 0 || target.index >= static_cast<int>(scenarios.countScenarios.size()))
            return nullptr;
        return &scenarios.countScenarios[static_cast<std::size_t>(target.index)].body;
    }
    if (target.kind == ScenarioImportReviewTargetKind::Default) return &scenarios.defaultScenario;
    return nullptr;
}

Params::CountScenario* ScenarioImportReviewTargetCountScenario(Params::Scenarios& scenarios,
                                                                const ScenarioImportReviewTarget& target) {
    if (target.kind != ScenarioImportReviewTargetKind::Count) return nullptr;
    if (target.index < 0 || target.index >= static_cast<int>(scenarios.countScenarios.size()))
        return nullptr;
    return &scenarios.countScenarios[static_cast<std::size_t>(target.index)];
}

Params::PatternScenario* ScenarioImportReviewTargetPatternScenario(
        Params::Scenarios& scenarios, const ScenarioImportReviewTarget& target) {
    if (target.kind != ScenarioImportReviewTargetKind::Pattern) return nullptr;
    if (target.index < 0 || target.index >= static_cast<int>(scenarios.patternScenarios.size()))
        return nullptr;
    return &scenarios.patternScenarios[static_cast<std::size_t>(target.index)];
}

void AppendScenarioMatchConditionsToCountScenario(
        Params::CountScenario& target, const std::vector<Params::ScenarioCountCondition>& conditions) {
    target.conditions.insert(target.conditions.end(), conditions.begin(), conditions.end());
}

bool ScenarioSlotPatternOverwriteNeedsConfirmation(const Params::PatternScenario& target) {
    return !target.slotPattern.empty();
}

void SetScenarioSlotPatternOnPatternScenario(Params::PatternScenario& target,
                                             const std::string& slotPattern) {
    target.slotPattern = slotPattern;
}

void AppendScenarioUnitPlacementsToBody(Params::ScenarioBody& target,
                                        const std::vector<Params::ScenarioUnitPlacement>& placements) {
    target.unitPlacements.insert(target.unitPlacements.end(), placements.begin(), placements.end());
}

} // namespace Ui
} // namespace SanmapGen
