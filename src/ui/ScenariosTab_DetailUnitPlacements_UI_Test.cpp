// ScenariosTab_DetailUnitPlacements_UI_Test.cpp — STEP261 acceptance: the pure, headless-testable
// half of the `body.unitPlacements` list editor (AppendDefaultScenarioUnitPlacement,
// ApplyScenarioUnitPlacementYawDegrees, ApplyScenarioUnitPlacementYawIfChanged). No imgui frame, no
// window, no GL context — same posture as ScenariosTab_UI_Test.cpp.
//
// The three functions under test have external linkage but are declared in NEITHER a header (this
// ticket's own "Files touched" list adds exactly one new declaration to ScenariosTab_UI.h,
// DrawScenarioUnitPlacementsSection) NOR here redundantly copied from one — they are forward-declared
// directly below, matching ScenariosTab_DetailUnitPlacements_UI.cpp's own signatures exactly, and
// this test binary links the whole SanGenV2 library (add_sangen_test, CMakeLists.txt) so the linker
// resolves them from that translation unit. Mirrors MarkersTab_Bundles_UI_Test.cpp's own posture for
// MarkersTab_BundleNodeBody_UI.cpp's equivalent Apply* functions.
#include "../params/Scenario_PARAMS.h"
#include <cstdio>
#include <vector>

namespace SanmapGen {
namespace Ui {
void AppendDefaultScenarioUnitPlacement(std::vector<Params::ScenarioUnitPlacement>& placements);
void ApplyScenarioUnitPlacementYawDegrees(Params::ScenarioUnitPlacement& placement, float yawDegrees);
void ApplyScenarioUnitPlacementYawIfChanged(Params::ScenarioUnitPlacement& placement, float yawDegrees,
                                            bool bSliderValueChanged);
} // namespace Ui
} // namespace SanmapGen

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

int failureCount = 0;

void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

// 1. Adding a row appends a default-constructed ScenarioUnitPlacement (identity rotation, zero
// position, empty army/template).
void RunAppendDefaultChecks() {
    std::vector<Params::ScenarioUnitPlacement> placements;
    AppendDefaultScenarioUnitPlacement(placements);
    Check(placements.size() == 1u, "appending adds exactly one row");
    const Params::ScenarioUnitPlacement& added = placements[0];
    Check(added.armyName.empty(), "the new row's armyName starts empty");
    Check(added.templateIdentifier.empty(), "the new row's templateIdentifier starts empty");
    Check(added.positionX == 0.0f && added.positionY == 0.0f && added.positionZ == 0.0f,
          "the new row's position starts at the origin");
    Check(added.rotationX == 0.0f && added.rotationY == 0.0f && added.rotationZ == 0.0f
              && added.rotationW == 1.0f,
          "the new row's rotation starts as the identity quaternion");
}

// 2. Editing the yaw slider updates rotationY/rotationW via Math::YawQuaternion and leaves
// rotationX/rotationZ at 0 (pure yaw, no drift into other axes).
void RunApplyYawDegreesChecks() {
    Params::ScenarioUnitPlacement placement;
    ApplyScenarioUnitPlacementYawDegrees(placement, 0.0f);
    Check(placement.rotationX == 0.0f && placement.rotationZ == 0.0f,
          "0 degrees leaves rotationX/rotationZ at 0");
    Check(placement.rotationY == 0.0f, "0 degrees writes rotationY == 0 (sin(0) == 0)");
    Check(placement.rotationW > 0.99f, "0 degrees writes rotationW ~= 1 (cos(0) == 1, identity)");

    ApplyScenarioUnitPlacementYawDegrees(placement, 180.0f);
    Check(placement.rotationX == 0.0f && placement.rotationZ == 0.0f,
          "180 degrees still leaves rotationX/rotationZ at 0");
    Check(placement.rotationY > 0.99f, "180 degrees writes rotationY ~= 1 (sin(90 deg) == 1)");
    const bool bNearZero = placement.rotationW > -0.01f && placement.rotationW < 0.01f;
    Check(bNearZero, "180 degrees writes rotationW ~= 0 (cos(90 deg) == 0)");

    ApplyScenarioUnitPlacementYawDegrees(placement, 90.0f);
    Check(placement.rotationX == 0.0f && placement.rotationZ == 0.0f,
          "90 degrees also leaves rotationX/rotationZ at 0");
}

// 3. Toggling "Advanced" and hand-editing a raw rotation field (a non-yaw-only value) is preserved
// verbatim across a frame with no slider interaction — the yaw slider must not silently
// re-derive/overwrite it.
void RunPreservesAdvancedEditWhenNotInteractingChecks() {
    Params::ScenarioUnitPlacement placement;
    placement.rotationX = 0.3f; placement.rotationY = 0.1f;
    placement.rotationZ = 0.2f; placement.rotationW = 0.91f;   // hand-edited, NOT yaw-only

    ApplyScenarioUnitPlacementYawIfChanged(placement, 45.0f, /*bSliderValueChanged=*/false);
    Check(placement.rotationX == 0.3f && placement.rotationY == 0.1f
              && placement.rotationZ == 0.2f && placement.rotationW == 0.91f,
          "a frame with no slider interaction leaves the Advanced-authored quaternion untouched");

    ApplyScenarioUnitPlacementYawIfChanged(placement, 45.0f, /*bSliderValueChanged=*/true);
    Check(placement.rotationX == 0.0f && placement.rotationZ == 0.0f,
          "an ACTUAL slider interaction does overwrite into the pure-yaw shape (X/Z zeroed)");
}

} // namespace

int main() {
    RunAppendDefaultChecks();
    RunApplyYawDegreesChecks();
    RunPreservesAdvancedEditWhenNotInteractingChecks();
    if (failureCount == 0) { std::printf("ALL PASS\n"); return 0; }
    std::printf("%d FAILURE(S)\n", failureCount);
    return 1;
}
