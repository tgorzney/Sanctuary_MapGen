// MapCanvas_MarkerDrag_UI.cpp — MapCanvas's own Marker-specific draw-pass definition (declared in
// MapCanvas_UI.h). The gesture-LIFECYCLE methods that used to live here (TryBeginManualMarkerDrag/
// ContinueManualMarkerDrag/EndManualMarkerDrag) are superseded by the generalized 3-way dispatcher
// (TryBeginManualInstanceDrag/ContinueManualInstanceDrag/EndManualInstanceDrag,
// MapCanvas_ManualDragDispatch_UI.cpp, ARCH §21.2/§21.3) — this file now owns the Markers-only
// stopgap draw pass alone. Split from MapCanvas_MarkerHitTest_UI.cpp/MapCanvas_MarkerRosterDraw_UI.cpp
// (STEP126, ceiling remediation) originally.
#include "MapCanvas_MarkerDrag_UI.h"
#include "MapCanvas_UI.h"
#include "MarkerSelectionHighlight_UI.h"   // NEW — STEP126
#include "PreviewComposite_UI.h"
#include "../params/MapRecipe_PARAMS.h"
#include <imgui.h>

namespace SanmapGen {
namespace Ui {

void MapCanvas::DrawManualMarkerDragPass(float regionOriginX, float regionOriginY) {
    if (composite == nullptr || manualMarkerDragMarkers == nullptr) return;
    static const std::vector<Params::MarkerInstanceLayer> kNoLayers;
    static const std::vector<Params::Army> kNoArmies;
    static const Params::GlobalMarkerSettings kDefaultGlobalMarkerSettings;
    static const Params::MarkerSymmetryFixSettings kDefaultMarkerSymmetryFixSettings;
    // STEP126 — recomputed fresh every frame (ARCH §19.19), discarded after this draw call. Null-safe:
    // no selection source wired -> -1 -> ComputeManualMarkerSelectionHighlight returns empty.
    const std::vector<int> selectedHighlight = (manualMarkerDragGeometry != nullptr)
        ? ComputeManualMarkerSelectionHighlight(*manualMarkerDragMarkers,
              manualMarkerDragLayers != nullptr ? *manualMarkerDragLayers : kNoLayers, *manualMarkerDragGeometry,
              manualMarkerDragRecipe != nullptr ? manualMarkerDragRecipe->globalSymmetryMask : 0,
              manualMarkerDragRecipe != nullptr ? manualMarkerDragRecipe->radialSymmetryRepeatCount : 0,
              manualMarkerDragRecipe != nullptr ? manualMarkerDragRecipe->markerSymmetryFixSettings.distanceTolerance
                                                 : kDefaultMarkerSymmetryFixSettings.distanceTolerance,
              manualMarkerSelectedInstanceIdentifier != nullptr ? *manualMarkerSelectedInstanceIdentifier : -1)
        : std::vector<int>{};
    DrawManualMarkerRoster(*manualMarkerDragMarkers, manualMarkerDragLayers != nullptr ? *manualMarkerDragLayers : kNoLayers,
                          manualMarkerDragRecipe != nullptr ? manualMarkerDragRecipe->armies : kNoArmies,
                          manualMarkerDragRecipe != nullptr ? manualMarkerDragRecipe->globalMarkerSettings : kDefaultGlobalMarkerSettings,
                          manualMarkerDragState, *composite, view, regionOriginX, regionOriginY,
                          selectedHighlight, *ImGui::GetWindowDrawList());
}

} // namespace Ui
} // namespace SanmapGen
