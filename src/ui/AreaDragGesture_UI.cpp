// AreaDragGesture_UI.cpp — see AreaDragGesture_UI.h for the contract. Ports
// Widget_AreaEditor.cpp:125-217's delta/aspect-lock/Ctrl-center-resize math verbatim.
#include "AreaDragGesture_UI.h"
#include "MapCanvasView_UI.h"
#include "PreviewComposite_UI.h"
#include <algorithm>
#include <cmath>

namespace SanmapGen {
namespace Ui {

void ComputeAreaHandleWorldPoints(const Params::MapArea& area, AreaHandleWorldPoint_UI outPoints[8]) {
    const float minX = area.originX,               minZ = area.originZ;
    const float maxX = area.originX + area.width,  maxZ = area.originZ + area.length;
    const float midX = (minX + maxX) * 0.5f,       midZ = (minZ + maxZ) * 0.5f;
    outPoints[0] = { AreaHandle_UI::N,  midX, minZ };
    outPoints[1] = { AreaHandle_UI::NE, maxX, minZ };
    outPoints[2] = { AreaHandle_UI::E,  maxX, midZ };
    outPoints[3] = { AreaHandle_UI::SE, maxX, maxZ };
    outPoints[4] = { AreaHandle_UI::S,  midX, maxZ };
    outPoints[5] = { AreaHandle_UI::SW, minX, maxZ };
    outPoints[6] = { AreaHandle_UI::W,  minX, midZ };
    outPoints[7] = { AreaHandle_UI::NW, minX, minZ };
}

AreaHandle_UI HitTestAreaHandles(const Params::MapArea& area, const PreviewComposite& composite,
                                 const MapCanvasView& view, float regionLocalX, float regionLocalY) {
    AreaHandleWorldPoint_UI points[8];
    ComputeAreaHandleWorldPoints(area, points);
    RegionLocalPoint projected[8];
    for (int index = 0; index < 8; ++index) {
        const PreviewComposite::PreviewPixelPoint pixel =
            composite.WorldToPreviewPixel(points[index].worldX, points[index].worldZ);
        projected[index] = view.ProjectPreviewPixelToRegionLocal(pixel.pixelX, pixel.pixelY);
    }
    const float radiusSquared = kAreaHandleScreenRadiusPixels * kAreaHandleScreenRadiusPixels;
    for (int index = 0; index < 8; ++index) {
        const float dx = regionLocalX - projected[index].regionLocalX;
        const float dy = regionLocalY - projected[index].regionLocalY;
        if (dx * dx + dy * dy < radiusSquared) return points[index].handle;
    }
    // Miss on all 8 handles: screen-space body/Center containment, reusing the already-projected
    // NW (index 7) / SE (index 3) corners — no second projection.
    const float lowX  = std::min(projected[7].regionLocalX, projected[3].regionLocalX);
    const float highX = std::max(projected[7].regionLocalX, projected[3].regionLocalX);
    const float lowY  = std::min(projected[7].regionLocalY, projected[3].regionLocalY);
    const float highY = std::max(projected[7].regionLocalY, projected[3].regionLocalY);
    if (regionLocalX >= lowX && regionLocalX <= highX && regionLocalY >= lowY && regionLocalY <= highY)
        return AreaHandle_UI::Center;
    return AreaHandle_UI::None;
}

bool IsWorldPointInsideArea(const Params::MapArea& area, float worldX, float worldZ) {
    return worldX >= area.originX && worldX <= area.originX + area.width
        && worldZ >= area.originZ && worldZ <= area.originZ + area.length;
}

bool BeginAreaDragGesture(AreaDragGestureState& state, const std::vector<Params::MapArea>& areas,
                          int areaIndex, AreaHandle_UI handle, float worldX, float worldZ) {
    state = AreaDragGestureState{};
    if (areaIndex < 0 || areaIndex >= static_cast<int>(areas.size())) return false;
    if (handle == AreaHandle_UI::None) return false;
    state.bActive        = true;
    state.areaIndex       = areaIndex;
    state.handle          = handle;
    state.dragStartRect   = areas[static_cast<std::size_t>(areaIndex)];
    state.dragStartWorldX = worldX;
    state.dragStartWorldZ = worldZ;
    state.aspectLockRatio = state.dragStartRect.length > 0.0f
        ? state.dragStartRect.width / state.dragStartRect.length : 1.0f;
    return true;
}

void UpdateAreaDragGesture(AreaDragGestureState& state, std::vector<Params::MapArea>& areas,
                           float worldX, float worldZ, bool bShiftHeld, bool bCtrlHeld) {
    if (!state.bActive) return;
    if (state.areaIndex < 0 || state.areaIndex >= static_cast<int>(areas.size())) {
        state.bActive = false;
        return;
    }
    Params::MapArea& area = areas[static_cast<std::size_t>(state.areaIndex)];
    const float dx = worldX - state.dragStartWorldX;
    const float dz = worldZ - state.dragStartWorldZ;

    if (state.handle == AreaHandle_UI::Center) {
        area.originX = state.dragStartRect.originX + dx;
        area.originZ = state.dragStartRect.originZ + dz;
        return;
    }

    const AreaHandle_UI h = state.handle;
    const float startWidth  = state.dragStartRect.width;
    const float startLength = state.dragStartRect.length;
    float deltaWidth = 0.0f, deltaLength = 0.0f;
    if (h == AreaHandle_UI::NE || h == AreaHandle_UI::E || h == AreaHandle_UI::SE) deltaWidth = dx;
    if (h == AreaHandle_UI::NW || h == AreaHandle_UI::W || h == AreaHandle_UI::SW) deltaWidth = -dx;
    if (h == AreaHandle_UI::SE || h == AreaHandle_UI::S || h == AreaHandle_UI::SW) deltaLength = dz;
    if (h == AreaHandle_UI::NE || h == AreaHandle_UI::N || h == AreaHandle_UI::NW) deltaLength = -dz;

    if (bCtrlHeld) { deltaWidth *= 2.0f; deltaLength *= 2.0f; }

    float newWidth  = startWidth  + deltaWidth;
    float newLength = startLength + deltaLength;

    if (bShiftHeld) {
        if (h == AreaHandle_UI::N || h == AreaHandle_UI::S) {
            newWidth = newLength * state.aspectLockRatio;
        } else if (h == AreaHandle_UI::E || h == AreaHandle_UI::W) {
            newLength = state.aspectLockRatio > 0.0f ? newWidth / state.aspectLockRatio : newLength;
        } else {   // a corner: NE, SE, SW, NW
            if (std::fabs(deltaWidth) > std::fabs(deltaLength)) newLength = state.aspectLockRatio > 0.0f
                ? newWidth / state.aspectLockRatio : newLength;
            else newWidth = newLength * state.aspectLockRatio;
        }
    }

    if (newWidth  < kAreaMinimumExtentWorldUnits) newWidth  = kAreaMinimumExtentWorldUnits;
    if (newLength < kAreaMinimumExtentWorldUnits) newLength = kAreaMinimumExtentWorldUnits;

    const float centerX = state.dragStartRect.originX + startWidth  * 0.5f;
    const float centerZ = state.dragStartRect.originZ + startLength * 0.5f;
    float newOriginX = state.dragStartRect.originX;
    float newOriginZ = state.dragStartRect.originZ;
    if (bCtrlHeld) {
        newOriginX = centerX - newWidth  * 0.5f;
        newOriginZ = centerZ - newLength * 0.5f;
    } else {
        if (h == AreaHandle_UI::NW || h == AreaHandle_UI::W || h == AreaHandle_UI::SW)
            newOriginX = state.dragStartRect.originX + startWidth - newWidth;
        if (h == AreaHandle_UI::N || h == AreaHandle_UI::S)
            newOriginX = centerX - newWidth * 0.5f;
        if (h == AreaHandle_UI::NW || h == AreaHandle_UI::N || h == AreaHandle_UI::NE)
            newOriginZ = state.dragStartRect.originZ + startLength - newLength;
        if (h == AreaHandle_UI::E || h == AreaHandle_UI::W)
            newOriginZ = centerZ - newLength * 0.5f;
    }

    area.originX = newOriginX; area.originZ = newOriginZ;
    area.width   = newWidth;   area.length   = newLength;
}

void EndAreaDragGesture(AreaDragGestureState& state) { state = AreaDragGestureState{}; }

} // namespace Ui
} // namespace SanmapGen
