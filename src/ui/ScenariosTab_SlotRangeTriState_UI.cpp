// ScenariosTab_SlotRangeTriState_UI.cpp — the composition matrix's tri-state evaluator (STEP253,
// ARCH_15_05_ParamsScenariosType.md §15.5 AMENDED 2026-09-04, "Follow-ups" section). Split out of
// ScenariosTab_Reachability_UI.cpp (which stays the boolean/brute-force reachability half) so the
// ARCH §1.5 file-size ceiling is respected — a companion split in the same spirit as that file's own
// split out of ScenariosTab_MatchRules_UI.cpp. Layer: UI.
//
// No imgui here at all (WidgetHelpers_UI.h "THE SPLIT"): every function is headless-testable.
#include "ScenariosTab_UI.h"
#include <algorithm>

namespace SanmapGen {
namespace Ui {
namespace {
bool EvaluateScenarioComparator(Params::ScenarioComparator comparator, int fieldValue, int value) {
    switch (comparator) {
        case Params::ScenarioComparator::Equal:          return fieldValue == value;
        case Params::ScenarioComparator::NotEqual:       return fieldValue != value;
        case Params::ScenarioComparator::GreaterThan:    return fieldValue >  value;
        case Params::ScenarioComparator::GreaterOrEqual: return fieldValue >= value;
        case Params::ScenarioComparator::LessThan:       return fieldValue <  value;
        case Params::ScenarioComparator::LessOrEqual:    return fieldValue <= value;
    }
    return false;
}
} // namespace

// Used ONLY by the composition matrix (ScenariosTab_Matrix_UI.cpp), which resolves a scenario for a
// bare (total, human, ai) triple with no real slot identity to consult. A SlotRangeOccupiedCount
// clause's occupied-in-range count can range anywhere across
// [max(0, occupiedTotal - (maxArmySlotCount - rangeWidth)), min(rangeWidth, occupiedTotal)] depending
// on WHICH slots are actually filled — every integer in that interval is achievable (moving one
// occupied slot in/out of the range shifts the count by exactly 1), so this walks the whole
// achievable interval rather than just its endpoints (correct for non-monotonic comparators like
// Equal/NotEqual too, not just the monotonic ones).
ScenarioConditionTriState EvaluateScenarioConditionsTriState(
        const std::vector<Params::ScenarioCountCondition>& conditions,
        int total, int human, int ai, int maxArmySlotCount) {
    bool bAmbiguous = false;
    for (const Params::ScenarioCountCondition& condition : conditions) {
        if (condition.field == Params::ScenarioCountField::SlotRangeOccupiedCount) {
            const int rangeWidth = std::max(0, condition.slotRangeEnd - condition.slotRangeStart + 1);
            const int occupiedTotal = human + ai;
            const int outsideWidth = std::max(0, maxArmySlotCount - rangeWidth);
            const int minOccupiedInRange = std::max(0, occupiedTotal - outsideWidth);
            const int maxOccupiedInRange = std::min(rangeWidth, occupiedTotal);

            bool bAnyPass = false, bAnyFail = false;
            for (int occupied = minOccupiedInRange; occupied <= maxOccupiedInRange; ++occupied) {
                if (EvaluateScenarioComparator(condition.comparator, occupied, condition.value)) bAnyPass = true;
                else bAnyFail = true;
            }
            if (!bAnyPass) return ScenarioConditionTriState::DefinitelyFalse;   // never satisfiable
            if (bAnyFail) bAmbiguous = true;                                   // sometimes satisfiable
            // else: satisfiable across the WHOLE achievable set -- this clause passes definitely.
        } else {
            const int fieldValue = condition.field == Params::ScenarioCountField::Total ? total
                                  : condition.field == Params::ScenarioCountField::HumanCount ? human : ai;
            if (!EvaluateScenarioComparator(condition.comparator, fieldValue, condition.value))
                return ScenarioConditionTriState::DefinitelyFalse;
        }
    }
    return bAmbiguous ? ScenarioConditionTriState::Ambiguous : ScenarioConditionTriState::DefinitelyTrue;
}

} // namespace Ui
} // namespace SanmapGen
