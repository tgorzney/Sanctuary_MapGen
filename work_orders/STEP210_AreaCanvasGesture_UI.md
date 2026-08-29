# STEP210 — Area canvas gesture: create-by-drag, 8-handle resize + body-move for `Params::MapArea`

**Layer:** UI. **Domain:** `MapCanvas` (the canvas gesture surface), `recipe.areas`, the Areas tab's
UI-only color side table (`AreasTabState::areaColors`). **Executor:** SanGen Coder. Authored by the
SanGen UI Expert, per `ARCH_21_08_AreaCanvasGesture.md` (§21.8, ratified) and
`ARCH_21_CanvasInteractionUnification.md` (the §21 index, updated with the §21.8 row). Every file this
ticket cites was read directly against the live tree while drafting it — this ticket is immediately
buildable, with no forward-looking/not-yet-landed prerequisites (unlike e.g. `STEP94`).

## Summary
Ports `gui/widgets/Widget_AreaEditor.cpp`'s create/resize/move algorithm onto the v2 canvas as a
brand-new, standalone (non-`Traits`, non-templated) gesture module, because `Params::MapArea` is a
flat vector with no group/transform/lock shape at all (§21.8 correction 1) — nothing here reuses
`InstanceDragGestureState`/`PropDragGesture_UI.h`'s `Traits` pattern. Scope, exactly per §21.8's
ruling:
- Drag on empty canvas space, while the Areas panel is active and unlocked, creates a new
  `Params::MapArea`.
- 8 resize handles + a body/Center move gesture for the selected area; a body hit on a *different*
  area re-selects it and begins its move in the same press.
- Every field write lands live in `recipe.areas`, every frame (no commit-on-release for the data —
  only the (absent, out-of-scope-here) recomposite gate would ever be release-gated, and Areas never
  trip one — `AreasTab_UI.h` SCOPE NOTE 1).
- `AreasTabState::bAreasLocked` gates the entire surface uniformly (handles, body-select, create,
  empty-click-deselect) — no partial gating.
- A new fill+border+handles+cursor-shape draw pass, reading the existing per-name `AreaColorEntry`
  table (`AreasTab_List_UI.h`'s `ResolveAreaColor`).

## Required reading
`ARCH_21_08_AreaCanvasGesture.md` (read in full — it is the binding law here; several corrections to
an earlier draft design are ratified in it, in place) and `ARCH_21_CanvasInteractionUnification.md`
(confirms §21.8 is independently dispatchable — its only real dependency on §21.1-§21.7 is the
already-shipped press/release skeleton in `ApplyPointerInput` and the `pressStartRegionLocalX/Y`/
`DrawMarqueeRectanglePass` fields it reuses).

---

## 1. New file: `src/ui/AreaDragGesture_UI.h`

Header-only declarations; the real logic lives in the paired `.cpp` (§21.8 correction 2 — unlike
`InstanceDragGesture_UI.h`, this is **not** header-only, and there is **no** `Traits` contract of any
kind). Forward-declares `PreviewComposite`/`MapCanvasView` rather than including their headers, to
keep this header's own dependency footprint minimal (only `<vector>` and `MapArea_PARAMS.h` are
needed by the declarations themselves).

