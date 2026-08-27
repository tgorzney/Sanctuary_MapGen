// MapImporter_MarkerLayerReconcile_IO.cpp — Io::ReconcileMarkerLayers, split into its own file
// (STEP115_MarkerPropDecalLayerReconciliationOnImport_IO) rather than added to
// MapImporter_Markers_IO.cpp, which is already 160 lines — already over the ARCH §1.5 hard-150
// ceiling with no documented exception. Mirrors this codebase's own precedent for exactly this
// situation (MapImporter_ParseDocument_IO.cpp was split out of MapImporter_IO.cpp under STEP35 when
// it grew too large). Declared in MapImporter_Recipe_IO.h beside ReadMarkerGroupsJson/ReadMarkersJson.
//
// A real, non-SanGen-authored `.sanmap` never carries `MarkerGroups` (SanGen-invented, no format
// precedent, see MapImporter_Markers_IO.cpp's own header comment) — every transform's layerIndex
// silently defaults to 0, pointing at a layer that does not exist, and the Manual Marker Layers tab
// has nothing to show. Synthesizes one MarkerInstanceLayer per `outRecipe.markers` GROUP entry (the
// marker TYPE, e.g. "Spawn"/"Alloys"), only when markerLayers is empty AND at least one marker group
// exists. A file that already carries MarkerGroups (even a short/partial one covering fewer groups
// than exist) is left exactly as read — PARTIAL coverage is explicitly out of scope for this ticket,
// this guard fires on EMPTY only. Every synthesized layer is struct-default (white, unlocked, no
// grid snap, default symmetry) — no distinguishing color invented (white-as-"unset" is this data
// model's existing convention).
#include "MapImporter_IO.h"
#include "../params/MapRecipe_PARAMS.h"

namespace SanmapGen {
namespace Io {

void ReconcileMarkerLayers(Params::MapRecipe& outRecipe, MapImportResult& result) {
    if (!outRecipe.markerLayers.empty() || outRecipe.markers.empty()) return;
    for (Params::MarkerInstanceGroup& group : outRecipe.markers) {
        Params::MarkerInstanceLayer layer;
        layer.name = group.name;
        // NEW — human's own bug report: an unset markerTypeName meant a synthesized layer never
        // matched any Type-section's `markerTypeName == typeName` test (MarkersTab_UI.cpp), so every
        // freshly-imported instance fell through to the flat "Instances" fallback list instead of a
        // real Layer. Canonicalized (CanonicalMarkerTypeSectionName) so a real map's plural group
        // name ("Alloys") still resolves to the singular Type-section ("Alloy") this field is
        // compared against everywhere else.
        layer.markerTypeName = Params::CanonicalMarkerTypeSectionName(group.name);
        const int newLayerIndex = static_cast<int>(outRecipe.markerLayers.size());
        layer.layerId = newLayerIndex;   // same sequential convention as ReadMarkerGroupsJson's own
                                          // legacy-backfill default (MapImporter_Markers_IO.cpp:121).
        outRecipe.markerLayers.push_back(layer);
        for (Params::MarkerTransform& transform : group.transforms)
            transform.layerIndex = newLayerIndex;
    }
    // One aggregate Warn per import, not per synthesized layer: this is a single whole-document
    // structural fact ("no MarkerGroups section"), not an independent per-instance correctness event
    // like ClampMarkerLayerIndex's per-transform warn (MapImporter_Markers_IO.cpp) — flooding the log
    // once per marker type on a map with many types would bury the signal.
    result.Warn("No MarkerGroups section present; synthesized "
               + std::to_string(outRecipe.markerLayers.size())
               + " marker layer(s) from the existing marker type(s) so the Manual Marker Layers tab"
                 " has something to show.");
}

} // namespace Io
} // namespace SanmapGen
