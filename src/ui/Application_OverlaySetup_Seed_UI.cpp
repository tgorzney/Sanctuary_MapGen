// Application_OverlaySetup_Seed_UI.cpp — STEP83 §1.5 split of Application_OverlaySetup_UI.cpp: the
// five launch-time seeding helpers `ConfigureDefaultOverlayLayers` calls. Split out once the
// procedural/manual Props-Reclaim partition (SeedPropReclaimDomains, STEP83 Item 1) pushed the
// original file over the soft-100 line ceiling (ARCH_01_05_FileSizeCeilings.md). Layer: UI. Same
// one-shot, launch-time-only posture as the file it was split from.
#include "Application_Defaults_UI.h"

namespace SanmapGen {
namespace Ui {

void PushProceduralRefs(std::vector<OverlaySubLayerRef_UI>& subLayers, int ruleCount) {
    for (int index = 0; index < ruleCount; ++index)
        subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::ProceduralRule, index, true});
}

void PushManualRefs(std::vector<OverlaySubLayerRef_UI>& subLayers, int recordCount) {
    for (int index = 0; index < recordCount; ++index)
        subLayers.push_back(OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::Manual, index, true});
}

// CONFIRMED (STEP79 "Downstream authority ruling"): flat/global index over the layer-concatenated
// rule sequence — see this ticket's header note and STEP50's matching, now-confirmed assumption.
// Zero Manual refs for either domain (STEP51 scope; Manual Alloy/SpawnsArmies routing over
// `recipe.markerLayers` is a later, ARCH_14_14-ruled successor ticket, gated on this one landing).
void SeedMarkerDomains(OverlayLayer_UI& alloyLayer, OverlayLayer_UI& spawnsArmiesLayer,
                       const Params::MapRecipe& recipe) {
    int flatIndex = 0;
    for (const Params::MarkerRuleLayer& layer : recipe.markerRuleLayers) {
        for (const Params::MarkerRule& rule : layer.rules) {
            OverlayLayer_UI& target = rule.category == Params::MarkerCategory::Spawn
                                           ? spawnsArmiesLayer : alloyLayer;
            target.subLayers.push_back(OverlaySubLayerRef_UI{
                OverlaySubLayerKind_UI::ProceduralRule, flatIndex, true});
            ++flatIndex;
        }
    }
}

void SeedUnitsManualSubLayers(OverlayLayer_UI& unitsLayer, const Params::MapRecipe& recipe) {
    int flatIndex = 0;
    for (const Params::Army& army : recipe.armies)
        for (std::size_t group = 0; group < army.groups.size(); ++group)
            unitsLayer.subLayers.push_back(
                OverlaySubLayerRef_UI{OverlaySubLayerKind_UI::Manual, flatIndex++, true});
}

// Props/Reclaim mutually-exclusively partition `recipe.propRules` by `bReclaimable` — the same
// seed-time routing SeedMarkerDomains performs on `category` (§14.6; STEP62's "exact same
// pattern"). `index` stays the GLOBAL recipe.propRules position, never a per-domain running
// count: it is what STEP50's CSR bucket is keyed on (bucketTotal == recipe.propRules.size()).
// Manual refs go to BOTH domains on purpose — one recipe.propLayers[k] can hold transforms from
// reclaimable AND non-reclaimable PropInstanceGroups, so a propLayer is not uniformly one
// domain's; that half of the partition closes at group granularity in the draw pass (STEP83 §5).
void SeedPropReclaimDomains(OverlayLayer_UI& propsLayer, OverlayLayer_UI& reclaimLayer,
                            const Params::MapRecipe& recipe) {
    const int manualLayerCount = static_cast<int>(recipe.propLayers.size());
    PushManualRefs(propsLayer.subLayers,   manualLayerCount);   // Manual first — STEP51's order
    PushManualRefs(reclaimLayer.subLayers, manualLayerCount);
    for (std::size_t index = 0; index < recipe.propRules.size(); ++index) {
        OverlayLayer_UI& target = recipe.propRules[index].bReclaimable ? reclaimLayer : propsLayer;
        target.subLayers.push_back(OverlaySubLayerRef_UI{
            OverlaySubLayerKind_UI::ProceduralRule, static_cast<int>(index), true});
    }
}

} // namespace Ui
} // namespace SanmapGen
