// MapCanvas_AreaDragDispatch_UI.cpp — MapCanvas::AreaGestureEligible/IsAreaLocked/TryBeginAreaDrag/
// ContinueAreaDrag/EndAreaDrag/CreateAreaFromDrag (ARCH §21.8). ARCH §14.18 Part 3 — the retired
// composite-side drag-suppression-index mechanism (and its own dedicated setter) that gave
// "exactly two recomposites per gesture" is GONE: TryBeginAreaDrag fires no refresh at all
// (selection is not a composite input); ContinueAreaDrag fires one real GPU recompose per frame
// the rectangle actually moves, gated by
// the Tier-B2 cost watchdog (AreaRecompositeThrottle_UI.h, items 18/20); EndAreaDrag's refresh is
// unconditional and exempt from the throttle. STEP212's per-area lock query (IsAreaLocked) and its
// gating of TryBeginAreaDrag's two hit-test steps are untouched by this ticket. Standalone sibling
// of MapCanvas_ManualDragDispatch_UI.cpp's 3-way Markers/Props/Decals dispatcher — Areas has no
// group/transform/lock shape to fit into that dispatcher's switch (§21.8 correction 1/5).
#include "MapCanvas_UI.h"
#include "AreaRecompositeThrottle_UI.h"
#include "AreasTab_List_UI.h"       // NextAreaName, MakeNamesUnique, ResolveAreaLocked
#include "PreviewComposite_UI.h"
#include <algorithm>
#include <cmath>
#include <imgui.h>

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
// class), never a member of `*this`. Reused verbatim by DrawAreaOverlayPass's cursor-shape section
// (MapCanvas_AreaDraw_UI.cpp) — one lock query, not two independently-maintained checks.
bool MapCanvas::IsAreaLocked(int areaIndex) const {
    if (manualAreaDrag.areas == nullptr || manualAreaDrag.areaLocks == nullptr) return true;
    if (areaIndex < 0 || areaIndex >= static_cast<int>(manualAreaDrag.areas->size())) return true;
    const Params::MapArea& area = (*manualAreaDrag.areas)[static_cast<std::size_t>(areaIndex)];
    return *ResolveAreaLocked(*manualAreaDrag.areaLocks, area.name);
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
                // ARCH §14.18 item 23-A — a begin fires NO refresh request: with the suppression
                // index gone, a begin changes no composite input (BeginAreaDragGesture does not move
                // the rectangle, and selection is not a composite input — border/handles are the
                // immediate-mode chrome pass's job, BuildMapAreaConfigurations reads no selection). A
                // begin-time recompose would produce a byte-identical image. It DOES reset the
                // watchdog, so every gesture starts un-throttled (item 20's own "no per-gesture
                // re-arm latch" — the reset lives at the START of the next gesture instead).
                if (bAreaDragActive) areaRecompositeThrottle = AreaRecompositeThrottleState();
                return bAreaDragActive;
            }
        }
    }

    // Step 2 — a miss on the selected area's own handles/body (or it was locked and so never
    // tested): body hit-test over EVERY UNLOCKED area, forward iteration, FIRST match wins, early
    // exit (ARCH §14.19 — supersedes this block's own former "last match wins" citation of the old
    // §21.8 convention: ascending index is now Z-descending, so the first unlocked hit IS the
    // topmost area). STEP212 interpretation call 1: a LOCKED area is excluded from this scan
    // entirely.
    int hitIndex = -1;
    for (int index = 0; index < static_cast<int>(areas.size()); ++index) {
        if (!IsAreaLocked(index)
            && IsWorldPointInsideArea(areas[static_cast<std::size_t>(index)], worldPoint.worldX, worldPoint.worldZ)) {
            hitIndex = index;
            break;   // ascending index is Z-descending: the first unlocked hit IS the topmost area.
        }
    }
    if (hitIndex < 0) return false;   // total miss (or every candidate locked) — release resolves click/create

    if (manualAreaDrag.selectedAreaIndex != nullptr) *manualAreaDrag.selectedAreaIndex = hitIndex;
    bAreaDragActive = BeginAreaDragGesture(manualAreaDrag.state, areas, hitIndex, AreaHandle_UI::Center,
                                           worldPoint.worldX, worldPoint.worldZ);
    if (bAreaDragActive) areaRecompositeThrottle = AreaRecompositeThrottleState();
    return bAreaDragActive;
}

