// MapCanvas_ScenarioEditModeOwnership_UI_Test.cpp — acceptance test 4: a state-ownership test, not
// visual inspection. Drives REAL live imgui frames (mirroring MapCanvas_Render_UI_Test.cpp's own
// GL-backed technique — this canvas needs a real presentation identifier to reach past
// MapCanvas::Draw()'s "nothing composited" early return) with a synthetic press-drag-release
// gesture, and asserts on `MapCanvas::View()`'s own pixel-center state: while Scenario Edit Mode is
// active the gesture never pans the view (regardless of hit/miss — the ownership gate itself, not
// one interaction outcome); once deactivated the identical gesture shape pans normally again. One
// translation unit of the MapCanvas_UI_Test binary.
#include "MapCanvas_UI.h"
#include "PreviewComposite_TestScene_UI.h"
#include "../params/MapRecipe_PARAMS.h"
#include <imgui.h>

namespace SanmapGen {
namespace Ui {
namespace {

void check(bool bCondition, const char* label) { CheckPreviewExpectation(bCondition, label); }

constexpr int   kPreviewResolution = 32;
constexpr float kRegionSidePixels  = 256.0f;
constexpr unsigned long long kFontAtlasIdentifier = 0xF0000003ull;

void BeginHeadlessFrame() {
    ImGuiIO& io = ImGui::GetIO();
    io.DeltaTime = 1.0f / 60.0f;
    unsigned char* atlasPixels = nullptr; int atlasWidth = 0, atlasHeight = 0;
    io.Fonts->GetTexDataAsRGBA32(&atlasPixels, &atlasWidth, &atlasHeight);
    io.Fonts->SetTexID(static_cast<ImTextureID>(kFontAtlasIdentifier));
    ImGui::NewFrame();
}

ImVec2 DrawOneFrame(MapCanvas& canvas) {
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(600.0f, 600.0f));
    ImGui::Begin("ScenarioEditModeOwnershipTestWindow");
    const ImVec2 regionOrigin = ImGui::GetCursorScreenPos();
    canvas.Draw("mapCanvas", kRegionSidePixels);
    ImGui::End();
    ImGui::Render();
    return regionOrigin;
}

void SimulatePressDragRelease(MapCanvas& canvas, ImVec2 pressPosition) {
    ImGuiIO& io = ImGui::GetIO();
    // Reposition with the button still UP first: whatever the shared ImGui cursor's last position
    // was (this helper may run more than once against the same live context), that jump must never
    // land on a frame ApplyDrag reads a delta from — it only does once the item is actually active.
    io.AddMousePosEvent(pressPosition.x, pressPosition.y);
    io.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame();
    DrawOneFrame(canvas);

    io.AddMouseButtonEvent(0, true);
    BeginHeadlessFrame();
    DrawOneFrame(canvas);
    io.AddMousePosEvent(pressPosition.x + 60.0f, pressPosition.y + 60.0f);
    BeginHeadlessFrame();
    DrawOneFrame(canvas);
    io.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame();
    DrawOneFrame(canvas);
}

// ARCH §21.2 — the RIGHT button is the only pan gesture left button-index 1, not 0 (imgui's
// convention: 0 = left, 1 = right). Mirrors SimulatePressDragRelease's own shape exactly.
void SimulateRightPressDragRelease(MapCanvas& canvas, ImVec2 pressPosition) {
    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(pressPosition.x, pressPosition.y);
    io.AddMouseButtonEvent(1, false);
    BeginHeadlessFrame();
    DrawOneFrame(canvas);

    io.AddMouseButtonEvent(1, true);
    BeginHeadlessFrame();
    DrawOneFrame(canvas);
    io.AddMousePosEvent(pressPosition.x + 60.0f, pressPosition.y + 60.0f);
    BeginHeadlessFrame();
    DrawOneFrame(canvas);
    io.AddMouseButtonEvent(1, false);
    BeginHeadlessFrame();
    DrawOneFrame(canvas);
}

} // namespace

void RunMapCanvasScenarioEditModeOwnershipChecks(Sys::GpuResourceManager& manager) {
    PreviewTestScene scene;
    BuildPreviewTestScene(scene);
    PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.fields,
                               scene.instances, scene.entityIdentifiers);
    ConfigurePreviewSettings(composite.Settings());
    composite.Settings().previewResolution = kPreviewResolution;
    composite.SetGpuResourceManager(&manager);
    composite.Compose();

