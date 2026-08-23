// MapCanvas_ScenarioEditMode_Baseline_UI.cpp — reads the real baked Alloy/SpawnsArmies marker
// instances (STEP50's Data::RuleBucketIndex CSR) as ghost-baseline candidates. Layer: UI. Pure,
// imgui-free, headless-testable. Walks each domain's ProceduralRule sub-layers exactly as
// MapCanvas_IconLayer_CullProcedural_UI.cpp's own ResolveProceduralSubLayer does (STEP53's own
// resolution pattern, mirrored here rather than reinvented, per STEP78's own instruction) — but
// does not include that module's restricted-scope internal header (only its own cull/draw trio may
// include it); the walk itself is five lines, cheap enough to duplicate rather than widen that
// module's own documented consumer set.
#include "MapCanvas_ScenarioEditMode_UI.h"
#include "OverlayLayer_Settings_UI.h"
#include "../data/PlacementResults_DATA.h"
#include "../data/RuleBucketIndexSet_DATA.h"

namespace SanmapGen {
namespace Ui {
namespace {

std::string TemplateIdentifierToString8Local(const char* characters) {
    std::string result;
    for (int index = 0; index < 7 && characters[index] != '\0'; ++index) result.push_back(characters[index]);
    return result;
}

bool BaselineSourcesReady(const ScenarioEditModeResolveInput& input) {
    return input.overlayLayerSettings != nullptr && input.placements != nullptr
        && input.ruleBucketIndex != nullptr;
}

} // namespace

void ResolveScenarioEditModeBaselineAlloys(const ScenarioEditModeResolveInput& input,
                                           std::vector<ScenarioEditModeBaselineInstance_UI>& outBaseline) {
    outBaseline.clear();
    if (!BaselineSourcesReady(input)) return;
    const Data::RuleBucketIndex& bucket = input.ruleBucketIndex->markers;
    for (const OverlayLayer_UI& layer : input.overlayLayerSettings->overlayLayers) {
        if (layer.domainKind != OverlayDomainKind_UI::Alloy) continue;
        for (const OverlaySubLayerRef_UI& subLayer : layer.subLayers) {
            if (subLayer.kind != OverlaySubLayerKind_UI::ProceduralRule) continue;
            const int ruleIndex = subLayer.index;
            const std::int32_t bucketBegin = bucket.BucketBegin(ruleIndex);
            const std::int32_t bucketEnd   = bucket.BucketEnd(ruleIndex);
            for (std::int32_t position = bucketBegin; position < bucketEnd; ++position) {
                const std::int32_t instanceIndex = bucket.InstanceIndexAt(position);
                if (instanceIndex < 0
                    || static_cast<std::size_t>(instanceIndex) >= input.placements->markers.Count())
                    continue;
                const std::size_t index = static_cast<std::size_t>(instanceIndex);
                ScenarioEditModeBaselineInstance_UI instance;
                instance.worldX = input.placements->markers.positionX[index];
                instance.worldY = input.placements->markers.positionY[index];
                instance.worldZ = input.placements->markers.positionZ[index];
                instance.templateIdentifier =
                    TemplateIdentifierToString8Local(input.placements->markers.templateIdentifier[index].characters);
                // §0 — the deterministic markerName convention this module's whole alloy round
                // trip (right-click remove -> ScenarioAlloyRemoval::markerName -> re-resolved next
                // frame) depends on.
                instance.markerName = "alloy_r" + std::to_string(ruleIndex) + "_"
                                     + std::to_string(position - bucketBegin);
                outBaseline.push_back(instance);
            }
        }
    }
}

// §0's positional spawn->army convention: armyIndex is this instance's 0-based position across
// EVERY SpawnsArmies sub-layer, walked in seed order (Application_OverlaySetup_UI.cpp's
// SeedMarkerDomains flattens `recipe.markerRuleLayers` layer-then-rule, so this walk visits
// sub-layers in that same order for an identical result).
void ResolveScenarioEditModeBaselineSpawns(const ScenarioEditModeResolveInput& input,
                                           std::vector<ScenarioEditModeBaselineInstance_UI>& outBaseline) {
    outBaseline.clear();
    if (!BaselineSourcesReady(input)) return;
    const Data::RuleBucketIndex& bucket = input.ruleBucketIndex->markers;
    int nextArmyIndex = 0;
    for (const OverlayLayer_UI& layer : input.overlayLayerSettings->overlayLayers) {
        if (layer.domainKind != OverlayDomainKind_UI::SpawnsArmies) continue;
        for (const OverlaySubLayerRef_UI& subLayer : layer.subLayers) {
            if (subLayer.kind != OverlaySubLayerKind_UI::ProceduralRule) continue;
            const int ruleIndex = subLayer.index;
            const std::int32_t bucketBegin = bucket.BucketBegin(ruleIndex);
            const std::int32_t bucketEnd   = bucket.BucketEnd(ruleIndex);
            for (std::int32_t position = bucketBegin; position < bucketEnd; ++position) {
                const std::int32_t instanceIndex = bucket.InstanceIndexAt(position);
                if (instanceIndex < 0
                    || static_cast<std::size_t>(instanceIndex) >= input.placements->markers.Count())
                    continue;
                const std::size_t index = static_cast<std::size_t>(instanceIndex);
                ScenarioEditModeBaselineInstance_UI instance;
                instance.worldX = input.placements->markers.positionX[index];
                instance.worldY = input.placements->markers.positionY[index];
                instance.worldZ = input.placements->markers.positionZ[index];
                instance.templateIdentifier =
                    TemplateIdentifierToString8Local(input.placements->markers.templateIdentifier[index].characters);
                instance.armyIndex = nextArmyIndex++;
                outBaseline.push_back(instance);
            }
        }
    }
}

} // namespace Ui
} // namespace SanmapGen
