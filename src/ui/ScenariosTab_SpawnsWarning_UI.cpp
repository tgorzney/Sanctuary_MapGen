// ScenariosTab_SpawnsWarning_UI.cpp — Fix §4's detail-panel banner (visibility tier 2 of 3). Tier 1
// (the list-row "\xE2\x9A\xA0 " prefix) is drawn inline where each row label is built
// (ScenariosTab_Lists_UI.cpp), and tier 3 (the export-time gate) is `AnyScenarioNeedsSpawnsAcknowledgment`
// in ScenariosTab_UI.h, exported for STEP77 to call — nothing to build here for it. Layer: UI.
//
// RESHAPED 2026-09-08 (STEP252, ARCH_15_12_ScenarioSpawnIdentity.md §15.12): [Set Explicit Spawns]
// no longer seeds a zeroed placeholder position (Correction 1's old text, now stale) — under the
// spawnId model there is nothing to seed a POSITION with at all: listing each army's own literal
// ARMY_XX name in `spawnIds` is itself the acknowledgment, and the two-step resolve
// (Io::ResolveSpawnId / ARCH_15_13) reads that army's live baked .sanmap position at match-load
// time, never a second stored copy of it.
#include "ScenariosTab_UI.h"
#include "imgui.h"

namespace SanmapGen {
namespace Ui {
namespace {

constexpr const char* kAcknowledgmentSentence =
    "Acknowledged: intentionally inherits the .sanmap baseline spawn.";

// One literal ARMY_XX spawnId per army — resolves live, at match-load time, to that army's own
// current .sanmap baked Spawn position (the literal-fallback branch of Io::ResolveSpawnId). Stores
// the engine identity (`Army::name`), never the display label (STEP76 amendment).
void SeedExplicitSpawnsFromArmies(Params::ScenarioBody& body, const std::vector<Params::Army>& armies) {
    body.spawnIds.clear();
    body.spawnIds.reserve(armies.size());
    for (const Params::Army& army : armies) body.spawnIds.push_back(army.name);
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
