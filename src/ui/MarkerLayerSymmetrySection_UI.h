// MarkerLayerSymmetrySection_UI.h — the "Layer Symmetry" section body drawn inside each Manual
// Marker Layer row (MarkersTab_ManualLayers_UI.cpp's DrawLayerRowBody): the row's own symmetry-axes
// control plus STEP107's "Fix Symmetry" backfill command (tolerance slider, overwrite checkbox,
// button, result line). Layer: UI. Split out of MarkersTab_ManualLayers_UI.cpp purely to keep that
// file under the ARCH §1.5 150-line ceiling — no behavior change, same posture as
// MarkerLayerIndexRepair_UI.h's own split off the same host file.
#pragma once
#include <vector>
#include "MarkersTab_ManualLayers_UI.h"
#include "../params/Geometry_PARAMS.h"
#include "../params/MarkerInstance_PARAMS.h"
#include "../params/Symmetry_PARAMS.h"

namespace SanmapGen {
namespace Ui {

// Draws the whole "Layer Symmetry" DrawSectionBegin/DrawSectionEnd block for one row: the existing
// symmetry-axes control, then (directly after it) STEP107's Fix Symmetry command. `layerIndex` is
// the row's own index into `markerLayers` (see MarkersTab_ManualLayers_UI.cpp's DrawLayerRowBody);
// `markerLayers`/`markers`/`geometry`/`globalSymmetryMask`/`globalRadialRepeatCount`/
// `markerSymmetryFixSettings` are `Ui::FixMarkerLayerSymmetry`'s own required inputs, threaded down
// from `DrawManualMarkerLayers`.
void DrawLayerSymmetrySection(Params::MarkerInstanceLayer& layer, int layerIndex,
                              const std::vector<Params::MarkerInstanceLayer>& markerLayers,
                              std::vector<Params::MarkerInstanceGroup>& markers,
                              const Params::Geometry& geometry, int globalSymmetryMask,
                              int globalRadialRepeatCount,
                              Params::MarkerSymmetryFixSettings& markerSymmetryFixSettings,
                              ManualMarkerLayersState& state);

} // namespace Ui
} // namespace SanmapGen
