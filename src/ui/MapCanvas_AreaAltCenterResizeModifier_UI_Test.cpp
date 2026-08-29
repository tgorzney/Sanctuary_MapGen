// MapCanvas_AreaAltCenterResizeModifier_UI_Test.cpp — STEP214 acceptance: Alt is an ADDITIONAL
// trigger for the exact same center-resize behavior ARCH §21.8 already ratified for Ctrl
// (UpdateAreaDragGesture's own math in AreaDragGesture_UI.cpp — completely unmodified by this
// ticket), not a new/different modifier semantic. GL-backed (mirrors
// MapCanvas_AreaDragSuppression_UI_Test.cpp's own technique exactly) because the one thing this
// ticket actually changed — MapCanvas_Draw_UI.cpp's own `io.KeyCtrl || io.KeyAlt` merge at the
// ContinueAreaDrag call site — lives in the ONE translation unit that reads imgui's `io` state, so
// the only way to prove it is a real MapCanvas::Draw() press/drag/release cycle with a real,
// AddKeyEvent-driven `io.KeyAlt`. Deliberately does NOT re-derive the resize math's own closed-form
// expected numbers (AreaDragGesture_UI_Test.cpp's RunCtrlCenterResizeChecks already owns that,
// headlessly, against UpdateAreaDragGesture directly) — it instead runs the IDENTICAL press/drag/
// release screen-pixel sequence four times (no modifier / Ctrl-only / Alt-only / Ctrl+Alt) and
// compares the resulting Params::MapArea bit-for-bit across runs, which sidesteps this file's own
// screen<->world projection math entirely while still proving Alt-alone reaches the exact same code
// path Ctrl-alone does, and that holding both together doesn't double-apply the doubling.
#include "MapCanvas_UI.h"
#include "PreviewComposite_TestScene_UI.h"
#include <imgui.h>

