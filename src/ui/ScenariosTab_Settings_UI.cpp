// ScenariosTab_Settings_UI.cpp — Fix §7: `maxArmySlotCount` authoring and its never-clamping,
// never-auto-raising warning banner (`ARCH_15_10_SlotPatternConstructionMoves.md` §15.10 point 2).
// Layer: UI.
#include "ScenariosTab_UI.h"
#include "SliderScalar_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

// `64` is a UI convenience ceiling only, not a PARAMS/ARCH constraint — §15.10: "no fixed upper
// bound is imposed."
ScalarSliderRange MaxArmySlotCountRange() { return IntegerScalarSliderRange(1, 64, 1); }

void DrawArmiesExceedingSlotCountBanner(const std::vector<Params::Army>& armies, int maxArmySlotCount) {
    const std::vector<std::string> affected = ArmiesExceedingSlotCount(armies, maxArmySlotCount);
    if (affected.empty()) return;
    std::string names;
    for (std::size_t index = 0u; index < affected.size(); ++index) {
        if (index > 0u) names += ", ";
        names += affected[index];
    }
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.75f, 0.15f, 1.0f));
    ImGui::TextWrapped(
        "maxArmySlotCount (%d) is smaller than the authored army roster (%d). These armies can "
        "never appear in any slotPattern: %s.",
        maxArmySlotCount, static_cast<int>(armies.size()), names.c_str());
    ImGui::PopStyleColor();
}

} // namespace

// Never blocks, never auto-raises (§15.10 point 2): the stepper and the banner are independent —
// the banner is always visible when true, not export-gated, so it is seen the moment it becomes true.
void DrawScenarioSettings(Params::Scenarios& scenarios, SectionState& settingsSection,
                          const std::vector<Params::Army>& armies) {
    if (!DrawSectionBegin("Settings", settingsSection)) return;
    RealtimeToggle maxSlotCountToggle;   // throwaway: Backend policy N/A, see ScenariosTab_Detail_UI.cpp
    DrawSliderScalarInteger("Max Army Slot Count", scenarios.maxArmySlotCount, MaxArmySlotCountRange(),
                            maxSlotCountToggle, WidgetStyle(), "%d");
    DrawArmiesExceedingSlotCountBanner(armies, scenarios.maxArmySlotCount);
    DrawSectionEnd();
}

} // namespace Ui
} // namespace SanmapGen