void MapCanvas::ContinueAreaDrag(float regionLocalX, float regionLocalY, bool bShiftHeld, bool bCtrlHeld) {
    if (!bAreaDragActive || manualAreaDrag.areas == nullptr || composite == nullptr) return;
    std::vector<Params::MapArea>& areas = *manualAreaDrag.areas;
    const int areaIndex = manualAreaDrag.state.areaIndex;

    // ARCH §14.18 item 4/23-A — the exact snapshot/compare idiom AreasTab_List_UI.h's own
    // SetAreaToMapSize already establishes: "reports whether the rectangle moved, so a button press
    // that changes nothing costs no recomposite." Guarded on range exactly as UpdateAreaDragGesture's
    // own defensive check (AreaDragGesture_UI.h) — an out-of-range areaIndex snapshots nothing and
    // bRectangleMoved stays false, matching UpdateAreaDragGesture's own no-op in that case.
    const bool bAreaIndexValid = areaIndex >= 0 && areaIndex < static_cast<int>(areas.size());
    Params::MapArea beforeRect;
    if (bAreaIndexValid) beforeRect = areas[static_cast<std::size_t>(areaIndex)];

    const PreviewPixelCoordinate previewPixel = view.ResolvePreviewPixel(regionLocalX, regionLocalY);
    const PreviewComposite::PreviewWorldPoint worldPoint = composite->PreviewPixelToWorld(
        static_cast<float>(previewPixel.pixelX), static_cast<float>(previewPixel.pixelY));
    UpdateAreaDragGesture(manualAreaDrag.state, areas, worldPoint.worldX, worldPoint.worldZ,
                          bShiftHeld, bCtrlHeld);

    bool bRectangleMoved = false;
    if (bAreaIndexValid) {
        const Params::MapArea& afterRect = areas[static_cast<std::size_t>(areaIndex)];
        bRectangleMoved = beforeRect.originX != afterRect.originX || beforeRect.originZ != afterRect.originZ
                        || beforeRect.width != afterRect.width || beforeRect.length != afterRect.length;
    }
    // ARCH §14.18 items 4/18/20 — the watchdog decides; this call site only supplies the frame's own
    // facts (did it move, what did the LAST compose cost, what time is it) and fires the SAME
    // areaCompositeRefreshCallback STEP211 already wired — never a second recomposite path.
    if (ShouldRequestAreaRecomposite(areaRecompositeThrottle, bRectangleMoved,
                                     composite->LastComposeMillis(), ImGui::GetTime() * 1000.0)
        && areaCompositeRefreshCallback)
        areaCompositeRefreshCallback();
}

void MapCanvas::EndAreaDrag() {
    EndAreaDragGesture(manualAreaDrag.state);
    bAreaDragActive = false;
    // ARCH §14.18 item 4/23-A — unconditional: the final rectangle must always be composited,
    // regardless of throttle state and regardless of whether the last frame moved, and is
    // explicitly EXEMPT from the throttle's interval (the gesture must always end in a
    // guaranteed-correct final image).
    if (areaCompositeRefreshCallback) areaCompositeRefreshCallback();
    areaRecompositeThrottle = AreaRecompositeThrottleState();
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
    // ARCH §14.19 — the ONE insertion function, keeps recipe.areas continuously sorted ascending
    // by size (supersedes the old push_back + size()-1 "landed at the back" assumption).
    const std::size_t newIndex = Params::InsertMapAreaSortedBySize(*manualAreaDrag.areas, area);
    MakeNamesUnique(*manualAreaDrag.areas);   // called HERE, not left for DrawAreasTab's end-of-frame
                                               // call — see ARCH §21.8's own "Create-by-drag" section
                                               // — confirmed: mutates .name in place only, never
                                               // reorders, so newIndex stays valid across this call
    // STEP212 — the human's own explicit rule: a freshly created area starts UNLOCKED. Reads the
    // area's FINAL (post-MakeNamesUnique) name back out of the vector rather than reusing the local
    // `area` copy's own name.
    if (manualAreaDrag.areaLocks != nullptr)
        ResolveAreaLocked(*manualAreaDrag.areaLocks,
                          (*manualAreaDrag.areas)[newIndex].name,
                          /*bDefaultLocked=*/false);
    if (manualAreaDrag.selectedAreaIndex != nullptr)
        *manualAreaDrag.selectedAreaIndex = static_cast<int>(newIndex);
    // ARCH §14.18 item 4/14 — a brand-new area must appear: one recomposite, unchanged by this
    // ticket. Not throttle-gated (a create is a one-shot event, not a per-frame gesture).
    if (areaCompositeRefreshCallback) areaCompositeRefreshCallback();
}

} // namespace Ui
} // namespace SanmapGen
