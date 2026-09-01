// MapExporter_MarkerLink_IO.h — `recipe.markerLinks` -> the top-level `MarkerLinks` PascalCase
// array (ARCH §19.28/§19.30, DESIGN_MarkerLink_R1.md §3.3/§3.8). Layer: IO. New file pair, own
// header (not folded into MapExporter_Recipe_IO.h's shared declaration block): a brand-new tier
// gets its own PARAMS file AND its own IO file, mirroring MarkerLayerBundle's own precedent
// (DESIGN_MarkerLink_R1.md §3.8's file-homes ruling).
#pragma once
#include <nlohmann/json.hpp>

namespace SanmapGen {
namespace Params { struct MapRecipe; }
namespace Io {

// `MarkerLinks: [ N × { Identifier, Name, ColorOverrideEnabled, Color({r,g,b,a}) } ]`
// (ARCH §19.30). Additive, no SanGenVersion bump. The two merged `LinkIdentifier` back-reference
// fields on `MarkerLayerBundles[i]`/`MarkerGroups[i]` are built by MapExporter_Markers_IO.cpp
// (already owns both sections), NOT here.
nlohmann::ordered_json BuildMarkerLinksJson(const Params::MapRecipe& recipe);

} // namespace Io
} // namespace SanmapGen
