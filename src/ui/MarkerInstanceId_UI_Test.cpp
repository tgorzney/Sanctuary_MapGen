// MarkerInstanceId_UI_Test.cpp — acceptance test for `NextMarkerInstanceIdentifier` (ARCH §19.16).
// No imgui frame, no window, no GL: the decision is pure by construction (MarkerInstanceId_UI.h).
// Mirrors MarkerLayerId_UI_Test.cpp's exact coverage shape, one tier down (group, then transform).
#include "MarkerInstanceId_UI.h"
#include <cstdio>

using namespace SanmapGen;

static int failureCount = 0;

static void Check(bool bCondition, const char* label) {
    if (!bCondition) { std::printf("FAIL %s\n", label); ++failureCount; }
}

static Params::MarkerTransform MakeTransform(int instanceIdentifier) {
    Params::MarkerTransform transform;
    transform.instanceIdentifier = instanceIdentifier;
    return transform;
}

int main() {
    std::vector<Params::MarkerInstanceGroup> emptyMarkers;
    Check(Ui::NextMarkerInstanceIdentifier(emptyMarkers) == 0, "an empty markers vector yields 0");

    std::vector<Params::MarkerInstanceGroup> singleGroupMarkers(1);
    singleGroupMarkers[0].transforms.push_back(MakeTransform(3));
    singleGroupMarkers[0].transforms.push_back(MakeTransform(0));
    singleGroupMarkers[0].transforms.push_back(MakeTransform(7));
    Check(Ui::NextMarkerInstanceIdentifier(singleGroupMarkers) == 8,
          "identifiers {3,0,7} (unsorted) yield 8 (max + 1), proving the scan is not order-dependent");

    std::vector<Params::MarkerInstanceGroup> twoGroupMarkers(2);
    twoGroupMarkers[0].transforms.push_back(MakeTransform(2));
    twoGroupMarkers[0].transforms.push_back(MakeTransform(9));
    twoGroupMarkers[1].transforms.push_back(MakeTransform(5));
    Check(Ui::NextMarkerInstanceIdentifier(twoGroupMarkers) == 10,
          "the max is scanned GLOBALLY across groups (9 in group A beats 5 in group B), not per-group");

    // Pure function, no hidden state: calling it twice without applying the first result anywhere
    // does not change its answer — it must be called again by the caller after each push_back, not
    // cached; monotonicity comes from the caller re-scanning, not from memory inside the function.
    const int firstCall = Ui::NextMarkerInstanceIdentifier(twoGroupMarkers);
    const int secondCall = Ui::NextMarkerInstanceIdentifier(twoGroupMarkers);
    Check(firstCall == secondCall, "calling it twice in a row with no mutation returns the same answer");

    // NextMarkerSymmetryGroupIdentifier — same shape, sentinel 0 (not -1): an all-ungrouped/empty
    // roster mints 1, never 0 (0 would collide with "ungrouped").
    Check(Ui::NextMarkerSymmetryGroupIdentifier(emptyMarkers) == 1,
          "an empty markers vector yields 1 — the sentinel floor is 0, not -1");
    std::vector<Params::MarkerInstanceGroup> ungroupedMarkers(1);
    ungroupedMarkers[0].transforms.push_back(MakeTransform(1));
    ungroupedMarkers[0].transforms.push_back(MakeTransform(2));   // symmetryGroupIdentifier left at 0 on both
    Check(Ui::NextMarkerSymmetryGroupIdentifier(ungroupedMarkers) == 1,
          "an all-ungrouped roster (every symmetryGroupIdentifier == 0) still yields 1, not 0");
    std::vector<Params::MarkerInstanceGroup> groupedMarkers(2);
    groupedMarkers[0].transforms.push_back(MakeTransform(1));
    groupedMarkers[0].transforms[0].symmetryGroupIdentifier = 4;
    groupedMarkers[1].transforms.push_back(MakeTransform(2));
    groupedMarkers[1].transforms[0].symmetryGroupIdentifier = 9;
    Check(Ui::NextMarkerSymmetryGroupIdentifier(groupedMarkers) == 10,
          "the max is scanned globally across groups (9 in group B beats 4 in group A), yielding max + 1");

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
