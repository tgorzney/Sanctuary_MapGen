// MapCanvas_ActivePanelGate_UI_Test.cpp — STEP113 acceptance test: a manual-marker drag gesture
// may only BEGIN while ApplicationPanel::Markers is the shell's active panel. Mirrors
// MapCanvas_ScenarioEditModeOwnership_UI_Test.cpp's own GL-backed real-imgui-frame technique,
// asserting both on the marker's own position (did the drag move it) and, for the refused case,
// on MapCanvas::View()'s pixel-center state (did the press fall through to the normal pan path).
// One translation unit of the MapCanvas_UI_Test binary.
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
constexpr unsigned long long kFontAtlasIdentifier = 0xF0000004ull;
constexpr float kMarkerWorldX = 2.0f;
constexpr float kMarkerWorldZ = 2.0f;

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
    ImGui::Begin("ActivePanelGateTestWindow");
    const ImVec2 regionOrigin = ImGui::GetCursorScreenPos();
    canvas.Draw("mapCanvas", kRegionSidePixels);
    ImGui::End();
    ImGui::Render();
    return regionOrigin;
}

// Primes one frame to learn this canvas's region origin, then resolves the screen point a world
// position projects to under this canvas's OWN current view state.
ImVec2 PressPositionOnMarker(MapCanvas& canvas, const PreviewComposite& composite,
                             float worldX, float worldZ) {
    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(-100.0f, -100.0f);
    io.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame();
    const ImVec2 regionOrigin = DrawOneFrame(canvas);
    const PreviewComposite::PreviewPixelPoint previewPixel = composite.WorldToPreviewPixel(worldX, worldZ);
    const RegionLocalPoint regionLocal =
        canvas.View().ProjectPreviewPixelToRegionLocal(previewPixel.pixelX, previewPixel.pixelY);
    return ImVec2(regionOrigin.x + regionLocal.regionLocalX, regionOrigin.y + regionLocal.regionLocalY);
}

void SimulatePressDragRelease(MapCanvas& canvas, ImVec2 pressPosition) {
    ImGuiIO& io = ImGui::GetIO();
    // Reposition with the button still UP first — this helper may run more than once against the
    // same live context, so the jump must never land on a frame ApplyDrag reads a delta from.
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

void WireCanvas(MapCanvas& canvas, Sys::GpuResourceManager& manager, const PreviewComposite& composite,
                std::vector<Params::MarkerInstanceGroup>& markers,
                const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                const Params::Geometry& geometry, const Params::MapRecipe& recipe) {
    canvas.SetPreviewTexture(&manager, composite.CompositeTexture(), composite.Resolution());
    canvas.SetPreviewComposite(&composite);
    canvas.View().SetRegionSide(kRegionSidePixels);
    canvas.SetOverlayRecipe(&recipe);
    canvas.SetManualMarkerDragSource(&markers, &markerLayers, &geometry, &recipe);
}

} // namespace

void RunMapCanvasActivePanelGateChecks(Sys::GpuResourceManager& manager) {
    PreviewTestScene scene;
    BuildPreviewTestScene(scene);
    PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.fields,
                               scene.instances, scene.entityIdentifiers);
    ConfigurePreviewSettings(composite.Settings());
    composite.Settings().previewResolution = kPreviewResolution;
    composite.SetGpuResourceManager(&manager);
    composite.Compose();
    // One unlocked layer, one ungrouped MarkerTransform at a known world position.
    std::vector<Params::MarkerInstanceGroup> markers(1);
    markers[0].transforms.push_back(Params::MarkerTransform{});
    Params::InstancedTransform& transform = markers[0].transforms[0].transform;
    markers[0].transforms[0].name = "Marker";
    transform.positionX = kMarkerWorldX; transform.positionZ = kMarkerWorldZ;
    std::vector<Params::MarkerInstanceLayer> markerLayers(1);   // default-constructed: unlocked
    Params::MapRecipe recipe;   // default globalSymmetryMask/radialSymmetryRepeatCount
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(600.0f, 600.0f);
    io.IniFilename = nullptr;

    MapCanvas canvas;
    WireCanvas(canvas, manager, composite, markers, markerLayers, scene.geometry, recipe);
    const ImVec2 pressPosition = PressPositionOnMarker(canvas, composite, kMarkerWorldX, kMarkerWorldZ);
    // Zoom in first: at minimum zoom the whole image fills the region, so ClampViewCenter() clamps
    // every pan back to center regardless — a fallthrough pan needs zoom to have anything to prove.
    canvas.ApplyScroll(kRegionSidePixels * 0.5f, kRegionSidePixels * 0.5f, 8.0f);
    const float initialViewCenterX = canvas.View().ViewCenterPixelX();
    const float initialViewCenterY = canvas.View().ViewCenterPixelY();
    // --- Case 1: Markers panel active — the drag BEGINS and follows the cursor ---
    ApplicationPanel activePanel = ApplicationPanel::Markers;
    canvas.SetActivePanelSource(&activePanel);
    SimulatePressDragRelease(canvas, pressPosition);
    check(transform.positionX != kMarkerWorldX || transform.positionZ != kMarkerWorldZ,
          "Markers panel active: a press-drag-release on a manual marker moves it");

    // --- Case 2: a non-Markers panel active — refused; falls through to the normal pan path ---
    transform.positionX = kMarkerWorldX; transform.positionZ = kMarkerWorldZ;
    activePanel = ApplicationPanel::Heightmap;   // same MapCanvas, same pointer, new value
    SimulatePressDragRelease(canvas, pressPosition);
    check(transform.positionX == kMarkerWorldX && transform.positionZ == kMarkerWorldZ,
          "a non-Markers panel active: the identical gesture leaves the marker untouched");
    check(canvas.View().ViewCenterPixelX() != initialViewCenterX
       || canvas.View().ViewCenterPixelY() != initialViewCenterY,
          "a non-Markers panel active: the press falls through to the normal pan path");

    // --- Case 3: no panel source wired (activePanelSource left at its nullptr default) ---
    MapCanvas noPanelSourceCanvas;   // SetActivePanelSource deliberately never called on this canvas
    WireCanvas(noPanelSourceCanvas, manager, composite, markers, markerLayers, scene.geometry, recipe);
    const ImVec2 noPanelPressPosition =
        PressPositionOnMarker(noPanelSourceCanvas, composite, kMarkerWorldX, kMarkerWorldZ);
    SimulatePressDragRelease(noPanelSourceCanvas, noPanelPressPosition);
    check(transform.positionX == kMarkerWorldX && transform.positionZ == kMarkerWorldZ,
          "no panel source wired: null refuses the drag rather than defaulting to permit it");
    ImGui::DestroyContext();
}

} // namespace Ui
} // namespace SanmapGen
