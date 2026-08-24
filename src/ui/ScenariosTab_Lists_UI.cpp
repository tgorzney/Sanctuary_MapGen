// ScenariosTab_Lists_UI.cpp — Fix §2: the three tiers and DrawScenariosTab's entry point. The pure
// list mechanics (ordinal badges, Duplicate, the nested-name repair, the DraggableList row builders)
// live in ScenariosTab_ListMechanics_UI.h, a private companion split for the ARCH §1.5 ceiling.
// Layer: UI.
//
// `previewDriver` is accepted for interface parity only — see ScenariosTab_UI.h's own note.
#include "ScenariosTab_ListMechanics_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

// Add + Duplicate toolbar shared by Tier 1/2.
template <typename ScenarioT>
void DrawScenarioTierToolbar(const char* addLabel, std::vector<ScenarioT>& scenarios,
                             ScenariosTabState& state, ScenarioSelectedTier tier) {
    if (ImGui::Button(addLabel)) {
        ScenarioT scenario;
        scenario.body.name = NextScenarioName(static_cast<int>(scenarios.size()));
        scenarios.push_back(scenario);
        state.selectedTier = tier;
        state.selectedIndex = static_cast<int>(scenarios.size()) - 1;
    }
    const int selectedIndex = state.selectedTier == tier ? state.selectedIndex : -1;
    ImGui::SameLine();
    ImGui::BeginDisabled(selectedIndex < 0);
    if (ImGui::Button("Duplicate")) DuplicateScenario(scenarios, selectedIndex, state);
    ImGui::EndDisabled();
}

// STEP110: each row's own settings now draw directly under that row, inside DrawScenarioPatternList/
// DrawScenarioCountList's own `drawRowBody` — whenever ITS OWN CollapsingHeader is open, never gated
// on `selectedTier`/`selectedIndex`. Each Draw*Tier is scoped under its own PushID so two sections
// left open at once (Pattern + Count, say) never collide on imgui's id stack. `selectedTier`/
// `selectedIndex` stay (Duplicate's own gate above, the DraggableList "Selected" highlight, and
// `ApplyScenarioListSignal`'s Select handling all still need them) — only the redundant full-panel
// draw that used to run once at the bottom for whatever they pointed at is gone.
void DrawScenarioPatternTier(Params::Scenarios& scenarios, ScenariosTabState& state,
                             const std::vector<Params::Army>& armies) {
    ImGui::PushID("pattern");
    if (!DrawSectionBegin("Exact Slot Patterns", state.patternSection)) { ImGui::PopID(); return; }
    ImGui::TextWrapped("Order here is cosmetic - exact-match only, first-and-only match wins "
                       "regardless of position.");
    DrawScenarioTierToolbar("Add Pattern Scenario", scenarios.patternScenarios, state, ScenarioSelectedTier::Pattern);
    const int priorSelection = state.selectedTier == ScenarioSelectedTier::Pattern ? state.selectedIndex : -1;
    const DraggableListSignal signal = DrawScenarioPatternList(scenarios.patternScenarios, state, armies,
                                                               scenarios.maxArmySlotCount, priorSelection);
    if (signal.bHasSignal())
        ApplyScenarioListSignal(scenarios.patternScenarios, state, ScenarioSelectedTier::Pattern, signal);
    DrawSectionEnd();
    ImGui::PopID();
}

void DrawScenarioCountTier(Params::Scenarios& scenarios, ScenariosTabState& state,
                           const std::vector<Params::Army>& armies) {
    ImGui::PushID("count");
    if (!DrawSectionBegin("Composition Rules", state.countSection)) { ImGui::PopID(); return; }
    ImGui::TextWrapped("Array order IS match priority (\xC2\xA7 15.6) - drag to reorder, checked top "
                       "to bottom; the label's leading number always agrees with position.");
    DrawScenarioTierToolbar("Add Composition Rule", scenarios.countScenarios, state, ScenarioSelectedTier::Count);
    const int priorSelection = state.selectedTier == ScenarioSelectedTier::Count ? state.selectedIndex : -1;
    const DraggableListSignal signal = DrawScenarioCountList(scenarios, state, armies, priorSelection);
    if (signal.bHasSignal())
        ApplyScenarioListSignal(scenarios.countScenarios, state, ScenarioSelectedTier::Count, signal);
    DrawSectionEnd();
    ImGui::PopID();
}

// No list widget, no create/duplicate/delete: exactly one `defaultScenario` always exists. Entering
// the section body selects it outright (Fix §2's "clicking inside the fixed Default panel").
void DrawScenarioDefaultTier(Params::Scenarios& scenarios, ScenariosTabState& state,
                             const std::vector<Params::Army>& armies) {
    ImGui::PushID("default");
    if (!DrawSectionBegin("Default (always matches)", state.defaultSection)) { ImGui::PopID(); return; }
    ImGui::TextWrapped("The catch-all: whatever no Tier 1/2 rule claims lands here.");
    SelectScenarioDefaultTier(state);
    // Tier 3 is never spawns-flagged (see ScenariosTab_UI.h) — no warning banner drawn.
    DrawScenarioBodyFields(scenarios.defaultScenario, armies, state.scenarioEditModeState, nullptr,
                           nullptr, scenarios.maxArmySlotCount);
    DrawSectionEnd();
    ImGui::PopID();
}

} // namespace

void DrawScenariosTab(Params::MapRecipe& recipe, ScenariosTabState& state, Pipeline::PreviewDriver*) {
    ImGui::PushID("scenariosTab");
    Params::Scenarios& scenarios = recipe.scenarios;
    DrawScenarioSettings(scenarios, state.settingsSection, recipe.armies);
    DrawScenarioPatternTier(scenarios, state, recipe.armies);
    DrawScenarioCountTier(scenarios, state, recipe.armies);
    DrawScenarioDefaultTier(scenarios, state, recipe.armies);
    DrawScenarioMatrix(scenarios, state.matrixSection);
    DrawScenarioRuntimeScriptSection(state);
    ImGui::PopID();
}

} // namespace Ui
} // namespace SanmapGen
