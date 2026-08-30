// MarkerSelectionHighlight_UI.h — the static selection-highlight computation (ARCH §19.19). Pure,
// imgui-free, testable with no window — same posture as MarkerDragGesture_UI.h/
// MarkerOrbitCorrespondence_UI.h. Deliberately NOT MarkerOrbitCorrespondence_UI.h's cross-frame
// matcher: this is a fresh, one-shot, discard-every-frame query, with none of that matcher's
// orbit-grows/shrinks-across-frames drift problem to solve. Calls the existing PIPELINE query
// Pipeline::BuildWorldSymmetryOrbit — UI -> PIPELINE, the already-legal query passthrough (ARCH
// §16.3) — no new PIPELINE surface.
#pragma once
#include <vector>
#include "../params/Geometry_PARAMS.h"
#include "../params/MarkerInstance_PARAMS.h"

namespace SanmapGen {
namespace Ui {

// Locates `selectedInstanceIdentifier` (ARCH §19.16) by linear scan across `markers`, resolves its
// effective symmetry (ResolveEffectiveMarkerSymmetry), queries Pipeline::BuildWorldSymmetryOrbit, and
// nearest-matches (within `distanceTolerance`) every orbit point beyond slot 0 against sibling
// transforms in the SAME MarkerInstanceGroup. Returns every matched instanceIdentifier, always
// including the selected instance itself as the first element. Returns empty for
// selectedInstanceIdentifier == -1 (no selection) or a stale identifier (deleted since selection —
// Constitution §6, never a crash). orbitCount <= 1 (no siblings, including the common "never
// dragged, symmetryGroupIdentifier == 0" case) returns just the selected instance, by construction —
// no special-casing needed (ARCH §19.19's own point: position-driven orbit matching subsumes it).
std::vector<int> ComputeManualMarkerSelectionHighlight(
    const std::vector<Params::MarkerInstanceGroup>& markers,
    const std::vector<Params::MarkerInstanceLayer>& markerLayers,
    const Params::Geometry& geometry, int globalSymmetryMask, int globalRadialRepeatCount,
    float distanceTolerance, int selectedInstanceIdentifier);

// STEP231 — the multi-select counterpart: unions ComputeManualMarkerSelectionHighlight's own
// single-instance result (the selected instance plus its own orbit siblings) across every id in
// `selectedInstanceIdentifiers`, de-duplicated (a later id's own orbit can legitimately re-discover an
// EARLIER id's own siblings — e.g. two siblings of the same symmetric pair both individually
// selected). Delegates entirely to the existing single-instance primitive, per id, in order, exactly
// once each — no duplicated matching/orbit logic (mirrors STEP230's own
// "loop over the existing single-key primitive" precedent, ToggleEachInSelectionSet).
std::vector<int> ComputeManualMarkerMultiSelectionHighlight(
    const std::vector<Params::MarkerInstanceGroup>& markers,
    const std::vector<Params::MarkerInstanceLayer>& markerLayers,
    const Params::Geometry& geometry, int globalSymmetryMask, int globalRadialRepeatCount,
    float distanceTolerance, const std::vector<int>& selectedInstanceIdentifiers);

} // namespace Ui
} // namespace SanmapGen
