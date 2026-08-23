// MapCanvas_ScenarioEditMode_InteractionInternal_UI.h — declarations shared by
// MapCanvas_ScenarioEditMode_HitTest_UI.cpp and MapCanvas_ScenarioEditMode_Interaction_UI.cpp
// ONLY (Constitution §1.5 ceiling split, mirrors MapCanvas_IconLayer_CullInternal_UI.h's own
// posture); not part of this module's public surface.
#pragma once
#include "MapCanvas_ScenarioEditMode_Ops_UI.h"

namespace SanmapGen {
namespace Ui {

class PreviewComposite;
class MapCanvasView;

// Linear screen-rect hit test (cardinality tens — STEP78's own explicit "no SpatialGrid" flag).
// Nearest candidate within kScenarioEditModeHitRadiusScreenPixels, or kScenarioEditModeNoIndex.
int HitTestScenarioEditModeCandidates(const std::vector<ScenarioEditMarkerCandidate_UI>& candidates,
                                      const PreviewComposite& composite, const MapCanvasView& view,
                                      float regionLocalX, float regionLocalY);

// The world point under a region-local cursor — STEP47's own inverse projection, composed exactly
// as MapCanvas::ApplyClick already does.
float ResolveScenarioEditModeWorldXUnderCursor(const PreviewComposite& composite, const MapCanvasView& view,
                                               float regionLocalX, float regionLocalY, float& outWorldZ);

float ScenarioEditModeDistanceSquared(float ax, float ay, float bx, float by);

} // namespace Ui
} // namespace SanmapGen
