// ScenariosTab_UI_Test.cpp — STEP74 acceptance: the pure, headless-testable half of the Scenarios
// tab (ScenarioNeedsSpawnsAcknowledgment, ArmiesExceedingSlotCount, the slot-pattern round trip, the
// AND-of-clauses evaluator, reachability/shadowing, the priority badge, Duplicate, and the Default
// panel's selection semantics). No imgui frame, no window, no GL context.
#include "ScenariosTab_UI.h"
#include "ScenariosTab_ListMechanics_UI.h"
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

Params::Army MakeArmy(const char* name) {
    Params::Army army;
    army.name = name;
    return army;
}

// 1. ScenarioNeedsSpawnsAcknowledgment truth table.
void RunSpawnsAcknowledgmentChecks() {
    Params::ScenarioBody body;
    Check(ScenarioNeedsSpawnsAcknowledgment(body), "empty spawns + empty note needs acknowledgment");
    body.authoringNote = "intentional";
    Check(!ScenarioNeedsSpawnsAcknowledgment(body), "empty spawns + a note does not");
    body.spawns.push_back(Params::ScenarioSpawn());
    Check(!ScenarioNeedsSpawnsAcknowledgment(body), "non-empty spawns + a note does not");
    body.authoringNote.clear();
    Check(!ScenarioNeedsSpawnsAcknowledgment(body), "non-empty spawns + no note still does not");
}

// 2. ArmiesExceedingSlotCount: indices, negative safety.
void RunArmiesExceedingSlotCountChecks() {
    std::vector<Params::Army> armies;
    for (int index = 0; index < 5; ++index) armies.push_back(MakeArmy(("Army" + std::to_string(index)).c_str()));

    const std::vector<std::string> exceeding = ArmiesExceedingSlotCount(armies, 3);
    Check(exceeding.size() == 2u && exceeding[0] == "Army3" && exceeding[1] == "Army4",
          "maxArmySlotCount 3 over a 5-army roster names exactly indices 3/4, in order");
    Check(ArmiesExceedingSlotCount(armies, 5).empty(), "a slot count covering the whole roster names nobody");
    Check(ArmiesExceedingSlotCount(armies, 99).empty(), "a slot count well above the roster names nobody");
    Check(ArmiesExceedingSlotCount(armies, -3).size() == 5u,
          "a negative slot count is treated as 0 - never crashes, never negative-indexes");
}

// 3. Slot-pattern toggle round trip, plus pad/truncate degradation.
void RunSlotPatternRoundTripChecks() {
    const std::string mixed = "h---A--hhAA-h-h";   // 15 chars, one short of 16
    Check(BuildSlotPatternFromToggles(ParseSlotPatternToToggles(mixed, 16)).size() == 16u,
          "parsing pads a short pattern out to maxArmySlotCount");
    Check(BuildSlotPatternFromToggles(ParseSlotPatternToToggles(mixed, 16)).back() == '-',
          "the padded tail is '-', never garbage");
    const std::string exact = "hA--hA--hA--hA--";   // 16 chars
    Check(BuildSlotPatternFromToggles(ParseSlotPatternToToggles(exact, 16)) == exact,
          "an exact-length pattern round-trips byte for byte");
    const std::string longPattern = exact + "hAhA";   // 20 chars
    const std::vector<char> truncated = ParseSlotPatternToToggles(longPattern, 16);
    Check(truncated.size() == 16u, "a longer stored pattern truncates to maxArmySlotCount, never crashes");
    Check(BuildSlotPatternFromToggles(truncated) == exact, "and the kept prefix matches exactly");
}

// 4. MatchesScenarioConditions: all 6 comparators x all 3 fields, vacuous empty conjunction.
void RunConditionMatchingChecks() {
    Check(MatchesScenarioConditions({}, 7, 3, 4), "an empty conjunction is vacuously true");

    const struct { Params::ScenarioComparator comparator; int value; bool bExpectMatch; } cases[] = {
        { Params::ScenarioComparator::Equal,          5, true  },
        { Params::ScenarioComparator::Equal,          6, false },
        { Params::ScenarioComparator::NotEqual,       6, true  },
        { Params::ScenarioComparator::NotEqual,       5, false },
        { Params::ScenarioComparator::GreaterThan,    4, true  },
        { Params::ScenarioComparator::GreaterThan,    5, false },
        { Params::ScenarioComparator::GreaterOrEqual, 5, true  },
        { Params::ScenarioComparator::GreaterOrEqual, 6, false },
        { Params::ScenarioComparator::LessThan,       6, true  },
        { Params::ScenarioComparator::LessThan,       5, false },
        { Params::ScenarioComparator::LessOrEqual,    5, true  },
        { Params::ScenarioComparator::LessOrEqual,    4, false },
    };
    for (const auto& testCase : cases) {
        for (const Params::ScenarioCountField field :
             { Params::ScenarioCountField::Total, Params::ScenarioCountField::HumanCount, Params::ScenarioCountField::AiCount }) {
            Params::ScenarioCountCondition condition;
            condition.field = field; condition.comparator = testCase.comparator; condition.value = testCase.value;
            const bool bMatched = MatchesScenarioConditions({ condition }, 5, 5, 5);
            Check(bMatched == testCase.bExpectMatch, "comparator/field truth table entry");
        }
    }
    // AND semantics: both clauses must pass.
    Params::ScenarioCountCondition totalIsFive; totalIsFive.field = Params::ScenarioCountField::Total; totalIsFive.value = 5;
    Params::ScenarioCountCondition humanIsTwo;
    humanIsTwo.field = Params::ScenarioCountField::HumanCount; humanIsTwo.value = 2;
    Check(MatchesScenarioConditions({ totalIsFive, humanIsTwo }, 5, 2, 3), "both clauses satisfied matches");
    Check(!MatchesScenarioConditions({ totalIsFive, humanIsTwo }, 5, 1, 4), "one clause failing fails the AND");
}

