// MapImporter_HeightmapDecomposition_IO.h — MODULE-INTERNAL: STEP105's single-baked-layer contract
// for an imported heightmap. Split out of MapImporter_Fields_IO.cpp under the ARCH §1.5 file-size
// ceilings. Not part of the public Io::MapImporter surface (MapImporter_IO.h) —
// MapImporter_Fields_IO.cpp's LoadBakedFields is the only caller, from its own tail once the
// heightmap itself has loaded.
#pragma once
#include <vector>

namespace SanmapGen {
namespace Data { class MapFields; struct BakedLayerImage; }
namespace Params { struct MapRecipe; }
namespace Io {

struct MapImportResult;

// This is the ticket that fixes the reported bug end to end: STEP99/100 made the "a baked layer
// reads a frozen image" mechanism exist; this makes import actually populate it. STEP105 revises
// STEP101's per-stratum split down to a single verbatim layer -- the source heightmap is a genuine
// single-channel field, never independently editable per material, so decomposing it algebraically
// was a fiction; per-stratum mask ART is instead fed to `Data::StratumArt::importedMask` /
// `Params::Stratum::importedMaskMode` (MapImporter_Fields_IO.cpp), a wholly separate concern. Two
// branches, one shared shape:
//   1. FRESH SYNTHESIS: `recipe.layerStack` is empty — a genuine externally-authored `.sanmap` with
//      no SanGen HeightmapStack section (the reported bug's exact scenario). Exactly ONE baked
//      `Params::Layer`, at its default `stratumIndex == 0` (`LAYER_SYSTEM_SPEC`'s "Stratum 0 = the
//      always-present base"), is minted into a new "Imported Bake" GeoLayer holding the loaded
//      heightfield verbatim, so NoiseBlendStage (STEP100) reproduces the loaded heightfield instead
//      of silently discarding it.
//   2. RE-HYDRATION: `recipe.layerStack` already has content (e.g. re-opening a `.sanmap` SanGen
//      itself exported after this feature shipped). Every EXISTING bBaked layer with no
//      `bakedImagePath` (never an Import-RAW layer — STEP102) is re-derived verbatim from the
//      freshly-reloaded heightfield. Never mints a new layer here — a SanGen-authored map's own
//      recipe already reproduces the baked art once; synthesizing more on top would double-apply
//      the height. A `.sanmap` still carrying more than one such layer under one GeoLayer (a
//      STEP101-era per-stratum export, now stale) is flagged via `result.Warn(...)` rather than
//      migrated — see the .cpp for the reasoning.
void DecomposeBakedHeightmapIntoLayers(Params::MapRecipe& recipe, Data::MapFields& fields,
                                       std::vector<Data::BakedLayerImage>& outBakedLayerImages,
                                       MapImportResult& result);

} // namespace Io
} // namespace SanmapGen
