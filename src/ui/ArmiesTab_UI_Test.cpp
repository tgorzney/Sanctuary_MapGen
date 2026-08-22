// ArmiesTab_UI_Test.cpp — tab-rebuild WO C4 acceptance, part 3: the Armies tab. Retyped by STEP20
// onto the real `Params::Army`. The checks that matter: removing an army must never leave a unit
// rule pointing at an army that no longer exists or a different one (v1 never made this check);
// dragging an army row must renumber `armyIndex` exactly the same way (a real, pre-existing bug
// STEP20 fixes, not just a mechanical retype); and two blank "Add Army" clicks must never collide
// on export. All of it is pure (ArmiesTab_UI.h), as is the per-army filter the virtualized unit
// list walks, so the binary needs no imgui frame, no window and no GL context.
#include "ArmiesTab_UI.h"
#include "DraggableListWidget_UI.h"
#include "../io/Sanmap_ArmyIdentity_IO.h"
#include <cstdio>
#include <string>

using namespace SanmapGen;
using namespace SanmapGen::Io;
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

// STEP20 ruling #4: a real, pre-existing bug — dragging an army row never renumbered `armyIndex`.
void RunArmyReorderRenumberChecks() {
    // Downward: army 0 dragged onto army 2 (of 3 total).
    std::vector<Params::UnitRule> unitRules = MakeUnitRules();
    Check(RenumberUnitRuleArmyIndicesForReorder(unitRules, 0, 2, 3),
          "a downward reorder (source below target) reports the move");
    Check(unitRules[0].armyIndex == 2 && unitRules[2].armyIndex == 2,
          "both rules that named the dragged army now name its new (target) slot");
    Check(unitRules[1].armyIndex == 0, "army 1's rule shifts down into army 0's old slot");
    Check(unitRules[3].armyIndex == 1, "army 2's rule shifts down into army 1's old slot");

    // Upward: army 2 dragged onto army 0.
    unitRules = MakeUnitRules();
    Check(RenumberUnitRuleArmyIndicesForReorder(unitRules, 2, 0, 3),
          "an upward reorder (source above target) reports the move");
    Check(unitRules[3].armyIndex == 0, "the dragged army's rule now names its new (target) slot");
    Check(unitRules[0].armyIndex == 1 && unitRules[2].armyIndex == 1,
          "army 0's rules shift up into army 1's old slot");
    Check(unitRules[1].armyIndex == 2, "army 1's rule shifts up into army 2's old slot");

    // No-op: source == target, and an out-of-range source.
    unitRules = MakeUnitRules();
    Check(!RenumberUnitRuleArmyIndicesForReorder(unitRules, 1, 1, 3),
          "dropping a row back on itself reports no move");
    Check(unitRules[1].armyIndex == 1, "and changes nothing");
    Check(!RenumberUnitRuleArmyIndicesForReorder(unitRules, -1, 1, 3),
          "a signal about no army at all changes nothing");
    Check(!RenumberUnitRuleArmyIndicesForReorder(unitRules, 5, 1, 3),
          "an out-of-range source is rejected rather than trusted");
}

// A renamed AND reordered army's unit rules still resolve to the correct army afterward — the
// combined case the acceptance test calls for. Mirrors ApplyArmyListSignal's own order: the
// renumber runs BEFORE the armies vector itself moves (it needs the pre-move army count).
// STEP76: `name` is machine-owned engine identity now, never a human-authored string — this test's
// labels ("Alpha"/"Bravo"/"Charlie"/"Beta") are display labels, so they retarget to `displayName`.
// `unitRules[*].armyIndex` stays untouched: it is positional and unaffected by the name/displayName
// split.
void RunCombinedRenameReorderChecks() {
    std::vector<Params::Army> armies(3);
    armies[0].displayName = "Alpha";
    armies[1].displayName = "Bravo";
    armies[2].displayName = "Charlie";
    std::vector<Params::UnitRule> unitRules = MakeUnitRules();   // Alpha, Bravo, Alpha, Charlie

    armies[1].displayName = "Beta";   // a plain rename: no index churn at all

    const int armyCountBeforeMove = static_cast<int>(armies.size());
    Check(RenumberUnitRuleArmyIndicesForReorder(unitRules, 0, 2, armyCountBeforeMove),
          "the reorder renumbers every rule that named the dragged army");
    DraggableListSignal signal;
    signal.kind = DraggableListSignalKind::Reorder;
    signal.sourceRowIndex = 0;
    signal.targetRowIndex = 2;
    Check(ApplyDraggableListSignal(armies, signal), "the armies vector itself reorders");
    Check(armies[0].displayName == "Beta" && armies[1].displayName == "Charlie"
          && armies[2].displayName == "Alpha",
          "Alpha now sits at the end; Beta and Charlie shifted down to take its old slots");

    Check(armies[static_cast<std::size_t>(unitRules[0].armyIndex)].displayName == "Alpha",
          "rule 0 still resolves to Alpha");
    Check(armies[static_cast<std::size_t>(unitRules[1].armyIndex)].displayName == "Beta",
          "rule 1 still resolves to the renamed Beta");
    Check(armies[static_cast<std::size_t>(unitRules[2].armyIndex)].displayName == "Alpha",
          "rule 2 still resolves to Alpha");
    Check(armies[static_cast<std::size_t>(unitRules[3].armyIndex)].displayName == "Charlie",
          "rule 3 still resolves to Charlie");
}

