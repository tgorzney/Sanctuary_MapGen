// MapCanvas_AreaDragDispatch_UI.cpp — MapCanvas::AreaGestureEligible/IsAreaLocked/TryBeginAreaDrag/
// ContinueAreaDrag/EndAreaDrag/CreateAreaFromDrag (ARCH §21.8), plus SetMapAreaSuppression
// (ARCH §14.17 item 11's exactly-two-recomposites-per-gesture rule). STEP212 adds the per-area lock
// query (IsAreaLocked) and gates TryBeginAreaDrag's two hit-test steps on it — the suppression/
// recomposite mechanism below is STEP211 territory and is otherwise byte-identical. Standalone
// sibling of MapCanvas_ManualDragDispatch_UI.cpp's 3-way Markers/Props/Decals dispatcher — Areas has
// no group/transform/lock shape to fit into that dispatcher's switch (§21.8 correction 1/5).
#include "MapCanvas_UI.h"
#include "AreasTab_List_UI.h"       // NextAreaName, MakeNamesUnique, ResolveAreaLocked
#include "PreviewComposite_UI.h"
#include <algorithm>
#include <cmath>

namespace SanmapGen {
namespace Ui {

// STEP212 — the Areas-panel-active gate ONLY. The lock check this function used to also perform
// (`!*manualAreaDrag.bAreasLocked`) is retired: lock is now per-area, so it cannot be answered until
// a specific area is known — every call site below asks IsAreaLocked(index) once it has one.
bool MapCanvas::AreaGestureEligible() const {
    return activePanelSource != nullptr && *activePanelSource == ApplicationPanel::Areas;
}

// STEP212 — missing sources or an out-of-range index refuse (answer locked), never silently permit
// (Constitution §6 — the same "null/false-safe refuses" posture AreaGestureEligible's own panel
// gate already uses). Declared `const`: it mutates only the POINTEE of `manualAreaDrag.areaLocks`
// (a lazy append, exactly `ResolveAreaColor`'s own already-established precedent elsewhere in this
// class, e.g. this file's own SetMapAreaSuppression neighbor and DrawAreaOverlayPass), never a
// member of `*this`. Reused verbatim by DrawAreaOverlayPass's cursor-shape section
// (MapCanvas_AreaDraw_UI.cpp) — one lock query, not two independently-maintained checks.
bool MapCanvas::IsAreaLocked(int areaIndex) const {
    if (manualAreaDrag.areas == nullptr || manualAreaDrag.areaLocks == nullptr) return true;
    if (areaIndex < 0 || areaIndex >= static_cast<int>(manualAreaDrag.areas->size())) return true;
    const Params::MapArea& area = (*manualAreaDrag.areas)[static_cast<std::size_t>(areaIndex)];
    return *ResolveAreaLocked(*manualAreaDrag.areaLocks, area.name);
}

// ARCH §14.17 item 11 — the ONE place the "did the suppressed index actually change" condition is
// evaluated, shared by every call site below (an index, never a `bEnabled` toggle: flipping
// `fieldLayers[i].bEnabled` for a drag's duration would clobber the user's own View-popup/left-column
// enable state, which this dedicated slot cannot collide with). STEP212 — untouched.
void MapCanvas::SetMapAreaSuppression(int areaIndex) {
    if (manualAreaDrag.mapAreaSuppressedIndex == nullptr) return;
    if (*manualAreaDrag.mapAreaSuppressedIndex == areaIndex) return;
    *manualAreaDrag.mapAreaSuppressedIndex = areaIndex;
    if (areaCompositeRefreshCallback) areaCompositeRefreshCallback();
}

bool MapCanvas::TryBeginAreaDrag(float regionLocalX, float regionLocalY) {
    bAreaDragActive = false;
    if (!AreaGestureEligible()) return false;
    if (manualAreaDrag.areas == nullptr || composite == nullptr) return false;
    std::vector<Params::MapArea>& areas = *manualAreaDrag.areas;

    const PreviewPixelCoordinate previewPixel = view.ResolvePreviewPixel(regionLocalX, regionLocalY);
    const PreviewComposite::PreviewWorldPoint worldPoint = composite->PreviewPixelToWorld(
        static_cast<float>(previewPixel.pixelX), static_cast<float>(previewPixel.pixelY));

    // Step 1 — a selection exists AND is unlocked: hit-test THAT one area's own 8 handles + body
    // first. STEP212: the old single upfront AreaGestureEligible() lock check is replaced by this
    // per-area IsAreaLocked() test, now that the lock is per-name, not a tab-wide bool.
    if (manualAreaDrag.selectedAreaIndex != nullptr) {
        const int selectedIndex = *manualAreaDrag.selectedAreaIndex;
        if (selectedIndex >= 0 && selectedIndex < static_cast<int>(areas.size())
            && !IsAreaLocked(selectedIndex)) {
            const AreaHandle_UI handle = HitTestAreaHandles(areas[static_cast<std::size_t>(selectedIndex)],
                                                             *composite, view, regionLocalX, regionLocalY);
            if (handle != AreaHandle_UI::None) {
                bAreaDragActive = BeginAreaDragGesture(manualAreaDrag.state, areas, selectedIndex, handle,
                                                       worldPoint.worldX, worldPoint.worldZ);
                // ARCH §14.17 item 11 — the FIRST of exactly two recomposites this gesture will cost.
                if (bAreaDragActive) SetMapAreaSuppression(selectedIndex);
                return bAreaDragActive;
            }
        }
    }

    // Step 2 — a miss on the selected area's own handles/body (or it was locked and so never
    // tested): body hit-test over EVERY UNLOCKED area, forward iteration, last match wins
    // (later-in-vector is drawn topmost, Widget_AreaEditor.cpp's own "reverse Z-order" comment,
    // Widget_AreaEditor.cpp:50). STEP212 interpretation call 1: a LOCKED area is excluded from this
    // scan entirely — it stays fully inert to canvas hit-testing, including re-selection, exactly
    // mirroring ARCH_21_08's own pre-STEP212 ruling ("Locked gates the whole surface, uniformly,
    // including selection-by-click... Selecting a different area while locked is still possible
    // through the Area Stack list... unaffected"), just applied per-area instead of tab-wide. A
    // locked area remains selectable only through the Area Stack list's own Select signal
    // (AreasTab_UI.cpp's ApplyAreaListSignal), untouched by this canvas-side gate.
    int hitIndex = -1;
    for (int index = 0; index < static_cast<int>(areas.size()); ++index)
        if (!IsAreaLocked(index)
            && IsWorldPointInsideArea(areas[static_cast<std::size_t>(index)], worldPoint.worldX, worldPoint.worldZ))
            hitIndex = index;
    if (hitIndex < 0) return false;   // total miss (or every candidate locked) — release resolves click/create

    if (manualAreaDrag.selectedAreaIndex != nullptr) *manualAreaDrag.selectedAreaIndex = hitIndex;
    bAreaDragActive = BeginAreaDragGesture(manualAreaDrag.state, areas, hitIndex, AreaHandle_UI::Center,
                                           worldPoint.worldX, worldPoint.worldZ);
    if (bAreaDragActive) SetMapAreaSuppression(hitIndex);
    return bAreaDragActive;
}

void MapCanvas::ContinueAreaDrag(float regionLocalX, float regionLocalY, bool bShiftHeld, bool bCtrlHeld) {
    if (!bAreaDragActive || manualAreaDrag.areas == nullptr || composite == nullptr) return;
    const PreviewPixelCoordinate previewPixel = view.ResolvePreviewPixel(regionLocalX, regionLocalY);
    const PreviewComposite::PreviewWorldPoint worldPoint = composite->PreviewPixelToWorld(
        static_cast<float>(previewPixel.pixelX), static_cast<float>(previewPixel.pixelY));
    // ARCH §21.8 correction 4 / §14.17 item 11 — writes recipe.areas LIVE every frame and requests
    // ZERO recomposites: the live visual is the bespoke immediate-mode pass in
    // MapCanvas_AreaDraw_UI.cpp, not a GPU recompose. STEP212 — untouched.
    UpdateAreaDragGesture(manualAreaDrag.state, *manualAreaDrag.areas, worldPoint.worldX, worldPoint.worldZ,
                          bShiftHeld, bCtrlHeld);
}

void MapCanvas::EndAreaDrag() {
    EndAreaDragGesture(manualAreaDrag.state);
    bAreaDragActive = false;
    // ARCH §14.17 item 11 — the SECOND of exactly two recomposites: the area rejoins the composite
    // input now that the gesture is over. STEP212 — untouched.
    SetMapAreaSuppression(-1);
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
    area.name = NextAreaName(static_cast<int>(manualAreaDrag.areas->size()));   // AreasTab_List_UI.h,
                                                                                 // the SAME helper "Add New Area" uses
    manualAreaDrag.areas->push_back(area);
    MakeNamesUnique(*manualAreaDrag.areas);   // called HERE, not left for DrawAreasTab's end-of-frame
                                               // call — see ARCH §21.8's own "Create-by-drag" section
    const int newIndex = static_cast<int>(manualAreaDrag.areas->size()) - 1;
    // STEP212 — the human's own explicit rule: a freshly created area starts UNLOCKED. Reads the
    // area's FINAL (post-MakeNamesUnique) name back out of the vector rather than reusing the local
    // `area` copy's own name — the local copy predates whatever rename a collision would have
    // applied, so using it here could silently key the lock entry to a name nothing in `areas` uses.
    if (manualAreaDrag.areaLocks != nullptr)
        ResolveAreaLocked(*manualAreaDrag.areaLocks,
                          (*manualAreaDrag.areas)[static_cast<std::size_t>(newIndex)].name,
                          /*bDefaultLocked=*/false);
    if (manualAreaDrag.selectedAreaIndex != nullptr)
        *manualAreaDrag.selectedAreaIndex = newIndex;
    // ARCH §14.17 item 11 — a brand-new area must appear: one recomposite, no suppression change.
    // STEP212 — untouched.
    if (areaCompositeRefreshCallback) areaCompositeRefreshCallback();
}

} // namespace Ui
} // namespace SanmapGen
