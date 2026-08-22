// MarkersTab_Placed_UI.h — the placed-marker list: every marker the Placement stage resolved,
// virtualized. Layer: UI. Accuracy class: Visual. TAB_REBUILD_PLAN "§ Markers · Placed markers".
//
// This is the shared VirtualList's reason to exist: a symmetric 4k map resolves tens of thousands
// of markers, and the list costs O(visible rows) per frame, never O(instanceCount). The buffer is
// `Data::PlacementInstances` — a real struct-of-arrays — so the rows are addressed by INDEX and
// nothing is copied into a per-item list (UI_FRAMEWORK_SPEC item 6).
//
// SCOPE NOTE (ARCH §8.4 — reported, not invented): the list is READ-ONLY. `Data::PlacementInstances`
// is the Placement stage's COMPUTED OUTPUT, and every DATA field has exactly one writing stage
// (Constitution §1) — a tab that edited it would be the UI simulating. The hand-authored roster
// (alias, position, spawn->army assignment, delete) is separate RECIPE content with a real PARAMS
// home — `Params::MarkerInstanceGroup`/`MarkerTransform` (ENTITY_AUTHORING_PARAMS_SPEC) — edited
// by `MarkersTab_Manual_UI.h`'s `DrawManualMarkers` (STEP49), a sibling block to this one. This
// list stays exactly as it was: unfiltered, previewing every procedurally-resolved marker
// regardless of manual-roster membership, same posture `PropsTab_Manual_UI.h` SCOPE NOTE 2 uses
// for its own read-only transform preview.
#pragma once
#include "Section_UI.h"

namespace SanmapGen {
namespace Data { class PlacementInstances; }
namespace Ui {

struct MarkersPlacedListState {
    SectionState section;
    float rowHeight  = 20.0f;    // must be the TRUE row height: the clipper scrolls with it
    float listHeight = 180.0f;
    int   selectedInstanceIndex = -1;
};

// The row a click may legally highlight: inside the buffer, or -1 for "nothing picked" — how a
// selection left over from a shorter previous generation is corrected rather than read off the
// end (Constitution §6).
inline int ResolvedPlacedMarkerSelection(int selectedInstanceIndex, int instanceCount) {
    if (instanceCount <= 0) return -1;
    if (selectedInstanceIndex < 0 || selectedInstanceIndex >= instanceCount) return -1;
    return selectedInstanceIndex;
}

// `placedMarkers` is nullable: before the first generation there is no buffer, and the section
// says so rather than drawing an empty frame.
void DrawPlacedMarkerList(const Data::PlacementInstances* placedMarkers,
                          MarkersPlacedListState& state);

} // namespace Ui
} // namespace SanmapGen
