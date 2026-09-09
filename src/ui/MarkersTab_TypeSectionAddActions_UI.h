// MarkersTab_TypeSectionAddActions_UI.h — STEP254: the Type-section header's own 4 "+Instance"/
// "+Group"/"+Layer(Manual)"/"+Layer(Procedural)" button handlers, relocated out of
// MarkersTab_UI.cpp's file-size-ceiling split. Each handler is its own named function, not one
// generic parameterized dispatcher (ARCH_19_MarkerLayerBundle.md §19.2 — domain-touching logic gets
// its own per-concern function, confirmed applicable here: each handler constructs a different
// concrete `Params::` type and touches different fields). The 3 pure-mechanics helpers these
// handlers share (FindOrCreateMarkerInstanceGroupByName/MapCenterWorldUnits/
// ResolveSelectedParentBundleIdentifier) live in the sibling MarkersTab_TypeSectionAddHelpers_UI.h —
// this ticket's own §1 fallback split, re-measured once this file's own preserved rationale comments
// alone pushed the .cpp over the 150-line hard ceiling.
#pragma once
#include <string>

namespace SanmapGen {
namespace Params { struct MapRecipe; }
namespace Ui {

struct MarkersTabState;

void ApplyAddInstanceButtonAction(Params::MapRecipe& recipe, MarkersTabState& state,
                                  const std::string& typeName);
void ApplyAddGroupButtonAction(Params::MapRecipe& recipe, MarkersTabState& state,
                               const std::string& typeName);
void ApplyAddManualLayerButtonAction(Params::MapRecipe& recipe, MarkersTabState& state,
                                     const std::string& typeName);
// Returns true unconditionally when called — mirrors DrawMarkersTab's own former inline
// `bRecipeMoved = true` (a fresh Rule Layer, seeded with one default rule, is immediately
// pipeline-visible). Caller ORs this into its own accumulated bRecipeMoved.
bool ApplyAddProceduralLayerButtonAction(Params::MapRecipe& recipe, MarkersTabState& state,
                                         const std::string& typeName);

} // namespace Ui
} // namespace SanmapGen
