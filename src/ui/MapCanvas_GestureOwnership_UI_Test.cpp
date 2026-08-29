// MapCanvas_GestureOwnership_UI_Test.cpp — ARCH §21.2/§21.5 end-to-end acceptance: the real
// ApplyPointerInput state machine, driven through live imgui frames (mirrors
// MapCanvas_ActivePanelGate_UI_Test.cpp's own GL-backed technique) — right-button pans, left-button
// never pans (drag-a-manual-instance or marquee-select instead), Ctrl-click toggles a real
// selection, and a locked manual instance is excluded from a marquee box that geometrically covers
// it. The lower-level primitives (OverlayInstanceKeySet_UI's own mutators, the generic hit-test/
// collect templates) already have their own exhaustive headless coverage elsewhere — this file only
// proves the WIRING through the live imgui pointer state machine is correct.
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
constexpr unsigned long long kFontAtlasIdentifier = 0xF0000005ull;

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
    ImGui::Begin("GestureOwnershipTestWindow");
    const ImVec2 regionOrigin = ImGui::GetCursorScreenPos();
    canvas.Draw("mapCanvas", kRegionSidePixels);
    ImGui::End();
    ImGui::Render();
    return regionOrigin;
}

// `io.KeyCtrl`/`io.KeyShift` are read-only, recomputed by NewFrame() from the key-event queue every
// frame (imgui.h's own comment) — setting them directly gets silently overwritten the next
// NewFrame() call. The real modifier state must go through AddKeyEvent instead, exactly as a real
// keyboard backend would report it.
void SetModifierKeys(bool bCtrl, bool bShift) {
    // io.KeyCtrl/io.KeyShift are derived from the DEDICATED ImGuiMod_Ctrl/ImGuiMod_Shift key-data
    // slots (GetMergedModsFromKeys(), imgui.cpp), NOT from ImGuiKey_LeftCtrl/LeftShift directly — a
    // real backend (imgui_impl_glfw.cpp's own ImGui_ImplGlfw_UpdateKeyModifiers) reports the merged
    // mod state through ImGuiMod_Ctrl/ImGuiMod_Shift explicitly, exactly mirrored here.
    ImGuiIO& io = ImGui::GetIO();
    io.AddKeyEvent(ImGuiMod_Ctrl, bCtrl);
    io.AddKeyEvent(ImGuiMod_Shift, bShift);
}

// button: 0 = left, 1 = right (imgui convention). bCtrl/bShift only matter for the left button.
void SimulatePressDragRelease(MapCanvas& canvas, ImVec2 pressPosition, ImVec2 releasePosition,
                              int button, bool bCtrl = false, bool bShift = false) {
    ImGuiIO& io = ImGui::GetIO();
    SetModifierKeys(bCtrl, bShift);
    io.AddMousePosEvent(pressPosition.x, pressPosition.y);
    io.AddMouseButtonEvent(button, false);
    BeginHeadlessFrame();
    DrawOneFrame(canvas);

    io.AddMouseButtonEvent(button, true);
    BeginHeadlessFrame();
    DrawOneFrame(canvas);
    io.AddMousePosEvent(releasePosition.x, releasePosition.y);
    BeginHeadlessFrame();
    DrawOneFrame(canvas);
    io.AddMouseButtonEvent(button, false);
    BeginHeadlessFrame();
    DrawOneFrame(canvas);
    SetModifierKeys(false, false);
}

// A stationary click: press and release at the SAME position (zero travel).
void SimulateStationaryClick(MapCanvas& canvas, ImVec2 position, bool bCtrl = false, bool bShift = false) {
    ImGuiIO& io = ImGui::GetIO();
    SetModifierKeys(bCtrl, bShift);
    io.AddMousePosEvent(position.x, position.y);
    io.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame();
    DrawOneFrame(canvas);
    io.AddMouseButtonEvent(0, true);
    BeginHeadlessFrame();
    DrawOneFrame(canvas);
    BeginHeadlessFrame();
    DrawOneFrame(canvas);
    io.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame();
    DrawOneFrame(canvas);
    SetModifierKeys(false, false);
}

ImVec2 ScreenPositionFor(const ImVec2& regionOrigin, MapCanvas& canvas, const PreviewComposite& composite,
                         float worldX, float worldZ) {
    const PreviewComposite::PreviewPixelPoint previewPixel = composite.WorldToPreviewPixel(worldX, worldZ);
    const RegionLocalPoint regionLocal =
        canvas.View().ProjectPreviewPixelToRegionLocal(previewPixel.pixelX, previewPixel.pixelY);
    return ImVec2(regionOrigin.x + regionLocal.regionLocalX, regionOrigin.y + regionLocal.regionLocalY);
}

} // namespace

