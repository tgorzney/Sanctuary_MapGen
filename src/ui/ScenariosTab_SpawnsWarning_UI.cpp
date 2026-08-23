// ScenariosTab_SpawnsWarning_UI.cpp — Fix §4's detail-panel banner (visibility tier 2 of 3). Tier 1
// (the list-row "\xE2\x9A\xA0 " prefix) is drawn inline where each row label is built
// (ScenariosTab_Lists_UI.cpp), and tier 3 (the export-time gate) is `AnyScenarioNeedsSpawnsAcknowledgment`
// in ScenariosTab_UI.h, exported for STEP77 to call — nothing to build here for it. Layer: UI.
//
// Constitution §6: [Set Explicit Spawns] seeds a ZEROED starting point — there is no "current
// baseline spawn" value reachable from `Params::MapRecipe` (STEP78's canvas mode owns that). Stated,
// not hidden, per Correction 1's own text.
#include "ScenariosTab_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

constexpr const char* kAcknowledgmentSentence =
    "Acknowledged: intentionally inherits the .sanmap baseline spawn.";

// One `ScenarioSpawn` per army, positioned at the origin — the honest zeroed placeholder Correction
// 1 documents; the designer hand-edits real positions via the flat list editor (Fix §5). Stores the
// engine identity (`Army::name`), never the display label (STEP76 amendment).
void SeedExplicitSpawnsFromArmies(Params::ScenarioBody& body, const std::vector<Params::Army>& armies) {
    body.spawns.clear();
    body.spawns.reserve(armies.size());
    for (const Params::Army& army : armies) {
        Params::ScenarioSpawn spawn;
        spawn.armyName = army.name;
        body.spawns.push_back(spawn);
    }
}

} // namespace

void DrawScenarioSpawnsWarningBanner(Params::ScenarioBody& body, const std::vector<Params::Army>& armies) {
    if (!ScenarioNeedsSpawnsAcknowledgment(body)) return;
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.75f, 0.15f, 1.0f));
    ImGui::TextWrapped(
        "No explicit spawn positions. This scenario will use whatever the .sanmap's shared baseline "
        "spawn currently is - which changes if ANY other scenario's baseline edit touches it.");
    ImGui::PopStyleColor();
    if (ImGui::Button("Set Explicit Spawns")) SeedExplicitSpawnsFromArmies(body, armies);
    ImGui::SameLine();
    if (ImGui::Button("I understand, inherit baseline")
        && body.authoringNote.find(kAcknowledgmentSentence) == std::string::npos) {
        if (!body.authoringNote.empty()) body.authoringNote += " ";
        body.authoringNote += kAcknowledgmentSentence;
    }
}

} // namespace Ui
} // namespace SanmapGen
