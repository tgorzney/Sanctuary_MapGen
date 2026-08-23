// MapCanvas_ScenarioEditMode_PreviewAs_UI_Test.cpp — the "Preview As" scratch composition
// synthesis (SynthesizeScenarioPreviewAsSlotPattern) and ScenarioEditModeState::Activate's own
// Tier 1 (verbatim) vs Tier 2/3 (synthesized) branching.
#include "MapCanvas_ScenarioEditMode_State_UI.h"
#include "PreviewComposite_TestScene_UI.h"

namespace SanmapGen {
namespace Ui {
namespace {

void Check(bool bCondition, const char* label) { CheckPreviewExpectation(bCondition, label); }

void CheckSynthesisSatisfiesConditions() {
    std::vector<Params::ScenarioCountCondition> conditions;
    Params::ScenarioCountCondition totalCondition;
    totalCondition.field = Params::ScenarioCountField::Total; totalCondition.comparator = Params::ScenarioComparator::Equal;
    totalCondition.value = 2;
    conditions.push_back(totalCondition);
    Params::ScenarioCountCondition humanCondition;
    humanCondition.field = Params::ScenarioCountField::HumanCount; humanCondition.comparator = Params::ScenarioComparator::Equal;
    humanCondition.value = 1;
    conditions.push_back(humanCondition);

    const std::string pattern = SynthesizeScenarioPreviewAsSlotPattern(conditions, 4);
    Check(pattern.size() == 4u, "the synthesized pattern is exactly maxArmySlotCount long");
    int humanCount = 0, aiCount = 0;
    for (const char slotCharacter : pattern) {
        if (slotCharacter == 'h') ++humanCount;
        if (slotCharacter == 'A') ++aiCount;
    }
    Check(humanCount == 1 && aiCount == 1, "the synthesized pattern actually satisfies total==2, human==1 (ai==1)");
}

void CheckSynthesisEmptyConditionsIsVacuouslyTrue() {
    const std::string pattern = SynthesizeScenarioPreviewAsSlotPattern({}, 3);
    Check(pattern == "---", "no conditions (Tier 3's own posture) synthesizes the trivial all-empty composition");
}

void CheckActivateTierBranching() {
    Params::ScenarioBody body;
    ScenarioEditModeState state;
    const std::string patternSlotPattern = "hA-";
    state.Activate(body, &patternSlotPattern, nullptr, 3);
    Check(state.previewAsSlotPattern == "hA-", "Tier 1 defaults Preview As to the pattern verbatim");
    Check(state.IsActive() && state.editedBody == &body, "Activate points editedBody at the given scenario");

    ScenarioEditModeState tier2State;
    std::vector<Params::ScenarioCountCondition> conditions;   // empty -> always matches
    tier2State.Activate(body, nullptr, &conditions, 2);
    Check(tier2State.previewAsSlotPattern == "--", "Tier 2/3 synthesizes instead of copying a pattern");

    tier2State.Deactivate();
    Check(!tier2State.IsActive() && tier2State.editedBody == nullptr, "Deactivate fully releases the slot");
}

} // namespace

void RunScenarioEditModePreviewAsChecks() {
    CheckSynthesisSatisfiesConditions();
    CheckSynthesisEmptyConditionsIsVacuouslyTrue();
    CheckActivateTierBranching();
}

} // namespace Ui
} // namespace SanmapGen
