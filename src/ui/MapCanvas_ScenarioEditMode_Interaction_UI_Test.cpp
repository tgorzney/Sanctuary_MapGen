// MapCanvas_ScenarioEditMode_Interaction_UI_Test.cpp — acceptance test 2 (left-drag materialization
// seeds from the real baseline position, then continues live) plus the right-click request
// resolution both context-menu kinds depend on. No imgui frame needed — ApplyScenarioEditModePointerInput
// is pure; STEP47's real projection API drives the world<->screen math (mirrors
// MapCanvas_Picking_UI_Test.cpp's own fixture construction, known-correct pixel mapping).
#include "MapCanvas_ScenarioEditMode_Ops_UI.h"
#include "MapCanvasView_UI.h"
#include "PreviewComposite_TestScene_UI.h"
#include "../params/Army_PARAMS.h"

namespace SanmapGen {
namespace Ui {
namespace {

void Check(bool bCondition, const char* label) { CheckPreviewExpectation(bCondition, label); }

constexpr int   kPreviewResolution  = 64;
constexpr float kRegionSidePixels   = 256.0f;
constexpr float kRegionCenterPixel  = 128.0f;   // world (2,2) on the 4-cell test map -> here

struct InteractionFixture {
    PreviewTestScene scene;
    PreviewComposite* composite;
    MapCanvasView view;

    InteractionFixture() {
        BuildPreviewTestScene(scene);
        composite = new PreviewComposite(scene.geometry, scene.water, scene.strata, scene.fields,
                                         scene.instances, scene.entityIdentifiers);
        ConfigurePreviewSettings(composite->Settings());
        composite->Settings().previewResolution = kPreviewResolution;
        composite->ComposeOnCpu();
        view.SetPreviewResolution(kPreviewResolution);
        view.SetRegionSide(kRegionSidePixels);
    }
    ~InteractionFixture() { delete composite; }
    InteractionFixture(const InteractionFixture&) = delete;
    InteractionFixture& operator=(const InteractionFixture&) = delete;
};

ScenarioEditModePointerFrame_UI MakeFrame(float regionLocalX, float regionLocalY, bool bActivated,
                                          bool bActive, bool bRightClicked = false) {
    ScenarioEditModePointerFrame_UI frame;
    frame.regionLocalX = regionLocalX; frame.regionLocalY = regionLocalY;
    frame.bPressActivated = bActivated; frame.bPressActive = bActive; frame.bRightClicked = bRightClicked;
    return frame;
}

void CheckMaterializeOnFirstDragThenLiveContinue() {
    InteractionFixture fixture;
    Params::ScenarioBody body;
    std::vector<Params::Army> armies(1); armies[0].name = "ARMY_01";

    ScenarioEditModeState state;
    state.Activate(body, nullptr, nullptr, 1);
    ScenarioEditMarkerCandidate_UI candidate;
    candidate.kind = ScenarioMarkerKind_UI::Spawn;
    candidate.state = ScenarioMarkerVisualState_UI::SpawnNoOverride;
    candidate.armyIndex = 0;
    candidate.worldX = 2.0f; candidate.worldY = 0.0f; candidate.worldZ = 2.0f;   // the REAL baseline
    state.lastResolvedCandidates.push_back(candidate);

    // Frame 1: press down exactly on the hollow candidate's screen position.
    ApplyScenarioEditModePointerInput(state, fixture.view, *fixture.composite, armies,
                                      MakeFrame(kRegionCenterPixel, kRegionCenterPixel, true, true));
    Check(state.bDragging, "the press begins a drag");
    Check(body.spawns.size() == 1u, "exactly one ScenarioSpawn is materialized");
    Check(body.spawns[0].armyName == "ARMY_01", "materialized for the candidate's own army");
    Check(body.spawns[0].positionX == 2.0f && body.spawns[0].positionZ == 2.0f,
          "seeded from the REAL baseline position (2,2), never zeroed");

    // Frame 2: still held, moved elsewhere — the row follows the cursor live.
    ApplyScenarioEditModePointerInput(state, fixture.view, *fixture.composite, armies,
                                      MakeFrame(kRegionCenterPixel + 32.0f, kRegionCenterPixel, false, true));
    Check(body.spawns.size() == 1u, "the drag updates the SAME row, never appends a second one");
    Check(body.spawns[0].positionX != 2.0f, "the live drag actually moved the row off its seeded position");

    // Frame 3: released.
    ApplyScenarioEditModePointerInput(state, fixture.view, *fixture.composite, armies,
                                      MakeFrame(kRegionCenterPixel + 32.0f, kRegionCenterPixel, false, false));
    Check(!state.bDragging, "releasing the press ends the drag");
}

void CheckRightClickBaselineAlloyRequestsRemoval() {
    InteractionFixture fixture;
    Params::ScenarioBody body;
    std::vector<Params::Army> armies;
    ScenarioEditModeState state;
    state.Activate(body, nullptr, nullptr, 0);
    ScenarioEditMarkerCandidate_UI candidate;
    candidate.kind = ScenarioMarkerKind_UI::Alloy;
    candidate.state = ScenarioMarkerVisualState_UI::AlloyKept;
    candidate.markerName = "alloy_r0_0";
    candidate.worldX = 2.0f; candidate.worldZ = 2.0f;
    state.lastResolvedCandidates.push_back(candidate);

    ApplyScenarioEditModePointerInput(state, fixture.view, *fixture.composite, armies,
                                      MakeFrame(kRegionCenterPixel, kRegionCenterPixel, false, false, true));
    Check(state.bContextMenuJustRequested, "a right-click hit on a baseline alloy requests a menu this frame");
    Check(state.pendingContextMenu.kind == ScenarioEditModeState::ContextMenuRequest::Kind::RemoveBaselineAlloy,
          "the request kind is RemoveBaselineAlloy");
    Check(state.pendingContextMenu.markerName == "alloy_r0_0", "the request carries the hit candidate's markerName");
}

void CheckRightClickEmptyCanvasRequestsAddForNearestArmy() {
    InteractionFixture fixture;
    Params::ScenarioBody body;
    std::vector<Params::Army> armies(1); armies[0].name = "ARMY_01";
    ScenarioEditModeState state;
    state.Activate(body, nullptr, nullptr, 1);
    ScenarioEditMarkerCandidate_UI spawnCandidate;
    spawnCandidate.kind = ScenarioMarkerKind_UI::Spawn;
    spawnCandidate.armyIndex = 0;
    spawnCandidate.worldX = 2.0f; spawnCandidate.worldZ = 2.0f;
    state.lastResolvedCandidates.push_back(spawnCandidate);

    // Right-click far from the one candidate (no hit) — resolves against the nearest army anchor.
    ApplyScenarioEditModePointerInput(state, fixture.view, *fixture.composite, armies,
                                      MakeFrame(kRegionCenterPixel - 96.0f, kRegionCenterPixel - 96.0f,
                                               false, false, true));
    Check(state.bContextMenuJustRequested, "empty-canvas right-click still requests a menu");
    Check(state.pendingContextMenu.kind == ScenarioEditModeState::ContextMenuRequest::Kind::AddAlloyForArmy,
          "the request kind is AddAlloyForArmy");
    Check(state.pendingContextMenu.armyName == "ARMY_01", "attributed to the only (nearest) army");
}

} // namespace

void RunScenarioEditModeInteractionChecks() {
    CheckMaterializeOnFirstDragThenLiveContinue();
    CheckRightClickBaselineAlloyRequestsRemoval();
    CheckRightClickEmptyCanvasRequestsAddForNearestArmy();
}

} // namespace Ui
} // namespace SanmapGen
