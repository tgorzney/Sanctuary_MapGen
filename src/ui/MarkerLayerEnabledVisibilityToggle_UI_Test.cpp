// MarkerLayerEnabledVisibilityToggle_UI_Test.cpp — STEP144 acceptance for the E/D + V/I coupled
// toggle rules. Pure logic only.
#include "MarkerLayerEnabledVisibilityToggle_UI.h"
#include <cstdio>

using namespace SanmapGen::Ui;

namespace {

int failureCount = 0;
void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

void RunEnabledToggleChecks() {
    bool bEnabled = true, bHidden = false;   // start Enabled/Visible
    ApplyMarkerRuleLayerEnabledToggle(bEnabled, bHidden);
    Check(!bEnabled && bHidden, "Enabled -> Disabled forces Hidden too");

    ApplyMarkerRuleLayerEnabledToggle(bEnabled, bHidden);
    Check(bEnabled && !bHidden, "Disabled -> Enabled forces Visible too");
}

void RunEnabledToggleFromEnabledHiddenChecks() {
    bool bEnabled = true, bHidden = true;    // Enabled/Hidden (a real, reachable state)
    ApplyMarkerRuleLayerEnabledToggle(bEnabled, bHidden);
    Check(!bEnabled && bHidden, "Enabled/Hidden -> Disabled stays Hidden (already was)");
}

void RunVisibilityToggleFromEnabledChecks() {
    bool bEnabled = true, bHidden = false;   // Enabled/Visible
    ApplyMarkerRuleLayerVisibilityToggle(bEnabled, bHidden);
    Check(bEnabled && bHidden, "from Enabled/Visible, V/I only flips Hidden -- Enabled untouched");

    ApplyMarkerRuleLayerVisibilityToggle(bEnabled, bHidden);
    Check(bEnabled && !bHidden, "and toggling back stays Enabled the whole time");
}

void RunVisibilityToggleFromDisabledAutoEnablesChecks() {
    bool bEnabled = false, bHidden = true;   // Disabled/Hidden -- the only Disabled state
    ApplyMarkerRuleLayerVisibilityToggle(bEnabled, bHidden);
    Check(bEnabled && !bHidden,
         "from Disabled/Hidden, V/I toward Visible auto-enables -- {Disabled,Visible} is never real");
}

} // namespace

int main() {
    RunEnabledToggleChecks();
    RunEnabledToggleFromEnabledHiddenChecks();
    RunVisibilityToggleFromEnabledChecks();
    RunVisibilityToggleFromDisabledAutoEnablesChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
