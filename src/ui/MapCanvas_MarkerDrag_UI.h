// MapCanvas_MarkerDrag_UI.h — the one imgui-including translation unit for STEP94: the linear
// manual-marker hit-test (Gap 5's routing-around Picking_UI::PickMarker/Data::SpatialGrid, which
// only ever see Data::PlacementInstances — manual markers never enter that buffer) and the minimal
// stopgap manual-marker draw (Gap 6 — manual markers have no rendering consumer of any kind before
// this ticket). Layer: UI. Kept separate from MapCanvas_Draw_UI.cpp (pan/zoom/click routing stays
// that file's one job) and from MarkerDragGesture_UI/MarkerOrbitCorrespondence_UI (pure logic, no
// imgui) — the same `Prepare`/`Cpu`/`Gpu`-style one-job-per-file split PreviewComposite_*_UI.cpp
// already establishes.
#pragma once
#include <vector>
#include "MarkerDragGesture_UI.h"
#include "MapCanvasView_UI.h"
#include "../params/MarkerInstance_PARAMS.h"

struct ImDrawList;

namespace SanmapGen {
namespace Ui {

class PreviewComposite;

// Nearest manual marker (any group) within `pickRadiusScreenPixels` of the region-local cursor —
// projected via STEP47's `PreviewComposite::WorldToPreviewPixel` + `MapCanvasView::
// ProjectPreviewPixelToRegionLocal`, exactly as `MapCanvas_ScenarioEditMode_HitTest_UI.cpp` already
// composes the same pair. O(manual marker count) — legitimate at "tens, not tens of thousands"
// (STEP49's own sizing note); NOT `Picking_UI::PickMarker`/`Data::SpatialGrid`, which operate only
// over `Data::PlacementInstances` (Gap 5 — manual markers have no presence there). Ties keep the
// first (lowest group, then lowest transform) index. Answers false (both out-params left at -1) for
// an unbaked composite, an empty roster, or no marker within radius.
bool HitTestManualMarkers(const std::vector<Params::MarkerInstanceGroup>& markers,
                          const PreviewComposite& composite, const MapCanvasView& view,
                          float regionLocalX, float regionLocalY, float pickRadiusScreenPixels,
                          int& outGroupIndex, int& outTransformIndex);

// The deliberately-minimal at-rest + ghost/refused-tint draw (Gap 6) — plain `AddCircleFilled`
// dots, one per `MarkerTransform`, tinted by its layer's color (or a neutral default). A gesture's
// own soft-hidden siblings are skipped (not erased, just not drawn); its unclaimed orbit slots draw
// as a distinct hollow ghost ring; a Spawn-refused gesture's whole group tints red and a short
// status tooltip is drawn. Superseded outright by a future real overlay/icon ticket — not
// `OverlayLayer_UI`/View-toolbar participation of any kind (ARCH_14_PreviewOverlayLayering.md §14).
void DrawManualMarkerRoster(const std::vector<Params::MarkerInstanceGroup>& markers,
                            const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                            const MarkerDragGestureState& dragState, const PreviewComposite& composite,
                            const MapCanvasView& view, float regionOriginX, float regionOriginY,
                            ImDrawList& drawList);

} // namespace Ui
} // namespace SanmapGen
