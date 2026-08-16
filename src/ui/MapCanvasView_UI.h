// MapCanvasView_UI.h — the map canvas's pan/zoom state and the ONE cursor -> preview-pixel
// mapping. Layer: UI. Header-only pure math: no imgui, no GL, no DATA — so the mapping a click
// depends on is testable without a UI frame, and there is exactly one copy of it (a second copy
// in the draw code is how a viewport drifts from its own hit-test).
// Model: the composited image always FILLS the square canvas region and zoom shrinks the visible
// window of the image (the texture-coordinate window below), so no part of the region is ever
// outside the image and the drawn rectangle never has to be clipped.
// A member type of MapCanvas_UI.h, not a widget anyone reaches independently.
#pragma once

namespace SanmapGen {
namespace Ui {

// Where a cursor landed in the composited image. `bInsideImage` false means "no pixel" — a
// caller must not read it as pixel (0,0) (Constitution §6: a cursor is untrusted input).
struct PreviewPixelCoordinate {
    int  pixelX       = 0;
    int  pixelY       = 0;
    bool bInsideImage = false;
};

// The visible window of the image in normalized texture coordinates — the low/high corner an
// image draw takes. At zoom 1 it is the whole image.
struct MapCanvasTextureWindow {
    float lowTextureCoordinateX  = 0.0f;   float lowTextureCoordinateY  = 0.0f;
    float highTextureCoordinateX = 1.0f;   float highTextureCoordinateY = 1.0f;
};

// Presentation settings (Constitution §8 — nothing here is a hidden literal).
struct MapCanvasViewSettings {
    float minimumZoomScale        = 1.0f;   // 1 = the whole image fills the region
    float maximumZoomScale        = 50.0f;
    float zoomStepFactor          = 1.1f;   // per wheel step, multiplicative
    float clickDragTolerancePixels = 3.0f;  // a press that moves less than this is a click
};

class MapCanvasView {
public:
    MapCanvasViewSettings settings;

    // The image being shown changed size: the view resets rather than keeping a window that no
    // longer addresses the image.
    void SetPreviewResolution(int resolution) {
        previewResolution = resolution > 0 ? resolution : 0;
        Reset();
    }
    void SetRegionSide(float side) { regionSidePixels = side > 0.0f ? side : 0.0f; }

    int   PreviewResolution() const { return previewResolution; }
    float RegionSidePixels() const { return regionSidePixels; }
    float ZoomScale() const { return zoomScale; }
    float ViewCenterPixelX() const { return viewCenterPixelX; }
    float ViewCenterPixelY() const { return viewCenterPixelY; }
    float VisibleSpanPixels() const {          // preview pixels across the region at this zoom
        return zoomScale > 0.0f ? static_cast<float>(previewResolution) / zoomScale : 0.0f;
    }

    void Reset() {
        zoomScale = settings.minimumZoomScale > 0.0f ? settings.minimumZoomScale : 1.0f;
        viewCenterPixelX = static_cast<float>(previewResolution) * 0.5f;
        viewCenterPixelY = viewCenterPixelX;
    }

    MapCanvasTextureWindow TextureWindow() const {
        MapCanvasTextureWindow window;
        if (previewResolution <= 0) return window;
        const float resolutionReciprocal = 1.0f / static_cast<float>(previewResolution);
        const float halfSpan = VisibleSpanPixels() * 0.5f;
        window.lowTextureCoordinateX  = (viewCenterPixelX - halfSpan) * resolutionReciprocal;
        window.lowTextureCoordinateY  = (viewCenterPixelY - halfSpan) * resolutionReciprocal;
        window.highTextureCoordinateX = (viewCenterPixelX + halfSpan) * resolutionReciprocal;
        window.highTextureCoordinateY = (viewCenterPixelY + halfSpan) * resolutionReciprocal;
        return window;
    }

