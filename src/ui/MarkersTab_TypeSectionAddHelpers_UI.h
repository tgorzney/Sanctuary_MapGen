// MarkersTab_TypeSectionAddHelpers_UI.h — STEP254: the 3 pure-mechanics helpers
// MarkersTab_TypeSectionAddActions_UI.cpp's own 4 button handlers share, split into their own
// sibling file when the parent file's own preserved STEP137/STEP152/STEP235 rationale comments
// pushed it over ARCH_01_05's 150-line hard ceiling (this ticket's own §1 fallback, re-measured
// rather than guessed). Not anonymous-namespace-local: `MarkersTab_TypeSectionAddActions_UI.cpp` is
// their only caller, but a cross-translation-unit helper needs a real declaration somewhere.
#pragma once
#include <string>
#include <vector>

namespace SanmapGen {
namespace Params {
struct Geometry;
struct MarkerInstanceGroup;
struct MarkerLayerBundle;
} // namespace Params
namespace Ui {

// The `Params::MarkerInstanceGroup` (the legacy Type-keyed roster `recipe.markers` still actually
// stores authored transforms in — `MarkerInstanceLayer` above it is metadata-only, ARCH §19.13) whose
// `name` matches this Type-section, minting one on first use. Real map data (a human-confirmed live
// re-import of an existing .sanmap) already stores its Alloy/Spawn markers in exactly this group, so
// "Add Instance" appends to the SAME roster that data already occupies rather than a parallel one.
// `name` arrives as a canonical Type-section name (e.g. "Alloy"). A real imported map's own group
// may be named the PLURAL alias ("Alloys") instead — CanonicalMarkerTypeSectionName folds both
// forms together so "+ Instance" appends to that SAME existing roster rather than minting a second,
// empty "Alloy" group that splits the data the comment above already promises stays unified.
Params::MarkerInstanceGroup& FindOrCreateMarkerInstanceGroupByName(
        std::vector<Params::MarkerInstanceGroup>& markers, const std::string& name);

// STEP137 — a new manual instance's default X/Z: the map's own center (human's own instruction —
// the struct's own (0,0,0) default sits at the map's CORNER under SanGen's corner-origin world-space
// convention, confirmed by Placement_Fields_PROC.cpp's own `mapCenter = (vertexSize - 1) * 0.5` and
// MarkerSymmetryDetection_PIPELINE.cpp's own `worldSize = mapSize * worldUnitsPerGenerationCell` extent).
float MapCenterWorldUnits(const Params::Geometry& geometry);

// STEP138/STEP139 — a newly-added Layer's OR Group's own `parentBundleIdentifier`: the currently-
// selected Group (Bundle), when one typed to THIS Type-section is selected (human's own instruction
// — "+ Layer"/"+ Group" should add under the selected Group, and Groups stay nestable); else root
// ("the base section"), the existing -1 convention every Bundle/Layer already carries. Guards on
// `markerTypeName` for the same cross-Type-section-selection reason `ResolveAddInstanceLayerIndex`
// above does. Safe to reuse for a brand-new Group too (no cycle check needed — a NEW node can never
// already be its own ancestor; `WouldReparentMarkerLayerBundleCreateCycle` only guards RE-parenting
// an EXISTING node, a different, still-untouched code path).
int ResolveSelectedParentBundleIdentifier(const std::vector<Params::MarkerLayerBundle>& bundles,
                                          int selectedBundleIdentifier, const std::string& typeName);

} // namespace Ui
} // namespace SanmapGen
