// MapRecipe_PARAMS.h — the complete editable settings for one map (the recipe).
// Layer: PARAMS. This is exactly what `mapGeneratorData` serializes and what the
// deterministic shared-generation mode transmits (settings + seed regenerate the map).
// Aggregates the geometry, layer stack, strata, placement rules, and water. Excludes execution
// concerns (dispatch/backend) — those are not reproducible-recipe content.
#pragma once
#include <vector>
#include "Army_PARAMS.h"
#include "Geometry_PARAMS.h"
#include "LayerStack_PARAMS.h"
#include "MapArea_PARAMS.h"
#include "MarkerRule_PARAMS.h"
#include "ScatterRule_PARAMS.h"
#include "Stratum_PARAMS.h"
#include "Symmetry_PARAMS.h"
#include "Water_PARAMS.h"

namespace SanmapGen {
namespace Params {

struct MapRecipe {
    Geometry               geometry;
    LayerStack             layerStack;
    // The ONE per-stratum settings array (ARCH §7.1). A stage reads a span of it; no stage
    // keeps a private per-stratum array. Shorter than MapFields::stratumCount is legal —
    // strata past the end run on their defaults.
    std::vector<Stratum>    strata;
    std::vector<MarkerRule> markerRules;
    std::vector<PropRule>   propRules;
    std::vector<DecalRule>  decalRules;
    std::vector<UnitRule>   unitRules;
    Water                   water;
    int                     globalSymmetryMask = SymmetryAxis::None;
    // Hand-placed, pass-through entity data (ENTITY_AUTHORING_PARAMS_SPEC) — round-trip fidelity
    // through the `.sanmap` `armies`/`areas` dictionaries is their entire purpose; no PROC stage
    // computes or reinterprets them.
    std::vector<Army>       armies;
    std::vector<MapArea>    areas;

    bool IsValid() const { return geometry.IsValid(); }
};

} // namespace Params
} // namespace SanmapGen
