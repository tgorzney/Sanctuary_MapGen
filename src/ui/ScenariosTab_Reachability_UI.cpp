// ScenariosTab_Reachability_UI.cpp — the pure, imgui-free half of Fix §3: slot-pattern toggle
// round-tripping, the AND-of-clauses evaluator that mirrors the runtime's Lua
// EvaluateScenarioConditions exactly, and the brute-force reachability checks it drives. Layer: UI.
// Split out of ScenariosTab_MatchRules_UI.cpp (which stays imgui-drawing only) so the ARCH §1.5
// file-size ceiling is respected without shrinking either concern's own comments — a companion split
// in the same spirit as AreasTab_List_UI.h/AreasTab_UI.cpp.
//
// No imgui here at all (WidgetHelpers_UI.h "THE SPLIT"): every function is headless-testable.
#include "ScenariosTab_UI.h"

namespace SanmapGen {
namespace Ui {
namespace {
std::string OrdinalSuffix(int oneBasedNumber) {
    const int lastTwoDigits = oneBasedNumber % 100;
    if (lastTwoDigits >= 11 && lastTwoDigits <= 13) return "th";
    switch (oneBasedNumber % 10) {
        case 1: return "st";
        case 2: return "nd";
        case 3: return "rd";
        default: return "th";
    }
}
} // namespace

// Fix §2's row-order priority badge ("1st checked", "2nd checked", ...) — colocated here rather
// than in ScenariosTab_ListMechanics_UI.h because it needs external linkage (the acceptance test
// calls it directly) and this file is already the tab's pure, imgui-free translation unit.
std::string ScenarioPriorityBadge(int zeroBasedRowIndex) {
    if (zeroBasedRowIndex < 0) return "";
    const int oneBased = zeroBasedRowIndex + 1;
    return std::to_string(oneBased) + OrdinalSuffix(oneBased) + " checked";
}

std::string BuildSlotPatternFromToggles(const std::vector<char>& toggles) {
    return std::string(toggles.begin(), toggles.end());
}

// Length mismatch (a pattern authored before a maxArmySlotCount edit) degrades gracefully: a short
// pattern pads with '-', a long one truncates, and any character the toggle cycle never produces is
// treated as '-' — never crashes, never silently writes the repaired string back (Constitution §6:
// a load must not mutate what it didn't touch — the caller only stores this on an actual toggle edit).
std::vector<char> ParseSlotPatternToToggles(const std::string& pattern, int maxArmySlotCount) {
    const int resolvedLength = maxArmySlotCount < 0 ? 0 : maxArmySlotCount;
    std::vector<char> toggles(static_cast<std::size_t>(resolvedLength), '-');
    const int copyLength = resolvedLength < static_cast<int>(pattern.size())
        ? resolvedLength : static_cast<int>(pattern.size());
    for (int index = 0; index < copyLength; ++index) {
        const char character = pattern[static_cast<std::size_t>(index)];
        toggles[static_cast<std::size_t>(index)] = (character == 'h' || character == 'A') ? character : '-';
    }
    return toggles;
}

// AND-of-clauses, matching the runtime's Lua truth table exactly. Vacuously true for an empty
// conjunction (no conditions authored == "always matches", the Tier 2 default-of-the-tier reading).
bool MatchesScenarioConditions(const std::vector<Params::ScenarioCountCondition>& conditions,
                               int total, int human, int ai) {
    for (const Params::ScenarioCountCondition& condition : conditions) {
        const int fieldValue = condition.field == Params::ScenarioCountField::Total ? total
                              : condition.field == Params::ScenarioCountField::HumanCount ? human : ai;
        bool bClausePasses = false;
        switch (condition.comparator) {
            case Params::ScenarioComparator::Equal:          bClausePasses = fieldValue == condition.value; break;
            case Params::ScenarioComparator::NotEqual:       bClausePasses = fieldValue != condition.value; break;
            case Params::ScenarioComparator::GreaterThan:    bClausePasses = fieldValue >  condition.value; break;
            case Params::ScenarioComparator::GreaterOrEqual: bClausePasses = fieldValue >= condition.value; break;
            case Params::ScenarioComparator::LessThan:       bClausePasses = fieldValue <  condition.value; break;
            case Params::ScenarioComparator::LessOrEqual:    bClausePasses = fieldValue <= condition.value; break;
        }
        if (!bClausePasses) return false;
    }
    return true;
}

namespace {
// The first countScenarios[] index matching this triple, by array order, or -1 (default's turn).
int FirstMatchingCountScenarioIndex(const Params::Scenarios& scenarios, int total, int human, int ai) {
    for (std::size_t index = 0u; index < scenarios.countScenarios.size(); ++index)
        if (MatchesScenarioConditions(scenarios.countScenarios[index].conditions, total, human, ai))
            return static_cast<int>(index);
    return -1;
}
} // namespace

// Brute-forces every (total, human, ai) triple in [0, maxArmySlotCount] (human+ai == total, both
// >= 0) and asks whether entryIndex is the FIRST scenario (by array order) matching ANY triple. Tier
// 1 exact-pattern entries are deliberately excluded — see ScenariosTab_UI.h.
bool IsCountScenarioReachable(const Params::Scenarios& scenarios, int countScenarioIndex) {
    if (countScenarioIndex < 0 || countScenarioIndex >= static_cast<int>(scenarios.countScenarios.size()))
        return false;
    for (int total = 0; total <= scenarios.maxArmySlotCount; ++total)
        for (int human = 0; human <= total; ++human)
            if (FirstMatchingCountScenarioIndex(scenarios, total, human, total - human) == countScenarioIndex)
                return true;
    return false;
}

bool IsDefaultScenarioReachable(const Params::Scenarios& scenarios) {
    for (int total = 0; total <= scenarios.maxArmySlotCount; ++total)
        for (int human = 0; human <= total; ++human)
            if (FirstMatchingCountScenarioIndex(scenarios, total, human, total - human) == -1)
                return true;
    return false;
}

// The "which earlier entry" second pass: tallies, over every triple, the actual first-match winner
// (excluding this entry itself), and reports the most common one — a cosmetic tie-break (ticket's
// own call). An entry with no winner ever recorded (self-contradictory conditions, no earlier entry
// to blame) still needs a label rather than a crash or a blank parenthetical.
std::string ScenarioReachabilityBadgeSuffix(const Params::Scenarios& scenarios, int countScenarioIndex) {
    const bool bReachable = countScenarioIndex < 0 ? IsDefaultScenarioReachable(scenarios)
                                                    : IsCountScenarioReachable(scenarios, countScenarioIndex);
    if (bReachable) return "";
    std::vector<int> winTally(scenarios.countScenarios.size(), 0);
    for (int total = 0; total <= scenarios.maxArmySlotCount; ++total)
        for (int human = 0; human <= total; ++human) {
            const int winner = FirstMatchingCountScenarioIndex(scenarios, total, human, total - human);
            if (winner >= 0 && winner != countScenarioIndex)
                ++winTally[static_cast<std::size_t>(winner)];
        }
    int mostCommonWinner = -1;
    for (std::size_t index = 0u; index < winTally.size(); ++index)
        if (mostCommonWinner < 0 || winTally[index] > winTally[static_cast<std::size_t>(mostCommonWinner)])
            mostCommonWinner = winTally[index] > 0 ? static_cast<int>(index) : mostCommonWinner;
    const std::string shadowName = mostCommonWinner < 0
        ? "an earlier rule (self-contradictory conditions?)"
        : ScenarioRowLabel(scenarios.countScenarios[static_cast<std::size_t>(mostCommonWinner)].body);
    return " \xE2\x9A\xA0 Unreachable (shadowed by " + shadowName + ")";
}

} // namespace Ui
} // namespace SanmapGen
