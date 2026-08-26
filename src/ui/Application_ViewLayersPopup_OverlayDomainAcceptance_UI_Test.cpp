// Application_ViewLayersPopup_OverlayDomainAcceptance_UI_Test.cpp — STEP201 acceptance, item 1: the
// View popup's "Overlays (screen-space)" section proven against the REAL launch-time seed
// (`ConfigureDefaultOverlayLayers`, Application_OverlaySetup_UI.cpp), not
// Application_ViewLayersPopup_FlatRowLayout_UI_Test.cpp's synthetic 3-row scene. That STEP200 test
// proves the Flat row layout mechanism in the abstract; this one closes the gap STEP201's own fix
// approach item 1 flags — the overlay section specifically was "not individually
// acceptance-tested... only terrain rows were" — by reproducing DrawOverlaySection's own
// DraggableList<OverlayLayer_UI>::Render call shape (Application_ViewLayersPopup_UI.cpp; the
// production function is file-local, so this mirrors it exactly, the same posture the FlatRowLayout
// file already uses for DrawTerrainSection) over the real 6-domain seed.
// One translation unit of the Application_ViewLayersPopup_UI_Test binary; main() lives in
// Application_ViewLayersPopup_UI_Test.cpp.
#include "Application_Defaults_UI.h"
#include "Application_ViewLayersPopup_UI.h"
#include "ArmiesTab_UI.h"
#include "ListWidget_TestFrame_UI.h"
#include "../params/MapRecipe_PARAMS.h"
#include <cfloat>
#include <vector>

using namespace SanmapGen;
using namespace SanmapGen::Ui;