```cpp
// AreaDragGesture_UI.h — the standalone canvas drag-and-follow/resize/create gesture for
// Params::MapArea (ARCH §21.8). Layer: UI. Unlike InstanceDragGesture_UI.h (Markers/Props/Decals,
// ARCH §21.3), this is NOT a Traits-wrapped template: recipe.areas is a flat vector with no
// group/transform/lock shape at all (§21.8 correction 1), so there is nothing to genericize over.
// Real logic lives in AreaDragGesture_UI.cpp — this header only declares state + free functions.
#pragma once
#include <vector>
#include "../params/MapArea_PARAMS.h"

namespace SanmapGen {
namespace Ui {

class PreviewComposite;
class MapCanvasView;

// 8 resize handles + Center (body move). Mirrors Widget_AreaEditor.cpp's own 1..9 numbering
// (dragCorner) verbatim, offset by 1 for the None=0 sentinel.
enum class AreaHandle_UI : int { None, N, NE, E, SE, S, SW, W, NW, Center };

// One of the 8 resize-handle world positions, tagged with which handle it is — computed once and
// shared by BOTH HitTestAreaHandles (hit-testing) and the draw pass' handle-circle rendering, so
// there is exactly one place the 8-point derivation lives.
struct AreaHandleWorldPoint_UI { AreaHandle_UI handle = AreaHandle_UI::None; float worldX = 0.0f; float worldZ = 0.0f; };

// Fills all 8 entries, in the fixed order N, NE, E, SE, S, SW, W, NW (indices 0..7) — the same fixed
// order Widget_AreaEditor.cpp:88-96's own if/else-if chain tie-breaks in.
void ComputeAreaHandleWorldPoints(const Params::MapArea& area, AreaHandleWorldPoint_UI outPoints[8]);

// One gesture's full state: which area/handle, and a full snapshot of the rectangle at
// gesture-start — every frame's delta is computed against this snapshot, mirroring
// Widget_AreaEditor.cpp's own dragStartArea snapshot verbatim.
struct AreaDragGestureState {
    bool            bActive        = false;
    int             areaIndex       = -1;     // index into recipe.areas
    AreaHandle_UI   handle          = AreaHandle_UI::None;
    Params::MapArea dragStartRect;
    float           dragStartWorldX = 0.0f;
    float           dragStartWorldZ = 0.0f;
    float           aspectLockRatio = 1.0f;   // startWidth / startLength, frozen at gesture-start
};

inline constexpr float kAreaHandleScreenRadiusPixels = 8.0f;   // ported verbatim, Widget_AreaEditor.cpp:46
inline constexpr float kAreaMinimumExtentWorldUnits  = 1.0f;   // ported verbatim, Widget_AreaEditor.cpp:183-184

// Screen-space, single-area test: ComputeAreaHandleWorldPoints, project each of the 8 through
// composite.WorldToPreviewPixel + view.ProjectPreviewPixelToRegionLocal, compare against
// (regionLocalX, regionLocalY) within kAreaHandleScreenRadiusPixels, in fixed N/NE/E/SE/S/SW/W/NW
// order — first match wins (mirrors Widget_AreaEditor.cpp:88-95's own if/else-if priority). A miss
// on all 8 falls back to a screen-space containment test against the already-projected NW/SE corners
// (outPoints[7]/outPoints[3]) — a hit there returns Center (mirrors Widget_AreaEditor.cpp:96's own
// body/Center check, same function, same fixed-order chain). Returns None if nothing hits.
AreaHandle_UI HitTestAreaHandles(const Params::MapArea& area, const PreviewComposite& composite,
                                 const MapCanvasView& view, float regionLocalX, float regionLocalY);

// World-space exact rectangle containment — used only for the "body hit-test over EVERY area" pass
// (TryBeginAreaDrag's own step 2, MapCanvas_AreaDragDispatch_UI.cpp), never for the single selected
// area's own handle/body test above (that one is screen-space, HitTestAreaHandles' own job).
bool IsWorldPointInsideArea(const Params::MapArea& area, float worldX, float worldZ);

// Mouse-down: hit-testing already happened. Refuses (state left inactive, returns false) for an
// out-of-range areaIndex or handle == AreaHandle_UI::None.
bool BeginAreaDragGesture(AreaDragGestureState& state, const std::vector<Params::MapArea>& areas,
                          int areaIndex, AreaHandle_UI handle, float worldX, float worldZ);

// One drag frame. Center: pure translate. Any of the 8 resize handles: Ctrl doubles the extent
// delta and resizes from the rect's own center; Shift locks the opposite axis to aspectLockRatio,
// the larger-magnitude delta deciding which axis leads on a corner handle. Each axis floors to
// kAreaMinimumExtentWorldUnits. No-op if `state` is not active or `state.areaIndex` is out of range
// (in which case state.bActive is also cleared, defensively).
void UpdateAreaDragGesture(AreaDragGestureState& state, std::vector<Params::MapArea>& areas,
                           float worldX, float worldZ, bool bShiftHeld, bool bCtrlHeld);

// Mouse-up: Areas have no materialize/cascade-delete step (§21.8 correction 1) — every field write
// already landed live during Update. This only clears `state`.
void EndAreaDragGesture(AreaDragGestureState& state);

} // namespace Ui
} // namespace SanmapGen
```

## 2. New file: `src/ui/AreaDragGesture_UI.cpp`

```cpp
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
```

**Verification against the port source (`gui/widgets/Widget_AreaEditor.cpp`):** the East/West-affecting
handle sets for `deltaWidth` (`{NE,E,SE}` / `{NW,W,SW}`), the North/South-affecting sets for
`deltaLength` (`{SE,S,SW}` / `{NE,N,NW}`), the Shift-aspect branch (N/S locks width-from-length, E/W
locks length-from-width, corners follow the larger-magnitude delta), the Ctrl-doubling applied to the
deltas *before* the floor, the floor applied *after* Shift but *before* the position recompute, and
the three position-override groups (`{NW,W,SW}` moves the X origin, `{N,S}` centers X, `{NW,N,NE}`
moves the Z origin, `{E,W}` centers Z; the remaining handles in each axis keep `dragStartRect`'s own
origin unchanged) were each checked line-by-line against `Widget_AreaEditor.cpp:145-209` while writing
this and match exactly, modulo the two deliberate, flagged additions below (out-of-scope for v1
fidelity, but Constitution §6-required):
- `BeginAreaDragGesture` refuses on an out-of-range `areaIndex`/a `None` handle (v1 cannot reach this
  case because its own `hoveredArea`/`hoveredCorner` gate already prevents it, but this port's callers
  are not gated identically, so the guard is added defensively).
- `aspectLockRatio`/the Shift E/W and corner branches guard `state.aspectLockRatio > 0.0f` before
  dividing by it (v1 divides unguarded by `aspect = startW/startL`; `startL` can only be zero if an
  area's own `length` was already zero, which the floor makes structurally unreachable after this
  ticket ships, but the guard costs nothing and matches this codebase's own `ReciprocalOrZero`-style
  divide discipline elsewhere).

---

## 3. Modified: `src/ui/MapCanvas_ManualDragSources_UI.h`