    Params::MapRecipe recipe;
    recipe.armies.push_back(Params::Army()); recipe.armies[0].name = "ARMY_01";
    Params::ScenarioBody body;
    ScenarioEditModeState scenarioEditMode;

    MapCanvas canvas;
    canvas.SetPreviewTexture(&manager, composite.CompositeTexture(), composite.Resolution());
    canvas.SetPreviewComposite(&composite);
    canvas.View().SetRegionSide(kRegionSidePixels);
    canvas.SetOverlayRecipe(&recipe);
    canvas.SetScenarioEditModeState(&scenarioEditMode);

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(600.0f, 600.0f);
    io.IniFilename = nullptr;

    // Frame 0 — priming, mouse away: establishes the region origin this window layout produces.
    io.AddMousePosEvent(-100.0f, -100.0f);
    io.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame();
    const ImVec2 regionOrigin = DrawOneFrame(canvas);
    // At the minimum zoom (the default) the whole image already fills the region, so
    // MapCanvasView::ClampViewCenter() clamps EVERY pan back to center by construction
    // (MapCanvasView_UI.h's own header comment) — zoom in first, exactly like
    // MapCanvas_View_UI_Test.cpp's own panning checks, so a pan has anything to prove at all.
    canvas.ApplyScroll(kRegionSidePixels * 0.5f, kRegionSidePixels * 0.5f, 8.0f);
    const float initialViewCenterX = canvas.View().ViewCenterPixelX();
    const float initialViewCenterY = canvas.View().ViewCenterPixelY();
    const ImVec2 pressPosition(regionOrigin.x + 40.0f, regionOrigin.y + 40.0f);

    // --- Mode ACTIVE: exclusive ownership — the gesture never reaches the pan path at all.
    // ARCH §21.2 — the left button never pans at all any more (left = click/marquee-select only);
    // the RIGHT button is the actual pan path now, and Scenario Edit Mode's exclusivity gate covers
    // it too ("no right-button pan while it owns the canvas", §21.2's own instruction) — proven with
    // the right-button gesture, the only one that could otherwise pan.
    scenarioEditMode.Activate(body, nullptr, nullptr, 1);
    SimulatePressDragRelease(canvas, pressPosition);
    check(canvas.View().ViewCenterPixelX() == initialViewCenterX
       && canvas.View().ViewCenterPixelY() == initialViewCenterY,
          "while Scenario Edit Mode is active, a LEFT-button gesture never pans the view (it never "
          "pans at all any more, active or not)");
    SimulateRightPressDragRelease(canvas, pressPosition);
    check(canvas.View().ViewCenterPixelX() == initialViewCenterX
       && canvas.View().ViewCenterPixelY() == initialViewCenterY,
          "while Scenario Edit Mode is active, a RIGHT-button drag is ALSO refused — no right-button "
          "pan while Scenario Edit Mode owns the canvas");

    // --- Mode OFF: the RIGHT-button gesture pans normally (ownership returned) — the left-button
    // gesture from before is no longer a meaningful proof of "ownership returned," since it never
    // pans regardless of mode.
    scenarioEditMode.Deactivate();
    SimulateRightPressDragRelease(canvas, pressPosition);
    check(canvas.View().ViewCenterPixelX() != initialViewCenterX
       || canvas.View().ViewCenterPixelY() != initialViewCenterY,
          "mode toggled off returns exclusive interaction ownership to the normal RIGHT-button pan path");

    ImGui::DestroyContext();
}

} // namespace Ui
} // namespace SanmapGen
