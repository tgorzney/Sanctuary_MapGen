// MarkerLayerBundle_PARAMS_Test.cpp — acceptance test for STEP119's pure resolvers:
// WouldReparentMarkerLayerBundleCreateCycle, CollectMarkerLayerBundleRecursiveLayerIndices,
// CollectMarkerLayerBundleRecursiveManualMembers. Mirrors GlobalMarkerSettings_PARAMS_Test.cpp's
// exact Check/failureCount/main() style. Headers compile standalone: no JSON, no CMake link beyond
// the default (add_sangen_test).
#include "MarkerLayerBundle_PARAMS.h"
#include "MarkerLayerBundleQuery_PARAMS.h"
#include <cstdio>

using namespace SanmapGen::Params;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

// root(1,-1) -> child(2,1) -> grandchild(3,2), plus an unrelated sibling root(4,-1). Shared by the
// two Collect* tests below.
std::vector<MarkerLayerBundle> MakeFourBundleTree() {
    std::vector<MarkerLayerBundle> bundles;
    MarkerLayerBundle root;       root.identifier = 1; root.parentBundleIdentifier = -1;
    MarkerLayerBundle child;      child.identifier = 2; child.parentBundleIdentifier = 1;
    MarkerLayerBundle grandchild; grandchild.identifier = 3; grandchild.parentBundleIdentifier = 2;
    MarkerLayerBundle sibling;    sibling.identifier = 4; sibling.parentBundleIdentifier = -1;
    bundles.push_back(root); bundles.push_back(child);
    bundles.push_back(grandchild); bundles.push_back(sibling);
    return bundles;
}

void CheckWouldReparentMarkerLayerBundleCreateCycle() {
    Check(WouldReparentMarkerLayerBundleCreateCycle(5, 5, {}),
          "self-parent (candidateId == newParentId) always returns true");

    std::vector<MarkerLayerBundle> simpleChain;
    MarkerLayerBundle first;  first.identifier = 1;  first.parentBundleIdentifier = -1;
    MarkerLayerBundle second; second.identifier = 2; second.parentBundleIdentifier = 1;
    simpleChain.push_back(first); simpleChain.push_back(second);
    Check(!WouldReparentMarkerLayerBundleCreateCycle(2, 1, simpleChain),
          "a simple valid reparent (candidate not anywhere on the new parent's chain) returns false");

    const std::vector<MarkerLayerBundle> threeLevelChain = MakeFourBundleTree();
    Check(WouldReparentMarkerLayerBundleCreateCycle(1, 3, threeLevelChain),
          "candidateId sitting two levels up from newParentId (1 -> 2 -> 3) returns true");

    std::vector<MarkerLayerBundle> mutualCycle;
    MarkerLayerBundle a; a.identifier = 1; a.parentBundleIdentifier = 2;
    MarkerLayerBundle b; b.identifier = 2; b.parentBundleIdentifier = 3;
    MarkerLayerBundle c; c.identifier = 3; c.parentBundleIdentifier = 1;
    mutualCycle.push_back(a); mutualCycle.push_back(b); mutualCycle.push_back(c);
    const bool bUnrelatedResult = WouldReparentMarkerLayerBundleCreateCycle(99, 1, mutualCycle);
    Check(!bUnrelatedResult,
          "an already-corrupt mutual-cycle table queried for an unrelated candidate/parent pair "
          "returns (bounded-step-count design) without hanging");
}

void CheckCollectMarkerLayerBundleRecursiveLayerIndices() {
    const std::vector<MarkerLayerBundle> bundles = MakeFourBundleTree();

    // 5 layers whose parentBundleIdentifier values are 1, 2, 3, 4, -1 respectively.
    std::vector<MarkerRuleLayer> ruleLayers(5);
    std::vector<MarkerInstanceLayer> instanceLayers(5);
    const int parentValues[5] = { 1, 2, 3, 4, -1 };
    for (int index = 0; index < 5; ++index) {
        ruleLayers[index].parentBundleIdentifier = parentValues[index];
        instanceLayers[index].parentBundleIdentifier = parentValues[index];
    }

    std::vector<int> outRuleLayerIndices;
    std::vector<int> outInstanceLayerIndices;
    CollectMarkerLayerBundleRecursiveLayerIndices(1, bundles, ruleLayers, instanceLayers,
                                                  outRuleLayerIndices, outInstanceLayerIndices);

    Check(outRuleLayerIndices.size() == 3 && outRuleLayerIndices[0] == 0
          && outRuleLayerIndices[1] == 1 && outRuleLayerIndices[2] == 2,
          "rule-layer indices parented at 1/2/3 (root/child/grandchild) are included, in order");
    Check(outInstanceLayerIndices.size() == 3 && outInstanceLayerIndices[0] == 0
          && outInstanceLayerIndices[1] == 1 && outInstanceLayerIndices[2] == 2,
          "instance-layer indices parented at 1/2/3 (root/child/grandchild) are included, in order");
    for (int index : outRuleLayerIndices)
        Check(index != 3 && index != 4, "the sibling(4)- and ungrouped(-1)-parented rule layers are excluded");
    for (int index : outInstanceLayerIndices)
        Check(index != 3 && index != 4,
              "the sibling(4)- and ungrouped(-1)-parented instance layers are excluded");
}

void CheckCollectMarkerLayerBundleRecursiveManualMembers() {
    const std::vector<MarkerLayerBundle> bundles = MakeFourBundleTree();

    // Same 5-entry parent spread as the LayerIndices test, but only MarkerInstanceLayer exists here
    // — by this function's own signature (no ruleLayers parameter), a Procedural-only layer
    // structurally cannot appear in the result; there is no runtime filter to bypass.
    std::vector<MarkerInstanceLayer> instanceLayers(5);
    const int parentValues[5] = { 1, 2, 3, 4, -1 };
    for (int index = 0; index < 5; ++index)
        instanceLayers[index].parentBundleIdentifier = parentValues[index];

    MarkerInstanceGroup group;
    group.name = "Alloys";
    for (int layerIndex = 0; layerIndex < 5; ++layerIndex) {
        MarkerTransform transform;
        transform.layerIndex = layerIndex;
        group.transforms.push_back(transform);
    }
    std::vector<MarkerInstanceGroup> markers;
    markers.push_back(group);

    const std::vector<std::pair<int,int>> members =
        CollectMarkerLayerBundleRecursiveManualMembers(1, bundles, instanceLayers, markers);

    Check(members.size() == 3, "exactly the 3 in-scope transforms (layers 1/2/3) are collected");
    if (members.size() == 3) {
        Check(members[0] == std::make_pair(0, 0) && members[1] == std::make_pair(0, 1)
              && members[2] == std::make_pair(0, 2),
              "the returned {groupIndex, transformIndex} pairs are exactly the in-scope transforms");
    }
}

} // namespace

int main() {
    CheckWouldReparentMarkerLayerBundleCreateCycle();
    CheckCollectMarkerLayerBundleRecursiveLayerIndices();
    CheckCollectMarkerLayerBundleRecursiveManualMembers();

    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
