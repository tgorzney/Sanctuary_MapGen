// Application_OverlaySetup_UI.cpp — launch-time default seeding for the six-domain overlay stack
// (`OverlayLayer_Settings_UI.h`, ARCH_14_02_DataModel.md §14.2). Layer: UI. One-shot: runs once, in
// `Application`'s constructor, mirroring `ConfigureDefaultPreview`'s own posture for `fieldLayers`
// (`Application_Defaults_UI.h`). No live resync as the recipe grows mid-session — out of scope here
// (STEP51).
#include "Application_Defaults_UI.h"

namespace SanmapGen {
namespace Ui {
namespace {

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

} // namespace

void ConfigureDefaultOverlayLayers(OverlayLayerSettings& overlaySettings,
                                    const Params::MapRecipe& recipe) {
    OverlayLayer_UI alloyLayer;        alloyLayer.name        = "Alloy";
    alloyLayer.domainKind                                     = OverlayDomainKind_UI::Alloy;
    OverlayLayer_UI spawnsArmiesLayer; spawnsArmiesLayer.name = "Spawns/Armies";
    spawnsArmiesLayer.domainKind                               = OverlayDomainKind_UI::SpawnsArmies;
    SeedMarkerDomains(alloyLayer, spawnsArmiesLayer, recipe);

    OverlayLayer_UI unitsLayer; unitsLayer.name = "Units";
    unitsLayer.domainKind = OverlayDomainKind_UI::Units;
    SeedUnitsManualSubLayers(unitsLayer, recipe);
    PushProceduralRefs(unitsLayer.subLayers, static_cast<int>(recipe.unitRules.size()));

    OverlayLayer_UI propsLayer; propsLayer.name = "Props";
    propsLayer.domainKind = OverlayDomainKind_UI::Props;
    PushManualRefs(propsLayer.subLayers, static_cast<int>(recipe.propLayers.size()));
    PushProceduralRefs(propsLayer.subLayers, static_cast<int>(recipe.propRules.size()));

    OverlayLayer_UI reclaimLayer; reclaimLayer.name = "Reclaim";
    reclaimLayer.domainKind = OverlayDomainKind_UI::Reclaim;   // stays empty — no data/rule yet

    OverlayLayer_UI decalsLayer; decalsLayer.name = "Decals";
    decalsLayer.domainKind = OverlayDomainKind_UI::Decals;
    PushManualRefs(decalsLayer.subLayers, static_cast<int>(recipe.decalLayers.size()));
    PushProceduralRefs(decalsLayer.subLayers, static_cast<int>(recipe.decalRules.size()));

    overlaySettings.overlayLayers = {alloyLayer, spawnsArmiesLayer, unitsLayer,
                                      propsLayer, reclaimLayer, decalsLayer};
}

bool ResolveUnitsManualSubLayer(const Params::MapRecipe& recipe, int flatSubLayerIndex,
                                 int& outArmyIndex, int& outGroupIndex) {
    outArmyIndex = -1;
    outGroupIndex = -1;
    if (flatSubLayerIndex < 0) return false;
    int remaining = flatSubLayerIndex;
    for (std::size_t army = 0; army < recipe.armies.size(); ++army) {
        const int groupCount = static_cast<int>(recipe.armies[army].groups.size());
        if (remaining < groupCount) {
            outArmyIndex  = static_cast<int>(army);
            outGroupIndex = remaining;
            return true;
        }
        remaining -= groupCount;
    }
    return false;
}

} // namespace Ui
} // namespace SanmapGen