namespace {

const ImVec2 kMouseAway = ImVec2(-FLT_MAX, -FLT_MAX);

struct OverlaySectionScene {
    std::vector<OverlayLayer_UI> layers;
    DraggableListSignal          signal;
    int                          bodyCallCount = 0;
    std::vector<float>           rowLeftX;
    std::vector<float>           rowTopY;
};

// Reproduces DrawOverlaySection's own describeRow/drawRowBody/Flat call shape
// (Application_ViewLayersPopup_UI.cpp) — the row body itself is a no-op probe here; this file only
// needs to prove the ROW renders/toggles, not the opacity slider it hosts.
DraggableListSignal RunOverlaySectionFrame(OverlaySectionScene& scene, ImVec2 mousePosition,
                                           bool bLeftButtonDown) {
    HeadlessMouseState mouse;
    mouse.position = mousePosition;
    mouse.bLeftButtonDown = bLeftButtonDown;
    const std::size_t rowCount = scene.layers.size();
    scene.rowLeftX.assign(rowCount, 0.0f);
    scene.rowTopY.assign(rowCount, 0.0f);
    RunHeadlessFrame(mouse, ImVec2(420.0f, 400.0f), [&] {
        scene.bodyCallCount = 0;
        scene.signal = DraggableList<OverlayLayer_UI>::Render(
            "ViewListOverlay", scene.layers,
            [&](int rowIndex) {
                const ImVec2 rowCorner = ImGui::GetCursorScreenPos();
                scene.rowLeftX[static_cast<std::size_t>(rowIndex)] = rowCorner.x;
                scene.rowTopY[static_cast<std::size_t>(rowIndex)] = rowCorner.y;
                const OverlayLayer_UI& layer = scene.layers[static_cast<std::size_t>(rowIndex)];
                DraggableListRow row;
                row.label    = layer.name.empty() ? "Overlay" : layer.name.c_str();
                row.bVisible = layer.bEnabled;
                return row;
            },
            [&](int) { ++scene.bodyCallCount; },
            -1, DraggableListRowLayout::Flat);
    });
    return scene.signal;
}

// Hover, press, release — same discipline the FlatRowLayout file's own ClickAtFlat uses (an
// AllowOverlap affordance is only interactable once it was hovered the PREVIOUS frame).
DraggableListSignal ClickAtOverlaySection(OverlaySectionScene& scene, ImVec2 position) {
    RunOverlaySectionFrame(scene, position, false);
    const DraggableListSignal pressSignal = RunOverlaySectionFrame(scene, position, true);
    const DraggableListSignal releaseSignal = RunOverlaySectionFrame(scene, position, false);
    return pressSignal.bHasSignal() ? pressSignal : releaseSignal;
}

// The exact launch-time seed Application::Application() hands DrawOverlaySection every session
// (Application_UI.cpp -> ConfigureDefaultOverlayLayers), over a REAL two-army roster —
// ARCH_14_16_PerArmyUnitsOverlayRows.md §14.16-A: Units seeds one row per `recipe.armies[i]`, named
// via ArmyRowLabel, so a 0-army recipe would silently hide that behavior from this acceptance file.
Params::MapRecipe MakeTwoArmyRecipe() {
    Params::MapRecipe recipe;
    Params::Army armyWithDisplayName; armyWithDisplayName.displayName = "Northern Fleet";
    Params::Army armyWithoutDisplayName; armyWithoutDisplayName.name = "ARMY_02";
    recipe.armies = {armyWithDisplayName, armyWithoutDisplayName};
    return recipe;
}

OverlaySectionScene MakeRealOverlayScene(const Params::MapRecipe& recipe) {
    OverlayLayerSettings settings;
    ConfigureDefaultOverlayLayers(settings, recipe);
    OverlaySectionScene scene;
    scene.layers = settings.overlayLayers;
    return scene;
}

int failureCount = 0;
void Check(bool bCondition, const char* label) {
    if (bCondition) return;
    std::printf("FAIL %s\n", label);
    ++failureCount;
}

// STEP201 fix approach item 1 / STEP200 fix approach points 2-3, re-proven over the real seed: every
// real domain row draws its body unconditionally (Flat, never collapsed), and a click at a row's own
// left margin (a Collapsible row's disclosure-arrow hit-box) never changes how many bodies draw.
// ARCH_14_16_PerArmyUnitsOverlayRows.md §14.16-A: over a real two-army roster, "Units" becomes two
// rows named via ArmyRowLabel (one displayName, one name-fallback) instead of one shared "Units" row.
void TestAllSixRealDomainRowsRenderFlatWithSeedNames() {
    HeadlessImguiSession session;
    const Params::MapRecipe recipe = MakeTwoArmyRecipe();
    OverlaySectionScene scene = MakeRealOverlayScene(recipe);
    Check(scene.layers.size() == 7u,
          "2 armies -> 2 Units rows: 7 overlay-domain rows (Alloy, Spawns, 2xUnits, Props, Reclaim, Decals)");
    const char* const expectedNames[] = {"Alloy", "Spawns", "Northern Fleet", "ARMY_02", "Props", "Reclaim", "Decals"};
    for (std::size_t index = 0; index < scene.layers.size() && index < 7u; ++index)
        Check(scene.layers[index].name == expectedNames[index],
              "row order/name matches ConfigureDefaultOverlayLayers's own seed order, Units rows named via ArmyRowLabel");

    RunOverlaySectionFrame(scene, kMouseAway, false);
    RunOverlaySectionFrame(scene, kMouseAway, false);   // settle
    Check(scene.bodyCallCount == static_cast<int>(scene.layers.size()),
          "every real domain row's inline body drew on the settle frame (Flat, never collapsed)");

    const ImVec2 arrowLikePoint(scene.rowLeftX[0] + 4.0f, scene.rowTopY[0] + 8.0f);
    RunOverlaySectionFrame(scene, arrowLikePoint, true);
    Check(scene.bodyCallCount == static_cast<int>(scene.layers.size()),
          "a click at the would-be arrow position still draws every real domain row's body");
    RunOverlaySectionFrame(scene, arrowLikePoint, false);
    Check(scene.bodyCallCount == static_cast<int>(scene.layers.size()),
          "releasing at the same point still draws every real domain row's body");
}

// STEP201 fix approach item 1: each row's own visibility affordance flips ONLY that row's
// OverlayLayer_UI::bEnabled, through the exact ApplyViewLayerSignal path
// Application::DrawViewLayersPopup() uses in production — leaving every other domain's bEnabled,
// and both PreviewFieldLayer's terrain section and the underlying recipe/placement data, untouched
// (this file never constructs either of those, by design: nothing here CAN move them).
void TestEachDomainRowTogglesOnlyItsOwnVisibility() {
    HeadlessImguiSession session;
    const Params::MapRecipe recipe = MakeTwoArmyRecipe();
    OverlaySectionScene scene = MakeRealOverlayScene(recipe);
    RunOverlaySectionFrame(scene, kMouseAway, false);
    RunOverlaySectionFrame(scene, kMouseAway, false);

    for (std::size_t targetRow = 0; targetRow < scene.layers.size(); ++targetRow) {
        std::vector<bool> before;
        for (const OverlayLayer_UI& layer : scene.layers) before.push_back(layer.bEnabled);

        float visibilityX = -1.0f;
        for (float probeX = scene.rowLeftX[targetRow]; probeX < scene.rowLeftX[targetRow] + 60.0f;
             probeX += 2.0f) {
            const DraggableListSignal probe =
                ClickAtOverlaySection(scene, ImVec2(probeX, scene.rowTopY[targetRow] + 8.0f));
            if (probe.kind == DraggableListSignalKind::ToggleVisibility
                && probe.sourceRowIndex == static_cast<int>(targetRow)) {
                visibilityX = probeX;
                ApplyViewLayerSignal(scene.layers, probe);   // the production DrawViewLayersPopup call
                break;
            }
        }
        Check(visibilityX > 0.0f, "row's own visibility affordance exists and reports its own row index");
        for (std::size_t index = 0; index < scene.layers.size(); ++index) {
            const bool expectedEnabled = (index == targetRow) ? !before[index] : before[index];
            Check(scene.layers[index].bEnabled == expectedEnabled,
                  "toggling one domain row flips only that row's bEnabled, never another row's");
        }
        RunOverlaySectionFrame(scene, kMouseAway, false);   // re-settle geometry for the next row
        RunOverlaySectionFrame(scene, kMouseAway, false);
    }
}

} // namespace

namespace SanmapGen {
namespace Ui {
int RunViewLayersOverlayDomainAcceptance() {
    TestAllSixRealDomainRowsRenderFlatWithSeedNames();
    TestEachDomainRowTogglesOnlyItsOwnVisibility();
    return failureCount;
}
} // namespace Ui
} // namespace SanmapGen
