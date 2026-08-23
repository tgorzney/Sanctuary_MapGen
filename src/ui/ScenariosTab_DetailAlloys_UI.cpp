// ScenariosTab_DetailAlloys_UI.cpp — Fix §5's alloys/alloysToAdd/alloysToRemove/navalFleet half of
// the shared `ScenarioBody` editor (split out of ScenariosTab_Detail_UI.cpp for the ARCH §1.5 file-
// size ceiling; called only by DrawScenarioBodyFields there). Layer: UI.
//
// alloys/alloysToAdd/alloysToRemove are the honest lower-fidelity fallback (free-text `markerName`,
// NOT a validated picker against real placed markers — STEP78's job, Fix §5's own note).
#include "ScenariosTab_UI.h"
#include "ArmiesTab_UI.h"
#include "Combo_UI.h"
#include "SliderScalar_UI.h"
#include "TextInput_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

ScalarSliderRange ExtendedFieldsWorldPositionRange() { return ScalarSliderRange{ -8192.0f, 8192.0f, 0.0f }; }

// Duplicated from ScenariosTab_Detail_UI.cpp (a different translation unit's anonymous namespace) —
// small enough that a shared header for one nine-line function would cost more than it saves.
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

// Shared row shape for `alloys` (Explicit) and `alloysToAdd` (Delta) — both are `ScenarioAlloyOverride`.
void DrawAlloyOverrideList(const char* addButtonLabel, std::vector<Params::ScenarioAlloyOverride>& overrides,
                           const std::vector<Params::Army>& armies) {
    const ScalarSliderRange range = ExtendedFieldsWorldPositionRange();
    int removeIndex = -1;
    for (std::size_t index = 0u; index < overrides.size(); ++index) {
        ImGui::PushID(static_cast<int>(index));
        Params::ScenarioAlloyOverride& entry = overrides[index];
        DrawArmyNameField("Army", entry.armyName, armies);
        TextInputRules markerRules; markerRules.bAllowEmpty = true; markerRules.maximumLength = 48;
        DrawTextInput("Marker Name", entry.markerName, markerRules);
        RealtimeToggle xToggle, yToggle, zToggle;
        ImGui::SetNextItemWidth(90.0f); DrawSliderScalar("X", entry.positionX, range, xToggle, WidgetStyle(), "%.1f");
        ImGui::SameLine(); ImGui::SetNextItemWidth(90.0f);
        DrawSliderScalar("Y", entry.positionY, range, yToggle, WidgetStyle(), "%.1f");
        ImGui::SameLine(); ImGui::SetNextItemWidth(90.0f);
        DrawSliderScalar("Z", entry.positionZ, range, zToggle, WidgetStyle(), "%.1f");
        ImGui::SameLine();
        if (ImGui::SmallButton("X##removeOverride")) removeIndex = static_cast<int>(index);
        ImGui::PopID();
    }
    if (removeIndex >= 0) overrides.erase(overrides.begin() + removeIndex);
    if (ImGui::Button(addButtonLabel)) overrides.push_back(Params::ScenarioAlloyOverride());
}

// `alloysToRemove` — armyName + markerName ONLY, no position (matches the struct).
void DrawAlloyRemovalList(std::vector<Params::ScenarioAlloyRemoval>& removals, const std::vector<Params::Army>& armies) {
    int removeIndex = -1;
    for (std::size_t index = 0u; index < removals.size(); ++index) {
        ImGui::PushID(static_cast<int>(index));
        Params::ScenarioAlloyRemoval& entry = removals[index];
        DrawArmyNameField("Army", entry.armyName, armies);
        ImGui::SameLine();
        TextInputRules markerRules; markerRules.bAllowEmpty = true; markerRules.maximumLength = 48;
        DrawTextInput("Marker Name", entry.markerName, markerRules);
        ImGui::SameLine();
        if (ImGui::SmallButton("X##removeRemoval")) removeIndex = static_cast<int>(index);
        ImGui::PopID();
    }
    if (removeIndex >= 0) removals.erase(removals.begin() + removeIndex);
    if (ImGui::Button("+ Add Alloy To Remove")) removals.push_back(Params::ScenarioAlloyRemoval());
}

} // namespace

void DrawScenarioBodyExtendedFields(Params::ScenarioBody& body, const std::vector<Params::Army>& armies) {
    ImGui::SeparatorText("Alloys (Explicit)");
    ImGui::PushID("alloys");
    DrawAlloyOverrideList("+ Add Alloy", body.alloys, armies);
    ImGui::PopID();

    ImGui::SeparatorText("Alloys To Add (Delta)");
    ImGui::PushID("alloysToAdd");
    DrawAlloyOverrideList("+ Add Alloy To Add", body.alloysToAdd, armies);
    ImGui::PopID();

    ImGui::SeparatorText("Alloys To Remove (Delta)");
    DrawAlloyRemovalList(body.alloysToRemove, armies);

    // Collapsed/de-emphasized rather than disabled: §15.5's "always emitted" rule means the data
    // still round-trips even while Navy is off, so nothing here is non-interactive — it defaults
    // CLOSED and dimmer when Navy is off, open when it is on, per Fix §5's own wording.
    if (!body.navy) ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    const bool bFleetOpen = ImGui::CollapsingHeader("Naval Fleet",
        body.navy ? ImGuiTreeNodeFlags_DefaultOpen : ImGuiTreeNodeFlags_None);
    if (!body.navy) ImGui::PopStyleColor();
    if (bFleetOpen) DrawScenarioNavalFleetFields(body.navalFleet, armies);
}

} // namespace Ui
} // namespace SanmapGen