    // Region-local cursor (screen pixels from the region's top-left corner) -> preview pixel.
    PreviewPixelCoordinate ResolvePreviewPixel(float regionLocalX, float regionLocalY) const {
        PreviewPixelCoordinate resolved;
        if (previewResolution <= 0 || regionSidePixels <= 0.0f) return resolved;
        const float span = VisibleSpanPixels();
        const float regionReciprocal = 1.0f / regionSidePixels;
        const float imagePointX = viewCenterPixelX + (regionLocalX * regionReciprocal - 0.5f) * span;
        const float imagePointY = viewCenterPixelY + (regionLocalY * regionReciprocal - 0.5f) * span;
        resolved.pixelX = static_cast<int>(FloorToInteger(imagePointX));
        resolved.pixelY = static_cast<int>(FloorToInteger(imagePointY));
        resolved.bInsideImage = regionLocalX >= 0.0f && regionLocalY >= 0.0f
                             && regionLocalX <= regionSidePixels && regionLocalY <= regionSidePixels
                             && resolved.pixelX >= 0 && resolved.pixelX < previewResolution
                             && resolved.pixelY >= 0 && resolved.pixelY < previewResolution;
        return resolved;
    }

    // A drag of the cursor moves the image WITH the cursor, so the window travels the other way.
    void PanByRegionPixels(float deltaRegionPixelsX, float deltaRegionPixelsY) {
        if (regionSidePixels <= 0.0f) return;
        const float pixelsPerRegionPixel = VisibleSpanPixels() / regionSidePixels;
        viewCenterPixelX -= deltaRegionPixelsX * pixelsPerRegionPixel;
        viewCenterPixelY -= deltaRegionPixelsY * pixelsPerRegionPixel;
        ClampViewCenter();
    }

    // Zoom about the cursor: the preview pixel under it stays under it. `zoomStepScale` is the
    // already-accumulated multiplier for the wheel movement (the caller owns the step curve), so
    // no zoom constant is written into this math.
    void ZoomAtRegionPoint(float regionLocalX, float regionLocalY, float zoomStepScale) {
        if (previewResolution <= 0 || regionSidePixels <= 0.0f || zoomStepScale <= 0.0f) return;
        const float regionReciprocal = 1.0f / regionSidePixels;
        const float fractionX = regionLocalX * regionReciprocal - 0.5f;
        const float fractionY = regionLocalY * regionReciprocal - 0.5f;
        const float previousSpan = VisibleSpanPixels();
        const float anchorPixelX = viewCenterPixelX + fractionX * previousSpan;
        const float anchorPixelY = viewCenterPixelY + fractionY * previousSpan;
        zoomScale = ClampZoom(zoomScale * zoomStepScale);
        const float span = VisibleSpanPixels();
        viewCenterPixelX = anchorPixelX - fractionX * span;
        viewCenterPixelY = anchorPixelY - fractionY * span;
        ClampViewCenter();
    }

private:
    static float FloorToInteger(float value) {
        const float truncated = static_cast<float>(static_cast<int>(value));
        return value < truncated ? truncated - 1.0f : truncated;   // no <cmath> for one floor
    }
    float ClampZoom(float requested) const {
        if (requested < settings.minimumZoomScale) return settings.minimumZoomScale;
        if (requested > settings.maximumZoomScale) return settings.maximumZoomScale;
        return requested;
    }
    static float ClampSpanCenter(float center, float halfSpan, float resolution) {
        if (halfSpan * 2.0f >= resolution) return resolution * 0.5f;   // the whole image is shown
        if (center < halfSpan) return halfSpan;
        return center > resolution - halfSpan ? resolution - halfSpan : center;
    }
    void ClampViewCenter() {
        const float halfSpan = VisibleSpanPixels() * 0.5f;
        const float resolution = static_cast<float>(previewResolution);
        viewCenterPixelX = ClampSpanCenter(viewCenterPixelX, halfSpan, resolution);
        viewCenterPixelY = ClampSpanCenter(viewCenterPixelY, halfSpan, resolution);
    }

    int   previewResolution = 0;      float regionSidePixels = 0.0f;
    float zoomScale         = 1.0f;
    float viewCenterPixelX  = 0.0f;   float viewCenterPixelY = 0.0f;
};

} // namespace Ui
} // namespace SanmapGen
