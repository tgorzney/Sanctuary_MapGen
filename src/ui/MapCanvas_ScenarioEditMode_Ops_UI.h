// MapCanvas_ScenarioEditMode_Ops_UI.h — MapCanvas_ScenarioEditMode_UI.h's interaction/commit/draw
// entry points, split out (Constitution §1.5 ceiling) exactly as MapCanvas_IconLayer_Ops_UI.h
// splits out of MapCanvas_IconLayer_UI.h. `struct ImDrawList` is only NAMED here (forward-declared)
// so MapCanvas_UI.h, which includes this for the setters, never sees imgui.h.
#pragma once
#include "MapCanvas_ScenarioEditMode_State_UI.h"

struct ImDrawList;

namespace SanmapGen {
namespace Params { struct Army; }
namespace Ui {

class PreviewComposite;
class MapCanvasView;
class IconAtlasPairingLookup;
struct IconAtlasManifest;

// _Interaction_UI.cpp — pure (imgui-free) pointer-gesture application, reusing STEP47's own
// world<->screen projection (MapCanvasView::ResolvePreviewPixel + PreviewComposite::
// PreviewPixelToWorld, composed exactly as MapCanvas::ApplyClick already does — mirrored, not
// reinvented).
struct ScenarioEditModePointerFrame_UI {
    float regionLocalX = 0.0f, regionLocalY = 0.0f;   // current cursor, region-local
    bool  bPressActivated = false;   // this frame is the press-down frame
    bool  bPressActive    = false;   // held down (a drag candidate)
    bool  bRightClicked   = false;
};
// A screen-pixel radius a press must land within to hit a candidate icon — Constitution §8, mirrors
// MapCanvas::pickRadiusScreenPixels's own named-setting posture (never a literal).
inline constexpr float kScenarioEditModeHitRadiusScreenPixels = 10.0f;

// Applies one frame's pointer gesture against `state.lastResolvedCandidates` (already resolved by
// this same frame's draw call — see MapCanvas_ScenarioEditMode_UI.h's own note). Mutates
// `state.editedBody` directly on a spawn drag/materialize, exactly like every ScenariosTab_*_UI.cpp
// widget already mutates `Params::ScenarioBody` in place (PARAMS sits downward of UI, ARCH §3). A
// right-click that resolves an actionable target sets `state.pendingContextMenu` +
// `state.bContextMenuJustRequested` for the chrome pass's popup to pick up the SAME frame.
void ApplyScenarioEditModePointerInput(ScenarioEditModeState& state, const MapCanvasView& view,
                                       const PreviewComposite& composite,
                                       const std::vector<Params::Army>& armies,
                                       const ScenarioEditModePointerFrame_UI& pointerFrame);

// _Commit_UI.cpp — whether "Remove for this scenario" / "Add Alloy Marker for Army" may actually
// commit for the given alloyMode, and why not when they cannot — never a silently-disabled item
// with no explanation (STEP78 acceptance test 3), and never a silent no-op click either.
bool CanRemoveBaselineAlloyForScenario(Params::ScenarioAlloyMode alloyMode);
const char* RemoveBaselineAlloyDisabledReason(Params::ScenarioAlloyMode alloyMode);
bool CanAddAlloyMarkerForScenario(Params::ScenarioAlloyMode alloyMode);
const char* AddAlloyMarkerDisabledReason(Params::ScenarioAlloyMode alloyMode);
// Commits a pending context-menu request — callers MUST gate on the Can*() pair above first; an
// ungated call for a mode that cannot express the action is a documented no-op, not a crash.
void CommitScenarioEditModeContextMenu(Params::ScenarioBody& body,
                                       const ScenarioEditModeState::ContextMenuRequest& request);

// _DrawMarkers_UI.cpp — resolves this frame's candidates into `state.lastResolvedCandidates` and
// draws each: §14.9's bulk-vertex-write technique for the atlas icon quad (when a pairing/atlas
// source resolves one), plus plain ImDrawList primitives for the state decorations (hollow/dashed
// outline, warning badge, grey strike, "+" badge, ghost red X) no atlas quad alone could carry —
// Backend policy: the SAME PrimReserve/PrimWriteVtx/PrimWriteIdx technique FlushIconLayerBucket
// uses, not a second backend (STEP53's own precedent), reimplemented locally rather than widening
// that pair's own restricted-scope internal header because this module additionally needs a full
// per-quad RGBA tint (army color) FlushIconLayerBucket's alpha-only tint cannot express.
struct ScenarioEditModeDrawInput {
    ScenarioEditModeResolveInput  resolveInput;
    const PreviewComposite*       composite    = nullptr;
    const MapCanvasView*          view         = nullptr;
    const IconAtlasPairingLookup* pairingLookup = nullptr;
    const IconAtlasManifest*      atlasManifest = nullptr;
    float regionOriginX = 0.0f, regionOriginY = 0.0f;
};
void DrawScenarioEditModeOverlay(ScenarioEditModeState& state, const ScenarioEditModeDrawInput& input,
                                 ImDrawList& drawList);

// _Chrome_UI.cpp — the legend strip + "Preview As" toggle row (drawn OUTSIDE the canvas image
// region, Application_Draw_UI.cpp's own call site) and the right-click context-menu popup (drawn
// INSIDE the canvas's own imgui scope, called from MapCanvas_Draw_UI.cpp right after
// DrawScenarioEditModeOverlay above so the popup opens the SAME frame the request was resolved).
void DrawScenarioEditModeChrome(ScenarioEditModeState& state, const std::vector<Params::Army>& armies,
                                int maxArmySlotCount);
void DrawScenarioEditModeContextMenuPopup(ScenarioEditModeState& state);

} // namespace Ui
} // namespace SanmapGen
