// MapImporter_HeightmapDecomposition_IO.cpp — see the header for the split rationale and the
// two-branch contract. Layer: IO.
#include "MapImporter_HeightmapDecomposition_IO.h"
#include "MapImporter_IO.h"
#include "../data/BakedLayerImage_DATA.h"
#include "../data/MapFields_DATA.h"
#include "../params/LayerStack_PARAMS.h"
#include "../params/MapRecipe_PARAMS.h"
#include <string>
#include <utility>

namespace SanmapGen {
namespace Io {
namespace {

// STEP109: the fresh-synthesis GeoLayer's own name, derived from the imported document's filename
// stem rather than a hardcoded literal or the document's own JSON `"name"` field — underscores
// become spaces, and any leading/trailing space left by a stem that started/ended with '_' is
// trimmed. An empty/whitespace-only stem (or no stem at all) falls back to "Imported Bake" so a
// GeoLayer's name is never empty.
std::string DeriveLayerNameFromFileName(const std::string& fileStem) {
    std::string name = fileStem;
    for (char& character : name) if (character == '_') character = ' ';
    const std::size_t first = name.find_first_not_of(' ');
    if (first == std::string::npos) return "Imported Bake";
    const std::size_t last = name.find_last_not_of(' ');
    return name.substr(first, last - first + 1);
}

} // namespace

void DecomposeBakedHeightmapIntoLayers(Params::MapRecipe& recipe, Data::MapFields& fields,
                                       std::vector<Data::BakedLayerImage>& outBakedLayerImages,
                                       MapImportResult& result, const std::string& sourceFileName) {
    const int vertexSize = fields.VertexSize();
    if (vertexSize < 2) return;

    if (!recipe.layerStack.geoLayers.empty()) {                 // RE-HYDRATION: unchanged gate
        for (Params::GeoLayer& group : recipe.layerStack.geoLayers) {
            // Migration hazard (not fixed here, see the header): a .sanmap already exported under
            // STEP101's per-stratum scheme has MULTIPLE bBaked/empty-bakedImagePath layers under one
            // GeoLayer. Every one of them now re-derives as the FULL heightfield verbatim, and
            // GeoLayerMode::Material with blendMode == Add sums its layers -- so re-opening such a
            // file would double-sum. Flagged, not migrated (STEP101 shipped hours before this ticket
            // with no real authored content yet); route a real fix to the IO Architecture Expert if
            // a genuine stale file ever surfaces.
            int matchingLayerCount = 0;
            for (const Params::Layer& layer : group.layers)
                if (layer.bBaked && layer.bakedImagePath.empty()) ++matchingLayerCount;
            if (matchingLayerCount > 1)
                result.Warn("GeoLayer \"" + group.name + "\" has " + std::to_string(matchingLayerCount)
                    + " baked layer(s) with no stored image -- this looks like a .sanmap exported "
                      "under the old per-stratum decomposition scheme (STEP101). Each will now "
                      "re-derive as the full heightfield verbatim, which may double-sum under "
                      "Material/Add blending.");
            for (Params::Layer& layer : group.layers) {
                if (!layer.bBaked || !layer.bakedImagePath.empty()) continue;
                Data::BakedLayerImage bakedImage;
                bakedImage.layerIdentifier = layer.layerIdentifier;
                bakedImage.image = fields.heightfield;           // STEP105: verbatim, no per-stratum mask
                outBakedLayerImages.push_back(std::move(bakedImage));
            }
        }
        return;
    }

    Params::GeoLayer importedGroup;
    importedGroup.name      = DeriveLayerNameFromFileName(sourceFileName);   // STEP109
    importedGroup.mode      = Params::GeoLayerMode::Material;
    importedGroup.blendMode = Params::HeightBlendMode::Add;

    Params::Layer layer;                                         // stratumIndex stays its default (0)
    layer.bBaked          = true;
    layer.layerIdentifier = Params::NextLayerIdentifier(recipe.layerStack);
    importedGroup.layers.push_back(layer);

    Data::BakedLayerImage bakedImage;
    bakedImage.layerIdentifier = layer.layerIdentifier;
    bakedImage.image = fields.heightfield;
    outBakedLayerImages.push_back(std::move(bakedImage));

    recipe.layerStack.geoLayers.push_back(std::move(importedGroup));
}

} // namespace Io
} // namespace SanmapGen
