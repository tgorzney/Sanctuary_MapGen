// ArmiesTab_UI_Test.cpp — tab-rebuild WO C4 acceptance, part 3: the Armies tab. The check that
// matters is the one v1 never made: removing an army must never leave a unit rule pointing at an
// army that no longer exists, and must never silently re-point it at a DIFFERENT army. That rule
// is pure (ArmiesTab_UI.h), as is the per-army filter the virtualized unit list walks, so the
// binary needs no imgui frame, no window and no GL context.
// NOT YET REGISTERED IN CMake — WO C4 does not own CMakeLists.txt (gate CD-int registers it).
#include "ArmiesTab_UI.h"
#include <cstdio>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

// Four rules across three armies, in recipe order: 0, 1, 0, 2.
std::vector<Params::UnitRule> MakeUnitRules() {
    std::vector<Params::UnitRule> unitRules(4);
    unitRules[0].armyIndex = 0; unitRules[0].count = 10;
    unitRules[1].armyIndex = 1; unitRules[1].count = 11;
    unitRules[2].armyIndex = 0; unitRules[2].count = 12;
    unitRules[3].armyIndex = 2; unitRules[3].count = 13;
    return unitRules;
}

void RunArmyRemovalChecks() {
    std::vector<Params::UnitRule> unitRules = MakeUnitRules();
    Check(DropUnitRulesForRemovedArmy(unitRules, 1), "removing an army with rules reports the move");
    Check(unitRules.size() == 3u, "its own rule is dropped - it can no longer name an owner");
    Check(unitRules[0].armyIndex == 0 && unitRules[1].armyIndex == 0,
          "armies BELOW the removed one keep their index");
    Check(unitRules[2].armyIndex == 1 && unitRules[2].count == 13,
          "and every army above it shifts down one, taking its own rules with it");

    unitRules = MakeUnitRules();
    Check(DropUnitRulesForRemovedArmy(unitRules, 2) && unitRules.size() == 3u,
          "removing the LAST army drops only its rules");
    Check(unitRules[0].armyIndex == 0 && unitRules[1].armyIndex == 1 && unitRules[2].armyIndex == 0,
          "and renumbers nothing, because nothing sat above it");

    unitRules = MakeUnitRules();
    Check(!DropUnitRulesForRemovedArmy(unitRules, -1) && unitRules.size() == 4u,
          "a signal about no army at all changes nothing");
    std::vector<Params::UnitRule> emptyRules;
    Check(!DropUnitRulesForRemovedArmy(emptyRules, 0), "and an empty recipe reports no move");
}

// The unit list is virtualized over an INDEX list rebuilt each frame, because one army's rules are
// not contiguous in the recipe.
void RunUnitRuleFilterChecks() {
    const std::vector<Params::UnitRule> unitRules = MakeUnitRules();
    std::vector<int> ruleIndices;
    ruleIndices.push_back(999);                       // stale content from a previous frame
    CollectUnitRuleIndicesForArmy(unitRules, 0, ruleIndices);
    Check(ruleIndices.size() == 2u && ruleIndices[0] == 0 && ruleIndices[1] == 2,
          "an army's rules are collected in recipe order, and the stale list is cleared first");

    CollectUnitRuleIndicesForArmy(unitRules, 2, ruleIndices);
    Check(ruleIndices.size() == 1u && ruleIndices[0] == 3, "a single-rule army collects one row");
    CollectUnitRuleIndicesForArmy(unitRules, 7, ruleIndices);
    Check(ruleIndices.empty(), "an army with no rules collects none");
    CollectUnitRuleIndicesForArmy(unitRules, -1, ruleIndices);
    Check(ruleIndices.empty(), "and 'no army selected' collects none rather than everything");
}

void RunUnitRuleMirrorChecks() {
    Params::UnitRule rule;
    rule.minSlope = 2.0f; rule.maxSlope = 30.0f; rule.minHeight = 0.1f; rule.maxHeight = 0.9f;
    ArmyUnitListState state;
    LoadUnitRuleValues(rule, state);
    Check(state.slopeValues.minimumValue == 2.0f && state.slopeValues.maximumValue == 30.0f
          && state.heightValues.minimumValue == 0.1f && state.heightValues.maximumValue == 0.9f,
          "both gate bands reach their widget mirrors");
    Check(!StoreUnitRuleValues(state, rule), "storing back what was loaded reports no move");
    state.heightValues.minimumValue = 0.2f;
    Check(StoreUnitRuleValues(state, rule) && rule.minHeight == 0.2f,
          "and a real edit reports the move and lands on the rule");
    Check(state.selectedRuleIndex == -1, "the list opens with nothing selected");
    Check(state.pendingUnitCount >= 1, "and the Add Units picker opens on at least one unit");
}

// SCOPE NOTE 1: an army is presentation state, so these are the only army-side invariants there
// are — the label never renders empty and the selection is fenced.
void RunArmyPresentationChecks() {
    ArmiesTabState state;
    Check(SelectedArmy(state) == nullptr, "an empty tab selects no army");
    state.armies.push_back(ArmyPresentation());
    state.selectedArmyIndex = 0;
    Check(SelectedArmy(state) == &state.armies[0], "the selected army is reachable");
    state.selectedArmyIndex = 1;
    Check(SelectedArmy(state) == nullptr, "one past the last army selects nothing");

    Check(ArmyRowLabel(state.armies[0]) != nullptr && ArmyRowLabel(state.armies[0])[0] != '\0',
          "an unnamed army still draws a label");
    state.armies[0].name = "Left";
    Check(std::string(ArmyRowLabel(state.armies[0])) == "Left", "a named one draws its name");
    Check(state.startingAlloysRange.maximumValue >= 100000.0f
          && state.startingEnergyRange.maximumValue >= 1000000.0f,
          "the resource fields carry the plan's limits");
    Check(kArmyFactionCount == 3, "the plan's three factions are offered");
}

} // namespace

int main() {
    RunArmyRemovalChecks();
    RunUnitRuleFilterChecks();
    RunUnitRuleMirrorChecks();
    RunArmyPresentationChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
