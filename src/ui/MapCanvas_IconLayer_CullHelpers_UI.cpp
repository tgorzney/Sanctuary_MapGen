// MapCanvas_IconLayer_CullHelpers_UI.cpp — the small, reusable pieces §1's procedural/manual
// sub-layer walkers both need: domain -> DATA-collection resolution (§14.6), the tpId-buffer ->
// string conversion, and per-layer AABB arithmetic. EmitCandidateIfVisible itself (the actual LOD
// + projection + pairing resolution) is the sibling MapCanvas_IconLayer_CullEmit_UI.cpp — split
// out to stay inside Constitution §1.5's file-size ceiling.
// Layer: UI. Pure, imgui-free, headless-testable.
#include "MapCanvas_IconLayer_CullInternal_UI.h"

namespace SanmapGen {
namespace Ui {

std::string TemplateIdentifierToString8(const char* characters) {
    std::string result;
    for (int index = 0; index < 7 && characters[index] != '\0'; ++index) result.push_back(characters[index]);
    return result;
}

bool TryResolveDomainCollection(OverlayDomainKind_UI domainKind, PlacementCollectionKind_UI& outCollection) {
    switch (domainKind) {
        case OverlayDomainKind_UI::Alloy:
        case OverlayDomainKind_UI::SpawnsArmies: outCollection = PlacementCollectionKind_UI::Markers; return true;
        // Props/Reclaim both resolve to the Props collection (§14.6: domain != DATA-bucket
        // identity) — STEP83's SeedPropReclaimDomains already routed each recipe.propRules[i]
        // into exactly one of the two layers' ProceduralRule refs, so whichever layer a ref
        // shows up in, STEP50's props CSR bucket (keyed by that same global rule index) is the
        // right one to read; no second index, no per-instance re-check here.
        case OverlayDomainKind_UI::Props:
        case OverlayDomainKind_UI::Reclaim:      outCollection = PlacementCollectionKind_UI::Props;   return true;
        case OverlayDomainKind_UI::Units:        outCollection = PlacementCollectionKind_UI::Units;   return true;
        case OverlayDomainKind_UI::Decals:       outCollection = PlacementCollectionKind_UI::Decals;  return true;
    }
    return false;
}

const Data::PlacementInstances& CollectionInstances(const Data::PlacementResults& placements,
                                                     PlacementCollectionKind_UI collection) {
    switch (collection) {
        case PlacementCollectionKind_UI::Props: return placements.props;
        case PlacementCollectionKind_UI::Units: return placements.units;
        case PlacementCollectionKind_UI::Decals: return placements.decals;
        case PlacementCollectionKind_UI::Markers: default: return placements.markers;
    }
}

const Data::RuleBucketIndex& CollectionRuleBucket(const Data::RuleBucketIndexSet& ruleBucketIndex,
                                                   PlacementCollectionKind_UI collection) {
    switch (collection) {
        case PlacementCollectionKind_UI::Props: return ruleBucketIndex.props;
        case PlacementCollectionKind_UI::Units: return ruleBucketIndex.units;
        case PlacementCollectionKind_UI::Decals: return ruleBucketIndex.decals;
        case PlacementCollectionKind_UI::Markers: default: return ruleBucketIndex.markers;
    }
}

bool WorldRectsIntersect(const LayerWorldAabb_UI& aabb, const ViewWorldRect_UI& viewRect) {
    if (!aabb.bValid) return false;
    return aabb.lowWorldX <= viewRect.highWorldX && aabb.highWorldX >= viewRect.lowWorldX
        && aabb.lowWorldZ <= viewRect.highWorldZ && aabb.highWorldZ >= viewRect.lowWorldZ;
}

void ResolveMarkerCategoryTintColor(Params::MarkerCategory category,
                                    const Params::GlobalMarkerSettings& settings,
                                    float& outRed, float& outGreen, float& outBlue) {
    switch (category) {
        case Params::MarkerCategory::Spawn:
            outRed = settings.colorSpawn[0]; outGreen = settings.colorSpawn[1]; outBlue = settings.colorSpawn[2];
            return;
        case Params::MarkerCategory::Alloys:
            outRed = settings.colorAlloy[0]; outGreen = settings.colorAlloy[1]; outBlue = settings.colorAlloy[2];
            return;
        // Generic/Expansion: no reserved color today. Plasma has no MarkerCategory value at all
        // (see this ticket's ⚠️ section) — deliberately falls here too, not a bug.
        default:
            outRed = outGreen = outBlue = 1.0f;
            return;
    }
}

// STEP122: mirrors ResolveMarkerCategoryTintColor's exact switch shape/posture (same file).
// Params::MarkerCategory (MarkerRule_PARAMS.h:18: Generic, Spawn, Alloys, Expansion) has NO
// Plasma value — the same pre-existing gap ResolveMarkerCategoryTintColor already documents
// ("Plasma has no MarkerCategory value at all") — Plasma-named procedural markers fall into the
// default branch below, same as Generic/Expansion. Not this ticket's gap to close.
float ResolveMarkerCategoryScale(Params::MarkerCategory category, const Params::GlobalMarkerSettings& settings) {
    switch (category) {
        case Params::MarkerCategory::Spawn:  return settings.scaleSpawn;
        case Params::MarkerCategory::Alloys: return settings.scaleAlloy;
        default:                             return 1.0f;
    }
}

void WidenAabb(LayerWorldAabb_UI& aabb, float worldX, float worldZ) {
    if (!aabb.bValid) {
        aabb.lowWorldX = aabb.highWorldX = worldX;
        aabb.lowWorldZ = aabb.highWorldZ = worldZ;
        aabb.bValid = true;
        return;
    }
    if (worldX < aabb.lowWorldX) aabb.lowWorldX = worldX;
    if (worldX > aabb.highWorldX) aabb.highWorldX = worldX;
    if (worldZ < aabb.lowWorldZ) aabb.lowWorldZ = worldZ;
    if (worldZ > aabb.highWorldZ) aabb.highWorldZ = worldZ;
}

} // namespace Ui
} // namespace SanmapGen