Add a third bundle struct alongside `ManualPropDragSources_UI`/`ManualDecalDragSources_UI` (same file,
per §21.8's own instruction — "the SAME file `ManualPropDragSources_UI`/`ManualDecalDragSources_UI`
already live in"):

```cpp
#include "AreaDragGesture_UI.h"
#include "AreasTab_List_UI.h"          // AreaColorEntry
#include "../params/MapArea_PARAMS.h"
```
(new includes, added alongside the existing three). Then, appended after `ManualDecalDragSources_UI`:

```cpp
struct ManualAreaDragSources_UI {
    std::vector<Params::MapArea>* areas             = nullptr;   // mutable: canvas creates/moves/resizes
    std::vector<AreaColorEntry>*  areaColors         = nullptr;   // mutable: ResolveAreaColor lazily
                                                                    // appends a default entry for a
                                                                    // freshly canvas-created area
    const bool*                   bAreasLocked       = nullptr;   // read-only: canvas never writes the lock
    int*                          selectedAreaIndex  = nullptr;   // mutable: auto-select-on-touch/deselect
    AreaDragGestureState           state;
};
```

---

## 4. Modified: `src/ui/MapCanvas_UI.h`

**New public setter** — insert directly after `SetManualDecalDragSource` (currently `MapCanvas_UI.h:146-154`):

```cpp
    // ARCH §21.8 — mirrors SetManualPropDragSource's shape minus Geometry/globalSymmetryRecipe (Areas
    // carry no symmetry/layer/lock concept of their own, §21.8 correction 1/3). `areas`/`areaColors`/
    // `selectedAreaIndex` are the only mutable pointers; `areasLocked` is read-only — the canvas never
    // writes the tab-wide lock.
    void SetManualAreaDragSource(std::vector<Params::MapArea>* areas, std::vector<AreaColorEntry>* areaColors,
                                  const bool* areasLocked, int* selectedAreaIndex) {
        manualAreaDrag.areas = areas; manualAreaDrag.areaColors = areaColors;
        manualAreaDrag.bAreasLocked = areasLocked; manualAreaDrag.selectedAreaIndex = selectedAreaIndex;
    }
```

**New private method declarations** — insert directly after the existing `ApplyMarqueeGesture`
declaration (currently `MapCanvas_UI.h:313-314`, the last line of the private method block before the
member-variable section):

```cpp
    // ARCH §21.8 — the Area canvas gesture: create-by-drag, 8-handle resize + body-move. Standalone,
    // not a fourth PlacementCollectionKind_UI (Areas has no group/transform/lock shape, §21.8
    // correction 1) — an independent sibling of TryBeginManualInstanceDrag's 3-way dispatcher, not a
    // fourth branch inside it.
    bool AreaGestureEligible() const;                                          // MapCanvas_AreaDragDispatch_UI.cpp
    bool TryBeginAreaDrag(float regionLocalX, float regionLocalY);             // ditto
    void ContinueAreaDrag(float regionLocalX, float regionLocalY, bool bShiftHeld, bool bCtrlHeld); // ditto
    void EndAreaDrag();                                                        // ditto
    void CreateAreaFromDrag(float pressRegionLocalX, float pressRegionLocalY,
                            float releaseRegionLocalX, float releaseRegionLocalY);   // ditto, release-time only
    void DrawAreaOverlayPass(float regionOriginX, float regionOriginY);        // MapCanvas_AreaDraw_UI.cpp
```

**New private fields** — insert directly after `bManualDecalDragActive` (currently `MapCanvas_UI.h:388-389`):

```cpp
    // ARCH §21.8 — the Area gesture's own drag source + live state (mirrors manualPropDrag/
    // manualDecalDrag's shape one struct type over). Independent of bManualMarkerDragActive/
    // bManualPropDragActive/bManualDecalDragActive's OR-chain on purpose (§21.8's own instruction —
    // Areas is not a fourth PlacementCollectionKind_UI member).
    ManualAreaDragSources_UI manualAreaDrag;
    bool                     bAreaDragActive = false;
```

No new `#include` is required in this file — `AreaColorEntry`/`Params::MapArea`/`AreaDragGestureState`
are already visible transitively through the already-included `MapCanvas_ManualDragSources_UI.h`
(§3 above).

---

## 5. New file: `src/ui/MapCanvas_AreaDragDispatch_UI.cpp`

Mirrors `MapCanvas_ManualDragDispatch_UI.cpp`'s role one tier over, as an independent sibling (NOT
folded into `TryBeginManualInstanceDrag`'s switch — Areas is not a `PlacementCollectionKind_UI`,
§21.8 correction 5).

```cpp
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
```

---

## 6. Modified: `src/ui/MapCanvas_Draw_UI.cpp`

