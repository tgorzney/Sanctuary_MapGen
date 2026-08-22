// MapCanvas_View_UI_Test.cpp — acceptance test, part 2: the pan/zoom cursor -> preview-pixel
// math. One translation unit of the MapCanvas_UI_Test binary (main() is MapCanvas_UI_Test.cpp).
// Needs neither GL nor an imgui frame, which is the point of keeping the mapping in
// MapCanvasView_UI.h: given a known pan/zoom and a known cursor, the resolved preview pixel is
// an exact expectation, not a screenshot.
#include "MapCanvas_UI.h"
#include "PreviewComposite_TestScene_UI.h"

namespace SanmapGen {
namespace Ui {
namespace {

void check(bool bCondition, const char* label) { CheckPreviewExpectation(bCondition, label); }

bool PixelIs(const PreviewPixelCoordinate& resolved, int pixelX, int pixelY) {
    return resolved.bInsideImage && resolved.pixelX == pixelX && resolved.pixelY == pixelY;
}

bool NearlyEqual(float value, float expected) {
    const float difference = value - expected;
    return difference < 0.001f && difference > -0.001f;
}

// A 64-pixel preview shown in a 256-pixel region: 4 screen pixels per preview pixel, so every
// expectation below is exact arithmetic.
void CheckUnzoomedMapping() {
    MapCanvasView view;
    view.SetPreviewResolution(64);
    view.SetRegionSide(256.0f);
    check(NearlyEqual(view.ZoomScale(), 1.0f), "a fresh view starts at the minimum zoom");
    check(NearlyEqual(view.VisibleSpanPixels(), 64.0f), "at zoom 1 the whole image is visible");
    check(PixelIs(view.ResolvePreviewPixel(128.0f, 128.0f), 32, 32),
          "the region centre resolves to the image centre pixel");
    check(PixelIs(view.ResolvePreviewPixel(0.0f, 0.0f), 0, 0),
          "the region's top-left corner resolves to pixel (0,0)");
    check(PixelIs(view.ResolvePreviewPixel(255.0f, 255.0f), 63, 63),
          "the region's last screen pixel resolves to the last preview pixel");
    check(PixelIs(view.ResolvePreviewPixel(20.0f, 44.0f), 5, 11),
          "an arbitrary cursor resolves by the 4-screen-pixels-per-preview-pixel scale");
    check(!view.ResolvePreviewPixel(-1.0f, 10.0f).bInsideImage
       && !view.ResolvePreviewPixel(300.0f, 10.0f).bInsideImage,
          "a cursor outside the region resolves to no pixel, not to pixel (0,0)");
}

// Zooming keeps the preview pixel under the cursor under the cursor — the property that makes a
// zoomed click land where the user is looking.
void CheckZoomAboutTheCursor() {
    MapCanvasView view;
    view.SetPreviewResolution(64);
    view.SetRegionSide(256.0f);
    const PreviewPixelCoordinate anchorBefore = view.ResolvePreviewPixel(64.0f, 64.0f);
    view.ZoomAtRegionPoint(64.0f, 64.0f, 2.0f);
    check(NearlyEqual(view.ZoomScale(), 2.0f), "one zoom step multiplies the zoom scale");
    check(NearlyEqual(view.VisibleSpanPixels(), 32.0f), "zoom 2 shows half the image");
    check(NearlyEqual(view.ViewCenterPixelX(), 24.0f) && NearlyEqual(view.ViewCenterPixelY(), 24.0f),
          "the view centre moves so the anchored pixel stays put");
    check(PixelIs(view.ResolvePreviewPixel(64.0f, 64.0f), anchorBefore.pixelX, anchorBefore.pixelY),
          "the preview pixel under the cursor is unchanged by the zoom");
    const MapCanvasTextureWindow window = view.TextureWindow();
    check(NearlyEqual(window.lowTextureCoordinateX, 0.125f)
       && NearlyEqual(window.highTextureCoordinateX, 0.625f),
          "the drawn texture window matches the zoomed view");
}

// Panning is measured in screen pixels and converted by the current zoom; the window can never
// leave the image.
void CheckPanAndClamping() {
    MapCanvasView view;
    view.SetPreviewResolution(64);
    view.SetRegionSide(256.0f);
    view.ZoomAtRegionPoint(128.0f, 128.0f, 2.0f);                 // centred zoom: centre stays 32
    check(NearlyEqual(view.ViewCenterPixelX(), 32.0f), "a centred zoom does not move the centre");
    view.PanByRegionPixels(-32.0f, -32.0f);                       // drag left/up by 32 screen px
    check(NearlyEqual(view.ViewCenterPixelX(), 36.0f) && NearlyEqual(view.ViewCenterPixelY(), 36.0f),
          "a drag pans by screen pixels scaled into preview pixels");
    check(PixelIs(view.ResolvePreviewPixel(128.0f, 128.0f), 36, 36),
          "after the pan the region centre resolves to the panned pixel");
    view.PanByRegionPixels(-4000.0f, -4000.0f);
    check(NearlyEqual(view.ViewCenterPixelX(), 48.0f),
          "panning stops at the image edge instead of showing outside the image");
    view.ZoomAtRegionPoint(128.0f, 128.0f, 0.0001f);
    check(NearlyEqual(view.ZoomScale(), view.settings.minimumZoomScale)
       && NearlyEqual(view.ViewCenterPixelX(), 32.0f),
          "zooming back out clamps to the minimum and recentres the whole image");
    view.ZoomAtRegionPoint(128.0f, 128.0f, 10000.0f);
    check(NearlyEqual(view.ZoomScale(), view.settings.maximumZoomScale),
          "zooming in stops at the maximum zoom setting");
}

// STEP47: ProjectPreviewPixelToRegionLocal is the inverse of ResolvePreviewPixel. The round trip
// is exact only up to ResolvePreviewPixel's own floor to an integer preview pixel, so the
// tolerance is one preview pixel's worth of region-local span at the current zoom.
void CheckRoundTripProjection() {
    MapCanvasView view;
    view.SetPreviewResolution(64);
    view.SetRegionSide(256.0f);

    auto checkRoundTrip = [](MapCanvasView& canvasView, float regionLocalX, float regionLocalY) {
        const PreviewPixelCoordinate resolved = canvasView.ResolvePreviewPixel(regionLocalX, regionLocalY);
        const RegionLocalPoint back = canvasView.ProjectPreviewPixelToRegionLocal(
            static_cast<float>(resolved.pixelX), static_cast<float>(resolved.pixelY));
        // The floor in ResolvePreviewPixel can lose up to one preview pixel, which is worth
        // RegionSidePixels()/VisibleSpanPixels() screen pixels at the current zoom — the
        // reciprocal of PreviewPixelsPerRegionPixel() (preview pixels PER screen pixel).
        const float tolerance = canvasView.PreviewPixelsPerRegionPixel() > 0.0f
            ? canvasView.RegionSidePixels() / canvasView.VisibleSpanPixels() + 0.01f
            : 0.01f;
        const float differenceX = back.regionLocalX - regionLocalX;
        const float differenceY = back.regionLocalY - regionLocalY;
        check((differenceX < tolerance && differenceX > -tolerance)
           && (differenceY < tolerance && differenceY > -tolerance),
              "the region-local point round-trips through the preview pixel within one preview pixel");
    };

    checkRoundTrip(view, 128.0f, 128.0f);
    checkRoundTrip(view, 20.0f, 44.0f);
    checkRoundTrip(view, 0.0f, 0.0f);
    checkRoundTrip(view, 255.0f, 255.0f);

    view.ZoomAtRegionPoint(64.0f, 64.0f, 2.0f);
    checkRoundTrip(view, 64.0f, 64.0f);
    checkRoundTrip(view, 100.0f, 150.0f);

    // A degenerate view (never sized) answers (0,0), not a divide-by-zero, mirroring
    // ResolvePreviewPixel's own early-return-zeroed contract.
    MapCanvasView freshView;
    const RegionLocalPoint degenerate = freshView.ProjectPreviewPixelToRegionLocal(10.0f, 10.0f);
    check(degenerate.regionLocalX == 0.0f && degenerate.regionLocalY == 0.0f,
          "an unsized view projects a preview pixel to (0,0) instead of dividing by zero");
}

} // namespace

void RunMapCanvasViewChecks() {
    CheckUnzoomedMapping();
    CheckZoomAboutTheCursor();
    CheckPanAndClamping();
    CheckRoundTripProjection();
}

} // namespace Ui
} // namespace SanmapGen
