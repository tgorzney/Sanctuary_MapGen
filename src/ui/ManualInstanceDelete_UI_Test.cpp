// ManualInstanceDelete_UI_Test.cpp — headless acceptance coverage for
// `DeleteManualInstancesById<GroupT>` and its three per-domain wrappers (STEP234,
// DESIGN_MarkerLink_R1.md §1.2). Pure/imgui-free, no window/GL — mirrors
// ManualInstanceHitTest_UI_Test.cpp's own shape for the sibling query primitive.
#include "ManualInstanceDelete_UI.h"
#include "../params/MarkerInstance_PARAMS.h"
#include "../params/MarkerLink_PARAMS.h"
#include "../params/PropInstance_PARAMS.h"
#include "../params/ScatterInstanceLayer_PARAMS.h"
#include <cstdio>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

int failureCount = 0;
void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

Params::MarkerTransform MakeMarkerTransform(int instanceIdentifier, int layerIndex = 0) {
    Params::MarkerTransform t;
    t.instanceIdentifier = instanceIdentifier;
    t.layerIndex = layerIndex;
    return t;
}

const std::function<bool(const Params::MarkerTransform&)> kAlwaysUnlocked =
    [](const Params::MarkerTransform&) { return false; };

// ---- The generic template: erases only the targeted identifiers, no-ops on a missing identifier,
// and returns the count actually erased.

void RunDeleteManualInstancesByIdChecks() {
    std::vector<Params::MarkerInstanceGroup> markers(2);
    markers[0].transforms.push_back(MakeMarkerTransform(1));
    markers[0].transforms.push_back(MakeMarkerTransform(2));
    markers[1].transforms.push_back(MakeMarkerTransform(3));

    const int erased = DeleteManualInstancesById(markers, std::vector<int>{2, 3, 999}, kAlwaysUnlocked);
    Check(erased == 2, "erases exactly the two targeted identifiers present; the missing one is a silent no-op");
    Check(markers[0].transforms.size() == 1 && markers[0].transforms[0].instanceIdentifier == 1,
          "the untargeted instance in group 0 survives untouched");
    Check(markers[1].transforms.empty(), "group 1's only instance, targeted, is erased");

    Check(DeleteManualInstancesById(markers, {}, kAlwaysUnlocked) == 0, "an empty identifier list erases nothing");
    Check(DeleteManualInstancesById(markers, std::vector<int>{42}, kAlwaysUnlocked) == 0,
          "an identifier naming nothing at all is a silent no-op, not an error");
}

// ---- A locked owning layer refuses the erase even though the identifier is targeted — a partial
// delete due to a locked member is a soft degrade, not a refusal of the whole batch.

void RunLockGateChecks() {
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].transforms.push_back(MakeMarkerTransform(10, /*layerIndex=*/0));   // unlocked
    markers[0].transforms.push_back(MakeMarkerTransform(11, /*layerIndex=*/5));   // locked

    const std::function<bool(const Params::MarkerTransform&)> lockLayerFive =
        [](const Params::MarkerTransform& transform) { return transform.layerIndex == 5; };
    const int erased = DeleteManualInstancesById(markers, std::vector<int>{10, 11}, lockLayerFive);
    Check(erased == 1, "only the unlocked targeted instance is erased; the locked one is skipped, count reflects it");
    Check(markers[0].transforms.size() == 1 && markers[0].transforms[0].instanceIdentifier == 11,
          "the locked instance survives, untouched");
}

// ---- The three per-domain wrappers each bind their own domain's lock predicate correctly.

void RunPerDomainWrapperChecks() {
    std::vector<Params::MarkerInstanceLayer> markerLayers(1);
    markerLayers[0].bLocked = true;
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].transforms.push_back(MakeMarkerTransform(1, /*layerIndex=*/0));   // locked layer
    markers[0].transforms.push_back(MakeMarkerTransform(2, /*layerIndex=*/1));   // out-of-range = unlocked
    const std::vector<Params::MarkerLink> noMarkerLinks;
    Check(DeleteSelectedManualMarkerInstances(markers, {1, 2}, markerLayers, noMarkerLinks) == 1,
          "DeleteSelectedManualMarkerInstances binds IsMarkerInstanceLocked correctly");
    Check(markers[0].transforms.size() == 1 && markers[0].transforms[0].instanceIdentifier == 1,
          "the marker on the locked layer survives");

    std::vector<Params::PropInstanceLayer> propLayers(1);
    propLayers[0].bLocked = true;
    std::vector<Params::PropInstanceGroup> props(1);
    Params::PropTransform lockedProp; lockedProp.instanceIdentifier = 5; lockedProp.layerIndex = 0;
    Params::PropTransform freeProp;   freeProp.instanceIdentifier   = 6; freeProp.layerIndex   = 1;
    props[0].transforms.push_back(lockedProp);
    props[0].transforms.push_back(freeProp);
    Check(DeleteSelectedManualPropInstances(props, {5, 6}, propLayers) == 1,
          "DeleteSelectedManualPropInstances binds IsPropInstanceLayerLocked correctly");
    Check(props[0].transforms.size() == 1 && props[0].transforms[0].instanceIdentifier == 5,
          "the prop on the locked layer survives");

    std::vector<Params::DecalInstanceLayer> decalLayers(1);
    decalLayers[0].bLocked = true;
    std::vector<Params::DecalInstanceGroup> decals(1);
    Params::DecalTransform lockedDecal; lockedDecal.instanceIdentifier = 7; lockedDecal.layerIndex = 0;
    Params::DecalTransform freeDecal;   freeDecal.instanceIdentifier   = 8; freeDecal.layerIndex   = 1;
    decals[0].transforms.push_back(lockedDecal);
    decals[0].transforms.push_back(freeDecal);
    Check(DeleteSelectedManualDecalInstances(decals, {7, 8}, decalLayers) == 1,
          "DeleteSelectedManualDecalInstances binds IsDecalInstanceLayerLocked correctly");
    Check(decals[0].transforms.size() == 1 && decals[0].transforms[0].instanceIdentifier == 7,
          "the decal on the locked layer survives");
}

// STEP249, ARCH §21.9 — `DeleteSelectedManualMarkerInstances`'s new `markerLinks` parameter: a
// Link-locked instance survives the erase even though its own owning Layer is unlocked; an unlinked
// sibling targeted in the same call is erased normally.
void RunInstanceTierLinkLockChecks() {
    std::vector<Params::MarkerInstanceLayer> unlockedLayers(1);   // the owning Layer is NOT locked
    std::vector<Params::MarkerLink> links(1);
    links[0].identifier = 500; links[0].bLocked = true;           // but the Link IS

    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].transforms.push_back(MakeMarkerTransform(20, /*layerIndex=*/0));
    markers[0].transforms[0].linkIdentifier = 500;                // Link-tagged, Link-locked
    markers[0].transforms.push_back(MakeMarkerTransform(21, /*layerIndex=*/0));   // unlinked sibling

    const int erased = DeleteSelectedManualMarkerInstances(markers, {20, 21}, unlockedLayers, links);
    Check(erased == 1, "only the unlinked sibling is erased; the Link-locked instance is skipped");
    Check(markers[0].transforms.size() == 1 && markers[0].transforms[0].instanceIdentifier == 20,
          "the Link-locked instance survives, untouched, even though its own Layer is unlocked");
}

} // namespace

int main() {
    RunDeleteManualInstancesByIdChecks();
    RunLockGateChecks();
    RunPerDomainWrapperChecks();
    RunInstanceTierLinkLockChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
