// ScenariosTab_SpawnPointPool_UI.cpp — Scenarios::spawnPoints, the shared custom-spawn-point pool
// editor (ARCH_15_12_ScenarioSpawnIdentity.md §15.12, STEP252). Layer: UI. Its own dedicated
// section/file, structurally similar to how `recipe.areas` gets AreasTab_List_UI.h as its own
// dedicated list — the pool is Scenarios-level, authored ONCE, never per-scenario-body.
//
// Backend policy N/A (no PROC stage reads `recipe.scenarios`) — same posture as every other
// ScenariosTab_*_UI.cpp file (see ScenariosTab_Detail_UI.cpp's own header note).
#include "ScenariosTab_UI.h"
#include "SliderScalar_UI.h"
#include "TextInput_UI.h"
#include "imgui.h"
#include "../io/ScenarioSpawnIdValidation_IO.h"

namespace SanmapGen {
namespace Ui {
namespace {

// A generous, world-coordinate-scale range — mirrors ScenariosTab_Detail_UI.cpp's own
// ScenarioWorldPositionRange (deliberately duplicated, small, per-file precedent already
// established throughout this exact file family).
ScalarSliderRange SpawnPointWorldPositionRange() { return ScalarSliderRange{ -8192.0f, 8192.0f, 0.0f }; }

// One pool row: spawnId (free text — the pool is the AUTHORITATIVE source of spawnIds, so there is
// nothing to pick FROM here, unlike the per-scenario spawnIds editor's own Combo), armyName (a
// DrawArmyNameField picker, ScenariosTab_Detail_UI.cpp), and x/y/z.
void DrawScenarioSpawnPointRow(Params::ScenarioSpawnPoint& point, const std::vector<Params::Army>& armies) {
    const ScalarSliderRange range = SpawnPointWorldPositionRange();
    TextInputRules idRules; idRules.bAllowEmpty = true; idRules.maximumLength = 48;
    DrawTextInput("Spawn Id", point.spawnId, idRules);
    DrawArmyNameField("Army", point.armyName, armies);
    RealtimeToggle xToggle, yToggle, zToggle;
    ImGui::SetNextItemWidth(90.0f);
    DrawSliderScalar("X", point.positionX, range, xToggle, WidgetStyle(), "%.1f");
    ImGui::SameLine(); ImGui::SetNextItemWidth(90.0f);
    DrawSliderScalar("Y", point.positionY, range, yToggle, WidgetStyle(), "%.1f");
    ImGui::SameLine(); ImGui::SetNextItemWidth(90.0f);
    DrawSliderScalar("Z", point.positionZ, range, zToggle, WidgetStyle(), "%.1f");
}

// Live, non-blocking inline warning for whichever ScenarioSpawnIdValidationReport violations are
// "pool"-descriptor ones — a scenario-specific violation is shown by the per-scenario editor
// instead (ScenariosTab_DetailSpawns_UI.cpp's own DrawScenarioSpawnIdWarnings).
void DrawSpawnPointPoolWarnings(const Params::Scenarios& scenarios) {
    const Io::ScenarioSpawnIdValidationReport report = Io::ValidateScenarioSpawnIds(scenarios);
    for (const Io::ScenarioSpawnIdValidationReport::Violation& violation : report.violations) {
        if (violation.descriptor != "pool") continue;
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.75f, 0.15f, 1.0f));
        ImGui::TextWrapped("\xE2\x9A\xA0 %s: %s", violation.detail.c_str(), violation.reason.c_str());
        ImGui::PopStyleColor();
    }
}

} // namespace

void DrawScenarioSpawnPointPoolFields(Params::Scenarios& scenarios, SectionState& poolSection,
                                      const std::vector<Params::Army>& armies) {
    if (!DrawSectionBegin("Custom Spawn Points", poolSection)) return;
    ImGui::TextWrapped("Shared across every scenario below (referenced by spawnId from any "
        "scenario's Spawns list). spawnId must not look like ARMY_XX (that namespace is reserved).");
    int removeIndex = -1;
    for (std::size_t index = 0u; index < scenarios.spawnPoints.size(); ++index) {
        ImGui::PushID(static_cast<int>(index));
        DrawScenarioSpawnPointRow(scenarios.spawnPoints[index], armies);
        if (ImGui::SmallButton("Remove##removeSpawnPoint")) removeIndex = static_cast<int>(index);
        ImGui::Separator();
        ImGui::PopID();
    }
    if (removeIndex >= 0) scenarios.spawnPoints.erase(scenarios.spawnPoints.begin() + removeIndex);
    if (ImGui::Button("+ Add Spawn Point")) scenarios.spawnPoints.push_back(Params::ScenarioSpawnPoint());
    DrawSpawnPointPoolWarnings(scenarios);
    DrawSectionEnd();
}

} // namespace Ui
} // namespace SanmapGen