**Press** (currently lines 170-174):
```cpp
    if (ImGui::IsItemActivated()) {
        bPressActive = true; pressTravelPixels = 0.0f;
        pressStartRegionLocalX = regionLocalX; pressStartRegionLocalY = regionLocalY;
        if (!TryBeginManualInstanceDrag(regionLocalX, regionLocalY))
            TryBeginAreaDrag(regionLocalX, regionLocalY);
    }
```
(today's call discards `TryBeginManualInstanceDrag`'s own `bool` return — it must now be captured so
the Area attempt only runs on a miss. Mutually exclusive by construction regardless of order — see
§21.8's own note — but the ordering above keeps the diff additive.)

**Continue** (currently lines 175-183):
```cpp
    const bool bManualDragActive = bManualMarkerDragActive || bManualPropDragActive || bManualDecalDragActive;
    if (bPressActive && ImGui::IsItemActive()) {
        pressTravelPixels += std::fabs(io.MouseDelta.x) + std::fabs(io.MouseDelta.y);
        if (bManualDragActive) ContinueManualInstanceDrag(regionLocalX, regionLocalY);
        else if (bAreaDragActive) ContinueAreaDrag(regionLocalX, regionLocalY, io.KeyShift, io.KeyCtrl);
    }
```

**Release** (currently lines 184-202) — Areas pre-empts the ordinary click/marquee fallback entirely
while its own panel is active (§21.8's own ruling — a canvas drag while the Areas panel is active
never falls through to ordinary marquee resolution):
```cpp
    if (bPressActive && ImGui::IsItemDeactivated()) {
        bPressActive = false;
        const bool bClick = pressTravelPixels <= view.settings.clickDragTolerancePixels;
        if (bManualDragActive) {
            EndManualInstanceDrag();
            if (bClick) ApplyClickGesture(regionLocalX, regionLocalY, io.KeyCtrl, io.KeyShift);
        } else if (bAreaDragActive) {
            EndAreaDrag();
        } else if (AreaGestureEligible()) {
            if (bClick) {
                if (manualAreaDrag.selectedAreaIndex != nullptr) *manualAreaDrag.selectedAreaIndex = -1;
            } else {
                CreateAreaFromDrag(pressStartRegionLocalX, pressStartRegionLocalY, regionLocalX, regionLocalY);
            }
        } else if (bClick) {
            ApplyClickGesture(regionLocalX, regionLocalY, io.KeyCtrl, io.KeyShift);
        } else {
            ApplyMarqueeGesture(pressStartRegionLocalX, pressStartRegionLocalY, regionLocalX, regionLocalY,
                                io.KeyCtrl, io.KeyShift);
        }
    }
```

**`DrawMarqueeRectanglePass`'s suppression guard** (currently line 118):
```cpp
    const bool bManualDragActive = bManualMarkerDragActive || bManualPropDragActive || bManualDecalDragActive;
    if (!bPressActive || bManualDragActive || bAreaDragActive) return;
```
(otherwise the generic rubber-band box would draw simultaneously with a live Area resize/move; note
this buys the create-by-drag preview rectangle for free with zero new draw code — see §21.8's own
"Note what this buys for free" paragraph.)

**New call site in `Draw()`** — add `DrawAreaOverlayPass(regionOrigin.x, regionOrigin.y);` as a new
sibling line. Placement chosen (§21.8 names only "a new sibling line beside
`DrawOverlayIconLayerPass`/`DrawMarqueeRectanglePass`," not an exact order — this is this ticket's own
placement choice, flagged below): directly after the existing `DrawMarqueeRectanglePass(...)` call
(currently line 49) and before `DrawScenarioEditModeOverlayPass(...)` (line 51):
```cpp
    DrawOverlayIconLayerPass(regionOrigin.x, regionOrigin.y, regionSidePixels);
    DrawManualMarkerDragPass(regionOrigin.x, regionOrigin.y);
    DrawMarqueeRectanglePass(regionOrigin.x, regionOrigin.y);
    DrawAreaOverlayPass(regionOrigin.x, regionOrigin.y);          // <- new
    DrawScenarioEditModeOverlayPass(regionOrigin.x, regionOrigin.y);
```

---

## 7. New file: `src/ui/MapCanvas_AreaDraw_UI.cpp`

```cpp
// MapCanvas_AreaDraw_UI.cpp — MapCanvas::DrawAreaOverlayPass (ARCH §21.8): fill+border every area
// every frame (a deliberate v2 simplification over v1's drag-only fill, §21.8's own note — the
// always-on fill re-reads the same live, possibly-dragged rect every frame for free), the 8 handles
// for the selected area only, and hover-only cursor-shape feedback (cosmetic; never threaded through
// AreaDragGestureState).
#include "MapCanvas_UI.h"
#include "AreasTab_List_UI.h"       // ResolveAreaColor
#include "PreviewComposite_UI.h"
#include <imgui.h>

namespace SanmapGen {
namespace Ui {

void MapCanvas::DrawAreaOverlayPass(float regionOriginX, float regionOriginY) {
    if (composite == nullptr || manualAreaDrag.areas == nullptr) return;
    ImDrawList* const drawList = ImGui::GetWindowDrawList();
    const std::vector<Params::MapArea>& areas = *manualAreaDrag.areas;
    const int selectedIndex = manualAreaDrag.selectedAreaIndex != nullptr ? *manualAreaDrag.selectedAreaIndex : -1;

    auto ToScreen = [&](float worldX, float worldZ) {
        const PreviewComposite::PreviewPixelPoint pixel = composite->WorldToPreviewPixel(worldX, worldZ);
        const RegionLocalPoint local = view.ProjectPreviewPixelToRegionLocal(pixel.pixelX, pixel.pixelY);
        return ImVec2(regionOriginX + local.regionLocalX, regionOriginY + local.regionLocalY);
    };

    for (int index = 0; index < static_cast<int>(areas.size()); ++index) {
        const Params::MapArea& area = areas[static_cast<std::size_t>(index)];
        const ImVec2 nwScreen = ToScreen(area.originX, area.originZ);
        const ImVec2 seScreen = ToScreen(area.originX + area.width, area.originZ + area.length);

        const float* const color = manualAreaDrag.areaColors != nullptr
            ? ResolveAreaColor(*manualAreaDrag.areaColors, area.name) : nullptr;
        const ImU32 fillColor = color != nullptr
            ? ImGui::ColorConvertFloat4ToU32(ImVec4(color[0], color[1], color[2], color[3]))
            : ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 0.35f));
        drawList->AddRectFilled(nwScreen, seScreen, fillColor);
        drawList->AddRect(nwScreen, seScreen, ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f)));

        if (index != selectedIndex) continue;
        AreaHandleWorldPoint_UI handlePoints[8];
        ComputeAreaHandleWorldPoints(area, handlePoints);
        const ImU32 handleColor = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        for (const AreaHandleWorldPoint_UI& handlePoint : handlePoints)
            drawList->AddCircleFilled(ToScreen(handlePoint.worldX, handlePoint.worldZ),
                                      kAreaHandleScreenRadiusPixels, handleColor);
    }

    // Cursor-shape feedback: hover-only, re-hit-tested fresh against the CURRENT cursor position —
    // gated on the cursor being within the canvas region (view.RegionSidePixels(), not
    // ImGui::IsItemHovered(), since this pass runs before this frame's InvisibleButton is declared).
    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(areas.size())) return;
    const ImGuiIO& io = ImGui::GetIO();
    const float hoverRegionLocalX = io.MousePos.x - regionOriginX;
    const float hoverRegionLocalY = io.MousePos.y - regionOriginY;
    const float regionSide = view.RegionSidePixels();
    if (hoverRegionLocalX < 0.0f || hoverRegionLocalY < 0.0f
        || hoverRegionLocalX > regionSide || hoverRegionLocalY > regionSide) return;

    const AreaHandle_UI hoveredHandle = HitTestAreaHandles(areas[static_cast<std::size_t>(selectedIndex)],
                                                           *composite, view, hoverRegionLocalX, hoverRegionLocalY);
    switch (hoveredHandle) {
        case AreaHandle_UI::N: case AreaHandle_UI::S: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS); break;
        case AreaHandle_UI::E: case AreaHandle_UI::W: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW); break;
        case AreaHandle_UI::NE: case AreaHandle_UI::SW: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNESW); break;
        case AreaHandle_UI::NW: case AreaHandle_UI::SE: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE); break;
        case AreaHandle_UI::Center: ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll); break;
        default: break;
    }
}

} // namespace Ui
} // namespace SanmapGen
```

---

## 8. Modified: `src/ui/Application_UI.cpp`

Insert directly after the existing `SetManualDecalDragSource` call (currently line 142), following
`SetManualPropDragSource`/`SetManualDecalDragSource`'s own literal wiring pattern:

```cpp
    // ARCH §21.8 — the Area canvas gesture's drag source: `recipe.areas`/`tabState.areas.areaColors`/
    // `tabState.areas.selectedAreaIndex` are the SAME storage the Areas tab itself edits — one source
    // of truth, never a second copy (this function's own established posture throughout).
    canvas.SetManualAreaDragSource(&recipe.areas, &tabState.areas.areaColors,
                                   &tabState.areas.bAreasLocked, &tabState.areas.selectedAreaIndex);
```

No other file needs edits — `AreasTab_UI.h`/`.cpp`/`AreasTab_List_UI.h`/`MapArea_PARAMS.h` are
unmodified; this ticket is additive over their existing, already-ratified shape.

---

## 9. New test file: `src/ui/AreaDragGesture_UI_Test.cpp`

Pure-logic, no imgui frame, no window backend — matches `AreasTab_UI_Test.cpp`'s own style/rigor for
everything that needs no screen-space projection at all (`BeginAreaDragGesture`,
`UpdateAreaDragGesture`, `EndAreaDragGesture`, `IsWorldPointInsideArea` — every one of these takes
plain world-space floats, no `PreviewComposite`/`MapCanvasView` argument). For `HitTestAreaHandles`
specifically (the one function that needs a `PreviewComposite`/`MapCanvasView` to project through),
build the composite via `PreviewComposite_TestScene_UI.h`'s `BuildPreviewTestScene`/
`ConfigurePreviewSettings` and call `composite.ComposeOnCpu()` — the SAME no-GL, no-imgui-frame
technique `MapCanvas_Picking_UI_Test.cpp:25-30` already established for testing coordinate math
headlessly (`ComposeOnCpu()` needs no `Sys::GpuResourceManager`/live GL context at all — it is the
CPU parity twin). No `ImGui::NewFrame()`/`Draw()` call anywhere in this file.

Concrete test cases (`main()` calling one `Run*Checks()` function per group, `Check`/`failureCount`
exactly as `AreasTab_UI_Test.cpp`'s own harness):

**`RunHitTestChecks()`** (uses the `ComposeOnCpu()` fixture above; a `Params::MapArea` at
`originX=0,originZ=0,width=100,length=100` at world scale `worldUnitsPerCell=1`, projected through a
composite/view configured so 1 world unit is several preview/region pixels — enough separation that
`kAreaHandleScreenRadiusPixels` circles do not overlap):
1. A region-local point exactly at the projected NW corner resolves `AreaHandle_UI::NW` (not
   `Center`, not `N`/`W` — handles win over body, and NW's own circle wins over its neighbors' at that
   exact point).
2. A region-local point exactly at the projected N-edge midpoint resolves `AreaHandle_UI::N`.
3. A region-local point well inside the rectangle body (e.g. its center) but outside every handle
   radius resolves `AreaHandle_UI::Center`.
4. A region-local point well outside the rectangle entirely (all 8 handles and the body) resolves
   `AreaHandle_UI::None`.
5. `IsWorldPointInsideArea` (world-space, no composite needed): a point inside the rect is `true`; a
   point on the boundary is `true` (inclusive per the `>=`/`<=` spec); a point outside is `false`.

**`RunMoveChecks()`** (pure world math, no composite):
6. `BeginAreaDragGesture` with `handle=Center` at `(worldX=5,worldZ=5)` on a rect at
   `(originX=10,originZ=10,width=20,length=20)`, then `UpdateAreaDragGesture` to `(worldX=8,worldZ=9)`
   (delta `+3,+4`): confirm `area.originX==13` and `area.originZ==14`, and `width`/`length` unchanged.
7. `BeginAreaDragGesture` with an out-of-range `areaIndex` (e.g. `areas.size()`) returns `false` and
   leaves `state.bActive` false.
8. `BeginAreaDragGesture` with `handle=AreaHandle_UI::None` returns `false`.

**`RunResizeChecks()`** (pure world math):
9. Begin on handle `E` at a rect `width=20,length=10`; drag `worldX` by `+5` (no modifiers): confirm
   `width==25`, `length==10` unchanged, `originX`/`originZ` unchanged (E keeps the West edge fixed).
10. Begin on handle `W`; drag `worldX` by `-5` (i.e. moving further west, growing the rect): confirm
    `width==25` and `originX` decreased by exactly 5 (the East edge stays fixed).
11. Begin on handle `N`; drag `worldZ` by `-4`: confirm `length` grew by 4 and `originZ` decreased by
    4 (South edge fixed), `width`/`originX` unchanged.
12. Begin on handle `NE`; drag `(worldX=+6, worldZ=-3)`: confirm `width` grew by 6 (East edge, no
    Ctrl/Shift), `length` grew by 3 (North edge), `originX` unchanged, `originZ` decreased by 3.

**`RunCtrlCenterResizeChecks()`**:
13. Begin on handle `E` at a rect `originX=0,width=20` (center X = 10); drag `worldX` by `+4` with
    `bCtrlHeld=true`: confirm the extent delta was DOUBLED (`width==28`, not `24`) and the rect is
    still centered on X=10 (`originX == 10 - 28/2 == -4`).

**`RunShiftAspectLockChecks()`**:
14. Begin on handle `E` at a rect `width=20,length=10` (aspect 2.0, frozen at gesture-start); drag
    `worldX` by `+10` with `bShiftHeld=true`: confirm `width==30` and `length==15` (locked to the
    frozen 2.0 aspect from the new width), `originZ` recentered per the E-handle's own Z-centering rule.
15. Begin on handle `N` at the same rect; drag `worldZ` by `-5` with `bShiftHeld=true`: confirm
    `length==15` and `width==30` (locked from the new length), consistent with the same frozen aspect.
16. Begin on handle `NE` (a corner) at the same rect; drag `(worldX=+10, worldZ=-1)` with
    `bShiftHeld=true` (the X delta's magnitude, 10, exceeds the Z delta's magnitude, 1): confirm width
    leads (`width==30`) and length is derived from it (`length==15`), not the reverse.

**`RunMinimumFloorChecks()`**:
17. Begin on handle `W` at a rect `width=5`; drag `worldX` by `+20` (shrinking width toward/through
    zero from the West side): confirm `width` floors at exactly `kAreaMinimumExtentWorldUnits` (1.0),
    never below, and `originX` is computed from the FLOORED width (matches the West-edge-fixed rule
    using the post-floor value, not a negative pre-floor one).
18. Same shrink on handle `N` (length axis): confirms the floor applies per-axis independently — a
    simultaneous corner-handle shrink on both axes floors each one separately, not jointly.

**`RunEndChecks()`**:
19. `EndAreaDragGesture` on an active state clears `bActive` to `false` and resets `areaIndex`/`handle`
    to their default-constructed values (`-1`/`AreaHandle_UI::None`) — confirms it performs no vector
    mutation of its own (a caller that never called `UpdateAreaDragGesture` at all still leaves
    `areas` completely untouched by `Begin`+`End` alone).

Header comment convention: mirror `AreasTab_UI_Test.cpp:8`'s own `// NOT YET REGISTERED IN CMake —
[...] gate registers it` caveat verbatim (registration is a gate-owned step, not authored by this
ticket, per that file's own established precedent — see §10 below for the actual line to add when
that gate runs).

---

## 10. CMakeLists.txt

`src/ui/*.cpp`/`*.h` are already covered by the existing `file(GLOB_RECURSE ... CONFIGURE_DEPENDS
"src/ui/*.cpp" "src/ui/*.h")` (`CMakeLists.txt:177-183`) that builds the `SanGenV2` library — the four
new production files (`AreaDragGesture_UI.h`/`.cpp`, `MapCanvas_AreaDragDispatch_UI.cpp`,
`MapCanvas_AreaDraw_UI.cpp`) need **no CMakeLists.txt edit** to be compiled into the library.

Only the new **test executable** needs explicit registration, exactly like every other single-file
pure-logic UI test (`CMakeLists.txt:865`'s own `add_sangen_test(AreasTab_UI_Test
src/ui/AreasTab_UI_Test.cpp)` is the literal pattern to copy — note that file's own header flags
"NOT YET REGISTERED IN CMake" yet it IS registered at line 865 today, confirming that flag is
historical/gate-owned and does not block the coder from adding the line directly if the gate has
already opened for this ticket's own PR). Add, alongside the other Areas-adjacent test targets:
```cmake
add_sangen_test(AreaDragGesture_UI_Test src/ui/AreaDragGesture_UI_Test.cpp)
```

---

## ARCH rules invoked
- `ARCH_21_08_AreaCanvasGesture.md` (§21.8) — this ticket's entire binding law; every algorithmic
  decision above traces to a specific ruling or correction in that file.
- `ARCH_21_CanvasInteractionUnification.md` (§21 index) — confirms §21.8 is independently
  dispatchable, and names its one real dependency (§21.2's press/release skeleton).
- Constitution §1 — UI sets PARAMS, never simulates; every write in this ticket lands in
  `recipe.areas` via a plain field assignment.
- Constitution §6 — an index (`areaIndex`, `selectedAreaIndex`) is always range-checked before
  dereference; a cursor/drag input is untrusted (mirrors `SelectedArea`'s own existing discipline,
  `AreasTab_UI.h:73-76`).
- Constitution §8 — `kAreaHandleScreenRadiusPixels`/`kAreaMinimumExtentWorldUnits` are named
  constants, not literals, per §21.8's own explicit ruling on both.

## Explicit out-of-scope
- **No `Params::Geometry` dependency anywhere in this ticket's new files** — §21.8 correction 3
  confirms neither the click/drag chain nor the gesture math needs it; `AreaOriginSliderRange`/
  `AreaExtentSliderRange`'s own `recipe.geometry.mapSize` use is a TAB-only slider-fencing concern,
  untouched by this ticket, and this ticket's canvas gesture fences neither origin nor extent to the
  map size (deliberate, v1-parity non-requirement, not an oversight).
- **No per-area lock field, no `Traits` contract, no `PlacementCollectionKind_UI` entry, no
  `OverlayInstanceKeySet_UI`/multi-select participation for Areas** — §21.8 corrections 1/5, all
  confirmed structurally absent from `Params::MapArea`/the existing selection machinery.
- **No change to `AreasTab_UI.h`/`.cpp`/`AreasTab_List_UI.h`/`MapArea_PARAMS.h`** — this ticket is
  strictly additive over their already-ratified, unmodified shape.
- **No `PreviewDriver`/recomposite-trigger wiring of any kind** — Areas feeds no generation stage;
  `PreviewDriver` already derives a recomposite-only signal from `recipe.areas` elsewhere (`AreasTab_
  UI.h` SCOPE NOTE 1), and this ticket's canvas writes are read live by the new draw pass every frame,
  needing no notification of its own (mirrors `MapCanvas_UI.h:264-268`'s existing "no stopgap draw
  needed" reasoning for Props/Decals).
- **No rotation, no per-area icon/label editing beyond the existing name-in-center convention** — not
  ratified by §21.8, not built here.

## Acceptance test (end-to-end, in addition to §9's unit coverage)
1. With the Areas panel active and `bAreasLocked=false`, a press-drag-release on empty canvas space
   creates exactly one new `Params::MapArea` in `recipe.areas`, auto-selected
   (`tabState.areas.selectedAreaIndex` points at it), named via `NextAreaName`, with a unique name
   (confirm via a second create when one `NewArea0` already exists — the second is `NewArea1`, not a
   duplicate `MakeNamesUnique` would otherwise have to repair post hoc).
2. A press-drag on one of the selected area's 8 handles resizes it live, every `ContinueAreaDrag`
   frame (not only at release) — confirm by reading `recipe.areas[index]` mid-drag (before release),
   matching the "no commit-on-release for the data" ruling.
3. A press-drag on the selected area's body (not a handle) moves it; a press-drag on a body-DIFFERENT
   area's rectangle re-selects that area (`selectedAreaIndex` changes) and begins its own move in the
   same press.
4. An empty-space click (zero travel) while the Areas panel is active deselects
   (`selectedAreaIndex == -1`); it does NOT fall through to `ApplyClickGesture`/`ApplyMarqueeGesture`.
5. With `bAreasLocked=true`, every one of the above (create, handle-resize, body-move, and even
   plain click-to-select) is refused uniformly — the canvas surface produces zero effect, but
   selecting a different area through the Area Stack list (`DrawAreaList`) still works.
6. With the Markers/Props/Decals/any-other panel active (not Areas), a drag on former-area screen
   space produces the SAME behavior as before this ticket (falls through to ordinary
   click/marquee/manual-instance-drag resolution) — `AreaGestureEligible()`'s panel gate is airtight.
7. `MapCanvas::Draw()` renders every area's fill+border every frame regardless of drag state
   (including a freshly-created, in-progress create-by-drag rectangle, drawn via the existing
   `DrawMarqueeRectanglePass` for free) and renders the 8 handle circles only for the selected area.
8. Full `SanGenV2` build stays clean; every existing test continues to pass; the new
   `AreaDragGesture_UI_Test` binary passes with `ALL PASS`.

---

## Interpretation calls made beyond §21.8's ratified text

§21.8's own pseudocode/prose was unusually complete, but a few implementation-level details were left
open (not architectural decisions — none of these change behavior a human would notice or that ARCH
ruled on) and had to be picked to make the ticket buildable mechanically. Flagging all of them:

1. **Hit-test radius formula.** v1's own code (`Widget_AreaEditor.cpp:86`) used
   `cornerRadius*cornerRadius*2.0f` as its threshold ("slightly larger hit area," per its own
   comment) — a hidden `*2.0` fudge factor on top of the named radius. §21.8's text says only
   "compared against regionLocalX/Y within `kAreaHandleScreenRadiusPixels`," without re-stating that
   multiplier as part of the ratified constant. This work-order specifies a plain `radius*radius`
   comparison (no hidden multiplier), consistent with Constitution §8's "named constant, not a hidden
   literal" spirit — a slightly smaller hit circle than v1's, not re-litigated by ARCH.
2. **Reusing the already-projected NW/SE corners for the Center/body screen-space test inside
   `HitTestAreaHandles`**, rather than a separate call, is a one-pass-efficiency factoring choice —
   behaviorally identical either way.
3. **Factoring `ComputeAreaHandleWorldPoints` as a shared helper**, called by both `HitTestAreaHandles`
   and the draw pass' handle-circle rendering, so the 8-point derivation exists in exactly one place.
   This is a function inside an already-ratified file, not a new file, so it doesn't add to §21.8's
   "New files this section ratifies" list, but it's still a factoring choice beyond what ARCH specified.
4. **`DrawAreaOverlayPass`'s exact call-site position within `Draw()`** — §21.8 says only "a new
   sibling line beside `DrawOverlayIconLayerPass`/`DrawMarqueeRectanglePass`," not an exact order. This
   work-order places it directly after `DrawMarqueeRectanglePass` and before
   `DrawScenarioEditModeOverlayPass`.
5. **Cursor-shape hover-gating via `view.RegionSidePixels()` bounds-checking**, instead of
   `ImGui::IsItemHovered()` — because `DrawAreaOverlayPass` runs before this frame's `InvisibleButton`
   is declared, so `IsItemHovered()` at that point would read the *previous* item (the `Image` call),
   which is timing-fragile. The view's own already-tracked region size is used instead.
6. **The new test file's fixture technique for `HitTestAreaHandles`** — `PreviewComposite` +
   `ComposeOnCpu()` (no GL, no imgui frame; the same technique `MapCanvas_Picking_UI_Test.cpp:25-30`
   already established), rather than the fully zero-dependency purity `AreasTab_UI_Test.cpp`'s other
   functions enjoy (those need no composite/view at all, since they're pure world-space math) — a
   necessary, explained deviation for the one function that must project through screen space, not a
   relaxation of the "no GL context" requirement.
7. **Two defensive additions in `AreaDragGesture_UI.cpp`** beyond a literal v1 port: `BeginAreaDragGesture`
   explicitly refuses an out-of-range `areaIndex`/a `None` handle (v1's own call sites can't reach that
   case, but this port's callers aren't gated identically), and `aspectLockRatio`'s divide is guarded
   against zero (v1 divides unguarded). Both are called out in the work-order text itself, not silent.

## Key files read/cited while drafting
`D:\Projects\Sanctuary\Map Generator\ARCH_21_08_AreaCanvasGesture.md`,
`D:\Projects\Sanctuary\Map Generator\ARCH_21_CanvasInteractionUnification.md`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_Draw_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_ManualDragDispatch_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_SelectionGesture_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_ManualDragSources_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\InstanceDragGesture_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PropDragGesture_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvasView_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_Prepare_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\AreasTab_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\AreasTab_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\AreasTab_List_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\AreasTab_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\Application_UI.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\Application_Panels_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\Application_TabState_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_Picking_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\MapCanvas_GestureOwnership_UI_Test.cpp`,
`D:\Projects\Sanctuary\Map Generator\src\ui\PreviewComposite_TestScene_UI.h`,
`D:\Projects\Sanctuary\Map Generator\src\params\MapArea_PARAMS.h`,
`D:\Projects\Sanctuary\Map Generator\gui\widgets\Widget_AreaEditor.h`,
`D:\Projects\Sanctuary\Map Generator\gui\widgets\Widget_AreaEditor.cpp`,
`D:\Projects\Sanctuary\Map Generator\CMakeLists.txt`,
and the `work_orders\STEP94_MarkerDragAndFollowSymmetry_UI.md` / `work_orders\STEP205_ManualMarkerMultiSelectClobberFix_UI.md` precedent tickets used for this document's own structure/rigor.
