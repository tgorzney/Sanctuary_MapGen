// Application_OverlaySetup_UI.cpp — launch-time default seeding for the six-domain overlay stack
// (`OverlayLayer_Settings_UI.h`, ARCH_14_02_DataModel.md §14.2). Layer: UI. One-shot: runs once, in
// `Application`'s constructor, mirroring `ConfigureDefaultPreview`'s own posture for `fieldLayers`
// (`Application_Defaults_UI.h`). No live resync as the recipe grows mid-session — out of scope here
// (STEP51).
#include "Application_Defaults_UI.h"
#include "ArmiesTab_UI.h"

namespace SanmapGen {
namespace Ui {

// Defined in the sibling seed-helper translation unit (Application_OverlaySetup_Seed_UI.cpp,
// ARCH_01_05_FileSizeCeilings.md §1.5 split, STEP83 Item 1). Not part of this module's public
// surface — only ConfigureDefaultOverlayLayers below calls them.
void PushProceduralRefs(std::vector<OverlaySubLayerRef_UI>& subLayers, int ruleCount);
void PushManualRefs(std::vector<OverlaySubLayerRef_UI>& subLayers, int recordCount);
void SeedMarkerDomains(OverlayLayer_UI& alloyLayer, OverlayLayer_UI& spawnsArmiesLayer,
                       const Params::MapRecipe& recipe);
// ARCH_14_16_PerArmyUnitsOverlayRows.md §14.16-A/B: `unitsLayers` is one row per `recipe.armies[i]`,
// army-index-aligned by construction (see the loop below) — no name-matching needed to find "the
// row whose army matches".
void SeedUnitsManualSubLayers(std::vector<OverlayLayer_UI>& unitsLayers, const Params::MapRecipe& recipe);
void SeedUnitsProceduralSubLayers(std::vector<OverlayLayer_UI>& unitsLayers, const Params::MapRecipe& recipe);
void SeedPropReclaimDomains(OverlayLayer_UI& propsLayer, OverlayLayer_UI& reclaimLayer,
                            const Params::MapRecipe& recipe);

// ARCH_14_16_PerArmyUnitsOverlayRows.md §14.16-A: N `OverlayLayer_UI{domainKind=Units}` rows, one
// per `recipe.armies[i]` in roster order, named via ArmyRowLabel — not one shared row. Every other
// domain's seeding is unchanged.
void ConfigureDefaultOverlayLayers(OverlayLayerSettings& overlaySettings,
                                    const Params::MapRecipe& recipe) {
    OverlayLayer_UI alloyLayer;        alloyLayer.name        = "Alloy";
    alloyLayer.domainKind                                     = OverlayDomainKind_UI::Alloy;
    OverlayLayer_UI spawnsArmiesLayer; spawnsArmiesLayer.name = "Spawns";
    spawnsArmiesLayer.domainKind                               = OverlayDomainKind_UI::SpawnsArmies;
    SeedMarkerDomains(alloyLayer, spawnsArmiesLayer, recipe);

    std::vector<OverlayLayer_UI> unitsLayers;
    unitsLayers.reserve(recipe.armies.size());
    for (const Params::Army& army : recipe.armies) {
        OverlayLayer_UI unitsLayer;
        unitsLayer.name       = ArmyRowLabel(army);
        unitsLayer.domainKind = OverlayDomainKind_UI::Units;
        unitsLayers.push_back(unitsLayer);
    }
    SeedUnitsManualSubLayers(unitsLayers, recipe);
    SeedUnitsProceduralSubLayers(unitsLayers, recipe);

    OverlayLayer_UI propsLayer;   propsLayer.name   = "Props";
    propsLayer.domainKind   = OverlayDomainKind_UI::Props;
    OverlayLayer_UI reclaimLayer; reclaimLayer.name = "Reclaim";
    reclaimLayer.domainKind = OverlayDomainKind_UI::Reclaim;
    SeedPropReclaimDomains(propsLayer, reclaimLayer, recipe);

    OverlayLayer_UI decalsLayer; decalsLayer.name = "Decals";
    decalsLayer.domainKind = OverlayDomainKind_UI::Decals;
    PushManualRefs(decalsLayer.subLayers, static_cast<int>(recipe.decalLayers.size()));
    PushProceduralRefs(decalsLayer.subLayers, static_cast<int>(recipe.decalRules.size()));

    overlaySettings.overlayLayers.clear();
    overlaySettings.overlayLayers.push_back(alloyLayer);
    overlaySettings.overlayLayers.push_back(spawnsArmiesLayer);
    for (const OverlayLayer_UI& unitsLayer : unitsLayers) overlaySettings.overlayLayers.push_back(unitsLayer);
    overlaySettings.overlayLayers.push_back(propsLayer);
    overlaySettings.overlayLayers.push_back(reclaimLayer);
    overlaySettings.overlayLayers.push_back(decalsLayer);
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