namespace SanmapGen {
namespace Ui {
namespace {

void check(bool bCondition, const char* label) { CheckPreviewExpectation(bCondition, label); }

constexpr int   kPreviewResolution = 64;
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
    ImGui::Begin("AreaAltCenterResizeModifierTestWindow");
    const ImVec2 regionOrigin = ImGui::GetCursorScreenPos();
    canvas.Draw("mapCanvas", kRegionSidePixels);
    ImGui::End();
    ImGui::Render();
    return regionOrigin;
}

// io.KeyCtrl/io.KeyAlt are read-only, recomputed by NewFrame() from the key-event queue every frame
// (imgui.h's own comment — MapCanvas_GestureOwnership_UI_Test.cpp's own SetModifierKeys already
// established this exact caveat for Ctrl/Shift) — the real modifier state must go through
// AddKeyEvent(ImGuiMod_*, ...), exactly as a real keyboard backend would report it.
void SetModifierKeys(bool bCtrl, bool bAlt) {
    ImGuiIO& io = ImGui::GetIO();
    io.AddKeyEvent(ImGuiMod_Ctrl, bCtrl);
    io.AddKeyEvent(ImGuiMod_Alt, bAlt);
}

ImVec2 ScreenPositionFor(const ImVec2& regionOrigin, MapCanvas& canvas, const PreviewComposite& composite,
                         float worldX, float worldZ) {
    const PreviewComposite::PreviewPixelPoint previewPixel = composite.WorldToPreviewPixel(worldX, worldZ);
    const RegionLocalPoint regionLocal =
        canvas.View().ProjectPreviewPixelToRegionLocal(previewPixel.pixelX, previewPixel.pixelY);
    return ImVec2(regionOrigin.x + regionLocal.regionLocalX, regionOrigin.y + regionLocal.regionLocalY);
}

// One full press-(E handle)-drag-(+40 screen px in X)-release cycle, with the given modifier keys
// held for the drag's own duration, against a FRESH Params::MapArea reset to the same starting
// rectangle every call — returns the resulting area so the caller can compare across runs. Does NOT
// touch `areaLocks` — the caller pre-seeds a single UNLOCKED entry for the shared area name once,
// up front, since every call here reuses that same name.
Params::MapArea RunEHandleResizeGesture(MapCanvas& canvas, const ImVec2& regionOrigin,
                                        const PreviewComposite& composite,
                                        std::vector<Params::MapArea>& areas,
                                        int& selectedAreaIndex, bool bCtrl, bool bAlt) {
    areas.clear();
    Params::MapArea area;
    area.name = "Resizable"; area.originX = 0.0f; area.originZ = 0.0f; area.width = 2.0f; area.length = 2.0f;
    areas.push_back(area);
    selectedAreaIndex = 0;

    // The E handle sits at (maxX, midZ) = (2, 1) for this rect — pressing exactly there guarantees
    // TryBeginAreaDrag's handle hit-test (kAreaHandleScreenRadiusPixels==8, distance 0 here) resolves
    // AreaHandle_UI::E, never Center.
    const ImVec2 handlePosition = ScreenPositionFor(regionOrigin, canvas, composite, 2.0f, 1.0f);
    const ImVec2 releasePosition(handlePosition.x + 40.0f, handlePosition.y);

    ImGuiIO& io = ImGui::GetIO();
    SetModifierKeys(bCtrl, bAlt);
    io.AddMousePosEvent(handlePosition.x, handlePosition.y);
    io.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame(); DrawOneFrame(canvas);
    io.AddMouseButtonEvent(0, true);
    BeginHeadlessFrame(); DrawOneFrame(canvas);
    io.AddMousePosEvent(releasePosition.x, releasePosition.y);
    BeginHeadlessFrame(); DrawOneFrame(canvas);
    io.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame(); DrawOneFrame(canvas);
    SetModifierKeys(false, false);

    return areas[0];
}

bool SameRect(const Params::MapArea& a, const Params::MapArea& b) {
    return a.originX == b.originX && a.originZ == b.originZ && a.width == b.width && a.length == b.length;
}

} // namespace

void RunMapCanvasAreaAltCenterResizeModifierChecks(Sys::GpuResourceManager& manager) {
    PreviewTestScene scene;
    BuildPreviewTestScene(scene);
    PreviewComposite composite(scene.geometry, scene.water, scene.strata, scene.areas, scene.fields,
                               scene.instances, scene.entityIdentifiers);
    ConfigurePreviewSettings(composite.Settings());
    composite.Settings().previewResolution = kPreviewResolution;
    composite.SetGpuResourceManager(&manager);
    composite.Compose();

    std::vector<Params::MapArea> areas;
    std::vector<AreaColorEntry>  areaColors;
    std::vector<AreaLockEntry>   areaLocks;
    // STEP212's per-area lock table defaults a first-touch name to LOCKED — pre-seed "Resizable"
    // UNLOCKED once, up front, exactly mirroring MapCanvas_AreaDragSuppression_UI_Test.cpp's own
    // established precedent for its own "Existing" area.
    ResolveAreaLocked(areaLocks, "Resizable", /*bDefaultLocked=*/false);
    int selectedAreaIndex = -1;

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(600.0f, 600.0f);
    io.IniFilename = nullptr;
    io.ConfigInputTrickleEventQueue = false;

    MapCanvas canvas;
    canvas.SetPreviewTexture(&manager, composite.CompositeTexture(), composite.Resolution());
    canvas.SetPreviewComposite(&composite);
    canvas.View().SetRegionSide(kRegionSidePixels);
    ApplicationPanel activePanel = ApplicationPanel::Areas;
    canvas.SetActivePanelSource(&activePanel);
    canvas.SetManualAreaDragSource(&areas, &areaColors, &areaLocks, &selectedAreaIndex);

    // Frame 0 — priming, mouse away: establishes the region origin this window layout produces
    // (mirrors every sibling GL-backed MapCanvas test's own frame-0 priming convention).
    io.AddMousePosEvent(-100.0f, -100.0f);
    io.AddMouseButtonEvent(0, false);
    BeginHeadlessFrame();
    const ImVec2 regionOrigin = DrawOneFrame(canvas);

    const Params::MapArea noModifierResult =
        RunEHandleResizeGesture(canvas, regionOrigin, composite, areas, selectedAreaIndex,
                                /*bCtrl=*/false, /*bAlt=*/false);
    const Params::MapArea ctrlOnlyResult =
        RunEHandleResizeGesture(canvas, regionOrigin, composite, areas, selectedAreaIndex,
                                /*bCtrl=*/true, /*bAlt=*/false);
    const Params::MapArea altOnlyResult =
        RunEHandleResizeGesture(canvas, regionOrigin, composite, areas, selectedAreaIndex,
                                /*bCtrl=*/false, /*bAlt=*/true);
    const Params::MapArea bothHeldResult =
        RunEHandleResizeGesture(canvas, regionOrigin, composite, areas, selectedAreaIndex,
                                /*bCtrl=*/true, /*bAlt=*/true);

    check(!SameRect(noModifierResult, ctrlOnlyResult),
          "sanity: Ctrl actually changes the E-handle resize result versus no modifier at all "
          "(otherwise the comparisons below would pass vacuously)");
    check(SameRect(altOnlyResult, ctrlOnlyResult),
          "STEP214 - Alt alone reaches the EXACT SAME center-resize result Ctrl alone already "
          "produces (ARCH Sec21.8's ratified UpdateAreaDragGesture math, unmodified by this ticket) - "
          "Alt is an additional trigger for the identical behavior, not a different one");
    check(SameRect(bothHeldResult, ctrlOnlyResult),
          "STEP214 - holding Ctrl AND Alt together produces the SAME result as either alone (a "
          "single OR'd boolean reaches UpdateAreaDragGesture - holding both never doubles the "
          "doubling)");

    ImGui::DestroyContext();
}

} // namespace Ui
} // namespace SanmapGen
