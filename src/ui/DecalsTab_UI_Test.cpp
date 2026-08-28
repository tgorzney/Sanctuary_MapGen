// DecalsTab_UI_Test.cpp — the Decals tab's own rule<->widget mirror checks, split out of
// PropsTab_UI_Test.cpp when ARCH §20 gave Decals its own top-level tab. Pure logic only, so the
// binary needs no imgui frame, no window and no GL context.
#include "DecalsTab_UI.h"
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

// A decal rule is a prop rule without the water/cliff affinities and without symmetry, so it gets
// its own mirrors rather than borrowing the prop ones (Params::DecalRule is a different struct).
void RunDecalRuleMirrorChecks() {
    Params::DecalRule rule;
    rule.minSlope = 1.0f; rule.maxSlope = 15.0f; rule.minHeight = 0.05f; rule.maxHeight = 0.55f;
    DecalRuleStackState state;
    LoadDecalRuleValues(rule, state);
    Check(state.slopeValues.maximumValue == 15.0f && state.heightValues.maximumValue == 0.55f,
          "a decal rule's bands reach their own mirrors");
    Check(!StoreDecalRuleValues(state, rule), "storing back what was loaded reports no move");
    state.heightValues.maximumValue = 0.6f;
    Check(StoreDecalRuleValues(state, rule) && rule.maxHeight == 0.6f,
          "and a real edit reports the move and lands on the rule");
    Check(state.selectedRuleIndex == 0, "the decal stack opens on its first row");
}

} // namespace

int main() {
    RunDecalRuleMirrorChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