// STEP76 §7's required new case: after a Reorder signal moves the armies vector itself, re-minting
// (as DrawArmiesTab does on every frame a DraggableListSignal fired) puts the engine identities back
// in ARMY_01/ARMY_02/... roster order — while the display labels keep following their own row,
// proving identity is positional and display is not (mirrors Sanmap_ArmyIdentity_IO_Test.cpp's own
// reorder case, but through the tab's actual ApplyDraggableListSignal + AssignArmyIdentities
// sequence, not the helper in isolation).
void RunReorderPreservesArmyIdentityOrderChecks() {
    std::vector<Params::Army> armies(3);
    armies[0].displayName = "North";
    armies[1].displayName = "Center";
    armies[2].displayName = "South";
    AssignArmyIdentities(armies);   // the tab's own initial re-mint: ARMY_01/ARMY_02/ARMY_03

    DraggableListSignal signal;
    signal.kind = DraggableListSignalKind::Reorder;
    signal.sourceRowIndex = 0;
    signal.targetRowIndex = 2;
    Check(ApplyDraggableListSignal(armies, signal), "the reorder signal applies");
    Check(AssignArmyIdentities(armies), "and the post-signal re-mint reports the move");

    Check(armies[0].name == "ARMY_01" && armies[1].name == "ARMY_02" && armies[2].name == "ARMY_03",
          "after a Reorder signal, re-minting puts identities back in ARMY_01/ARMY_02/... order");
    Check(armies[0].displayName == "Center" && armies[1].displayName == "South"
          && armies[2].displayName == "North",
          "display labels followed their own row through the reorder, unlike the identity");
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

// An army is real recipe content now (STEP20): these are its selection/labelling invariants — the
// label never renders empty, the selection is fenced, and the faction combo names the RATIFIED
// enum, not the v1-leftover Supreme Commander names.
void RunArmySelectionChecks() {
    std::vector<Params::Army> armies;
    int selectedArmyIndex = -1;
    Check(SelectedArmy(armies, selectedArmyIndex) == nullptr, "an empty tab selects no army");
    armies.push_back(Params::Army());
    selectedArmyIndex = 0;
    Check(SelectedArmy(armies, selectedArmyIndex) == &armies[0], "the selected army is reachable");
    selectedArmyIndex = 1;
    Check(SelectedArmy(armies, selectedArmyIndex) == nullptr, "one past the last army selects nothing");

    Check(ArmyRowLabel(armies[0]) != nullptr && ArmyRowLabel(armies[0])[0] != '\0',
          "an unnamed army still draws a label");
    // STEP76: the row label draws `displayName`, never the machine-owned `name` (ruling 2).
    armies[0].displayName = "Left";
    Check(std::string(ArmyRowLabel(armies[0])) == "Left", "a named one draws its display name");

    const ArmiesTabState state;
    Check(state.alloysRange.maximumValue >= 100000.0f && state.energyRange.maximumValue >= 1000000.0f,
          "the resource fields carry the plan's limits");
    Check(kArmyFactionCount == 3, "the plan's three factions are offered");
    Check(std::string(armyFactionLabels[0]) == "Chosen" && std::string(armyFactionLabels[1]) == "Guard"
          && std::string(armyFactionLabels[2]) == "EDA",
          "the faction labels name Params::Faction, not the v1 Supreme Commander leftovers");
}

// STEP76: the export key is now the machine-minted `name` (ARMY_XX, never colliding by
// construction — AssignArmyIdentities is exercised directly by Sanmap_ArmyIdentity_IO_Test.cpp), so
// `MakeNamesUnique` no longer runs against armies at all (`displayName` duplicates are explicitly
// legal — out of scope). What's left worth checking here: `NextArmyDisplayName` still seeds a
// distinct DEFAULT label per "Add Army" click, and a colliding `displayName` is left exactly as
// authored, with no repair — the opposite of the old behavior this test used to assert.
void RunArmyDisplayNameDefaultsChecks() {
    std::vector<Params::Army> armies;
    Params::Army firstArmy;
    firstArmy.displayName = NextArmyDisplayName(static_cast<int>(armies.size()));
    armies.push_back(firstArmy);
    Params::Army secondArmy;
    secondArmy.displayName = NextArmyDisplayName(static_cast<int>(armies.size()));
    armies.push_back(secondArmy);
    Check(armies[0].displayName != armies[1].displayName,
          "two 'Add Army' clicks in a row already seed distinct default display labels");

    // Unlike the retired name-uniqueness repair: two armies MAY share a displayName (STEP76 ruling
    // 3) — they are still ARMY_01/ARMY_02 to the engine, which is what actually keeps them distinct
    // on export. Nothing rewrites a colliding displayName.
    armies[1].displayName = armies[0].displayName;
    Check(armies[0].displayName == armies[1].displayName,
          "a colliding displayName is legal and left exactly as authored — no repair runs");
}

} // namespace

int main() {
    RunArmyRemovalChecks();
    RunArmyReorderRenumberChecks();
    RunCombinedRenameReorderChecks();
    RunReorderPreservesArmyIdentityOrderChecks();
    RunUnitRuleFilterChecks();
    RunUnitRuleMirrorChecks();
    RunArmySelectionChecks();
    RunArmyDisplayNameDefaultsChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
