// ScenariosTab_DetailNaval_UI.cpp — Fix §5's `navalFleet` fields (fleet / pondSideByArmy /
// sideBiasDistance), split out of ScenariosTab_DetailAlloys_UI.cpp for the ARCH §1.5 file-size
// ceiling; called only by DrawScenarioBodyExtendedFields there, itself gated on `body.navy`. Layer: UI.
#include "ScenariosTab_UI.h"
#include "ArmiesTab_UI.h"
#include "Combo_UI.h"
#include "SliderScalar_UI.h"
#include "TextInput_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

// Duplicated small helper (see ScenariosTab_Detail_UI.cpp's own copy for why this is not shared).
void DrawArmyNameField(const char* label, std::string& armyNameKey, const std::vector<Params::Army>& armies) {
    if (armies.empty()) {
        TextInputRules rules; rules.bAllowEmpty = true; rules.maximumLength = 48;
        DrawTextInput(label, armyNameKey, rules);
        return;
    }
    std::vector<const char*> labels;
    labels.reserve(armies.size());
    int selectedIndex = -1;
    for (std::size_t index = 0u; index < armies.size(); ++index) {
        labels.push_back(ArmyRowLabel(armies[index]));
        if (armies[index].name == armyNameKey) selectedIndex = static_cast<int>(index);
    }
    ComboOptions options; options.labels = labels.data(); options.count = static_cast<int>(labels.size());
    if (DrawCombo(label, selectedIndex, options).bCommitted && selectedIndex >= 0)
        armyNameKey = armies[static_cast<std::size_t>(selectedIndex)].name;
}

void DrawNavalFleetEntries(std::vector<Params::ScenarioNavalFleetEntry>& fleet) {
    int removeIndex = -1;
    for (std::size_t index = 0u; index < fleet.size(); ++index) {
        ImGui::PushID(static_cast<int>(index));
        Params::ScenarioNavalFleetEntry& entry = fleet[index];
        TextInputRules idRules; idRules.maximumLength = 16; idRules.bAllowEmpty = true;
        DrawTextInput("Template Id", entry.templateIdentifier, idRules);
        ImGui::SameLine(); ImGui::SetNextItemWidth(70.0f);
        ImGui::InputInt("Count", &entry.count, 1);
        ImGui::SameLine();
        if (ImGui::SmallButton("X##removeFleetEntry")) removeIndex = static_cast<int>(index);
        ImGui::PopID();
    }
    if (removeIndex >= 0) fleet.erase(fleet.begin() + removeIndex);
    if (ImGui::Button("+ Add Fleet Entry")) fleet.push_back(Params::ScenarioNavalFleetEntry());
}

// Sparse: an army absent from this list defaults to East (the live reference's own fallback,
// Scenario_PARAMS.h's own comment) — this editor only ever writes an EXPLICIT row.
void DrawNavalPondSideAssignments(std::vector<Params::ScenarioNavalPondAssignment>& pondSideByArmy,
                                  const std::vector<Params::Army>& armies) {
    static const char* const sideLabels[2] = { "West", "East" };
    int removeIndex = -1;
    for (std::size_t index = 0u; index < pondSideByArmy.size(); ++index) {
        ImGui::PushID(static_cast<int>(index));
        Params::ScenarioNavalPondAssignment& entry = pondSideByArmy[index];
        DrawArmyNameField("Army", entry.armyName, armies);
        ImGui::SameLine();
        int sideIndex = entry.side == Params::ScenarioNavalPondSide::West ? 0 : 1;
        ComboOptions sideOptions; sideOptions.labels = sideLabels; sideOptions.count = 2;
        if (DrawCombo("Side", sideIndex, sideOptions).bCommitted)
            entry.side = sideIndex == 0 ? Params::ScenarioNavalPondSide::West : Params::ScenarioNavalPondSide::East;
        ImGui::SameLine();
        if (ImGui::SmallButton("X##removePondSide")) removeIndex = static_cast<int>(index);
        ImGui::PopID();
    }
    if (removeIndex >= 0) pondSideByArmy.erase(pondSideByArmy.begin() + removeIndex);
    if (ImGui::Button("+ Add Pond Side Assignment")) pondSideByArmy.push_back(Params::ScenarioNavalPondAssignment());
}

} // namespace

void DrawScenarioNavalFleetFields(Params::ScenarioNavalFleet& navalFleet, const std::vector<Params::Army>& armies) {
    ImGui::TextUnformatted("Fleet");
    DrawNavalFleetEntries(navalFleet.fleet);
    ImGui::TextUnformatted("Pond Side By Army");
    DrawNavalPondSideAssignments(navalFleet.pondSideByArmy, armies);
    RealtimeToggle sideBiasToggle;
    DrawSliderScalar("Side Bias Distance", navalFleet.sideBiasDistance,
                     ScalarSliderRange{ 0.0f, 4096.0f, 0.0f }, sideBiasToggle, WidgetStyle(), "%.1f");
}

} // namespace Ui
} // namespace SanmapGen
