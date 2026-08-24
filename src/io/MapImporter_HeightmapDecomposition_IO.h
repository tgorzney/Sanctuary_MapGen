// MapImporter_HeightmapDecomposition_IO.h — MODULE-INTERNAL: STEP105's single-baked-layer contract
// for an imported heightmap. Split out of MapImporter_Fields_IO.cpp under the ARCH §1.5 file-size
// ceilings. Not part of the public Io::MapImporter surface (MapImporter_IO.h) —
// MapImporter_Fields_IO.cpp's LoadBakedFields is the only caller, from its own tail once the
// heightmap itself has loaded.
#pragma once
#include <string>
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
//      always-present base"), is minted into a new GeoLayer holding the loaded heightfield verbatim,
//      so NoiseBlendStage (STEP100) reproduces the loaded heightfield instead of silently discarding
//      it. STEP109: the new GeoLayer's name is derived from `sourceFileName` (the document's own
//      filename stem, underscores replaced with spaces — `DeriveLayerNameFromFileName`, the .cpp),
//      never from the document's own JSON `"name"` field.
//   2. RE-HYDRATION: `recipe.layerStack` already has content (e.g. re-opening a `.sanmap` SanGen
//      itself exported after this feature shipped). Every EXISTING bBaked layer with no
//      `bakedImagePath` (never an Import-RAW layer — STEP102) is re-derived verbatim from the
//      freshly-reloaded heightfield. Never mints a new layer here — a SanGen-authored map's own
//      recipe already reproduces the baked art once; synthesizing more on top would double-apply
//      the height. A `.sanmap` still carrying more than one such layer under one GeoLayer (a
//      STEP101-era per-stratum export, now stale) is flagged via `result.Warn(...)` rather than
//      migrated — see the .cpp for the reasoning.
// `sourceFileName` (STEP109) is the imported document's own filename STEM (never its JSON
// `"name"` field), computed once by the caller (`LoadBakedFields`, `MapImporter_Fields_IO.cpp`) via
// `std::filesystem::path(documentPath).stem().string()`. Only consumed by the FRESH-SYNTHESIS
// branch — the re-hydration branch never renames an existing, possibly hand-renamed, GeoLayer.
void DecomposeBakedHeightmapIntoLayers(Params::MapRecipe& recipe, Data::MapFields& fields,
                                       std::vector<Data::BakedLayerImage>& outBakedLayerImages,
                                       MapImportResult& result, const std::string& sourceFileName);

} // namespace Io
} // namespace SanmapGen
