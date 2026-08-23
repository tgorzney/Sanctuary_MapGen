// ScenariosTab_MatchRules_UI.cpp — Fix §3's imgui-drawing half: the Tier 1 slot-pattern toggle row
// and the Tier 2 AND-of-clauses condition table. The pure round-trip/evaluator/reachability
// functions this draws over live in ScenariosTab_Reachability_UI.cpp (ARCH §1.5 split). Layer: UI.
//
// Constitution §8: the 3 ScenarioCountField labels and the 6 comparator DISPLAY labels are real
// tables — the enum index is what is stored (Correction 17's own note: friendlier symbols never
// change the disk format).
#include "ScenariosTab_UI.h"
#include "ArmiesTab_UI.h"
#include "Combo_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

enum : int { kScenarioCountFieldCount = 3, kScenarioComparatorCount = 6 };
const char* const scenarioCountFieldLabels[kScenarioCountFieldCount] = { "Total", "Human", "AI" };
const char* const scenarioComparatorLabels[kScenarioComparatorCount] = {
    "=", "\xE2\x89\xA0", ">", "\xE2\x89\xA5", "<", "\xE2\x89\xA4"   // = != > >= < <=
};
// Wider, unambiguous symbols for the read-only auto-generated summary sentence — the combo stays
// terse, the sentence stays readable.
const char* const scenarioComparatorSummaryTokens[kScenarioComparatorCount] = {
    "==", "!=", ">", ">=", "<", "<="
};

char NextSlotToggleValue(char current) {
    if (current == '-') return 'h';
    if (current == 'h') return 'A';
    return '-';
}

// One 3-state cell: draws its current letter over a tinted square, returns true if clicked.
bool DrawSlotToggleCell(char value, ImU32 tintColor) {
    const float cellSize = ImGui::GetTextLineHeightWithSpacing();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##cell", ImVec2(cellSize, cellSize));
    const bool bClicked = ImGui::IsItemClicked();
    const bool bHovered = ImGui::IsItemHovered();
    ImDrawList* const drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(origin, ImVec2(origin.x + cellSize, origin.y + cellSize), tintColor, 2.0f);
    if (bHovered) drawList->AddRect(origin, ImVec2(origin.x + cellSize, origin.y + cellSize),
                                    IM_COL32(255, 255, 255, 200), 2.0f);
    const char glyph[2] = { value, '\0' };
    const ImVec2 textSize = ImGui::CalcTextSize(glyph);
    drawList->AddText(ImVec2(origin.x + (cellSize - textSize.x) * 0.5f, origin.y + (cellSize - textSize.y) * 0.5f),
                      IM_COL32(0, 0, 0, 255), glyph);
    return bClicked;
}

} // namespace

// `-` empty / `h` human / `A` AI. Tinted by `recipe.armies[i].armyColor` where `i < armies.size()`,
// grey beyond — a slot with no authored army still needs a legible toggle. The repaired-length
// string is written back to `slotPattern` ONLY on the frame a toggle is actually clicked
// (Constitution §6 — a load must not mutate what it didn't touch).
void DrawSlotPatternToggleRow(std::string& slotPattern, const std::vector<Params::Army>& armies,
                              int maxArmySlotCount) {
    std::vector<char> toggles = ParseSlotPatternToToggles(slotPattern, maxArmySlotCount);
    bool bAnyClicked = false;
    for (std::size_t index = 0u; index < toggles.size(); ++index) {
        if (index > 0u) ImGui::SameLine();
        ImGui::PushID(static_cast<int>(index));
        const bool bHasArmy = index < armies.size();
        const ImU32 tint = bHasArmy
            ? ImGui::ColorConvertFloat4ToU32(ImVec4(armies[index].armyColor[0], armies[index].armyColor[1],
                                                    armies[index].armyColor[2], armies[index].armyColor[3]))
            : IM_COL32(96, 96, 96, 255);
        if (DrawSlotToggleCell(toggles[index], tint)) {
            toggles[index] = NextSlotToggleValue(toggles[index]);
            bAnyClicked = true;
        }
        if (bHasArmy && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", ArmyRowLabel(armies[index]));
        ImGui::PopID();
    }
    if (bAnyClicked) slotPattern = BuildSlotPatternFromToggles(toggles);
}

// One row per condition: field combo, comparator combo (display-only symbols), integer stepper for
// value; `+`/`x` add/remove. AND-of-clauses only — no OR-group UI (named future extension, Fix §3).
void DrawScenarioCountConditionsEditor(std::vector<Params::ScenarioCountCondition>& conditions) {
    ComboOptions fieldOptions;
    fieldOptions.labels = scenarioCountFieldLabels; fieldOptions.count = kScenarioCountFieldCount;
    ComboOptions comparatorOptions;
    comparatorOptions.labels = scenarioComparatorLabels; comparatorOptions.count = kScenarioComparatorCount;

    int removeIndex = -1;
    std::string summary = "Matches when: ";
    for (std::size_t index = 0u; index < conditions.size(); ++index) {
        ImGui::PushID(static_cast<int>(index));
        Params::ScenarioCountCondition& condition = conditions[index];
        // Field/Is are shared Combo_UI controls — each one claims the full row width internally
        // (Combo_UI.cpp), so they stack rather than sit side by side; only the compact Value stepper
        // and the remove button share a line.
        int fieldIndex = static_cast<int>(condition.field);
        if (DrawCombo("Field", fieldIndex, fieldOptions).bCommitted)
            condition.field = static_cast<Params::ScenarioCountField>(fieldIndex);
        int comparatorIndex = static_cast<int>(condition.comparator);
        if (DrawCombo("Is", comparatorIndex, comparatorOptions).bCommitted)
            condition.comparator = static_cast<Params::ScenarioComparator>(comparatorIndex);
        ImGui::SetNextItemWidth(80.0f);
        ImGui::InputInt("Value", &condition.value, 1);
        ImGui::SameLine();
        if (ImGui::SmallButton("x##removeCondition")) removeIndex = static_cast<int>(index);

        if (index > 0u) summary += " and ";
        summary += scenarioCountFieldLabels[fieldIndex];
        summary += " ";
        summary += scenarioComparatorSummaryTokens[comparatorIndex];
        summary += " " + std::to_string(condition.value);
        ImGui::PopID();
    }
    if (removeIndex >= 0) conditions.erase(conditions.begin() + removeIndex);
    if (ImGui::Button("+ Add Condition")) conditions.push_back(Params::ScenarioCountCondition());
    ImGui::TextWrapped("%s", conditions.empty() ? "Matches when: (always - no conditions authored)"
                                                : summary.c_str());
}

} // namespace Ui
} // namespace SanmapGen
