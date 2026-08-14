// MapRecipe_PARAMS.h — the complete editable settings for one map (the recipe).
// Layer: PARAMS. This is exactly what `mapGeneratorData` serializes and what the
// deterministic shared-generation mode transmits (settings + seed regenerate the map).
// Aggregates the geometry, layer stack, placement rules, and water. Excludes execution
// concerns (dispatch/backend) — those are not reproducible-recipe content.
#pragma once
#include <vector>
#include "Geometry_PARAMS.h"
#include "LayerStack_PARAMS.h"
#include "MarkerRule_PARAMS.h"
#include "ScatterRule_PARAMS.h"
#include "Water_PARAMS.h"

namespace SanmapGen {
namespace Params {

struct MapRecipe {
    Geometry               geometry;
    LayerStack             layerStack;
    std::vector<MarkerRule> markerRules;
    std::vector<PropRule>   propRules;
    std::vector<DecalRule>  decalRules;
    Water                   water;
    int                     globalSymmetryMask = 0;

    bool IsValid() const { return geometry.IsValid(); }
};

} // namespace Params
} // namespace SanmapGen
