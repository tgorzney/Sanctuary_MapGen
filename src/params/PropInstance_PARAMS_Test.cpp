// PropInstance_PARAMS_Test.cpp — acceptance for NextPropInstanceIdentifier/
// NextDecalInstanceIdentifier (ARCH §21.4), mirroring NextMarkerInstanceIdentifier's own test shape
// (MarkersTab_UI_Test.cpp) at this file's PARAMS-resident home.
#include "PropInstance_PARAMS.h"
#include <cstdio>

using namespace SanmapGen;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

void RunNextPropInstanceIdentifierChecks() {
    std::vector<Params::PropInstanceGroup> props;
    Check(Params::NextPropInstanceIdentifier(props) == 0, "an empty roster mints 0");

    Params::PropInstanceGroup groupOne;
    Params::PropTransform t0; t0.instanceIdentifier = 0;
    Params::PropTransform t1; t1.instanceIdentifier = 2;
    groupOne.transforms.push_back(t0);
    groupOne.transforms.push_back(t1);
    props.push_back(groupOne);

    Params::PropInstanceGroup groupTwo;
    Params::PropTransform t2; t2.instanceIdentifier = 1;
    groupTwo.transforms.push_back(t2);
    props.push_back(groupTwo);

    Check(Params::NextPropInstanceIdentifier(props) == 3,
          "max-plus-one across every group's transforms (roster-wide, not per-group): {0,2,1} mints 3");
}

void RunNextDecalInstanceIdentifierChecks() {
    std::vector<Params::DecalInstanceGroup> decals;
    Check(Params::NextDecalInstanceIdentifier(decals) == 0, "an empty roster mints 0");

    Params::DecalInstanceGroup group;
    Params::DecalTransform t0; t0.instanceIdentifier = 5;
    group.transforms.push_back(t0);
    decals.push_back(group);

    Check(Params::NextDecalInstanceIdentifier(decals) == 6, "id {5} mints 6 - max-plus-one");
}

void RunFieldDefaultChecks() {
    Params::PropTransform propTransform;
    Check(propTransform.instanceIdentifier == -1, "PropTransform::instanceIdentifier defaults to -1 (unassigned)");
    Check(propTransform.symmetryGroupIdentifier == 0, "PropTransform::symmetryGroupIdentifier defaults to 0 (ungrouped)");
    Params::DecalTransform decalTransform;
    Check(decalTransform.instanceIdentifier == -1, "DecalTransform::instanceIdentifier defaults to -1 (unassigned)");
    Check(decalTransform.symmetryGroupIdentifier == 0, "DecalTransform::symmetryGroupIdentifier defaults to 0 (ungrouped)");
}

} // namespace

int main() {
    RunNextPropInstanceIdentifierChecks();
    RunNextDecalInstanceIdentifierChecks();
    RunFieldDefaultChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
