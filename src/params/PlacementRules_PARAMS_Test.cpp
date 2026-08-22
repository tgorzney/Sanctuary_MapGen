// PlacementRules_PARAMS_Test.cpp — acceptance test for M1-4 (marker/prop/decal/water).
//   g++ -O2 -std=c++17 PlacementRules_PARAMS_Test.cpp -o t && ./t
#include "MarkerRule_PARAMS.h"
#include "ScatterRule_PARAMS.h"
#include "Water_PARAMS.h"
#include <cstdio>

using namespace SanmapGen::Params;

int main() {
    int failures = 0;

    MarkerRule marker;
    if (marker.maxSlope != 89.9f || marker.count != 4 || marker.priority != MarkerPriority::LargestArea)
        { std::printf("FAIL marker defaults\n"); ++failures; }
    if (marker.focusGradient != FocusGradient::None)
        { std::printf("FAIL marker focus\n"); ++failures; }

    // STEP66: the symmetry triplet moved up one tier, off MarkerRule onto MarkerRuleLayer.
    MarkerRuleLayer markerLayer;
    if (!markerLayer.bEnabled || markerLayer.bHidden || !markerLayer.symmetry.bSymmetryUseGlobal
        || markerLayer.symmetry.symmetryMask != 0 || markerLayer.symmetry.radialSymmetryRepeatCount != 3)
        { std::printf("FAIL marker rule layer defaults\n"); ++failures; }

    PropRule prop;
    if (prop.density != 0.5f || prop.bAvoidWater || prop.bNearCliffs || prop.bReclaimable)
        { std::printf("FAIL prop defaults\n"); ++failures; }

    DecalRule decal;
    if (decal.maxSlope != 89.9f) { std::printf("FAIL decal defaults\n"); ++failures; }

    Water water;
    if (water.bEnabled || water.waterLevelMaximum != 0.0f) { std::printf("FAIL water defaults\n"); ++failures; }

    if (failures == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failures);
    return 1;
}
