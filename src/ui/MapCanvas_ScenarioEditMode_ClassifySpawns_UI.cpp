// MapCanvas_ScenarioEditMode_ClassifySpawns_UI.cpp — one candidate per army: SpawnExplicit when
// `body.spawns` already names it, else SpawnNoOverride at the real baked baseline position (§0's
// positional army convention, MapCanvas_ScenarioEditMode_UI.h) when one exists. Layer: UI. Pure,
// imgui-free, headless-testable.
#include "MapCanvas_ScenarioEditMode_UI.h"
#include "../params/Army_PARAMS.h"

namespace SanmapGen {
namespace Ui {
namespace {

int FindSpawnRowForArmy(const std::vector<Params::ScenarioSpawn>& spawns, const std::string& armyName) {
    for (std::size_t index = 0; index < spawns.size(); ++index)
        if (spawns[index].armyName == armyName) return static_cast<int>(index);
    return kScenarioEditModeNoIndex;
}

const ScenarioEditModeBaselineInstance_UI* FindBaselineForArmy(
    const std::vector<ScenarioEditModeBaselineInstance_UI>& baseline, int armyIndex) {
    for (const ScenarioEditModeBaselineInstance_UI& instance : baseline)
        if (instance.armyIndex == armyIndex) return &instance;
    return nullptr;
}

} // namespace

void AppendScenarioEditModeSpawnCandidates(const ScenarioEditModeResolveInput& input,
                                           const Params::ScenarioBody& body,
                                           std::vector<ScenarioEditMarkerCandidate_UI>& outCandidates) {
    if (input.armies == nullptr) return;
    std::vector<ScenarioEditModeBaselineInstance_UI> baseline;
    ResolveScenarioEditModeBaselineSpawns(input, baseline);

    for (std::size_t armyIndex = 0; armyIndex < input.armies->size(); ++armyIndex) {
        const Params::Army& army = (*input.armies)[armyIndex];
        const int spawnRow = FindSpawnRowForArmy(body.spawns, army.name);
        const ScenarioEditModeBaselineInstance_UI* baselineInstance =
            FindBaselineForArmy(baseline, static_cast<int>(armyIndex));

        ScenarioEditMarkerCandidate_UI candidate;
        candidate.kind = ScenarioMarkerKind_UI::Spawn;
        candidate.armyIndex = static_cast<int>(armyIndex);

        if (spawnRow != kScenarioEditModeNoIndex) {
            const Params::ScenarioSpawn& spawn = body.spawns[static_cast<std::size_t>(spawnRow)];
            candidate.state = ScenarioMarkerVisualState_UI::SpawnExplicit;
            candidate.worldX = spawn.positionX; candidate.worldY = spawn.positionY; candidate.worldZ = spawn.positionZ;
            candidate.spawnRowIndex = spawnRow;
            if (baselineInstance != nullptr) candidate.templateIdentifier = baselineInstance->templateIdentifier;
            outCandidates.push_back(candidate);
        } else if (baselineInstance != nullptr) {
            // No real baseline position, no override: nothing to draw/materialize from — an army
            // with more slots than baked spawn instances is a recipe-authoring gap outside this
            // module's scope (it never invents a zeroed placeholder — STEP74 §4's own flagged gap
            // this ticket exists to close, the opposite direction).
            candidate.state = ScenarioMarkerVisualState_UI::SpawnNoOverride;
            candidate.worldX = baselineInstance->worldX; candidate.worldY = baselineInstance->worldY;
            candidate.worldZ = baselineInstance->worldZ;
            candidate.templateIdentifier = baselineInstance->templateIdentifier;
            outCandidates.push_back(candidate);
        }
    }
}

} // namespace Ui
} // namespace SanmapGen
