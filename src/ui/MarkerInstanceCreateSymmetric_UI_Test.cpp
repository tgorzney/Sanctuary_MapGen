// MarkerInstanceCreateSymmetric_UI_Test.cpp — CreateSymmetricManualMarkerInstances acceptance.
// Human's own bug report: "When creating an Instance, symmetry needs to be checked and duplicates
// created for proper symmetry." Pure logic, no imgui frame needed (mirrors MarkerDragGesture_UI_Test.cpp's
// own posture).
#include "MarkerInstanceCreateSymmetric_UI.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../params/Symmetry_PARAMS.h"
#include <cmath>
#include <cstdio>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

int failures = 0;

void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failures; }
}

bool NearlyEqual(float a, float b, float tolerance = 1e-3f) { return std::fabs(a - b) <= tolerance; }

// mapSize=10, worldUnitsPerCell=1 -> world units == cell units, map center at world (5,5) — the
// same fixture MarkerDragGesture_UI_Test.cpp's own MakeTestGeometry uses, so MirrorAcrossX's own
// already-proven behavior there (x mirrors about 5.0, z untouched) applies here unchanged.
Params::Geometry MakeTestGeometry() {
    Params::Geometry geometry;
    geometry.mapSize = 10;
    geometry.worldUnitsPerCell = 1.0f;
    return geometry;
}

// A brand-new instance placed OFF the mirror axis materializes itself AND its mirrored sibling,
// sharing one freshly-minted, non-zero symmetryGroupIdentifier.
void RunOffAxisCreatesSymmetricPairChecks() {
    Params::MarkerInstanceGroup group;
    const std::vector<Params::MarkerInstanceGroup> markers;   // empty roster: no id collisions to avoid
    std::vector<Params::MarkerInstanceLayer> layers(1);       // default: bSymmetryEnabled=true, uses global

    const int sourceIdentifier = CreateSymmetricManualMarkerInstances(
        group, markers, layers, MakeTestGeometry(), Params::SymmetryAxis::MirrorAcrossX, 3,
        /*layerIndex=*/0, /*worldX=*/2.0f, /*worldY=*/1.5f, /*worldZ=*/3.0f);

    Check(group.transforms.size() == 2u, "an off-axis create materializes the source AND its mirrored sibling");
    Check(sourceIdentifier >= 0 && sourceIdentifier == group.transforms[0].instanceIdentifier,
         "the returned identifier is the SOURCE point's own instanceIdentifier");
    Check(NearlyEqual(group.transforms[0].transform.positionX, 2.0f)
       && NearlyEqual(group.transforms[0].transform.positionZ, 3.0f),
         "the source instance lands exactly where requested");
    Check(NearlyEqual(group.transforms[1].transform.positionX, 8.0f)
       && NearlyEqual(group.transforms[1].transform.positionZ, 3.0f),
         "the mirrored sibling lands at the MirrorAcrossX-reflected point (about world x=5)");
    Check(group.transforms[0].transform.positionY == 1.5f && group.transforms[1].transform.positionY == 1.5f,
         "both instances share the requested Y");
    Check(group.transforms[0].layerIndex == 0 && group.transforms[1].layerIndex == 0,
         "both instances land on the requested layer");
    Check(group.transforms[0].symmetryGroupIdentifier != 0
       && group.transforms[0].symmetryGroupIdentifier == group.transforms[1].symmetryGroupIdentifier,
         "both instances share one freshly-minted, non-zero symmetryGroupIdentifier");
    Check(group.transforms[0].instanceIdentifier != group.transforms[1].instanceIdentifier,
         "each materialized instance mints its own distinct instanceIdentifier");
}

// A brand-new instance placed EXACTLY on the mirror axis has a 1-point orbit — exactly one instance,
// left ungrouped (symmetryGroupIdentifier 0), byte-identical to the pre-fix single push_back.
void RunOnAxisCreatesSingleUngroupedInstanceChecks() {
    Params::MarkerInstanceGroup group;
    const std::vector<Params::MarkerInstanceGroup> markers;
    std::vector<Params::MarkerInstanceLayer> layers(1);

    CreateSymmetricManualMarkerInstances(group, markers, layers, MakeTestGeometry(),
                                         Params::SymmetryAxis::MirrorAcrossX, 3,
                                         /*layerIndex=*/0, /*worldX=*/5.0f, /*worldY=*/0.0f, /*worldZ=*/3.0f);

    Check(group.transforms.size() == 1u, "a create exactly ON the mirror axis materializes only the one point");
    Check(group.transforms[0].symmetryGroupIdentifier == 0,
         "the lone instance stays ungrouped (sentinel 0), not tagged into a 1-member symmetry group");
}

// A Layer with SYM off forces the effective mask to None regardless of the global mask — the
// resolution ResolveEffectiveMarkerSymmetry already guarantees, exercised end to end here.
void RunSymmetryDisabledLayerCreatesSingleInstanceChecks() {
    Params::MarkerInstanceGroup group;
    const std::vector<Params::MarkerInstanceGroup> markers;
    std::vector<Params::MarkerInstanceLayer> layers(1);
    layers[0].bSymmetryEnabled = false;

    CreateSymmetricManualMarkerInstances(group, markers, layers, MakeTestGeometry(),
                                         Params::SymmetryAxis::MirrorAcrossX, 3,
                                         /*layerIndex=*/0, /*worldX=*/2.0f, /*worldY=*/0.0f, /*worldZ=*/3.0f);

    Check(group.transforms.size() == 1u,
         "SYM off on the target layer creates exactly one instance, even off-axis under a mirroring global mask");
    Check(group.transforms[0].symmetryGroupIdentifier == 0, "and that lone instance stays ungrouped");
}

