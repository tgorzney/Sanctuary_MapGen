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
