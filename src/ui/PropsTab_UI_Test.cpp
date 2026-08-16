// PropsTab_UI_Test.cpp — tab-rebuild WO C4 acceptance, part 4: the Props tab and the two other
// stacks it hosts (the manual prop groups and the decal rules). Pure logic only — the rule<->widget
// mirrors, the group-color override, the label fallbacks and the tpId probe — so the binary needs
// no imgui frame, no window and no GL context.
// NOT YET REGISTERED IN CMake — WO C4 does not own CMakeLists.txt (gate CD-int registers it).
#include "PropsTab_UI.h"
#include <cstdio>
#include <cstring>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

void RunPropRuleMirrorChecks() {
    Params::PropRule rule;
    rule.minSlope = 3.0f; rule.maxSlope = 25.0f; rule.minHeight = 0.2f; rule.maxHeight = 0.8f;
    PropsTabState state;
    LoadPropRuleValues(rule, state);
    Check(state.slopeValues.minimumValue == 3.0f && state.slopeValues.maximumValue == 25.0f
          && state.heightValues.minimumValue == 0.2f && state.heightValues.maximumValue == 0.8f,
          "both gate bands reach their widget mirrors");
    Check(!StorePropRuleValues(state, rule), "storing back what was loaded reports no move");
    state.slopeValues.minimumValue = 4.0f;
    Check(StorePropRuleValues(state, rule) && rule.minSlope == 4.0f,
          "and a real edit reports the move and lands on the rule");
    Check(state.densityRange.minimumValue == 0.0f && state.densityRange.maximumValue == 1.0f,
          "density is a 0-1 proportion");
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

// The tpId is a fixed 8-byte field whose last byte need not be a terminator, so "has a template"
// is a probe on the FIRST byte, never a strlen.
void RunTemplateIdentifierChecks() {
    Params::PropRule rule;
    Check(!PropRuleHasTemplateIdentifier(rule), "a fresh rule has no template typed yet");
    std::memcpy(rule.transform.templateIdentifier, "ual0001", 7u);
    Check(PropRuleHasTemplateIdentifier(rule), "and a typed tpId is seen");
    Check(rule.transform.templateIdentifier[7] == '\0',
          "the eighth byte stays clear, so the %.7s row label cannot run off the field");
}

// The manual groups are presentation state (SCOPE NOTE 1), so the invariants are the override and
// the fences around them.
void RunManualPropGroupChecks() {
    ManualPropLayersState state;
    Check(SelectedManualPropGroup(state) == nullptr, "an empty block selects no group");
    state.groups.push_back(ManualPropGroup());
    state.selectedGroupIndex = 0;
    Check(SelectedManualPropGroup(state) == &state.groups[0], "the selected group is reachable");
    state.selectedGroupIndex = 2;
    Check(SelectedManualPropGroup(state) == nullptr, "an index past the last group selects nothing");

    ManualPropGroup& group = state.groups[0];
    group.previewColor[0] = 0.25f;
    state.groupColor[0]   = 0.75f;
    Check(EffectiveManualPropGroupColor(state, group) == group.previewColor,
          "with the shared tint off a group draws its OWN color");
    state.bUseGroupColor = true;
    Check(EffectiveManualPropGroupColor(state, group) == state.groupColor,
          "and with it on every group draws the one shared color");

    Check(ManualPropGroupRowLabel(group) != nullptr && ManualPropGroupRowLabel(group)[0] != '\0',
          "an unnamed group still draws a label");
    Check(state.iconScaleRange.minimumValue == 0.1f && state.iconScaleRange.maximumValue == 10.0f,
          "the icon scale sliders carry the plan's 0.1-10");
}

} // namespace

int main() {
    RunPropRuleMirrorChecks();
    RunDecalRuleMirrorChecks();
    RunTemplateIdentifierChecks();
    RunManualPropGroupChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
