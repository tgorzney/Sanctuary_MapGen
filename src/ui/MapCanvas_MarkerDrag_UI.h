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
#include "../params/Army_PARAMS.h"
#include "../params/GlobalMarkerSettings_PARAMS.h"
#include "../params/MarkerLink_PARAMS.h"

struct ImDrawList;

namespace SanmapGen {
namespace Ui {

class PreviewComposite;
struct OverlayLayerSettings;   // BUGFIX_OverlayVisibilityAndPropIconFallback_R1 — see below

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
// dots, one per `MarkerTransform`, tinted by its layer's color override, its group's type-default
// color, or a neutral default. A gesture's own soft-hidden siblings are skipped (not erased, just
// not drawn); its unclaimed orbit slots draw as a distinct hollow ghost ring; a Spawn-refused
// gesture's whole group tints red and a short status tooltip is drawn.
// BUGFIX_OverlayVisibilityAndPropIconFallback_R1, Part 1 — `overlayLayerSettings` (new, nullable,
// trailing/defaulted parameter, last below) is consulted per-group (IsMarkerGroupDomainVisible,
// MapCanvas_MarkerRosterDraw_UI.cpp's own anonymous namespace) so a group whose View-toolbar row is
// toggled off (`OverlayLayer_UI::bEnabled`) is skipped here too, exactly like the gated
// DrawOverlayIconLayerPass (STEP53) already honors that same state — this pass used to draw
// unconditionally, painting every marker on top regardless of the toggle (the "dead STEP94 stopgap"
// bug). `nullptr` (no source wired — every pre-ticket call site, including this file's own tests)
// means "unfiltered," byte-identical to the pre-ticket behavior; never a regression for a caller
// that does not care about visibility gating. Still NOT `OverlayLayer_UI`/View-toolbar
// PARTICIPATION of any other kind (ARCH_14_PreviewOverlayLayering.md §14) — this is visibility-only.
// STEP126: `selectedHighlightInstanceIdentifiers` is this frame's ComputeManualMarkerSelectionHighlight
// result — every instanceIdentifier that should draw with the select tint (ARCH §19.18), highest
// priority after refused-drag-red. Empty = nothing selected, no highlight branch taken.
// Follow-up to BUGFIX_UniversalCoordinateConversionAndDragRewrite_UI — widened from a single
// `const MarkerDragGestureState& dragState` to `dragStates`: one entry per instance currently being
// dragged this gesture (a multi-select drag carries one `InstanceDragGestureState` per grabbed
// instance, MapCanvas_UI.h's own `manualMarkerDragEntries`). A single-element vector is
// byte-identical to the pre-widening one-state call: every instance's own ghost/soft-hide/refused
// feedback now shows, not just the first-grabbed instance's.
void DrawManualMarkerRoster(const std::vector<Params::MarkerInstanceGroup>& markers,
                            const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                            const std::vector<Params::Army>& armies,
                            const Params::GlobalMarkerSettings& globalMarkerSettings,
                            const std::vector<MarkerDragGestureState>& dragStates, const PreviewComposite& composite,
                            const MapCanvasView& view, float regionOriginX, float regionOriginY,
                            const std::vector<int>& selectedHighlightInstanceIdentifiers,
                            const std::vector<Params::MarkerLink>& markerLinks,   // NEW — STEP246
                            ImDrawList& drawList,
                            const OverlayLayerSettings* overlayLayerSettings = nullptr);   // NEW — see above

} // namespace Ui
} // namespace SanmapGen
