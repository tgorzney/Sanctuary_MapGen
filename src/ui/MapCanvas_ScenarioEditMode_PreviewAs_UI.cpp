// MapCanvas_ScenarioEditMode_PreviewAs_UI.cpp — the "Preview As" scratch composition synthesis
// (STEP78 Fix section's own item) + ScenarioEditModeState::Activate. Layer: UI. The synthesis
// itself is pure/imgui-free/headless-testable; Activate lives here because it is this function's
// only caller.
#include "MapCanvas_ScenarioEditMode_State_UI.h"
#include "ScenariosTab_UI.h"

namespace SanmapGen {
namespace Ui {

std::string SynthesizeScenarioPreviewAsSlotPattern(
    const std::vector<Params::ScenarioCountCondition>& conditions, int maxArmySlotCount) {
    const int clampedMax = maxArmySlotCount < 0 ? 0 : maxArmySlotCount;
    for (int total = 0; total <= clampedMax; ++total) {
        for (int human = 0; human <= total; ++human) {
            const int ai = total - human;
            if (!MatchesScenarioConditions(conditions, total, human, ai)) continue;
            std::string pattern(static_cast<std::size_t>(clampedMax), '-');
            for (int index = 0; index < human; ++index) pattern[static_cast<std::size_t>(index)] = 'h';
            for (int index = 0; index < ai; ++index) pattern[static_cast<std::size_t>(human + index)] = 'A';
            return pattern;
        }
    }
    return std::string(static_cast<std::size_t>(clampedMax), '-');   // no satisfying triple found
}

void ScenarioEditModeState::Activate(Params::ScenarioBody& body, const std::string* patternSlotPattern,
                                     const std::vector<Params::ScenarioCountCondition>* countConditions,
                                     int maxArmySlotCount) {
    bActive = true;
    editedBody = &body;
    bDragging = false;
    dragRowIndex = kScenarioEditModeNoIndex;
    lastResolvedCandidates.clear();
    pendingContextMenu = ContextMenuRequest();
    bContextMenuJustRequested = false;
    if (patternSlotPattern != nullptr) {
        previewAsSlotPattern = *patternSlotPattern;
    } else {
        static const std::vector<Params::ScenarioCountCondition> emptyConditions;
        previewAsSlotPattern = SynthesizeScenarioPreviewAsSlotPattern(
            countConditions != nullptr ? *countConditions : emptyConditions, maxArmySlotCount);
    }
}

} // namespace Ui
} // namespace SanmapGen
