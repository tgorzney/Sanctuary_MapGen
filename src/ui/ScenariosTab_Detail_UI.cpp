// ScenariosTab_Detail_UI.cpp — Fix §5's core `ScenarioBody` fields: name, area, spawnsUnits,
// alloyMode (with its consequence card), the spawns list, and authoringNote.
// alloys/alloysToAdd/alloysToRemove are ScenariosTab_DetailAlloys_UI.cpp's half (ARCH §1.5 split).
// Layer: UI. `spawnsUnits` RENAMED 2026-08-28, was `navy` (STEP204, ARCH_15_05 §15.5 amended).
//
// Backend policy N/A (no PROC stage reads `recipe.scenarios`): every scalar here uses a THROWAWAY,
// function-local `RealtimeToggle` rather than one persisted in `ScenariosTabState` — the value write
// on drag already happens unconditionally inside `StepScalarSliderInteraction` before the toggle is
// even consulted, so a toggle with no cross-frame memory costs nothing real; only `bCommitted`'s
// exact frame is imprecise, and nothing here reads `bCommitted` (there is no dirty flag to trip).
//
// Constitution §8: alloyMode's four labels each carry a real consequence card, per Fix §5.
#include "ScenariosTab_UI.h"
#include "ArmiesTab_UI.h"
#include "Checkbox_UI.h"
#include "Combo_UI.h"
#include "SliderScalar_UI.h"
#include "TextInput_UI.h"
#include "imgui.h"
#include <cstring>

