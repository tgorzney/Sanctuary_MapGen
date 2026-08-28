// MapImporter_DecalGroups_IO.cpp — the top-level `DecalGroups` array -> `recipe.decalLayers`.
// Layer: IO. Split out of MapImporter_Decals_IO.cpp (ARCH §20), mirroring
// MapImporter_PropGroups_IO.cpp's own split-out-of-Props precedent for the identical file-size
// reason. `ReadDecalGroupsJson` MUST still run before `ReadDecalsJson`
// (MapImporter_Decals_IO.cpp's own header comment, unchanged).
#include "JsonPrimitives_IO.h"
#include "../params/MapRecipe_PARAMS.h"
#include "../params/Symmetry_PARAMS.h"   // radialSymmetryRepeatCountMinimum/Maximum

namespace SanmapGen {
namespace Io {

// `DecalGroups` — a plain array walk, full field parity with `ReadMarkerGroupsJson`
// (MapImporter_MarkerGroups_IO.cpp) EXCEPT no type-tag field (ARCH §20 — Decals has exactly one
// Type Section).
void ReadDecalGroupsJson(const nlohmann::json& document, Params::MapRecipe& outRecipe) {
    if (!document.contains("DecalGroups") || !document["DecalGroups"].is_array()) return;
    outRecipe.decalLayers.clear();
    for (const nlohmann::json& layerJson : document["DecalGroups"]) {
        Params::DecalInstanceLayer layer;
        layer.layerId = static_cast<int>(outRecipe.decalLayers.size());   // legacy-backfill default
        if (layerJson.is_object()) {
            ReadJsonText(layerJson, "Name", layer.name);
            if (layerJson.contains("Color") && layerJson["Color"].is_object()) {
                const nlohmann::json& color = layerJson["Color"];
                ReadJsonFloat(color, "r", layer.color[0]);
                ReadJsonFloat(color, "g", layer.color[1]);
                ReadJsonFloat(color, "b", layer.color[2]);
                ReadJsonFloat(color, "a", layer.color[3]);
            }
            ReadJsonFloat(layerJson, "IconScale", layer.iconScale);
            ReadJsonInteger(layerJson, "Id", layer.layerId);
            ReadJsonBoolean(layerJson, "SymmetryUseGlobal", layer.symmetry.bSymmetryUseGlobal);
            ReadJsonInteger(layerJson, "SymmetryMask", layer.symmetry.symmetryMask);
            ReadJsonIntegerClamped(layerJson, "RadialSymmetryRepeatCount",
                                  Params::radialSymmetryRepeatCountMinimum,
                                  Params::radialSymmetryRepeatCountMaximum,
                                  layer.symmetry.radialSymmetryRepeatCount);
            ReadJsonBoolean(layerJson, "Locked", layer.bLocked);
            ReadJsonBoolean(layerJson, "Hidden", layer.bHidden);
            ReadJsonBoolean(layerJson, "GridSnapEnabled", layer.bGridSnapEnabled);
            ReadJsonFloat(layerJson, "GridSnapSizeWorldUnits", layer.gridSnapSizeWorldUnits);
            ReadJsonBoolean(layerJson, "ColorOverrideEnabled", layer.bColorOverrideEnabled);
            ReadJsonBoolean(layerJson, "SymmetryEnabled", layer.bSymmetryEnabled);
            ReadJsonInteger(layerJson, "ParentBundleIdentifier", layer.parentBundleIdentifier);
        }
        outRecipe.decalLayers.push_back(layer);
    }
}

} // namespace Io
} // namespace SanmapGen
