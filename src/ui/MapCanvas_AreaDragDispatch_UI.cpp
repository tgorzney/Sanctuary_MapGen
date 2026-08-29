// MapCanvas_AreaDragDispatch_UI.cpp — MapCanvas::AreaGestureEligible/TryBeginAreaDrag/
// ContinueAreaDrag/EndAreaDrag/CreateAreaFromDrag (ARCH §21.8). Standalone sibling of
// MapCanvas_ManualDragDispatch_UI.cpp's 3-way Markers/Props/Decals dispatcher — Areas has no
// group/transform/lock shape to fit into that dispatcher's switch (§21.8 correction 1/5).
#include "MapCanvas_UI.h"
#include "AreasTab_List_UI.h"       // NextAreaName, MakeNamesUnique (via UniqueNameList_UI.h)
#include "PreviewComposite_UI.h"
#include <algorithm>
#include <cmath>

namespace SanmapGen {
namespace Ui {

bool MapCanvas::AreaGestureEligible() const {
    return activePanelSource != nullptr && *activePanelSource == ApplicationPanel::Areas
        && manualAreaDrag.bAreasLocked != nullptr && !*manualAreaDrag.bAreasLocked;
}

bool MapCanvas::TryBeginAreaDrag(float regionLocalX, float regionLocalY) {
    bAreaDragActive = false;
    if (!AreaGestureEligible()) return false;
    if (manualAreaDrag.areas == nullptr || composite == nullptr) return false;
    std::vector<Params::MapArea>& areas = *manualAreaDrag.areas;

    const PreviewPixelCoordinate previewPixel = view.ResolvePreviewPixel(regionLocalX, regionLocalY);
    const PreviewComposite::PreviewWorldPoint worldPoint = composite->PreviewPixelToWorld(
        static_cast<float>(previewPixel.pixelX), static_cast<float>(previewPixel.pixelY));

    // Step 1 — a selection exists: hit-test THAT one area's own 8 handles + body first.
    if (manualAreaDrag.selectedAreaIndex != nullptr) {
        const int selectedIndex = *manualAreaDrag.selectedAreaIndex;
        if (selectedIndex >= 0 && selectedIndex < static_cast<int>(areas.size())) {
            const AreaHandle_UI handle = HitTestAreaHandles(areas[static_cast<std::size_t>(selectedIndex)],
                                                             *composite, view, regionLocalX, regionLocalY);
            if (handle != AreaHandle_UI::None) {
                bAreaDragActive = BeginAreaDragGesture(manualAreaDrag.state, areas, selectedIndex, handle,
                                                       worldPoint.worldX, worldPoint.worldZ);
                return bAreaDragActive;
            }
        }
    }

    // Step 2 — a miss on the selected area's own handles/body: body hit-test over EVERY area,
    // forward iteration, last match wins (later-in-vector is drawn topmost, Widget_AreaEditor.cpp's
    // own "reverse Z-order" comment, Widget_AreaEditor.cpp:50).
    int hitIndex = -1;
    for (int index = 0; index < static_cast<int>(areas.size()); ++index)
        if (IsWorldPointInsideArea(areas[static_cast<std::size_t>(index)], worldPoint.worldX, worldPoint.worldZ))
            hitIndex = index;
    if (hitIndex < 0) return false;   // total miss — no state recorded; release resolves click/create

    if (manualAreaDrag.selectedAreaIndex != nullptr) *manualAreaDrag.selectedAreaIndex = hitIndex;
    bAreaDragActive = BeginAreaDragGesture(manualAreaDrag.state, areas, hitIndex, AreaHandle_UI::Center,
                                           worldPoint.worldX, worldPoint.worldZ);
    return bAreaDragActive;
}

void MapCanvas::ContinueAreaDrag(float regionLocalX, float regionLocalY, bool bShiftHeld, bool bCtrlHeld) {
    if (!bAreaDragActive || manualAreaDrag.areas == nullptr || composite == nullptr) return;
    const PreviewPixelCoordinate previewPixel = view.ResolvePreviewPixel(regionLocalX, regionLocalY);
    const PreviewComposite::PreviewWorldPoint worldPoint = composite->PreviewPixelToWorld(
        static_cast<float>(previewPixel.pixelX), static_cast<float>(previewPixel.pixelY));
    UpdateAreaDragGesture(manualAreaDrag.state, *manualAreaDrag.areas, worldPoint.worldX, worldPoint.worldZ,
                          bShiftHeld, bCtrlHeld);
}

void MapCanvas::EndAreaDrag() {
    EndAreaDragGesture(manualAreaDrag.state);
    bAreaDragActive = false;
}

void MapCanvas::CreateAreaFromDrag(float pressRegionLocalX, float pressRegionLocalY,
                                   float releaseRegionLocalX, float releaseRegionLocalY) {
    if (manualAreaDrag.areas == nullptr || composite == nullptr) return;
    const PreviewPixelCoordinate pressPixel = view.ResolvePreviewPixel(pressRegionLocalX, pressRegionLocalY);
    const PreviewPixelCoordinate releasePixel = view.ResolvePreviewPixel(releaseRegionLocalX, releaseRegionLocalY);
    const PreviewComposite::PreviewWorldPoint pressWorld = composite->PreviewPixelToWorld(
        static_cast<float>(pressPixel.pixelX), static_cast<float>(pressPixel.pixelY));
    const PreviewComposite::PreviewWorldPoint releaseWorld = composite->PreviewPixelToWorld(
        static_cast<float>(releasePixel.pixelX), static_cast<float>(releasePixel.pixelY));

    Params::MapArea area;
    area.originX = std::min(pressWorld.worldX, releaseWorld.worldX);
    area.originZ = std::min(pressWorld.worldZ, releaseWorld.worldZ);
    area.width   = std::max(kAreaMinimumExtentWorldUnits, std::fabs(releaseWorld.worldX - pressWorld.worldX));
    area.length  = std::max(kAreaMinimumExtentWorldUnits, std::fabs(releaseWorld.worldZ - pressWorld.worldZ));
    area.name = NextAreaName(static_cast<int>(manualAreaDrag.areas->size()));   // AreasTab_List_UI.h:62,
                                                                                 // the SAME helper "Add New Area" uses
    manualAreaDrag.areas->push_back(area);
    MakeNamesUnique(*manualAreaDrag.areas);   // called HERE, not left for DrawAreasTab's end-of-frame
                                               // call — see ARCH §21.8's own "Create-by-drag" section
    if (manualAreaDrag.selectedAreaIndex != nullptr)
        *manualAreaDrag.selectedAreaIndex = static_cast<int>(manualAreaDrag.areas->size()) - 1;
}

} // namespace Ui
} // namespace SanmapGen
