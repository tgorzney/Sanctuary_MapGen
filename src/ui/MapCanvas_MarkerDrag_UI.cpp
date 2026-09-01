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
    static const std::vector<Params::MarkerLink> kNoLinks;
    static const Params::GlobalMarkerSettings kDefaultGlobalMarkerSettings;
    static const Params::MarkerSymmetryFixSettings kDefaultMarkerSymmetryFixSettings;
    // STEP246, ARCH §19.33/§21.9 — reuses the already-threaded manualMarkerDragRecipe pointer
    // (globalSymmetryMask/radialSymmetryRepeatCount already source from it just below); no new
    // injected field needed on MapCanvas_UI.h.
    const std::vector<Params::MarkerLink>& markerLinks =
        manualMarkerDragRecipe != nullptr ? manualMarkerDragRecipe->markerLinks : kNoLinks;
    // STEP231 — sourced directly from THIS class's own canonical multi-select set
    // (selectedInstanceKeys, ARCH §21.1, MapCanvas_UI.h:377), not the retired injected single-scalar
    // pointer (SetManualMarkerSelectionSource/manualMarkerSelectedInstanceIdentifier — see this
    // ticket's own Interpretation calls for why: that mechanism predates §21.1's ordered set and had
    // become a stale second copy of data this class already owns as ground truth, the exact "one
    // source of truth, never a second copy" principle every OTHER canvas.Set*Source call already
    // follows). Every manually-selected marker's own symmetry orbit is unioned
    // (ComputeManualMarkerMultiSelectionHighlight), not just the MRU primary's.
    std::vector<int> selectedManualInstanceIdentifiers;
    for (const OverlayInstanceKey_UI& key : selectedInstanceKeys.keys)
        if (key.bValid && key.collection == PlacementCollectionKind_UI::Markers && key.bManual)
            selectedManualInstanceIdentifiers.push_back(key.instanceIndex);
    const std::vector<int> selectedHighlight = (manualMarkerDragGeometry != nullptr)
        ? ComputeManualMarkerMultiSelectionHighlight(*manualMarkerDragMarkers,
              manualMarkerDragLayers != nullptr ? *manualMarkerDragLayers : kNoLayers, *manualMarkerDragGeometry,
              manualMarkerDragRecipe != nullptr ? manualMarkerDragRecipe->globalSymmetryMask : 0,
              manualMarkerDragRecipe != nullptr ? manualMarkerDragRecipe->radialSymmetryRepeatCount : 0,
              manualMarkerDragRecipe != nullptr ? manualMarkerDragRecipe->markerSymmetryFixSettings.distanceTolerance
                                                 : kDefaultMarkerSymmetryFixSettings.distanceTolerance,
              selectedManualInstanceIdentifiers)
        : std::vector<int>{};
    DrawManualMarkerRoster(*manualMarkerDragMarkers, manualMarkerDragLayers != nullptr ? *manualMarkerDragLayers : kNoLayers,
                          manualMarkerDragRecipe != nullptr ? manualMarkerDragRecipe->armies : kNoArmies,
                          manualMarkerDragRecipe != nullptr ? manualMarkerDragRecipe->globalMarkerSettings : kDefaultGlobalMarkerSettings,
                          manualMarkerDragState, *composite, view, regionOriginX, regionOriginY,
                          selectedHighlight, markerLinks, *ImGui::GetWindowDrawList());
}

} // namespace Ui
} // namespace SanmapGen