// A roster that already has symmetry groups mints the NEXT id (max + 1), never reusing one already
// in use elsewhere in the roster — the same collision-avoidance NextMarkerInstanceIdentifier already
// guarantees for instanceIdentifier, one field over.
void RunMintsNextSymmetryGroupIdentifierChecks() {
    std::vector<Params::MarkerInstanceGroup> existingMarkers(1);
    existingMarkers[0].transforms.resize(2);
    existingMarkers[0].transforms[0].instanceIdentifier     = 10;
    existingMarkers[0].transforms[0].symmetryGroupIdentifier = 5;
    existingMarkers[0].transforms[1].instanceIdentifier     = 11;
    existingMarkers[0].transforms[1].symmetryGroupIdentifier = 5;

    Params::MarkerInstanceGroup group;
    std::vector<Params::MarkerInstanceLayer> layers(1);

    CreateSymmetricManualMarkerInstances(group, existingMarkers, layers, MakeTestGeometry(),
                                         Params::SymmetryAxis::MirrorAcrossX, 3,
                                         /*layerIndex=*/0, /*worldX=*/2.0f, /*worldY=*/0.0f, /*worldZ=*/3.0f);

    Check(group.transforms[0].symmetryGroupIdentifier == 6,
         "a fresh pair mints max(existing symmetryGroupIdentifier) + 1, not a colliding/reused id");
    Check(group.transforms[0].instanceIdentifier == 12 && group.transforms[1].instanceIdentifier == 13,
         "instanceIdentifier likewise continues from the roster-wide maximum (11), never restarting at 0");
}

// Human's own follow-up bug report — "symmetry duplicates are not created". Root cause: the map's
// own dead CENTER is a FIXED POINT under RotateHalfTurn — the recipe's own DEFAULT global mask
// (Params::MapRecipe::globalSymmetryMask, MapRecipe_PARAMS.h) — so "+ Instance"'s own default spawn
// position (MapCenterWorldUnits, MarkersTab_UI.cpp) always collapsed the orbit to 1 point regardless
// of the target layer's symmetry settings. This documents BOTH the exact collapse (on-center, the
// pre-fix bug) and the fix (MarkersTab_UI.cpp's own small diagonal nudge off center actually breaks
// the fixed point, under the REAL default mask — not just MirrorAcrossX, this file's other checks).
void RunRotateHalfTurnCenterFixedPointChecks() {
    const Params::Geometry geometry = MakeTestGeometry();   // mapCenter == world (5, 5)
    std::vector<Params::MarkerInstanceLayer> layers(1);     // default: bSymmetryEnabled=true, uses global

    {
        Params::MarkerInstanceGroup onCenterGroup;
        const std::vector<Params::MarkerInstanceGroup> markers;
        CreateSymmetricManualMarkerInstances(onCenterGroup, markers, layers, geometry,
                                             Params::SymmetryAxis::RotateHalfTurn, 0,
                                             /*layerIndex=*/0, /*worldX=*/5.0f, /*worldY=*/0.0f, /*worldZ=*/5.0f);
        Check(onCenterGroup.transforms.size() == 1u,
             "the pre-fix bug, reproduced directly: a create exactly on the map's dead center is a "
             "RotateHalfTurn fixed point — the orbit collapses to 1 point even though symmetry is on");
    }
    {
        // Mirrors MarkersTab_UI.cpp's own fix exactly: mapCenter + kNewInstanceCenterOffsetWorldUnits (4.0f)
        // on both axes.
        Params::MarkerInstanceGroup offCenterGroup;
        const std::vector<Params::MarkerInstanceGroup> markers;
        CreateSymmetricManualMarkerInstances(offCenterGroup, markers, layers, geometry,
                                             Params::SymmetryAxis::RotateHalfTurn, 0,
                                             /*layerIndex=*/0, /*worldX=*/9.0f, /*worldY=*/0.0f, /*worldZ=*/9.0f);
        Check(offCenterGroup.transforms.size() == 2u,
             "the fix: nudged 4 world units off center on both axes, the SAME default global mask "
             "now produces a real symmetric pair");
        Check(NearlyEqual(offCenterGroup.transforms[1].transform.positionX, 1.0f)
           && NearlyEqual(offCenterGroup.transforms[1].transform.positionZ, 1.0f),
             "the rotated sibling lands at the expected 180-degree point about center (5,5)");
        Check(offCenterGroup.transforms[0].symmetryGroupIdentifier != 0
           && offCenterGroup.transforms[0].symmetryGroupIdentifier
              == offCenterGroup.transforms[1].symmetryGroupIdentifier,
             "the pair shares one freshly-minted, non-zero symmetryGroupIdentifier — properly grouped");
    }
}

} // namespace

int main() {
    RunOffAxisCreatesSymmetricPairChecks();
    RunOnAxisCreatesSingleUngroupedInstanceChecks();
    RunSymmetryDisabledLayerCreatesSingleInstanceChecks();
    RunMintsNextSymmetryGroupIdentifierChecks();
    RunRotateHalfTurnCenterFixedPointChecks();
    if (failures == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failures);
    return 1;
}
