// MarkerSymmetryFixCommand_UI.h — the "Fix Symmetry" button's command body (STEP107). Layer: UI.
// Pure, imgui-free — same testable-with-no-window posture as MarkerDragGesture_UI.h. Performs the
// ONLY PARAMS writes in this ticket: Pipeline::FindMarkerSymmetryMatches (read-only) supplies which
// candidates form confirmed orbits; this function allocates fresh symmetryGroupIdentifier values and
// writes them, per ARCH's "UI sets PARAMS, PIPELINE/PROC never do" rule (Constitution §1).
#pragma once
#include <vector>
#include "../params/Geometry_PARAMS.h"
#include "../params/MarkerInstance_PARAMS.h"

namespace SanmapGen {
namespace Ui {

struct MarkerSymmetryFixResult {
    int confirmedGroupCount = 0;   // fresh symmetryGroupIdentifier values allocated this run
    int unmatchedSlotCount  = 0;   // Pipeline::FindMarkerSymmetryMatches's own unmatched-slot total
};

// `markers` is `recipe.markers` (mutated). `layerIndex` is the target layer — the row this command
// was invoked from (see STEP107 §1/§2: no longer `state.selectedLayerIndex` under the row-based UI,
// but the parameter's own meaning — "which position in recipe.markerLayers" — is unchanged).
// `effectiveSymmetryMask`/`effectiveRadialRepeatCount` are the layer's already-resolved values
// (ResolveEffectiveMarkerSymmetry, MarkerDragGesture_UI.h:66-77 — call it at the button call site, do
// not re-derive the bSymmetryUseGlobal ternary here). `distanceTolerance` is the caller's current
// `Params::MarkerSymmetryFixSettings::distanceTolerance` value (world units — a real,
// designer-editable PARAMS field, read by value here since this function never mutates it).
// `bOverwrite` selects skip vs overwrite mode per STEP107 §2.
MarkerSymmetryFixResult FixMarkerLayerSymmetry(std::vector<Params::MarkerInstanceGroup>& markers,
                                               const Params::Geometry& geometry, int layerIndex,
                                               int effectiveSymmetryMask,
                                               int effectiveRadialRepeatCount,
                                               float distanceTolerance, bool bOverwrite);

} // namespace Ui
} // namespace SanmapGen