namespace SanmapGen {
namespace Ui {
namespace {

enum : int { kScenarioAlloyModeCount = 4 };
const char* const scenarioAlloyModeLabels[kScenarioAlloyModeCount] = { "Explicit", "Occupancy", "Keep All", "Delta" };
const char* const scenarioAlloyModeConsequenceCards[kScenarioAlloyModeCount] = {
    "You list every army's alloys below. Any army NOT listed loses its alloy markers entirely.",
    "Uses the map's own baked alloy positions. Empty army slots lose their markers; filled slots keep them.",
    "Uses the map's own baked alloy positions. Nothing is ever deleted, even for empty slots.",
    "\xE2\x9A\xA0 Reserved - not yet used by any shipped scenario. Only listed Adds/Removes apply."
};

// A generous, world-coordinate-scale range: DrawScenarioBodyFields carries no `mapSize` to derive a
// tighter one from (unlike AreasTab_UI's AreaOriginSliderRange), so this stays a fixed constant.
ScalarSliderRange ScenarioWorldPositionRange() { return ScalarSliderRange{ -8192.0f, 8192.0f, 0.0f }; }

// armies[i].displayName label, `Army::name` key — the Combo shows the human label, the stored
// `armyName` stays the machine identity (STEP76 amendment). Falls back to a free-text field when no
// armies are authored yet, so authoring is never blocked.
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

// `area.name` is never shown/edited (STEP69 §1: no JSON counterpart, must stay empty) — only the
// four rectangle scalars are drawn, same per-field pattern AreasTab_UI uses.
void DrawScenarioAreaFields(Params::MapArea& area) {
    const ScalarSliderRange range = ScenarioWorldPositionRange();
    RealtimeToggle originXToggle, originZToggle, widthToggle, lengthToggle;
    DrawSliderScalar("Area Origin X", area.originX, range, originXToggle, WidgetStyle(), "%.1f");
    DrawSliderScalar("Area Origin Z", area.originZ, range, originZToggle, WidgetStyle(), "%.1f");
    DrawSliderScalar("Area Width", area.width, range, widthToggle, WidgetStyle(), "%.1f");
    DrawSliderScalar("Area Length", area.length, range, lengthToggle, WidgetStyle(), "%.1f");
}

void DrawScenarioAlloyModeField(Params::ScenarioBody& body) {
    ComboOptions options; options.labels = scenarioAlloyModeLabels; options.count = kScenarioAlloyModeCount;
    int modeIndex = static_cast<int>(body.alloyMode);
    if (DrawCombo("Alloy Mode", modeIndex, options).bCommitted)
        body.alloyMode = static_cast<Params::ScenarioAlloyMode>(modeIndex);
    ImGui::TextWrapped("%s", scenarioAlloyModeConsequenceCards[modeIndex]);
}

// Flat list, no drag-reorder (order not load-bearing). Cardinality is tens of rows, so a plain loop
// is correct — no VirtualListWidget_UI (Fix §5: that is for 100k rows).
void DrawScenarioSpawnsList(std::vector<Params::ScenarioSpawn>& spawns, const std::vector<Params::Army>& armies) {
    const ScalarSliderRange range = ScenarioWorldPositionRange();
    int removeIndex = -1;
    for (std::size_t index = 0u; index < spawns.size(); ++index) {
        ImGui::PushID(static_cast<int>(index));
        Params::ScenarioSpawn& spawn = spawns[index];
        DrawArmyNameField("Army", spawn.armyName, armies);
        RealtimeToggle xToggle, yToggle, zToggle;
        ImGui::SameLine(); ImGui::SetNextItemWidth(90.0f);
        DrawSliderScalar("X", spawn.positionX, range, xToggle, WidgetStyle(), "%.1f");
        ImGui::SameLine(); ImGui::SetNextItemWidth(90.0f);
        DrawSliderScalar("Y", spawn.positionY, range, yToggle, WidgetStyle(), "%.1f");
        ImGui::SameLine(); ImGui::SetNextItemWidth(90.0f);
        DrawSliderScalar("Z", spawn.positionZ, range, zToggle, WidgetStyle(), "%.1f");
        ImGui::SameLine();
        if (ImGui::SmallButton("X##removeSpawn")) removeIndex = static_cast<int>(index);
        ImGui::PopID();
    }
    if (removeIndex >= 0) spawns.erase(spawns.begin() + removeIndex);
    if (ImGui::Button("+ Add Spawn")) spawns.push_back(Params::ScenarioSpawn());
}

// Raw `ImGui::InputTextMultiline` per Fix §5's own text — a multi-line box has no shared-library
// widget yet, so this stages a local buffer exactly as TextInput_UI.cpp does for the single-line one.
void DrawAuthoringNoteField(std::string& authoringNote) {
    char buffer[1024];
    std::size_t writtenLength = authoringNote.size() < sizeof(buffer) - 1u
        ? authoringNote.size() : sizeof(buffer) - 1u;
    std::memcpy(buffer, authoringNote.data(), writtenLength);
    buffer[writtenLength] = '\0';
    if (ImGui::InputTextMultiline("##authoringNote", buffer, sizeof(buffer), ImVec2(-1.0f, 60.0f)))
        authoringNote = buffer;
}

// STEP78 — mode-entry toggle, default off. A local bool mirrors whether the single `editedBody`
// slot points at THIS scenario; flipping it steals that slot (Activate) or frees it (Deactivate).
// nullptr `editModeState` draws nothing (feature not wired), never a crash.
void DrawScenarioEditModeToggle(Params::ScenarioBody& body, ScenarioEditModeState* editModeState,
                                const std::string* patternSlotPattern,
                                const std::vector<Params::ScenarioCountCondition>* countConditions,
                                int maxArmySlotCount) {
    if (editModeState == nullptr) return;
    bool bEditingThisScenario = editModeState->editedBody == &body;
    if (DrawCheckbox("Edit On Map (Scenario Edit Mode)", bEditingThisScenario).bValueChanged) {
        if (bEditingThisScenario) editModeState->Activate(body, patternSlotPattern, countConditions, maxArmySlotCount);
        else editModeState->Deactivate();
    }
}

} // namespace

void DrawScenarioBodyFields(Params::ScenarioBody& body, const std::vector<Params::Army>& armies,
                            ScenarioEditModeState* editModeState, const std::string* patternSlotPattern,
                            const std::vector<Params::ScenarioCountCondition>* countConditions,
                            int maxArmySlotCount) {
    TextInputRules nameRules; nameRules.maximumLength = 64; nameRules.bAllowEmpty = true;
    DrawTextInput("Name", body.name, nameRules);
    DrawScenarioEditModeToggle(body, editModeState, patternSlotPattern, countConditions, maxArmySlotCount);
    ImGui::SeparatorText("Area");
    DrawScenarioAreaFields(body.area);
    DrawCheckbox("Spawns Units", body.spawnsUnits);
    // Two-step opt-in, load-bearing to say out loud (ARCH_15_05_ParamsScenariosType.md §15.5,
    // MAP_SCENARIO_SPEC.md §11): this checkbox alone spawns nothing. A matching name-keyed branch
    // must ALSO exist in the map's own Lua dispatch. This UI does not, and cannot, validate that
    // such a branch exists (ARCH §15.5 OPEN item 2 — not this ticket's to resolve).
    ImGui::TextWrapped("%s", "\xE2\x9A\xA0 Setting this alone spawns nothing. The map's own Lua "
        "runtime also needs a matching branch, keyed off this scenario's Name, that actually "
        "spawns units.");
    ImGui::SeparatorText("Alloys");
    DrawScenarioAlloyModeField(body);
    ImGui::SeparatorText("Spawns");
    DrawScenarioSpawnsList(body.spawns, armies);
    ImGui::SeparatorText("Authoring Note");
    DrawAuthoringNoteField(body.authoringNote);
    DrawScenarioBodyExtendedFields(body, armies);
}

} // namespace Ui
} // namespace SanmapGen
