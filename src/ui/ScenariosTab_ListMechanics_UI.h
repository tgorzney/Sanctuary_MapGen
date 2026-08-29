// ScenariosTab_ListMechanics_UI.h — Fix §2's pure list rules (ordinal badges, the nested-`.body.name`
// uniqueness repair, Duplicate, selection resolution) plus the two DraggableList row builders. Layer:
// UI. PRIVATE to ScenariosTab_Lists_UI.cpp — the sole translation unit that includes this header —
// split out purely for the ARCH §1.5 file-size ceiling, the same posture AreasTab_List_UI.h has to
// AreasTab_UI.cpp. Every symbol is `inline`: nothing here is part of the tab's cross-file API (that
// lives in ScenariosTab_UI.h).
#pragma once
#include "DraggableListWidget_UI.h"
#include "ScenariosTab_UI.h"
#include <string>

namespace SanmapGen {
namespace Ui {

// `.body.name` is nested, so `UniqueNameList_UI.h`'s `MakeNamesUnique<T>` (which needs `.name`
// directly on T) cannot be used as-is — Fix §2's "thin adapter" against the nested field.
template <typename ScenarioT>
inline void MakeScenarioBodyNamesUnique(std::vector<ScenarioT>& scenarios) {
    for (std::size_t index = 0u; index < scenarios.size(); ++index) {
        bool bTaken = false;
        for (std::size_t earlier = 0u; earlier < index; ++earlier)
            if (scenarios[earlier].body.name == scenarios[index].body.name) { bTaken = true; break; }
        if (!bTaken) continue;
        const std::string baseName = scenarios[index].body.name;
        int suffix = 1;
        bool bClash;
        do {
            scenarios[index].body.name = baseName + "_" + std::to_string(suffix++);
            bClash = false;
            for (std::size_t earlier = 0u; earlier < index; ++earlier)
                if (scenarios[earlier].body.name == scenarios[index].body.name) { bClash = true; break; }
        } while (bClash);
    }
}

template <typename ScenarioT>
inline void DuplicateScenario(std::vector<ScenarioT>& scenarios, int selectedIndex, ScenariosTabState& state) {
    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(scenarios.size())) return;
    scenarios.insert(scenarios.begin() + selectedIndex + 1, scenarios[static_cast<std::size_t>(selectedIndex)]);
    MakeScenarioBodyNamesUnique(scenarios);
    state.selectedIndex = selectedIndex + 1;
}

// Select/Reorder/Delete for one tier's vector, keeping `state.selectedIndex` valid afterward
// (Constitution §6). Visibility/Lock carry no meaning here (no per-row bit either tier owns) and are
// ignored, matching AreasTab_UI.cpp's own AFFORDANCE SCOPE precedent.
template <typename ScenarioT>
inline void ApplyScenarioListSignal(std::vector<ScenarioT>& scenarios, ScenariosTabState& state,
                                    ScenarioSelectedTier tier, const DraggableListSignal& signal) {
    if (signal.kind == DraggableListSignalKind::Select) {
        state.selectedTier = tier;
        state.selectedIndex = signal.sourceRowIndex;
        return;
    }
    if (signal.kind == DraggableListSignalKind::ToggleVisibility
        || signal.kind == DraggableListSignalKind::ToggleLock) return;
    if (!ApplyDraggableListSignal(scenarios, signal)) return;
    state.selectedTier = tier;
    if (state.selectedIndex >= static_cast<int>(scenarios.size()))
        state.selectedIndex = static_cast<int>(scenarios.size()) - 1;
}

inline std::string ScenarioPatternRowLabel(const Params::PatternScenario& scenario) {
    std::string label = ScenarioNeedsSpawnsAcknowledgment(scenario.body) ? "\xE2\x9A\xA0 " : "";
    label += ScenarioRowLabel(scenario.body);
    return label;
}

inline std::string ScenarioCountRowLabel(const Params::Scenarios& scenarios, int rowIndex) {
    const Params::CountScenario& scenario = scenarios.countScenarios[static_cast<std::size_t>(rowIndex)];
    std::string label = ScenarioNeedsSpawnsAcknowledgment(scenario.body) ? "\xE2\x9A\xA0 " : "";
    label += ScenarioPriorityBadge(rowIndex) + ". " + ScenarioRowLabel(scenario.body);
    label += ScenarioReachabilityBadgeSuffix(scenarios, rowIndex);
    return label;
}

// MUTATES the caller's `scenarios` ONLY from inside `drawRowBody` (never from `describeRow`, which
// stays read-only per the DraggableList contract) — STEP110: each row's OWN settings (slot-pattern
// toggles, the spawns warning, the shared body-field editor) now draw directly under that row's own
// header, whenever ITS OWN CollapsingHeader is open, never gated on `selectedIndex`. `selectedIndex`
// still flows in, purely for the DraggableList "Selected" highlight (the same posture STEP104 left
// `selectedLayerIndex` in for LayerEditor_Group_UI.cpp).
inline DraggableListSignal DrawScenarioPatternList(std::vector<Params::PatternScenario>& scenarios,
                                                    ScenariosTabState& state,
                                                    const std::vector<Params::Army>& armies,
                                                    const std::vector<Params::MapArea>& areas,
                                                    int maxArmySlotCount, int selectedIndex) {
    std::string labelBuffer;
    return DraggableList<Params::PatternScenario>::Render(
        "patternScenarios", scenarios,
        [&](int rowIndex) {
            labelBuffer = ScenarioPatternRowLabel(scenarios[static_cast<std::size_t>(rowIndex)]);
            DraggableListRow row; row.label = labelBuffer.c_str();
            return row;
        },
        [&](int rowIndex) {
            Params::PatternScenario& scenario = scenarios[static_cast<std::size_t>(rowIndex)];
            DrawSlotPatternToggleRow(scenario.slotPattern, armies, maxArmySlotCount);
            DrawScenarioSpawnsWarningBanner(scenario.body, armies);
            DrawScenarioBodyFields(scenario.body, armies, areas, state.scenarioEditModeState,
                                   &scenario.slotPattern, nullptr, maxArmySlotCount);
        },
        selectedIndex);
}

inline DraggableListSignal DrawScenarioCountList(Params::Scenarios& scenarios, ScenariosTabState& state,
                                                  const std::vector<Params::Army>& armies,
                                                  const std::vector<Params::MapArea>& areas,
                                                  int selectedIndex) {
    std::string labelBuffer;
    return DraggableList<Params::CountScenario>::Render(
        "countScenarios", scenarios.countScenarios,
        [&](int rowIndex) {
            labelBuffer = ScenarioCountRowLabel(scenarios, rowIndex);
            DraggableListRow row; row.label = labelBuffer.c_str();
            return row;
        },
        [&](int rowIndex) {
            Params::CountScenario& scenario = scenarios.countScenarios[static_cast<std::size_t>(rowIndex)];
            DrawScenarioCountConditionsEditor(scenario.conditions);
            DrawScenarioSpawnsWarningBanner(scenario.body, armies);
            DrawScenarioBodyFields(scenario.body, armies, areas, state.scenarioEditModeState, nullptr,
                                   &scenario.conditions, scenarios.maxArmySlotCount);
        },
        selectedIndex);
}

} // namespace Ui
} // namespace SanmapGen
