// MapCanvas_MarkerDrag_UI.cpp — MapCanvas's own gesture-lifecycle method definitions (declared in
// MapCanvas_UI.h). Split from MapCanvas_MarkerHitTest_UI.cpp/MapCanvas_MarkerRosterDraw_UI.cpp
// (STEP126, ceiling remediation) — this file now owns lifecycle only, those own hit-test/draw.
#include "MapCanvas_MarkerDrag_UI.h"
#include "MapCanvas_UI.h"
#include "MarkerSelectionHighlight_UI.h"   // NEW — STEP126
#include "PreviewComposite_UI.h"
#include "../params/MapRecipe_PARAMS.h"
#include <imgui.h>

namespace SanmapGen {
namespace Ui {

bool MapCanvas::TryBeginManualMarkerDrag(float regionLocalX, float regionLocalY) {
    if (manualMarkerDragMarkers == nullptr || manualMarkerDragGeometry == nullptr
        || manualMarkerDragRecipe == nullptr || composite == nullptr) return false;
    // STEP113 — a drag may only BEGIN while the Markers panel is active. Guard-clause negated-OR
    // form, matching this function's OWN existing null-check style immediately above (not
    // DrawScenarioEditModeOverlayPass's positive "!= nullptr && ->IsActive()" gate-and-proceed
    // form) — both are the same null-safety posture, applied as the shape each call site already
    // uses. Null (no shell has wired a panel source, e.g. a test harness) refuses, never defaults
    // to permitting a drag — same null-safe-refuses posture as the existing scenarioEditModeState
    // pointer (MapCanvas_UI.h:173), not a new convention.
    if (activePanelSource == nullptr || *activePanelSource != ApplicationPanel::Markers) return false;
    int hitGroupIndex = -1, hitTransformIndex = -1;
    if (!HitTestManualMarkers(*manualMarkerDragMarkers, *composite, view, regionLocalX, regionLocalY,
                              pickRadiusScreenPixels, hitGroupIndex, hitTransformIndex))
        return false;
    static const std::vector<Params::MarkerInstanceLayer> kNoLayers;
    return BeginMarkerDragGesture(manualMarkerDragState, *manualMarkerDragMarkers,
                                  manualMarkerDragLayers != nullptr ? *manualMarkerDragLayers : kNoLayers,
                                  *manualMarkerDragGeometry, manualMarkerDragRecipe->globalSymmetryMask,
                                  manualMarkerDragRecipe->radialSymmetryRepeatCount,
                                  hitGroupIndex, hitTransformIndex);
}

void MapCanvas::ContinueManualMarkerDrag(float regionLocalX, float regionLocalY) {
    if (manualMarkerDragMarkers == nullptr || manualMarkerDragGeometry == nullptr || composite == nullptr) return;
    const PreviewPixelCoordinate previewPixel = view.ResolvePreviewPixel(regionLocalX, regionLocalY);
    const PreviewComposite::PreviewWorldPoint worldPoint = composite->PreviewPixelToWorld(
        static_cast<float>(previewPixel.pixelX), static_cast<float>(previewPixel.pixelY));
    static const std::vector<Params::MarkerInstanceLayer> kNoLayers;
    UpdateMarkerDragGesture(manualMarkerDragState, *manualMarkerDragMarkers,
                           manualMarkerDragLayers != nullptr ? *manualMarkerDragLayers : kNoLayers,
                           *manualMarkerDragGeometry, worldPoint.worldX, worldPoint.worldZ);
}

void MapCanvas::EndManualMarkerDrag() {
    if (manualMarkerDragMarkers == nullptr || manualMarkerDragGeometry == nullptr) {
        manualMarkerDragState = MarkerDragGestureState{};
        return;
    }
    EndMarkerDragGesture(manualMarkerDragState, *manualMarkerDragMarkers, *manualMarkerDragGeometry);
}

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
