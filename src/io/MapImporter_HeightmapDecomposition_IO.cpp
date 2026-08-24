// MapImporter_HeightmapDecomposition_IO.cpp — see the header for the split rationale and the
// two-branch contract. Layer: IO.
#include "MapImporter_HeightmapDecomposition_IO.h"
#include "../data/BakedLayerImage_DATA.h"
#include "../data/MapFields_DATA.h"
#include "../params/LayerStack_PARAMS.h"
#include "../params/MapRecipe_PARAMS.h"
#include <utility>

namespace SanmapGen {
namespace Io {
namespace {

// The shared per-stratum formula: stratum `stratum`'s baked contribution is the loaded heightfield
// masked by its `materialProportions` weight. Stratum 0 (the implicit, unmasked base) is whatever
// is NOT covered by strata 1-8, floored at zero (never negative from a mask sum past 1.0).
// `outAnyContribution` tells the caller whether any cell actually reads this stratum, so a
// synthesized layer that would be uniformly zero can be skipped (stratum 0 excepted — it is
// mandatory, matching LAYER_SYSTEM_SPEC's "Stratum 0 = the always-present base").
Data::FloatField ComputeStratumBakedImage(const Data::MapFields& fields, int stratum, int vertexSize,
                                          bool& outAnyContribution) {
    Data::FloatField image;
    image.Resize(vertexSize, vertexSize, 0.0f);
    outAnyContribution = false;
    for (int y = 0; y < vertexSize; ++y) {
        for (int x = 0; x < vertexSize; ++x) {
            float weight;
            if (stratum == 0) {
                float coveredByOthers = 0.0f;
                for (int other = 1; other < Data::MapFields::stratumCount; ++other)
                    coveredByOthers += fields.materialProportions[other].Get(x, y);
                weight = 1.0f - coveredByOthers;
                if (weight < 0.0f) weight = 0.0f;
            } else {
                weight = fields.materialProportions[stratum].Get(x, y);
            }
            if (weight > 0.0001f) outAnyContribution = true;
            image.Set(x, y, fields.heightfield.Get(x, y) * weight);
        }
    }
    return image;
}

} // namespace

void DecomposeBakedHeightmapIntoLayers(Params::MapRecipe& recipe, Data::MapFields& fields,
                                       std::vector<Data::BakedLayerImage>& outBakedLayerImages) {
    const int vertexSize = fields.VertexSize();
    if (vertexSize < 2) return;

    if (!recipe.layerStack.geoLayers.empty()) {
        for (Params::GeoLayer& group : recipe.layerStack.geoLayers) {
            for (Params::Layer& layer : group.layers) {
                if (!layer.bBaked || !layer.bakedImagePath.empty()) continue;
                bool bAnyContribution = false;
                Data::BakedLayerImage bakedImage;
                bakedImage.layerIdentifier = layer.layerIdentifier;
                bakedImage.image = ComputeStratumBakedImage(fields, layer.stratumIndex, vertexSize,
                                                             bAnyContribution);
                outBakedLayerImages.push_back(std::move(bakedImage));
            }
        }
        return;
    }

    Params::GeoLayer importedGroup;
    importedGroup.name      = "Imported Bake";
    importedGroup.mode      = Params::GeoLayerMode::Material;
    importedGroup.blendMode = Params::HeightBlendMode::Add;

    int nextIdentifier = Params::NextLayerIdentifier(recipe.layerStack);   // 0 for a fresh recipe
    for (int stratum = 0; stratum < Data::MapFields::stratumCount; ++stratum) {
        bool bAnyContribution = false;
        Data::FloatField image = ComputeStratumBakedImage(fields, stratum, vertexSize, bAnyContribution);
        if (stratum != 0 && !bAnyContribution) continue;   // skip empty strata -- stratum 0 is mandatory

        Params::Layer layer;                  // every OTHER field stays default -- that's what makes
        layer.stratumIndex   = stratum;       // ReshapeLayerValue an identity pass-through (STEP100)
        layer.bBaked          = true;
        layer.layerIdentifier = nextIdentifier++;

        Data::BakedLayerImage bakedImage;
        bakedImage.layerIdentifier = layer.layerIdentifier;
        bakedImage.image = std::move(image);

        importedGroup.layers.push_back(layer);
        outBakedLayerImages.push_back(std::move(bakedImage));
    }
    recipe.layerStack.geoLayers.push_back(std::move(importedGroup));
}

} // namespace Io
} // namespace SanmapGen
