// MapImporter_HeightmapDecomposition_IO.h — MODULE-INTERNAL: STEP101's per-stratum decomposition of
// an imported baked heightmap into Params::Layer entries + Data::BakedLayerImage pixels. Split out
// of MapImporter_Fields_IO.cpp under the ARCH §1.5 file-size ceilings. Not part of the public
// Io::MapImporter surface (MapImporter_IO.h) — MapImporter_Fields_IO.cpp's LoadBakedFields is the
// only caller, from its own tail once the heightmap itself has loaded.
#pragma once
#include <vector>

namespace SanmapGen {
namespace Data { class MapFields; struct BakedLayerImage; }
namespace Params { struct MapRecipe; }
namespace Io {

// This is the ticket that fixes the reported bug end to end: STEP99/100 made the "a baked layer
// reads a frozen image" mechanism exist; this makes import actually populate it. Two branches, one
// shared per-stratum formula (MapImporter_HeightmapDecomposition_IO.cpp's own ComputeStratumBakedImage):
//   1. FRESH SYNTHESIS: `recipe.layerStack` is empty — a genuine externally-authored `.sanmap` with
//      no SanGen HeightmapStack section (the reported bug's exact scenario). One baked Params::Layer
//      per non-empty stratum (stratum 0 always included) is minted into a new "Imported Bake"
//      GeoLayer, so NoiseBlendStage (STEP100) reproduces the loaded heightfield instead of silently
//      discarding it.
//   2. RE-HYDRATION: `recipe.layerStack` already has content (e.g. re-opening a `.sanmap` SanGen
//      itself exported after this feature shipped). Every EXISTING bBaked layer with no
//      `bakedImagePath` (STEP101's own decomposed layers, never an Import-RAW layer — STEP102) is
//      re-derived from the freshly-reloaded fields, keyed by its own stratumIndex/layerIdentifier.
//      Never mints a new layer here — a SanGen-authored map's own recipe already reproduces the
//      baked art once; synthesizing more on top would double-apply the height.
void DecomposeBakedHeightmapIntoLayers(Params::MapRecipe& recipe, Data::MapFields& fields,
                                       std::vector<Data::BakedLayerImage>& outBakedLayerImages);

} // namespace Io
} // namespace SanmapGen
