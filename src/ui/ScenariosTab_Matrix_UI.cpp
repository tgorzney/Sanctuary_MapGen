// ScenariosTab_Matrix_UI.cpp — Fix §6's live composition preview: a triangular grid over
// (total, human) with `ai` implied, resolved by walking countScenarios -> defaultScenario in match
// order. Tier 1 is never resolved by count alone (a triple does not determine a slot pattern), so a
// registered pattern scenario whose own h/A letter counts equal this cell's (human, ai) marks the
// cell with a corner hatch flag rather than a distinct fill. Layer: UI. At the default 16 the grid
// is <=17x17, so no virtualization (Fix §6).
#include "ScenariosTab_UI.h"
#include "imgui.h"
#include <functional>

namespace SanmapGen {
namespace Ui {
namespace {

std::string ResolvedScenarioNameForTriple(const Params::Scenarios& scenarios, int total, int human, int ai) {
    for (const Params::CountScenario& scenario : scenarios.countScenarios)
        if (MatchesScenarioConditions(scenario.conditions, total, human, ai))
            return ScenarioRowLabel(scenario.body);
    return ScenarioRowLabel(scenarios.defaultScenario);
}

// A registered exact pattern's own h/A letter counts fix ONE (human, ai) pair regardless of total —
// that is the only triple it could ever pre-empt.
bool AnyPatternScenarioMayPreempt(const Params::Scenarios& scenarios, int human, int ai) {
    for (const Params::PatternScenario& scenario : scenarios.patternScenarios) {
        int humanCount = 0, aiCount = 0;
        for (const char slotCharacter : scenario.slotPattern) {
            if (slotCharacter == 'h') ++humanCount;
            else if (slotCharacter == 'A') ++aiCount;
        }
        if (humanCount == human && aiCount == ai) return true;
    }
    return false;
}

// A deterministic, arbitrary color per resolved name — same name always reads the same color, so a
// map author can spot "this whole diagonal band is Scenario X" at a glance.
ImU32 HashedCellColor(const std::string& name) {
    const float hue = static_cast<float>(std::hash<std::string>()(name) % 360u) / 360.0f;
    float red, green, blue;
    ImGui::ColorConvertHSVtoRGB(hue, 0.55f, 0.85f, red, green, blue);
    return ImGui::ColorConvertFloat4ToU32(ImVec4(red, green, blue, 1.0f));
}

void DrawScenarioMatrixCell(const Params::Scenarios& scenarios, int total, int human, int ai, float cellSize) {
    const std::string resolvedName = ResolvedScenarioNameForTriple(scenarios, total, human, ai);
    const bool bMayBePreempted = AnyPatternScenarioMayPreempt(scenarios, human, ai);
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const ImVec2 farCorner(origin.x + cellSize, origin.y + cellSize);
    ImGui::InvisibleButton("##cell", ImVec2(cellSize, cellSize));
    ImDrawList* const drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(origin, farCorner, HashedCellColor(resolvedName), 2.0f);
    // The hatch overlay: a corner flag rather than full diagonal hatching (same "may pre-empt"
    // signal, far simpler geometry than clipped diagonal lines for a cosmetic indicator).
    if (bMayBePreempted)
        drawList->AddTriangleFilled(origin, ImVec2(origin.x + cellSize * 0.4f, origin.y),
                                    ImVec2(origin.x, origin.y + cellSize * 0.4f), IM_COL32(0, 0, 0, 200));
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Total %d / Human %d / AI %d\n%s%s", total, human, ai, resolvedName.c_str(),
                          bMayBePreempted
                              ? "\n(Hatched: a Tier 1 exact pattern MAY pre-empt this result)" : "");
}

} // namespace

void DrawScenarioMatrix(const Params::Scenarios& scenarios, SectionState& matrixSection) {
    if (!DrawSectionBegin("Composition Matrix", matrixSection)) return;
    ImGui::TextWrapped(
        "Hatched cells have at least one registered exact-pattern scenario that MAY pre-empt this "
        "result - Tier 1 depends on which slots are filled, not just how many.");
    const int maxTotal = scenarios.maxArmySlotCount < 0 ? 0 : scenarios.maxArmySlotCount;
    const float cellSize = ImGui::GetTextLineHeightWithSpacing();
    for (int total = 0; total <= maxTotal; ++total) {
        for (int human = 0; human <= total; ++human) {
            if (human > 0) ImGui::SameLine();
            ImGui::PushID(total * 1000 + human);
            DrawScenarioMatrixCell(scenarios, total, human, total - human, cellSize);
            ImGui::PopID();
        }
    }
    DrawSectionEnd();
}

} // namespace Ui
} // namespace SanmapGen