void RunMapCanvasGestureOwnershipChecks(Sys::GpuResourceManager& manager) {
    PreviewTestScene scene;
    BuildPreviewTestScene(scene);
    PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.fields,
                               scene.instances, scene.entityIdentifiers);
    ConfigurePreviewSettings(composite.Settings());
    composite.Settings().previewResolution = kPreviewResolution;
    composite.SetGpuResourceManager(&manager);
    composite.Compose();

    // Two unlocked manual markers close together, plus a THIRD, locked one nearby — all inside the
    // marquee box the test below draws, so the lock-gate has something real to exclude.
    std::vector<Params::MarkerInstanceGroup> markers(1);
    Params::MarkerTransform markerA; markerA.transform.positionX = 1.0f; markerA.transform.positionZ = 1.0f;
    markerA.instanceIdentifier = 1; markerA.layerIndex = 0;
    Params::MarkerTransform markerB; markerB.transform.positionX = 2.0f; markerB.transform.positionZ = 1.0f;
    markerB.instanceIdentifier = 2; markerB.layerIndex = 0;
    Params::MarkerTransform markerLocked; markerLocked.transform.positionX = 1.5f; markerLocked.transform.positionZ = 2.0f;
    markerLocked.instanceIdentifier = 3; markerLocked.layerIndex = 1;
    markers[0].transforms.push_back(markerA);
    markers[0].transforms.push_back(markerB);
    markers[0].transforms.push_back(markerLocked);
    std::vector<Params::MarkerInstanceLayer> markerLayers(2);
    markerLayers[1].bLocked = true;   // layerIndex 1 — markerLocked's own layer
    Params::MapRecipe recipe;
    ApplicationPanel activePanel = ApplicationPanel::Markers;

    MapCanvas canvas;
    canvas.SetPreviewTexture(&manager, composite.CompositeTexture(), composite.Resolution());
    canvas.SetPreviewComposite(&composite);
    canvas.View().SetRegionSide(kRegionSidePixels);
    canvas.SetOverlayRecipe(&recipe);
    canvas.SetActivePanelSource(&activePanel);
    canvas.SetManualMarkerDragSource(&markers, &markerLayers, &scene.geometry, &recipe);

    OverlayInstanceKeySet_UI lastReportedSet;
    canvas.SetSelectionChangedCallback([&](const OverlayInstanceKey_UI&, const OverlayInstanceKeySet_UI& selectedKeys) {
        lastReportedSet = selectedKeys;
    });

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(600.0f, 600.0f);
    io.IniFilename = nullptr;
    // Scripted input, not a live human: deliver every queued event (mouse position/button, key
    // modifiers) within the SAME frame instead of imgui's default trickle-one-event-per-frame
    // pacing, which is tuned for a real backend's natural per-frame event cadence.
    io.ConfigInputTrickleEventQueue = false;

    // Frame 0 — priming, mouse away: establishes the region origin this window layout produces.
    io.AddMousePosEvent(-100.0f, -100.0f);
    io.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame();
    const ImVec2 regionOrigin = DrawOneFrame(canvas);
    canvas.ApplyScroll(kRegionSidePixels * 0.5f, kRegionSidePixels * 0.5f, 2.0f);   // zoom in: give a pan something to prove
    const float initialViewCenterX = canvas.View().ViewCenterPixelX();
    const float initialViewCenterY = canvas.View().ViewCenterPixelY();

    // --- ARCH §21.2: RIGHT-button drag pans the view. --- (a fixed REGION-LOCAL corner, not a
    // world-space projection — independent of zoom level, so it can never accidentally land off the
    // visible region the way a near-edge world coordinate can once scaled by an arbitrary zoom.)
    const ImVec2 emptySpacePosition(regionOrigin.x + 20.0f, regionOrigin.y + 20.0f);
    const ImVec2 emptySpaceReleasePosition(emptySpacePosition.x + 60.0f, emptySpacePosition.y + 60.0f);
    SimulatePressDragRelease(canvas, emptySpacePosition, emptySpaceReleasePosition, /*button=*/1);
    check(canvas.View().ViewCenterPixelX() != initialViewCenterX
       || canvas.View().ViewCenterPixelY() != initialViewCenterY,
          "ARCH §21.2 — a RIGHT-button press-drag-release pans the view");

    // --- ARCH §21.2: LEFT-button drag over empty space (no manual hit) never pans. ---
    const float panViewCenterX = canvas.View().ViewCenterPixelX();
    const float panViewCenterY = canvas.View().ViewCenterPixelY();
    SimulatePressDragRelease(canvas, emptySpacePosition, emptySpaceReleasePosition, /*button=*/0);
    check(canvas.View().ViewCenterPixelX() == panViewCenterX && canvas.View().ViewCenterPixelY() == panViewCenterY,
          "ARCH §21.2 — a LEFT-button drag over empty space never pans the view (no fallthrough pan path)");

    // --- STEP207: MapCanvas::Draw emits a visible rubber-band rectangle while a left-drag marquee
    // is in progress, and none once it settles back to idle. Reuses the SAME empty-space positions
    // the no-pan check above already used — the drag resolves against zero markers there, so the
    // vertex-count comparison below isolates the rectangle draw command alone, with no
    // selection-highlight side effect to confound it.
    {
        // Baseline: SimulateStationaryClick's own 4-frame shape, inlined here (that helper does not
        // expose its intermediate per-frame draw-data signal) to capture the "equivalent frame" — a
        // held press that never moved, never a marquee.
        io.AddMousePosEvent(emptySpacePosition.x, emptySpacePosition.y);
        io.AddMouseButtonEvent(0, false);
        BeginHeadlessFrame(); DrawOneFrame(canvas);
        io.AddMouseButtonEvent(0, true);
        BeginHeadlessFrame(); DrawOneFrame(canvas);
        BeginHeadlessFrame(); DrawOneFrame(canvas);   // the "equivalent frame" — held, zero travel
        const int stationaryVertexCount = ImGui::GetDrawData()->TotalVtxCount;
        io.AddMouseButtonEvent(0, false);
        BeginHeadlessFrame(); DrawOneFrame(canvas);

        // Marquee drag: SimulatePressDragRelease's own 4-frame shape, inlined for the same reason —
        // capture the mid-drag frame (button held, mouse moved to the release position, one frame
        // before the button-up frame).
        io.AddMousePosEvent(emptySpacePosition.x, emptySpacePosition.y);
        io.AddMouseButtonEvent(0, false);
        BeginHeadlessFrame(); DrawOneFrame(canvas);
        io.AddMouseButtonEvent(0, true);
        BeginHeadlessFrame(); DrawOneFrame(canvas);
        io.AddMousePosEvent(emptySpaceReleasePosition.x, emptySpaceReleasePosition.y);
        BeginHeadlessFrame(); DrawOneFrame(canvas);   // mid-drag frame
        const int midDragVertexCount = ImGui::GetDrawData()->TotalVtxCount;
        check(midDragVertexCount > stationaryVertexCount,
              "STEP207 — an active left-drag marquee draws a visible rubber-band rectangle (more "
              "draw-list vertices than an ordinary stationary click's equivalent frame)");

        // Button-up frame, then one settle frame: the overlay pass reads `bPressActive` BEFORE
        // ApplyPointerInput runs each frame (same as every sibling overlay pass in this file, e.g.
        // DrawManualMarkerDragPass), so the button-up frame's OWN draw still reflects last frame's
        // still-true press state; the frame after is where it catches up to false.
        io.AddMouseButtonEvent(0, false);
        BeginHeadlessFrame(); DrawOneFrame(canvas);   // button-up frame
        BeginHeadlessFrame(); DrawOneFrame(canvas);   // settle frame
        const int settledVertexCount = ImGui::GetDrawData()->TotalVtxCount;
        check(settledVertexCount == stationaryVertexCount,
              "STEP207 — the marquee rectangle is gone once the button-up frame's effect settles, "
              "back to the exact no-rectangle baseline");
    }

    // --- ARCH §21.2/§21.5: a marquee box covering all three markers selects only the two UNLOCKED
    // ones — the locked marker is excluded from the box result entirely, not merely deprioritized.
    const ImVec2 boxPressPosition = ScreenPositionFor(regionOrigin, canvas, composite, 0.5f, 0.5f);
    const ImVec2 boxReleasePosition = ScreenPositionFor(regionOrigin, canvas, composite, 3.0f, 3.0f);
    lastReportedSet.keys.clear();
    SimulatePressDragRelease(canvas, boxPressPosition, boxReleasePosition, /*button=*/0);
    bool bContainsA = false, bContainsB = false, bContainsLocked = false;
    for (const OverlayInstanceKey_UI& key : lastReportedSet.keys) {
        if (key.instanceIndex == 1) bContainsA = true;
        if (key.instanceIndex == 2) bContainsB = true;
        if (key.instanceIndex == 3) bContainsLocked = true;
    }
    check(bContainsA && bContainsB && !bContainsLocked,
          "ARCH §21.2/§21.5 — a marquee box covering all three markers selects only the two "
          "UNLOCKED ones; the locked marker is excluded entirely");

    // --- ARCH §21.1: Ctrl-click toggles a real selection through the live pointer state machine. ---
    const ImVec2 markerAPosition = ScreenPositionFor(regionOrigin, canvas, composite, 1.0f, 1.0f);
    SimulateStationaryClick(canvas, markerAPosition);   // plain click: selects {A}
    check(lastReportedSet.keys.size() == 1 && lastReportedSet.keys[0].instanceIndex == 1,
          "a plain click selects exactly the clicked marker");
    SimulateStationaryClick(canvas, markerAPosition, /*bCtrl=*/true);   // Ctrl-click the SAME marker: toggles off
    check(lastReportedSet.keys.empty(),
          "ARCH §21.1 — a Ctrl-click on an already-selected marker toggles it OFF through the live "
          "pointer state machine, not just the underlying OverlayInstanceKeySet_UI mutator directly");

    ImGui::DestroyContext();
}

} // namespace Ui
} // namespace SanmapGen
