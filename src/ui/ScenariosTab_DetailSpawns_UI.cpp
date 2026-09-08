// ScenariosTab_DetailSpawns_UI.cpp — the per-scenario `body.spawnIds` picker + its own live inline
// warning, split out of ScenariosTab_Detail_UI.cpp for the ARCH §1.5 file-size ceiling (mirrors
// ScenariosTab_DetailAlloys_UI.cpp's own split, called only by DrawScenarioBodyFields there).
// Layer: UI. STEP252 (ARCH_15_12_ScenarioSpawnIdentity.md §15.12) — retires the old 3-slider-per-row
// ScenarioSpawn editor entirely: `ScenarioBody::spawnIds` is a flat array of bare strings now,
// positions are never authored here (either resolved from `Scenarios::spawnPoints`, or read live off
// the .sanmap's own baked ARMY_XX default at match-load time).
//
// `DrawArmyNameField` is declared in ScenariosTab_UI.h and DEFINED here (not anonymous-namespace-
// private) — shared by this file's own DrawSpawnIdField AND the spawn-point pool editor
// (ScenariosTab_SpawnPointPool_UI.cpp).
#include "ScenariosTab_UI.h"
#include "ArmiesTab_UI.h"
#include "Combo_UI.h"
#include "TextInput_UI.h"
#include "imgui.h"
#include "../io/ScenarioSpawnIdValidation_IO.h"

namespace SanmapGen {
namespace Ui {
namespace {

// One row's spawnId picker: a Combo over the pool's own spawnIds plus every ARMY_XX name (either
// is a legal reference, ARCH_15_12_ScenarioSpawnIdentity.md §15.12) — falls back to a free-text
// field when neither exists yet, matching DrawArmyNameField's own no-options posture.
void DrawSpawnIdField(const char* label, std::string& spawnId,
                      const std::vector<Params::ScenarioSpawnPoint>& spawnPoints,
                      const std::vector<Params::Army>& armies) {
    std::vector<std::string> optionStrings;
    optionStrings.reserve(spawnPoints.size() + armies.size());
    for (const Params::ScenarioSpawnPoint& point : spawnPoints) optionStrings.push_back(point.spawnId);
    for (const Params::Army& army : armies) optionStrings.push_back(army.name);
    if (optionStrings.empty()) {
        TextInputRules rules; rules.bAllowEmpty = true; rules.maximumLength = 48;
        DrawTextInput(label, spawnId, rules);
        return;
    }
    std::vector<const char*> labels;
    labels.reserve(optionStrings.size());
    int selectedIndex = -1;
    for (std::size_t index = 0u; index < optionStrings.size(); ++index) {
        labels.push_back(optionStrings[index].c_str());
        if (optionStrings[index] == spawnId) selectedIndex = static_cast<int>(index);
    }
    ComboOptions options; options.labels = labels.data(); options.count = static_cast<int>(labels.size());
    if (DrawCombo(label, selectedIndex, options).bCommitted && selectedIndex >= 0)
        spawnId = optionStrings[static_cast<std::size_t>(selectedIndex)];
}

// Live, non-blocking inline warning (ImGui::TextColored) for whichever ScenarioSpawnIdValidationReport
// violations name THIS scenario specifically (never the pool's own "pool"-descriptor violations,
// which the spawn-point pool editor shows instead — ScenariosTab_SpawnPointPool_UI.cpp).
void DrawScenarioSpawnIdWarnings(const Params::ScenarioBody& body, const Params::Scenarios& scenarios) {
    const std::string descriptor = body.name.empty() ? std::string("(unnamed scenario)") : body.name;
    const Io::ScenarioSpawnIdValidationReport report = Io::ValidateScenarioSpawnIds(scenarios);
    for (const Io::ScenarioSpawnIdValidationReport::Violation& violation : report.violations) {
        if (violation.descriptor != descriptor) continue;
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.75f, 0.15f, 1.0f));
        ImGui::TextWrapped("\xE2\x9A\xA0 %s: %s", violation.detail.c_str(), violation.reason.c_str());
        ImGui::PopStyleColor();
    }
}

// Flat list, no drag-reorder (order not load-bearing). Cardinality is tens of rows, so a plain loop
// is correct — no VirtualListWidget_UI (Fix §5: that is for 100k rows).
void DrawScenarioSpawnIdsList(std::vector<std::string>& spawnIds,
                              const std::vector<Params::ScenarioSpawnPoint>& spawnPoints,
                              const std::vector<Params::Army>& armies) {
    int removeIndex = -1;
    for (std::size_t index = 0u; index < spawnIds.size(); ++index) {
        ImGui::PushID(static_cast<int>(index));
        DrawSpawnIdField("Spawn Id", spawnIds[index], spawnPoints, armies);
        ImGui::SameLine();
        if (ImGui::SmallButton("X##removeSpawnId")) removeIndex = static_cast<int>(index);
        ImGui::PopID();
    }
    if (removeIndex >= 0) spawnIds.erase(spawnIds.begin() + removeIndex);
    if (ImGui::Button("+ Add Spawn Id")) spawnIds.push_back(std::string());
}

} // namespace

// armies[i].displayName label, `Army::name` key — the Combo shows the human label, the stored
// `armyNameKey` stays the machine identity (STEP76 amendment). Falls back to a free-text field when
// no armies are authored yet, so authoring is never blocked. Declared in ScenariosTab_UI.h (not
// anonymous-namespace-private) so ScenariosTab_SpawnPointPool_UI.cpp can share it (STEP252).
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

void DrawScenarioSpawnIdsSection(Params::ScenarioBody& body, const Params::Scenarios& scenarios,
                                 const std::vector<Params::Army>& armies) {
    DrawScenarioSpawnIdsList(body.spawnIds, scenarios.spawnPoints, armies);
    DrawScenarioSpawnIdWarnings(body, scenarios);
}

} // namespace Ui
} // namespace SanmapGen