Params::CountScenario MakeCountScenario(const char* name, Params::ScenarioCountField field,
                                        Params::ScenarioComparator comparator, int value) {
    Params::CountScenario scenario;
    scenario.body.name = name;
    Params::ScenarioCountCondition condition;
    condition.field = field; condition.comparator = comparator; condition.value = value;
    scenario.conditions.push_back(condition);
    return scenario;
}

// 5. IsCountScenarioReachable: the exact shadowing case MAP_SCENARIO_SPEC.md §4 names.
void RunReachabilityChecks() {
    Params::Scenarios scenarios;
    scenarios.maxArmySlotCount = 8;
    scenarios.countScenarios.push_back(
        MakeCountScenario("Exactly3", Params::ScenarioCountField::Total, Params::ScenarioComparator::Equal, 3));
    scenarios.countScenarios.push_back(
        MakeCountScenario("AnyTotal", Params::ScenarioCountField::Total, Params::ScenarioComparator::GreaterOrEqual, 0));
    Check(IsCountScenarioReachable(scenarios, 0), "checked-first entry wins the triples it matches");
    Check(IsCountScenarioReachable(scenarios, 1), "the catch-all second entry still wins every other triple");

    Params::Scenarios shadowed;
    shadowed.maxArmySlotCount = 8;
    shadowed.countScenarios.push_back(
        MakeCountScenario("AnyTotal", Params::ScenarioCountField::Total, Params::ScenarioComparator::GreaterOrEqual, 0));
    shadowed.countScenarios.push_back(
        MakeCountScenario("Exactly3", Params::ScenarioCountField::Total, Params::ScenarioComparator::Equal, 3));
    Check(IsCountScenarioReachable(shadowed, 0), "the reordered catch-all, now first, is reachable");
    Check(!IsCountScenarioReachable(shadowed, 1),
          "and the exact-3 rule behind it is unreachable - shadowed by the earlier catch-all");

    // `scenarios`'s two rules together cover total==3 (rule 0) and everything else (rule 1's
    // total>=0) - nothing ever falls through, so default is unreachable there.
    Check(!IsDefaultScenarioReachable(scenarios), "a catch-all count rule leaves default unreachable");
    Params::Scenarios onlyExactly3;
    onlyExactly3.maxArmySlotCount = 8;
    onlyExactly3.countScenarios.push_back(
        MakeCountScenario("Exactly3", Params::ScenarioCountField::Total, Params::ScenarioComparator::Equal, 3));
    Check(IsDefaultScenarioReachable(onlyExactly3),
          "with only a narrow count rule, every other total still falls through to default");
}

// 6. ScenarioPriorityBadge ordinal labels.
void RunPriorityBadgeChecks() {
    Check(ScenarioPriorityBadge(0) == "1st checked", "row 0 reads 1st checked");
    Check(ScenarioPriorityBadge(1) == "2nd checked", "row 1 reads 2nd checked");
    Check(ScenarioPriorityBadge(2) == "3rd checked", "row 2 reads 3rd checked");
    Check(ScenarioPriorityBadge(3) == "4th checked", "row 3 reads 4th checked");
    Check(ScenarioPriorityBadge(10) == "11th checked", "row 10 (11th) is the 'teen' exception, not 11st");
}

// 7. Duplicate inserts an equal-content copy at index 1, names uniquified.
void RunDuplicateChecks() {
    std::vector<Params::PatternScenario> patterns;
    Params::PatternScenario original;
    original.body.name = "Opening";
    original.slotPattern = "hA--";
    patterns.push_back(original);
    Params::PatternScenario second;
    second.body.name = "Second";
    patterns.push_back(second);

    ScenariosTabState state;
    state.selectedIndex = 0;
    DuplicateScenario(patterns, 0, state);
    Check(patterns.size() == 3u, "duplicate inserts one new row");
    Check(patterns[1].slotPattern == "hA--", "the copy carries the original's content");
    Check(patterns[1].body.name == "Opening_1", "and its name is uniquified against the original");
    Check(state.selectedIndex == 1, "selection follows the freshly duplicated row");
}

// 8. Selecting the Default panel sets selectedTier == Default; selectedIndex is never consulted.
void RunDefaultTierSelectionChecks() {
    ScenariosTabState state;
    state.selectedTier = ScenarioSelectedTier::Pattern;
    state.selectedIndex = 7;   // deliberately stale/garbage
    SelectScenarioDefaultTier(state);
    Check(state.selectedTier == ScenarioSelectedTier::Default, "the Default panel is now selected");
    Check(state.selectedIndex == 7,
          "selectedIndex is left untouched - unlike a Select signal, Default never reads or writes it");
}

} // namespace

int main() {
    RunSpawnsAcknowledgmentChecks();
    RunArmiesExceedingSlotCountChecks();
    RunSlotPatternRoundTripChecks();
    RunConditionMatchingChecks();
    RunReachabilityChecks();
    RunPriorityBadgeChecks();
    RunDuplicateChecks();
    RunDefaultTierSelectionChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
