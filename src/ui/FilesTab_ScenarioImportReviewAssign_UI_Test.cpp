// FilesTab_ScenarioImportReviewAssign_UI_Test.cpp — STEP266 acceptance for the pure, headless half
// of the review/assign panel (FilesTab_ScenarioImportReviewAssign_UI.cpp): target resolution, the
// three shapes' own Append/Set functions, and the slot-pattern overwrite confirm gate. No imgui
// frame, no window, no GL context — same posture as every other Files-tab test unit.
#include "FilesTab_ScenarioImportReview_UI.h"
#include "FilesTab_TestSupport_UI.h"
#include "../params/Scenario_PARAMS.h"

namespace SanmapGen {
namespace FilesTabTest {
namespace {

Params::Scenarios BuildFixtureScenarios() {
    Params::Scenarios scenarios;
    Params::PatternScenario pattern;
    pattern.body.name = "PatternA";
    pattern.slotPattern = "hhhh------------";
    scenarios.patternScenarios.push_back(pattern);
    Params::CountScenario count;
    count.body.name = "CountA";
    scenarios.countScenarios.push_back(count);
    scenarios.defaultScenario.name = "DefaultX";
    return scenarios;
}

// ResolveScenarioImportReviewTarget decodes the flattened combo index in the documented order:
// pattern rows, then count rows, then exactly one default row, out-of-range -> None.
void CheckResolveScenarioImportReviewTargetDecodesFlattenedOrder() {
    const Params::Scenarios scenarios = BuildFixtureScenarios();
    const Ui::ScenarioImportReviewTarget patternTarget = Ui::ResolveScenarioImportReviewTarget(scenarios, 0);
    Check(patternTarget.kind == Ui::ScenarioImportReviewTargetKind::Pattern && patternTarget.index == 0,
          "index 0 resolves to the one PatternScenario");

    const Ui::ScenarioImportReviewTarget countTarget = Ui::ResolveScenarioImportReviewTarget(scenarios, 1);
    Check(countTarget.kind == Ui::ScenarioImportReviewTargetKind::Count && countTarget.index == 0,
          "index 1 resolves to the one CountScenario");

    const Ui::ScenarioImportReviewTarget defaultTarget = Ui::ResolveScenarioImportReviewTarget(scenarios, 2);
    Check(defaultTarget.kind == Ui::ScenarioImportReviewTargetKind::Default,
          "index 2 resolves to the default scenario");

    const Ui::ScenarioImportReviewTarget outOfRange = Ui::ResolveScenarioImportReviewTarget(scenarios, 3);
    Check(outOfRange.kind == Ui::ScenarioImportReviewTargetKind::None,
          "an out-of-range index resolves to None rather than reading off the end");

    const Ui::ScenarioImportReviewTarget negative = Ui::ResolveScenarioImportReviewTarget(scenarios, -1);
    Check(negative.kind == Ui::ScenarioImportReviewTargetKind::None,
          "a negative (never-picked) index resolves to None too");
}

// Work-order test 2: appending a candidate match-condition-set to a chosen CountScenario adds
// exactly that vector to body.conditions, leaving every OTHER scenario's conditions untouched.
void CheckAppendMatchConditionsOnlyTouchesChosenScenario() {
    Params::Scenarios scenarios = BuildFixtureScenarios();
    scenarios.countScenarios.push_back(Params::CountScenario());   // a second, untouched scenario
    scenarios.countScenarios[1].body.name = "CountB";
    Params::ScenarioCountCondition preExisting;
    preExisting.value = 7;
    scenarios.countScenarios[0].conditions.push_back(preExisting);   // append, never overwrite

    std::vector<Params::ScenarioCountCondition> candidateConditions;
    Params::ScenarioCountCondition candidate;
    candidate.field = Params::ScenarioCountField::HumanCount;
    candidate.value = 2;
    candidateConditions.push_back(candidate);

    Ui::AppendScenarioMatchConditionsToCountScenario(scenarios.countScenarios[0], candidateConditions);
    Check(scenarios.countScenarios[0].conditions.size() == 2,
          "the pre-existing condition survives and the candidate is appended after it");
    Check(scenarios.countScenarios[0].conditions[0].value == 7 && scenarios.countScenarios[0].conditions[1].value == 2,
          "append order is preserved (pre-existing first, candidate trailing)");
    Check(scenarios.countScenarios[1].conditions.empty(),
          "the OTHER CountScenario's conditions are completely untouched");
}

// Work-order test 3: overwriting a PatternScenario that already has a non-empty slotPattern is
// flagged as needing confirmation; an empty slotPattern needs none.
void CheckSlotPatternOverwriteConfirmationGate() {
    Params::PatternScenario nonEmptyTarget;
    nonEmptyTarget.slotPattern = "hhhh------------";
    Check(Ui::ScenarioSlotPatternOverwriteNeedsConfirmation(nonEmptyTarget),
          "a target with an existing non-empty slotPattern requires confirmation");

    Params::PatternScenario emptyTarget;
    Check(!Ui::ScenarioSlotPatternOverwriteNeedsConfirmation(emptyTarget),
          "a target with an empty slotPattern needs no confirmation");

    // SetScenarioSlotPatternOnPatternScenario itself is the unconditional overwrite; the gate above
    // is the caller's own job (the draw unit routes through a confirm dialog first).
    Ui::SetScenarioSlotPatternOnPatternScenario(nonEmptyTarget, "----hhhh--------");
    Check(nonEmptyTarget.slotPattern == "----hhhh--------", "the overwrite itself always applies verbatim");
}

// Work-order test 4: appending a candidate unit-placement batch to a chosen scenario adds exactly
// those rows to body.unitPlacements, preserving whatever was already there.
void CheckAppendUnitPlacementsAddsExactRows() {
    Params::ScenarioBody body;
    Params::ScenarioUnitPlacement preExisting;
    preExisting.armyName = "ARMY_01";
    body.unitPlacements.push_back(preExisting);

    std::vector<Params::ScenarioUnitPlacement> candidateBatch;
    Params::ScenarioUnitPlacement first;  first.armyName = "ARMY_02";
    Params::ScenarioUnitPlacement second; second.armyName = "ARMY_03";
    candidateBatch.push_back(first);
    candidateBatch.push_back(second);

    Ui::AppendScenarioUnitPlacementsToBody(body, candidateBatch);
    Check(body.unitPlacements.size() == 3, "the pre-existing row survives and both candidates are appended");
    Check(body.unitPlacements[0].armyName == "ARMY_01" && body.unitPlacements[1].armyName == "ARMY_02"
              && body.unitPlacements[2].armyName == "ARMY_03",
          "append order is preserved, verbatim");
}

// ResetScenarioImportReviewState replaces every candidate/scratch field wholesale, with fresh -1
// combo scratch sized to match — never carries a stale row's target pick forward.
void CheckResetScenarioImportReviewStateSeedsFreshScratch() {
    Ui::ScenarioImportReviewState state;
    state.matchConditionTargetComboIndex = { 4, 5 };   // stale, from a PREVIOUS import
    state.pendingSlotPatternOverwriteRow = 2;
    state.slotPatternOverwriteConfirm.bOpenRequested = true;

    std::vector<Io::ScenarioMatchConditionCandidate> matchConditions(1);
    std::vector<Io::ScenarioSlotPatternExtractionEntry> slotPatterns(3);
    std::vector<Params::ScenarioUnitPlacement> unitPlacements(2);
    Ui::ResetScenarioImportReviewState(state, matchConditions, slotPatterns, unitPlacements);

    Check(state.matchConditionCandidates.size() == 1, "match-condition candidates replaced wholesale");
    Check(state.slotPatternCandidates.size() == 3, "slot-pattern candidates replaced wholesale");
    Check(state.unitPlacementCandidates.size() == 2, "unit-placement candidates replaced wholesale");
    Check(state.matchConditionTargetComboIndex.size() == 1 && state.matchConditionTargetComboIndex[0] == -1,
          "match-condition combo scratch is resized and reset to -1, never carried forward stale");
    Check(state.slotPatternTargetComboIndex.size() == 3 && state.slotPatternTargetComboIndex[0] == -1,
          "slot-pattern combo scratch is resized and reset to -1 too");
    Check(state.unitPlacementTargetComboIndex == -1, "the shared unit-placement combo scratch resets to -1");
    Check(state.pendingSlotPatternOverwriteRow == -1, "no row is pending confirmation after a fresh import");
    Check(!state.slotPatternOverwriteConfirm.bOpenRequested,
          "a stale open-requested confirm dialog from a previous import is not carried forward");
}

} // namespace

void RunScenarioImportReviewAssignTests() {
    CheckResolveScenarioImportReviewTargetDecodesFlattenedOrder();
    CheckAppendMatchConditionsOnlyTouchesChosenScenario();
    CheckSlotPatternOverwriteConfirmationGate();
    CheckAppendUnitPlacementsAddsExactRows();
    CheckResetScenarioImportReviewStateSeedsFreshScratch();
}

} // namespace FilesTabTest
} // namespace SanmapGen
